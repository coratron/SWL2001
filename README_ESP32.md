# LoRa Basic Modem ESP-IDF Component

This is an ESP-IDF component port of the LoRa Basic Modem (LBM) library, providing a complete LoRaWAN implementation for ESP32 microcontrollers with support for multiple radio types.

## Features

- **Full LoRaWAN L2 1.0.4 Specification Compliance**
- **Multi-Radio Support**: SX126X, SX127X, LR11XX, SX128X
- **Regional Parameters**: Support for all LoRaWAN regions
- **Class A, B, C Support** (configurable)
- **FUOTA (Firmware Update Over The Air)**
- **Multicast Support**
- **ALCSync (Application Layer Clock Synchronization)**
- **ESP-IDF Integration**: Native component with menuconfig support

## Supported Hardware

### ESP32 Microcontrollers
- ESP32
- ESP32-S2
- ESP32-S3
- ESP32-C3
- ESP32-C6
- ESP32-H2

### Supported LoRa Radios
- **SX126X Series**: SX1261, SX1262, SX1268
- **SX127X Series**: SX1272, SX1276
- **LR11XX Series**: LR1110, LR1120, LR1121
- **SX128X Series**: SX1280, SX1281

## Quick Start

### 1. Add Component to Your ESP-IDF Project

Add this component to your ESP-IDF project by including it in your `components` directory or using the ESP Component Registry.

```bash
# Method 1: Copy to components directory
cp -r /path/to/lbm_component /path/to/your_project/components/lbm

# Method 2: Add as git submodule
cd /path/to/your_project/components
git submodule add <repository-url> lbm
```

### 2. Configure the Component

Use ESP-IDF's menuconfig to configure the LoRa Basic Modem:

```bash
idf.py menuconfig
```

Navigate to **Component config → LoRa Basic Modem Configuration** and configure:

- **Radio Type**: Select your radio (SX126X, SX127X, LR11XX, SX128X)
- **Region**: Select your LoRaWAN region (EU868, US915, AS923, etc.)
- **GPIO Configuration**: Configure pins for NSS, RESET, BUSY, DIO pins
- **Radio Specific Settings**: Configure radio-specific parameters

### 3. Hardware Connections

#### SX126X Connections Example
```
ESP32 Pin    SX126X Pin    Description
----------   ----------    -----------
GPIO 5       NSS           SPI Chip Select
GPIO 18      SCK           SPI Clock  
GPIO 19      MISO          SPI MISO
GPIO 23      MOSI          SPI MOSI
GPIO 2       RESET         Radio Reset
GPIO 4       BUSY          Radio Busy (SX126X only)
GPIO 21      DIO1          Digital IO 1
```

#### SX127X Connections Example
```
ESP32 Pin    SX127X Pin    Description
----------   ----------    -----------
GPIO 5       NSS           SPI Chip Select
GPIO 18      SCK           SPI Clock
GPIO 19      MISO          SPI MISO
GPIO 23      MOSI          SPI MOSI
GPIO 2       RESET         Radio Reset
GPIO 21      DIO0          Digital IO 0
GPIO 22      DIO1          Digital IO 1
GPIO 25      DIO2          Digital IO 2 (optional)
```

### 4. Basic Example Code

