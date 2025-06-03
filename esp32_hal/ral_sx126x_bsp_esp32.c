/*!
 * \file      ral_sx126x_bsp_esp32.c
 *
 * \brief     ESP32 Board Support Package for SX126X RAL
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

#include "ral_sx126x_bsp.h"
#include "sx126x.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS -----------------------------------------------------------
 */

#define TAG "RAL_SX126X_BSP"

// Power amplifier configuration based on ESP32 board design
#define SX126X_LP_MIN_OUTPUT_POWER -17
#define SX126X_LP_MAX_OUTPUT_POWER 15
#define SX126X_HP_MIN_OUTPUT_POWER -9
#define SX126X_HP_MAX_OUTPUT_POWER 22

// Power consumption values (in uA) - these should be calibrated for your specific board
#define SX126X_GFSK_RX_CONSUMPTION_DCDC 4200
#define SX126X_GFSK_RX_BOOSTED_CONSUMPTION_DCDC 4800
#define SX126X_GFSK_RX_CONSUMPTION_LDO 8000
#define SX126X_GFSK_RX_BOOSTED_CONSUMPTION_LDO 9300

#define SX126X_LORA_RX_CONSUMPTION_DCDC 4600
#define SX126X_LORA_RX_BOOSTED_CONSUMPTION_DCDC 5300
#define SX126X_LORA_RX_CONSUMPTION_LDO 8880
#define SX126X_LORA_RX_BOOSTED_CONSUMPTION_LDO 10100

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

// Power consumption lookup table for Low Power PA mode (DCDC regulator)
static const uint32_t ral_sx126x_convert_tx_dbm_to_ua_reg_mode_dcdc_lp[] = {
    5200,   // -17 dBm
    5400,   // -16 dBm
    5600,   // -15 dBm
    5700,   // -14 dBm
    5800,   // -13 dBm
    6000,   // -12 dBm
    6100,   // -11 dBm
    6200,   // -10 dBm
    6500,   //  -9 dBm
    6800,   //  -8 dBm
    7000,   //  -7 dBm
    7300,   //  -6 dBm
    7500,   //  -5 dBm
    7900,   //  -4 dBm
    8300,   //  -3 dBm
    8800,   //  -2 dBm
    9300,   //  -1 dBm
    9800,   //   0 dBm
    10600,  //   1 dBm
    11400,  //   2 dBm
    12200,  //   3 dBm
    12900,  //   4 dBm
    13800,  //   5 dBm
    14700,  //   6 dBm
    15600,  //   7 dBm
    16500,  //   8 dBm
    17600,  //   9 dBm
    18700,  //  10 dBm
    19900,  //  11 dBm
    21200,  //  12 dBm
    22600,  //  13 dBm
    24100,  //  14 dBm
    25700,  //  15 dBm
};

// Power consumption lookup table for High Power PA mode (DCDC regulator)
static const uint32_t ral_sx126x_convert_tx_dbm_to_ua_reg_mode_dcdc_hp[] = {
    27500,  //  -9 dBm
    28100,  //  -8 dBm
    28700,  //  -7 dBm
    29300,  //  -6 dBm
    29900,  //  -5 dBm
    30500,  //  -4 dBm
    31200,  //  -3 dBm
    31900,  //  -2 dBm
    32600,  //  -1 dBm
    33400,  //   0 dBm
    34200,  //   1 dBm
    35000,  //   2 dBm
    35900,  //   3 dBm
    36800,  //   4 dBm
    37700,  //   5 dBm
    38700,  //   6 dBm
    39700,  //   7 dBm
    40800,  //   8 dBm
    41900,  //   9 dBm
    43100,  //  10 dBm
    44300,  //  11 dBm
    45600,  //  12 dBm
    46900,  //  13 dBm
    48300,  //  14 dBm
    49700,  //  15 dBm
    51200,  //  16 dBm
    52800,  //  17 dBm
    54400,  //  18 dBm
    56100,  //  19 dBm
    57900,  //  20 dBm
    59700,  //  21 dBm
    61600,  //  22 dBm
};

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void ral_sx126x_bsp_get_reg_mode(const void* context, sx126x_reg_mod_t* reg_mode)
{
    (void)context;
    
    // Default to DCDC regulator mode for better efficiency
    // Can be overridden via Kconfig if needed
#ifdef CONFIG_LBM_SX126X_USE_LDO_REGULATOR
    *reg_mode = SX126X_REG_MODE_LDO;
    ESP_LOGI(TAG, "Using LDO regulator mode");
#else
    *reg_mode = SX126X_REG_MODE_DCDC;
    ESP_LOGI(TAG, "Using DCDC regulator mode");
#endif
}

