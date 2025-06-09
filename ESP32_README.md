# LoRa Basics Modem SX127x ESP32 Component

This directory contains the ESP-IDF component wrapper for the LoRa Basics Modem SX127x radio driver, enabling easy integration with ESP32-based projects.

## Overview

The ESP32 wrapper provides:
- Complete ESP-IDF component integration
- Configurable hardware abstraction layer (HAL)
- SPI communication with debugging capabilities
- Interrupt handling with optional dedicated task
- Register inspection and debugging utilities
- Weak function implementations for reset and NSS control
- Performance monitoring and statistics

## Features

### Hardware Abstraction
- **SPI Communication**: Full-duplex SPI with configurable frequency and DMA support
- **GPIO Control**: Configurable pins for NSS, Reset, and DIO interrupts
- **Interrupt Handling**: Edge-triggered interrupts with optional task-based processing
- **Weak Functions**: Override reset and NSS control for custom hardware implementations

### Debugging & Monitoring
- **SPI Debug Logging**: Detailed transaction logging with hex dumps
- **Register Inspection**: Complete register dump and query by name functionality
- **Interrupt Statistics**: Count and timing information for DIO events
- **Performance Metrics**: SPI transaction timing and latency measurements

### Configuration
- **Kconfig Integration**: Full menuconfig support for all parameters
- **Selective Compilation**: Enable/disable features based on requirements
- **Memory Options**: Static vs dynamic allocation choices
- **Testing Support**: Built-in self-test and diagnostic functions

## File Structure

```
esp32_hal/
├── include/
│   └── sx127x_esp_wrapper.h          # Public API header
├── private_include/
│   └── sx127x_esp_internal.h         # Internal definitions
├── sx127x_hal_esp.c                  # HAL implementation
├── sx127x_esp_wrapper.c              # Main wrapper functions
└── sx127x_esp_debug.c                # Debug and inspection utilities
```

## Integration Guide

### 1. Add as ESP-IDF Component

Add this repository as a git submodule in your ESP-IDF project:

```bash
cd your_esp_project/components
git submodule add <repository_url> lbm_sx127x
```

### 2. Configure Hardware

Use `idf.py menuconfig` to configure the component:

```
Component config → LoRa Basics Modem SX127x Configuration
```

Key configuration sections:
- **Radio Selection**: Choose SX1272 or SX1276
- **Hardware Configuration**: Set GPIO pins and SPI settings
- **Interrupt Configuration**: Configure DIO handling
- **Debug Options**: Enable logging and diagnostics

### 3. Basic Usage Example

```c
#include "sx127x_esp_wrapper.h"

void app_main(void)
{
    // Initialize radio structure
    sx127x_t radio = {0};
    
    // Get default configuration
    sx127x_esp_config_t config;
    sx127x_esp_get_default_config(&config);
    
    // Customize configuration if needed
    config.spi_frequency = 8000000;  // 8 MHz
    config.dio0_gpio = GPIO_NUM_26;
    
    // Initialize the radio
    sx127x_esp_err_t err = sx127x_esp_init(&radio, &config);
    if (err != SX127X_ESP_OK) {
        ESP_LOGE("APP", "Failed to initialize radio: %s", 
                 sx127x_esp_err_to_string(err));
        return;
    }
    
    // Check if radio is responding
    if (sx127x_esp_is_radio_responding(&radio)) {
        ESP_LOGI("APP", "Radio initialized successfully");
        
        // Print register dump for debugging
        sx127x_esp_print_register_dump(&radio);
    }
    
    // Your application code here...
    
    // Cleanup
    sx127x_esp_deinit(&radio);
}
```

## Configuration Options

### Hardware Configuration

| Parameter | Description | Default |
|-----------|-------------|---------|
| `LBM_SX127X_SPI_HOST` | SPI host (0-2) | 1 |
| `LBM_SX127X_SPI_FREQUENCY` | SPI frequency (Hz) | 8000000 |
| `LBM_SX127X_USE_DMA` | Enable DMA for SPI | true |
| `LBM_SX127X_NSS_GPIO` | NSS (CS) pin | 5 |
| `LBM_SX127X_RESET_GPIO` | Reset pin | 14 |
| `LBM_SX127X_DIO0_GPIO` | DIO0 interrupt pin | 26 |
| `LBM_SX127X_DIO1_GPIO` | DIO1 interrupt pin | 27 |
| `LBM_SX127X_DIO2_GPIO` | DIO2 interrupt pin | 14 |

### Debug Options

| Parameter | Description | Default |
|-----------|-------------|---------|
| `LBM_SX127X_DEBUG_SPI` | Enable SPI debug logging | false |
| `LBM_SX127X_DEBUG_INTERRUPTS` | Enable interrupt debugging | false |
| `LBM_SX127X_INTERRUPT_STATS` | Keep interrupt statistics | true |
| `LBM_SX127X_ENABLE_REGISTER_DUMP` | Enable register inspection | true |

