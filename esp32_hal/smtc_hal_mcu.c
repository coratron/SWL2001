/*!
 * \file      smtc_hal_mcu.c
 *
 * \brief     MCU Hardware Abstraction Layer implementation for ESP32
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

#include "smtc_hal_mcu.h"
#include "smtc_hal_dbg_trace.h"
#include "smtc_hal_lp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_idf_version.h"

// Radio includes for SX127x
#if defined(SX1272) || defined(SX1276)
#include "sx127x.h"
#include "sx127x_esp_wrapper.h"
#include "smtc_modem_utilities.h"
#endif

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */
static const char *TAG = "HAL_MCU";

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */
static bool low_power_enabled = true;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void hal_mcu_critical_section_begin(uint32_t *mask)
{
    *mask = portSET_INTERRUPT_MASK_FROM_ISR();
}

void hal_mcu_critical_section_end(uint32_t *mask)
{
    portCLEAR_INTERRUPT_MASK_FROM_ISR(*mask);
}

void hal_mcu_disable_irq(void)
{
    portDISABLE_INTERRUPTS();
}

void hal_mcu_enable_irq(void)
{
    portENABLE_INTERRUPTS();
}

void hal_mcu_init(void)
{
    // Initialize any required ESP32-specific hardware
    // This is typically done in app_main() for ESP-IDF applications
    
    // Note: Logging is avoided here as this function may be called with interrupts disabled
    // Timer initialization will be done after interrupts are enabled
}

__attribute__((weak)) void hal_mcu_reset(void)
{
#ifdef CONFIG_LBM_USE_CUSTOM_MCU_RESET
    ESP_LOGW(TAG, "Custom MCU reset implementation should be provided. Using default fallback.");
#endif
    ESP_LOGE(TAG, "MCU reset requested");
    esp_restart();
}

void hal_mcu_wait_us(const int32_t microseconds)
{
    if (microseconds <= 0)
    {
        return;
    }

    if (microseconds < 1000)
    {
        // For short delays, use esp_rom_delay_us
        esp_rom_delay_us(microseconds);
    }
    else
    {
        // For longer delays, use FreeRTOS delay
        vTaskDelay(pdMS_TO_TICKS(microseconds / 1000));
    }
}

void hal_mcu_set_sleep_for_ms(const int32_t milliseconds)
{
    if (!low_power_enabled || milliseconds <= 0)
    {
        return;
    }

    // Note: Avoid logging here as this function may be called with interrupts disabled

    if (milliseconds < 10)
    {
        // For very short sleeps, just use a delay
        vTaskDelay(pdMS_TO_TICKS(milliseconds));
    }
    else
    {
        // ESP32 light sleep requires interrupts to be enabled
        // Check if interrupts are disabled and handle accordingly
        bool irq_was_disabled = false;
        
        // Check interrupt state by trying to disable them
        // If they're already disabled, this will be a no-op
        uint32_t irq_state = portSET_INTERRUPT_MASK_FROM_ISR();
        if (irq_state != 0) {
            // Interrupts were enabled, so we disabled them
            portCLEAR_INTERRUPT_MASK_FROM_ISR(irq_state);
        } else {
            // Interrupts were already disabled
            irq_was_disabled = true;
        }
        
        if (irq_was_disabled) {
            // Can't use light sleep with interrupts disabled, use delay instead
            vTaskDelay(pdMS_TO_TICKS(milliseconds));
        } else {
            // Configure light sleep
            esp_sleep_enable_timer_wakeup(milliseconds * 1000); // Convert to microseconds
            esp_light_sleep_start();
        }
    }
}

void hal_mcu_disable_low_power_wait(void)
{
    low_power_enabled = false;
    ESP_LOGD(TAG, "Low power mode disabled");
}

void hal_mcu_enable_low_power_wait(void)
{
    low_power_enabled = true;
    ESP_LOGD(TAG, "Low power mode enabled");
}

__attribute__((weak)) void hal_mcu_panic(void)
{
#ifdef CONFIG_LBM_USE_CUSTOM_MCU_RESET
    ESP_LOGW(TAG, "Custom MCU panic implementation should be provided. Using default fallback.");
#endif
    ESP_LOGE(TAG, "MCU panic requested - system will restart");
    esp_restart();
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

/* --- EOF ------------------------------------------------------------------ */
