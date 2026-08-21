/**
 * @file sx127x_hal_esp.c
 * @brief ESP32 Hardware Abstraction Layer for SX127x radio driver
 *
 * This file implements the SX127x HAL functions for ESP32, providing
 * SPI communication, GPIO control, and interrupt handling.
 */

#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "sx127x.h"
#include "sx127x_hal.h"
#include "sx127x_esp_wrapper.h"
#include "sx127x_esp_internal.h"
#include "lora_hal_lbm_audit.h"

static const char *TAG = "sx127x_hal_esp";

// Global ISR service installation flag
static bool s_isr_service_installed = false;

// Forward declaration
void sx127x_hal_gpio_reset(gpio_num_t reset_gpio);

/**
 * @brief Get radio ID based on compile-time configuration
 */
sx127x_radio_id_t sx127x_hal_get_radio_id(const sx127x_t *radio)
{
#if defined(CONFIG_LBM_SX127X_RADIO_SX1272) || defined(SX1272)
    return SX127X_RADIO_ID_SX1272;
#elif defined(CONFIG_LBM_SX127X_RADIO_SX1276) || defined(SX1276)
    return SX127X_RADIO_ID_SX1276;
#else
#error "Please define the radio type in Kconfig"
#endif
}

/**
 * @brief Install GPIO ISR service if not already installed
 */
static esp_err_t install_gpio_isr_service_safe(void)
{
    // Check if we've already handled ISR service installation
    if (s_isr_service_installed)
    {
        ESP_LOGD(TAG, "GPIO ISR service already confirmed available");
        return ESP_OK;
    }

    // Try to install ISR service - this will fail if already installed
    esp_err_t ret = gpio_install_isr_service(CONFIG_LBM_SX127X_INTERRUPT_PRIORITY);
    
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "GPIO ISR service installed successfully");
        s_isr_service_installed = true;
    }
    else if (ret == ESP_ERR_INVALID_STATE)
    {
        // ISR service already installed by another component (board_support.cpp)
        ESP_LOGI(TAG, "GPIO ISR service already installed by another component - using existing service");
        s_isr_service_installed = true;
        ret = ESP_OK;  // This is not an error for us
    }
    else
    {
        ESP_LOGE(TAG, "Failed to install GPIO ISR service: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

/**
 * @brief Attach DIO interrupt handlers
 */
void sx127x_hal_dio_irq_attach(const sx127x_t *radio)
{
    sx127x_esp_context_t *ctx = sx127x_esp_get_context(radio);
    if (!ctx)
    {
        ESP_LOGE(TAG, "Invalid radio context");
        return;
    }

    /* Boot-time collision guard (Issue #332): Ensure DIO0, DIO1, DIO2 use distinct GPIOs.
     * ESP-IDF gpio_isr_handler_add() silently overwrites on duplicate GPIO, so a config
     * collision (e.g., DIO2=26 == DIO0=26) replaces DIO0's TX_DONE/RX_DONE handler with
     * DIO2's no-op, wedging the LoRa stack in LWPSTATE_SEND → LBM failsafe panic loop. */
    if ((ctx->dio_pins[0] == ctx->dio_pins[1]) ||
        (ctx->dio_pins[0] == ctx->dio_pins[2]) ||
        (ctx->dio_pins[1] == ctx->dio_pins[2]))
    {
        ESP_LOGE(TAG,
                 "DIO pin collision detected: DIO0=%d, DIO1=%d, DIO2=%d. "
                 "Each DIO must use a distinct GPIO. Check CONFIG_LBM_SX127X_DIO{0,1,2}_GPIO.",
                 ctx->dio_pins[0], ctx->dio_pins[1], ctx->dio_pins[2]);
        return;
    }

    // Store callbacks from radio structure
    ctx->dio_callbacks[0] = radio->dio_0_irq_handler;
    ctx->dio_callbacks[1] = radio->dio_1_irq_handler;
    ctx->dio_callbacks[2] = radio->dio_2_irq_handler;

    // Configure GPIO pins for interrupts
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << ctx->dio_pins[0]) |
                        (1ULL << ctx->dio_pins[1]) |
                        (1ULL << ctx->dio_pins[2]),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure DIO GPIO pins: %s", esp_err_to_name(ret));
        return;
    }

    // Install ISR service in a thread-safe manner
    ret = install_gpio_isr_service_safe();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to ensure GPIO ISR service installation");
        return;
    }

    // Remove any existing handlers first to avoid conflicts
    gpio_isr_handler_remove(ctx->dio_pins[0]);
    gpio_isr_handler_remove(ctx->dio_pins[1]);
    gpio_isr_handler_remove(ctx->dio_pins[2]);

    // Attach ISR handlers
    ret = gpio_isr_handler_add(ctx->dio_pins[0], sx127x_esp_dio0_isr, ctx);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add DIO0 ISR handler: %s", esp_err_to_name(ret));
        return;
    }

    ret = gpio_isr_handler_add(ctx->dio_pins[1], sx127x_esp_dio1_isr, ctx);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add DIO1 ISR handler: %s", esp_err_to_name(ret));
        goto cleanup_dio0;
    }

    ret = gpio_isr_handler_add(ctx->dio_pins[2], sx127x_esp_dio2_isr, ctx);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add DIO2 ISR handler: %s", esp_err_to_name(ret));
        goto cleanup_dio1;
    }

    ESP_LOGI(TAG, "DIO interrupts attached: DIO0=%d, DIO1=%d, DIO2=%d",
             ctx->dio_pins[0], ctx->dio_pins[1], ctx->dio_pins[2]);
    return;

