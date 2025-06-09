/*!
 * \file      smtc_hal_rtc.c
 *
 * \brief     RTC Hardware Abstraction Layer implementation for ESP32
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
#include <time.h>
#include <sys/time.h>

#include "smtc_hal_rtc.h"
#include "esp_timer.h"
#include "esp_log.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

#define TAG "LBM_RTC"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static esp_timer_handle_t wakeup_timer_handle = NULL;
static bool rtc_initialized = false;
static uint32_t offset_to_test_wrapping = 0;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

static void wakeup_timer_callback(void *arg);

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void hal_rtc_init(void)
{
    if (!rtc_initialized)
    {
        // Create the wakeup timer
        esp_timer_create_args_t timer_args = {
            .callback = &wakeup_timer_callback,
            .arg = NULL,
            .name = "lbm_wakeup"};

        esp_err_t err = esp_timer_create(&timer_args, &wakeup_timer_handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to create wakeup timer: %s", esp_err_to_name(err));
        }

        rtc_initialized = true;
        ESP_LOGI(TAG, "RTC HAL initialized");
    }
}

uint32_t hal_rtc_get_time_s(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)tv.tv_sec;
}

uint32_t hal_rtc_get_time_ms(void)
{
    int64_t time_us = esp_timer_get_time();
    return (uint32_t)(time_us / 1000);
}

void hal_rtc_wakeup_timer_set_ms(const int32_t milliseconds)
{
    if (wakeup_timer_handle != NULL)
    {
        // Stop any existing timer
        esp_timer_stop(wakeup_timer_handle);

        if (milliseconds > 0)
        {
            // Start the timer
            esp_err_t err = esp_timer_start_once(wakeup_timer_handle, milliseconds * 1000);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to start wakeup timer: %s", esp_err_to_name(err));
            }
        }
    }
}

void hal_rtc_wakeup_timer_stop(void)
{
    if (wakeup_timer_handle != NULL)
    {
        esp_timer_stop(wakeup_timer_handle);
    }
}

void hal_rtc_set_offset_to_test_wrapping(uint32_t offset)
{
    offset_to_test_wrapping = offset;
}

uint32_t hal_rtc_get_offset_to_test_wrapping(void)
{
    return offset_to_test_wrapping;
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static void wakeup_timer_callback(void *arg)
{
    // This callback will be called when the wakeup timer expires
    // In a real implementation, this might trigger a task or event
    ESP_LOGD(TAG, "Wakeup timer expired");
}

/* --- EOF ------------------------------------------------------------------ */
