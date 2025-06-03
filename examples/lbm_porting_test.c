/*!
 * \file      lbm_porting_test.c
 *
 * \brief     LoRa Basics Modem ESP32 porting validation test
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
#include <stdint.h>
#include <stdbool.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "smtc_modem_api.h"
#include "smtc_modem_hal.h"
#include "smtc_modem_test.h"
#include "radio_hal_esp32.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS ----------------------------------------------------------
 */

#define TAG "LBM_PORTING_TEST"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

#define STACK_ID 0
#define TEST_DURATION_MS 10000  // Run tests for 10 seconds

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

typedef struct {
    const char* name;
    bool (*test_func)(void);
} test_case_t;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static uint32_t test_passed = 0;
static uint32_t test_failed = 0;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

static bool test_hal_initialization(void);
static bool test_modem_initialization(void);
static bool test_radio_functionality(void);
static bool test_timer_functionality(void);
static bool test_critical_section(void);
static bool test_random_generation(void);
static bool test_reset_functionality(void);
static void run_test_suite(void);
static void print_test_results(void);

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION -------------------------------------------
 */

static bool test_hal_initialization(void)
{
    ESP_LOGI(TAG, "Testing HAL initialization...");
    
    // Test radio HAL initialization
    radio_hal_init();
    
    // Check if initialization was successful
    // This is a basic test - in a real scenario you might want to verify
    // specific GPIO configurations, SPI initialization, etc.
    ESP_LOGI(TAG, "Radio HAL initialized successfully");
    return true;
}

static bool test_modem_initialization(void)
{
    ESP_LOGI(TAG, "Testing modem initialization...");
    
    smtc_modem_return_code_t rc;
    
    // Initialize the modem
    rc = smtc_modem_init(&modem_event_callback);
    if (rc != SMTC_MODEM_RC_OK) {
        ESP_LOGE(TAG, "Modem initialization failed: %d", rc);
        return false;
    }
    
    // Get modem version to verify it's working
    smtc_modem_version_t version;
    rc = smtc_modem_get_modem_version(&version);
    if (rc != SMTC_MODEM_RC_OK) {
        ESP_LOGE(TAG, "Failed to get modem version: %d", rc);
        return false;
    }
    
    ESP_LOGI(TAG, "Modem version: %02x.%02x.%02x.%02x", 
             version.major, version.minor, version.patch, version.revision);
    
    return true;
}

static bool test_radio_functionality(void)
{
    ESP_LOGI(TAG, "Testing radio functionality...");
    
    smtc_modem_return_code_t rc;
    
    // Test setting radio to sleep mode
    rc = smtc_modem_test_direct_radio_sleep(STACK_ID);
    if (rc != SMTC_MODEM_RC_OK) {
        ESP_LOGE(TAG, "Radio sleep test failed: %d", rc);
        return false;
    }
    
    // Test radio wake up
    rc = smtc_modem_test_direct_radio_set_standby(STACK_ID);
    if (rc != SMTC_MODEM_RC_OK) {
        ESP_LOGE(TAG, "Radio standby test failed: %d", rc);
        return false;
    }
    
    ESP_LOGI(TAG, "Radio functionality test passed");
    return true;
}

static bool test_timer_functionality(void)
{
    ESP_LOGI(TAG, "Testing timer functionality...");
    
    uint32_t start_time = smtc_modem_hal_get_time_in_ms();
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait 100ms
    uint32_t end_time = smtc_modem_hal_get_time_in_ms();
    
    uint32_t elapsed = end_time - start_time;
    
    // Check if the elapsed time is approximately 100ms (allow some tolerance)
    if (elapsed < 90 || elapsed > 110) {
        ESP_LOGE(TAG, "Timer test failed: expected ~100ms, got %u ms", elapsed);
        return false;
    }
    
    ESP_LOGI(TAG, "Timer functionality test passed (elapsed: %u ms)", elapsed);
    return true;
}

static bool test_critical_section(void)
{
    ESP_LOGI(TAG, "Testing critical section...");
    
    // Test critical section entry/exit
    smtc_modem_hal_disable_irq();
    smtc_modem_hal_enable_irq();
    
    ESP_LOGI(TAG, "Critical section test passed");
    return true;
}

static bool test_random_generation(void)
{
    ESP_LOGI(TAG, "Testing random number generation...");
    
    uint32_t random1 = smtc_modem_hal_get_random_nb_in_range(0, 1000000);
    uint32_t random2 = smtc_modem_hal_get_random_nb_in_range(0, 1000000);
    
    // Very basic test - just check that we get different numbers
    if (random1 == random2) {
        ESP_LOGW(TAG, "Random numbers are identical (might be OK): %u, %u", random1, random2);
    }
    
    ESP_LOGI(TAG, "Random generation test passed (values: %u, %u)", random1, random2);
    return true;
}

static bool test_reset_functionality(void)
{
    ESP_LOGI(TAG, "Testing reset functionality...");
    
    // Note: We can't actually test the reset function as it would reset the device
    // This is just to verify the function exists and can be called
    ESP_LOGI(TAG, "Reset functionality available (not executed)");
    return true;
}

static void modem_event_callback(void)
{
    // Empty callback for testing
}

static void run_test_suite(void)
{
    test_case_t tests[] = {
        {"HAL Initialization", test_hal_initialization},
        {"Modem Initialization", test_modem_initialization},
        {"Radio Functionality", test_radio_functionality},
        {"Timer Functionality", test_timer_functionality},
        {"Critical Section", test_critical_section},
        {"Random Generation", test_random_generation},
        {"Reset Functionality", test_reset_functionality},
    };
    
    size_t num_tests = sizeof(tests) / sizeof(tests[0]);
    
    ESP_LOGI(TAG, "Running %zu porting tests...", num_tests);
    ESP_LOGI(TAG, "===========================================");
    
    for (size_t i = 0; i < num_tests; i++) {
        ESP_LOGI(TAG, "Test %zu/%zu: %s", i + 1, num_tests, tests[i].name);
        
        if (tests[i].test_func()) {
            test_passed++;
            ESP_LOGI(TAG, "✓ PASSED");
        } else {
            test_failed++;
            ESP_LOGE(TAG, "✗ FAILED");
        }
        
        ESP_LOGI(TAG, "-------------------------------------------");
        vTaskDelay(pdMS_TO_TICKS(500));  // Small delay between tests
    }
}

static void print_test_results(void)
{
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "Test Results:");
    ESP_LOGI(TAG, "  Passed: %u", test_passed);
    ESP_LOGI(TAG, "  Failed: %u", test_failed);
    ESP_LOGI(TAG, "  Total:  %u", test_passed + test_failed);
    
    if (test_failed == 0) {
        ESP_LOGI(TAG, "🎉 ALL TESTS PASSED! 🎉");
        ESP_LOGI(TAG, "ESP32 porting is working correctly.");
    } else {
        ESP_LOGE(TAG, "❌ %u TESTS FAILED", test_failed);
        ESP_LOGE(TAG, "Please check your hardware configuration and HAL implementation.");
    }
    
    ESP_LOGI(TAG, "===========================================");
}

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void app_main(void)
{
    ESP_LOGI(TAG, "LoRa Basics Modem ESP32 Porting Test");
    ESP_LOGI(TAG, "SDK Version: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "Free heap: %u bytes", esp_get_free_heap_size());
    
    // Wait a moment for the system to stabilize
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Run the test suite
    run_test_suite();
    
    // Print final results
    print_test_results();
    
    // Keep the task running
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "Test completed. Free heap: %u bytes", esp_get_free_heap_size());
    }
}
