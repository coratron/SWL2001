/*!
 * \file      radio_hal_esp32.c
 *
 * \brief     ESP32 Radio Hardware Abstraction Layer implementation
 *
 * \copyright Copyright (c) 2023
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdint.h>
#include <stdbool.h>

#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_intr_alloc.h"

#include "sdkconfig.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS ----------------------------------------------------------
 */

#define TAG "RADIO_HAL"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

// SPI Configuration
#define SPI_HOST_ID HSPI_HOST
#define SPI_DMA_CHAN 1

// Default pin configuration (can be overridden via Kconfig)
#ifndef CONFIG_LBM_RADIO_NSS_PIN
#define CONFIG_LBM_RADIO_NSS_PIN 5
#endif

#ifndef CONFIG_LBM_RADIO_RESET_PIN
#define CONFIG_LBM_RADIO_RESET_PIN 14
#endif

#ifndef CONFIG_LBM_RADIO_BUSY_PIN
#define CONFIG_LBM_RADIO_BUSY_PIN 32
#endif

#ifndef CONFIG_LBM_RADIO_DIO1_PIN
#define CONFIG_LBM_RADIO_DIO1_PIN 33
#endif

#ifndef CONFIG_LBM_RADIO_DIO2_PIN
#define CONFIG_LBM_RADIO_DIO2_PIN -1
#endif

#ifndef CONFIG_LBM_RADIO_DIO3_PIN
#define CONFIG_LBM_RADIO_DIO3_PIN -1
#endif

#ifndef CONFIG_LBM_SPI_FREQUENCY
#define CONFIG_LBM_SPI_FREQUENCY 8000000
#endif

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

typedef struct {
    spi_device_handle_t spi_handle;
    void (*irq_callback)(void* context);
    void* irq_context;
    bool initialized;
    bool is_sleeping;
} radio_hal_context_t;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static radio_hal_context_t radio_hal_ctx = {0};
static SemaphoreHandle_t spi_mutex = NULL;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

static void IRAM_ATTR dio1_isr_handler(void* arg);
#ifdef CONFIG_LBM_RADIO_SX127X
static void IRAM_ATTR dio0_isr_handler(void* arg);
static void IRAM_ATTR dio2_isr_handler(void* arg);
#endif
static esp_err_t radio_spi_init(void);
static void radio_configure_pins(void);

/*
 * -----------------------------------------------------------------------------
 * --- WEAK FUNCTION PROTOTYPES (can be overridden by user) ------------------
 */

// These functions can be overridden by the user for custom hardware implementations
__attribute__((weak)) void radio_hal_reset_impl(void);
__attribute__((weak)) bool radio_hal_is_busy_impl(void);
__attribute__((weak)) void radio_hal_wakeup_impl(void);
__attribute__((weak)) esp_err_t radio_hal_spi_write_impl(const uint8_t* command, uint16_t command_length,
                                                        const uint8_t* data, uint16_t data_length);
__attribute__((weak)) esp_err_t radio_hal_spi_read_impl(const uint8_t* command, uint16_t command_length,
                                                       uint8_t* data, uint16_t data_length);
__attribute__((weak)) void radio_hal_irq_config_impl(void (*callback)(void* context), void* context);
__attribute__((weak)) void radio_hal_irq_clear_pending_impl(void);

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void radio_hal_init(void)
{
    if (radio_hal_ctx.initialized) {
        return;
    }

    // Initialize SPI mutex
    if (spi_mutex == NULL) {
        spi_mutex = xSemaphoreCreateMutex();
        if (spi_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create SPI mutex");
            return;
        }
    }

    // Initialize SPI
    if (radio_spi_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI");
        return;
    }

    // Configure radio pins
    radio_configure_pins();

    radio_hal_ctx.initialized = true;
    radio_hal_ctx.is_sleeping = false;
    
    ESP_LOGI(TAG, "Radio HAL initialized for %s", 
#ifdef CONFIG_LBM_RADIO_SX126X
             "SX126X"
#elif defined(CONFIG_LBM_RADIO_SX127X)
             "SX127X"
#elif defined(CONFIG_LBM_RADIO_LR11XX)
             "LR11XX"
#elif defined(CONFIG_LBM_RADIO_SX128X)
             "SX128X"
#else
             "Unknown"
#endif
    );
}

