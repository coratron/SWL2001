/*!
 * \file      smtc_hal_gpio_pin_names.h
 *
 * \brief     Defines ESP32 platform pin names
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

#ifndef __SMTC_HAL_GPIO_PIN_NAMES_H__
#define __SMTC_HAL_GPIO_PIN_NAMES_H__

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

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC TYPES ------------------------------------------------------------
 */

typedef enum gpio_pin_names_e {
  // ESP32 GPIO pins
  GPIO_0 = 0,
  GPIO_1 = 1,
  GPIO_2 = 2,
  GPIO_3 = 3,
  GPIO_4 = 4,
  GPIO_5 = 5,
  GPIO_6 = 6,   // Flash - usually not available
  GPIO_7 = 7,   // Flash - usually not available
  GPIO_8 = 8,   // Flash - usually not available
  GPIO_9 = 9,   // Flash - usually not available
  GPIO_10 = 10, // Flash - usually not available
  GPIO_11 = 11, // Flash - usually not available
  GPIO_12 = 12,
  GPIO_13 = 13,
  GPIO_14 = 14,
  GPIO_15 = 15,
  GPIO_16 = 16,
  GPIO_17 = 17,
  GPIO_18 = 18,
  GPIO_19 = 19,
  GPIO_20 = 20, // Not available on ESP32
  GPIO_21 = 21,
  GPIO_22 = 22,
  GPIO_23 = 23,
  GPIO_24 = 24, // Not available on ESP32
  GPIO_25 = 25,
  GPIO_26 = 26,
  GPIO_27 = 27,
  GPIO_28 = 28, // Not available on ESP32
  GPIO_29 = 29, // Not available on ESP32
  GPIO_30 = 30, // Not available on ESP32
  GPIO_31 = 31, // Not available on ESP32
  GPIO_32 = 32,
  GPIO_33 = 33,
  GPIO_34 = 34, // Input only
  GPIO_35 = 35, // Input only
  GPIO_36 = 36, // Input only
  GPIO_37 = 37, // Input only
  GPIO_38 = 38, // Input only
  GPIO_39 = 39, // Input only

  // Common aliases
  NC = 0xFF // Not connected
} hal_gpio_pin_names_t;

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS PROTOTYPES ---------------------------------------------
 */

#ifdef __cplusplus
}
#endif

#endif // __SMTC_HAL_GPIO_PIN_NAMES_H__

/* --- EOF ------------------------------------------------------------------ */
