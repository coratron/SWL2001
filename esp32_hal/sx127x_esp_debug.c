/**
 * @file sx127x_esp_debug.c
 * @brief ESP32 debugging and inspection utilities for SX127x
 * 
 * This file implements debugging functions including register dump,
 * inspection utilities, and performance monitoring.
 */

#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "sx127x.h"
#include "sx127x_hal.h"
#include "sx127x_esp_wrapper.h"
#include "sx127x_esp_internal.h"

static const char* TAG = "sx127x_esp_debug";

#ifdef CONFIG_LBM_SX127X_ENABLE_REGISTER_DUMP

// SX127x register definitions for debugging
static const sx127x_esp_register_def_t sx127x_registers[] = {
    // Common registers
    {0x00, "RegFifo", "FIFO read/write access", true},
    {0x01, "RegOpMode", "Operating mode and LoRa/FSK selection", true},
    {0x02, "RegFrfMsb", "RF carrier frequency, MSB", true},
    {0x03, "RegFrfMid", "RF carrier frequency, intermediate", true},
    {0x04, "RegFrfLsb", "RF carrier frequency, LSB", true},
    {0x05, "RegPaConfig", "PA selection and output power control", true},
    {0x06, "RegPaRamp", "PA ramp time control", true},
    {0x07, "RegOcp", "Over current protection control", true},
    {0x08, "RegLna", "LNA settings", true},
    
    // LoRa mode registers
    {0x0D, "RegFifoAddrPtr", "FIFO SPI pointer", false},
    {0x0E, "RegFifoTxBaseAddr", "FIFO TX base address", false},
    {0x0F, "RegFifoRxBaseAddr", "FIFO RX base address", false},
    {0x10, "RegFifoRxCurrentAddr", "FIFO RX current address", false},
    {0x11, "RegIrqFlagsMask", "IRQ flags mask", false},
    {0x12, "RegIrqFlags", "IRQ flags", false},
    {0x13, "RegRxNbBytes", "Number of received bytes", false},
    {0x14, "RegRxHeaderCntValueMsb", "Number of valid headers received, MSB", false},
    {0x15, "RegRxHeaderCntValueLsb", "Number of valid headers received, LSB", false},
    {0x16, "RegRxPacketCntValueMsb", "Number of valid packets received, MSB", false},
    {0x17, "RegRxPacketCntValueLsb", "Number of valid packets received, LSB", false},
    {0x18, "RegModemStat", "Live LoRa modem status", false},
    {0x19, "RegPktSnrValue", "Last packet SNR value", false},
    {0x1A, "RegPktRssiValue", "Last packet RSSI value", false},
    {0x1B, "RegRssiValue", "Current RSSI value", false},
    {0x1C, "RegHopChannel", "FHSS start channel", false},
    {0x1D, "RegModemConfig1", "Modem PHY config 1", false},
    {0x1E, "RegModemConfig2", "Modem PHY config 2", false},
    {0x1F, "RegSymbTimeoutLsb", "Receiver timeout value", false},
    {0x20, "RegPreambleMsb", "Preamble length, MSB", false},
    {0x21, "RegPreambleLsb", "Preamble length, LSB", false},
    {0x22, "RegPayloadLength", "Payload length", false},
    {0x23, "RegMaxPayloadLength", "Maximum payload length", false},
    {0x24, "RegHopPeriod", "FHSS hop period", false},
    {0x25, "RegFifoRxByteAddr", "Address of last byte written in FIFO", false},
    {0x26, "RegModemConfig3", "Modem PHY config 3", false},
    
    // Common registers (continued)
    {0x40, "RegDioMapping1", "Mapping of pins DIO0 to DIO3", true},
    {0x41, "RegDioMapping2", "Mapping of pins DIO4 and DIO5, ClkOut frequency", true},
    {0x42, "RegVersion", "Semtech ID relating the silicon revision", true},
    
    // Additional LoRa registers
    {0x4B, "RegTcxo", "TCXO or XTAL input setting", false},
    {0x4D, "RegPaDac", "Higher power settings of the PA", true},
    {0x5B, "RegFormerTemp", "Stored temperature during the former IQ calibration", false},
    {0x61, "RegAgcRef", "Adjustment of the AGC thresholds", false},
    {0x62, "RegAgcThresh1", "Adjustment of the AGC thresholds", false},
    {0x63, "RegAgcThresh2", "Adjustment of the AGC thresholds", false},
    {0x64, "RegAgcThresh3", "Adjustment of the AGC thresholds", false},
};

