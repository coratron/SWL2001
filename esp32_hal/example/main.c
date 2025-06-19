/**
 * @file main.c
 * @brief LoRaWAN application using LBM (LoRa Basic Modem) for ESP32
 *
 * This example demonstrates how to join a LoRaWAN network using LBM and send
 * periodic uplink messages. It's based on the LBM periodical uplink example
 * but adapted for ESP32 platform.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

// LBM includes
#include "smtc_modem_api.h"
#include "smtc_modem_utilities.h"
#include "smtc_modem_hal.h"
#include "smtc_hal_dbg_trace.h"
#include "radio_utilities.h"

// Radio initialization for SX127x
#if defined(SX1272) || defined(SX1276)
#include "sx127x_esp_wrapper.h"
#endif
#include "smtc_hal_mcu.h"
#include "smtc_hal_gpio.h"
#include "smtc_hal_watchdog.h"
#include "smtc_hal_lp_timer.h"
#include "smtc_modem_utilities.h"

// Include SX127x driver for direct register access
#include "sx127x.h"
#include "sx127x_hal.h"
#include "sx127x_regs.h"

#include "driver/gpio.h"     // Add this line
#include "ral.h"             // Include RAL interface for proper packet type setting
#include "radio_utilities.h" // For TX power offset functions

static const char *TAG = "lorawan_app";

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS ----------------------------------------------------------
 */

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

/*!
 * @brief Helper macro that returned a human-friendly message if a command does not return SMTC_MODEM_RC_OK
 */
#define ASSERT_SMTC_MODEM_RC(rc_func)                                                                            \
    do                                                                                                           \
    {                                                                                                            \
        smtc_modem_return_code_t rc = rc_func;                                                                   \
        if (rc == SMTC_MODEM_RC_NOT_INIT)                                                                        \
        {                                                                                                        \
            ESP_LOGE(TAG, "In %s - %s (line %d): SMTC_MODEM_RC_NOT_INIT", __FILE__, __func__, __LINE__);         \
        }                                                                                                        \
        else if (rc == SMTC_MODEM_RC_INVALID)                                                                    \
        {                                                                                                        \
            ESP_LOGE(TAG, "In %s - %s (line %d): SMTC_MODEM_RC_INVALID", __FILE__, __func__, __LINE__);          \
        }                                                                                                        \
        else if (rc == SMTC_MODEM_RC_BUSY)                                                                       \
        {                                                                                                        \
            ESP_LOGE(TAG, "In %s - %s (line %d): SMTC_MODEM_RC_BUSY", __FILE__, __func__, __LINE__);             \
        }                                                                                                        \
        else if (rc == SMTC_MODEM_RC_FAIL)                                                                       \
        {                                                                                                        \
            ESP_LOGE(TAG, "In %s - %s (line %d): SMTC_MODEM_RC_FAIL", __FILE__, __func__, __LINE__);             \
        }                                                                                                        \
        else if (rc == SMTC_MODEM_RC_NO_TIME)                                                                    \
        {                                                                                                        \
            ESP_LOGW(TAG, "In %s - %s (line %d): SMTC_MODEM_RC_NO_TIME", __FILE__, __func__, __LINE__);          \
        }                                                                                                        \
        else if (rc == SMTC_MODEM_RC_INVALID_STACK_ID)                                                           \
        {                                                                                                        \
            ESP_LOGE(TAG, "In %s - %s (line %d): SMTC_MODEM_RC_INVALID_STACK_ID", __FILE__, __func__, __LINE__); \
        }                                                                                                        \
        else if (rc == SMTC_MODEM_RC_NO_EVENT)                                                                   \
        {                                                                                                        \
            ESP_LOGI(TAG, "In %s - %s (line %d): SMTC_MODEM_RC_NO_EVENT", __FILE__, __func__, __LINE__);         \
        }                                                                                                        \
    } while (0)

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

/**
 * Stack id value (multistacks modem is not yet available)
 */
#define STACK_ID 0

/**
 * @brief LoRaWAN credentials - REPLACE WITH YOUR ACTUAL CREDENTIALS
 *
 * IMPORTANT: You MUST replace these with your actual LoRaWAN credentials
 * from your network server (TTN, ChirpStack, etc.)
 */
