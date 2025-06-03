idf_component_register(
    SRCS 
        # Core modem files
        "lbm_lib/smtc_modem_core/smtc_modem.c"
        "lbm_lib/smtc_modem_core/smtc_modem_test.c"
        
        # Modem utilities
        "lbm_lib/smtc_modem_core/modem_utilities/modem_core.c"
        "lbm_lib/smtc_modem_core/modem_utilities/modem_context.c"
        "lbm_lib/smtc_modem_core/modem_utilities/modem_event_utilities.c"
        
        # Modem supervisor
        "lbm_lib/smtc_modem_core/modem_supervisor/modem_supervisor.c"
        "lbm_lib/smtc_modem_core/modem_supervisor/modem_supervisor_light.c"
        "lbm_lib/smtc_modem_core/modem_supervisor/modem_tx_protocol_manager.c"
        
        # LoRaWAN stack core
        "lbm_lib/smtc_modem_core/lorawan_stack/lorawan_api.c"
        "lbm_lib/smtc_modem_core/lorawan_stack/lorawan_certification.c"
        "lbm_lib/smtc_modem_core/lorawan_stack/lorawan_send_management.c"
        
        # MAC layer
        "lbm_lib/smtc_modem_core/lr1mac/src/lr1_stack_mac_layer.c"
        "lbm_lib/smtc_modem_core/lr1mac/src/lr1mac_core.c"
        "lbm_lib/smtc_modem_core/lr1mac/src/lr1mac_utilities.c"
        "lbm_lib/smtc_modem_core/lr1mac/src/services/smtc_duty_cycle.c"
        "lbm_lib/smtc_modem_core/lr1mac/src/services/smtc_lbt.c"
        "lbm_lib/smtc_modem_core/lr1mac/src/services/smtc_lora_cad_bt.c"
        "lbm_lib/smtc_modem_core/lr1mac/src/services/smtc_ping_slot.c"
        "lbm_lib/smtc_modem_core/lr1mac/src/services/smtc_multicast.c"
        
        # Regional abstraction layer
        "lbm_lib/smtc_modem_core/smtc_ral/src/ral.c"
        "lbm_lib/smtc_modem_core/smtc_ral/src/ral_lr11xx.c"
        "lbm_lib/smtc_modem_core/smtc_ral/src/ral_sx126x.c"
        "lbm_lib/smtc_modem_core/smtc_ral/src/ral_sx127x.c"
        "lbm_lib/smtc_modem_core/smtc_ral/src/ral_sx128x.c"
        
        # RALF (RAL abstraction)
        "lbm_lib/smtc_modem_core/smtc_ralf/src/ralf_lr11xx.c"
        "lbm_lib/smtc_modem_core/smtc_ralf/src/ralf_sx126x.c"
        "lbm_lib/smtc_modem_core/smtc_ralf/src/ralf_sx127x.c"
        "lbm_lib/smtc_modem_core/smtc_ralf/src/ralf_sx128x.c"
        
        # Radio planner
        "lbm_lib/smtc_modem_core/radio_planner/src/radio_planner.c"
        "lbm_lib/smtc_modem_core/radio_planner/src/radio_planner_hal.c"
        
        # Cryptographic functions
        "lbm_lib/smtc_modem_core/smtc_modem_crypto/smtc_modem_crypto.c"
        "lbm_lib/smtc_modem_core/smtc_modem_crypto/soft_secure_element.c"
        "lbm_lib/smtc_modem_core/smtc_modem_crypto/smtc_secure_element.c"
        
        # Regional parameters
        "lbm_lib/smtc_modem_core/smtc_real/src/smtc_real.c"
        "lbm_lib/smtc_modem_core/smtc_real/src/region_as_923.c"
        "lbm_lib/smtc_modem_core/smtc_real/src/region_au_915.c"
        "lbm_lib/smtc_modem_core/smtc_real/src/region_cn_470.c"
        "lbm_lib/smtc_modem_core/smtc_real/src/region_cn_470_rp_1_0.c"
        "lbm_lib/smtc_modem_core/smtc_real/src/region_eu_868.c"
        "lbm_lib/smtc_modem_core/smtc_real/src/region_in_865.c"
        "lbm_lib/smtc_modem_core/smtc_real/src/region_kr_920.c"
        "lbm_lib/smtc_modem_core/smtc_real/src/region_ru_864.c"
        "lbm_lib/smtc_modem_core/smtc_real/src/region_us_915.c"
        "lbm_lib/smtc_modem_core/smtc_real/src/region_ww2g4.c"
        
        # Services (optional features)
        "lbm_lib/smtc_modem_core/modem_services/fifo_ctrl.c"
        
        # Radio drivers (conditional based on selected radio)
        "lbm_lib/smtc_modem_core/radio_drivers/sx126x_driver/src/sx126x.c"
        "lbm_lib/smtc_modem_core/radio_drivers/sx127x_driver/src/sx127x.c"
        "lbm_lib/smtc_modem_core/radio_drivers/lr11xx_driver/src/lr11xx_bootloader.c"
        "lbm_lib/smtc_modem_core/radio_drivers/lr11xx_driver/src/lr11xx_hal.c"
        "lbm_lib/smtc_modem_core/radio_drivers/lr11xx_driver/src/lr11xx_radio.c"
        "lbm_lib/smtc_modem_core/radio_drivers/lr11xx_driver/src/lr11xx_regmem.c"
        "lbm_lib/smtc_modem_core/radio_drivers/lr11xx_driver/src/lr11xx_system.c"
        "lbm_lib/smtc_modem_core/radio_drivers/sx128x_driver/src/sx128x.c"
        
        # ESP32 HAL implementation
        "esp32_hal/smtc_modem_hal_esp32.c"
        "esp32_hal/radio_hal_esp32.c"
        "esp32_hal/ral_sx126x_bsp_esp32.c"
        "esp32_hal/ral_sx127x_bsp_esp32.c"
        
    INCLUDE_DIRS 
        "lbm_lib/smtc_modem_api"
        "lbm_lib/smtc_modem_hal"
        "lbm_lib/smtc_modem_core"
        "lbm_lib/smtc_modem_core/modem_utilities"
        "lbm_lib/smtc_modem_core/modem_supervisor"
        "lbm_lib/smtc_modem_core/lorawan_stack"
        "lbm_lib/smtc_modem_core/lr1mac/src"
        "lbm_lib/smtc_modem_core/lr1mac/src/services"
        "lbm_lib/smtc_modem_core/smtc_ral/src"
        "lbm_lib/smtc_modem_core/smtc_ralf/src"
        "lbm_lib/smtc_modem_core/radio_planner/src"
        "lbm_lib/smtc_modem_core/smtc_modem_crypto"
        "lbm_lib/smtc_modem_core/smtc_real/src"
        "lbm_lib/smtc_modem_core/modem_services"
        "lbm_lib/smtc_modem_core/radio_drivers/sx126x_driver/src"
        "lbm_lib/smtc_modem_core/radio_drivers/sx127x_driver/src"
        "lbm_lib/smtc_modem_core/radio_drivers/lr11xx_driver/src"
        "lbm_lib/smtc_modem_core/radio_drivers/sx128x_driver/src"
        "esp32_hal"
        "."
        
    REQUIRES 
        "driver"
        "esp_timer"
        "nvs_flash" 
        "spi_flash"
        "freertos"
        "esp_hw_support"
        "esp_system"
        "log"
        "heap"
        "mbedtls"
)