#define SX127X_NUM_REGISTERS (sizeof(sx127x_registers) / sizeof(sx127x_registers[0]))

/**
 * @brief Dump all radio registers for debugging
 */
sx127x_esp_err_t sx127x_esp_dump_registers(sx127x_t* radio, sx127x_esp_register_info_t* reg_dump, size_t* count)
{
    if (!radio || !reg_dump || !count) {
        return SX127X_ESP_ERR_INVALID_ARG;
    }

    sx127x_esp_context_t* ctx = sx127x_esp_get_context(radio);
    if (!ctx || !ctx->initialized) {
        return SX127X_ESP_ERR_NOT_INITIALIZED;
    }

    size_t reg_count = 0;
    
    ESP_LOGI(TAG, "Dumping SX127x registers...");

    for (size_t i = 0; i < SX127X_NUM_REGISTERS && reg_count < *count; i++) {
        uint8_t value;
        sx127x_hal_status_t status = sx127x_hal_read(radio, sx127x_registers[i].address, &value, 1);
        
        if (status == SX127X_HAL_STATUS_OK) {
            reg_dump[reg_count].address = sx127x_registers[i].address;
            reg_dump[reg_count].value = value;
            reg_dump[reg_count].name = sx127x_registers[i].name;
            reg_dump[reg_count].description = sx127x_registers[i].description;
            reg_count++;
        } else {
            ESP_LOGW(TAG, "Failed to read register 0x%02X (%s)", 
                     sx127x_registers[i].address, sx127x_registers[i].name);
        }
    }

    *count = reg_count;
    ESP_LOGI(TAG, "Successfully dumped %zu registers", reg_count);
    
    return SX127X_ESP_OK;
}

/**
 * @brief Read a specific register by name
 */
sx127x_esp_err_t sx127x_esp_read_register_by_name(sx127x_t* radio, const char* reg_name, uint8_t* value)
{
    if (!radio || !reg_name || !value) {
        return SX127X_ESP_ERR_INVALID_ARG;
    }

    sx127x_esp_context_t* ctx = sx127x_esp_get_context(radio);
    if (!ctx || !ctx->initialized) {
        return SX127X_ESP_ERR_NOT_INITIALIZED;
    }

    // Find register by name
    for (size_t i = 0; i < SX127X_NUM_REGISTERS; i++) {
        if (strcmp(sx127x_registers[i].name, reg_name) == 0) {
            sx127x_hal_status_t status = sx127x_hal_read(radio, sx127x_registers[i].address, value, 1);
            if (status == SX127X_HAL_STATUS_OK) {
                ESP_LOGD(TAG, "Read %s (0x%02X) = 0x%02X", reg_name, sx127x_registers[i].address, *value);
                return SX127X_ESP_OK;
            } else {
                ESP_LOGE(TAG, "Failed to read register %s", reg_name);
                return SX127X_ESP_ERR_RADIO_NOT_FOUND;
            }
        }
    }

    ESP_LOGE(TAG, "Register '%s' not found", reg_name);
    return SX127X_ESP_ERR_INVALID_ARG;
}

/**
 * @brief Print register dump to console
 */