cleanup_dio1:
    gpio_isr_handler_remove(ctx->dio_pins[1]);
cleanup_dio0:
    gpio_isr_handler_remove(ctx->dio_pins[0]);
}

/**
 * @brief Write data to SX127x via SPI
 */
sx127x_hal_status_t sx127x_hal_write(const sx127x_t *radio, const uint16_t address,
                                     const uint8_t *data, const uint16_t data_len)
{
    sx127x_esp_context_t *ctx = sx127x_esp_get_context(radio);
    if (!ctx || !ctx->initialized)
    {
        return SX127X_HAL_STATUS_ERROR;
    }

    // Take SPI mutex for thread safety
    if (xSemaphoreTake(ctx->spi_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        ESP_LOGW(TAG, "Failed to take SPI mutex");
        return SX127X_HAL_STATUS_ERROR;
    }

    sx127x_hal_status_t status = SX127X_HAL_STATUS_OK;

#ifdef CONFIG_LBM_SX127X_DEBUG_SPI
    ESP_LOGD(TAG, "SPI Write: Addr=0x%04X, Len=%d", address, data_len);
    ESP_LOG_BUFFER_HEXDUMP(TAG, data, data_len, ESP_LOG_DEBUG);
#endif

    // Critical timing section - minimal logging only for payload and mode changes
#ifdef CONFIG_LBM_SX127X_DEBUG_CRITICAL_TIMING
    if (address == 0x00 && data_len >= 1)
    {
        // FIFO write - this is the payload!
        ESP_LOGI(TAG, "🚨 CRITICAL: Writing %d bytes to FIFO (payload transmission!)", data_len);
    }
    else if (address == 0x01 && data_len == 1)
    {
        // OpMode register - check if switching to TX mode
        uint8_t opmode = data[0];
        if ((opmode & 0x07) == 3)
        { // Mode = 3 is TX mode
            ESP_LOGI(TAG, "🚨 CRITICAL: Setting radio to TX mode (OpMode=0x%02X)", opmode);
        }
    }
#endif

    // Start timing measurement
    sx127x_esp_spi_timing_start(ctx);

    // Prepare SPI transaction
    spi_transaction_t trans = {
        .length = (1 + data_len) * 8, // Total bits
        .tx_buffer = NULL,
        .rx_buffer = NULL};

    // Use stack allocation for small transactions to avoid malloc overhead
    uint8_t stack_buffer[64];  // Stack buffer for small transactions
    uint8_t *tx_buffer;
    
    if ((1 + data_len) <= sizeof(stack_buffer))
    {
        tx_buffer = stack_buffer;
    }
    else
    {
        // Allocate buffer for large transactions
        tx_buffer = malloc(1 + data_len);
        if (!tx_buffer)
        {
            ESP_LOGE(TAG, "Failed to allocate SPI buffer");
            status = SX127X_HAL_STATUS_ERROR;
            goto cleanup;
        }
    }

    // Prepare write command (address with MSB set)
    tx_buffer[0] = address | 0x80;
    memcpy(&tx_buffer[1], data, data_len);
    trans.tx_buffer = tx_buffer;

    // Execute SPI transaction
    esp_err_t ret = spi_device_transmit(ctx->spi_device, &trans);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI write failed: %s", esp_err_to_name(ret));
        status = SX127X_HAL_STATUS_ERROR;
    }

    // Free buffer only if it was dynamically allocated
    if ((1 + data_len) > sizeof(stack_buffer) && tx_buffer)
    {
        free(tx_buffer);
    }

cleanup:
    // End timing measurement
    sx127x_esp_spi_timing_end(ctx);

#ifdef CONFIG_LBM_SX127X_DEBUG_SPI
    ESP_LOGD(TAG, "SPI Write Status: %d", status);
#endif

    xSemaphoreGive(ctx->spi_mutex);
    return status;
}

