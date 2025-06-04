# Collect all source files recursively
file(GLOB_RECURSE LBM_SOURCES
    "lbm_lib/*.c"
    "esp32_hal/*.c"
)

# Remove geolocation service files if geolocation is disabled
if(NOT CONFIG_LBM_GEOLOCATION)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*geolocation_services.*")
endif()

# Remove stream service files if stream is disabled
if(NOT CONFIG_LBM_STREAM)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*stream_packages.*")
endif()

# Remove device management files if device management is disabled
if(NOT CONFIG_LBM_DEVICE_MANAGEMENT)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*cloud_dm_package.*")
endif()

# Remove Class B files if Class B is disabled
if(NOT CONFIG_LBM_CLASS_B)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*lorawan_class_b_management.*")
endif()

# Remove FUOTA-related files if FUOTA is disabled
if(NOT CONFIG_LBM_FUOTA)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*lorawan_packages/firmware_management_protocol.*")
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*lorawan_packages/fragmented_data_block_transport.*")
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*lorawan_packages/multi_package_access.*")
endif()

# Remove multicast files if multicast is disabled
if(NOT CONFIG_LBM_MULTICAST)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*lorawan_packages/remote_multicast_setup.*")
endif()

# Remove certification files if certification is disabled
if(NOT CONFIG_LBM_CERTIFICATION)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*lorawan_packages/lorawan_certification.*")
endif()

# Remove relay files based on configuration
if(NOT CONFIG_LBM_RELAY_TX)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*relay/relay_tx/.*")
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*relay_service/lorawan_relay_tx_service.*")
endif()
if(NOT CONFIG_LBM_RELAY_RX)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*relay/relay_rx/.*")
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*relay_service/lorawan_relay_rx_service.*")
endif()

# Remove regional files based on configuration
if(NOT CONFIG_LBM_REGION_EU_868)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*region_eu_868.*")
endif()

if(NOT CONFIG_LBM_REGION_US_915)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*region_us_915.*")
endif()

if(NOT CONFIG_LBM_REGION_AS_923)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*region_as_923.*")
endif()

if(NOT CONFIG_LBM_REGION_AU_915)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*region_au_915.*")
endif()

if(NOT CONFIG_LBM_REGION_CN_470)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*region_cn_470.*")
endif()

if(NOT CONFIG_LBM_REGION_IN_865)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*region_in_865.*")
endif()

if(NOT CONFIG_LBM_REGION_KR_920)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*region_kr_920.*")
endif()

if(NOT CONFIG_LBM_REGION_RU_864)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*region_ru_864.*")
endif()

if(NOT CONFIG_LBM_REGION_WW2G4)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*region_ww2g4.*")
endif()

# Remove almanac files if not enabled
if(NOT CONFIG_LBM_ALMANAC)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*almanac.*")
endif()

# Remove radio-specific files based on selected radio
if(NOT CONFIG_LBM_RADIO_LR11XX)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*lr11xx.*")
endif()

if(NOT CONFIG_LBM_RADIO_SX126X)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*sx126x.*")
endif()

if(NOT CONFIG_LBM_RADIO_SX127X)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*sx127x.*")
endif()

if(NOT CONFIG_LBM_RADIO_SX128X)
    list(FILTER LBM_SOURCES EXCLUDE REGEX ".*sx128x.*")
endif()

# Always exclude LLCC68 files since we don't have that radio driver
list(FILTER LBM_SOURCES EXCLUDE REGEX ".*llcc68.*")

# Collect all header directories recursively
file(GLOB_RECURSE LBM_HEADERS
    "lbm_lib/*.h"
    "esp32_hal/*.h"
)

# Extract unique directories from header file paths
set(LBM_INCLUDE_DIRS)
foreach(header ${LBM_HEADERS})
    get_filename_component(header_dir ${header} DIRECTORY)
    list(APPEND LBM_INCLUDE_DIRS ${header_dir})
endforeach()
list(REMOVE_DUPLICATES LBM_INCLUDE_DIRS)

