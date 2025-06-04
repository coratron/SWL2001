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

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "sx127x_hal.h"
#include "radio_hal_esp32.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS ----------------------------------------------------------
 */

#define TAG "SX127X_HAL"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

// Default pin configuration
#ifndef CONFIG_LBM_RADIO_DIO0_PIN
    #define CONFIG_LBM_RADIO_DIO0_PIN 12
#endif

#ifndef CONFIG_LBM_RADIO_DIO1_PIN
    #define CONFIG_LBM_RADIO_DIO1_PIN 13
#endif

#ifndef CONFIG_LBM_RADIO_DIO2_PIN
    #define CONFIG_LBM_RADIO_DIO2_PIN -1
#endif

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static esp_timer_handle_t rx_timer_handle = NULL;

// Static variables for timer callback handling
static void (*timer_callback)(void* context) = NULL;
static void* timer_context = NULL;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

static void rx_timer_callback(void* arg);
static void IRAM_ATTR dio_isr_handler(void* arg);

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

sx127x_hal_status_t sx127x_hal_write(const sx127x_t* radio, const uint16_t address, const uint8_t* data, const uint16_t data_length)
{
    // Ensure radio HAL is initialized
    radio_hal_init();

    // Prepare the command byte (write operation, address)
    uint8_t cmd = address | 0x80;  // Set MSB for write operation

    // Use radio HAL for SPI communication
    esp_err_t ret = radio_hal_spi_write(&cmd, 1, data, data_length);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI write failed: %s", esp_err_to_name(ret));
        return SX127X_HAL_STATUS_ERROR;
    }

    return SX127X_HAL_STATUS_OK;
}

sx127x_hal_status_t sx127x_hal_read(const sx127x_t* radio, const uint16_t address, uint8_t* data, const uint16_t data_length)
{
    // Ensure radio HAL is initialized
    radio_hal_init();

    // Prepare the command byte (read operation, address)
    uint8_t cmd = address & 0x7F;  // Clear MSB for read operation

    // Use radio HAL for SPI communication
    esp_err_t ret = radio_hal_spi_read(&cmd, 1, data, data_length);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI read failed: %s", esp_err_to_name(ret));
        return SX127X_HAL_STATUS_ERROR;
    }

    return SX127X_HAL_STATUS_OK;
}

void sx127x_hal_reset(const sx127x_t* radio)
{
    // Use radio HAL for reset functionality
    radio_hal_reset();
    ESP_LOGI(TAG, "SX127x reset completed");
}

sx127x_hal_status_t sx127x_hal_wakeup(const sx127x_t* radio)
{
    // Use radio HAL for wakeup functionality
    radio_hal_wakeup();
    return SX127X_HAL_STATUS_OK;
}

// Timer management functions
sx127x_hal_status_t sx127x_hal_timer_start(const sx127x_t* radio, const uint32_t time_in_ms, void (*callback)(void* context))
{
    // Store the callback and context for later use
    timer_callback = callback;
    timer_context = (void*)radio;

    // Stop any existing timer
    if (rx_timer_handle != NULL) {
        esp_timer_stop(rx_timer_handle);
        esp_timer_delete(rx_timer_handle);
        rx_timer_handle = NULL;
    }

    // Create timer
    esp_timer_create_args_t timer_args = {
        .callback = rx_timer_callback, 
        .arg = (void*)radio, 
        .dispatch_method = ESP_TIMER_TASK, 
        .name = "sx127x_rx_timer"
    };

    esp_err_t ret = esp_timer_create(&timer_args, &rx_timer_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RX timer: %s", esp_err_to_name(ret));
        return SX127X_HAL_STATUS_ERROR;
    }

    // Start timer
    ret = esp_timer_start_once(rx_timer_handle, time_in_ms * 1000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start RX timer: %s", esp_err_to_name(ret));
        return SX127X_HAL_STATUS_ERROR;
    }

    return SX127X_HAL_STATUS_OK;
}

sx127x_hal_status_t sx127x_hal_timer_stop(const sx127x_t* radio)
{
    if (rx_timer_handle != NULL) {
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
    // Ensure radio HAL is initialized (this will configure the pins and interrupts)
    radio_hal_init();

    // Configure radio HAL interrupt callback to forward to SX127x handler
    radio_hal_irq_config(dio_isr_handler, (void*)radio);

    ESP_LOGI(TAG, "SX127x IRQ attached successfully");
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static void rx_timer_callback(void* arg)
{
    // Call the stored timer callback if available
    if (timer_callback != NULL) {
        timer_callback(timer_context);
    }
}

static void dio_isr_handler(void* arg)
{
    const sx127x_t* radio = (const sx127x_t*)arg;
    if (radio && radio->dio_0_irq_handler) {
        radio->dio_0_irq_handler((void*)radio->hal_context);
    }
}
