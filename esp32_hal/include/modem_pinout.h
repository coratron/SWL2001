/*!
 * \file      modem_pinout.h
 *
 * \brief     ESP32 pin definitions for LoRa Basic Modem
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
#ifndef __MODEM_PINOUT_H__
#define __MODEM_PINOUT_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC MACROS -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC CONSTANTS --------------------------------------------------------
 */

// Debug UART pins (used for trace output)
#define DEBUG_UART_TX 1 // GPIO1 (U0TXD)
#define DEBUG_UART_RX 3 // GPIO3 (U0RXD)

// Hardware modem UART pins (used for modem communication)
#define HW_MODEM_TX_LINE 17 // GPIO17 (U2TXD)
#define HW_MODEM_RX_LINE 16 // GPIO16 (U2RXD)

// SX127x radio pins
#define RADIO_RESET 14 // GPIO14
#define RADIO_MOSI 23  // GPIO23 (VSPI MOSI)
#define RADIO_MISO 19  // GPIO19 (VSPI MISO)
#define RADIO_SCLK 18  // GPIO18 (VSPI CLK)
#define RADIO_NSS 5    // GPIO5 (VSPI CS)
#define RADIO_DIO_0 26 // GPIO26
#define RADIO_DIO_1 35 // GPIO35
#define RADIO_DIO_2 34 // GPIO34
#define RADIO_DIO_3 39 // GPIO39
#define RADIO_DIO_4 36 // GPIO36
#define RADIO_DIO_5 -1 // Not connected

// Antenna switch pins (if used)
#define RADIO_ANT_SWITCH_RX 21 // GPIO21
#define RADIO_ANT_SWITCH_TX 22 // GPIO22

// Legacy antenna switch definition (for compatibility)
#define RADIO_ANTENNA_SWITCH RADIO_ANT_SWITCH_TX // Default to TX switch pin

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC TYPES ------------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS PROTOTYPES ---------------------------------------------
 */

#ifdef __cplusplus
}
#endif

#endif // __MODEM_PINOUT_H__

/* --- EOF ------------------------------------------------------------------ */