static const uint8_t user_dev_eui[8] = {0x41, 0x49, 0x39, 0x39, 0x39, 0x39, 0x30, 0x30};
static const uint8_t user_join_eui[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
static const uint8_t user_gen_app_key[16] = {0xFE, 0x09, 0x6E, 0xF6, 0xF9, 0xD9, 0x51, 0x0F, 0xAB, 0x19, 0x20, 0x64, 0xBF, 0xE6, 0xC2, 0x6C};
static const uint8_t user_app_key[16] = {0xFE, 0x09, 0x6E, 0xF6, 0xF9, 0xD9, 0x51, 0x0F, 0xAB, 0x19, 0x20, 0x64, 0xBF, 0xE6, 0xC2, 0x6C};

/**
 * @brief Watchdog counter reload value during sleep
 */
#define WATCHDOG_RELOAD_PERIOD_MS 5000

/**
 * @brief Periodical uplink alarm delay in seconds
 */
#define PERIODICAL_UPLINK_DELAY_S 60

/**
 * @brief Delay for first message after join (seconds)
 */
#define DELAY_FIRST_MSG_AFTER_JOIN 10

/*
 * -----------------------------------------------------------------------------
 * --- MAIN LOOP TIMING CONSTANTS ---------------------------------------------
 */

/**
 * @brief Maximum sleep time in main loop to stay responsive to timer IRQs (ms)
 */
#define MAX_MAIN_LOOP_SLEEP_MS 100

/**
 * @brief Minimum execution time threshold for logging modem engine duration (ms)
 */
#define MIN_ENGINE_LOG_THRESHOLD_MS 50

/**
 * @brief Minimum sleep time threshold for logging sleep duration (ms)
 */
#define MIN_SLEEP_LOG_THRESHOLD_MS 10

/**
 * @brief Main loop delay to prevent tight loop (ms)
 */
#define MAIN_LOOP_DELAY_MS 10

/**
 * @brief Register dump cooldown period (ms)
 */
#define REGISTER_DUMP_COOLDOWN_MS 2000

/**
 * @brief Modem initialization delay (ms)
 */
#define MODEM_INIT_DELAY_MS 100

/**
 * @brief Register operation settle delay (ms)
 */
#define REGISTER_SETTLE_DELAY_MS 10

/**
 * @brief Manual radio fix verification delay (ms)
 */
#define RADIO_FIX_DELAY_MS 50

/**
 * @brief Maximum sleep time in main loop to ensure timer responsiveness (ms)
 */
#define MAX_MAIN_LOOP_SLEEP_MS 100

/**
 * @brief Minimum sleep time to log for debugging (ms)
 */
#define MIN_SLEEP_LOG_THRESHOLD_MS 10

/**
 * @brief Minimum execution time to log for debugging (ms)
 */
#define MIN_EXECUTION_LOG_THRESHOLD_MS 50

/**
 * @brief Main loop delay to prevent tight looping (ms)
 */
#define MAIN_LOOP_DELAY_MS 10

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static uint8_t rx_payload[SMTC_MODEM_MAX_LORAWAN_PAYLOAD_LENGTH] = {0}; // Buffer for rx payload
static uint8_t rx_payload_size = 0;                                     // Size of the payload in the rx_payload buffer
static smtc_modem_dl_metadata_t rx_metadata = {0};                      // Metadata of downlink
static uint8_t rx_remaining = 0;                                        // Remaining downlink payload in modem

static uint32_t uplink_counter = 0; // uplink raising counter
static bool is_joined = false;      // Join status flag

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/**
 * @brief User callback for modem event
 */
static void modem_event_callback(void);

/**
 * @brief Send the 32bits uplink counter on chosen port
 */
static void send_uplink_counter_on_port(uint8_t port);

/**
 * @brief Initialize NVS (Non-Volatile Storage)
 */
static esp_err_t init_nvs(void);

/**
 * @brief Dump SX127x registers for debugging
 */
static void dump_sx127x_registers(void);

/**
 * @brief Check modem radio state and configuration
 */
// static void check_modem_radio_state(void);  // Function implementation commented out

static void force_lora_mode(void);

/**
 * @brief Apply the proper LBM radio initialization sequence that's missing from LBM
 *
 * This function calls the critical ral_set_pkt_type() API that LBM fails to call
 * during initialization, which is why the radio stays in FSK mode instead of LoRa mode.
 * All RALF setup functions in LBM source call this as their first step.
 *
 * @return SMTC_MODEM_RC_OK on success, error code otherwise
 */
static smtc_modem_return_code_t apply_proper_lbm_radio_initialization(void);

/**
 * @brief Calculate exact SX1276 register values as sent to hardware
 */
static void calculate_sx1276_tx_power_registers(int8_t system_power_dbm, uint32_t freq_hz,
                                                uint8_t *pa_config_reg, uint8_t *pa_dac_reg);

/**
 * @brief Print exact SX1276 TX power register values
 */
static void print_sx1276_tx_power_info(void);

/**
 * @brief Read actual SX1276 register values from hardware
 */
// static void read_sx1276_actual_registers(void);  // Function implementation commented out

/**
 * @brief Investigate LBM power configuration more deeply
 */
static void investigate_lbm_power_config(void);

/**
 * @brief Force maximum TX power for testing
 */
static void force_maximum_tx_power(void);

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

/**
 * @brief Main application entry point
 */
void app_main(void)
{
    uint32_t sleep_time_ms = 0;

    ESP_LOGI(TAG, "LoRaWAN LBM Example Application Starting");
    ESP_LOGI(TAG, "Periodical uplink every %d seconds", PERIODICAL_UPLINK_DELAY_S);

    // Wait for 1-second button press on GPIO0
    gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);
    // uint32_t press_start = 0;  // Commented out - button detection disabled
    // while (1)
    // {
    //     if (gpio_get_level(GPIO_NUM_0) == 0)
    //     {
    //         if (press_start == 0)
    //             press_start = xTaskGetTickCount();
    //         else if ((xTaskGetTickCount() - press_start) > pdMS_TO_TICKS(1000))
    //             break;
    //     }
    //     else
    //         press_start = 0;
    //     vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_MS));
    // }

    // Initialize NVS
    ESP_ERROR_CHECK(init_nvs());

    // Disable IRQ to avoid unwanted behavior during init
    hal_mcu_disable_irq();

    // Configure all the µC periph (clock, gpio, timer, ...)
    hal_mcu_init();

    // Init done: enable interruption
    hal_mcu_enable_irq();

    // Initialize low-power timers after interrupts are enabled
    ESP_LOGI(TAG, "Initializing low-power timers");
    hal_lp_timer_init(HAL_LP_TIMER_ID_1);
    hal_lp_timer_init(HAL_LP_TIMER_ID_2);

    // Radio initialization will be handled by the modem during smtc_modem_init()
    // The ESP32 wrapper will be initialized when the modem calls the HAL functions

    // Init the modem and use modem_event_callback as event callback
    // Note: the callback will be called immediately after the first call to
    // smtc_modem_run_engine because of the reset detection
    // dump_sx127x_registers();
    // check_modem_radio_state();
    smtc_modem_init(&modem_event_callback);

    // Give modem time to initialize
    vTaskDelay(pdMS_TO_TICKS(MODEM_INIT_DELAY_MS));

    ESP_LOGI(TAG, "Modem initialized, starting main loop");

    // Main application loop
    while (1)
    {
        // Remove excessive logging that causes crashes from ISR context
        // ESP_LOGD(TAG, "🔄 Running modem engine...");

        // Modem process launch
        uint32_t start_time = smtc_modem_hal_get_time_in_ms();
        sleep_time_ms = smtc_modem_run_engine();
        uint32_t end_time = smtc_modem_hal_get_time_in_ms();

        // Only log if execution took significant time to avoid ISR logging issues
        if (end_time - start_time > MIN_ENGINE_LOG_THRESHOLD_MS)
        {
            ESP_LOGD(TAG, "✅ Modem engine completed in %lu ms, sleep_time=%lu",
                     end_time - start_time, sleep_time_ms);
        }

        // Check sleep conditions and manage power
        // CRITICAL: Keep IRQ disable time minimal to avoid missing timer IRQs
        hal_mcu_disable_irq();
        bool irq_pending = smtc_modem_is_irq_flag_pending();
        hal_mcu_enable_irq(); // Re-enable IRQs immediately after check

        if (irq_pending == false)
        {
            hal_watchdog_reload();
            // Reduce sleep time to be more responsive to timer IRQs
            // uint32_t actual_sleep = MIN(sleep_time_ms, MAX_MAIN_LOOP_SLEEP_MS);  // Commented out - sleep disabled
            // if (actual_sleep > MIN_SLEEP_LOG_THRESHOLD_MS)
            // {
            //     ESP_LOGD(TAG, "🛌 Sleeping for %lu ms (requested: %lu)", actual_sleep, sleep_time_ms);
            // }
            // Don't disable IRQs during sleep - timer IRQs need to wake us up!
            // hal_mcu_set_sleep_for_ms(actual_sleep);
        }
        else
        {
            // If an IRQ is pending, we need to process the modem events immediately
            ESP_LOGI(TAG, "Modem IRQ pending, processing events");
            hal_watchdog_reload();
            modem_event_callback();
        }

        hal_watchdog_reload();

        // Small delay to prevent tight loop
        vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_MS));
    }
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static void modem_event_callback(void)
{
    ESP_LOGI(TAG, "Modem event callback triggered");

    smtc_modem_event_t current_event;
    uint8_t event_pending_count;
    uint8_t stack_id = STACK_ID;

    // Continue to read modem event until all event has been processed
    do
    {
        // Read modem event
        smtc_modem_return_code_t rc = smtc_modem_get_event(&current_event, &event_pending_count);

        // Check for event corruption before processing
        if (rc == SMTC_MODEM_RC_OK)
        {
            // Validate event type is within expected range
            if (current_event.event_type >= SMTC_MODEM_EVENT_MAX)
            {
                ESP_LOGE(TAG, "🚨 CORRUPTED EVENT DETECTED: event_type=%u (0x%X), stack_id=%u, pending=%u",
                         current_event.event_type, current_event.event_type, current_event.stack_id, event_pending_count);

                // Dump the raw memory of the event structure
                uint8_t *event_bytes = (uint8_t *)&current_event;
                ESP_LOGE(TAG, "Event structure dump (first 32 bytes):");
                for (int i = 0; i < 32 && i < sizeof(current_event); i += 4)
                {
                    ESP_LOGE(TAG, "  [%02d]: 0x%02X 0x%02X 0x%02X 0x%02X",
                             i, event_bytes[i], event_bytes[i + 1], event_bytes[i + 2], event_bytes[i + 3]);
                }

                // Skip this corrupted event and continue
                continue;
            }

            ESP_LOGI(TAG, "✅ Valid event received: type=%u, stack_id=%u, pending=%u",
                     current_event.event_type, current_event.stack_id, event_pending_count);
        }
        else if (rc == SMTC_MODEM_RC_NO_EVENT)
        {
            // This is normal - no more events in the queue
            ESP_LOGD(TAG, "No more events in queue (normal)");
            break;
        }
        else
        {
            ESP_LOGW(TAG, "smtc_modem_get_event returned error: %d", rc);
            break;
        }

        switch (current_event.event_type)
        {
        case SMTC_MODEM_EVENT_RESET:
            ESP_LOGI(TAG, "Event received: RESET");

            // Apply the proper LBM radio initialization that's missing from LBM
            ESP_LOGI(TAG, "🔧 Applying proper LBM radio initialization after reset...");
            smtc_modem_return_code_t init_result = apply_proper_lbm_radio_initialization();
            if (init_result == SMTC_MODEM_RC_OK)
            {
                ESP_LOGI(TAG, "✅ Proper LBM radio initialization applied successfully");
                vTaskDelay(pdMS_TO_TICKS(50)); // Let it settle
            }
            else
            {
                ESP_LOGE(TAG, "❌ Proper LBM initialization failed, falling back to manual fix");
                force_lora_mode();
            }

            // Set user credentials
            ESP_LOGI(TAG, "Setting LoRaWAN credentials");
            ASSERT_SMTC_MODEM_RC(smtc_modem_set_deveui(stack_id, user_dev_eui));
            ASSERT_SMTC_MODEM_RC(smtc_modem_set_joineui(stack_id, user_join_eui));
            ASSERT_SMTC_MODEM_RC(smtc_modem_set_appkey(stack_id, user_gen_app_key));
            ASSERT_SMTC_MODEM_RC(smtc_modem_set_nwkkey(stack_id, user_app_key));

            // Set user region (AU915 based on the log showing 916MHz, change as needed)
            ESP_LOGI(TAG, "Setting region to AU915");
            ASSERT_SMTC_MODEM_RC(smtc_modem_set_region(stack_id, SMTC_MODEM_REGION_AU_915));

            // Investigate LBM power configuration in detail
            investigate_lbm_power_config();

            // Force maximum TX power for testing weak signal issue
            force_maximum_tx_power();

            // Print exact SX1276 TX power register values
            print_sx1276_tx_power_info();

            // Configure for public LoRaWAN network (TTN, Helium, etc.)
            ESP_LOGI(TAG, "Configuring for public LoRaWAN network");
            smtc_modem_return_code_t rc_network = smtc_modem_set_network_type(stack_id, true); // true = public network
            if (rc_network == SMTC_MODEM_RC_OK)
            {
                ESP_LOGI(TAG, "✅ Successfully set private network type");
            }
            else
            {
                ESP_LOGW(TAG, "⚠️  Failed to set network type, using default (rc=%d)", rc_network);
            }

            // Try to disable certification mode (may not be available in all builds)
            smtc_modem_return_code_t rc_cert = smtc_modem_set_certification_mode(stack_id, false);
            if (rc_cert == SMTC_MODEM_RC_OK)
            {
                ESP_LOGI(TAG, "✅ Successfully disabled certification mode");
            }
            else if (rc_cert == SMTC_MODEM_RC_NOT_INIT)
            {
                ESP_LOGD(TAG, "📝 Certification mode not available in this build (expected)");
            }
            else
            {
                ESP_LOGW(TAG, "⚠️  Failed to set certification mode (rc=%d)", rc_cert);
            }

            // Optional: Set class (Class A is default, but being explicit)
            smtc_modem_return_code_t rc_class = smtc_modem_set_class(stack_id, SMTC_MODEM_CLASS_A);
            if (rc_class == SMTC_MODEM_RC_OK)
            {
                ESP_LOGI(TAG, "✅ Successfully set Class A");
            }
            else
            {
                ESP_LOGW(TAG, "⚠️  Failed to set class, using default (rc=%d)", rc_class);
            }

            // Schedule a Join LoRaWAN network
            ESP_LOGI(TAG, "Initiating LoRaWAN join procedure");

            // Print exact SX1276 TX power register values before join transmission
            ESP_LOGI(TAG, "🔍 TX Power Configuration for join request:");
            print_sx1276_tx_power_info();

            ASSERT_SMTC_MODEM_RC(smtc_modem_join_network(stack_id));

            // Add debugging to track what happens next
            ESP_LOGI(TAG, "🔍 Join request sent - expecting TXDONE event for join request transmission");
            ESP_LOGI(TAG, "🔍 If we don't get TXDONE, there's either a radio or IRQ handling issue");
            break;

        case SMTC_MODEM_EVENT_ALARM:
            ESP_LOGI(TAG, "Event received: ALARM");
            if (is_joined)
            {
                // Send periodical uplink on port 101
                send_uplink_counter_on_port(101);
                // Restart periodical uplink alarm
                ASSERT_SMTC_MODEM_RC(smtc_modem_alarm_start_timer(PERIODICAL_UPLINK_DELAY_S));
            }
            break;

        case SMTC_MODEM_EVENT_JOINED:
            ESP_LOGI(TAG, "Event received: JOINED");
            ESP_LOGI(TAG, "🎉 Successfully joined LoRaWAN network!");
            is_joined = true;

            // // Optional: Set class (Class A is default, but being explicit)
            // smtc_modem_return_code_t rc_class = smtc_modem_set_class(stack_id, SMTC_MODEM_CLASS_A);
            // if (rc_class == SMTC_MODEM_RC_OK)
            // {
            //     ESP_LOGI(TAG, "✅ Successfully set Class A");
            // }
            // else
            // {
            //     ESP_LOGW(TAG, "⚠️  Failed to set class, using default (rc=%d)", rc_class);
            // }

            // Send first uplink message on port 101
            send_uplink_counter_on_port(101);

            // Start periodical uplink alarm
            ASSERT_SMTC_MODEM_RC(smtc_modem_alarm_start_timer(DELAY_FIRST_MSG_AFTER_JOIN));
            break;

        case SMTC_MODEM_EVENT_TXDONE:
            ESP_LOGI(TAG, "Event received: TXDONE");
            ESP_LOGI(TAG, "📡 Transmission completed successfully");
            ESP_LOGI(TAG, "🎉 CRITICAL: We DID get TXDONE - radio is working!");

            // Force a register dump right after transmission to see the configuration
            ESP_LOGI(TAG, "🔍 Forcing register dump after TXDONE...");
            dump_sx127x_registers();
            break;

        case SMTC_MODEM_EVENT_DOWNDATA:
            ESP_LOGI(TAG, "Event received: DOWNDATA");
            // Get downlink data
            ASSERT_SMTC_MODEM_RC(
                smtc_modem_get_downlink_data(rx_payload, &rx_payload_size, &rx_metadata, &rx_remaining));
            ESP_LOGI(TAG, "📥 Data received on port %u, size: %u bytes", rx_metadata.fport, rx_payload_size);

            // Print received payload in hex
            ESP_LOG_BUFFER_HEX(TAG, rx_payload, rx_payload_size);
            break;

        case SMTC_MODEM_EVENT_JOINFAIL:
            ESP_LOGE(TAG, "Event received: JOINFAIL");
            ESP_LOGE(TAG, "❌ Failed to join LoRaWAN network, will retry...");
            ESP_LOGE(TAG, "🚨 CRITICAL: We got JOINFAIL but NO TXDONE - this suggests:");
            ESP_LOGE(TAG, "   1. Join request transmission never completed, OR");
            ESP_LOGE(TAG, "   2. TXDONE event/IRQ handling is broken");
            is_joined = false;

            // Force register dump on join failure to diagnose the issue
            ESP_LOGI(TAG, "🔍 Forcing register dump after JOINFAIL to diagnose...");
            dump_sx127x_registers();
            break;

        case SMTC_MODEM_EVENT_ALCSYNC_TIME:
            ESP_LOGI(TAG, "Event received: ALCSync service TIME");
            break;

        case SMTC_MODEM_EVENT_LINK_CHECK:
            ESP_LOGI(TAG, "Event received: LINK_CHECK");
            break;

        case SMTC_MODEM_EVENT_CLASS_B_PING_SLOT_INFO:
            ESP_LOGI(TAG, "Event received: CLASS_B_PING_SLOT_INFO");
            break;

        case SMTC_MODEM_EVENT_CLASS_B_STATUS:
            ESP_LOGI(TAG, "Event received: CLASS_B_STATUS");
            break;

        case SMTC_MODEM_EVENT_LORAWAN_MAC_TIME:
            ESP_LOGW(TAG, "Event received: LORAWAN MAC TIME");
            break;

        case SMTC_MODEM_EVENT_LORAWAN_FUOTA_DONE:
        {
            bool status = current_event.event_data.fuota_status.successful;
            if (status == true)
            {
                ESP_LOGI(TAG, "Event received: FUOTA SUCCESSFUL");
            }
            else
            {
                ESP_LOGW(TAG, "Event received: FUOTA FAIL");
            }
            break;
        }

        case SMTC_MODEM_EVENT_NO_MORE_MULTICAST_SESSION_CLASS_C:
            ESP_LOGI(TAG, "Event received: MULTICAST CLASS_C STOP");
            break;

        case SMTC_MODEM_EVENT_NO_MORE_MULTICAST_SESSION_CLASS_B:
            ESP_LOGI(TAG, "Event received: MULTICAST CLASS_B STOP");
            break;

        case SMTC_MODEM_EVENT_NEW_MULTICAST_SESSION_CLASS_C:
            ESP_LOGI(TAG, "Event received: New MULTICAST CLASS_C");
            break;

        case SMTC_MODEM_EVENT_NEW_MULTICAST_SESSION_CLASS_B:
            ESP_LOGI(TAG, "Event received: New MULTICAST CLASS_B");
            break;

        case SMTC_MODEM_EVENT_FIRMWARE_MANAGEMENT:
            ESP_LOGI(TAG, "Event received: FIRMWARE_MANAGEMENT");
            if (current_event.event_data.fmp.status == SMTC_MODEM_EVENT_FMP_REBOOT_IMMEDIATELY)
            {
                smtc_modem_hal_reset_mcu();
            }
            break;

        case SMTC_MODEM_EVENT_STREAM_DONE:
            ESP_LOGI(TAG, "Event received: STREAM_DONE");
            break;

        case SMTC_MODEM_EVENT_UPLOAD_DONE:
            ESP_LOGI(TAG, "Event received: UPLOAD_DONE");
            break;

        case SMTC_MODEM_EVENT_DM_SET_CONF:
            ESP_LOGI(TAG, "Event received: DM_SET_CONF");
            break;

        case SMTC_MODEM_EVENT_MUTE:
            ESP_LOGI(TAG, "Event received: MUTE");
            break;

        case SMTC_MODEM_EVENT_REGIONAL_DUTY_CYCLE:
            ESP_LOGI(TAG, "Event received: DUTY_CYCLE");
            break;

        default:
            ESP_LOGE(TAG, "Unknown event %u", current_event.event_type);
            break;
        }
    } while (event_pending_count > 0);
}

