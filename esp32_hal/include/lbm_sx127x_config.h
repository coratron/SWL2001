/**
 * @file lbm_sx127x_config.h
 * @brief LoRa Basics Modem SX127x ESP32 Configuration Header
 *
 * This file provides access to Kconfig values for LBM SX127x configuration.
 * Include this file to access hardware and software configuration parameters.
 */

#ifndef LBM_SX127X_CONFIG_H
#define LBM_SX127X_CONFIG_H

#include "sdkconfig.h"
#include "driver/gpio.h"  // For GPIO_NUM_NC

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Hardware Configuration
// ============================================================================

#define LBM_SX127X_SPI_HOST CONFIG_LBM_SX127X_SPI_HOST
#define LBM_SX127X_SPI_FREQUENCY CONFIG_LBM_SX127X_SPI_FREQUENCY
#define LBM_SX127X_USE_DMA CONFIG_LBM_SX127X_USE_DMA

// GPIO Pin Configuration
#define LBM_SX127X_NSS_GPIO CONFIG_LBM_SX127X_NSS_GPIO
#ifdef CONFIG_LBM_SX127X_USE_CUSTOM_RESET
#define LBM_SX127X_RESET_GPIO GPIO_NUM_NC  // Not used when custom reset is enabled
#else
#define LBM_SX127X_RESET_GPIO CONFIG_LBM_SX127X_RESET_GPIO
#endif
#define LBM_SX127X_DIO0_GPIO CONFIG_LBM_SX127X_DIO0_GPIO
#define LBM_SX127X_DIO1_GPIO CONFIG_LBM_SX127X_DIO1_GPIO
#define LBM_SX127X_DIO2_GPIO CONFIG_LBM_SX127X_DIO2_GPIO
#define LBM_SX127X_MISO_GPIO CONFIG_LBM_SX127X_MISO_GPIO
#define LBM_SX127X_MOSI_GPIO CONFIG_LBM_SX127X_MOSI_GPIO
#define LBM_SX127X_SCK_GPIO CONFIG_LBM_SX127X_SCK_GPIO

// Interrupt Configuration
#define LBM_SX127X_INTERRUPT_PRIORITY CONFIG_LBM_SX127X_INTERRUPT_PRIORITY
#define LBM_SX127X_USE_DEDICATED_TASK CONFIG_LBM_SX127X_USE_DEDICATED_TASK

#ifdef CONFIG_LBM_SX127X_USE_DEDICATED_TASK
#define LBM_SX127X_EVENT_TASK_PRIORITY CONFIG_LBM_SX127X_EVENT_TASK_PRIORITY
#define LBM_SX127X_EVENT_TASK_STACK_SIZE CONFIG_LBM_SX127X_EVENT_TASK_STACK_SIZE
#endif

#define LBM_SX127X_EVENT_QUEUE_SIZE CONFIG_LBM_SX127X_EVENT_QUEUE_SIZE

// ============================================================================
// Radio Type Configuration
// ============================================================================

#ifdef CONFIG_LBM_SX127X_RADIO_SX1272
#define LBM_SX127X_RADIO_TYPE "SX1272"
#define LBM_SX127X_IS_SX1272 1
#define LBM_SX127X_IS_SX1276 0
#elif CONFIG_LBM_SX127X_RADIO_SX1276
#define LBM_SX127X_RADIO_TYPE "SX1276"
#define LBM_SX127X_IS_SX1272 0
#define LBM_SX127X_IS_SX1276 1
#endif

// ============================================================================
// LoRaWAN Configuration
// ============================================================================

#define LBM_NUMBER_OF_STACKS CONFIG_LBM_NUMBER_OF_STACKS

// Map LBM_NUMBER_OF_STACKS to NUMBER_OF_STACKS for LBM library compatibility
#define NUMBER_OF_STACKS LBM_NUMBER_OF_STACKS

