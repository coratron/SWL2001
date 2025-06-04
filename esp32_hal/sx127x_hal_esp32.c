/*!
 * \file      sx127x_hal_esp32.c
 *
 * \brief     ESP32 SX127x HAL implementation
 *
 * \copyright Copyright (c) 2023
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sx127x_hal.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS ----------------------------------------------------------
 */

#define TAG "SX127X_HAL"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

// SPI Configuration
#ifndef CONFIG_LBM_SPI_HOST
    #define CONFIG_LBM_SPI_HOST HSPI_HOST
#endif

#ifndef CONFIG_LBM_SPI_FREQUENCY_HZ
    #define CONFIG_LBM_SPI_FREQUENCY_HZ 8000000
#endif

// Default pin configuration
#ifndef CONFIG_LBM_RADIO_NSS_PIN
    #define CONFIG_LBM_RADIO_NSS_PIN 5
#endif

#ifndef CONFIG_LBM_RADIO_RESET_PIN
    #define CONFIG_LBM_RADIO_RESET_PIN -1
#endif

#ifndef CONFIG_LBM_RADIO_DIO0_PIN
    #define CONFIG_LBM_RADIO_DIO0_PIN 12
#endif

#ifndef CONFIG_LBM_RADIO_DIO1_PIN
    #define CONFIG_LBM_RADIO_DIO1_PIN 13
#endif

#ifndef CONFIG_LBM_RADIO_DIO2_PIN
    #define CONFIG_LBM_RADIO_DIO2_PIN -1
#endif

#ifndef CONFIG_LBM_SPI_MISO_PIN
    #define CONFIG_LBM_SPI_MISO_PIN 19
#endif

#ifndef CONFIG_LBM_SPI_MOSI_PIN
    #define CONFIG_LBM_SPI_MOSI_PIN 23
#endif

#ifndef CONFIG_LBM_SPI_CLK_PIN
    #define CONFIG_LBM_SPI_CLK_PIN 18
#endif

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static spi_device_handle_t spi_device = NULL;
static bool hal_initialized = false;
static esp_timer_handle_t rx_timer_handle = NULL;

// Static variables for timer callback handling
static void (*timer_callback)(void* context) = NULL;
static void* timer_context = NULL;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

static esp_err_t sx127x_spi_init(void);
static void rx_timer_callback(void* arg);
static void IRAM_ATTR dio_isr_handler(void* arg);

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

sx127x_hal_status_t sx127x_hal_write(const sx127x_t* radio, const uint16_t address, const uint8_t* data, const uint16_t data_length)
{
    if(!hal_initialized)
    {
        if(sx127x_spi_init() != ESP_OK)
        {
            return SX127X_HAL_STATUS_ERROR;
        }
    }

    esp_err_t ret;
    spi_transaction_t trans = {0};

    // Prepare the command byte (write operation, address)
    uint8_t cmd = address | 0x80;  // Set MSB for write operation

    // Create buffer with command + data
    uint8_t* tx_buffer = malloc(1 + data_length);
    if(tx_buffer == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate TX buffer");
        return SX127X_HAL_STATUS_ERROR;
    }

    tx_buffer[0] = cmd;
    memcpy(&tx_buffer[1], data, data_length);

    trans.length = (1 + data_length) * 8;  // Total length in bits
    trans.tx_buffer = tx_buffer;

    ret = spi_device_transmit(spi_device, &trans);

    free(tx_buffer);

    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI write failed: %s", esp_err_to_name(ret));
        return SX127X_HAL_STATUS_ERROR;
    }

    return SX127X_HAL_STATUS_OK;
}