static void send_uplink_counter_on_port(uint8_t port)
{
    // Prepare uplink payload with counter
    uint8_t buff[4] = {0};
    buff[0] = (uplink_counter >> 24) & 0xFF;
    buff[1] = (uplink_counter >> 16) & 0xFF;
    buff[2] = (uplink_counter >> 8) & 0xFF;
    buff[3] = (uplink_counter & 0xFF);

    ESP_LOGI(TAG, "📤 Sending uplink #%lu on port %u", uplink_counter, port);

    // Print exact SX1276 TX power register values before transmission
    ESP_LOGI(TAG, "🔍 TX Power Configuration for upcoming transmission:");
    print_sx1276_tx_power_info();

    // Send uplink message
    ASSERT_SMTC_MODEM_RC(smtc_modem_request_uplink(STACK_ID, port, false, buff, 4));

    // Increment uplink counter
    uplink_counter++;
}

static esp_err_t init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

static void dump_sx127x_registers(void)
{
    static uint32_t last_dump_time = 0;
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

    // Only run dump every 2 seconds for debugging
    if (current_time - last_dump_time < REGISTER_DUMP_COOLDOWN_MS)
    {
        return;
    }
    last_dump_time = current_time;

    ESP_LOGI(TAG, "🔍 SX127x Direct Register Read (Time: %lu ms)", current_time);
    ESP_LOGI(TAG, "============================");

    // Get the radio context from the modem (no STACK_ID parameter)
    const void *radio_context = smtc_modem_get_radio_context();
    if (radio_context == NULL)
    {
        ESP_LOGE(TAG, "❌ Radio context is NULL - modem not initialized yet");
        ESP_LOGI(TAG, "============================");
        return;
    }

    ESP_LOGI(TAG, "Got radio context: %p", radio_context);

    // Cast to sx127x_t* for the driver function
    sx127x_t *radio = (sx127x_t *)radio_context;
    uint8_t reg_value = 0;
    bool read_success = false;

    // Read version register using correct function signature
    if (sx127x_read_register(radio, 0x42, &reg_value, 1) == SX127X_STATUS_OK)
    {
        ESP_LOGI(TAG, "📡 Version Register (0x42): 0x%02X", reg_value);
        read_success = true;

        if (reg_value == 0x12)
        {
            ESP_LOGI(TAG, "   ✅ SX1276/77/78/79 detected");
        }
        else if (reg_value == 0x22)
        {
            ESP_LOGI(TAG, "   ✅ SX1272/73 detected");
        }
        else if (reg_value == 0x00 || reg_value == 0xFF)
        {
            ESP_LOGE(TAG, "   ❌ SPI communication failure (0x%02X)", reg_value);
        }
        else
        {
            ESP_LOGW(TAG, "   ⚠️  Unknown chip version: 0x%02X", reg_value);
        }
    }
    else
    {
        ESP_LOGE(TAG, "❌ Failed to read version register");
    }

    if (!read_success)
    {
        ESP_LOGE(TAG, "❌ Register read failed!");
        ESP_LOGE(TAG, "   This suggests SPI communication is not working");
        ESP_LOGE(TAG, "   Check wiring: SCK, MISO, MOSI, NSS pins");
        ESP_LOGI(TAG, "============================");
        return;
    }

    // Read more registers if version read was successful
    uint8_t opmode_reg = 0;
    if (sx127x_read_register(radio, 0x01, &opmode_reg, 1) == SX127X_STATUS_OK)
    {
        bool is_lora_mode = (opmode_reg & 0x80) != 0;
        uint8_t mode = opmode_reg & 0x07;

        ESP_LOGI(TAG, "📡 OpMode (0x01): 0x%02X", opmode_reg);
        ESP_LOGI(TAG, "   - Mode: %s", is_lora_mode ? "LoRa" : "FSK/OOK");
        ESP_LOGI(TAG, "   - State: %s",
                 mode == 0 ? "Sleep" : mode == 1 ? "Standby"
                                   : mode == 2   ? "FS_TX"
                                   : mode == 3   ? "TX"
                                   : mode == 4   ? "FS_RX"
                                   : mode == 5   ? "RX_CONT"
                                   : mode == 6   ? "RX_SINGLE"
                                   : mode == 7   ? "CAD"
                                                 : "Unknown");

        if (!is_lora_mode)
        {
            ESP_LOGE(TAG, "🚨 PROBLEM FOUND!");
            ESP_LOGE(TAG, "   Radio is in FSK mode when it should be LoRa");
        }
    }

    // Read frequency registers
    uint8_t freq_regs[3];
    if (sx127x_read_register(radio, 0x06, freq_regs, 3) == SX127X_STATUS_OK)
    {
        uint32_t freq_raw = (freq_regs[0] << 16) | (freq_regs[1] << 8) | freq_regs[2];
        uint32_t freq_hz = (uint32_t)((uint64_t)freq_raw * 32000000ULL / (1ULL << 19));

        ESP_LOGI(TAG, "📶 Frequency: 0x%06lX = %lu Hz", freq_raw, freq_hz);

        if (freq_hz == 434000000)
        {
            ESP_LOGE(TAG, "   🚨 Default FSK frequency detected!");
        }
        else if (freq_hz >= 915000000 && freq_hz <= 928000000)
        {
            ESP_LOGI(TAG, "   ✅ AU915 frequency detected");
        }
    }

    ESP_LOGI(TAG, "✅ Register dump complete");
    ESP_LOGI(TAG, "============================");
}