// Region Configuration
#ifdef CONFIG_LBM_REGION_EU868
#define LBM_LORAWAN_REGION "EU868"
#define LBM_LORAWAN_REGION_EU868 1
#elif CONFIG_LBM_REGION_US915
#define LBM_LORAWAN_REGION "US915"
#define LBM_LORAWAN_REGION_US915 1
#elif CONFIG_LBM_REGION_AS923
#define LBM_LORAWAN_REGION "AS923"
#define LBM_LORAWAN_REGION_AS923 1
#elif CONFIG_LBM_REGION_AU915
#define LBM_LORAWAN_REGION "AU915"
#define LBM_LORAWAN_REGION_AU915 1
#elif CONFIG_LBM_REGION_CN470
#define LBM_LORAWAN_REGION "CN470"
#define LBM_LORAWAN_REGION_CN470 1
#elif CONFIG_LBM_REGION_IN865
#define LBM_LORAWAN_REGION "IN865"
#define LBM_LORAWAN_REGION_IN865 1
#elif CONFIG_LBM_REGION_KR920
#define LBM_LORAWAN_REGION "KR920"
#define LBM_LORAWAN_REGION_KR920 1
#elif CONFIG_LBM_REGION_RU864
#define LBM_LORAWAN_REGION "RU864"
#define LBM_LORAWAN_REGION_RU864 1
#endif

// LoRaWAN Version Configuration
#ifdef CONFIG_LBM_LORAWAN_VERSION_RP2_101
#define LBM_LORAWAN_VERSION "RP2-1.0.1"
#elif CONFIG_LBM_LORAWAN_VERSION_RP2_103
#define LBM_LORAWAN_VERSION "RP2-1.0.3"
#elif CONFIG_LBM_LORAWAN_VERSION_RP2_104
#define LBM_LORAWAN_VERSION "RP2-1.0.4"
#endif

// Class Support
#define LBM_ENABLE_CLASS_B CONFIG_LBM_ENABLE_CLASS_B
#define LBM_ENABLE_CLASS_C CONFIG_LBM_ENABLE_CLASS_C
#define LBM_ENABLE_MULTICAST CONFIG_LBM_ENABLE_MULTICAST
#define LBM_ENABLE_FUOTA CONFIG_LBM_ENABLE_FUOTA

// ============================================================================
// Security Configuration
// ============================================================================

#ifdef CONFIG_LBM_SOFT_SE
#define LBM_SECURE_ELEMENT_TYPE "Software"
#define LBM_USE_SOFT_SE 1
#define LBM_USE_HARD_SE 0
#elif CONFIG_LBM_HARD_SE
#define LBM_SECURE_ELEMENT_TYPE "Hardware"
#define LBM_USE_SOFT_SE 0
#define LBM_USE_HARD_SE 1
#endif

#define LBM_ENABLE_CERTIFICATION CONFIG_LBM_ENABLE_CERTIFICATION
#define LBM_ENABLE_STORE_AND_FORWARD CONFIG_LBM_ENABLE_STORE_AND_FORWARD

// ============================================================================
// Debug and Tracing Configuration
// ============================================================================

#define LBM_ENABLE_HAL_DBG_TRACE CONFIG_LBM_ENABLE_HAL_DBG_TRACE
#define LBM_ENABLE_MODEM_TRACE CONFIG_LBM_ENABLE_MODEM_TRACE
#define LBM_ENABLE_RADIO_TRACE CONFIG_LBM_ENABLE_RADIO_TRACE
#define LBM_ENABLE_LR1MAC_TRACE CONFIG_LBM_ENABLE_LR1MAC_TRACE
#define LBM_ENABLE_CRYPTO_TRACE CONFIG_LBM_ENABLE_CRYPTO_TRACE
#define LBM_TRACE_LEVEL CONFIG_LBM_TRACE_LEVEL

// ============================================================================
// Advanced Configuration
// ============================================================================

#define LBM_MAX_NB_OF_RX_WINDOW CONFIG_LBM_MAX_NB_OF_RX_WINDOW
#define LBM_CRYSTAL_ERROR_PPM CONFIG_LBM_CRYSTAL_ERROR_PPM

