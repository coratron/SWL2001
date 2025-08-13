/*!
 * \file      smtc_hal_trace.c
 *
 * \brief     Trace Print Hardware Abstraction Layer implementation for ESP32
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
#include <stdio.h>
#include <string.h>
#include <stdarg.h> // va_list

#include "smtc_hal_trace.h"
#include "esp_log.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */
#define PRINT_BUFFER_SIZE 512
#define TAG "LBM_TRACE"

// Trace level definitions (matching Kconfig)
#define TRACE_LEVEL_NONE    0
#define TRACE_LEVEL_ERROR   1
#define TRACE_LEVEL_WARN    2
#define TRACE_LEVEL_INFO    3
#define TRACE_LEVEL_DEBUG   4
#define TRACE_LEVEL_VERBOSE 5

// Get the configured trace level from Kconfig
#ifndef CONFIG_LBM_TRACE_LEVEL
#define CONFIG_LBM_TRACE_LEVEL 3  // Default to INFO if not configured
#endif

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */
void hal_trace_print_var(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    hal_trace_print(fmt, args);
    va_end(args);
}

void hal_trace_print(const char *fmt, va_list argp)
{
    // If trace level is NONE, don't print anything
    if (CONFIG_LBM_TRACE_LEVEL == TRACE_LEVEL_NONE)
    {
        return;
    }
    
    char string[PRINT_BUFFER_SIZE];
    int len = vsnprintf(string, PRINT_BUFFER_SIZE, fmt, argp);
    
    // Handle error case
    if (len < 0)
    {
        ESP_LOGE(TAG, "vsnprintf error in hal_trace_print");
        return;
    }
    
    // Handle truncation case
    if (len >= PRINT_BUFFER_SIZE)
    {
        // Ensure null termination
        string[PRINT_BUFFER_SIZE - 1] = '\0';
        // Remove newline if present at the end of truncated string
        if (PRINT_BUFFER_SIZE > 1 && string[PRINT_BUFFER_SIZE - 2] == '\n')
        {
            string[PRINT_BUFFER_SIZE - 2] = '\0';
        }
        ESP_LOGW(TAG, "%s [TRUNCATED]", string);
    }
    else if (len > 0)
    {
        // Remove newline if present to avoid double newlines in ESP_LOG
        if (string[len - 1] == '\n')
        {
            string[len - 1] = '\0';
        }
        
        // Detect log level from message content and filter based on CONFIG_LBM_TRACE_LEVEL
        // The LBM macros add "ERROR: ", "WARN: ", "INFO: " prefixes
        if (strstr(string, "ERROR: ") != NULL)
        {
            if (CONFIG_LBM_TRACE_LEVEL >= TRACE_LEVEL_ERROR)
            {
                ESP_LOGE(TAG, "%s", string);
            }
        }
        else if (strstr(string, "WARN: ") != NULL)
        {
            if (CONFIG_LBM_TRACE_LEVEL >= TRACE_LEVEL_WARN)
            {
                ESP_LOGW(TAG, "%s", string);
            }
        }
        else if (strstr(string, "INFO: ") != NULL)
        {
            if (CONFIG_LBM_TRACE_LEVEL >= TRACE_LEVEL_INFO)
            {
                ESP_LOGI(TAG, "%s", string);
            }
        }
        else
        {
            // Default to INFO level for messages without explicit level prefix
            // This includes regular TRACE_PRINTF calls which are informational
            if (CONFIG_LBM_TRACE_LEVEL >= TRACE_LEVEL_INFO)
            {
                ESP_LOGI(TAG, "%s", string);
            }
        }
    }
    // len == 0 case (empty string) - do nothing
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

/* --- EOF ------------------------------------------------------------------ */