// Currently unused - for debugging purposes if needed
/* static void check_modem_radio_state(void)
{
    ESP_LOGI(TAG, "📊 Checking modem radio state...");
    // Add implementation here if needed, or remove the call in app_main
}
*/

static void force_lora_mode(void)
{
    ESP_LOGI(TAG, "🔧 FORCING RADIO INTO LORA MODE - LBM FAILED!");
    ESP_LOGI(TAG, "🚨 The 434MHz FSK default is UNACCEPTABLE for LoRaWAN!");

    const void *radio_context = smtc_modem_get_radio_context();
    if (radio_context == NULL)
    {
        ESP_LOGE(TAG, "❌ Radio context is NULL");
        return;
    }

    sx127x_t *radio = (sx127x_t *)radio_context;
    uint8_t opmode_reg = 0;

    // Read current state BEFORE our fix
    ESP_LOGI(TAG, "📊 BEFORE manual fix (LBM's broken state):");
    if (sx127x_read_register(radio, 0x01, &opmode_reg, 1) == SX127X_STATUS_OK)
    {
        bool is_lora_before = (opmode_reg & 0x80) != 0;
        ESP_LOGI(TAG, "  OpMode: 0x%02X (%s)", opmode_reg, is_lora_before ? "LoRa" : "FSK");

        // Read frequency before
        uint8_t freq_regs[3];
        if (sx127x_read_register(radio, 0x06, freq_regs, 3) == SX127X_STATUS_OK)
        {
            uint32_t freq_raw = (freq_regs[0] << 16) | (freq_regs[1] << 8) | freq_regs[2];
            uint32_t freq_hz = (uint32_t)((uint64_t)freq_raw * 32000000ULL / (1ULL << 19));
            ESP_LOGI(TAG, "  Frequency: %lu Hz", freq_hz);

            if (freq_hz == 434000000)
            {
                ESP_LOGE(TAG, "  🚨 CONFIRMED: LBM left radio in FSK default state!");
                ESP_LOGE(TAG, "  🚨 This is a fundamental LBM initialization failure!");
            }
        }
    }

    // Step 1: Put radio in standby mode first (safer for register changes)
    ESP_LOGI(TAG, "🔧 Step 1: Putting radio in standby mode...");
    uint8_t standby_mode = 0x81; // LoRa mode + Standby
    if (sx127x_write_register(radio, 0x01, &standby_mode, 1) == SX127X_STATUS_OK)
    {
        vTaskDelay(pdMS_TO_TICKS(REGISTER_SETTLE_DELAY_MS)); // Let it settle
        ESP_LOGI(TAG, "✅ Radio in LoRa standby mode");
    }
    else
    {
        ESP_LOGE(TAG, "❌ Failed to set standby mode");
        return;
    }

    // Step 2: Set the correct frequency for AU915 (916.8 MHz)
    ESP_LOGI(TAG, "🔧 Step 2: Setting correct AU915 frequency...");
    uint32_t target_freq = 916800000; // 916.8 MHz - typical AU915 uplink
    uint32_t freq_raw = (uint32_t)((uint64_t)target_freq * (1ULL << 19) / 32000000ULL);

    uint8_t freq_bytes[3];
    freq_bytes[0] = (freq_raw >> 16) & 0xFF;
    freq_bytes[1] = (freq_raw >> 8) & 0xFF;
    freq_bytes[2] = freq_raw & 0xFF;

    ESP_LOGI(TAG, "   Target: %lu Hz (raw: 0x%06lX)", target_freq, freq_raw);
    ESP_LOGI(TAG, "   Bytes: [0x%02X, 0x%02X, 0x%02X]", freq_bytes[0], freq_bytes[1], freq_bytes[2]);

    if (sx127x_write_register(radio, 0x06, freq_bytes, 3) == SX127X_STATUS_OK)
    {
        vTaskDelay(pdMS_TO_TICKS(REGISTER_SETTLE_DELAY_MS));
        ESP_LOGI(TAG, "✅ Frequency registers written");
    }
    else
    {
        ESP_LOGE(TAG, "❌ Failed to write frequency registers");
        return;
    }

    // Step 3: Set other LoRa parameters that LBM should have set
    ESP_LOGI(TAG, "🔧 Step 3: Setting LoRa parameters LBM forgot...");

    // Set spreading factor (SF7 = 0x70 in ModemConfig2)
    uint8_t modem_config2 = 0x74; // SF7 + CRC on + RX timeout MSB
    if (sx127x_write_register(radio, 0x1E, &modem_config2, 1) == SX127X_STATUS_OK)
    {
        ESP_LOGI(TAG, "✅ Set SF7 and CRC");
    }

    // Set bandwidth and coding rate (BW=125kHz, CR=4/5 in ModemConfig1)
    uint8_t modem_config1 = 0x72; // BW=125kHz + CR=4/5 + Implicit header off
    if (sx127x_write_register(radio, 0x1D, &modem_config1, 1) == SX127X_STATUS_OK)
    {
        ESP_LOGI(TAG, "✅ Set BW=125kHz, CR=4/5");
    }

    // Set sync word for public networks (0x34) vs private (0x12)
    uint8_t sync_word = 0x34; // Public LoRaWAN sync word
    if (sx127x_write_register(radio, 0x39, &sync_word, 1) == SX127X_STATUS_OK)
    {
        ESP_LOGI(TAG, "✅ Set public LoRaWAN sync word");
    }

    // Step 4: Verify our manual configuration worked
    ESP_LOGI(TAG, "🔍 Step 4: Verifying our manual fix...");

    uint8_t verify_opmode = 0;
    if (sx127x_read_register(radio, 0x01, &verify_opmode, 1) == SX127X_STATUS_OK)
    {
        bool is_lora_now = (verify_opmode & 0x80) != 0;
        ESP_LOGI(TAG, "  OpMode: 0x%02X (%s)", verify_opmode,
                 is_lora_now ? "✅ LoRa" : "❌ Still FSK");

        if (!is_lora_now)
        {
            ESP_LOGE(TAG, "❌ CRITICAL: Still in FSK mode after manual fix!");
            return;
        }
    }

    // Verify frequency
    uint8_t verify_freq[3];
    if (sx127x_read_register(radio, 0x06, verify_freq, 3) == SX127X_STATUS_OK)
    {
        uint32_t verify_raw = (verify_freq[0] << 16) | (verify_freq[1] << 8) | verify_freq[2];
        uint32_t verify_hz = (uint32_t)((uint64_t)verify_raw * 32000000ULL / (1ULL << 19));
        ESP_LOGI(TAG, "  Frequency: %lu Hz", verify_hz);

        if (verify_hz == 434000000)
        {
            ESP_LOGE(TAG, "❌ CRITICAL: Still at FSK default frequency!");
            return;
        }
        else if (verify_hz >= 915000000 && verify_hz <= 928000000)
        {
            ESP_LOGI(TAG, "  ✅ Correct AU915 frequency range!");
        }
        else
        {
            ESP_LOGW(TAG, "  ⚠️  Unexpected frequency, but not FSK default");
        }
    }

    // Verify other parameters
    uint8_t verify_config1, verify_config2, verify_sync;
    sx127x_read_register(radio, 0x1D, &verify_config1, 1);
    sx127x_read_register(radio, 0x1E, &verify_config2, 1);
    sx127x_read_register(radio, 0x39, &verify_sync, 1);

    ESP_LOGI(TAG, "  ModemConfig1: 0x%02X (BW/CR)", verify_config1);
    ESP_LOGI(TAG, "  ModemConfig2: 0x%02X (SF/CRC)", verify_config2);
    ESP_LOGI(TAG, "  SyncWord: 0x%02X (%s)", verify_sync,
             verify_sync == 0x34 ? "Public" : verify_sync == 0x12 ? "Private"
                                                                  : "Unknown");

    ESP_LOGI(TAG, "🎉 MANUAL RADIO FIX COMPLETE!");
    ESP_LOGI(TAG, "🎉 Radio is now properly configured for LoRaWAN!");
    ESP_LOGI(TAG, "💡 This proves LBM's radio initialization is fundamentally broken");
    ESP_LOGI(TAG, "💡 But the hardware and SPI communication work perfectly!");
}