```c
#include "smtc_modem_api.h"
#include "example_credentials.h"

// LoRaWAN credentials (update these!)
static const uint8_t dev_eui[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t join_eui[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t app_key[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

void app_main(void)
{
    // Initialize the modem
    smtc_modem_init(&modem_radio_irq_callback);
    
    // Configure LoRaWAN credentials
    smtc_modem_set_deveui(STACK_ID, dev_eui);
    smtc_modem_set_joineui(STACK_ID, join_eui);
    smtc_modem_set_appkey(STACK_ID, app_key);
    
    // Set region
    smtc_modem_set_region(STACK_ID, SMTC_MODEM_REGION_EU_868);
    
    // Join the network
    smtc_modem_join_network(STACK_ID);
    
    while(1) {
        // Process modem events
        smtc_modem_run_engine();
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

## Configuration Options

### Basic Configuration

| Option | Description | Default |
|--------|-------------|---------|
| `LBM_RADIO_TYPE` | Radio type selection | SX126X |
| `LBM_REGION` | LoRaWAN region | EU868 |
| `LBM_CLASS_B_ENABLED` | Enable Class B support | No |
| `LBM_CLASS_C_ENABLED` | Enable Class C support | No |
| `LBM_MULTICAST_ENABLED` | Enable multicast support | No |

### GPIO Configuration

Configure the GPIO pins used for radio communication:

- **NSS Pin**: SPI Chip Select
- **RESET Pin**: Radio reset pin
- **BUSY Pin**: Radio busy pin (SX126X only)
- **DIO Pins**: Digital I/O pins for interrupts

### SX126X Specific Configuration

- **Regulator Mode**: LDO or DCDC
- **DIO2 as RF Switch**: Enable/disable
- **TCXO Configuration**: Crystal oscillator settings
- **RX Boost**: Enable for improved sensitivity
- **OCP (Over Current Protection)**: Current limit settings

### SX127X Specific Configuration

- **Board Type**: Select PCB variant
- **OCP Value**: Over current protection setting
- **PA Ramp Time**: Power amplifier ramp time

## Custom Hardware Support

The component is designed to support various hardware configurations beyond the default GPIO-based implementation. Many functions in the radio HAL are implemented as **weak functions** that can be overridden for custom hardware setups.

### Weak Function Architecture

The radio HAL uses weak functions that can be overridden in your application code without modifying the component source. This enables support for:

- **I2C/SPI Port Expanders** (MCP23017, PCF8574, etc.)
- **GPIO Expanders** and shift registers
- **Custom SPI Implementations** (software SPI, different hardware)
- **External Interrupt Controllers**
- **Power Management Integration**
- **Multi-Radio Configurations**

### Override Weak Functions

The following functions can be overridden in your application code for custom implementations:

#### GPIO Control Functions
- **`radio_hal_reset_impl()`** - Override for custom reset control (e.g., I2C port expanders, GPIO expanders)
- **`radio_hal_is_busy_impl()`** - Override for custom busy signal reading (SX126X only)
- **`radio_hal_wakeup_impl()`** - Override for custom radio wakeup procedures

#### SPI Communication Functions  
- **`radio_hal_spi_write_impl()`** - Override for custom SPI implementations (software SPI, different hardware)
- **`radio_hal_spi_read_impl()`** - Override for custom SPI read implementations

#### Interrupt Functions
- **`radio_hal_irq_config_impl()`** - Override for custom interrupt handling
- **`radio_hal_irq_clear_pending_impl()`** - Override for custom interrupt clearing

### Complete Example Implementation

See `examples/radio_hal_custom_example.c` for a comprehensive example showing how to implement radio control using:

1. **I2C Port Expander (MCP23017)** - Complete implementation with GPIO control
2. **Shift Register Implementation** - Using 74HC595 for output control
3. **Custom Power Management** - With radio power switching
4. **Alternative SPI Methods** - Different SPI configurations

### Example: Using I2C Port Expander

```c
// Override reset function for MCP23017 I2C port expander
void radio_hal_reset_impl(void)
{
    // Assert reset via I2C port expander
    mcp23017_set_pin(RADIO_RESET_PORT_BIT, false);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Release reset
    mcp23017_set_pin(RADIO_RESET_PORT_BIT, true);
    vTaskDelay(pdMS_TO_TICKS(10));
}

// Override busy check for port expander
bool radio_hal_is_busy_impl(void)
{
    bool busy_state;
    mcp23017_get_pin(RADIO_BUSY_PORT_BIT, &busy_state);
    return busy_state;
}
```

### Example: Custom SPI Implementation

```c
// Override SPI functions for software SPI or custom hardware
esp_err_t radio_hal_spi_write_impl(const uint8_t* command, uint16_t command_length,
                                  const uint8_t* data, uint16_t data_length)
{
    // Your custom SPI implementation here
    // Could be software SPI, different SPI peripheral, etc.
    return ESP_OK;
}
```c
// Override SPI functions for software SPI or custom hardware
esp_err_t radio_hal_spi_write_impl(const uint8_t* command, uint16_t command_length,
                                  const uint8_t* data, uint16_t data_length)
{
    // Your custom SPI implementation here
    // Could be software SPI, different SPI peripheral, etc.
    return ESP_OK;
}
```

