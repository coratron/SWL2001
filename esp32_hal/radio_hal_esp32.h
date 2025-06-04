/*!
 * \file      radio_hal_esp32.h
 *
 * \brief     ESP32 Radio Hardware Abstraction Layer header
 *
 * \copyright Copyright (c) 2023
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef RADIO_HAL_ESP32_H
#define RADIO_HAL_ESP32_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS PROTOTYPES ---------------------------------------------
 */

/**
 * @brief Initialize the radio HAL
 * 
 * This function initializes the SPI bus, configures GPIO pins, and sets up
 * interrupt handlers for all supported radio types (SX126x, SX127x, etc.)
 */
void radio_hal_init(void);

/**
 * @brief Reset the radio
 */
void radio_hal_reset(void);

/**
 * @brief Check if radio is busy
 * @return true if radio is busy, false otherwise
 */
bool radio_hal_is_busy(void);

/**
 * @brief Wake up the radio from sleep
 */
void radio_hal_wakeup(void);

/**
 * @brief Set radio sleep state tracking
 * @param sleep_state true if radio is going to sleep, false if waking up
 */
void radio_hal_set_sleep(bool sleep_state);

/**
 * @brief Write data to radio via SPI
 * @param command Command bytes
 * @param command_length Number of command bytes
 * @param data Data bytes to write
 * @param data_length Number of data bytes
 * @return ESP_OK on success
 */
esp_err_t radio_hal_spi_write(const uint8_t* command, uint16_t command_length,
                             const uint8_t* data, uint16_t data_length);

/**
 * @brief Read data from radio via SPI
 * @param command Command bytes
 * @param command_length Number of command bytes
 * @param data Buffer to read data into
 * @param data_length Number of data bytes to read
 * @return ESP_OK on success
 */
esp_err_t radio_hal_spi_read(const uint8_t* command, uint16_t command_length,
                            uint8_t* data, uint16_t data_length);

/**
 * @brief Configure radio interrupt callback
 * @param callback Callback function
 * @param context Context to pass to callback
 */
void radio_hal_irq_config(void (*callback)(void* context), void* context);

/**
 * @brief Clear pending radio interrupts
 */
void radio_hal_irq_clear_pending(void);

/*
 * -----------------------------------------------------------------------------
 * --- WEAK FUNCTIONS (can be overridden by user) -----------------------------
 */

/**
 * @brief Reset the radio implementation (WEAK - can be overridden)
 * 
 * Default implementation uses direct GPIO control.
 * Override this function for custom reset implementations
 * (e.g., using port expanders, different GPIO libraries, etc.)
 */
void radio_hal_reset_impl(void);

/**
 * @brief Check if radio is busy implementation (WEAK - can be overridden)
 * 
 * Default implementation reads the BUSY pin directly (SX126X) or returns false (SX127X).
 * Override this function for custom busy signal reading.
 * 
 * @return true if radio is busy, false otherwise
 */
bool radio_hal_is_busy_impl(void);

/**
 * @brief Wake up the radio implementation (WEAK - can be overridden)
 * 
 * Default implementation uses NSS toggling (SX126X) or dummy SPI command (SX127X).
 * Override this function for custom wakeup implementations.
 */
void radio_hal_wakeup_impl(void);

/**
 * @brief Write data to radio via SPI implementation (WEAK - can be overridden)
 * 
 * Default implementation uses ESP-IDF SPI driver.
 * Override this function for custom SPI implementations
 * (e.g., using different SPI libraries, software SPI, etc.)
 * 
 * @param command Command bytes
 * @param command_length Number of command bytes
 * @param data Data bytes to write
 * @param data_length Number of data bytes
 * @return ESP_OK on success
 */
esp_err_t radio_hal_spi_write_impl(const uint8_t* command, uint16_t command_length,
                                  const uint8_t* data, uint16_t data_length);

/**
 * @brief Read data from radio via SPI implementation (WEAK - can be overridden)
 * 
 * Default implementation uses ESP-IDF SPI driver.
 * Override this function for custom SPI implementations.
 * 
 * @param command Command bytes
 * @param command_length Number of command bytes
 * @param data Buffer to read data into
 * @param data_length Number of data bytes to read
 * @return ESP_OK on success
 */
esp_err_t radio_hal_spi_read_impl(const uint8_t* command, uint16_t command_length,
                                 uint8_t* data, uint16_t data_length);

/**
 * @brief Configure radio interrupt callback implementation (WEAK - can be overridden)
 * 
 * Default implementation uses ESP-IDF GPIO interrupts.
 * Override this function for custom interrupt handling.
 * 
 * @param callback Callback function
 * @param context Context to pass to callback
 */
void radio_hal_irq_config_impl(void (*callback)(void* context), void* context);

/**
 * @brief Clear pending radio interrupts implementation (WEAK - can be overridden)
 * 
 * Default implementation uses ESP-IDF GPIO interrupt control.
 * Override this function for custom interrupt clearing.
 */
void radio_hal_irq_clear_pending_impl(void);

#ifdef __cplusplus
}
#endif

#endif /* RADIO_HAL_ESP32_H */