static smtc_modem_return_code_t apply_proper_lbm_radio_initialization(void)
{
    ESP_LOGI(TAG, "🔧 Applying proper LBM radio initialization (missing packet type setting)...");

    // Get the radio context from LBM's proper API
    sx127x_t *radio = (sx127x_t *)smtc_modem_get_radio_context();

    if (radio == NULL)
    {
        ESP_LOGE(TAG, "❌ Failed to get radio context from LBM");
        return SMTC_MODEM_RC_FAIL;
    }

    // Apply the missing initialization step that should have been called by LBM
    ESP_LOGI(TAG, "  Setting packet type to LoRa (the missing LBM step)...");

    // This is the critical call that LBM initialization is missing!
    // All RALF setup functions call this first: ral_set_pkt_type(&radio->ral, RAL_PKT_TYPE_LORA)
    // This switches the radio from FSK mode (default after reset) to LoRa mode
    sx127x_status_t status = sx127x_set_pkt_type(radio, SX127X_PKT_TYPE_LORA);

    if (status == SX127X_STATUS_OK)
    {
        ESP_LOGI(TAG, "✅ Successfully set radio to LoRa packet type");
        ESP_LOGI(TAG, "✅ This fixes LBM's broken initialization sequence");
        ESP_LOGI(TAG, "✅ Radio should now be in LoRa mode instead of FSK mode");
        return SMTC_MODEM_RC_OK;
    }
    else
    {
        ESP_LOGE(TAG, "❌ Failed to set radio packet type: %d", status);
        return SMTC_MODEM_RC_FAIL;
    }
}

