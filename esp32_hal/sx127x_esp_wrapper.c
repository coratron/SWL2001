/**
 * @file sx127x_esp_wrapper.c
 * @brief ESP32 wrapper implementation for SX127x radio driver
 * 
 * This file implements the main wrapper functions for ESP32 integration,
 * including initialization, configuration, and utility functions.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "sx127x.h"
#include "sx127x_hal.h"
#include "sx127x_esp_wrapper.h"
#include "sx127x_esp_internal.h"

static const char* TAG = "sx127x_esp_wrapper";

// Static context storage for single radio instance
static sx127x_esp_context_t s_radio_context = {0};

/**
 * @brief Get ESP32 context from radio structure
 */
sx127x_esp_context_t* sx127x_esp_get_context(const sx127x_t* radio)
{
    if (!radio || radio->context != &s_radio_context) {
        return NULL;
    }
    return &s_radio_context;
}

/**
 * @brief Get default ESP32 configuration
 */
void sx127x_esp_get_default_config(sx127x_esp_config_t* config)
{
    if (!config) {
        return;
    }

    memset(config, 0, sizeof(sx127x_esp_config_t));

    // SPI configuration
    config->spi_host = CONFIG_LBM_SX127X_SPI_HOST;
    config->spi_frequency = CONFIG_LBM_SX127X_SPI_FREQUENCY;
    config->use_dma = CONFIG_LBM_SX127X_USE_DMA;

    // GPIO pins
    config->miso_gpio = CONFIG_LBM_SX127X_MISO_GPIO;
    config->mosi_gpio = CONFIG_LBM_SX127X_MOSI_GPIO;
    config->sck_gpio = CONFIG_LBM_SX127X_SCK_GPIO;
    config->nss_gpio = CONFIG_LBM_SX127X_NSS_GPIO;
    config->reset_gpio = CONFIG_LBM_SX127X_RESET_GPIO;
    config->dio0_gpio = CONFIG_LBM_SX127X_DIO0_GPIO;
    config->dio1_gpio = CONFIG_LBM_SX127X_DIO1_GPIO;
    config->dio2_gpio = CONFIG_LBM_SX127X_DIO2_GPIO;

    // Interrupt configuration
    config->interrupt_priority = CONFIG_LBM_SX127X_INTERRUPT_PRIORITY;
    config->use_dedicated_task = CONFIG_LBM_SX127X_USE_DEDICATED_TASK;
    config->event_task_priority = CONFIG_LBM_SX127X_EVENT_TASK_PRIORITY;
    config->event_task_stack_size = CONFIG_LBM_SX127X_EVENT_TASK_STACK_SIZE;
    config->event_queue_size = CONFIG_LBM_SX127X_EVENT_QUEUE_SIZE;
}

/**
 * @brief Validate configuration parameters
 */
sx127x_esp_err_t sx127x_esp_validate_config(const sx127x_esp_config_t* config)
{
    if (!config) {
        return SX127X_ESP_ERR_INVALID_ARG;
    }

    // Validate SPI host
    if (config->spi_host < SPI1_HOST || config->spi_host > SPI3_HOST) {
        ESP_LOGE(TAG, "Invalid SPI host: %d", config->spi_host);
        return SX127X_ESP_ERR_INVALID_ARG;
    }

    // Validate SPI frequency
    if (config->spi_frequency < 1000000 || config->spi_frequency > 20000000) {
        ESP_LOGE(TAG, "Invalid SPI frequency: %lu", config->spi_frequency);
        return SX127X_ESP_ERR_INVALID_ARG;
    }

    // Validate GPIO pins
    if (config->miso_gpio >= GPIO_NUM_MAX || config->mosi_gpio >= GPIO_NUM_MAX ||
        config->sck_gpio >= GPIO_NUM_MAX || config->nss_gpio >= GPIO_NUM_MAX ||
        config->reset_gpio >= GPIO_NUM_MAX || config->dio0_gpio >= GPIO_NUM_MAX ||
        config->dio1_gpio >= GPIO_NUM_MAX || config->dio2_gpio >= GPIO_NUM_MAX) {
        ESP_LOGE(TAG, "Invalid GPIO pin configuration");
        return SX127X_ESP_ERR_INVALID_ARG;
    }

    return SX127X_ESP_OK;
}

