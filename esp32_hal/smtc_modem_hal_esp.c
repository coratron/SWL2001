/*!
 * \file      smtc_modem_hal_esp.c
 *
 * \brief     Modem Hardware Abstraction Layer API implementation for ESP32.
 *
 * The Clear BSD License
 * Copyright Semtech Corporation 2021. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Semtech corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdint.h>  // C99 types
#include <stdbool.h> // bool type
#include <stdio.h>   // for variadic args
#include <stdarg.h>  // for variadic args
#include <string.h>  // for memcpy

#include "smtc_modem_hal.h"
#include "smtc_hal_dbg_trace.h"
#include "lorawan_session/lorawan_session_context.h"

#include "smtc_hal_gpio.h"
#include "smtc_hal_lp_timer.h"
#include "smtc_hal_mcu.h"
#include "smtc_hal_rng.h"
#include "smtc_hal_rtc.h"
#include "smtc_hal_trace.h"
#include "smtc_hal_uart.h"
#include "smtc_hal_watchdog.h"

#include "modem_pinout.h"

// ESP32 specific includes
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if defined(SX1272) || defined(SX1276)
#include "smtc_modem_utilities.h"
#include "sx127x.h"
#endif

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

#define NVS_NAMESPACE "lbm_modem"
#define NVS_KEY_MODEM_CONTEXT "modem_ctx"
#define NVS_KEY_MODEM_KEY_CONTEXT "modem_key"
#define NVS_KEY_LORAWAN_CONTEXT "lorawan_ctx"
#define NVS_KEY_SECURE_ELEMENT_CONTEXT "se_ctx"
#define NVS_KEY_LORAWAN_SESSION_CONTEXT "lbm_session"
#define NVS_KEY_CRASHLOG "crashlog"
#define NVS_KEY_CRASHLOG_STATUS "crash_stat"

static const char *TAG = "smtc_modem_hal";

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

#if !defined(SX127X)
static hal_gpio_irq_t radio_dio_irq;
#endif

// Crashlog storage in RTC memory (survives resets but not power cycles)
RTC_DATA_ATTR static uint8_t crashlog_buff_rtc[CRASH_LOG_SIZE];
RTC_DATA_ATTR static volatile uint8_t crashlog_length_rtc;
RTC_DATA_ATTR static volatile bool crashlog_available_rtc;

// LoRaWAN session context storage in RTC memory (fast access with battery backup)
RTC_DATA_ATTR static lorawan_session_context_t session_context_rtc;
RTC_DATA_ATTR static volatile bool session_context_valid_rtc;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

static esp_err_t nvs_write_blob_safe(const char *key, const void *data, size_t length);
static esp_err_t nvs_read_blob_safe(const char *key, void *data, size_t *length);
static void session_context_store_to_rtc(const uint8_t *buffer, const uint32_t size);
static void session_context_restore_from_rtc(uint8_t *buffer, const uint32_t size);
static void session_context_sync_to_nvs(void);
static void session_context_restore_from_nvs(void);

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

/* ------------ Reset management ------------*/
__attribute__((weak)) void smtc_modem_hal_reset_mcu(void)
{
#ifdef CONFIG_LBM_USE_CUSTOM_MCU_RESET
    ESP_LOGW(TAG, "Custom MCU reset implementation should be provided. Using default fallback.");
#endif
    hal_mcu_reset();
}

/* ------------ Watchdog management ------------*/

void smtc_modem_hal_reload_wdog(void)
{
    hal_watchdog_reload();
}

/* ------------ Time management ------------*/

uint32_t smtc_modem_hal_get_time_in_s(void)
{
    return hal_rtc_get_time_s();
}

uint32_t smtc_modem_hal_get_time_in_ms(void)
{
    return hal_rtc_get_time_ms();
}

void smtc_modem_hal_set_offset_to_test_wrapping(const uint32_t offset_to_test_wrapping)
{
    hal_rtc_set_offset_to_test_wrapping(offset_to_test_wrapping);
}

/* ------------ Timer management ------------*/

