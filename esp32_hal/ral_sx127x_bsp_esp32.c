/*!
 * \file      ral_sx127x_bsp_esp32.c
 *
 * \brief     ESP32 Board Support Package for SX127X RAL
 *
 * \copyright Copyright (c) 2023
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdint.h>
#include <stdbool.h>

#include "esp_log.h"
#include "sdkconfig.h"

#include "ral_sx127x_bsp.h"
#include "sx127x.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS -----------------------------------------------------------
 */

#define TAG "RAL_SX127X_BSP"

// SX127x board types
#define SX1276MB1LAS 0
#define SX1276MB1MAS 1

// Power levels and consumption
#define SX127X_MIN_OUTPUT_POWER -3
#define SX127X_MAX_OUTPUT_POWER 17

// Power consumption values (in uA) - these should be calibrated for your specific board
#define SX127X_GFSK_RX_CONSUMPTION 12500
#define SX127X_GFSK_RX_BOOSTED_CONSUMPTION 13500
#define SX127X_LORA_RX_CONSUMPTION 12500  
#define SX127X_LORA_RX_BOOSTED_CONSUMPTION 13500

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

// Power consumption lookup table for different power levels (in uA)
static const uint32_t ral_sx127x_convert_tx_dbm_to_ua[] = {
    12800,  // -3 dBm
    13500,  // -2 dBm  
    14200,  // -1 dBm
    14900,  //  0 dBm
    15600,  //  1 dBm
    16400,  //  2 dBm
    17200,  //  3 dBm
    18000,  //  4 dBm
    18800,  //  5 dBm
    19700,  //  6 dBm
    20600,  //  7 dBm
    21500,  //  8 dBm
    22500,  //  9 dBm
    23500,  // 10 dBm
    24500,  // 11 dBm
    25600,  // 12 dBm
    26700,  // 13 dBm
    27900,  // 14 dBm
    29100,  // 15 dBm
    30400,  // 16 dBm
    31700,  // 17 dBm
};

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

static uint8_t get_board_type(void);

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void ral_sx127x_bsp_get_tx_cfg(const void* context,
                               const ral_sx127x_bsp_tx_cfg_input_params_t* input_params,
                               ral_sx127x_bsp_tx_cfg_output_params_t* output_params)
{
    (void)context;
    
    int8_t power_dbm = input_params->system_output_pwr_in_dbm;
    uint32_t freq_hz = input_params->freq_in_hz;
    uint8_t board_type = get_board_type();
    
    // Clamp power to valid range
    if (power_dbm < SX127X_MIN_OUTPUT_POWER) {
        power_dbm = SX127X_MIN_OUTPUT_POWER;
    }
    if (power_dbm > SX127X_MAX_OUTPUT_POWER) {
        power_dbm = SX127X_MAX_OUTPUT_POWER;
    }
    
    // Configure PA based on power level and board type
    if (board_type == SX1276MB1LAS) {
        // Low power board (up to +14dBm with RFO pin)
        if (power_dbm <= 14) {
            output_params->pa_cfg.pa_select = SX127X_PA_SELECT_RFO;
            output_params->pa_cfg.is_20_dbm_output_on = false;
        } else {
            // Use PA_BOOST for higher power (up to +17dBm)
            output_params->pa_cfg.pa_select = SX127X_PA_SELECT_BOOST;
            output_params->pa_cfg.is_20_dbm_output_on = false;
        }
    } else {
        // High power board (SX1276MB1MAS) - use PA_BOOST for all power levels
        output_params->pa_cfg.pa_select = SX127X_PA_SELECT_BOOST;
        
        if (power_dbm <= 17) {
            output_params->pa_cfg.is_20_dbm_output_on = false;
        } else {
            // For +20dBm operation, need special configuration
            output_params->pa_cfg.is_20_dbm_output_on = true;
        }
    }
    
    // Set ramp time
#if defined(CONFIG_LBM_SX127X_RAMP_3400_US)
    output_params->pa_ramp_time = SX127X_RAMP_3400_US;
#elif defined(CONFIG_LBM_SX127X_RAMP_2000_US)
    output_params->pa_ramp_time = SX127X_RAMP_2000_US;
#elif defined(CONFIG_LBM_SX127X_RAMP_1000_US)
    output_params->pa_ramp_time = SX127X_RAMP_1000_US;
#elif defined(CONFIG_LBM_SX127X_RAMP_500_US)
    output_params->pa_ramp_time = SX127X_RAMP_500_US;
#elif defined(CONFIG_LBM_SX127X_RAMP_250_US)
    output_params->pa_ramp_time = SX127X_RAMP_250_US;
#elif defined(CONFIG_LBM_SX127X_RAMP_125_US)
    output_params->pa_ramp_time = SX127X_RAMP_125_US;
#elif defined(CONFIG_LBM_SX127X_RAMP_100_US)
    output_params->pa_ramp_time = SX127X_RAMP_100_US;
#elif defined(CONFIG_LBM_SX127X_RAMP_62_US)
    output_params->pa_ramp_time = SX127X_RAMP_62_US;
#elif defined(CONFIG_LBM_SX127X_RAMP_50_US)
    output_params->pa_ramp_time = SX127X_RAMP_50_US;
#elif defined(CONFIG_LBM_SX127X_RAMP_40_US)
    output_params->pa_ramp_time = SX127X_RAMP_40_US;
#elif defined(CONFIG_LBM_SX127X_RAMP_31_US)
    output_params->pa_ramp_time = SX127X_RAMP_31_US;
#elif defined(CONFIG_LBM_SX127X_RAMP_25_US)
    output_params->pa_ramp_time = SX127X_RAMP_25_US;
#elif defined(CONFIG_LBM_SX127X_RAMP_20_US)
    output_params->pa_ramp_time = SX127X_RAMP_20_US;
#elif defined(CONFIG_LBM_SX127X_RAMP_15_US)
    output_params->pa_ramp_time = SX127X_RAMP_15_US;
#elif defined(CONFIG_LBM_SX127X_RAMP_12_US)
    output_params->pa_ramp_time = SX127X_RAMP_12_US;
#elif defined(CONFIG_LBM_SX127X_RAMP_10_US)
    output_params->pa_ramp_time = SX127X_RAMP_10_US;
#else
    output_params->pa_ramp_time = SX127X_RAMP_40_US;  // Default ramp time
#endif
    
    output_params->chip_output_pwr_in_dbm_configured = power_dbm;
    output_params->chip_output_pwr_in_dbm_expected = power_dbm;
    
    ESP_LOGD(TAG, "TX config: power=%d dBm, freq=%lu Hz, PA=%s, 20dBm=%s", 
             power_dbm, freq_hz,
             (output_params->pa_cfg.pa_select == SX127X_PA_SELECT_RFO) ? "RFO" : "PA_BOOST",
             output_params->pa_cfg.is_20_dbm_output_on ? "enabled" : "disabled");
}

