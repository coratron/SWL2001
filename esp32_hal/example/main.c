/**
 * @file main.c
 * @brief Example application demonstrating SX127x ESP32 wrapper usage
 * 
 * This example shows how to initialize and use the SX127x radio with ESP32.
 * It demonstrates basic functionality including initialization, register inspection,
 * and interrupt handling.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "sx127x_esp_wrapper.h"

static const char* TAG = "sx127x_example";

// Global radio instance
static sx127x_t g_radio = {0};

/**
 * @brief DIO interrupt callback example
 */
static void dio0_callback(void* context)
{
    ESP_LOGI(TAG, "DIO0 interrupt received");
    // Handle TX/RX done interrupt
}

static void dio1_callback(void* context)
{
    ESP_LOGI(TAG, "DIO1 interrupt received");
    // Handle RX timeout or FHSS change channel
}

static void dio2_callback(void* context)
{
    ESP_LOGI(TAG, "DIO2 interrupt received");
    // Handle FHSS change channel or sync detect
}

/**
 * @brief Initialize the SX127x radio
 */
static esp_err_t init_radio(void)
{
    // Get default configuration
    sx127x_esp_config_t config;
    sx127x_esp_get_default_config(&config);
    
    // Customize configuration for your hardware
    config.spi_frequency = 8000000;  // 8 MHz SPI
    config.use_dma = true;
    
    // GPIO configuration (adjust for your board)
    config.nss_gpio = GPIO_NUM_5;
    config.reset_gpio = GPIO_NUM_14;
    config.dio0_gpio = GPIO_NUM_26;
    config.dio1_gpio = GPIO_NUM_27;
    config.dio2_gpio = GPIO_NUM_33;
    
    // SPI pins (adjust for your board)
    config.miso_gpio = GPIO_NUM_19;
    config.mosi_gpio = GPIO_NUM_23;
    config.sck_gpio = GPIO_NUM_18;
    
    // Interrupt configuration
    config.use_dedicated_task = true;
    config.event_task_priority = 10;
    config.event_task_stack_size = 4096;
    
    ESP_LOGI(TAG, "Initializing SX127x radio...");
    
    // Initialize the radio
    sx127x_esp_err_t err = sx127x_esp_init(&g_radio, &config);
    if (err != SX127X_ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize radio: %s", sx127x_esp_err_to_string(err));
        return ESP_FAIL;
    }
    
    // Set up interrupt callbacks
    g_radio.dio_0_irq_handler = dio0_callback;
    g_radio.dio_1_irq_handler = dio1_callback;
    g_radio.dio_2_irq_handler = dio2_callback;
    
    // Attach interrupt handlers
    sx127x_hal_dio_irq_attach(&g_radio);
    
    ESP_LOGI(TAG, "Radio initialized successfully");
    return ESP_OK;
}

/**
 * @brief Test radio communication
 */
static void test_radio_communication(void)
{
    ESP_LOGI(TAG, "Testing radio communication...");
    
    // Check if radio is responding
    if (!sx127x_esp_is_radio_responding(&g_radio)) {
        ESP_LOGE(TAG, "Radio is not responding!");
        return;
    }
    
    // Get radio version
    uint8_t version;
    sx127x_esp_err_t err = sx127x_esp_get_radio_version(&g_radio, &version);
    if (err == SX127X_ESP_OK) {
        ESP_LOGI(TAG, "Radio version: 0x%02X", version);
        
        // Identify radio type
        if (version == 0x12) {
            ESP_LOGI(TAG, "Detected SX1276 radio");
        } else if (version == 0x22) {
            ESP_LOGI(TAG, "Detected SX1272 radio");
        } else {
            ESP_LOGW(TAG, "Unknown radio version");
        }
    }
    
    // Test reading specific registers
    uint8_t op_mode;
    err = sx127x_esp_read_register_by_name(&g_radio, "RegOpMode", &op_mode);
    if (err == SX127X_ESP_OK) {
        ESP_LOGI(TAG, "Operating mode register: 0x%02X", op_mode);
    }
    
    // Print complete register dump
    ESP_LOGI(TAG, "Printing register dump:");
    sx127x_esp_print_register_dump(&g_radio);
}

