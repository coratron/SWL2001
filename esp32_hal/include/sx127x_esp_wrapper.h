/**
 * @file sx127x_esp_wrapper.h
 * @brief ESP32 wrapper for SX127x LoRa radio driver
 * 
 * This file provides ESP-IDF specific wrapper functions for the SX127x radio driver,
 * including initialization, configuration, and utility functions.
 */

#ifndef SX127X_ESP_WRAPPER_H
#define SX127X_ESP_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// Include SX127x driver headers
#include "sx127x.h"
#include "sx127x_hal.h"
#include "ral_sx127x_bsp.h"

/**
 * @brief ESP32-specific error codes
 */
typedef enum {
    SX127X_ESP_OK = 0,                  ///< Success
    SX127X_ESP_ERR_INVALID_ARG,         ///< Invalid argument
    SX127X_ESP_ERR_NO_MEM,              ///< Out of memory
    SX127X_ESP_ERR_SPI_INIT,            ///< SPI initialization failed
    SX127X_ESP_ERR_GPIO_INIT,           ///< GPIO initialization failed
    SX127X_ESP_ERR_INTERRUPT_INIT,      ///< Interrupt initialization failed
    SX127X_ESP_ERR_RADIO_NOT_FOUND,     ///< Radio not responding
    SX127X_ESP_ERR_TIMEOUT,             ///< Operation timeout
    SX127X_ESP_ERR_NOT_INITIALIZED,     ///< Component not initialized
    SX127X_ESP_ERR_RADIO_BUSY,          ///< Radio is busy
} sx127x_esp_err_t;

/**
 * @brief ESP32 hardware configuration for SX127x
 */
typedef struct {
    // SPI configuration
    spi_host_device_t spi_host;         ///< SPI host (SPI2_HOST, SPI3_HOST, etc.)
    uint32_t spi_frequency;             ///< SPI clock frequency in Hz
    bool use_dma;                       ///< Enable DMA for SPI transfers
    
    // GPIO pins
    gpio_num_t miso_gpio;               ///< MISO GPIO pin
    gpio_num_t mosi_gpio;               ///< MOSI GPIO pin
    gpio_num_t sck_gpio;                ///< SCK GPIO pin
    gpio_num_t nss_gpio;                ///< NSS (chip select) GPIO pin
    gpio_num_t reset_gpio;              ///< Reset GPIO pin
    gpio_num_t dio0_gpio;               ///< DIO0 GPIO pin
    gpio_num_t dio1_gpio;               ///< DIO1 GPIO pin
    gpio_num_t dio2_gpio;               ///< DIO2 GPIO pin
    
    // Interrupt configuration
    int interrupt_priority;             ///< Interrupt priority (1-7)
    bool use_dedicated_task;            ///< Use dedicated task for event processing
    uint32_t event_task_priority;       ///< Event task priority
    uint32_t event_task_stack_size;     ///< Event task stack size
    uint32_t event_queue_size;          ///< Event queue size
} sx127x_esp_config_t;

/**
 * @brief Interrupt statistics structure
 */
typedef struct {
    uint32_t dio0_count;                ///< DIO0 interrupt count
    uint32_t dio1_count;                ///< DIO1 interrupt count
    uint32_t dio2_count;                ///< DIO2 interrupt count
    uint64_t last_dio0_time;            ///< Last DIO0 interrupt timestamp (us)
    uint64_t last_dio1_time;            ///< Last DIO1 interrupt timestamp (us)
    uint64_t last_dio2_time;            ///< Last DIO2 interrupt timestamp (us)
    uint32_t missed_events;             ///< Number of missed events
    uint32_t queue_overflows;           ///< Number of queue overflows
} sx127x_esp_interrupt_stats_t;

/**
 * @brief Register information structure for debugging
 */
typedef struct {
    uint8_t address;                    ///< Register address
    uint8_t value;                      ///< Register value
    const char* name;                   ///< Register name
    const char* description;            ///< Register description
} sx127x_esp_register_info_t;

/**
 * @brief Performance metrics structure
 */