/**
 * @brief Read data from SX127x via SPI
 */
sx127x_hal_status_t sx127x_hal_read(const sx127x_t *radio, const uint16_t address,
                                    uint8_t *data, const uint16_t data_len)
{
    sx127x_esp_context_t *ctx = sx127x_esp_get_context(radio);
    if (!ctx || !ctx->initialized)
    {
        return SX127X_HAL_STATUS_ERROR;
    }

    // Take SPI mutex for thread safety
    if (xSemaphoreTake(ctx->spi_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        ESP_LOGW(TAG, "Failed to take SPI mutex");
        return SX127X_HAL_STATUS_ERROR;
    }

    sx127x_hal_status_t status = SX127X_HAL_STATUS_OK;

#ifdef CONFIG_LBM_SX127X_DEBUG_SPI
    ESP_LOGD(TAG, "SPI Read: Addr=0x%04X, Len=%d", address, data_len);
#endif

    // Critical timing section - minimal logging only for important register reads
#ifdef CONFIG_LBM_SX127X_DEBUG_CRITICAL_TIMING
    if (address == 0x12 && data_len == 1)
    {
        // IRQ flags register - critical for transmission status
        ESP_LOGD(TAG, "🔍 Reading IRQ flags register (0x12) - checking for TXDONE");
    }
    else if (address == 0x01 && data_len == 1)
    {
        // OpMode register - check current mode
        ESP_LOGD(TAG, "🔍 Reading OpMode register (0x01) - checking radio mode");
    }
#endif

    // Start timing measurement
    sx127x_esp_spi_timing_start(ctx);

    // Prepare SPI transaction
    spi_transaction_t trans = {
        .length = (1 + data_len) * 8, // Total bits
        .tx_buffer = NULL,
        .rx_buffer = NULL};

    // Use stack allocation for small transactions to avoid malloc overhead
    uint8_t tx_stack_buffer[64];  // Stack buffer for small transactions
    uint8_t rx_stack_buffer[64];  // Stack buffer for small transactions
    uint8_t *tx_buffer;
    uint8_t *rx_buffer;
    
    if ((1 + data_len) <= sizeof(tx_stack_buffer))
    {
        tx_buffer = tx_stack_buffer;
        rx_buffer = rx_stack_buffer;
    }
    else
    {
        // Allocate buffers for large transactions
        tx_buffer = malloc(1 + data_len);
        rx_buffer = malloc(1 + data_len);

        if (!tx_buffer || !rx_buffer)
        {
            ESP_LOGE(TAG, "Failed to allocate SPI buffers");
            status = SX127X_HAL_STATUS_ERROR;
            goto cleanup;
        }
    }

    // Prepare read command (address with MSB clear)
    tx_buffer[0] = address & 0x7F;
    memset(&tx_buffer[1], 0, data_len); // Dummy bytes for read

    trans.tx_buffer = tx_buffer;
    trans.rx_buffer = rx_buffer;

    // Execute SPI transaction
    esp_err_t ret = spi_device_transmit(ctx->spi_device, &trans);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI read failed: %s", esp_err_to_name(ret));
        status = SX127X_HAL_STATUS_ERROR;
        goto cleanup;
    }

    // Copy received data (skip first byte which is dummy)
    memcpy(data, &rx_buffer[1], data_len);

    // Critical timing section - minimal logging for important register values
#ifdef CONFIG_LBM_SX127X_DEBUG_CRITICAL_TIMING
    if (address == 0x12 && data_len == 1)
    {
        // IRQ flags register - critical for transmission status
        uint8_t irq_flags = data[0];
        ESP_LOGD(TAG, "🔍 IRQ flags = 0x%02X (TXDONE=%s)", irq_flags, (irq_flags & 0x08) ? "YES" : "NO");
    }
    else if (address == 0x01 && data_len == 1)
    {
        // OpMode register - check current mode
        uint8_t opmode = data[0];
        uint8_t mode = opmode & 0x07;
        const char *mode_str = (mode == 0) ? "Sleep" : (mode == 1) ? "Standby"
                                                   : (mode == 2)   ? "FS_TX"
                                                   : (mode == 3)   ? "TX"
                                                   : (mode == 4)   ? "FS_RX"
                                                   : (mode == 5)   ? "RX_CONT"
                                                   : (mode == 6)   ? "RX_SINGLE"
                                                   : (mode == 7)   ? "CAD"
                                                                   : "Unknown";
        ESP_LOGD(TAG, "🔍 OpMode = 0x%02X (%s mode, %s)", opmode, mode_str, (opmode & 0x80) ? "LoRa" : "FSK");
    }
#endif

#ifdef CONFIG_LBM_SX127X_DEBUG_SPI
    ESP_LOG_BUFFER_HEXDUMP(TAG, data, data_len, ESP_LOG_DEBUG);
#endif

cleanup:
    // Free buffers only if they were dynamically allocated
    if ((1 + data_len) > sizeof(tx_stack_buffer))
    {
        if (tx_buffer)
            free(tx_buffer);
        if (rx_buffer)
            free(rx_buffer);
    }

    // End timing measurement
    sx127x_esp_spi_timing_end(ctx);

#ifdef CONFIG_LBM_SX127X_DEBUG_SPI
    ESP_LOGD(TAG, "SPI Read Status: %d", status);
#endif

    xSemaphoreGive(ctx->spi_mutex);
    return status;
}