#define LBM_ENABLE_STREAM CONFIG_LBM_ENABLE_STREAM
#define LBM_ENABLE_LFU CONFIG_LBM_ENABLE_LFU
#define LBM_ENABLE_DM_INFO_SERVICES CONFIG_LBM_ENABLE_DM_INFO_SERVICES
#define LBM_ENABLE_ALMANAC_SERVICES CONFIG_LBM_ENABLE_ALMANAC_SERVICES
#define LBM_CUSTOM_RADIO_PLANNER CONFIG_LBM_CUSTOM_RADIO_PLANNER
#define LBM_ENABLE_CSMA CONFIG_LBM_ENABLE_CSMA
#define LBM_ENABLE_LBT CONFIG_LBM_ENABLE_LBT

// ============================================================================
// Power Management Configuration
// ============================================================================

#define LBM_ENABLE_LOW_POWER_MODE CONFIG_LBM_ENABLE_LOW_POWER_MODE
#define LBM_ENABLE_POWER_OPTIMIZATION CONFIG_LBM_ENABLE_POWER_OPTIMIZATION

#ifdef CONFIG_LBM_ENABLE_LOW_POWER_MODE
#define LBM_LOW_POWER_TIMER_MS CONFIG_LBM_LOW_POWER_TIMER_MS
#endif

// ============================================================================
// Feature Configuration Helpers
// ============================================================================

// Modulation support
#define LBM_SX127X_ENABLE_LORA CONFIG_LBM_SX127X_ENABLE_LORA
#define LBM_SX127X_ENABLE_GFSK CONFIG_LBM_SX127X_ENABLE_GFSK
#define LBM_SX127X_ENABLE_OOK CONFIG_LBM_SX127X_ENABLE_OOK

// Debug features
#define LBM_SX127X_DEBUG_SPI CONFIG_LBM_SX127X_DEBUG_SPI
#define LBM_SX127X_DEBUG_INTERRUPTS CONFIG_LBM_SX127X_DEBUG_INTERRUPTS
#define LBM_SX127X_INTERRUPT_STATS CONFIG_LBM_SX127X_INTERRUPT_STATS
#define LBM_SX127X_ENABLE_REGISTER_DUMP CONFIG_LBM_SX127X_ENABLE_REGISTER_DUMP

// Memory configuration
#define LBM_SX127X_STATIC_ALLOCATION CONFIG_LBM_SX127X_STATIC_ALLOCATION
#define LBM_SX127X_SPI_BUFFER_SIZE CONFIG_LBM_SX127X_SPI_BUFFER_SIZE

// Testing
#define LBM_SX127X_ENABLE_SELF_TEST CONFIG_LBM_SX127X_ENABLE_SELF_TEST
#define LBM_SX127X_TEST_SPI_LOOPBACK CONFIG_LBM_SX127X_TEST_SPI_LOOPBACK

// NVS Configuration
#define LBM_SX127X_NVS_NAMESPACE CONFIG_LBM_SX127X_NVS_NAMESPACE
#define LBM_SX127X_NVS_ENABLE_ENCRYPTION CONFIG_LBM_SX127X_NVS_ENABLE_ENCRYPTION

// ============================================================================
// Configuration Validation Macros
// ============================================================================

/**
 * @brief Validate that essential configuration is present
 */
#define LBM_CONFIG_VALIDATE()                                                  \
  do {                                                                         \
    _Static_assert(LBM_NUMBER_OF_STACKS >= 1 && LBM_NUMBER_OF_STACKS <= 4,     \
                   "LBM_NUMBER_OF_STACKS must be between 1 and 4");            \
    _Static_assert(LBM_SX127X_SPI_FREQUENCY >= 1000000 &&                      \
                       LBM_SX127X_SPI_FREQUENCY <= 20000000,                   \
                   "LBM_SX127X_SPI_FREQUENCY must be between 1MHz and 20MHz"); \
    _Static_assert(LBM_TRACE_LEVEL >= 0 && LBM_TRACE_LEVEL <= 5,               \
                   "LBM_TRACE_LEVEL must be between 0 and 5");                 \
  } while (0)

#ifdef __cplusplus
}
#endif

#endif // LBM_SX127X_CONFIG_H