## API Reference

### Initialization Functions

```c
// Initialize radio with configuration
sx127x_esp_err_t sx127x_esp_init(sx127x_t* radio, const sx127x_esp_config_t* config);

// Deinitialize and cleanup
sx127x_esp_err_t sx127x_esp_deinit(sx127x_t* radio);

// Get default configuration
void sx127x_esp_get_default_config(sx127x_esp_config_t* config);
```

### Debugging Functions

```c
// Check if radio is responding
bool sx127x_esp_is_radio_responding(sx127x_t* radio);

// Get radio version
sx127x_esp_err_t sx127x_esp_get_radio_version(sx127x_t* radio, uint8_t* version);

// Print all registers to console
void sx127x_esp_print_register_dump(sx127x_t* radio);

// Read specific register by name
sx127x_esp_err_t sx127x_esp_read_register_by_name(sx127x_t* radio, const char* reg_name, uint8_t* value);
```

### Statistics Functions

```c
// Get interrupt statistics
sx127x_esp_err_t sx127x_esp_get_interrupt_stats(sx127x_t* radio, sx127x_esp_interrupt_stats_t* stats);

// Reset interrupt statistics
sx127x_esp_err_t sx127x_esp_reset_interrupt_stats(sx127x_t* radio);

// Get performance metrics
sx127x_esp_err_t sx127x_esp_get_performance_metrics(sx127x_t* radio, sx127x_esp_performance_t* performance);
```

## Weak Functions

The following functions are declared as weak and can be overridden for custom hardware:

### Reset Control
```c
__attribute__((weak)) void sx127x_hal_reset(const sx127x_t* radio);
```
Override this function if your reset pin is controlled via a port expander or custom logic.

### NSS Control
```c
__attribute__((weak)) void sx127x_hal_nss_control(const sx127x_t* radio, bool active);
```
Override this function if your NSS pin is controlled via a port expander or custom logic.

## Debugging Features

### SPI Transaction Logging

Enable `LBM_SX127X_DEBUG_SPI` to see detailed SPI transactions:

```
D (1234) sx127x_hal_esp: SPI Write: Addr=0x0001, Len=1
D (1234) sx127x_hal_esp: 80
D (1234) sx127x_hal_esp: SPI Write Status: 0
```

### Register Inspection

Use the register dump functionality to inspect radio state:

```c
// Print all registers
sx127x_esp_print_register_dump(&radio);

// Read specific register
uint8_t version;
sx127x_esp_read_register_by_name(&radio, "RegVersion", &version);
```

### Interrupt Statistics

Monitor interrupt activity:

```c
sx127x_esp_interrupt_stats_t stats;
sx127x_esp_get_interrupt_stats(&radio, &stats);

ESP_LOGI("APP", "DIO0 interrupts: %lu", stats.dio0_count);
ESP_LOGI("APP", "DIO1 interrupts: %lu", stats.dio1_count);
ESP_LOGI("APP", "Last DIO0 time: %llu us", stats.last_dio0_time);
```

## Performance Considerations

### SPI Configuration
- Use DMA for better performance with large transfers
- Adjust SPI frequency based on your requirements (1-20 MHz)
- Consider interrupt priority for time-critical applications

### Memory Usage
- Enable static allocation to avoid heap fragmentation
- Adjust buffer sizes based on your packet requirements
- Use dedicated task for interrupt processing in high-throughput scenarios

### Power Management
- Disable unused features to reduce code size
- Use interrupt statistics to optimize power consumption
- Consider GPIO configuration for low-power modes

## Troubleshooting

### Common Issues

1. **Radio not responding**
   - Check SPI wiring and configuration
   - Verify power supply and reset timing
   - Use register dump to verify communication

2. **Interrupt issues**
   - Check DIO pin configuration
   - Verify interrupt priority settings
   - Enable interrupt debugging for diagnostics

3. **SPI communication errors**
   - Enable SPI debug logging
   - Check clock frequency and mode
   - Verify NSS timing and control

### Debug Steps

1. Enable debug logging in menuconfig
2. Use `sx127x_esp_is_radio_responding()` to verify basic communication
3. Print register dump to check radio state
4. Monitor interrupt statistics for timing issues
5. Check performance metrics for SPI timing

## License

This ESP32 wrapper follows the same license as the LoRa Basics Modem library.

## Contributing

When contributing to the ESP32 wrapper:
1. Follow ESP-IDF coding standards
2. Add appropriate Kconfig options for new features
3. Include debug logging for troubleshooting
4. Update this README with new functionality
5. Test with both SX1272 and SX1276 variants