/**
 * @brief Reset the SX127x radio (weak function - can be overridden)
 */
void sx127x_hal_reset(const sx127x_t *radio)
{
#ifdef CONFIG_LBM_SX127X_USE_CUSTOM_RESET
    ESP_LOGW(TAG, "Custom reset implementation should be provided. Using default fallback.");
#endif

    sx127x_esp_context_t *ctx = sx127x_esp_get_context(radio);
    if (!ctx)
    {
        // Context not initialized yet - initialize it now
        ESP_LOGI(TAG, "Radio context not initialized, initializing now");
        sx127x_esp_err_t err = sx127x_esp_init_for_lbm((sx127x_t *)radio);
        if (err != SX127X_ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to initialize radio context: %s", sx127x_esp_err_to_string(err));
            return;
        }
        ctx = sx127x_esp_get_context(radio);
        if (!ctx)
        {
            ESP_LOGE(TAG, "Still invalid radio context after initialization");
            return;
        }

        /* Audit log: verify radio chip presence and log variant (SX1276/SX1272) */
        lora_hal_lbm_audit_log_version(ctx->spi_device);
    }

    ESP_LOGI(TAG, "Resetting SX127x radio");

    // Additional safety check for the GPIO value to prevent GPIO 238 error
    if (ctx->reset_gpio >= GPIO_NUM_MAX)
    {
        ESP_LOGE(TAG, "Invalid reset GPIO: %d (max: %d)", ctx->reset_gpio, GPIO_NUM_MAX);
        ESP_LOGE(TAG, "This indicates memory corruption or uninitialized context!");
#ifdef CONFIG_LBM_SX127X_USE_CUSTOM_RESET
        ESP_LOGE(TAG, "Custom reset is enabled - reset_gpio should not be used");
        // For custom reset, set to NC (not connected)
        ctx->reset_gpio = GPIO_NUM_NC;
        ESP_LOGW(TAG, "Using GPIO_NUM_NC for custom reset implementation");
#else
        ESP_LOGE(TAG, "Expected reset GPIO: %d", CONFIG_LBM_SX127X_RESET_GPIO);
        // Use the configured value as fallback
        ctx->reset_gpio = CONFIG_LBM_SX127X_RESET_GPIO;
        ESP_LOGW(TAG, "Using fallback reset GPIO: %d", ctx->reset_gpio);
#endif
    }

    ESP_LOGI(TAG, "Using reset GPIO: %d", ctx->reset_gpio);

    // Call the GPIO reset function (can be overridden)
    sx127x_hal_gpio_reset(ctx->reset_gpio);
}

/**
 * @brief GPIO reset function (weak - can be overridden for custom GPIO implementations)
 * This function handles the actual GPIO manipulation for reset.
 * Override this function if you're using I2C GPIO expanders or other custom GPIO.
 */
