#!/bin/bash

# LoRa Basics Modem ESP32 Component Integration Test
# This script helps validate the ESP-IDF component setup

set -e

echo "LoRa Basics Modem ESP32 Component Integration Test"
echo "=================================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in an ESP-IDF environment
check_esp_idf() {
    print_status "Checking ESP-IDF environment..."
    
    if [ -z "$IDF_PATH" ]; then
        print_error "IDF_PATH is not set. Please source the ESP-IDF environment first:"
        echo "  . \$IDF_PATH/export.sh"
        exit 1
    fi
    
    if ! command -v idf.py &> /dev/null; then
        print_error "idf.py not found. Please ensure ESP-IDF is properly installed and sourced."
        exit 1
    fi
    
    print_status "ESP-IDF environment: OK"
    echo "  IDF_PATH: $IDF_PATH"
    echo "  IDF version: $(idf.py --version 2>/dev/null || echo 'Unknown')"
}

# Check if this is a valid ESP-IDF project
check_project_structure() {
    print_status "Checking project structure..."
    
    if [ ! -f "CMakeLists.txt" ]; then
        print_error "No CMakeLists.txt found. Are you in an ESP-IDF project directory?"
        exit 1
    fi
    
    if [ ! -d "components" ]; then
        print_warning "No components directory found. Creating one..."
        mkdir -p components
    fi
    
    print_status "Project structure: OK"
}

# Check if LBM component is properly integrated
check_lbm_component() {
    print_status "Checking LBM component integration..."
    
    LBM_COMPONENT_PATH=""
    
    # Check if LBM component exists in components directory
    if [ -d "components/lbm" ]; then
        LBM_COMPONENT_PATH="components/lbm"
    elif [ -d "components/LoRa-Basics-Modem" ]; then
        LBM_COMPONENT_PATH="components/LoRa-Basics-Modem"
    else
        print_error "LBM component not found in components directory."
        echo "Please copy the LBM component to:"
        echo "  ./components/lbm/"
        echo "or create a symlink:"
        echo "  ln -s /path/to/lbm/component ./components/lbm"
        exit 1
    fi
    
    # Check if component has required files
    if [ ! -f "$LBM_COMPONENT_PATH/CMakeLists.txt" ]; then
        print_error "LBM component CMakeLists.txt not found"
        exit 1
    fi
    
    if [ ! -f "$LBM_COMPONENT_PATH/Kconfig" ]; then
        print_error "LBM component Kconfig not found"
        exit 1
    fi
    
    print_status "LBM component: OK"
    echo "  Component path: $LBM_COMPONENT_PATH"
}

# Test configuration
test_configuration() {
    print_status "Testing component configuration..."
    
    # Try to configure the project
    if idf.py reconfigure > /dev/null 2>&1; then
        print_status "Configuration: OK"
    else
        print_error "Configuration failed. Run 'idf.py reconfigure' to see detailed errors."
        return 1
    fi
}

# Test compilation
test_compilation() {
    print_status "Testing compilation..."
    
    # Try to build the project
    print_status "Building project (this may take a while)..."
    if idf.py build > build.log 2>&1; then
        print_status "Compilation: OK"
        rm -f build.log
    else
        print_error "Compilation failed. Check build.log for details:"
        echo "  tail build.log"
        return 1
    fi
}

# Create a simple test application if main doesn't exist
create_test_app() {
    if [ ! -f "main/main.c" ] && [ ! -f "main/lbm_porting_test.c" ]; then
        print_status "Creating test application..."
        
        mkdir -p main
        
        cat > main/CMakeLists.txt << 'EOF'
idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS ".")
EOF
        
        cat > main/main.c << 'EOF'
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Test including LBM headers
#include "smtc_modem_api.h"
#include "radio_hal_esp32.h"

static const char *TAG = "LBM_TEST";

void app_main(void)
{
    ESP_LOGI(TAG, "LoRa Basics Modem ESP32 Integration Test");
    ESP_LOGI(TAG, "LBM headers included successfully!");
    
    // Basic test - just verify we can call LBM functions
    smtc_modem_version_t version;
    smtc_modem_return_code_t rc = smtc_modem_get_modem_version(&version);
    
    if (rc == SMTC_MODEM_RC_OK) {
        ESP_LOGI(TAG, "Modem version: %02x.%02x.%02x.%02x", 
                 version.major, version.minor, version.patch, version.revision);
    } else {
        ESP_LOGI(TAG, "Modem not initialized (expected at this stage)");
    }
    
    ESP_LOGI(TAG, "Integration test completed successfully!");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "LBM component is properly integrated");
    }
}
EOF
        
        print_status "Test application created in main/"
    fi
}

# Main test sequence
main() {
    echo ""
    check_esp_idf
    echo ""
    check_project_structure
    echo ""
    check_lbm_component
    echo ""
    create_test_app
    echo ""
    test_configuration
    echo ""
    test_compilation
    echo ""
    
    print_status "🎉 All integration tests passed!"
    echo ""
    echo "Next steps:"
    echo "1. Configure your hardware settings: idf.py menuconfig"
    echo "2. Flash to your ESP32: idf.py flash monitor"
    echo "3. Check the examples in components/lbm/examples/"
    echo ""
    echo "For hardware configuration help, see:"
    echo "  components/lbm/README_ESP32.md"
}

# Run the tests
main "$@"