void smtc_modem_hal_start_timer(const uint32_t milliseconds, void (*callback)(void *context), void *context)
{
    hal_lp_timer_start(HAL_LP_TIMER_ID_1, milliseconds,
                       &(hal_lp_timer_irq_t){.context = context, .callback = callback});
}

void smtc_modem_hal_stop_timer(void)
{
    hal_lp_timer_stop(HAL_LP_TIMER_ID_1);
}

/* ------------ IRQ management ------------*/

void smtc_modem_hal_disable_modem_irq(void)
{
    hal_gpio_irq_disable();
    hal_lp_timer_irq_disable(HAL_LP_TIMER_ID_1);
#if (SX127X)
    hal_lp_timer_irq_disable(HAL_LP_TIMER_ID_2);
#endif
}

void smtc_modem_hal_enable_modem_irq(void)
{
    hal_gpio_irq_enable();
    hal_lp_timer_irq_enable(HAL_LP_TIMER_ID_1);
#if (SX127X)
    hal_lp_timer_irq_enable(HAL_LP_TIMER_ID_2);
#endif
}

/* ------------ Context saving management ------------*/

void smtc_modem_hal_context_restore(const modem_context_type_t ctx_type, uint32_t offset, uint8_t *buffer,
                                    const uint32_t size)
{
    const char *nvs_key = NULL;

    switch (ctx_type)
    {
    case CONTEXT_MODEM:
        nvs_key = NVS_KEY_MODEM_CONTEXT;
        break;
    case CONTEXT_KEY_MODEM:
        nvs_key = NVS_KEY_MODEM_KEY_CONTEXT;
        break;
    case CONTEXT_LORAWAN_STACK:
        nvs_key = NVS_KEY_LORAWAN_CONTEXT;
        break;
    case CONTEXT_SECURE_ELEMENT:
        nvs_key = NVS_KEY_SECURE_ELEMENT_CONTEXT;
        break;
    case CONTEXT_FUOTA:
        // Not implemented for ESP32 yet
        ESP_LOGW(TAG, "FUOTA context restore not implemented");
        memset(buffer, 0, size);
        return;
    case CONTEXT_STORE_AND_FORWARD:
        // Not implemented for ESP32 yet
        ESP_LOGW(TAG, "Store and Forward context restore not implemented");
        memset(buffer, 0, size);
        return;
    case CONTEXT_LORAWAN_SESSION:
        // Restore from RTC memory (primary) with NVS fallback
        session_context_restore_from_rtc(buffer, size);
        return;
    default:
        ESP_LOGE(TAG, "Unknown context type: %d", ctx_type);
        hal_mcu_panic();
        return;
    }

    size_t actual_size = size;
    esp_err_t err = nvs_read_blob_safe(nvs_key, buffer, &actual_size);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to restore context %s: %s", nvs_key, esp_err_to_name(err));
        memset(buffer, 0, size);
    }
}

void smtc_modem_hal_context_store(const modem_context_type_t ctx_type, uint32_t offset, const uint8_t *buffer,
                                  const uint32_t size)
{
    const char *nvs_key = NULL;

    switch (ctx_type)
    {
    case CONTEXT_MODEM:
        nvs_key = NVS_KEY_MODEM_CONTEXT;
        break;
    case CONTEXT_KEY_MODEM:
        nvs_key = NVS_KEY_MODEM_KEY_CONTEXT;
        break;
    case CONTEXT_LORAWAN_STACK:
        nvs_key = NVS_KEY_LORAWAN_CONTEXT;
        break;
    case CONTEXT_SECURE_ELEMENT:
        nvs_key = NVS_KEY_SECURE_ELEMENT_CONTEXT;
        break;
    case CONTEXT_FUOTA:
        // Not implemented for ESP32 yet
        ESP_LOGW(TAG, "FUOTA context store not implemented");
        return;
    case CONTEXT_STORE_AND_FORWARD:
        // Not implemented for ESP32 yet
        ESP_LOGW(TAG, "Store and Forward context store not implemented");
        return;
    case CONTEXT_LORAWAN_SESSION:
        // Store to RTC memory (primary) and sync to NVS (backup)
        session_context_store_to_rtc(buffer, size);
        return;
    default:
        ESP_LOGE(TAG, "Unknown context type: %d", ctx_type);
        hal_mcu_panic();
        return;
    }

    esp_err_t err = nvs_write_blob_safe(nvs_key, buffer, size);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to store context %s: %s", nvs_key, esp_err_to_name(err));
    }
}