/**
 * @brief Monitor radio statistics
 */
static void monitor_statistics(void)
{
    ESP_LOGI(TAG, "Monitoring radio statistics...");
    
    // Get interrupt statistics
    sx127x_esp_interrupt_stats_t int_stats;
    sx127x_esp_err_t err = sx127x_esp_get_interrupt_stats(&g_radio, &int_stats);
    if (err == SX127X_ESP_OK) {
        ESP_LOGI(TAG, "Interrupt Statistics:");
        ESP_LOGI(TAG, "  DIO0 count: %lu", int_stats.dio0_count);
        ESP_LOGI(TAG, "  DIO1 count: %lu", int_stats.dio1_count);
        ESP_LOGI(TAG, "  DIO2 count: %lu", int_stats.dio2_count);
        ESP_LOGI(TAG, "  Missed events: %lu", int_stats.missed_events);
        ESP_LOGI(TAG, "  Queue overflows: %lu", int_stats.queue_overflows);
    }
    
    // Get performance metrics
    sx127x_esp_performance_t perf;
    err = sx127x_esp_get_performance_metrics(&g_radio, &perf);
    if (err == SX127X_ESP_OK) {
        ESP_LOGI(TAG, "Performance Metrics:");
        ESP_LOGI(TAG, "  Avg SPI transaction time: %lu us", perf.spi_transaction_time_us);
        ESP_LOGI(TAG, "  Interrupt latency: %lu us", perf.interrupt_latency_us);
    }
}

/**
 * @brief Example LoRa configuration and basic operations
 */
static void configure_lora_mode(void)
{
    ESP_LOGI(TAG, "Configuring LoRa mode...");
    
    // Put radio in sleep mode first
    sx127x_set_sleep(&g_radio);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Set LoRa mode
    sx127x_set_lora_mode(&g_radio);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Configure basic LoRa parameters
    sx127x_set_rf_freq(&g_radio, 868000000);  // 868 MHz
    sx127x_set_tx_power(&g_radio, 14);        // 14 dBm
    sx127x_set_lora_bandwidth(&g_radio, SX127X_LORA_BW_125_KHZ);
    sx127x_set_lora_spreading_factor(&g_radio, SX127X_LORA_SF7);
    sx127x_set_lora_coding_rate(&g_radio, SX127X_LORA_CR_4_5);
    
    ESP_LOGI(TAG, "LoRa mode configured");
}

/**
 * @brief Main application task
 */
static void radio_task(void* pvParameters)
{
    ESP_LOGI(TAG, "Starting SX127x example application");
    
    // Initialize radio
    if (init_radio() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize radio, stopping task");
        vTaskDelete(NULL);
        return;
    }
    
    // Test basic communication
    test_radio_communication();
    
    // Configure LoRa mode
    configure_lora_mode();
    
    // Main loop
    while (1) {
        // Monitor statistics every 10 seconds
        monitor_statistics();
        
        // Test register read
        uint8_t rssi_value;
        sx127x_esp_err_t err = sx127x_esp_read_register_by_name(&g_radio, "RegRssiValue", &rssi_value);
        if (err == SX127X_ESP_OK) {
            ESP_LOGI(TAG, "Current RSSI value: 0x%02X", rssi_value);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10000));  // Wait 10 seconds
    }
}

/**
 * @brief Application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "SX127x ESP32 Example Application");
    ESP_LOGI(TAG, "Version: %d.%d.%d", 
             LBM_SX127X_VERSION_MAJOR, 
             LBM_SX127X_VERSION_MINOR, 
             LBM_SX127X_VERSION_PATCH);
    
    // Create radio task
    xTaskCreate(radio_task, "radio_task", 8192, NULL, 5, NULL);
}