typedef struct {
    uint32_t spi_transaction_time_us;   ///< Average SPI transaction time
    uint32_t interrupt_latency_us;      ///< Average interrupt latency
    uint32_t tx_setup_time_us;          ///< TX setup time
    uint32_t rx_setup_time_us;          ///< RX setup time
} sx127x_esp_performance_t;

/**
 * @brief Initialize SX127x with ESP32-specific configuration
 * 
 * @param radio Pointer to SX127x radio structure
 * @param config ESP32 hardware configuration
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_init(sx127x_t* radio, const sx127x_esp_config_t* config);

/**
 * @brief Deinitialize SX127x and free resources
 * 
 * @param radio Pointer to SX127x radio structure
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_deinit(sx127x_t* radio);

/**
 * @brief Initialize radio for LBM modem use with default configuration
 * 
 * @param radio Pointer to SX127x radio structure
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_init_for_lbm(sx127x_t* radio);

/**
 * @brief Get default ESP32 configuration
 * 
 * @param config Pointer to configuration structure to fill
 */
void sx127x_esp_get_default_config(sx127x_esp_config_t* config);

/**
 * @brief Enable or disable DIO interrupts
 * 
 * @param radio Pointer to SX127x radio structure
 * @param enable True to enable, false to disable
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_dio_enable(sx127x_t* radio, bool enable);

/**
 * @brief Get interrupt statistics
 * 
 * @param radio Pointer to SX127x radio structure
 * @param stats Pointer to statistics structure to fill
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_get_interrupt_stats(sx127x_t* radio, sx127x_esp_interrupt_stats_t* stats);

/**
 * @brief Reset interrupt statistics
 * 
 * @param radio Pointer to SX127x radio structure
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_reset_interrupt_stats(sx127x_t* radio);

/**
 * @brief Perform self-test of radio communication
 * 
 * @param radio Pointer to SX127x radio structure
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_self_test(sx127x_t* radio);

/**
 * @brief Test SPI communication with radio
 * 
 * @param radio Pointer to SX127x radio structure
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_spi_test(sx127x_t* radio);

/**
 * @brief Test interrupt functionality
 * 
 * @param radio Pointer to SX127x radio structure
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_interrupt_test(sx127x_t* radio);

#ifdef CONFIG_LBM_SX127X_ENABLE_REGISTER_DUMP
/**
 * @brief Dump all radio registers for debugging
 * 
 * @param radio Pointer to SX127x radio structure
 * @param reg_dump Pointer to array to store register information
 * @param count Pointer to variable to store number of registers dumped
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_dump_registers(sx127x_t* radio, sx127x_esp_register_info_t* reg_dump, size_t* count);

/**
 * @brief Read a specific register by name
 * 
 * @param radio Pointer to SX127x radio structure
 * @param reg_name Register name
 * @param value Pointer to store register value
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_read_register_by_name(sx127x_t* radio, const char* reg_name, uint8_t* value);

/**
 * @brief Print register dump to console
 * 
 * @param radio Pointer to SX127x radio structure
 */
void sx127x_esp_print_register_dump(sx127x_t* radio);
#endif

/**
 * @brief Get performance metrics
 * 
 * @param radio Pointer to SX127x radio structure
 * @param performance Pointer to performance structure to fill
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_get_performance_metrics(sx127x_t* radio, sx127x_esp_performance_t* performance);

/**
 * @brief Convert ESP32 error code to string
 * 
 * @param err Error code
 * @return const char* Error string
 */
const char* sx127x_esp_err_to_string(sx127x_esp_err_t err);

/**
 * @brief Check if radio is responding
 * 
 * @param radio Pointer to SX127x radio structure
 * @return bool True if radio is responding, false otherwise
 */
bool sx127x_esp_is_radio_responding(sx127x_t* radio);

/**
 * @brief Get radio version information
 * 
 * @param radio Pointer to SX127x radio structure
 * @param version Pointer to store version information
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_get_radio_version(sx127x_t* radio, uint8_t* version);

#ifdef __cplusplus
}
#endif

#endif // SX127X_ESP_WRAPPER_H