void smtc_modem_hal_context_flash_pages_erase(const modem_context_type_t ctx_type, uint32_t offset, uint8_t nb_page)
{
    switch (ctx_type)
    {
    case CONTEXT_STORE_AND_FORWARD:
        ESP_LOGW(TAG, "Store and Forward flash erase not implemented");
        break;
    default:
        ESP_LOGE(TAG, "Flash erase not supported for context type: %d", ctx_type);
        hal_mcu_panic();
        break;
    }
}

/* ------------ crashlog management ------------*/

void smtc_modem_hal_crashlog_store(const uint8_t *crash_string, uint8_t crash_string_length)
{
    crashlog_length_rtc = MIN(crash_string_length, CRASH_LOG_SIZE);
    memcpy(crashlog_buff_rtc, crash_string, crashlog_length_rtc);
    crashlog_available_rtc = true;

    // Also store in NVS for persistence across power cycles
    nvs_write_blob_safe(NVS_KEY_CRASHLOG, crashlog_buff_rtc, crashlog_length_rtc);
    uint8_t status = 1;
    nvs_write_blob_safe(NVS_KEY_CRASHLOG_STATUS, &status, sizeof(status));
}

void smtc_modem_hal_crashlog_restore(uint8_t *crash_string, uint8_t *crash_string_length)
{
    *crash_string_length = (crashlog_length_rtc > CRASH_LOG_SIZE) ? CRASH_LOG_SIZE : crashlog_length_rtc;
    memcpy(crash_string, crashlog_buff_rtc, *crash_string_length);
}

void smtc_modem_hal_crashlog_set_status(bool available)
{
    crashlog_available_rtc = available;
    uint8_t status = available ? 1 : 0;
    nvs_write_blob_safe(NVS_KEY_CRASHLOG_STATUS, &status, sizeof(status));
}

bool smtc_modem_hal_crashlog_get_status(void)
{
    return crashlog_available_rtc;
}

/* ------------ assert management ------------*/

void smtc_modem_hal_on_panic(uint8_t *func, uint32_t line, const char *fmt, ...)
{
    uint8_t out_buff[255] = {0};
    uint8_t out_len = snprintf((char *)out_buff, sizeof(out_buff), "%s:%lu ", func, line);

    va_list args;
    va_start(args, fmt);
    out_len += vsprintf((char *)&out_buff[out_len], fmt, args);
    va_end(args);

    smtc_modem_hal_crashlog_store(out_buff, out_len);

    SMTC_HAL_TRACE_ERROR("Modem panic: %s\n", out_buff);
    ESP_LOGE(TAG, "Modem panic: %s", out_buff);
    smtc_modem_hal_reset_mcu();
}

/* ------------ Random management ------------*/

uint32_t smtc_modem_hal_get_random_nb_in_range(const uint32_t val_1, const uint32_t val_2)
{
    return hal_rng_get_random_in_range(val_1, val_2);
}

/* ------------ Radio env management ------------*/

void smtc_modem_hal_irq_config_radio_irq(void (*callback)(void *context), void *context)
{
#if defined(SX1272) || defined(SX1276)
    sx127x_t *radio = (sx127x_t *)smtc_modem_get_radio_context();
    sx127x_irq_attach(radio, callback, context);
#else
    radio_dio_irq.pin = RADIO_DIOX;
    radio_dio_irq.callback = callback;
    radio_dio_irq.context = context;
    hal_gpio_irq_attach(&radio_dio_irq);
#endif
}

void smtc_modem_hal_start_radio_tcxo(void)
{
    // SX127x typically doesn't have TCXO control
    // Implement if your board has TCXO control
}