void radio_hal_reset(void)
{
    radio_hal_reset_impl();
}

bool radio_hal_is_busy(void)
{
    return radio_hal_is_busy_impl();
}

void radio_hal_wakeup(void)
{
    if (!radio_hal_ctx.is_sleeping) {
        return;
    }
    
    radio_hal_wakeup_impl();
    
    radio_hal_ctx.is_sleeping = false;
    ESP_LOGD(TAG, "Radio woken up");
}

void radio_hal_set_sleep(bool sleep_state)
{
    radio_hal_ctx.is_sleeping = sleep_state;
    ESP_LOGD(TAG, "Radio sleep state: %s", sleep_state ? "sleeping" : "awake");
}

esp_err_t radio_hal_spi_write(const uint8_t* command, uint16_t command_length,
                             const uint8_t* data, uint16_t data_length)
{
    return radio_hal_spi_write_impl(command, command_length, data, data_length);
}

esp_err_t radio_hal_spi_read(const uint8_t* command, uint16_t command_length,
                            uint8_t* data, uint16_t data_length)
{
    return radio_hal_spi_read_impl(command, command_length, data, data_length);
}

void radio_hal_irq_config(void (*callback)(void* context), void* context)
{
    radio_hal_irq_config_impl(callback, context);
}

void radio_hal_irq_clear_pending(void)
{
    radio_hal_irq_clear_pending_impl();
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static void IRAM_ATTR dio1_isr_handler(void* arg)
{
    if (radio_hal_ctx.irq_callback != NULL) {
        radio_hal_ctx.irq_callback(radio_hal_ctx.irq_context);
    }
}

static esp_err_t radio_spi_init(void)
{
    spi_bus_config_t bus_config = {
        .miso_io_num = GPIO_NUM_19,
        .mosi_io_num = GPIO_NUM_23,
        .sclk_io_num = GPIO_NUM_18,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 256,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST_ID, &bus_config, SPI_DMA_CHAN));

    spi_device_interface_config_t dev_config = {
        .clock_speed_hz = CONFIG_LBM_SPI_FREQUENCY,
        .mode = 0, // CPOL = 0, CPHA = 0
        .spics_io_num = -1, // We'll handle CS manually
        .queue_size = 1,
        .flags = 0,
        .pre_cb = NULL,
        .post_cb = NULL,
    };

    return spi_bus_add_device(SPI_HOST_ID, &dev_config, &radio_hal_ctx.spi_handle);
}

static void radio_configure_pins(void)
{
    gpio_config_t io_conf = {0};

    // Configure NSS pin
    io_conf.pin_bit_mask = (1ULL << CONFIG_LBM_RADIO_NSS_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    gpio_set_level(CONFIG_LBM_RADIO_NSS_PIN, 1);

    // Configure RESET pin
    io_conf.pin_bit_mask = (1ULL << CONFIG_LBM_RADIO_RESET_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    gpio_set_level(CONFIG_LBM_RADIO_RESET_PIN, 1);

#ifdef CONFIG_LBM_RADIO_SX126X
    // Configure BUSY pin (SX126X only)
    if (CONFIG_LBM_RADIO_BUSY_PIN >= 0) {
        io_conf.pin_bit_mask = (1ULL << CONFIG_LBM_RADIO_BUSY_PIN);
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        ESP_ERROR_CHECK(gpio_config(&io_conf));
    }
#endif

    // Configure DIO1 pin (interrupt)
    if (CONFIG_LBM_RADIO_DIO1_PIN >= 0) {
        io_conf.pin_bit_mask = (1ULL << CONFIG_LBM_RADIO_DIO1_PIN);
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.intr_type = GPIO_INTR_POSEDGE;
        ESP_ERROR_CHECK(gpio_config(&io_conf));

        // Install ISR service and handler
        ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
        ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_LBM_RADIO_DIO1_PIN, dio1_isr_handler, NULL));
    }

#ifdef CONFIG_LBM_RADIO_SX127X
    // Configure DIO0 pin for SX127X (additional interrupt)
    if (CONFIG_LBM_RADIO_DIO2_PIN >= 0) {
        io_conf.pin_bit_mask = (1ULL << CONFIG_LBM_RADIO_DIO2_PIN);
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.intr_type = GPIO_INTR_POSEDGE;
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_LBM_RADIO_DIO2_PIN, dio0_isr_handler, NULL));
    }

    // Configure DIO2 pin for SX127X (additional interrupt)  
    if (CONFIG_LBM_RADIO_DIO3_PIN >= 0) {
        io_conf.pin_bit_mask = (1ULL << CONFIG_LBM_RADIO_DIO3_PIN);
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.intr_type = GPIO_INTR_POSEDGE;
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_LBM_RADIO_DIO3_PIN, dio2_isr_handler, NULL));
    }