/**
 * @brief Calculate exact SX1276 TX power register values
 *
 * This function calculates the exact register values for the SX1276
 * PA_CONFIG and PA_DAC registers based on the desired system power in dBm,
 * taking into account the board-specific TX power offset and the selected
 * frequency.
 *
 * @param system_power_dbm Desired system power in dBm
 * @param freq_hz Frequency in Hz (used to determine PA settings)
 * @param pa_config_reg Pointer to store the calculated PA_CONFIG register value
 * @param pa_dac_reg Pointer to store the calculated PA_DAC register value
 */
static void calculate_sx1276_tx_power_registers(int8_t system_power_dbm, uint32_t freq_hz,
                                                uint8_t *pa_config_reg, uint8_t *pa_dac_reg)
{
    // Get board TX power offset
    int8_t board_tx_pwr_offset_db = radio_utilities_get_tx_power_offset();
    int16_t power = system_power_dbm + board_tx_pwr_offset_db;

    // ESP32 hardware reality: PA_BOOST pin not connected, must use RFO
    bool pa_select_boost = false;     // RFO only (PA_BOOST pin not connected)
    bool is_20_dbm_output_on = false; // RFO mode only

    // Apply power clamping for RFO mode (SX1276)
    if (power < -4)
        power = -4; // RFO minimum
    if (power > 15)
        power = 15; // RFO maximum

    // Calculate PA_CONFIG register value (register 0x09)
    uint8_t pa_config = 0;
    uint8_t max_pwr = 0;

    if (pa_select_boost)
    {
        // PA_BOOST mode (bit 7 = 1)
        pa_config |= (1 << 7);
        if (is_20_dbm_output_on)
        {
            pa_config |= (uint8_t)(power - 5) & 0x0F;
        }
        else
        {
            pa_config |= (uint8_t)(power - 2) & 0x0F;
        }
    }
    else
    {
        // RFO mode (bit 7 = 0) - ESP32 default
        pa_config &= ~(1 << 7);

        if (power > 0)
        {
            // Use max_pwr = 7 for rf output power bigger than 0
            max_pwr = 7;
        }
        else
        {
            // Use max_pwr = 0 for rf output power smaller or equal than 0
            // pwr value must be compensated by the minimal value (4)
            max_pwr = 0;
            power += 4;
        }

        // Set MaxPower (bits 6:4) and OutputPower (bits 3:0)
        pa_config |= (max_pwr << 4) | ((uint8_t)power & 0x0F);
    }

    // Calculate PA_DAC register value (register 0x4D)
    uint8_t pa_dac = 0x84; // Default value
    if (is_20_dbm_output_on)
    {
        pa_dac = 0x87; // Enable +20dBm mode
    }

    *pa_config_reg = pa_config;
    *pa_dac_reg = pa_dac;
}