void ral_sx126x_bsp_get_rf_switch_cfg(const void* context, bool* dio2_is_set_as_rf_switch)
{
    (void)context;
    
    // Configure DIO2 as RF switch control if no external RF switch is used
    // This depends on your board design
#ifdef CONFIG_LBM_SX126X_USE_DIO2_RF_SWITCH
    *dio2_is_set_as_rf_switch = true;
    ESP_LOGI(TAG, "DIO2 configured as RF switch");
#else
    *dio2_is_set_as_rf_switch = false;
    ESP_LOGI(TAG, "DIO2 not used as RF switch");
#endif
}

void ral_sx126x_bsp_get_tx_cfg(const void* context, 
                               const ral_sx126x_bsp_tx_cfg_input_params_t* input_params,
                               ral_sx126x_bsp_tx_cfg_output_params_t* output_params)
{
    (void)context;
    
    int8_t power_dbm = input_params->system_output_pwr_in_dbm;
    uint32_t freq_hz = input_params->freq_in_hz;
    
    // Configure PA parameters based on power level
    // For SX126X, we use device_sel to choose between SX1261 (0x01) and SX1262/SX1268 (0x00)
    // Most ESP32 modules use SX1262, so default to device_sel = 0x00
    output_params->pa_cfg.device_sel = 0x00;  // SX1262/SX1268
    output_params->pa_cfg.pa_lut = 0x01;      // Default LUT value
    
    // Configure HP (High Power) settings
    if (power_dbm > 14) {
        output_params->pa_cfg.hp_max = 0x07;         // Max HP setting
        output_params->pa_cfg.pa_duty_cycle = 0x04;  // Default duty cycle
        
        // Clamp power to valid range for HP
        if (power_dbm > 22) {
            power_dbm = 22;
        }
        
        ESP_LOGD(TAG, "Using HP settings for %d dBm", power_dbm);
    } else {
        output_params->pa_cfg.hp_max = 0x02;         // Lower HP setting
        output_params->pa_cfg.pa_duty_cycle = 0x02;  // Lower duty cycle
        
        // Clamp power to valid range for LP
        if (power_dbm < -9) {
            power_dbm = -9;
        }
        
        ESP_LOGD(TAG, "Using LP settings for %d dBm", power_dbm);
    }
    
    output_params->chip_output_pwr_in_dbm_configured = power_dbm;
    output_params->chip_output_pwr_in_dbm_expected = power_dbm;
    
    // Set ramp time based on configuration
#if defined(CONFIG_LBM_SX126X_RAMP_10_US)
    output_params->pa_ramp_time = SX126X_RAMP_10_US;
#elif defined(CONFIG_LBM_SX126X_RAMP_20_US)
    output_params->pa_ramp_time = SX126X_RAMP_20_US;
#elif defined(CONFIG_LBM_SX126X_RAMP_40_US)
    output_params->pa_ramp_time = SX126X_RAMP_40_US;
#elif defined(CONFIG_LBM_SX126X_RAMP_80_US)
    output_params->pa_ramp_time = SX126X_RAMP_80_US;
#elif defined(CONFIG_LBM_SX126X_RAMP_200_US)
    output_params->pa_ramp_time = SX126X_RAMP_200_US;
#elif defined(CONFIG_LBM_SX126X_RAMP_800_US)
    output_params->pa_ramp_time = SX126X_RAMP_800_US;
#elif defined(CONFIG_LBM_SX126X_RAMP_1700_US)
    output_params->pa_ramp_time = SX126X_RAMP_1700_US;
#elif defined(CONFIG_LBM_SX126X_RAMP_3400_US)
    output_params->pa_ramp_time = SX126X_RAMP_3400_US;
#else
    output_params->pa_ramp_time = SX126X_RAMP_40_US;  // Default ramp time
#endif
    
    ESP_LOGD(TAG, "TX config: power=%d dBm, freq=%lu Hz, device_sel=0x%02X", 
             power_dbm, freq_hz, output_params->pa_cfg.device_sel);
}