/**
 * @brief Initialize SPI bus and device
 */
sx127x_esp_err_t sx127x_esp_spi_init(sx127x_esp_context_t* ctx)
{
    esp_err_t ret;

    // Configure SPI bus
    ctx->spi_bus_config.miso_io_num = ctx->config.miso_gpio;
    ctx->spi_bus_config.mosi_io_num = ctx->config.mosi_gpio;
    ctx->spi_bus_config.sclk_io_num = ctx->config.sck_gpio;
    ctx->spi_bus_config.quadwp_io_num = -1;
    ctx->spi_bus_config.quadhd_io_num = -1;
    ctx->spi_bus_config.max_transfer_sz = CONFIG_LBM_SX127X_SPI_BUFFER_SIZE;

    // Initialize SPI bus
    ret = spi_bus_initialize(ctx->config.spi_host, &ctx->spi_bus_config, 
                            ctx->config.use_dma ? SPI_DMA_CH_AUTO : SPI_DMA_DISABLED);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return SX127X_ESP_ERR_SPI_INIT;
    }

    // Configure SPI device
    ctx->spi_device_config.clock_speed_hz = ctx->config.spi_frequency;
    ctx->spi_device_config.mode = 0;  // SPI mode 0 (CPOL=0, CPHA=0)
    ctx->spi_device_config.spics_io_num = ctx->config.nss_gpio;
    ctx->spi_device_config.queue_size = 7;
    ctx->spi_device_config.flags = 0;

    // Add SPI device
    ret = spi_bus_add_device(ctx->config.spi_host, &ctx->spi_device_config, &ctx->spi_device);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        spi_bus_free(ctx->config.spi_host);
        return SX127X_ESP_ERR_SPI_INIT;
    }

    ESP_LOGI(TAG, "SPI initialized: Host=%d, Freq=%lu Hz, DMA=%s", 
             ctx->config.spi_host, ctx->config.spi_frequency,
             ctx->config.use_dma ? "enabled" : "disabled");

    return SX127X_ESP_OK;
}

/**
 * @brief Initialize GPIO pins
 */
sx127x_esp_err_t sx127x_esp_gpio_init(sx127x_esp_context_t* ctx)
{
    esp_err_t ret;

    // Store GPIO pins
    ctx->dio_pins[0] = ctx->config.dio0_gpio;
    ctx->dio_pins[1] = ctx->config.dio1_gpio;
    ctx->dio_pins[2] = ctx->config.dio2_gpio;
    ctx->nss_gpio = ctx->config.nss_gpio;
    ctx->reset_gpio = ctx->config.reset_gpio;

    // Configure reset pin as output
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << ctx->reset_gpio),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure reset GPIO: %s", esp_err_to_name(ret));
        return SX127X_ESP_ERR_GPIO_INIT;
    }

    // Set reset pin to inactive state
    gpio_set_level(ctx->reset_gpio, 1);

    ESP_LOGI(TAG, "GPIO initialized: Reset=%d, DIO0=%d, DIO1=%d, DIO2=%d", 
             ctx->reset_gpio, ctx->dio_pins[0], ctx->dio_pins[1], ctx->dio_pins[2]);

    return SX127X_ESP_OK;
}

/**
 * @brief Initialize interrupt handling
 */
sx127x_esp_err_t sx127x_esp_interrupt_init(sx127x_esp_context_t* ctx)
{
    if (!ctx->config.use_dedicated_task) {
        ESP_LOGI(TAG, "Dedicated task disabled, interrupts will be processed in ISR");
        return SX127X_ESP_OK;
    }

    // Create event queue
    ctx->event_queue = xQueueCreate(ctx->config.event_queue_size, sizeof(sx127x_esp_dio_event_t));
    if (!ctx->event_queue) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return SX127X_ESP_ERR_INTERRUPT_INIT;
    }

    // Create event processing task
    BaseType_t ret = xTaskCreate(sx127x_esp_event_task, 
                                "sx127x_events",
                                ctx->config.event_task_stack_size,
                                ctx,
                                ctx->config.event_task_priority,
                                &ctx->event_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create event task");
        vQueueDelete(ctx->event_queue);
        ctx->event_queue = NULL;
        return SX127X_ESP_ERR_INTERRUPT_INIT;
    }

    ESP_LOGI(TAG, "Interrupt handling initialized with dedicated task");
    return SX127X_ESP_OK;
}