sx127x_hal_status_t sx127x_hal_read(const sx127x_t* radio, const uint16_t address, uint8_t* data, const uint16_t data_length)
{
    if(!hal_initialized)
    {
        if(sx127x_spi_init() != ESP_OK)
        {
            return SX127X_HAL_STATUS_ERROR;
        }
    }

    esp_err_t ret;
    spi_transaction_t trans = {0};

    // Prepare the command byte (read operation, address)
    uint8_t cmd = address & 0x7F;  // Clear MSB for read operation

    // Create buffers
    uint8_t* tx_buffer = malloc(1 + data_length);
    uint8_t* rx_buffer = malloc(1 + data_length);

    if(tx_buffer == NULL || rx_buffer == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate buffers");
        if(tx_buffer)
            free(tx_buffer);
        if(rx_buffer)
            free(rx_buffer);
        return SX127X_HAL_STATUS_ERROR;
    }

    tx_buffer[0] = cmd;
    memset(&tx_buffer[1], 0x00, data_length);  // Send dummy bytes for read

    trans.length = (1 + data_length) * 8;  // Total length in bits
    trans.tx_buffer = tx_buffer;
    trans.rx_buffer = rx_buffer;

    ret = spi_device_transmit(spi_device, &trans);

    if(ret == ESP_OK)
    {
        // Copy received data (skip the first dummy byte)
        memcpy(data, &rx_buffer[1], data_length);
    }
    else
    {
        ESP_LOGE(TAG, "SPI read failed: %s", esp_err_to_name(ret));
    }

    free(tx_buffer);
    free(rx_buffer);

    return (ret == ESP_OK) ? SX127X_HAL_STATUS_OK : SX127X_HAL_STATUS_ERROR;
}

void sx127x_hal_reset(const sx127x_t* radio)
{
    // Configure reset pin
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << CONFIG_LBM_RADIO_RESET_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);

    // Reset sequence: LOW -> HIGH
    gpio_set_level(CONFIG_LBM_RADIO_RESET_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(CONFIG_LBM_RADIO_RESET_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(6));  // Wait for radio to be ready

    ESP_LOGI(TAG, "SX127x reset completed");
}

sx127x_hal_status_t sx127x_hal_wakeup(const sx127x_t* radio)
{
    // For SX127x, wakeup is typically done by NSS toggle
    // This is handled automatically by SPI transactions
    return SX127X_HAL_STATUS_OK;
}

// Timer management functions
sx127x_hal_status_t sx127x_hal_timer_start(const sx127x_t* radio, const uint32_t time_in_ms, void (*callback)(void* context))
{
    // Store the callback and context for later use
    timer_callback = callback;
    timer_context = (void*)radio;

    // Stop any existing timer
    if(rx_timer_handle != NULL)
    {
        esp_timer_stop(rx_timer_handle);
        esp_timer_delete(rx_timer_handle);
        rx_timer_handle = NULL;
    }

    // Create timer
    esp_timer_create_args_t timer_args = {
        .callback = rx_timer_callback, .arg = (void*)radio, .dispatch_method = ESP_TIMER_TASK, .name = "sx127x_rx_timer"};

    esp_err_t ret = esp_timer_create(&timer_args, &rx_timer_handle);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create RX timer: %s", esp_err_to_name(ret));
        return SX127X_HAL_STATUS_ERROR;
    }

    // Start timer
    ret = esp_timer_start_once(rx_timer_handle, time_in_ms * 1000);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start RX timer: %s", esp_err_to_name(ret));
        return SX127X_HAL_STATUS_ERROR;
    }

    return SX127X_HAL_STATUS_OK;
}

sx127x_hal_status_t sx127x_hal_timer_stop(const sx127x_t* radio)
{
    if(rx_timer_handle != NULL)
    {
        esp_timer_stop(rx_timer_handle);
        esp_timer_delete(rx_timer_handle);
        rx_timer_handle = NULL;
    }

    // Clear stored callback
    timer_callback = NULL;
    timer_context = NULL;

    return SX127X_HAL_STATUS_OK;
}

bool sx127x_hal_timer_is_started(const sx127x_t* radio)
{
    return (rx_timer_handle != NULL);
}

// GPIO and IRQ management functions
sx127x_radio_id_t sx127x_hal_get_radio_id(const sx127x_t* radio)
{
    // Return SX1276 as default radio ID
    return SX127X_RADIO_ID_SX1276;
}