__attribute__((weak)) void sx127x_hal_gpio_reset(gpio_num_t reset_gpio)
{
    ESP_LOGI(TAG, "Default GPIO reset using ESP32 GPIO %d", reset_gpio);

#if defined(CONFIG_LBM_SX127X_RADIO_SX1272) || defined(SX1272)
    // SX1272: Set RESET pin to 1
    gpio_set_level(reset_gpio, 1);
#elif defined(CONFIG_LBM_SX127X_RADIO_SX1276) || defined(SX1276)
    // SX1276: Set RESET pin to 0
    gpio_set_level(reset_gpio, 0);
#endif

    // Wait 1 ms
    vTaskDelay(pdMS_TO_TICKS(1));

    // Configure RESET pin as input (high-Z)
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << reset_gpio),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);

    // Wait for radio to be ready
    vTaskDelay(pdMS_TO_TICKS(6));
}

/**
 * @brief Get DIO1 pin state
 */
uint32_t sx127x_hal_get_dio_1_pin_state(const sx127x_t *radio)
{
    sx127x_esp_context_t *ctx = sx127x_esp_get_context(radio);
    if (!ctx)
    {
        return 0;
    }

    return gpio_get_level(ctx->dio_pins[1]);
}

/**
 * @brief ESP timer callback for RX timeout
 *
 * Delivers the timeout as a synthetic event on the DIO event queue
 * to avoid SPI transactions on the esp_timer task (spec §5.5).
 */
static void sx127x_esp_rx_timer_callback(void *arg)
{
    sx127x_esp_context_t *ctx = (sx127x_esp_context_t *)arg;
    if (!ctx)
    {
        return;
    }

    /* Read started flag and generation counter under lock */
    bool started;
    uint32_t gen;
    portENTER_CRITICAL(&ctx->rx_timer_lock);
    started = ctx->rx_timer_started;
    gen = ctx->rx_timer_gen;
    if (started)
    {
        ctx->rx_timer_started = false;
    }
    portEXIT_CRITICAL(&ctx->rx_timer_lock);

    if (!started)
    {
        /* Timer was stopped before expiry - stale callback */
        return;
    }

    /* Queue synthetic event for DIO event task */
    if (ctx->event_queue != NULL)
    {
        sx127x_esp_dio_event_t event = {
            .dio_num = SX127X_ESP_EVENT_RX_TIMER,
            .timestamp = esp_timer_get_time(),
            .gen = gen
        };

        BaseType_t ret = xQueueSend(ctx->event_queue, &event, 0);
        if (ret != pdTRUE)
        {
            ESP_LOGW(TAG, "RX timeout event queue full - dropped");
        }
        else
        {
            ESP_LOGD(TAG, "SX127x RX timeout queued (gen=%" PRIu32 ")", gen);
        }
    }
    else
    {
        /* No event queue - log warning (should have been caught at start) */
        ESP_LOGW(TAG, "RX timeout fired but no event queue configured");
    }
}

/**
 * @brief Warn if event queue is missing (one-time warning)
 */
static void sx127x_esp_warn_no_event_queue(void)
{
    static bool warned_once = false;
    if (!warned_once)
    {
        ESP_LOGW(TAG, "RX timer started but no event queue - callback on expiry will warn");
        warned_once = true;
    }
}

/**
 * @brief Create RX timeout timer (lazy initialization helper)
 */
static sx127x_hal_status_t sx127x_esp_create_rx_timer(sx127x_esp_context_t *ctx)
{
    const esp_timer_create_args_t timer_args = {
        .callback = sx127x_esp_rx_timer_callback,
        .arg = ctx,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "sx127x_rx_timeout",
        .skip_unhandled_events = false
    };

    esp_err_t ret = esp_timer_create(&timer_args, &ctx->rx_timer);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create RX timeout timer: %s", esp_err_to_name(ret));
        return SX127X_HAL_STATUS_ERROR;
    }

    ESP_LOGI(TAG, "SX127x RX timeout timer created (Class C support)");
    return SX127X_HAL_STATUS_OK;
}

/**
 * @brief Start timer for SX127x operations
 *
 * Used to emulate hardware RX timeout for Class C continuous receive mode.
 * The SX127x chip lacks hardware timeout in continuous RX, so LBM uses a
 * 120s software timer to periodically re-launch RX and avoid radio planner
 * failsafe (128s limit).
 */
