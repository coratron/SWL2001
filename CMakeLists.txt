# CMakeLists.txt for LoRa Basics Modem SX127x ESP-IDF Component

# Recursively collect all C source files (excluding examples)
file(GLOB_RECURSE ALL_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/esp32_hal/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/*.c"
)

# Note: We need to create our own ESP32-specific smtc_modem_hal.c file
# The one in lbm_examples contains STM32-specific code

# Initialize component sources with all found files
set(COMPONENT_SRCS ${ALL_SOURCES})

# Define files to exclude based on various conditions
set(EXCLUDE_FILES)

# Exclude example application files (we only want HAL and lib)
file(GLOB_RECURSE APPLICATION_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_applications/*.c"
)
list(APPEND EXCLUDE_FILES ${APPLICATION_FILES})

# Exclude example HAL files (they use incorrect API calls)
file(GLOB_RECURSE EXAMPLE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/esp32_hal/example/*.c"
)
list(APPEND EXCLUDE_FILES ${EXAMPLE_FILES})

# Exclude relay TX/RX files (they require ADD_RELAY_TX/ADD_RELAY_RX flags)
file(GLOB_RECURSE RELAY_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/modem_services/relay_tx/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/modem_services/relay_rx/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/modem_services/relay_service/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/relay/*.c"
)
list(APPEND EXCLUDE_FILES ${RELAY_FILES})

# Filter out non-SX127x radio drivers
file(GLOB_RECURSE NON_SX127X_RADIOS
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/radio_drivers/sx126x_driver/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/radio_drivers/sx128x_driver/*.c" 
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/radio_drivers/lr11xx_driver/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/smtc_ral/src/ral_sx126x.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/smtc_ral/src/ral_sx128x.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/smtc_ral/src/ral_lr11xx.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/smtc_ral/src/ral_llcc68.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/smtc_ralf/src/ralf_sx126x.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/smtc_ralf/src/ralf_sx128x.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/smtc_ralf/src/ralf_lr11xx.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/smtc_ralf/src/ralf_llcc68.c"
)
list(APPEND EXCLUDE_FILES ${NON_SX127X_RADIOS})

# Filter out non-SX127x crypto engines
file(GLOB_RECURSE NON_SX127X_CRYPTO
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/smtc_modem_crypto/lr11xx_crypto_engine/*.c"
)
list(APPEND EXCLUDE_FILES ${NON_SX127X_CRYPTO})

# Conditional exclusion: regions
set(ALL_REGIONS
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_eu_868.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_us_915.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_as_923.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_au_915.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_cn_470.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_in_865.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_kr_920.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_ru_864.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_ww2g4.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_cn_470_rp_1_0.c"
)

# Exclude all regions first, then include only enabled ones
list(APPEND EXCLUDE_FILES ${ALL_REGIONS})

# Include enabled regions back
if(CONFIG_LBM_REGION_EU868)
    list(REMOVE_ITEM EXCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_eu_868.c")
endif()
if(CONFIG_LBM_REGION_US915)
    list(REMOVE_ITEM EXCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_us_915.c")
endif()
if(CONFIG_LBM_REGION_AS923)
    list(REMOVE_ITEM EXCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_as_923.c")
endif()
if(CONFIG_LBM_REGION_AU915)
    list(REMOVE_ITEM EXCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_au_915.c")
endif()
if(CONFIG_LBM_REGION_CN470)
    list(REMOVE_ITEM EXCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_cn_470.c")
endif()
if(CONFIG_LBM_REGION_IN865)
    list(REMOVE_ITEM EXCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_in_865.c")
endif()
if(CONFIG_LBM_REGION_KR920)
    list(REMOVE_ITEM EXCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_kr_920.c")
endif()
if(CONFIG_LBM_REGION_RU864)
    list(REMOVE_ITEM EXCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_ru_864.c")
endif()
if(CONFIG_LBM_REGION_WW2G4)
    list(REMOVE_ITEM EXCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_ww2g4.c")
endif()
if(CONFIG_LBM_REGION_CN470_RP_1_0)
    list(REMOVE_ITEM EXCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_cn_470_rp_1_0.c")
endif()

# Conditional exclusion: Class B files
set(CLASS_B_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lorawan_manager/lorawan_class_b_management.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/lr1mac_class_b/smtc_beacon_sniff.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/lr1mac_class_b/smtc_ping_slot.c"
)

if(NOT CONFIG_LBM_ENABLE_CLASS_B)
    list(APPEND EXCLUDE_FILES ${CLASS_B_FILES})
endif()

# Conditional exclusion: Geolocation services
file(GLOB_RECURSE GEOLOCATION_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/geolocation_services/*.c"
)
if(NOT CONFIG_LBM_ENABLE_GEOLOCATION_SERVICES)
    list(APPEND EXCLUDE_FILES ${GEOLOCATION_FILES})
endif()

# Conditional exclusion: Store and Forward
file(GLOB_RECURSE STORE_FORWARD_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/modem_services/store_and_forward_flash/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/modem_services/store_and_forward/*.c"
)
if(NOT CONFIG_LBM_ENABLE_STORE_AND_FORWARD)
    list(APPEND EXCLUDE_FILES ${STORE_FORWARD_FILES})
endif()

# Conditional exclusion: Almanac services
file(GLOB_RECURSE ALMANAC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/modem_services/almanac_packages/*.c"
)
if(NOT CONFIG_LBM_ENABLE_ALMANAC_SERVICES)
    list(APPEND EXCLUDE_FILES ${ALMANAC_FILES})
endif()

# Conditional exclusion: Multicast
file(GLOB_RECURSE MULTICAST_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lorawan_packages/remote_multicast_setup/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lorawan_packages/fragmented_data_block_transport/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lorawan_packages/multi_package_access/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lr1mac/src/services/smtc_multicast/*.c"
)
if(NOT CONFIG_LBM_ENABLE_MULTICAST)
    list(APPEND EXCLUDE_FILES ${MULTICAST_FILES})
endif()

# Conditional exclusion: Certification
set(CERT_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lorawan_packages/lorawan_certification/lorawan_certification.c"
)
if(NOT CONFIG_LBM_ENABLE_CERTIFICATION)
    list(APPEND EXCLUDE_FILES ${CERT_FILES})
endif()

# Conditional exclusion: Firmware Management Protocol (FMP)
file(GLOB_RECURSE FMP_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/smtc_modem_core/lorawan_packages/firmware_management_protocol/*.c"
)
if(NOT CONFIG_LBM_ENABLE_FUOTA)
    list(APPEND EXCLUDE_FILES ${FMP_FILES})
endif()

# Remove excluded files from component sources
foreach(exclude_file ${EXCLUDE_FILES})
    list(REMOVE_ITEM COMPONENT_SRCS ${exclude_file})
endforeach()

# Filter out any files that don't actually exist (in case glob patterns didn't match)
set(FINAL_SOURCES)
foreach(src_file ${COMPONENT_SRCS})
    if(EXISTS ${src_file})
        list(APPEND FINAL_SOURCES ${src_file})
    endif()
endforeach()
set(COMPONENT_SRCS ${FINAL_SOURCES})

# Debug output (can be removed in production)  
list(LENGTH COMPONENT_SRCS NUM_SOURCES)
message(STATUS "LBM Component: Found ${NUM_SOURCES} source files")

# Remove duplicates
list(REMOVE_DUPLICATES COMPONENT_SRCS)

# Dynamically discover include directories
file(GLOB_RECURSE ALL_HEADERS 
    "${CMAKE_CURRENT_SOURCE_DIR}/esp32_hal/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/lbm_lib/*.h"
)

# Extract unique directories from header files
set(INCLUDE_DIRS)
foreach(header ${ALL_HEADERS})
    get_filename_component(header_dir ${header} DIRECTORY)
    list(APPEND INCLUDE_DIRS ${header_dir})
endforeach()

# Remove duplicates and sort
list(REMOVE_DUPLICATES INCLUDE_DIRS)
list(SORT INCLUDE_DIRS)

# Convert absolute paths to relative paths for cleaner output
set(COMPONENT_ADD_INCLUDEDIRS)
foreach(inc_dir ${INCLUDE_DIRS})
    file(RELATIVE_PATH rel_path ${CMAKE_CURRENT_SOURCE_DIR} ${inc_dir})
    list(APPEND COMPONENT_ADD_INCLUDEDIRS ${rel_path})
endforeach()

# Add essential directories that may not have headers but are needed
list(APPEND COMPONENT_ADD_INCLUDEDIRS
    "esp32_hal"
    "esp32_hal/include"
    "lbm_lib"
    "lbm_lib/smtc_modem_core"
)

# Private includes
set(COMPONENT_PRIV_INCLUDEDIRS
    "esp32_hal/private_include"
    "lbm_lib/smtc_modem_core/logging"
)

# Component dependencies
set(COMPONENT_REQUIRES
    driver
    esp_timer
    nvs_flash
    log
)

# Register component with compile definitions
idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS}
    PRIV_INCLUDE_DIRS ${COMPONENT_PRIV_INCLUDEDIRS}
    REQUIRES ${COMPONENT_REQUIRES}
)

# Component-specific compile definitions
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    SX127X
    LBM_SX127X_VERSION_MAJOR=1
    LBM_SX127X_VERSION_MINOR=0
    LBM_SX127X_VERSION_PATCH=0
    # Essential LBM definitions
    NUMBER_OF_STACKS=${CONFIG_LBM_NUMBER_OF_STACKS}
    LBM_NB_OF_STACK=${CONFIG_LBM_NUMBER_OF_STACKS}
    MODEM_HAL_FEATURE_OFF=0
)

# Conditional compile definitions
if(CONFIG_LBM_SX127X_RADIO_SX1272)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX1272)
endif()

if(CONFIG_LBM_SX127X_RADIO_SX1276)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SX1276)
endif()

if(CONFIG_LBM_SX127X_ENABLE_LORA)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE LBM_SX127X_LORA_ENABLED)
endif()

if(CONFIG_LBM_SX127X_ENABLE_GFSK)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE LBM_SX127X_GFSK_ENABLED)
endif()

if(CONFIG_LBM_SX127X_ENABLE_OOK)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE LBM_SX127X_OOK_ENABLED)
endif()

if(CONFIG_LBM_SX127X_DEBUG_SPI)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE LBM_SX127X_DEBUG_SPI)
endif()

if(CONFIG_LBM_SX127X_DEBUG_INTERRUPTS)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE LBM_SX127X_DEBUG_INTERRUPTS)
endif()

if(CONFIG_LBM_SX127X_INTERRUPT_STATS)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE LBM_SX127X_INTERRUPT_STATS)
endif()

if(CONFIG_LBM_SX127X_ENABLE_REGISTER_DUMP)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE LBM_SX127X_ENABLE_REGISTER_DUMP)
endif()

if(CONFIG_LBM_SX127X_STATIC_ALLOCATION)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE LBM_SX127X_STATIC_ALLOCATION)
endif()

# LoRaWAN Region Configuration
if(CONFIG_LBM_REGION_EU868)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_EU_868=1)
endif()

if(CONFIG_LBM_REGION_US915)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_US_915=1)
endif()

if(CONFIG_LBM_REGION_AS923)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_AS_923=1)
endif()

if(CONFIG_LBM_REGION_AU915)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_AU_915=1)
endif()

if(CONFIG_LBM_REGION_CN470)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_CN_470=1)
endif()

if(CONFIG_LBM_REGION_IN865)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_IN_865=1)
endif()

if(CONFIG_LBM_REGION_KR920)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_KR_920=1)
endif()

if(CONFIG_LBM_REGION_RU864)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_RU_864=1)
endif()

if(CONFIG_LBM_REGION_WW2G4)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_WW2G4=1)
endif()

if(CONFIG_LBM_REGION_CN470_RP_1_0)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE REGION_CN_470_RP_1_0=1)
endif()

# LoRaWAN Version Configuration
if(CONFIG_LBM_LORAWAN_VERSION_RP2_101)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE RP2_101)
endif()

if(CONFIG_LBM_LORAWAN_VERSION_RP2_103)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE RP2_103)
endif()

if(CONFIG_LBM_LORAWAN_VERSION_RP2_104)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE RP2_104)
endif()

# Class Support Configuration
if(CONFIG_LBM_ENABLE_CLASS_B)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_CLASS_B)
endif()

if(CONFIG_LBM_ENABLE_CLASS_C)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_CLASS_C)
endif()

if(CONFIG_LBM_ENABLE_MULTICAST)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE 
        ADD_MULTICAST
        SMTC_MULTICAST
    )
endif()

if(CONFIG_LBM_ENABLE_FUOTA)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_FUOTA)
endif()

# Security Configuration
if(CONFIG_LBM_SOFT_SE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SMTC_SE_USE_SOFT_SE)
endif()

if(CONFIG_LBM_HARD_SE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SMTC_SE_USE_HARD_SE)
endif()

if(CONFIG_LBM_ENABLE_CERTIFICATION)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_CERTIFICATION)
endif()

if(CONFIG_LBM_ENABLE_STORE_AND_FORWARD)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_STORE_AND_FORWARD)
endif()

# Debug and Tracing Configuration
if(CONFIG_LBM_ENABLE_HAL_DBG_TRACE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE 
        ADD_SMTC_HAL_DBG_TRACE
        SMTC_MODEM_HAL_DBG_TRACE_COLOR
    )
endif()

if(CONFIG_LBM_ENABLE_MODEM_TRACE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_MODEM_TRACE)
endif()

if(CONFIG_LBM_ENABLE_RADIO_TRACE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_RADIO_TRACE)
endif()

if(CONFIG_LBM_ENABLE_LR1MAC_TRACE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_LR1MAC_TRACE)
endif()

if(CONFIG_LBM_ENABLE_CRYPTO_TRACE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_CRYPTO_TRACE)
endif()

# Power Management Configuration
if(CONFIG_LBM_ENABLE_LOW_POWER_MODE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_LOW_POWER_MODE)
endif()

if(CONFIG_LBM_ENABLE_POWER_OPTIMIZATION)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_POWER_OPTIMIZATION)
endif()

# Advanced Features Configuration
if(CONFIG_LBM_ENABLE_STREAM)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_STREAM)
endif()

if(CONFIG_LBM_ENABLE_LFU)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_LFU)
endif()

if(CONFIG_LBM_ENABLE_DM_INFO_SERVICES)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_DM_INFO_SERVICES)
endif()

if(CONFIG_LBM_ENABLE_ALMANAC_SERVICES)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_ALMANAC_SERVICES)
endif()

if(CONFIG_LBM_CUSTOM_RADIO_PLANNER)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CUSTOM_RADIO_PLANNER)
endif()

if(CONFIG_LBM_ENABLE_CSMA)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_CSMA)
endif()

if(CONFIG_LBM_ENABLE_LBT)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ADD_LBT)
endif()

# Configuration values that need to be passed as numbers
target_compile_definitions(${COMPONENT_LIB} PRIVATE 
    MAX_NB_OF_RX_WINDOW=${CONFIG_LBM_MAX_NB_OF_RX_WINDOW}
    CRYSTAL_ERROR_PPM=${CONFIG_LBM_CRYSTAL_ERROR_PPM}
    TRACE_LEVEL=${CONFIG_LBM_TRACE_LEVEL}
)

# Compiler flags for better code quality
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-but-set-parameter
    -Wno-unused-variable
    -Wno-missing-field-initializers
    -Wno-unused-function
    -Wno-incompatible-pointer-types
    -Wno-array-parameter
    -Wno-implicit-function-declaration
)