uint32_t sx127x_hal_get_dio_1_pin_state(const sx127x_t* radio)
{
#if defined(CONFIG_LBM_RADIO_DIO1_PIN) && CONFIG_LBM_RADIO_DIO1_PIN >= 0
    return gpio_get_level(CONFIG_LBM_RADIO_DIO1_PIN);
#else
    return 0;  // Default if DIO1 not configured
#endif
}

void sx127x_hal_dio_irq_attach(const sx127x_t* radio)
{
    // Configure DIO0 interrupt (main interrupt pin)
#if defined(CONFIG_LBM_RADIO_DIO0_PIN) && CONFIG_LBM_RADIO_DIO0_PIN >= 0
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << CONFIG_LBM_RADIO_DIO0_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);

    // Install ISR service if not already done
    esp_err_t ret = gpio_install_isr_service(0);
    if(ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGW(TAG, "GPIO ISR service already installed or failed: %s", esp_err_to_name(ret));
        // Continue anyway, service might be installed by another component
    }

    // Add ISR handler
    ret = gpio_isr_handler_add(CONFIG_LBM_RADIO_DIO0_PIN, dio_isr_handler, (void*)radio);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add DIO0 ISR handler: %s", esp_err_to_name(ret));
        return;
    }
#endif

    ESP_LOGI(TAG, "SX127x IRQ attached successfully");
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static esp_err_t sx127x_spi_init(void)
{
    if(hal_initialized)
    {
        return ESP_OK;
    }

    esp_err_t ret;

    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = CONFIG_LBM_SPI_MISO_PIN,
        .mosi_io_num = CONFIG_LBM_SPI_MOSI_PIN,
        .sclk_io_num = CONFIG_LBM_SPI_CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 256,
        .flags = SPICOMMON_BUSFLAG_MASTER,
    };

    ret = spi_bus_initialize(CONFIG_LBM_SPI_HOST, &buscfg, SPI_DMA_DISABLED);
    if(ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure SPI device
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = CONFIG_LBM_SPI_FREQUENCY_HZ,
        .mode = 0,  // SPI mode 0 (CPOL=0, CPHA=0)
        .spics_io_num = CONFIG_LBM_RADIO_NSS_PIN,
        .queue_size = 7,
        .flags = 0,  // Remove half-duplex flag to fix DMA issues
    };

    ret = spi_bus_add_device(CONFIG_LBM_SPI_HOST, &devcfg, &spi_device);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure DIO pins as inputs
    // Configure DIO pins with guards for undefined pins
    uint64_t dio_pin_mask = 0;

#if defined(CONFIG_LBM_RADIO_DIO0_PIN) && CONFIG_LBM_RADIO_DIO0_PIN >= 0
    dio_pin_mask |= (1ULL << CONFIG_LBM_RADIO_DIO0_PIN);
#endif
#if defined(CONFIG_LBM_RADIO_DIO1_PIN) && CONFIG_LBM_RADIO_DIO1_PIN >= 0
    dio_pin_mask |= (1ULL << CONFIG_LBM_RADIO_DIO1_PIN);
#endif
#if defined(CONFIG_LBM_RADIO_DIO2_PIN) && CONFIG_LBM_RADIO_DIO2_PIN >= 0
    dio_pin_mask |= (1ULL << CONFIG_LBM_RADIO_DIO2_PIN);
#endif

    if(dio_pin_mask != 0)
    {
        gpio_config_t dio_conf = {
            .intr_type = GPIO_INTR_DISABLE,
            .mode = GPIO_MODE_INPUT,
            .pin_bit_mask = dio_pin_mask,
            .pull_down_en = 0,
            .pull_up_en = 0,
        };
        gpio_config(&dio_conf);
    }

    hal_initialized = true;
    ESP_LOGI(TAG, "SX127x HAL initialized successfully");

    return ESP_OK;
}

static void rx_timer_callback(void* arg)
{
    // Call the stored timer callback if available
    if(timer_callback != NULL)
    {
        timer_callback(timer_context);
    }
}

static void dio_isr_handler(void* arg)
{
    const sx127x_t* radio = (const sx127x_t*)arg;
    if(radio && radio->dio_0_irq_handler)
    {
        radio->dio_0_irq_handler((void*)radio->hal_context);
    }
}
