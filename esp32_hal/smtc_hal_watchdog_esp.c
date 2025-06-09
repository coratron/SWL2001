/*!
 * \file      smtc_hal_watchdog_esp.c
 *
 * \brief     Watchdog Hardware Abstraction Layer implementation for ESP32
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

#include <stdint.h>
#include <stdbool.h>

#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "smtc_hal_watchdog.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

#define WATCHDOG_TIMEOUT_S 30 // 30 seconds timeout

static const char *TAG = "WDT_HAL";

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static bool watchdog_initialized = false;
static TaskHandle_t current_task_handle = NULL;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void hal_watchdog_init(void)
{
    if (watchdog_initialized)
    {
        return;
    }

    esp_err_t ret;

    // Configure the task watchdog
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = WATCHDOG_TIMEOUT_S * 1000,
        .idle_core_mask = 0, // Don't watch idle tasks
        .trigger_panic = true};

    ret = esp_task_wdt_init(&wdt_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize task watchdog: %s", esp_err_to_name(ret));
        return;
    }

    // Add current task to watchdog
    current_task_handle = xTaskGetCurrentTaskHandle();
    ret = esp_task_wdt_add(current_task_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add current task to watchdog: %s", esp_err_to_name(ret));
        return;
    }

    watchdog_initialized = true;
    ESP_LOGI(TAG, "Watchdog initialized with %d second timeout", WATCHDOG_TIMEOUT_S);
}

void hal_watchdog_reload(void)
{
    if (!watchdog_initialized)
    {
        return;
    }

    esp_err_t ret = esp_task_wdt_reset();
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to reset watchdog: %s", esp_err_to_name(ret));
    }
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

/* --- EOF ------------------------------------------------------------------ */
