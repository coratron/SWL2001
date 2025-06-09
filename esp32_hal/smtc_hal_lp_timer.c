/*!
 * \file      smtc_hal_lp_timer.c
 *
 * \brief     Low Power Timer Hardware Abstraction Layer implementation for ESP32
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
#include <string.h>

#include "smtc_hal_lp_timer.h"
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
#define MAX_TIMER_COUNT 2
#define TAG "LBM_LP_TIMER"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

typedef struct
{
    esp_timer_handle_t handle;
    hal_lp_timer_irq_t irq_context;
    bool initialized;
    bool running;
} lp_timer_context_t;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static lp_timer_context_t timers[MAX_TIMER_COUNT] = {0};

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

static void timer_callback(void *arg);

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void hal_lp_timer_init(hal_lp_timer_id_t id)
{
    if (id >= MAX_TIMER_COUNT)
    {
        ESP_LOGE(TAG, "Invalid timer ID: %d", id);
        return;
    }

    lp_timer_context_t *timer = &timers[id];

    if (timer->initialized)
    {
        ESP_LOGW(TAG, "Timer %d already initialized", id);
        return;
    }

    const esp_timer_create_args_t timer_args = {
        .callback = timer_callback,
        .arg = &timers[id],
        .dispatch_method = ESP_TIMER_TASK,
        .name = (id == HAL_LP_TIMER_ID_1) ? "lbm_timer_1" : "lbm_timer_2",
        .skip_unhandled_events = false};

    esp_err_t err = esp_timer_create(&timer_args, &timer->handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create timer %d: %s", id, esp_err_to_name(err));
        return;
    }

    timer->initialized = true;
    timer->running = false;
    memset(&timer->irq_context, 0, sizeof(hal_lp_timer_irq_t));

    ESP_LOGI(TAG, "Timer %d initialized", id);
}

void hal_lp_timer_start(hal_lp_timer_id_t id, const uint32_t milliseconds, const hal_lp_timer_irq_t *tmr_irq)
{
    if (id >= MAX_TIMER_COUNT)
    {
        ESP_LOGE(TAG, "Invalid timer ID: %d", id);
        return;
    }

    lp_timer_context_t *timer = &timers[id];

    if (!timer->initialized)
    {
        ESP_LOGE(TAG, "Timer %d not initialized", id);
        return;
    }

    if (timer->running)
    {
        hal_lp_timer_stop(id);
    }

    if (tmr_irq != NULL)
    {
        timer->irq_context = *tmr_irq;
    }
    else
    {
        memset(&timer->irq_context, 0, sizeof(hal_lp_timer_irq_t));
    }

    uint64_t timeout_us = (uint64_t)milliseconds * 1000;
    esp_err_t err = esp_timer_start_once(timer->handle, timeout_us);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start timer %d: %s", id, esp_err_to_name(err));
        return;
    }

    timer->running = true;
    // Only log for longer timers to avoid spam during normal operation
    if (milliseconds >= 100) {
        ESP_LOGD(TAG, "Timer %d started for %u ms", id, milliseconds);
    }
}

void hal_lp_timer_stop(hal_lp_timer_id_t id)
{
    if (id >= MAX_TIMER_COUNT)
    {
        ESP_LOGE(TAG, "Invalid timer ID: %d", id);
        return;
    }

    lp_timer_context_t *timer = &timers[id];

    if (!timer->initialized)
    {
        ESP_LOGE(TAG, "Timer %d not initialized", id);
        return;
    }

    if (timer->running)
    {
        esp_err_t err = esp_timer_stop(timer->handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to stop timer %d: %s", id, esp_err_to_name(err));
        }
        timer->running = false;
        ESP_LOGD(TAG, "Timer %d stopped", id);
    }
}

void hal_lp_timer_irq_enable(hal_lp_timer_id_t id)
{
    // ESP32 timers are automatically enabled when started
    // This function is kept for API compatibility
    ESP_LOGD(TAG, "Timer %d IRQ enable (no-op on ESP32)", id);
}

void hal_lp_timer_irq_disable(hal_lp_timer_id_t id)
{
    // ESP32 timers are automatically disabled when stopped
    // This function is kept for API compatibility
    ESP_LOGD(TAG, "Timer %d IRQ disable (no-op on ESP32)", id);
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static void timer_callback(void *arg)
{
    lp_timer_context_t *timer = (lp_timer_context_t *)arg;

    if (timer == NULL)
    {
        ESP_LOGE(TAG, "Timer callback with NULL context");
        return;
    }

    timer->running = false;

    if (timer->irq_context.callback != NULL)
    {
        timer->irq_context.callback(timer->irq_context.context);
    }
}

/* --- EOF ------------------------------------------------------------------ */
