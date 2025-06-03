/*!
 * \file      smtc_modem_hal_esp32.c
 *
 * \brief     ESP32 Hardware Abstraction Layer implementation for LoRa Basics Modem
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
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_random.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/rtc_io.h"
#include "soc/rtc_wdt.h"

#include "smtc_modem_hal.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS ----------------------------------------------------------
 */

#define TAG "LBM_HAL"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

#define NVS_NAMESPACE "lbm_storage"
#define MAX_NVS_KEY_LENGTH 15

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

typedef struct
{
    esp_timer_handle_t timer_handle;
    void (*callback)(void* context);
    void* context;
} lbm_timer_t;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static lbm_timer_t lbm_timer = {0};
static SemaphoreHandle_t spi_mutex = NULL;
static nvs_handle_t nvs_handle;
static bool nvs_initialized = false;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

static void timer_callback(void* arg);
static esp_err_t init_nvs(void);

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

/* ------------ Reset management ------------*/
void smtc_modem_hal_reset_mcu(void)
{
    ESP_LOGI(TAG, "Resetting MCU");
    esp_restart();
}

/* ------------ Watchdog management ------------*/
void smtc_modem_hal_reload_wdog(void)
{
    // ESP32 has automatic watchdog, just feed the task watchdog
    esp_task_wdt_reset();
}

/* ------------ Time management ------------*/
uint32_t smtc_modem_hal_get_time_in_s(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000000ULL);
}

uint32_t smtc_modem_hal_get_time_in_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

uint32_t smtc_modem_hal_get_time_in_us(void)
{
    return (uint32_t)esp_timer_get_time();
}

uint32_t smtc_modem_hal_get_time_in_100us(void)
{
    return (uint32_t)(esp_timer_get_time() / 100ULL);
}

int32_t smtc_modem_hal_get_time_compensation_in_ms(void)
{
    // No compensation needed for ESP32
    return 0;
}

/* ------------ Timer management ------------*/
void smtc_modem_hal_start_timer(const uint32_t milliseconds, void (*callback)(void* context), void* context)
{
    if (lbm_timer.timer_handle != NULL) {
        esp_timer_stop(lbm_timer.timer_handle);
    }

    lbm_timer.callback = callback;
    lbm_timer.context = context;

    if (lbm_timer.timer_handle == NULL) {
        esp_timer_create_args_t timer_args = {
            .callback = timer_callback,
            .arg = NULL,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "lbm_timer"
        };
        ESP_ERROR_CHECK(esp_timer_create(&timer_args, &lbm_timer.timer_handle));
    }

    ESP_ERROR_CHECK(esp_timer_start_once(lbm_timer.timer_handle, milliseconds * 1000ULL));
}

void smtc_modem_hal_stop_timer(void)
{
    if (lbm_timer.timer_handle != NULL) {
        esp_timer_stop(lbm_timer.timer_handle);
    }
}

/* ------------ IRQ management ------------*/
void smtc_modem_hal_disable_modem_irq(void)
{
    taskENTER_CRITICAL();
}

void smtc_modem_hal_enable_modem_irq(void)
{
    taskEXIT_CRITICAL();
}

/* ------------ Context saving management ------------*/
void smtc_modem_hal_context_restore(const modem_context_type_t ctx_type, uint32_t offset, uint8_t* buffer, const uint32_t size)
{
    if (!nvs_initialized) {
        if (init_nvs() != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize NVS");
            return;
        }
    }

    char key[MAX_NVS_KEY_LENGTH];
    snprintf(key, sizeof(key), "ctx_%d_%lu", ctx_type, offset);

    size_t required_size = size;
    esp_err_t err = nvs_get_blob(nvs_handle, key, buffer, &required_size);
    
    if (err != ESP_OK || required_size != size) {
        ESP_LOGW(TAG, "Context restore failed for key %s, initializing with zeros", key);
        memset(buffer, 0, size);
    }
}

void smtc_modem_hal_context_store(const modem_context_type_t ctx_type, uint32_t offset, const uint8_t* buffer, const uint32_t size)
{
    if (!nvs_initialized) {
        if (init_nvs() != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize NVS");
            return;
        }
    }

    char key[MAX_NVS_KEY_LENGTH];
    snprintf(key, sizeof(key), "ctx_%d_%lu", ctx_type, offset);

    esp_err_t err = nvs_set_blob(nvs_handle, key, buffer, size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to store context for key %s", key);
        return;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS");
    }
}

/* ------------ Crashlog management ------------*/
void smtc_modem_hal_store_crashlog(uint8_t crashlog[CRASH_LOG_SIZE])
{
    ESP_LOGI(TAG, "Storing crashlog");
    smtc_modem_hal_context_store(CONTEXT_CRASHLOG, 0, crashlog, CRASH_LOG_SIZE);
}