# Add compile definitions based on configuration
target_compile_definitions(${COMPONENT_LIB} PRIVATE
# Add compile definitions based on configuration
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    # HAL specific defines
    HAL_DBG_TRACE=1
    MODEM_HAL_DBG_TRACE=1
    
    # ESP32 specific
    ESP_PLATFORM=1
)

# Radio type conditional definitions
if(CONFIG_LBM_RADIO_SX126X)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X=1)
endif()

if(CONFIG_LBM_RADIO_SX127X)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X=1)
endif()

if(CONFIG_LBM_RADIO_LR11XX)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE LR11XX=1)
endif()

if(CONFIG_LBM_RADIO_SX128X)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX128X=1)
endif()

# Regional parameters conditional definitions
if(CONFIG_LBM_REGION_EU_868)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_EU_868=1)
endif()

if(CONFIG_LBM_REGION_US_915)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_US_915=1)
endif()

if(CONFIG_LBM_REGION_AS_923)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_AS_923=1)
endif()

if(CONFIG_LBM_REGION_AU_915)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_AU_915=1)
endif()

if(CONFIG_LBM_REGION_CN_470)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_CN_470=1)
endif()

if(CONFIG_LBM_REGION_IN_865)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_IN_865=1)
endif()

if(CONFIG_LBM_REGION_KR_920)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_KR_920=1)
endif()

if(CONFIG_LBM_REGION_RU_864)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_RU_864=1)
endif()

# Optional features conditional definitions
if(CONFIG_LBM_STREAM)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_SMTC_STREAM=1)
endif()

if(CONFIG_LBM_DEVICE_MANAGEMENT)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_SMTC_CLOUD_DEVICE_MANAGEMENT=1)
endif()

if(CONFIG_LBM_ALMANAC)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_ALMANAC=1)
endif()

if(CONFIG_LBM_GEOLOCATION)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_SMTC_GEOLOCATION=1)
endif()

if(CONFIG_LBM_RELAY_TX)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_SMTC_RELAY_TX=1)
endif()