void ral_sx127x_bsp_get_ocp_value(const void* context, uint8_t* ocp_trim_value)
{
    (void)context;
    
    // Over Current Protection trim value
    // Imax = 45 + 5 * ocp_trim_value [mA] if ocp_trim_value <= 15 (120 [mA])
    // Imax = -30 + 10 * ocp_trim_value [mA] if 15 < ocp_trim_value <= 27 (130 to 240 [mA])
    // Default Imax = 100mA (ocp_trim_value = 0x0B)
    
#ifdef CONFIG_LBM_SX127X_OCP_TRIM_VALUE
    *ocp_trim_value = CONFIG_LBM_SX127X_OCP_TRIM_VALUE;
#else
    *ocp_trim_value = 0x0B;  // Default 100mA
#endif
    
    uint16_t current_ma;
    if (*ocp_trim_value <= 15) {
        current_ma = 45 + 5 * (*ocp_trim_value);
    } else if (*ocp_trim_value <= 27) {
        current_ma = -30 + 10 * (*ocp_trim_value);
    } else {
        current_ma = 240;
    }
    
    ESP_LOGD(TAG, "OCP trim value: 0x%02X (%d mA)", *ocp_trim_value, current_ma);
}

ral_status_t ral_sx127x_bsp_get_instantaneous_tx_power_consumption(
    const void* context,
    const ral_sx127x_bsp_tx_cfg_output_params_t* tx_cfg_output_params_local,
    uint32_t* pwr_consumption_in_ua)
{
    (void)context;
    
    int8_t power_dbm = tx_cfg_output_params_local->chip_output_pwr_in_dbm_configured;
    
    // Calculate index into power consumption table
    int index = power_dbm - SX127X_MIN_OUTPUT_POWER;
    
    if (index >= 0 && index < (int)(sizeof(ral_sx127x_convert_tx_dbm_to_ua) / sizeof(uint32_t))) {
        *pwr_consumption_in_ua = ral_sx127x_convert_tx_dbm_to_ua[index];
    } else {
        // Fallback for out-of-range values
        *pwr_consumption_in_ua = 20000;  // Default 20mA
    }
    
    ESP_LOGV(TAG, "TX power consumption: %lu uA for %d dBm", 
             *pwr_consumption_in_ua, power_dbm);
    
    return RAL_STATUS_OK;
}

ral_status_t ral_sx127x_bsp_get_instantaneous_gfsk_rx_power_consumption(
    const void* context, bool rx_boosted, uint32_t* pwr_consumption_in_ua)
{
    (void)context;
    
    if (rx_boosted) {
        *pwr_consumption_in_ua = SX127X_GFSK_RX_BOOSTED_CONSUMPTION;
    } else {
        *pwr_consumption_in_ua = SX127X_GFSK_RX_CONSUMPTION;
    }
    
    ESP_LOGV(TAG, "GFSK RX power consumption: %lu uA (boosted=%s)", 
             *pwr_consumption_in_ua, rx_boosted ? "true" : "false");
    
    return RAL_STATUS_OK;
}

ral_status_t ral_sx127x_bsp_get_instantaneous_lora_rx_power_consumption(
    const void* context, bool rx_boosted, uint32_t* pwr_consumption_in_ua)
{
    (void)context;
    
    if (rx_boosted) {
        *pwr_consumption_in_ua = SX127X_LORA_RX_BOOSTED_CONSUMPTION;
    } else {
        *pwr_consumption_in_ua = SX127X_LORA_RX_CONSUMPTION;
    }
    
    ESP_LOGV(TAG, "LoRa RX power consumption: %lu uA (boosted=%s)", 
             *pwr_consumption_in_ua, rx_boosted ? "true" : "false");
    
    return RAL_STATUS_OK;
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static uint8_t get_board_type(void)
{
    // Determine board type based on configuration
    // This should be set based on your actual hardware design
    
#ifdef CONFIG_LBM_SX127X_BOARD_TYPE_MAS
    return SX1276MB1MAS;  // High power board
#else
    return SX1276MB1LAS;  // Low power board (default)
#endif
}