void sx127x_esp_print_register_dump(sx127x_t* radio)
{
    sx127x_esp_register_info_t reg_dump[SX127X_NUM_REGISTERS];
    size_t count = SX127X_NUM_REGISTERS;
    
    sx127x_esp_err_t err = sx127x_esp_dump_registers(radio, reg_dump, &count);
    if (err != SX127X_ESP_OK) {
        ESP_LOGE(TAG, "Failed to dump registers: %s", sx127x_esp_err_to_string(err));
        return;
    }

    printf("\n=== SX127x Register Dump ===\n");
    printf("Addr | Value | Name                    | Description\n");
    printf("-----|-------|-------------------------|------------------------------------------\n");
    
    for (size_t i = 0; i < count; i++) {
        printf("0x%02X | 0x%02X  | %-23s | %s\n",
               reg_dump[i].address,
               reg_dump[i].value,
               reg_dump[i].name,
               reg_dump[i].description);
    }
    
    printf("=============================\n\n");
}

#endif // CONFIG_LBM_SX127X_ENABLE_REGISTER_DUMP

/**
 * @brief Check if radio is responding
 */
bool sx127x_esp_is_radio_responding(sx127x_t* radio)
{
    if (!radio) {
        return false;
    }

    sx127x_esp_context_t* ctx = sx127x_esp_get_context(radio);
    if (!ctx || !ctx->initialized) {
        return false;
    }

    // Read version register
    uint8_t version;
    sx127x_hal_status_t status = sx127x_hal_read(radio, 0x42, &version, 1);
    
    if (status != SX127X_HAL_STATUS_OK) {
        ESP_LOGW(TAG, "Failed to read version register");
        return false;
    }

    // Check for valid version values
    // SX1272: 0x22, SX1276: 0x12
    if (version == 0x22 || version == 0x12) {
        ESP_LOGD(TAG, "Radio responding, version: 0x%02X", version);
        return true;
    }

    ESP_LOGW(TAG, "Unexpected version register value: 0x%02X", version);
    return false;
}

/**
 * @brief Get radio version information
 */