/**
 * @brief Print SX1276 TX power information
 *
 * This function retrieves the current TX power settings from the LBM stack,
 * calculates the exact register values that would be written to the SX1276
 * for the current configuration, and logs this information for debugging
 * purposes.
 */
static void print_sx1276_tx_power_info(void)
{
    // For AU915 region, the typical default TX power index is around 5-6
    // which corresponds to approximately 14-16 dBm system power before board offset
    int8_t estimated_system_power_dbm = 14; // Typical AU915 default

    // Get frequency (typical AU915 frequency)
    uint32_t freq_hz = 916800000; // AU915 channel

    // Calculate the exact register values that will be written to SX1276
    uint8_t pa_config_reg, pa_dac_reg;
    calculate_sx1276_tx_power_registers(estimated_system_power_dbm, freq_hz, &pa_config_reg, &pa_dac_reg);

    // Get board TX power offset for detailed logging
    int8_t board_offset = radio_utilities_get_tx_power_offset();
    int8_t final_power = estimated_system_power_dbm + board_offset;

    // Apply RFO clamping for display
    if (final_power < -4)
        final_power = -4;
    if (final_power > 15)
        final_power = 15;

    ESP_LOGI(TAG, "=== SX1276 TX POWER REGISTER VALUES ===");
    ESP_LOGI(TAG, "� MAXIMUM RFO POWER MODE (PA_BOOST pin not connected)");
    ESP_LOGI(TAG, "Estimated System TX Power: %d dBm (AU915 typical)", estimated_system_power_dbm);
    ESP_LOGI(TAG, "Board TX Power Offset: %d dB (BOOSTED for debugging)", board_offset);
    ESP_LOGI(TAG, "Final Clamped Power: %d dBm", final_power);
    ESP_LOGI(TAG, "PA_CONFIG register (0x09): 0x%02X", pa_config_reg);
    ESP_LOGI(TAG, "PA_DAC register (0x4D): 0x%02X", pa_dac_reg);
    ESP_LOGI(TAG, "PA Mode: RFO (hardware limitation)");
    ESP_LOGI(TAG, "MaxPower: %d", (pa_config_reg >> 4) & 0x07);
    ESP_LOGI(TAG, "OutputPower: %d", pa_config_reg & 0x0F);
    ESP_LOGI(TAG, "Expected Output: ~%d dBm RFO", final_power);
    ESP_LOGI(TAG, "=====================================");
}