# Make paths relative to component directory
set(LBM_INCLUDE_DIRS_RELATIVE)
foreach(include_dir ${LBM_INCLUDE_DIRS})
    file(RELATIVE_PATH rel_path ${CMAKE_CURRENT_SOURCE_DIR} ${include_dir})
    list(APPEND LBM_INCLUDE_DIRS_RELATIVE ${rel_path})
endforeach()

# Add current directory and common include paths
list(APPEND LBM_INCLUDE_DIRS_RELATIVE
    "."
    "lbm_lib"
)

idf_component_register(
    SRCS ${LBM_SOURCES}
    INCLUDE_DIRS ${LBM_INCLUDE_DIRS_RELATIVE}
        
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
        "soc"
        "hal"
        "esp_common"
)

# Add compile definitions based on configuration
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    # HAL specific defines
    HAL_DBG_TRACE=1
    MODEM_HAL_DBG_TRACE=1
    
    # ESP32 specific
    ESP_PLATFORM=1
    
    # Stack configuration
    NUMBER_OF_STACKS=1
)

# Radio type conditional definitions
if(CONFIG_LBM_RADIO_SX126X)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX126X=1)
endif()

if(CONFIG_LBM_RADIO_SX127X)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX127X=1)
    
    # Define specific SX127X chip type
    if(CONFIG_LBM_SX127X_CHIP_SX1272)
        target_compile_definitions(${COMPONENT_LIB} PRIVATE SX1272=1)
    elseif(CONFIG_LBM_SX127X_CHIP_SX1276)
        target_compile_definitions(${COMPONENT_LIB} PRIVATE SX1276=1)
    endif()
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

if(CONFIG_LBM_REGION_WW2G4)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_WW2G4=1)
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
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_LBM_GEOLOCATION=1)
endif()

if(CONFIG_LBM_RELAY_TX)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_RELAY_TX=1)
endif()

if(CONFIG_LBM_RELAY_RX)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_RELAY_RX=1)
endif()

if(CONFIG_LBM_CSMA)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ENABLE_CSMA=1)
endif()

if(CONFIG_LBM_LBT)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_LBT=1)
endif()

if(CONFIG_LBM_CLASS_B)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_CLASS_B=1)
endif()

if(CONFIG_LBM_CLASS_C)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_CLASS_C=1)
endif()

if(CONFIG_LBM_MULTICAST)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SMTC_MULTICAST=1)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SMTC_MULTICAST_MAX_SESSIONS=${CONFIG_LBM_MULTICAST_MAX_SESSIONS})
endif()

if(CONFIG_LBM_FUOTA)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ENABLE_FUOTA_FULL=1)
    if(CONFIG_LBM_FUOTA_FMP)
        target_compile_definitions(${COMPONENT_LIB} PRIVATE ENABLE_FUOTA_FMP=1)
    endif()
    if(CONFIG_LBM_FUOTA_MPA)
        target_compile_definitions(${COMPONENT_LIB} PRIVATE ENABLE_FUOTA_MPA=1)
    endif()
endif()

if(CONFIG_LBM_CERTIFICATION)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ENABLE_LORAWAN_CERTIFICATION=1)
endif()

if(CONFIG_LBM_TEST_MODE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ENABLE_TEST_MODE=1)
endif()

# LoRaWAN Regional Parameters version conditional definitions
if(CONFIG_LBM_RP2_103)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE RP2_103=1)
endif()

if(CONFIG_LBM_RP2_104)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE RP2_104=1)
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

# Memory and timing configuration
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    MODEM_MIN_RANDOM_DELAY_MS=${CONFIG_LBM_MIN_RANDOM_DELAY_MS}
    MODEM_MAX_RANDOM_DELAY_MS=${CONFIG_LBM_MAX_RANDOM_DELAY_MS}
    MODEM_INITIAL_JOIN_DELAY_S=${CONFIG_LBM_INITIAL_JOIN_DELAY_S}
    MODEM_MAX_JOIN_DELAY_S=${CONFIG_LBM_MAX_JOIN_DELAY_S}
    MODEM_WATCHDOG_TIMEOUT_MS=${CONFIG_LBM_WATCHDOG_TIMEOUT_MS}
)

# Add compiler flags for optimization and warnings
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-variable
    -Os
)
