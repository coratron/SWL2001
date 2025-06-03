/*!
 * \file      lbm_example.c
 *
 * \brief     LoRa Basics Modem ESP32 example application
 *
 * \copyright Copyright (c) 2023
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "smtc_modem_api.h"
#include "smtc_modem_utilities.h"
#include "radio_hal_esp32.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS ----------------------------------------------------------
 */

#define TAG "LBM_EXAMPLE"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

// LoRaWAN configuration - Update these with your values!
static const uint8_t user_dev_eui[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t user_join_eui[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t user_app_key[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define STACK_ID 0
#define LORAWAN_APP_PORT 2
#define UPLINK_PERIOD_MS 60000  // Send uplink every 60 seconds

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

typedef enum {
    APP_EVENT_JOINED = BIT0,
    APP_EVENT_JOIN_FAIL = BIT1,
    APP_EVENT_TX_DONE = BIT2,
    APP_EVENT_DOWNLINK = BIT3,
} app_event_bits_t;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static EventGroupHandle_t app_event_group;
static uint32_t uplink_counter = 0;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

static void modem_event_callback(void);
static void process_modem_events(void);
static void send_uplink_message(void);
static void app_task(void* pvParameters);

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void app_main(void)
{
    ESP_LOGI(TAG, "LoRa Basics Modem ESP32 Example");
    ESP_LOGI(TAG, "SDK Version: %s", esp_get_idf_version());

    // Create event group for application events
    app_event_group = xEventGroupCreate();
    if (app_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return;
    }

    // Initialize radio HAL
    radio_hal_init();

    // Initialize the modem
    smtc_modem_init(modem_event_callback);

    // Get modem version
    smtc_modem_version_t modem_version;
    if (smtc_modem_get_modem_version(&modem_version) == SMTC_MODEM_RC_OK) {
        ESP_LOGI(TAG, "Modem version: %d.%d.%d", 
                 modem_version.major, modem_version.minor, modem_version.patch);
    }

    // Configure LoRaWAN parameters
    smtc_modem_return_code_t rc;

    // Set DevEUI
    rc = smtc_modem_set_deveui(STACK_ID, user_dev_eui);
    if (rc != SMTC_MODEM_RC_OK) {
        ESP_LOGE(TAG, "Failed to set DevEUI: %d", rc);
        return;
    }

    // Set JoinEUI
    rc = smtc_modem_set_joineui(STACK_ID, user_join_eui);
    if (rc != SMTC_MODEM_RC_OK) {
        ESP_LOGE(TAG, "Failed to set JoinEUI: %d", rc);
        return;
    }

    // Set AppKey
    rc = smtc_modem_set_appkey(STACK_ID, user_app_key);
    if (rc != SMTC_MODEM_RC_OK) {
        ESP_LOGE(TAG, "Failed to set AppKey: %d", rc);
        return;
    }

    // Set region (adjust as needed)
    rc = smtc_modem_set_region(STACK_ID, SMTC_MODEM_REGION_EU_868);
    if (rc != SMTC_MODEM_RC_OK) {
        ESP_LOGE(TAG, "Failed to set region: %d", rc);
        return;
    }

    // Set class (Class A by default)
    rc = smtc_modem_set_class(STACK_ID, SMTC_MODEM_CLASS_A);
    if (rc != SMTC_MODEM_RC_OK) {
        ESP_LOGE(TAG, "Failed to set class: %d", rc);
        return;
    }

    ESP_LOGI(TAG, "LoRaWAN configuration completed");

    // Create application task
    if (xTaskCreate(app_task, "app_task", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create application task");
        return;
    }

    ESP_LOGI(TAG, "Application started");
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static void modem_event_callback(void)
{
    // This callback is called from the modem when an event occurs
    // We'll process events in the main task to avoid blocking the modem
}

static void process_modem_events(void)
{
    smtc_modem_event_t event;
    smtc_modem_return_code_t rc;

    while ((rc = smtc_modem_get_event(&event, STACK_ID)) == SMTC_MODEM_RC_OK) {
        switch (event.event_type) {
            case SMTC_MODEM_EVENT_RESET:
                ESP_LOGI(TAG, "Modem reset event");
                break;

            case SMTC_MODEM_EVENT_ALARM:
                ESP_LOGI(TAG, "Alarm event");
                break;

            case SMTC_MODEM_EVENT_JOINED:
                ESP_LOGI(TAG, "Network joined!");
                xEventGroupSetBits(app_event_group, APP_EVENT_JOINED);
                break;

            case SMTC_MODEM_EVENT_JOINFAIL:
                ESP_LOGW(TAG, "Join failed");
                xEventGroupSetBits(app_event_group, APP_EVENT_JOIN_FAIL);
                break;

            case SMTC_MODEM_EVENT_TXDONE:
                ESP_LOGI(TAG, "TX done - Status: %d", event.event_data.txdone.status);
                xEventGroupSetBits(app_event_group, APP_EVENT_TX_DONE);
                break;

            case SMTC_MODEM_EVENT_DOWNDATA:
                ESP_LOGI(TAG, "Downlink received - Port: %d, Size: %d", 
                         event.event_data.downdata.fport, 
                         event.event_data.downdata.length);
                
                // Print downlink data
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, event.event_data.downdata.data, 
                                       event.event_data.downdata.length, ESP_LOG_INFO);
                
                xEventGroupSetBits(app_event_group, APP_EVENT_DOWNLINK);
                break;

            case SMTC_MODEM_EVENT_LINK_CHECK:
                ESP_LOGI(TAG, "Link check - Status: %d", event.event_data.link_check.status);
                break;

            case SMTC_MODEM_EVENT_CLASS_B_STATUS:
                ESP_LOGI(TAG, "Class B status: %d", event.event_data.class_b_status.status);
                break;

            default:
                ESP_LOGD(TAG, "Unhandled event: %d", event.event_type);
                break;
        }
    }
}

static void send_uplink_message(void)
{
    uint8_t payload[10];
    
    // Create a simple payload with counter and some sensor data
    payload[0] = (uplink_counter >> 24) & 0xFF;
    payload[1] = (uplink_counter >> 16) & 0xFF;
    payload[2] = (uplink_counter >> 8) & 0xFF;
    payload[3] = uplink_counter & 0xFF;
    
    // Add some dummy sensor data
    payload[4] = 0x01; // Sensor type: temperature
    payload[5] = 0x67; // Temperature: 25.5°C (255 = 25.5)
    payload[6] = 0x02; // Sensor type: humidity
    payload[7] = 0x32; // Humidity: 50%
    payload[8] = 0x03; // Sensor type: battery
    payload[9] = 0xFE; // Battery: 99%

    smtc_modem_return_code_t rc = smtc_modem_request_uplink(
        STACK_ID, LORAWAN_APP_PORT, false, payload, sizeof(payload)
    );

    if (rc == SMTC_MODEM_RC_OK) {
        ESP_LOGI(TAG, "Uplink #%lu queued", uplink_counter);
        uplink_counter++;
    } else {
        ESP_LOGE(TAG, "Failed to queue uplink: %d", rc);
    }
}

static void app_task(void* pvParameters)
{
    TickType_t last_uplink_time = 0;
    EventBits_t event_bits;
    bool is_joined = false;

    ESP_LOGI(TAG, "Starting join procedure...");
    
    // Start join procedure
    smtc_modem_return_code_t rc = smtc_modem_join_network(STACK_ID);
    if (rc != SMTC_MODEM_RC_OK) {
        ESP_LOGE(TAG, "Failed to start join: %d", rc);
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        // Run the modem engine
        uint32_t sleep_time_ms = smtc_modem_run_engine();
        
        // Process any pending events
        process_modem_events();

        // Check for application events
        event_bits = xEventGroupWaitBits(
            app_event_group,
            APP_EVENT_JOINED | APP_EVENT_JOIN_FAIL | APP_EVENT_TX_DONE | APP_EVENT_DOWNLINK,
            pdTRUE,  // Clear bits on exit
            pdFALSE, // Don't wait for all bits
            0        // Don't block
        );

        if (event_bits & APP_EVENT_JOINED) {
            is_joined = true;
            last_uplink_time = xTaskGetTickCount();
            ESP_LOGI(TAG, "Device joined network, starting periodic uplinks");
        }

        if (event_bits & APP_EVENT_JOIN_FAIL) {
            ESP_LOGW(TAG, "Join failed, retrying in 30 seconds...");
            vTaskDelay(pdMS_TO_TICKS(30000));
            
            rc = smtc_modem_join_network(STACK_ID);
            if (rc != SMTC_MODEM_RC_OK) {
                ESP_LOGE(TAG, "Failed to restart join: %d", rc);
            }
        }

        if (event_bits & APP_EVENT_TX_DONE) {
            ESP_LOGI(TAG, "Transmission completed");
        }

        if (event_bits & APP_EVENT_DOWNLINK) {
            ESP_LOGI(TAG, "Downlink processed");
        }

        // Send periodic uplinks if joined
        if (is_joined) {
            TickType_t current_time = xTaskGetTickCount();
            if ((current_time - last_uplink_time) >= pdMS_TO_TICKS(UPLINK_PERIOD_MS)) {
                send_uplink_message();
                last_uplink_time = current_time;
            }
        }

        // Sleep for the time suggested by the modem engine
        uint32_t delay_ms = (sleep_time_ms > 100) ? 100 : sleep_time_ms;
        if (delay_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        } else {
            vTaskDelay(1); // Yield to other tasks
        }
    }
}