void ral_sx126x_bsp_get_xosc_cfg(const void* context, ral_xosc_cfg_t* xosc_cfg,
                                 sx126x_tcxo_ctrl_voltages_t* supply_voltage, 
                                 uint32_t* startup_time_in_tick)
{
    (void)context;
    
    // Configure crystal oscillator based on board design
#ifdef CONFIG_LBM_SX126X_USE_TCXO
    *xosc_cfg = RAL_XOSC_CFG_TCXO_RADIO_CTRL;
    
    // TCXO supply voltage configuration
#if defined(CONFIG_LBM_SX126X_TCXO_1_6V)
    *supply_voltage = SX126X_TCXO_CTRL_1_6V;
#elif defined(CONFIG_LBM_SX126X_TCXO_1_7V)
    *supply_voltage = SX126X_TCXO_CTRL_1_7V;
#elif defined(CONFIG_LBM_SX126X_TCXO_1_8V)
    *supply_voltage = SX126X_TCXO_CTRL_1_8V;
#elif defined(CONFIG_LBM_SX126X_TCXO_2_2V)
    *supply_voltage = SX126X_TCXO_CTRL_2_2V;
#elif defined(CONFIG_LBM_SX126X_TCXO_2_4V)
    *supply_voltage = SX126X_TCXO_CTRL_2_4V;
#elif defined(CONFIG_LBM_SX126X_TCXO_2_7V)
    *supply_voltage = SX126X_TCXO_CTRL_2_7V;
#elif defined(CONFIG_LBM_SX126X_TCXO_3_0V)
    *supply_voltage = SX126X_TCXO_CTRL_3_0V;
#elif defined(CONFIG_LBM_SX126X_TCXO_3_3V)
    *supply_voltage = SX126X_TCXO_CTRL_3_3V;
#else
    *supply_voltage = SX126X_TCXO_CTRL_3_0V;  // Default 3.0V
#endif
    
    // TCXO startup time in 15.625 us steps
#ifdef CONFIG_LBM_SX126X_TCXO_STARTUP_TIME_MS
    *startup_time_in_tick = (CONFIG_LBM_SX126X_TCXO_STARTUP_TIME_MS * 1000) / 15.625;
#else
    *startup_time_in_tick = 320;  // Default 5ms
#endif
    
    ESP_LOGI(TAG, "Using TCXO with %s voltage, startup time: %lu ticks", 
             (*supply_voltage == SX126X_TCXO_CTRL_1_6V) ? "1.6V" :
             (*supply_voltage == SX126X_TCXO_CTRL_1_7V) ? "1.7V" :
             (*supply_voltage == SX126X_TCXO_CTRL_1_8V) ? "1.8V" :
             (*supply_voltage == SX126X_TCXO_CTRL_2_2V) ? "2.2V" :
             (*supply_voltage == SX126X_TCXO_CTRL_2_4V) ? "2.4V" :
             (*supply_voltage == SX126X_TCXO_CTRL_2_7V) ? "2.7V" :
             (*supply_voltage == SX126X_TCXO_CTRL_3_0V) ? "3.0V" : "3.3V",
             *startup_time_in_tick);
#else
    // Use crystal oscillator
    *xosc_cfg = RAL_XOSC_CFG_XTAL;
    *supply_voltage = SX126X_TCXO_CTRL_3_0V;  // Not used for XTAL
    *startup_time_in_tick = 0;  // Not used for XTAL
    
    ESP_LOGI(TAG, "Using crystal oscillator");
#endif
}

void ral_sx126x_bsp_get_trim_cap(const void* context, uint8_t* trimming_cap_xta, 
                                 uint8_t* trimming_cap_xtb)
{
    (void)context;
    
    // Crystal trimming capacitor values
    // These should be calibrated for your specific board and crystal
#ifdef CONFIG_LBM_SX126X_TRIM_CAP_XTA
    *trimming_cap_xta = CONFIG_LBM_SX126X_TRIM_CAP_XTA;
#else
    *trimming_cap_xta = 0x09;  // Default value
#endif

#ifdef CONFIG_LBM_SX126X_TRIM_CAP_XTB
    *trimming_cap_xtb = CONFIG_LBM_SX126X_TRIM_CAP_XTB;
#else
    *trimming_cap_xtb = 0x09;  // Default value
#endif
    
    ESP_LOGD(TAG, "Crystal trim caps: XTA=0x%02X, XTB=0x%02X", 
             *trimming_cap_xta, *trimming_cap_xtb);
}

void ral_sx126x_bsp_get_rx_boost_cfg(const void* context, bool* rx_boost_is_activated)
{
    (void)context;
    
    // Enable RX boost for better sensitivity if configured
#ifdef CONFIG_LBM_SX126X_RX_BOOST_ENABLE
    *rx_boost_is_activated = true;
    ESP_LOGI(TAG, "RX boost enabled");
#else
    *rx_boost_is_activated = false;
    ESP_LOGD(TAG, "RX boost disabled");
#endif
}

