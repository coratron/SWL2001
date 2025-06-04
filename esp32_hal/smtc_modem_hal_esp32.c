/**
 * @file      smtc_modem_hal_esp32.c
 *
 * @brief     ESP32 implementation of SMTC Modem HAL functions
 *
 * Copyright (c) 2024
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdarg.h>   // va_list
#include <stdbool.h>  // bool type
#include <stdint.h>   // C99 types
#include <string.h>   // memcpy, memset

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "smtc_modem_hal.h"
#include "soc/rtc.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

#define NVS_NAMESPACE_MODEM       "lbm_modem"
#define NVS_NAMESPACE_LORAWAN     "lbm_lorawan"
#define NVS_NAMESPACE_FUOTA       "lbm_fuota"
#define NVS_NAMESPACE_SECURE_ELEM "lbm_se"
#define NVS_NAMESPACE_STORE_FWD   "lbm_sf"
#define NVS_NAMESPACE_CRASHLOG    "lbm_crash"

#define NVS_NAMESPACE             "lbm_hal"  // Default namespace

#define FLASH_PAGE_SIZE           4096
#define STORE_AND_FORWARD_PAGES   3

static const char* TAG = "smtc_modem_hal";
#define MAX_NVS_KEY_LENGTH 15

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/**
 * @brief HAL status type
 */
typedef enum smtc_modem_hal_status_e
{
    SMTC_MODEM_HAL_STATUS_OK = 0,
    SMTC_MODEM_HAL_STATUS_ERROR,
} smtc_modem_hal_status_t;

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
static nvs_handle_t nvs_hal_handle;
static bool nvs_initialized = false;
static portMUX_TYPE modem_irq_mux = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE irq_mux = portMUX_INITIALIZER_UNLOCKED;

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
    if(lbm_timer.timer_handle != NULL)
    {
        esp_timer_stop(lbm_timer.timer_handle);
    }

    lbm_timer.callback = callback;
    lbm_timer.context = context;

    if(lbm_timer.timer_handle == NULL)
    {
        esp_timer_create_args_t timer_args = {
            .callback = timer_callback, .arg = NULL, .dispatch_method = ESP_TIMER_TASK, .name = "lbm_timer"};
        ESP_ERROR_CHECK(esp_timer_create(&timer_args, &lbm_timer.timer_handle));
    }

    ESP_ERROR_CHECK(esp_timer_start_once(lbm_timer.timer_handle, milliseconds * 1000ULL));
}

void smtc_modem_hal_stop_timer(void)
{
    if(lbm_timer.timer_handle != NULL)
    {
        esp_timer_stop(lbm_timer.timer_handle);
    }
}

/* ------------ IRQ management ------------*/
void smtc_modem_hal_disable_modem_irq(void)
{
    taskENTER_CRITICAL(&irq_mux);
}

void smtc_modem_hal_enable_modem_irq(void)
{
    taskEXIT_CRITICAL(&irq_mux);
}

/* ------------ Context saving management ------------*/
void smtc_modem_hal_context_restore(const modem_context_type_t ctx_type, uint32_t offset, uint8_t* buffer, const uint32_t size)
{
    if(!nvs_initialized)
    {
        if(init_nvs() != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to initialize NVS");
            return;
        }
    }

    char key[MAX_NVS_KEY_LENGTH];
    snprintf(key, sizeof(key), "ctx_%d_%lu", ctx_type, offset);

    size_t required_size = size;
    esp_err_t err = nvs_get_blob(nvs_hal_handle, key, buffer, &required_size);

    if(err != ESP_OK || required_size != size)
    {
        ESP_LOGW(TAG, "Context restore failed for key %s, initializing with zeros", key);
        memset(buffer, 0, size);
    }
}

void smtc_modem_hal_context_store(const modem_context_type_t ctx_type, uint32_t offset, const uint8_t* buffer, const uint32_t size)
{
    if(!nvs_initialized)
    {
        if(init_nvs() != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to initialize NVS");
            return;
        }
    }

    char key[MAX_NVS_KEY_LENGTH];
    snprintf(key, sizeof(key), "ctx_%d_%lu", ctx_type, offset);

    esp_err_t err = nvs_set_blob(nvs_hal_handle, key, buffer, size);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to store context for key %s", key);
        return;
    }

    err = nvs_commit(nvs_hal_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to commit NVS");
    }
}