### Using the Complete Example

To implement custom hardware support in your project:

1. **Copy the example file** to your project:
   ```bash
   cp components/lbm/examples/radio_hal_custom_example.c main/
   ```

2. **Modify the hardware configuration** at the top of the file for your specific setup:
   ```c
   #define MCP23017_I2C_ADDR       0x20    // Your I2C address
   #define RADIO_RESET_PORT_BIT    0       // Your pin assignments
   #define I2C_MASTER_SCL_IO       22      // Your I2C pins
   ```

3. **Enable the desired implementation** by uncommenting the appropriate code sections

4. **Build and test** your custom implementation

### Hardware Configuration Examples

#### MCP23017 I2C Port Expander
- **Best for**: Saving GPIO pins, multiple radio control
- **Pros**: 16 GPIO pins over I2C, interrupt support
- **Cons**: Slower response than direct GPIO

#### 74HC595 Shift Register  
- **Best for**: Output-only control, cascading multiple devices
- **Pros**: Very low pin count (3 pins for unlimited outputs)
- **Cons**: Output only, no input feedback

#### Custom SPI Bus
- **Best for**: Shared SPI bus, different timing requirements
- **Pros**: Flexible timing, custom protocols
- **Cons**: More complex implementation

## API Usage

### Initialization

```c
// Initialize the modem
smtc_modem_return_code_t rc = smtc_modem_init(&radio_irq_callback);
if (rc != SMTC_MODEM_RC_OK) {
    // Handle initialization error
}
```

### LoRaWAN Configuration

```c
// Set credentials
smtc_modem_set_deveui(STACK_ID, dev_eui);
smtc_modem_set_joineui(STACK_ID, join_eui);
smtc_modem_set_appkey(STACK_ID, app_key);

// Set region
smtc_modem_set_region(STACK_ID, SMTC_MODEM_REGION_EU_868);

// Set device class
smtc_modem_set_class(STACK_ID, SMTC_MODEM_CLASS_A);
```

### Network Operations

```c
// Join network
smtc_modem_join_network(STACK_ID);

// Send uplink data
uint8_t data[] = "Hello LoRaWAN!";
smtc_modem_request_uplink(STACK_ID, 2, false, data, sizeof(data));

// Check for downlink data
uint8_t rx_buffer[255];
uint8_t rx_buffer_size = sizeof(rx_buffer);
smtc_modem_get_downlink_data(STACK_ID, rx_buffer, &rx_buffer_size, &metadata);
```

### Event Handling

```c
// Main event loop
void modem_event_process(void)
{
    smtc_modem_event_t current_event;
    uint8_t event_pending_count;
    
    // Process all pending events
    do {
        smtc_modem_get_event(&current_event, &event_pending_count);
        
        switch(current_event.event_type) {
            case SMTC_MODEM_EVENT_JOINED:
                printf("Successfully joined LoRaWAN network\n");
                break;
                
            case SMTC_MODEM_EVENT_TXDONE:
                printf("Uplink transmission completed\n");
                break;
                
            case SMTC_MODEM_EVENT_DOWNDATA:
                printf("Downlink data received\n");
                // Process received data
                break;
                
            default:
                break;
        }
    } while(event_pending_count > 0);
}
```

## Advanced Features

### FUOTA (Firmware Update Over The Air)

Enable FUOTA support in menuconfig and implement the required callbacks:

```c
// Enable FUOTA
smtc_modem_set_certification_mode(STACK_ID, true);

// Handle FUOTA events
case SMTC_MODEM_EVENT_FUOTA_START:
    // Prepare for firmware update
    break;
    
case SMTC_MODEM_EVENT_FUOTA_COMPLETE:
    // Firmware update completed
    esp_restart();  // Restart to apply new firmware
    break;
```

### Class B Operation

```c
// Enable Class B
smtc_modem_set_class(STACK_ID, SMTC_MODEM_CLASS_B);

// Configure ping slot periodicity
smtc_modem_class_b_set_ping_slot_periodicity(STACK_ID, SMTC_MODEM_CLASS_B_PINGSLOT_128_S);
```

### Multicast

```c
// Configure multicast session
smtc_modem_multicast_set_grp_config(STACK_ID, mc_grp_id, mc_grp_addr, 
                                     mc_nwk_skey, mc_app_skey);

// Start multicast session
smtc_modem_multicast_start_session(STACK_ID, mc_grp_id);
```

## Debugging

### Enable Debug Logging

In menuconfig, navigate to **Component config → Log output** and set the log level to DEBUG for detailed logging.

### Common Issues

1. **Radio not responding**: Check SPI connections and power supply
2. **Join failures**: Verify LoRaWAN credentials and region settings  
3. **GPIO conflicts**: Ensure GPIO pins are not used by other peripherals
4. **Memory issues**: Increase stack size if experiencing stack overflow

### Debug Functions

```c
// Get modem version
smtc_modem_version_t modem_version;
smtc_modem_get_modem_version(&modem_version);
printf("Modem version: %d.%d.%d\n", modem_version.major, 
       modem_version.minor, modem_version.patch);

// Get radio status
uint32_t status = smtc_modem_get_status(STACK_ID);
printf("Modem status: 0x%08lx\n", status);
```

## Example Projects

### Basic LoRaWAN Application

See `examples/lbm_example.c` for a complete example demonstrating:
- Modem initialization
- Network joining
- Periodic uplink transmission
- Downlink data handling

### Configuration Examples

Different hardware configurations are provided in the examples:

- **ESP32 + SX1276**: Classic ESP32 with SX1276 radio
- **ESP32-S3 + SX1262**: ESP32-S3 with SX1262 radio
- **ESP32-C3 + LR1110**: ESP32-C3 with LR1110 transceiver

## Performance Considerations

### Memory Usage

- **Flash**: ~200KB (typical configuration)
- **RAM**: ~20KB (runtime usage)
- **Stack**: Minimum 4KB recommended for main task

### Power Consumption

The modem supports low-power operation:
- Implement sleep modes between transmissions
- Use appropriate radio power settings
- Consider ESP32 deep sleep for battery applications

## Migration from Other Platforms

### From Arduino LoRaWAN Libraries

Key differences:
- Event-driven architecture vs. blocking calls
- More comprehensive LoRaWAN feature support
- Built-in FUOTA and multicast capabilities

### From STM32 LBM Implementations

- GPIO configuration through menuconfig vs. compile-time defines
- ESP-IDF HAL integration vs. STM32 HAL
- FreeRTOS integration for task management

## Troubleshooting

### Build Issues

```bash
# Clean and rebuild
idf.py clean
idf.py build

# Check component dependencies
idf.py menuconfig
```

### Runtime Issues

1. **Enable debug logging** to identify issues
2. **Check hardware connections** with a multimeter
3. **Verify LoRaWAN credentials** are correct
4. **Test with known good gateway** in range

## Support and Resources

### Documentation
- [LoRa Basics Modem Documentation](../lbm_lib/README.md)
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/)
- [LoRaWAN Specification](https://resources.lora-alliance.org/)

### Community
- [ESP32 Forums](https://www.esp32.com/)
- [LoRa Developer Portal](https://lora-developers.semtech.com/)

### License

This ESP-IDF component is released under the same license as the original LoRa Basic Modem library. See [LICENSE](../LICENSE.txt) for details.

---

**Note**: Remember to update LoRaWAN credentials (DevEUI, JoinEUI, AppKey) before deploying to production. The example credentials provided will not work with real LoRaWAN networks.