void ral_sx126x_bsp_get_ocp_value(const void* context, uint8_t* ocp_in_step_of_2_5_ma)
{
    (void)context;
    
    // Over Current Protection value in steps of 2.5mA
    // Default is 140mA (0x38 = 56 * 2.5mA = 140mA)
#ifdef CONFIG_LBM_SX126X_OCP_VALUE
    *ocp_in_step_of_2_5_ma = CONFIG_LBM_SX126X_OCP_VALUE;
#else
    *ocp_in_step_of_2_5_ma = 0x38;  // 140mA
#endif
    
    ESP_LOGD(TAG, "OCP value: %d (%.1fmA)", 
             *ocp_in_step_of_2_5_ma, (*ocp_in_step_of_2_5_ma) * 2.5);
}

void ral_sx126x_bsp_get_cad_det_peak(const void* context, uint8_t* cad_det_peak)
{
    (void)context;
    
    // Channel Activity Detection DetPeak value
#ifdef CONFIG_LBM_SX126X_CAD_DET_PEAK
    *cad_det_peak = CONFIG_LBM_SX126X_CAD_DET_PEAK;
#else
    *cad_det_peak = 0x18;  // Default value
#endif
    
    ESP_LOGD(TAG, "CAD DetPeak: 0x%02X", *cad_det_peak);
}

ral_status_t ral_sx126x_bsp_get_instantaneous_tx_power_consumption(
    const void* context,
    const ral_sx126x_bsp_tx_cfg_output_params_t* tx_cfg_output_params_local,
    sx126x_reg_mod_t radio_reg_mode, uint32_t* pwr_consumption_in_ua)
{
    (void)context;
    (void)radio_reg_mode;
    
    int8_t power_dbm = tx_cfg_output_params_local->chip_output_pwr_in_dbm_configured;
    
    // Use hp_max to determine if we're using LP or HP mode
    // hp_max <= 0x02 indicates LP mode, hp_max >= 0x07 indicates HP mode
    if (tx_cfg_output_params_local->pa_cfg.hp_max <= 0x02) {
        // Low Power PA
        int index = power_dbm - SX126X_LP_MIN_OUTPUT_POWER;
        if (index >= 0 && index < (int)(sizeof(ral_sx126x_convert_tx_dbm_to_ua_reg_mode_dcdc_lp) / sizeof(uint32_t))) {
            *pwr_consumption_in_ua = ral_sx126x_convert_tx_dbm_to_ua_reg_mode_dcdc_lp[index];
        } else {
            *pwr_consumption_in_ua = 10000;  // Default fallback
        }
    } else {
        // High Power PA
        int index = power_dbm - SX126X_HP_MIN_OUTPUT_POWER;
        if (index >= 0 && index < (int)(sizeof(ral_sx126x_convert_tx_dbm_to_ua_reg_mode_dcdc_hp) / sizeof(uint32_t))) {
            *pwr_consumption_in_ua = ral_sx126x_convert_tx_dbm_to_ua_reg_mode_dcdc_hp[index];
        } else {
            *pwr_consumption_in_ua = 30000;  // Default fallback
        }
    }
    
    ESP_LOGV(TAG, "TX power consumption: %lu uA for %d dBm", 
             *pwr_consumption_in_ua, power_dbm);
    
    return RAL_STATUS_OK;
}

ral_status_t ral_sx126x_bsp_get_instantaneous_gfsk_rx_power_consumption(
    const void* context, sx126x_reg_mod_t radio_reg_mode, bool rx_boosted, uint32_t* pwr_consumption_in_ua)
{
    (void)context;
    (void)radio_reg_mode;
    
    if (rx_boosted) {
        *pwr_consumption_in_ua = SX126X_GFSK_RX_BOOSTED_CONSUMPTION_DCDC;
    } else {
        *pwr_consumption_in_ua = SX126X_GFSK_RX_CONSUMPTION_DCDC;
    }
    
    ESP_LOGV(TAG, "GFSK RX power consumption: %lu uA (boosted=%s)", 
             *pwr_consumption_in_ua, rx_boosted ? "true" : "false");
    
    return RAL_STATUS_OK;
}

ral_status_t ral_sx126x_bsp_get_instantaneous_lora_rx_power_consumption(
    const void* context, sx126x_reg_mod_t radio_reg_mode, bool rx_boosted, uint32_t* pwr_consumption_in_ua)
{
    (void)context;
    (void)radio_reg_mode;
    
    if (rx_boosted) {
        *pwr_consumption_in_ua = SX126X_LORA_RX_BOOSTED_CONSUMPTION_DCDC;
    } else {
        *pwr_consumption_in_ua = SX126X_LORA_RX_CONSUMPTION_DCDC;
    }
    
    ESP_LOGV(TAG, "LoRa RX power consumption: %lu uA (boosted=%s)", 
             *pwr_consumption_in_ua, rx_boosted ? "true" : "false");
    
    return RAL_STATUS_OK;
}