/* ------------ Crashlog management ------------*/
void smtc_modem_hal_crashlog_store(const uint8_t* crash_string, uint8_t crash_string_length)
{
    if(!nvs_initialized)
    {
        if(init_nvs() != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to initialize NVS");
            return;
        }
    }

    ESP_LOGI(TAG, "Storing crashlog (%d bytes)", crash_string_length);

    // Store the crashlog data
    esp_err_t err = nvs_set_blob(nvs_hal_handle, "crashlog_data", crash_string, crash_string_length);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to store crashlog data");
        return;
    }

    // Store the crashlog length
    err = nvs_set_u8(nvs_hal_handle, "crashlog_len", crash_string_length);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to store crashlog length");
        return;
    }

    // Set status to available
    smtc_modem_hal_crashlog_set_status(true);

    err = nvs_commit(nvs_hal_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to commit crashlog to NVS");
    }
}

void smtc_modem_hal_crashlog_restore(uint8_t* crash_string, uint8_t* crash_string_length)
{
    if(!nvs_initialized)
    {
        if(init_nvs() != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to initialize NVS");
            return;
        }
    }

    ESP_LOGI(TAG, "Retrieving crashlog");

    // Get the crashlog length first
    uint8_t length = 0;
    esp_err_t err = nvs_get_u8(nvs_hal_handle, "crashlog_len", &length);
    if(err != ESP_OK)
    {
        ESP_LOGW(TAG, "No crashlog length found");
        *crash_string_length = 0;
        return;
    }

    // Limit length to maximum size
    length = (length > CRASH_LOG_SIZE) ? CRASH_LOG_SIZE : length;

    // Get the crashlog data
    size_t required_size = length;
    err = nvs_get_blob(nvs_hal_handle, "crashlog_data", crash_string, &required_size);
    if(err != ESP_OK || required_size != length)
    {
        ESP_LOGW(TAG, "Failed to retrieve crashlog data");
        *crash_string_length = 0;
        return;
    }

    *crash_string_length = length;
}

void smtc_modem_hal_crashlog_set_status(bool available)
{
    if(!nvs_initialized)
    {
        if(init_nvs() != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to initialize NVS");
            return;
        }
    }

    uint8_t status = available ? 1 : 0;
    esp_err_t err = nvs_set_u8(nvs_hal_handle, "crashlog_avail", status);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to store crashlog status");
        return;
    }

    err = nvs_commit(nvs_hal_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to commit crashlog status to NVS");
    }
}

bool smtc_modem_hal_crashlog_get_status(void)
{
    if(!nvs_initialized)
    {
        if(init_nvs() != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to initialize NVS");
            return false;
        }
    }

    uint8_t status = 0;
    esp_err_t err = nvs_get_u8(nvs_hal_handle, "crashlog_avail", &status);
    if(err != ESP_OK)
    {
        return false;
    }

    return (status == 1);
}