if(CONFIG_LBM_RELAY_RX)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_SMTC_RELAY_RX=1)
endif()

if(CONFIG_LBM_CSMA)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ENABLE_CSMA=1)
endif()

if(CONFIG_LBM_LBT)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_LBT=1)
endif()

# SX126X specific configuration options
if(CONFIG_LBM_SX126X_USE_LDO_REGULATOR)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_USE_LDO_REGULATOR=1)
endif()

if(CONFIG_LBM_SX126X_USE_DIO2_RF_SWITCH)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_USE_DIO2_RF_SWITCH=1)
endif()

if(CONFIG_LBM_SX126X_USE_TCXO)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_USE_TCXO=1)
endif()

if(CONFIG_LBM_SX126X_RX_BOOST_ENABLE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_RX_BOOST_ENABLE=1)
endif()

# SX126X TCXO voltage definitions
if(CONFIG_LBM_SX126X_TCXO_1_6V)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_TCXO_VOLTAGE=0)
elseif(CONFIG_LBM_SX126X_TCXO_1_7V)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_TCXO_VOLTAGE=1)
elseif(CONFIG_LBM_SX126X_TCXO_1_8V)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_TCXO_VOLTAGE=2)
elseif(CONFIG_LBM_SX126X_TCXO_2_2V)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_TCXO_VOLTAGE=3)
elseif(CONFIG_LBM_SX126X_TCXO_2_4V)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_TCXO_VOLTAGE=4)
elseif(CONFIG_LBM_SX126X_TCXO_2_7V)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_TCXO_VOLTAGE=5)
elseif(CONFIG_LBM_SX126X_TCXO_3_0V)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_TCXO_VOLTAGE=6)
elseif(CONFIG_LBM_SX126X_TCXO_3_3V)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_TCXO_VOLTAGE=7)
endif()

# SX126X PA ramp time definitions
if(CONFIG_LBM_SX126X_RAMP_10_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_PA_RAMP_TIME=0)
elseif(CONFIG_LBM_SX126X_RAMP_20_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_PA_RAMP_TIME=1)
elseif(CONFIG_LBM_SX126X_RAMP_40_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_PA_RAMP_TIME=2)
elseif(CONFIG_LBM_SX126X_RAMP_80_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_PA_RAMP_TIME=3)
elseif(CONFIG_LBM_SX126X_RAMP_200_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_PA_RAMP_TIME=4)
elseif(CONFIG_LBM_SX126X_RAMP_800_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_PA_RAMP_TIME=5)
elseif(CONFIG_LBM_SX126X_RAMP_1700_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_PA_RAMP_TIME=6)
elseif(CONFIG_LBM_SX126X_RAMP_3400_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X_PA_RAMP_TIME=7)
endif()

# SX127X specific configuration options
if(CONFIG_LBM_SX127X_BOARD_TYPE_MAS)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X_BOARD_TYPE_MAS=1)
endif()

# SX127X PA ramp time definitions
if(CONFIG_LBM_SX127X_RAMP_3400_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X_PA_RAMP_TIME=0)
elseif(CONFIG_LBM_SX127X_RAMP_2000_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X_PA_RAMP_TIME=1)
elseif(CONFIG_LBM_SX127X_RAMP_1000_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X_PA_RAMP_TIME=2)
elseif(CONFIG_LBM_SX127X_RAMP_500_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X_PA_RAMP_TIME=3)
elseif(CONFIG_LBM_SX127X_RAMP_250_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X_PA_RAMP_TIME=4)
elseif(CONFIG_LBM_SX127X_RAMP_125_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X_PA_RAMP_TIME=5)
elseif(CONFIG_LBM_SX127X_RAMP_100_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X_PA_RAMP_TIME=6)
elseif(CONFIG_LBM_SX127X_RAMP_62_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X_PA_RAMP_TIME=7)
elseif(CONFIG_LBM_SX127X_RAMP_50_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X_PA_RAMP_TIME=8)
elseif(CONFIG_LBM_SX127X_RAMP_40_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X_PA_RAMP_TIME=9)
elseif(CONFIG_LBM_SX127X_RAMP_31_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X_PA_RAMP_TIME=10)
elseif(CONFIG_LBM_SX127X_RAMP_25_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X_PA_RAMP_TIME=11)
elseif(CONFIG_LBM_SX127X_RAMP_20_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X_PA_RAMP_TIME=12)
elseif(CONFIG_LBM_SX127X_RAMP_15_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X_PA_RAMP_TIME=13)
elseif(CONFIG_LBM_SX127X_RAMP_12_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X_PA_RAMP_TIME=14)
elseif(CONFIG_LBM_SX127X_RAMP_10_US)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X_PA_RAMP_TIME=15)
endif()

# Add compiler flags for optimization and warnings
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-variable
    -Os
)