void smtc_modem_hal_stop_radio_tcxo(void)
{
    // SX127x typically doesn't have TCXO control
    // Implement if your board has TCXO control
}

uint32_t smtc_modem_hal_get_radio_tcxo_startup_delay_ms(void)
{
    // SX127x typically doesn't have TCXO
    return 0;
}

void smtc_modem_hal_set_ant_switch(bool is_tx_on)
{
#if !(defined(SX1272) || defined(SX1276))
    hal_gpio_set_value(RADIO_ANTENNA_SWITCH, (is_tx_on == true) ? 1 : 0);
#endif
}

/* ------------ Environment management ------------*/

uint8_t smtc_modem_hal_get_battery_level(void)
{
    // ESP32 implementation - you may want to implement actual battery monitoring
    // According to LoRaWan 1.0.4 spec:
    // 0: The end-device is connected to an external power source.
    // 1..254: Battery level, where 1 is the minimum and 254 is the maximum.
    // 255: The end-device was not able to measure the battery level.
    return 0; // Assume external power source for now
}

int8_t smtc_modem_hal_get_board_delay_ms(void)
{
    // The board delay is the time needed between calling ral_set_tx()/ral_set_rx()
    // and the radio actually entering TX/RX state. This depends on MCU speed and SPI bus speed.
    // For ESP32 with SX127x, this should be 1-2ms, not 500ms.
    ESP_LOGI(TAG, "Board delay: 2ms");
    return 2; // Actual board/SPI delay - typically 1-2ms for ESP32
}

/* ------------ Trace management ------------*/

void smtc_modem_hal_print_trace(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    hal_trace_print(fmt, args);
    va_end(args);
}

/* ------------ Fuota management ------------*/

#if defined(USE_FUOTA)
uint32_t smtc_modem_hal_get_hw_version_for_fuota(void)
{
    // ESP32 hardware version - customize as needed
    return 0x45535033; // "ESP3" in hex
}

uint32_t smtc_modem_hal_get_fw_version_for_fuota(void)
{
    // Firmware version - customize as needed
    return 0x01000000; // Version 1.0.0.0
}

uint8_t smtc_modem_hal_get_fw_status_available_for_fuota(void)
{
    // Firmware status - customize as needed
    return 3;
}

uint32_t smtc_modem_hal_get_next_fw_version_for_fuota(void)
{
    // Next firmware version - customize as needed
    return 0x01010000; // Version 1.1.0.0
}

uint8_t smtc_modem_hal_get_fw_delete_status_for_fuota(uint32_t fw_to_delete_version)
{
    if (fw_to_delete_version != smtc_modem_hal_get_next_fw_version_for_fuota())
    {
        return 2;
    }
    else
    {
        return 0;
    }
}
#endif // USE_FUOTA

/* ------------ Needed for Cloud  ------------*/

int8_t smtc_modem_hal_get_temperature(void)
{
    // ESP32 temperature sensor - you may want to implement actual temperature reading
    return 25; // Default room temperature
}

uint16_t smtc_modem_hal_get_voltage_mv(void)
{
    // ESP32 voltage - you may want to implement actual voltage reading
    return 3300; // 3.3V typical
}

/* ------------ Needed for Store and Forward service  ------------*/
#if defined(USE_STORE_AND_FORWARD)
uint16_t smtc_modem_hal_store_and_forward_get_number_of_pages(void)
{
    // ESP32 flash page configuration for store and forward
    return 10; // Customize based on your flash allocation
}

uint16_t smtc_modem_hal_flash_get_page_size(void)
{
    // ESP32 flash page size
    return 4096; // ESP32 flash sector size
}
#endif

/* ------------ For Real Time OS compatibility  ------------*/

// Task notification mechanism for immediate timer callback handling
static TaskHandle_t main_task_handle = NULL;

void smtc_modem_hal_set_main_task_handle(TaskHandle_t task_handle)
{
    main_task_handle = task_handle;
    ESP_LOGI(TAG, "Main task handle set for immediate timer notifications");
}