/* ------------ assert management ------------*/
void smtc_modem_hal_on_panic(uint8_t* func, uint32_t line, const char* fmt, ...)
{
    uint8_t out_buff[255] = {0};
    uint8_t out_len = snprintf((char*)out_buff, sizeof(out_buff), "%s:%lu ", func, line);

    va_list args;
    va_start(args, fmt);
    out_len += vsnprintf((char*)&out_buff[out_len], sizeof(out_buff) - out_len, fmt, args);
    va_end(args);

    // Store crash information before reset
    smtc_modem_hal_crashlog_store(out_buff, out_len);

    ESP_LOGE(TAG, "Modem panic: %s", out_buff);

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

    if(min_val == max_val)
    {
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
    return 5;  // Typical value, adjust based on your TCXO
}

/* ------------ Environment management ------------*/
uint8_t smtc_modem_hal_get_battery_level(void)
{
    // Return battery level percentage (0-254, 255 = unknown)
    // This needs to be implemented based on your battery monitoring circuit
    return 255;  // Unknown by default
}

int8_t smtc_modem_hal_get_temperature(void)
{
    // Return temperature in Celsius
    // ESP32 has internal temperature sensor, but it's not very accurate
    return 25;  // Default room temperature
}

uint8_t smtc_modem_hal_get_voltage(void)
{
    // Return voltage in 1/50V units (e.g., 165 = 3.3V)
    // This needs to be implemented based on your voltage monitoring circuit
    return 165;  // 3.3V default
}

uint16_t smtc_modem_hal_get_voltage_mv(void)
{
    // Return voltage in millivolts
    // For ESP32, we can use ADC to read voltage
    // This is a basic implementation - adjust based on your voltage divider circuit
    return 3300;  // 3.3V default in millivolts
}

void smtc_modem_hal_set_ant_switch(bool is_tx_on)
{
// Set antenna switch for TX/RX
// Implementation depends on your board's antenna switch design
// This is typically controlled by GPIO pins

// Example implementation (adjust GPIO pins based on your board):
// GPIO pin for TX/RX control (set via Kconfig or define)
#ifndef CONFIG_LBM_ANT_SWITCH_PIN
    #define CONFIG_LBM_ANT_SWITCH_PIN -1  // Disabled by default
#endif

    if(CONFIG_LBM_ANT_SWITCH_PIN >= 0)
    {
        gpio_set_level(CONFIG_LBM_ANT_SWITCH_PIN, is_tx_on ? 1 : 0);
    }
}

int8_t smtc_modem_hal_get_board_delay_ms(void)
{
    // Return board-specific delay compensation
    return 1;  // Typical value for ESP32
}

/* ------------ FUOTA management ------------*/
uint32_t smtc_modem_hal_fuota_get_allocated_memory(void)
{
    // Return allocated memory for FUOTA in bytes
    // This depends on your application's available memory
    return 64 * 1024;  // 64KB default
}

smtc_modem_hal_status_t smtc_modem_hal_fuota_store(uint32_t addr, const uint8_t* data, uint32_t size)
{
    // Store FUOTA data to flash
    // Implementation depends on your flash partitioning scheme
    ESP_LOGW(TAG, "FUOTA store not implemented - addr: 0x%08lx, size: %lu", addr, size);
    return SMTC_MODEM_HAL_STATUS_OK;
}

smtc_modem_hal_status_t smtc_modem_hal_fuota_read(uint32_t addr, uint8_t* data, uint32_t size)
{
    // Read FUOTA data from flash
    // Implementation depends on your flash partitioning scheme
    ESP_LOGW(TAG, "FUOTA read not implemented - addr: 0x%08lx, size: %lu", addr, size);
    return SMTC_MODEM_HAL_STATUS_OK;
}

smtc_modem_hal_status_t smtc_modem_hal_fuota_start_install(void)
{
    // Start FUOTA installation process
    ESP_LOGW(TAG, "FUOTA install not implemented");
    return SMTC_MODEM_HAL_STATUS_OK;
}

/* ------------ Store and Forward management ------------*/
smtc_modem_hal_status_t smtc_modem_hal_store_and_forward_flash_clear_pending_data(void)
{
    // Clear pending store and forward data
    ESP_LOGW(TAG, "Store and forward clear not implemented");
    return SMTC_MODEM_HAL_STATUS_OK;
}

uint16_t smtc_modem_hal_store_and_forward_get_number_of_pages(void)
{
    // Return number of pages available for store and forward
    return STORE_AND_FORWARD_PAGES;
}

uint16_t smtc_modem_hal_store_and_forward_get_page_size(void)
{
    // Return page size for store and forward
    return FLASH_PAGE_SIZE;
}

smtc_modem_hal_status_t smtc_modem_hal_store_and_forward_flash_erase_page(uint16_t page_id)
{
    // Erase a page for store and forward
    ESP_LOGW(TAG, "Store and forward erase page %d not implemented", page_id);
    return SMTC_MODEM_HAL_STATUS_OK;
}

smtc_modem_hal_status_t smtc_modem_hal_store_and_forward_flash_write(uint16_t page_id, uint16_t offset, const uint8_t* data, uint16_t size)
{
    // Write data to store and forward flash
    ESP_LOGW(TAG, "Store and forward write not implemented - page: %d, offset: %d, size: %d", page_id, offset, size);
    return SMTC_MODEM_HAL_STATUS_OK;
}

smtc_modem_hal_status_t smtc_modem_hal_store_and_forward_flash_read(uint16_t page_id, uint16_t offset, uint8_t* data, uint16_t size)
{
    // Read data from store and forward flash
    ESP_LOGW(TAG, "Store and forward read not implemented - page: %d, offset: %d, size: %d", page_id, offset, size);
    return SMTC_MODEM_HAL_STATUS_OK;
}

/* ------------ Trace management ------------*/
void smtc_modem_hal_print_trace(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    esp_log_writev(ESP_LOG_INFO, TAG, fmt, args);
    va_end(args);
}

/* ------------ User LBM IRQ management ------------*/
void smtc_modem_hal_user_lbm_irq(void)
{
    // This function should be called from the radio planner when an IRQ occurs
    // In this ESP32 implementation, it's handled by the timer and radio IRQ callbacks
    // No additional implementation needed here as the interrupts are handled directly
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static void timer_callback(void* arg)
{
    if(lbm_timer.callback != NULL)
    {
        lbm_timer.callback(lbm_timer.context);
    }
}

static esp_err_t init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_hal_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    nvs_initialized = true;
    return ESP_OK;
}