void smtc_modem_hal_get_crashlog(uint8_t crashlog[CRASH_LOG_SIZE])
{
    ESP_LOGI(TAG, "Retrieving crashlog");
    smtc_modem_hal_context_restore(CONTEXT_CRASHLOG, 0, crashlog, CRASH_LOG_SIZE);
}

void smtc_modem_hal_set_crashlog_status(bool available)
{
    uint8_t status = available ? 1 : 0;
    smtc_modem_hal_context_store(CONTEXT_CRASHLOG, CRASH_LOG_SIZE, &status, 1);
}

bool smtc_modem_hal_get_crashlog_status(void)
{
    uint8_t status = 0;
    smtc_modem_hal_context_restore(CONTEXT_CRASHLOG, CRASH_LOG_SIZE, &status, 1);
    return (status == 1);
}

/* ------------ assert management ------------*/
void smtc_modem_hal_on_panic(uint8_t* func, uint32_t line, const char* fmt, ...)
{
    ESP_LOGE(TAG, "PANIC in function %s at line %lu", func, line);
    
    va_list args;
    va_start(args, fmt);
    esp_log_writev(ESP_LOG_ERROR, TAG, fmt, args);
    va_end(args);
    
    // Store crash information before reset
    uint8_t crashlog[CRASH_LOG_SIZE] = {0};
    snprintf((char*)crashlog, sizeof(crashlog), "PANIC: %s:%lu", func, line);
    smtc_modem_hal_store_crashlog(crashlog);
    smtc_modem_hal_set_crashlog_status(true);
    
    // Reset the system
    esp_restart();
}

/* ------------ Random management ------------*/
uint32_t smtc_modem_hal_get_random_nb(void)
{
    return esp_random();
}

uint32_t smtc_modem_hal_get_random_nb_in_range(const uint32_t val_1, const uint32_t val_2)
{
    uint32_t min_val = (val_1 < val_2) ? val_1 : val_2;
    uint32_t max_val = (val_1 < val_2) ? val_2 : val_1;
    
    if (min_val == max_val) {
        return min_val;
    }
    
    uint32_t range = max_val - min_val;
    return min_val + (esp_random() % (range + 1));
}

/* ------------ Radio env management ------------*/
void smtc_modem_hal_irq_config_radio_irq(void (*callback)(void* context), void* context)
{
    // This will be implemented in radio_hal_esp32.c
    extern void radio_hal_irq_config(void (*callback)(void* context), void* context);
    radio_hal_irq_config(callback, context);
}

void smtc_modem_hal_radio_irq_clear_pending(void)
{
    // This will be implemented in radio_hal_esp32.c
    extern void radio_hal_irq_clear_pending(void);
    radio_hal_irq_clear_pending();
}

void smtc_modem_hal_start_radio_tcxo(void)
{
    // Typically not needed for ESP32-based designs
    // Implementation depends on specific board design
}

void smtc_modem_hal_stop_radio_tcxo(void)
{
    // Typically not needed for ESP32-based designs
    // Implementation depends on specific board design
}

uint32_t smtc_modem_hal_get_radio_tcxo_startup_delay_ms(void)
{
    // Return the TCXO startup delay in milliseconds
    // This depends on the specific TCXO used
    return 5; // Typical value, adjust based on your TCXO
}

/* ------------ Environment management ------------*/
uint8_t smtc_modem_hal_get_battery_level(void)
{
    // Return battery level percentage (0-254, 255 = unknown)
    // This needs to be implemented based on your battery monitoring circuit
    return 255; // Unknown by default
}

int8_t smtc_modem_hal_get_temperature(void)
{
    // Return temperature in Celsius
    // ESP32 has internal temperature sensor, but it's not very accurate
    return 25; // Default room temperature
}

uint8_t smtc_modem_hal_get_voltage(void)
{
    // Return voltage in 1/50V units (e.g., 165 = 3.3V)
    // This needs to be implemented based on your voltage monitoring circuit
    return 165; // 3.3V default
}

int8_t smtc_modem_hal_get_board_delay_ms(void)
{
    // Return board-specific delay compensation
    return 1; // Typical value for ESP32
}

/* ------------ Trace management ------------*/
void smtc_modem_hal_print_trace(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    esp_log_writev(ESP_LOG_INFO, TAG, fmt, args);
    va_end(args);
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static void timer_callback(void* arg)
{
    if (lbm_timer.callback != NULL) {
        lbm_timer.callback(lbm_timer.context);
    }
}

static esp_err_t init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    nvs_initialized = true;
    return ESP_OK;
}