void smtc_modem_hal_user_lbm_irq(void)
{
    // Immediately notify the main task that a timer IRQ has occurred
    if (main_task_handle != NULL)
    {
        // Notify from any context (ISR or task)
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(main_task_handle, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR();
        }
    }
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static esp_err_t nvs_write_blob_safe(const char *key, const void *data, size_t length)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_set_blob(nvs_handle, key, data, length);
    if (err == ESP_OK)
    {
        err = nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);
    return err;
}

static esp_err_t nvs_read_blob_safe(const char *key, void *data, size_t *length)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_get_blob(nvs_handle, key, data, length);
    nvs_close(nvs_handle);
    return err;
}

/*
 * -----------------------------------------------------------------------------
 * --- SESSION CONTEXT HELPER FUNCTIONS ---------------------------------------
 */

static void session_context_store_to_rtc(const uint8_t *buffer, const uint32_t size)
{
    if (buffer == NULL || size != sizeof(lorawan_session_context_t))
    {
        ESP_LOGE(TAG, "Invalid session context data (size: %lu, expected: %zu)", 
                 size, sizeof(lorawan_session_context_t));
        return;
    }
    
    // Copy data to RTC memory
    memcpy(&session_context_rtc, buffer, sizeof(lorawan_session_context_t));
    
    // Validate the context
    uint32_t current_time = smtc_modem_hal_get_time_in_s();
    lorawan_session_validation_t validation = lorawan_session_validate_context(&session_context_rtc, current_time);
    
    if (validation == LORAWAN_SESSION_VALID)
    {
        session_context_valid_rtc = true;
        ESP_LOGI(TAG, "Session context stored to RTC memory (valid)");
        
        // NVS sync disabled - RTC memory sufficient for deep sleep cycling
        // static uint32_t save_counter = 0;
        // static join_status_t last_join_status = NOT_JOINED;
        // 
        // save_counter++;
        // if (save_counter >= 10 || session_context_rtc.join_status != last_join_status)
        // {
        //     session_context_sync_to_nvs();
        //     save_counter = 0;
        //     last_join_status = session_context_rtc.join_status;
        // }
    }
    else
    {
        session_context_valid_rtc = false;
        ESP_LOGW(TAG, "Session context stored but validation failed: %d", validation);
    }
}

static void session_context_restore_from_rtc(uint8_t *buffer, const uint32_t size)
{
    if (buffer == NULL || size != sizeof(lorawan_session_context_t))
    {
        ESP_LOGE(TAG, "Invalid session context buffer (size: %lu, expected: %zu)", 
                 size, sizeof(lorawan_session_context_t));
        memset(buffer, 0, size);
        return;
    }
    
    // Check if RTC memory contains valid session data
    if (session_context_valid_rtc)
    {
        uint32_t current_time = smtc_modem_hal_get_time_in_s();
        lorawan_session_validation_t validation = lorawan_session_validate_context(&session_context_rtc, current_time);
        
        if (validation == LORAWAN_SESSION_VALID)
        {
            // RTC memory context is valid, use it
            memcpy(buffer, &session_context_rtc, sizeof(lorawan_session_context_t));
            ESP_LOGI(TAG, "Session context restored from RTC memory");
            return;
        }
        else
        {
            ESP_LOGW(TAG, "RTC session context invalid: %d, trying NVS backup", validation);
        }
    }
    
    // NVS fallback disabled - RTC memory only
    // session_context_restore_from_nvs();
    // 
    // // Check if NVS restore was successful
    // if (session_context_valid_rtc)
    // {
    //     uint32_t current_time = smtc_modem_hal_get_time_in_s();
    //     lorawan_session_validation_t validation = lorawan_session_validate_context(&session_context_rtc, current_time);
    //     
    //     if (validation == LORAWAN_SESSION_VALID)
    //     {
    //         memcpy(buffer, &session_context_rtc, sizeof(lorawan_session_context_t));
    //         ESP_LOGI(TAG, "Session context restored from NVS backup");
    //         return;
    //     }
    // }
    
    // RTC failed, return default context (NVS fallback disabled)
    ESP_LOGW(TAG, "RTC session context invalid, returning default context");
    lorawan_session_init_context((lorawan_session_context_t*)buffer);
}

