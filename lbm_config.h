/*!
 * \file      lbm_config.h
 *
 * \brief     LoRa Basics Modem configuration for ESP32
 *
 * \copyright Copyright (c) 2023
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LBM_CONFIG_H
#define LBM_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include "sdkconfig.h"

/*
 * -----------------------------------------------------------------------------
 * --- RADIO CONFIGURATION ----------------------------------------------------
 */

// Radio type selection
#ifdef CONFIG_LBM_RADIO_SX126X
#define SX126X
#endif

#ifdef CONFIG_LBM_RADIO_SX127X
#define SX127X
#endif

#ifdef CONFIG_LBM_RADIO_LR11XX
#define LR11XX
#endif

#ifdef CONFIG_LBM_RADIO_SX128X
#define SX128X
#endif

/*
 * -----------------------------------------------------------------------------
 * --- REGION CONFIGURATION ---------------------------------------------------
 */

#ifdef CONFIG_LBM_REGION_EU_868
#define REGION_EU_868
#endif

#ifdef CONFIG_LBM_REGION_US_915
#define REGION_US_915
#endif

#ifdef CONFIG_LBM_REGION_AS_923
#define REGION_AS_923
#endif

#ifdef CONFIG_LBM_REGION_AU_915
#define REGION_AU_915
#endif

#ifdef CONFIG_LBM_REGION_CN_470
#define REGION_CN_470
#endif

#ifdef CONFIG_LBM_REGION_IN_865
#define REGION_IN_865
#endif

#ifdef CONFIG_LBM_REGION_KR_920
#define REGION_KR_920
#endif

#ifdef CONFIG_LBM_REGION_RU_864
#define REGION_RU_864
#endif

/*
 * -----------------------------------------------------------------------------
 * --- FEATURE CONFIGURATION --------------------------------------------------
 */

#ifdef CONFIG_LBM_STREAM
#define ADD_SMTC_STREAM
#endif

#ifdef CONFIG_LBM_DEVICE_MANAGEMENT
#define ADD_SMTC_CLOUD_DEVICE_MANAGEMENT
#endif

#ifdef CONFIG_LBM_ALMANAC
#define ADD_ALMANAC
#endif

#ifdef CONFIG_LBM_GEOLOCATION
#define ADD_GEOLOCATION
#define ADD_GNSS
#define ADD_WIFI
#endif

#ifdef CONFIG_LBM_RELAY_TX
#define ADD_RELAY_TX
#endif

#ifdef CONFIG_LBM_RELAY_RX
#define ADD_RELAY_RX
#endif

#ifdef CONFIG_LBM_CSMA
#define ADD_CSMA
#endif

#ifdef CONFIG_LBM_LBT
#define ADD_LBT
#endif

/*
 * -----------------------------------------------------------------------------
 * --- DEBUG CONFIGURATION ----------------------------------------------------
 */

#ifdef CONFIG_LBM_HAL_DBG_TRACE
#define HAL_DBG_TRACE 1
#else
#define HAL_DBG_TRACE 0
#endif

#ifdef CONFIG_LBM_MODEM_DBG_TRACE
#define MODEM_HAL_DBG_TRACE 1
#else
#define MODEM_HAL_DBG_TRACE 0
#endif

/*
 * -----------------------------------------------------------------------------
 * --- HARDWARE CONFIGURATION -------------------------------------------------
 */

// Maximum number of LoRaWAN stacks (typically 1 for most applications)
#define NUMBER_OF_STACKS 1

// Enable multicast support
#ifdef CONFIG_LBM_MULTICAST
#define SMTC_MULTICAST

// Number of multicast sessions
#define SMTC_MULTICAST_MAX_SESSIONS CONFIG_LBM_MULTICAST_MAX_SESSIONS
#endif

// FUOTA (Firmware Update Over The Air) support
#ifdef CONFIG_LBM_FUOTA
#define ENABLE_FUOTA_FULL

#ifdef CONFIG_LBM_FUOTA_FMP
#define ENABLE_FUOTA_FMP
#endif

#ifdef CONFIG_LBM_FUOTA_MPA
#define ENABLE_FUOTA_MPA
#endif
#endif

#ifdef CONFIG_LBM_CLASS_B
#define ADD_CLASS_B
#endif

#ifdef CONFIG_LBM_CLASS_C
#define ADD_CLASS_C
#endif

// Default stack configuration
#define STACK_ID_0 0

/*
 * -----------------------------------------------------------------------------
 * --- MEMORY CONFIGURATION ---------------------------------------------------
 */

// Size of the fifo buffer for internal events
#define FIFO_LORAWAN_SIZE CONFIG_LBM_FIFO_LORAWAN_SIZE

/*
 * -----------------------------------------------------------------------------
 * --- TIMING CONFIGURATION ---------------------------------------------------
 */

// Default random delay range for transmissions (in ms)
#define MODEM_MIN_RANDOM_DELAY_MS CONFIG_LBM_MIN_RANDOM_DELAY_MS
#define MODEM_MAX_RANDOM_DELAY_MS CONFIG_LBM_MAX_RANDOM_DELAY_MS

// Default join procedure settings
#define MODEM_INITIAL_JOIN_DELAY_S CONFIG_LBM_INITIAL_JOIN_DELAY_S
#define MODEM_MAX_JOIN_DELAY_S CONFIG_LBM_MAX_JOIN_DELAY_S

// Watchdog timeout
#define MODEM_WATCHDOG_TIMEOUT_MS CONFIG_LBM_WATCHDOG_TIMEOUT_MS

/*
 * -----------------------------------------------------------------------------
 * --- RADIO SPECIFIC CONFIGURATION -------------------------------------------
 */

#ifdef SX126X
// SX126X specific configuration
#define ENABLE_SX126X_TCXO_CONTROL
#define SX126X_USE_DIO2_AS_RF_SWITCH
#endif

#ifdef SX127X
// SX127X specific configuration
#define USE_RADIO_DEBUG
#endif

#ifdef LR11XX
// LR11XX specific configuration
#define USE_LR11XX_CE
#define LR11XX_ENABLE_TCXO
#endif

/*
 * -----------------------------------------------------------------------------
 * --- CERTIFICATION CONFIGURATION --------------------------------------------
 */

// Enable LoRaWAN certification features
#ifdef CONFIG_LBM_CERTIFICATION
#define ENABLE_LORAWAN_CERTIFICATION
#endif

// Enable test mode
#ifdef CONFIG_LBM_TEST_MODE
#define ENABLE_TEST_MODE
#endif

#ifdef __cplusplus
}
#endif

#endif /* LBM_CONFIG_H */