#endif
}

#ifdef CONFIG_LBM_RADIO_SX127X
static void IRAM_ATTR dio0_isr_handler(void* arg)
{
    // DIO0 interrupt handler for SX127X
    if (radio_hal_ctx.irq_callback != NULL) {
        radio_hal_ctx.irq_callback(radio_hal_ctx.irq_context);
    }
}

static void IRAM_ATTR dio2_isr_handler(void* arg)
{
    // DIO2 interrupt handler for SX127X
    if (radio_hal_ctx.irq_callback != NULL) {
        radio_hal_ctx.irq_callback(radio_hal_ctx.irq_context);
    }
}
#endif

/*
 * -----------------------------------------------------------------------------
 * --- WEAK FUNCTION IMPLEMENTATIONS (can be overridden by user) --------------
 */

__attribute__((weak)) void radio_hal_reset_impl(void)
{
    // Default implementation using direct GPIO control
    // Users can override this function for custom reset implementations
    // (e.g., using port expanders, different GPIO libraries, etc.)
    gpio_set_level(CONFIG_LBM_RADIO_RESET_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(CONFIG_LBM_RADIO_RESET_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(5));
    ESP_LOGD(TAG, "Radio reset (default implementation)");
}

__attribute__((weak)) bool radio_hal_is_busy_impl(void)
{
    // Default implementation using direct GPIO control
    // Users can override this function for custom busy signal reading
#ifdef CONFIG_LBM_RADIO_SX126X
    // SX126X has a dedicated BUSY pin
    return gpio_get_level(CONFIG_LBM_RADIO_BUSY_PIN) == 1;
#else
    // SX127X doesn't have a BUSY pin, assume not busy
    // The SPI transactions will handle any timing requirements
    return false;
#endif
}

__attribute__((weak)) void radio_hal_wakeup_impl(void)
{
    // Default implementation using direct GPIO/SPI control
    // Users can override this function for custom wakeup implementations
#ifdef CONFIG_LBM_RADIO_SX126X
    // For SX126x, pulling NSS low will wake up the radio
    gpio_set_level(CONFIG_LBM_RADIO_NSS_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(CONFIG_LBM_RADIO_NSS_PIN, 1);
#elif defined(CONFIG_LBM_RADIO_SX127X)
    // For SX127X, any SPI transaction will wake up the radio
    // We can send a dummy command to wake it up
    uint8_t dummy_cmd = 0x00;
    radio_hal_spi_write_impl(&dummy_cmd, 1, NULL, 0);
#endif
    ESP_LOGD(TAG, "Radio wakeup (default implementation)");
}

__attribute__((weak)) esp_err_t radio_hal_spi_write_impl(const uint8_t* command, uint16_t command_length,
                                                        const uint8_t* data, uint16_t data_length)
{
    // Default implementation using ESP-IDF SPI driver
    // Users can override this function for custom SPI implementations
    // (e.g., using different SPI libraries, software SPI, etc.)
    
    if (!radio_hal_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t ret = ESP_OK;

    // Wait for radio to be ready
    uint32_t timeout = 1000; // 1ms timeout
    while (radio_hal_is_busy() && timeout--) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    if (timeout == 0) {
        ESP_LOGE(TAG, "Radio busy timeout");
        xSemaphoreGive(spi_mutex);
        return ESP_ERR_TIMEOUT;
    }

    // Prepare transaction
    spi_transaction_t trans = {0};
    
    if (command_length + data_length <= 4) {
        // Use cmd/addr fields for small transfers
        trans.flags = SPI_TRANS_USE_TXDATA;
        memcpy(trans.tx_data, command, command_length);
        if (data_length > 0) {
            memcpy(trans.tx_data + command_length, data, data_length);
        }
        trans.length = (command_length + data_length) * 8;
    } else {
        // Use buffer for larger transfers
        uint8_t* tx_buffer = malloc(command_length + data_length);
        if (tx_buffer == NULL) {
            xSemaphoreGive(spi_mutex);
            return ESP_ERR_NO_MEM;
        }
        
        memcpy(tx_buffer, command, command_length);
        if (data_length > 0) {
            memcpy(tx_buffer + command_length, data, data_length);
        }
        
        trans.tx_buffer = tx_buffer;
        trans.length = (command_length + data_length) * 8;
    }

    gpio_set_level(CONFIG_LBM_RADIO_NSS_PIN, 0);
    ret = spi_device_transmit(radio_hal_ctx.spi_handle, &trans);
    gpio_set_level(CONFIG_LBM_RADIO_NSS_PIN, 1);

    if (!(trans.flags & SPI_TRANS_USE_TXDATA)) {
        free((void*)trans.tx_buffer);
    }

    xSemaphoreGive(spi_mutex);
    return ret;
}

__attribute__((weak)) esp_err_t radio_hal_spi_read_impl(const uint8_t* command, uint16_t command_length,
                                                       uint8_t* data, uint16_t data_length)
{
    // Default implementation using ESP-IDF SPI driver
    // Users can override this function for custom SPI implementations
    
    if (!radio_hal_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t ret = ESP_OK;

    // Wait for radio to be ready
    uint32_t timeout = 1000;
    while (radio_hal_is_busy() && timeout--) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    if (timeout == 0) {
        ESP_LOGE(TAG, "Radio busy timeout");
        xSemaphoreGive(spi_mutex);
        return ESP_ERR_TIMEOUT;
    }

    // Send command first
    spi_transaction_t cmd_trans = {0};
    cmd_trans.flags = SPI_TRANS_USE_TXDATA;
    memcpy(cmd_trans.tx_data, command, command_length);
    cmd_trans.length = command_length * 8;

    gpio_set_level(CONFIG_LBM_RADIO_NSS_PIN, 0);
    ret = spi_device_transmit(radio_hal_ctx.spi_handle, &cmd_trans);
    
    if (ret == ESP_OK && data_length > 0) {
        // Read data
        spi_transaction_t data_trans = {0};
        if (data_length <= 4) {
            data_trans.flags = SPI_TRANS_USE_RXDATA;
            data_trans.length = data_length * 8;
            data_trans.rxlength = data_length * 8;
            
            ret = spi_device_transmit(radio_hal_ctx.spi_handle, &data_trans);
            if (ret == ESP_OK) {
                memcpy(data, data_trans.rx_data, data_length);
            }
        } else {
            data_trans.rx_buffer = data;
            data_trans.length = data_length * 8;
            data_trans.rxlength = data_length * 8;
            
            ret = spi_device_transmit(radio_hal_ctx.spi_handle, &data_trans);
        }
    }
    
    gpio_set_level(CONFIG_LBM_RADIO_NSS_PIN, 1);

    xSemaphoreGive(spi_mutex);
    return ret;
}

__attribute__((weak)) void radio_hal_irq_config_impl(void (*callback)(void* context), void* context)
{
    // Default implementation using ESP-IDF GPIO interrupts
    // Users can override this function for custom interrupt handling
    radio_hal_ctx.irq_callback = callback;
    radio_hal_ctx.irq_context = context;
}

__attribute__((weak)) void radio_hal_irq_clear_pending_impl(void)
{
    // Default implementation using ESP-IDF GPIO interrupts
    // Users can override this function for custom interrupt clearing
    gpio_intr_disable(CONFIG_LBM_RADIO_DIO1_PIN);
    gpio_intr_enable(CONFIG_LBM_RADIO_DIO1_PIN);
}
