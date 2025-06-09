/*!
 * \file      smtc_hal_uart_esp.c
 *
 * \brief     UART Hardware Abstraction Layer implementation for ESP32
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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"

#include "smtc_hal_uart.h"
#include "modem_pinout.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

#define HW_MODEM_UART_NUM UART_NUM_2
#define TRACE_UART_NUM UART_NUM_0
#define UART_BUF_SIZE 1024
#define UART_QUEUE_SIZE 20

static const char *TAG = "UART_HAL";

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static QueueHandle_t hw_modem_uart_queue = NULL;
static uint8_t *hw_modem_rx_buffer = NULL;
static uint16_t hw_modem_rx_size = 0;
static bool hw_modem_uart_initialized = false;
static bool trace_uart_initialized = false;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void hw_modem_uart_init(void)
{
    if (hw_modem_uart_initialized)
    {
        return;
    }

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Install UART driver with event queue
    ESP_ERROR_CHECK(uart_driver_install(HW_MODEM_UART_NUM, UART_BUF_SIZE, UART_BUF_SIZE,
                                        UART_QUEUE_SIZE, &hw_modem_uart_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(HW_MODEM_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(HW_MODEM_UART_NUM, HW_MODEM_TX_LINE, HW_MODEM_RX_LINE,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    hw_modem_uart_initialized = true;
    ESP_LOGI(TAG, "Hardware modem UART initialized on pins TX:%d, RX:%d",
             HW_MODEM_TX_LINE, HW_MODEM_RX_LINE);
}

void trace_uart_init(void)
{
    if (trace_uart_initialized)
    {
        return;
    }

    // UART0 is typically already initialized by ESP-IDF for console output
    // We just mark it as initialized for our purposes
    trace_uart_initialized = true;
    ESP_LOGI(TAG, "Trace UART initialized (using default console UART)");
}

void hw_modem_uart_deinit(void)
{
    if (!hw_modem_uart_initialized)
    {
        return;
    }

    if (hw_modem_uart_queue)
    {
        vQueueDelete(hw_modem_uart_queue);
        hw_modem_uart_queue = NULL;
    }

    uart_driver_delete(HW_MODEM_UART_NUM);
    hw_modem_uart_initialized = false;
    ESP_LOGI(TAG, "Hardware modem UART deinitialized");
}

void trace_uart_deinit(void)
{
    if (!trace_uart_initialized)
    {
        return;
    }

    // Don't actually deinitialize UART0 as it's used for console
    trace_uart_initialized = false;
    ESP_LOGI(TAG, "Trace UART deinitialized");
}

void hw_modem_uart_dma_start_rx(uint8_t *buff, uint16_t size)
{
    if (!hw_modem_uart_initialized)
    {
        ESP_LOGE(TAG, "Hardware modem UART not initialized");
        return;
    }

    hw_modem_rx_buffer = buff;
    hw_modem_rx_size = size;

    // Clear any pending data in the UART buffer
    uart_flush_input(HW_MODEM_UART_NUM);

    ESP_LOGD(TAG, "Started DMA RX with buffer size: %d", size);
}

void hw_modem_uart_dma_stop_rx(void)
{
    if (!hw_modem_uart_initialized)
    {
        return;
    }

    hw_modem_rx_buffer = NULL;
    hw_modem_rx_size = 0;

    // Flush any remaining data
    uart_flush_input(HW_MODEM_UART_NUM);

    ESP_LOGD(TAG, "Stopped DMA RX");
}

void hw_modem_uart_tx(uint8_t *buff, uint8_t len)
{
    if (!hw_modem_uart_initialized)
    {
        ESP_LOGE(TAG, "Hardware modem UART not initialized");
        return;
    }

    if (buff == NULL || len == 0)
    {
        return;
    }

    int bytes_written = uart_write_bytes(HW_MODEM_UART_NUM, (const char *)buff, len);
    if (bytes_written != len)
    {
        ESP_LOGE(TAG, "Failed to write all bytes to hardware modem UART: %d/%d",
                 bytes_written, len);
    }

    // Wait for transmission to complete
    uart_wait_tx_done(HW_MODEM_UART_NUM, portMAX_DELAY);

    ESP_LOGD(TAG, "Transmitted %d bytes on hardware modem UART", len);
}

void trace_uart_tx(uint8_t *buff, uint8_t len)
{
    if (!trace_uart_initialized)
    {
        return;
    }

    if (buff == NULL || len == 0)
    {
        return;
    }

    // Use ESP-IDF console output for trace UART
    // This will go through UART0 which is the default console
    fwrite(buff, 1, len, stdout);
    fflush(stdout);

    ESP_LOGD(TAG, "Transmitted %d bytes on trace UART", len);
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

/* --- EOF ------------------------------------------------------------------ */