/**
 * @brief Initialize SX127x with ESP32-specific configuration
 */
sx127x_esp_err_t sx127x_esp_init(sx127x_t* radio, const sx127x_esp_config_t* config)
{
    if (!radio || !config) {
        return SX127X_ESP_ERR_INVALID_ARG;
    }

    // Validate configuration
    sx127x_esp_err_t err = sx127x_esp_validate_config(config);
    if (err != SX127X_ESP_OK) {
        return err;
    }

    // Initialize context
    memset(&s_radio_context, 0, sizeof(s_radio_context));
    s_radio_context.radio = radio;
    s_radio_context.config = *config;

    // Create mutexes
    s_radio_context.spi_mutex = xSemaphoreCreateMutex();
    s_radio_context.state_mutex = xSemaphoreCreateMutex();
    
    if (!s_radio_context.spi_mutex || !s_radio_context.state_mutex) {
        ESP_LOGE(TAG, "Failed to create mutexes");
        if (s_radio_context.spi_mutex) vSemaphoreDelete(s_radio_context.spi_mutex);
        if (s_radio_context.state_mutex) vSemaphoreDelete(s_radio_context.state_mutex);
        return SX127X_ESP_ERR_NO_MEM;
    }

    // Initialize SPI
    err = sx127x_esp_spi_init(&s_radio_context);
    if (err != SX127X_ESP_OK) {
        goto cleanup;
    }

    // Initialize GPIO
    err = sx127x_esp_gpio_init(&s_radio_context);
    if (err != SX127X_ESP_OK) {
        goto cleanup;
    }

    // Initialize interrupt handling
    err = sx127x_esp_interrupt_init(&s_radio_context);
    if (err != SX127X_ESP_OK) {
        goto cleanup;
    }

    // Set radio context
    radio->context = &s_radio_context;
    s_radio_context.initialized = true;

    // Initialize the radio driver
    sx127x_status_t radio_status = sx127x_init(radio);
    if (radio_status != SX127X_STATUS_OK) {
        ESP_LOGE(TAG, "Failed to initialize SX127x radio: %d", radio_status);
        err = SX127X_ESP_ERR_RADIO_NOT_FOUND;
        goto cleanup;
    }

    ESP_LOGI(TAG, "SX127x ESP32 wrapper initialized successfully");
    return SX127X_ESP_OK;

cleanup:
    sx127x_esp_deinit(radio);
    return err;
}

/**
 * @brief Deinitialize SX127x and free resources
 */
sx127x_esp_err_t sx127x_esp_deinit(sx127x_t* radio)
{
    sx127x_esp_context_t* ctx = sx127x_esp_get_context(radio);
    if (!ctx) {
        return SX127X_ESP_ERR_NOT_INITIALIZED;
    }

    // Stop event task if running
    if (ctx->event_task) {
        vTaskDelete(ctx->event_task);
        ctx->event_task = NULL;
    }

    // Delete event queue
    if (ctx->event_queue) {
        vQueueDelete(ctx->event_queue);
        ctx->event_queue = NULL;
    }

    // Remove SPI device
    if (ctx->spi_device) {
        spi_bus_remove_device(ctx->spi_device);
        ctx->spi_device = NULL;
    }

    // Free SPI bus
    spi_bus_free(ctx->config.spi_host);

    // Delete mutexes
    if (ctx->spi_mutex) {
        vSemaphoreDelete(ctx->spi_mutex);
        ctx->spi_mutex = NULL;
    }
    if (ctx->state_mutex) {
        vSemaphoreDelete(ctx->state_mutex);
        ctx->state_mutex = NULL;
    }

    // Clear context
    ctx->initialized = false;
    radio->context = NULL;

    ESP_LOGI(TAG, "SX127x ESP32 wrapper deinitialized");
    return SX127X_ESP_OK;
}

/**
 * @brief Convert ESP32 error code to string
 */