sx127x_esp_err_t sx127x_esp_get_radio_version(sx127x_t* radio, uint8_t* version)
{
    if (!radio || !version) {
        return SX127X_ESP_ERR_INVALID_ARG;
    }

    sx127x_esp_context_t* ctx = sx127x_esp_get_context(radio);
    if (!ctx || !ctx->initialized) {
        return SX127X_ESP_ERR_NOT_INITIALIZED;
    }

    sx127x_hal_status_t status = sx127x_hal_read(radio, 0x42, version, 1);
    if (status != SX127X_HAL_STATUS_OK) {
        ESP_LOGE(TAG, "Failed to read version register");
        return SX127X_ESP_ERR_RADIO_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Radio version: 0x%02X", *version);
    return SX127X_ESP_OK;
}

/**
 * @brief Get interrupt statistics
 */
sx127x_esp_err_t sx127x_esp_get_interrupt_stats(sx127x_t* radio, sx127x_esp_interrupt_stats_t* stats)
{
    if (!radio || !stats) {
        return SX127X_ESP_ERR_INVALID_ARG;
    }

    sx127x_esp_context_t* ctx = sx127x_esp_get_context(radio);
    if (!ctx || !ctx->initialized) {
        return SX127X_ESP_ERR_NOT_INITIALIZED;
    }

#ifdef CONFIG_LBM_SX127X_INTERRUPT_STATS
    // Take state mutex for thread safety
    if (xSemaphoreTake(ctx->state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        *stats = ctx->stats;
        xSemaphoreGive(ctx->state_mutex);
        return SX127X_ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to take state mutex for interrupt stats");
        return SX127X_ESP_ERR_TIMEOUT;
    }
#else
    memset(stats, 0, sizeof(sx127x_esp_interrupt_stats_t));
    ESP_LOGW(TAG, "Interrupt statistics not enabled in configuration");
    return SX127X_ESP_OK;
#endif
}

/**
 * @brief Reset interrupt statistics
 */
sx127x_esp_err_t sx127x_esp_reset_interrupt_stats(sx127x_t* radio)
{
    if (!radio) {
        return SX127X_ESP_ERR_INVALID_ARG;
    }

    sx127x_esp_context_t* ctx = sx127x_esp_get_context(radio);
    if (!ctx || !ctx->initialized) {
        return SX127X_ESP_ERR_NOT_INITIALIZED;
    }

#ifdef CONFIG_LBM_SX127X_INTERRUPT_STATS
    // Take state mutex for thread safety
    if (xSemaphoreTake(ctx->state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memset(&ctx->stats, 0, sizeof(ctx->stats));
        xSemaphoreGive(ctx->state_mutex);
        ESP_LOGI(TAG, "Interrupt statistics reset");
        return SX127X_ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to take state mutex for interrupt stats reset");
        return SX127X_ESP_ERR_TIMEOUT;
    }
#else
    ESP_LOGW(TAG, "Interrupt statistics not enabled in configuration");
    return SX127X_ESP_OK;
#endif
}

/**
 * @brief Get performance metrics
 */
sx127x_esp_err_t sx127x_esp_get_performance_metrics(sx127x_t* radio, sx127x_esp_performance_t* performance)
{
    if (!radio || !performance) {
        return SX127X_ESP_ERR_INVALID_ARG;
    }

    sx127x_esp_context_t* ctx = sx127x_esp_get_context(radio);
    if (!ctx || !ctx->initialized) {
        return SX127X_ESP_ERR_NOT_INITIALIZED;
    }

    // Take state mutex for thread safety
    if (xSemaphoreTake(ctx->state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Calculate average SPI transaction time
        if (ctx->spi_transaction_count > 0) {
            performance->spi_transaction_time_us = (uint32_t)(ctx->total_spi_time / ctx->spi_transaction_count);
        } else {
            performance->spi_transaction_time_us = 0;
        }

        // Other metrics would be calculated here
        performance->interrupt_latency_us = 0;  // TODO: Implement
        performance->tx_setup_time_us = 0;      // TODO: Implement
        performance->rx_setup_time_us = 0;      // TODO: Implement

        xSemaphoreGive(ctx->state_mutex);
        
        ESP_LOGD(TAG, "Performance metrics - SPI avg: %lu us", performance->spi_transaction_time_us);
        return SX127X_ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to take state mutex for performance metrics");
        return SX127X_ESP_ERR_TIMEOUT;
    }
}

/**
 * @brief Enable or disable DIO interrupts
 */
sx127x_esp_err_t sx127x_esp_dio_enable(sx127x_t* radio, bool enable)
{
    if (!radio) {
        return SX127X_ESP_ERR_INVALID_ARG;
    }

    sx127x_esp_context_t* ctx = sx127x_esp_get_context(radio);
    if (!ctx || !ctx->initialized) {
        return SX127X_ESP_ERR_NOT_INITIALIZED;
    }

    esp_err_t ret;
    
    if (enable && !ctx->interrupts_enabled) {
        // Enable interrupts
        ret = gpio_intr_enable(ctx->dio_pins[0]);
        ret |= gpio_intr_enable(ctx->dio_pins[1]);
        ret |= gpio_intr_enable(ctx->dio_pins[2]);
        
        if (ret == ESP_OK) {
            ctx->interrupts_enabled = true;
            ESP_LOGI(TAG, "DIO interrupts enabled");
        } else {
            ESP_LOGE(TAG, "Failed to enable DIO interrupts");
            return SX127X_ESP_ERR_INTERRUPT_INIT;
        }
    } else if (!enable && ctx->interrupts_enabled) {
        // Disable interrupts
        ret = gpio_intr_disable(ctx->dio_pins[0]);
        ret |= gpio_intr_disable(ctx->dio_pins[1]);
        ret |= gpio_intr_disable(ctx->dio_pins[2]);
        
        if (ret == ESP_OK) {
            ctx->interrupts_enabled = false;
            ESP_LOGI(TAG, "DIO interrupts disabled");
        } else {
            ESP_LOGE(TAG, "Failed to disable DIO interrupts");
            return SX127X_ESP_ERR_INTERRUPT_INIT;
        }
    }

    return SX127X_ESP_OK;
}