sx127x_hal_status_t sx127x_hal_timer_start(const sx127x_t *radio, const uint32_t time_in_ms,
                                           void (*callback)(void *context))
{
    sx127x_esp_context_t *ctx = sx127x_esp_get_context(radio);
    if (!ctx || !ctx->initialized)
    {
        return SX127X_HAL_STATUS_ERROR;
    }

    if (!callback || time_in_ms == 0U)
    {
        ESP_LOGE(TAG, "Invalid timer parameters: callback=%p, time=%" PRIu32 " ms",
                 (void *)callback, time_in_ms);
        return SX127X_HAL_STATUS_ERROR;
    }

    if (ctx->event_queue == NULL)
    {
        sx127x_esp_warn_no_event_queue();
    }

    if (ctx->rx_timer == NULL)
    {
        if (sx127x_esp_create_rx_timer(ctx) != SX127X_HAL_STATUS_OK)
        {
            return SX127X_HAL_STATUS_ERROR;
        }
    }

    /* Stop timer outside critical section (esp_timer_stop takes internal lock) */
    (void)esp_timer_stop(ctx->rx_timer);

    /* Update state under critical section */
    portENTER_CRITICAL(&ctx->rx_timer_lock);
    ctx->rx_timer_callback = callback;
    ctx->rx_timer_gen++;
    ctx->rx_timer_started = true;
    portEXIT_CRITICAL(&ctx->rx_timer_lock);

    /* Start timer outside critical section */
    uint64_t timeout_us = (uint64_t)time_in_ms * 1000ULL;
    esp_err_t ret = esp_timer_start_once(ctx->rx_timer, timeout_us);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start RX timeout timer: %s", esp_err_to_name(ret));
        /* Clear started flag on failure */
        portENTER_CRITICAL(&ctx->rx_timer_lock);
        ctx->rx_timer_started = false;
        portEXIT_CRITICAL(&ctx->rx_timer_lock);
        return SX127X_HAL_STATUS_ERROR;
    }

    static bool first_use = true;
    if (first_use)
    {
        ESP_LOGI(TAG, "SX127x SW RX timeout timer armed: %" PRIu32 " ms", time_in_ms);
        first_use = false;
    }
    else
    {
        ESP_LOGD(TAG, "SX127x RX timeout timer armed: %" PRIu32 " ms", time_in_ms);
    }

    return SX127X_HAL_STATUS_OK;
}

/**
 * @brief Stop timer
 */
sx127x_hal_status_t sx127x_hal_timer_stop(const sx127x_t *radio)
{
    sx127x_esp_context_t *ctx = sx127x_esp_get_context(radio);
    if (!ctx || !ctx->initialized)
    {
        return SX127X_HAL_STATUS_ERROR;
    }

    /* Idempotent: safe to call even if not started */
    if (ctx->rx_timer == NULL)
    {
        return SX127X_HAL_STATUS_OK;
    }

    /* Stop timer outside critical section */
    esp_err_t ret = esp_timer_stop(ctx->rx_timer);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        /* ESP_ERR_INVALID_STATE means timer wasn't running - that's OK */
        ESP_LOGW(TAG, "Failed to stop RX timeout timer: %s", esp_err_to_name(ret));
        return SX127X_HAL_STATUS_ERROR;
    }

    /* Update state under critical section - increment gen to invalidate any queued events */
    portENTER_CRITICAL(&ctx->rx_timer_lock);
    ctx->rx_timer_started = false;
    ctx->rx_timer_gen++;
    portEXIT_CRITICAL(&ctx->rx_timer_lock);

    ESP_LOGD(TAG, "SX127x RX timeout timer stopped");
    return SX127X_HAL_STATUS_OK;
}

/**
 * @brief Check if timer is started
 */
bool sx127x_hal_timer_is_started(const sx127x_t *radio)
{
    sx127x_esp_context_t *ctx = sx127x_esp_get_context(radio);
    if (!ctx)
    {
        return false;
    }

    return ctx->rx_timer_started;
}

/**
 * @brief NSS (Chip Select) control (weak function - can be overridden)
 */
__attribute__((weak)) void sx127x_hal_nss_control(const sx127x_t *radio, bool active)
{
    sx127x_esp_context_t *ctx = sx127x_esp_get_context(radio);
    if (!ctx)
    {
        return;
    }

    // NSS is active low
    gpio_set_level(ctx->nss_gpio, active ? 0 : 1);

#ifdef CONFIG_LBM_SX127X_DEBUG_SPI
    ESP_LOGD(TAG, "NSS %s", active ? "ACTIVE" : "INACTIVE");
#endif
}