const char* sx127x_esp_err_to_string(sx127x_esp_err_t err)
{
    switch (err) {
        case SX127X_ESP_OK: return "Success";
        case SX127X_ESP_ERR_INVALID_ARG: return "Invalid argument";
        case SX127X_ESP_ERR_NO_MEM: return "Out of memory";
        case SX127X_ESP_ERR_SPI_INIT: return "SPI initialization failed";
        case SX127X_ESP_ERR_GPIO_INIT: return "GPIO initialization failed";
        case SX127X_ESP_ERR_INTERRUPT_INIT: return "Interrupt initialization failed";
        case SX127X_ESP_ERR_RADIO_NOT_FOUND: return "Radio not responding";
        case SX127X_ESP_ERR_TIMEOUT: return "Operation timeout";
        case SX127X_ESP_ERR_NOT_INITIALIZED: return "Component not initialized";
        case SX127X_ESP_ERR_RADIO_BUSY: return "Radio is busy";
        default: return "Unknown error";
    }
}

/**
 * @brief Event processing task
 */
void sx127x_esp_event_task(void* pvParameters)
{
    sx127x_esp_context_t* ctx = (sx127x_esp_context_t*)pvParameters;
    sx127x_esp_dio_event_t event;

    ESP_LOGI(TAG, "Event processing task started");

    while (1) {
        if (xQueueReceive(ctx->event_queue, &event, portMAX_DELAY)) {
#ifdef CONFIG_LBM_SX127X_DEBUG_INTERRUPTS
            ESP_LOGD(TAG, "Processing DIO%d interrupt at %llu us", event.dio_num, event.timestamp);
#endif

#ifdef CONFIG_LBM_SX127X_INTERRUPT_STATS
            sx127x_esp_update_interrupt_stats(ctx, event.dio_num, event.timestamp);
#endif

            // Call the appropriate callback
            if (event.dio_num < 3 && ctx->dio_callbacks[event.dio_num]) {
                ctx->dio_callbacks[event.dio_num](ctx->radio);
            }
        }
    }
}

/**
 * @brief DIO interrupt service routines
 */
void IRAM_ATTR sx127x_esp_dio0_isr(void* arg)
{
    sx127x_esp_context_t* ctx = (sx127x_esp_context_t*)arg;
    sx127x_esp_dio_event_t event = {
        .dio_num = 0,
        .timestamp = esp_timer_get_time()
    };
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if (ctx->event_queue) {
        xQueueSendFromISR(ctx->event_queue, &event, &xHigherPriorityTaskWoken);
    } else if (ctx->dio_callbacks[0]) {
        // Direct callback if no task
        ctx->dio_callbacks[0](ctx->radio);
    }
    
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR sx127x_esp_dio1_isr(void* arg)
{
    sx127x_esp_context_t* ctx = (sx127x_esp_context_t*)arg;
    sx127x_esp_dio_event_t event = {
        .dio_num = 1,
        .timestamp = esp_timer_get_time()
    };
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if (ctx->event_queue) {
        xQueueSendFromISR(ctx->event_queue, &event, &xHigherPriorityTaskWoken);
    } else if (ctx->dio_callbacks[1]) {
        // Direct callback if no task
        ctx->dio_callbacks[1](ctx->radio);
    }
    
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR sx127x_esp_dio2_isr(void* arg)
{
    sx127x_esp_context_t* ctx = (sx127x_esp_context_t*)arg;
    sx127x_esp_dio_event_t event = {
        .dio_num = 2,
        .timestamp = esp_timer_get_time()
    };
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if (ctx->event_queue) {
        xQueueSendFromISR(ctx->event_queue, &event, &xHigherPriorityTaskWoken);
    } else if (ctx->dio_callbacks[2]) {
        // Direct callback if no task
        ctx->dio_callbacks[2](ctx->radio);
    }
    
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

#ifdef CONFIG_LBM_SX127X_INTERRUPT_STATS
/**
 * @brief Update interrupt statistics
 */
void sx127x_esp_update_interrupt_stats(sx127x_esp_context_t* ctx, uint8_t dio_num, uint64_t timestamp)
{
    if (dio_num >= 3) return;

    switch (dio_num) {
        case 0:
            ctx->stats.dio0_count++;
            ctx->stats.last_dio0_time = timestamp;
            break;
        case 1:
            ctx->stats.dio1_count++;
            ctx->stats.last_dio1_time = timestamp;
            break;
        case 2:
            ctx->stats.dio2_count++;
            ctx->stats.last_dio2_time = timestamp;
            break;
    }
}
#endif
