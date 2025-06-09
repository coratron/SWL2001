/*!
 * \file      smtc_hal_uart.h
 *
 * \brief     UART Hardware Abstraction Layer definition for ESP32
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
#ifndef __SMTC_HAL_UART_H__
#define __SMTC_HAL_UART_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdbool.h> // bool type
#include <stdint.h>  // C99 types

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC MACROS -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC CONSTANTS --------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC TYPES ------------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS PROTOTYPES ---------------------------------------------
 */

/*!
 * \brief Initialize hardware modem UART interface
 */
void hw_modem_uart_init(void);

/*!
 * \brief Initialize trace UART interface
 */
void trace_uart_init(void);

/*!
 * \brief Deinitialize hardware modem UART interface
 */
void hw_modem_uart_deinit(void);

/*!
 * \brief Deinitialize trace UART interface
 */
void trace_uart_deinit(void);

/*!
 * \brief Start DMA reception on hardware modem UART
 *
 * \param [in] buff Buffer to receive data
 * \param [in] size Size of the buffer
 */
void hw_modem_uart_dma_start_rx(uint8_t *buff, uint16_t size);

/*!
 * \brief Stop DMA reception on hardware modem UART
 */
void hw_modem_uart_dma_stop_rx(void);

/*!
 * \brief Transmit data on hardware modem UART
 *
 * \param [in] buff Buffer containing data to transmit
 * \param [in] len  Length of data to transmit
 */
void hw_modem_uart_tx(uint8_t *buff, uint8_t len);

/*!
 * \brief Transmit data on trace UART
 *
 * \param [in] buff Buffer containing data to transmit
 * \param [in] len  Length of data to transmit
 */
void trace_uart_tx(uint8_t *buff, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif // __SMTC_HAL_UART_H__

/* --- EOF ------------------------------------------------------------------ */