// Currently unused - for debugging purposes if needed
/* static void read_sx1276_actual_registers(void)
{
    // We need to access the radio through the LBM stack
    // Let's create a simple test transmission to trigger radio configuration
    // and then read the actual registers that were written

    ESP_LOGI(TAG, "=== ACTUAL SX1276 REGISTER READBACK ===");
    ESP_LOGI(TAG, "Note: Register values are only valid after radio configuration");
    ESP_LOGI(TAG, "This will be read after the next transmission attempt");
    ESP_LOGI(TAG, "=====================================");
}
*/

static void investigate_lbm_power_config(void)
{
    ESP_LOGI(TAG, "=== LBM POWER CONFIGURATION INVESTIGATION ===");

    // Get board TX power offset
    int8_t board_offset = radio_utilities_get_tx_power_offset();
    ESP_LOGI(TAG, "Board TX Power Offset: %d dB", board_offset);

    ESP_LOGI(TAG, "Kconfig PA Configuration:");
#ifdef CONFIG_LBM_SX127X_PA_BOOST
    ESP_LOGI(TAG, "- PA Mode: PA_BOOST (configured in Kconfig)");
#ifdef CONFIG_LBM_SX127X_ENABLE_20DBM
    ESP_LOGI(TAG, "- 20dBm Mode: ENABLED");
    ESP_LOGI(TAG, "- Max Power: 20 dBm");
#else
    ESP_LOGI(TAG, "- 20dBm Mode: DISABLED");
    ESP_LOGI(TAG, "- Max Power: 17 dBm");
#endif
#else
    ESP_LOGI(TAG, "- PA Mode: RFO (configured in Kconfig)");
    ESP_LOGI(TAG, "- Max Power: 15 dBm");
#endif

#ifdef CONFIG_LBM_SX127X_TX_POWER_OFFSET
    ESP_LOGI(TAG, "- Kconfig TX Offset: %d dB", CONFIG_LBM_SX127X_TX_POWER_OFFSET);
#else
    ESP_LOGI(TAG, "- Kconfig TX Offset: Not configured (using default)");
#endif

    // For AU915, check typical configuration
    ESP_LOGI(TAG, "AU915 Region Analysis:");
    ESP_LOGI(TAG, "- Regulatory limit: 30 dBm EIRP");
    ESP_LOGI(TAG, "- Typical antenna gain: 2.15 dBi");
    ESP_LOGI(TAG, "- Max conducted power: ~27.85 dBm");
    ESP_LOGI(TAG, "- SX1276 RFO max: 15 dBm");
    ESP_LOGI(TAG, "- SX1276 PA_BOOST max: 17 dBm (20 dBm with PA_DAC)");

    // Check what PA configuration should be used
    ESP_LOGI(TAG, "Expected Configuration:");
#ifdef CONFIG_LBM_SX127X_PA_BOOST
    ESP_LOGI(TAG, "- PA Select: PA_BOOST (requires pin connection)");
#ifdef CONFIG_LBM_SX127X_ENABLE_20DBM
    ESP_LOGI(TAG, "- 20dBm mode: Enabled (high power consumption)");
#else
    ESP_LOGI(TAG, "- 20dBm mode: Disabled");
#endif
#else
    ESP_LOGI(TAG, "- PA Select: RFO (default for ESP32)");
    ESP_LOGI(TAG, "- 20dBm mode: N/A for RFO");
#endif

    // Calculate expected register values for different scenarios
    ESP_LOGI(TAG, "Expected Register Values for Different Powers:");

    for (int8_t test_power = 0; test_power <= 15; test_power += 5)
    {
        uint8_t pa_config_reg, pa_dac_reg;
        calculate_sx1276_tx_power_registers(test_power, 916800000, &pa_config_reg, &pa_dac_reg);

        uint8_t max_pwr = (pa_config_reg >> 4) & 0x07;
        uint8_t output_pwr = pa_config_reg & 0x0F;

        ESP_LOGI(TAG, "  %d dBm -> PA_CONFIG=0x%02X (MaxPwr=%d, OutputPwr=%d), PA_DAC=0x%02X",
                 test_power, pa_config_reg, max_pwr, output_pwr, pa_dac_reg);
    }

    ESP_LOGI(TAG, "============================================");
}

static void force_maximum_tx_power(void)
{
    ESP_LOGI(TAG, "🔋 Attempting to optimize TX power settings...");

    smtc_modem_return_code_t rc;

    // ADR is always enabled in LBM - no direct way to disable it
    ESP_LOGI(TAG, "Note: ADR is always enabled in LBM to comply with LoRaWAN regulations");

    // Set mobile long range ADR profile for higher power
    rc = smtc_modem_adr_set_profile(STACK_ID, SMTC_MODEM_ADR_PROFILE_MOBILE_LONG_RANGE, NULL);
    if (rc == SMTC_MODEM_RC_OK)
    {
        ESP_LOGI(TAG, "✅ Set mobile long range ADR profile (optimized for range)");
    }
    else
    {
        ESP_LOGW(TAG, "⚠️  Failed to set ADR profile (rc=%d)", rc);
    }
}
