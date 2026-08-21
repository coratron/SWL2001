/**
 * @file sx127x_esp_internal.h
 * @brief Internal definitions for ESP32 SX127x wrapper
 *
 * This file contains internal structures and definitions used by the ESP32
 * wrapper implementation. Not intended for external use.
 */

#ifndef SX127X_ESP_INTERNAL_H
#define SX127X_ESP_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sx127x_esp_wrapper.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief DIO interrupt event structure
 */
typedef struct {
  uint8_t dio_num;    ///< DIO pin number (0, 1, 2) or SX127X_ESP_EVENT_RX_TIMER
  uint64_t timestamp; ///< Timestamp when interrupt occurred (us)
  uint32_t gen;       ///< Generation counter for RX timer events (0 for DIO)
} sx127x_esp_dio_event_t;

/**
 * @brief Special event type for synthetic RX timeout events
 */
#define SX127X_ESP_EVENT_RX_TIMER 3U

/**
 * @brief ESP32 context structure for SX127x radio
 */
typedef struct {
  // Radio reference
  sx127x_t *radio;

  // SPI configuration
  spi_device_handle_t spi_device;
  spi_bus_config_t spi_bus_config;
  spi_device_interface_config_t spi_device_config;

  // GPIO configuration
  gpio_num_t dio_pins[3]; ///< DIO0, DIO1, DIO2 pins
  gpio_num_t nss_gpio;
  gpio_num_t reset_gpio;

  // Interrupt handling
  void (*dio_callbacks[3])(void *context);
  QueueHandle_t event_queue;
  TaskHandle_t event_task;
  bool interrupts_enabled;

  // Thread safety
  SemaphoreHandle_t spi_mutex;
  SemaphoreHandle_t state_mutex;

  // Statistics
#ifdef CONFIG_LBM_SX127X_INTERRUPT_STATS
  sx127x_esp_interrupt_stats_t stats;
#endif

  // Performance metrics
  uint64_t last_spi_start_time;
  uint32_t spi_transaction_count;
  uint64_t total_spi_time;

  // State
  bool initialized;
  bool radio_busy;

  // RX Timeout Timer (software emulation for SX127x continuous RX)
  esp_timer_handle_t rx_timer;
  void (*rx_timer_callback)(void *context);
  volatile bool rx_timer_started;
  volatile uint32_t rx_timer_gen;
  portMUX_TYPE rx_timer_lock;

  // Configuration
  sx127x_esp_config_t config;
} sx127x_esp_context_t;

/**
 * @brief Register definition for debugging
 */
typedef struct {
  uint8_t address;
  const char *name;
  const char *description;
  bool is_common; ///< True if register exists in both LoRa and GFSK modes
} sx127x_esp_register_def_t;

/**
 * @brief Get ESP32 context from radio structure
 *
 * @param radio Pointer to SX127x radio structure
 * @return sx127x_esp_context_t* Pointer to ESP32 context or NULL if not found
 */
sx127x_esp_context_t *sx127x_esp_get_context(const sx127x_t *radio);

/**
 * @brief Initialize SPI bus and device
 *
 * @param ctx ESP32 context
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_spi_init(sx127x_esp_context_t *ctx);

/**
 * @brief Deinitialize SPI bus and device
 *
 * @param ctx ESP32 context
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_spi_deinit(sx127x_esp_context_t *ctx);

/**
 * @brief Initialize GPIO pins
 *
 * @param ctx ESP32 context
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_gpio_init(sx127x_esp_context_t *ctx);

/**
 * @brief Deinitialize GPIO pins
 *
 * @param ctx ESP32 context
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_gpio_deinit(sx127x_esp_context_t *ctx);

/**
 * @brief Initialize interrupt handling
 *
 * @param ctx ESP32 context
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_interrupt_init(sx127x_esp_context_t *ctx);

/**
 * @brief Deinitialize interrupt handling
 *
 * @param ctx ESP32 context
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_interrupt_deinit(sx127x_esp_context_t *ctx);

/**
 * @brief Event processing task
 *
 * @param pvParameters Task parameters (ESP32 context)
 */
void sx127x_esp_event_task(void *pvParameters);

/**
 * @brief DIO0 interrupt service routine
 *
 * @param arg ISR argument (ESP32 context)
 */
void sx127x_esp_dio0_isr(void *arg);

/**
 * @brief DIO1 interrupt service routine
 *
 * @param arg ISR argument (ESP32 context)
 */
void sx127x_esp_dio1_isr(void *arg);

/**
 * @brief DIO2 interrupt service routine
 *
 * @param arg ISR argument (ESP32 context)
 */
void sx127x_esp_dio2_isr(void *arg);

/**
 * @brief Update interrupt statistics
 *
 * @param ctx ESP32 context
 * @param dio_num DIO pin number
 * @param timestamp Interrupt timestamp
 */
void sx127x_esp_update_interrupt_stats(sx127x_esp_context_t *ctx,
                                       uint8_t dio_num, uint64_t timestamp);

/**
 * @brief Start SPI transaction timing
 *
 * @param ctx ESP32 context
 */
static inline void sx127x_esp_spi_timing_start(sx127x_esp_context_t *ctx) {
  ctx->last_spi_start_time = esp_timer_get_time();
}

/**
 * @brief End SPI transaction timing
 *
 * @param ctx ESP32 context
 */
static inline void sx127x_esp_spi_timing_end(sx127x_esp_context_t *ctx) {
  uint64_t duration = esp_timer_get_time() - ctx->last_spi_start_time;
  ctx->total_spi_time += duration;
  ctx->spi_transaction_count++;
}

/**
 * @brief Convert ESP-IDF error to SX127x ESP error
 *
 * @param esp_err ESP-IDF error code
 * @return sx127x_esp_err_t Converted error code
 */
sx127x_esp_err_t sx127x_esp_convert_error(esp_err_t esp_err);

/**
 * @brief Validate configuration parameters
 *
 * @param config Configuration to validate
 * @return sx127x_esp_err_t Error code
 */
sx127x_esp_err_t sx127x_esp_validate_config(const sx127x_esp_config_t *config);

#ifdef __cplusplus
}
#endif

#endif // SX127X_ESP_INTERNAL_H
