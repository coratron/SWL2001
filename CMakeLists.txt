# CMakeLists.txt for LoRa Basics Modem SX127x ESP-IDF Component

# Component sources
set(COMPONENT_SRCS
    "esp32_hal/sx127x_hal_esp.c"
    "esp32_hal/sx127x_esp_wrapper.c"
    "esp32_hal/sx127x_esp_debug.c"
)

# Core LBM sources
list(APPEND COMPONENT_SRCS
    "lbm_lib/smtc_modem_core/radio_drivers/sx127x_driver/src/sx127x.c"
    "lbm_lib/smtc_modem_core/smtc_ral/src/ral_sx127x.c"
    "lbm_lib/smtc_modem_core/smtc_ralf/src/ralf_sx127x.c"
    "lbm_lib/smtc_modem_core/smtc_modem_crypto/soft_secure_element/aes.c"
    "lbm_lib/smtc_modem_core/smtc_modem_crypto/soft_secure_element/cmac.c"
    "lbm_lib/smtc_modem_core/smtc_modem_crypto/soft_secure_element/soft_se.c"
)

# Conditional compilation based on Kconfig
if(CONFIG_LBM_SX127X_ENABLE_SELF_TEST)
    list(APPEND COMPONENT_SRCS "esp32_hal/sx127x_esp_test.c")
endif()

# Component includes
set(COMPONENT_ADD_INCLUDEDIRS
    "esp32_hal/include"
    "lbm_lib/smtc_modem_core/radio_drivers/sx127x_driver/src"
    "lbm_lib/smtc_modem_core/smtc_ral/src"
    "lbm_lib/smtc_modem_core/smtc_ralf/src"
    "lbm_lib/smtc_modem_core/smtc_modem_crypto/soft_secure_element"
    "lbm_lib/smtc_modem_hal"
    "lbm_lib/smtc_modem_api"
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

# Register component
register_component()

# Compile definitions based on Kconfig
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    -DSX127X
)

if(CONFIG_LBM_SX127X_RADIO_SX1272)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE -DSX1272)
endif()

if(CONFIG_LBM_SX127X_RADIO_SX1276)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE -DSX1276)
endif()

if(CONFIG_LBM_SX127X_ENABLE_LORA)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE -DLBM_SX127X_LORA_ENABLED)
endif()

if(CONFIG_LBM_SX127X_ENABLE_GFSK)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE -DLBM_SX127X_GFSK_ENABLED)
endif()

if(CONFIG_LBM_SX127X_ENABLE_OOK)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE -DLBM_SX127X_OOK_ENABLED)
endif()

if(CONFIG_LBM_SX127X_DEBUG_SPI)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE -DLBM_SX127X_DEBUG_SPI)
endif()

if(CONFIG_LBM_SX127X_DEBUG_INTERRUPTS)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE -DLBM_SX127X_DEBUG_INTERRUPTS)
endif()

if(CONFIG_LBM_SX127X_INTERRUPT_STATS)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE -DLBM_SX127X_INTERRUPT_STATS)
endif()

if(CONFIG_LBM_SX127X_ENABLE_REGISTER_DUMP)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE -DLBM_SX127X_ENABLE_REGISTER_DUMP)
endif()

if(CONFIG_LBM_SX127X_STATIC_ALLOCATION)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE -DLBM_SX127X_STATIC_ALLOCATION)
endif()

# Compiler flags
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
)

# Version information
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    LBM_SX127X_VERSION_MAJOR=1
    LBM_SX127X_VERSION_MINOR=0
    LBM_SX127X_VERSION_PATCH=0
)