static void session_context_sync_to_nvs(void)
{
    // NVS sync disabled - RTC memory sufficient for deep sleep cycling
    // if (!session_context_valid_rtc)
    // {
    //     ESP_LOGW(TAG, "Cannot sync invalid session context to NVS");
    //     return;
    // }
    // 
    // // Update timestamp before saving to NVS
    // session_context_rtc.last_save_timestamp = smtc_modem_hal_get_time_in_s();
    // session_context_rtc.save_counter++;
    // 
    // // Recalculate CRC
    // session_context_rtc.crc32 = lorawan_session_calculate_crc(&session_context_rtc);
    // 
    // esp_err_t err = nvs_write_blob_safe(NVS_KEY_LORAWAN_SESSION_CONTEXT, 
    //                                     &session_context_rtc, sizeof(lorawan_session_context_t));
    // if (err == ESP_OK)
    // {
    //     ESP_LOGI(TAG, "Session context synced to NVS backup");
    // }
    // else
    // {
    //     ESP_LOGE(TAG, "Failed to sync session context to NVS: %s", esp_err_to_name(err));
    // }
}

static void session_context_restore_from_nvs(void)
{
    // NVS restore disabled - RTC memory only
    // size_t actual_size = sizeof(lorawan_session_context_t);
    // esp_err_t err = nvs_read_blob_safe(NVS_KEY_LORAWAN_SESSION_CONTEXT, 
    //                                    &session_context_rtc, &actual_size);
    // 
    // if (err == ESP_OK && actual_size == sizeof(lorawan_session_context_t))
    // {
    //     uint32_t current_time = smtc_modem_hal_get_time_in_s();
    //     lorawan_session_validation_t validation = lorawan_session_validate_context(&session_context_rtc, current_time);
    //     
    //     if (validation == LORAWAN_SESSION_VALID)
    //     {
    //         session_context_valid_rtc = true;
    //         ESP_LOGI(TAG, "Session context restored from NVS backup");
    //     }
    //     else
    //     {
    //         session_context_valid_rtc = false;
    //         ESP_LOGW(TAG, "NVS session context validation failed: %d", validation);
    //     }
    // }
    // else
    // {
    //     session_context_valid_rtc = false;
    //     ESP_LOGW(TAG, "Failed to restore session context from NVS: %s", esp_err_to_name(err));
    //     
    //     // Initialize with default values
    //     lorawan_session_init_context(&session_context_rtc);
    // }
}

/* ------------ Session management ------------*/

bool smtc_modem_hal_should_preserve_session(void)
{
    esp_reset_reason_t reset_reason = esp_reset_reason();
    
    ESP_LOGI(TAG, "Reset reason check for session preservation: %d (%s)", reset_reason,
             reset_reason == ESP_RST_POWERON ? "POWER_ON" :
             reset_reason == ESP_RST_EXT ? "EXTERNAL_RESET" :
             reset_reason == ESP_RST_SW ? "SOFTWARE_RESET" :
             reset_reason == ESP_RST_PANIC ? "PANIC_RESET" :
             reset_reason == ESP_RST_INT_WDT ? "INTERRUPT_WDT" :
             reset_reason == ESP_RST_TASK_WDT ? "TASK_WDT" :
             reset_reason == ESP_RST_WDT ? "OTHER_WDT" :
             reset_reason == ESP_RST_DEEPSLEEP ? "DEEP_SLEEP_WAKEUP" :
             reset_reason == ESP_RST_BROWNOUT ? "BROWNOUT" :
             reset_reason == ESP_RST_SDIO ? "SDIO" :
             "UNKNOWN");
    
    // Only preserve session on deep sleep wakeup
    // All other reset types (power-on, software reset, crash, etc.) should start fresh
    bool should_preserve = (reset_reason == ESP_RST_DEEPSLEEP);
    
    ESP_LOGI(TAG, "Session preservation decision: %s", should_preserve ? "PRESERVE" : "FRESH_JOIN");
    
    return should_preserve;
}

/* --- EOF ------------------------------------------------------------------ */
