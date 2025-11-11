#!/bin/bash

# ESP-NOW Controller - Simple Upload Script
# This script uploads firmware to both controller and receiver devices

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to check if device is connected
check_device() {
    if ls /dev/cu.usbmodem* 1> /dev/null 2>&1; then
        echo -e "${GREEN}✓ Device detected${NC}"
        return 0
    else
        echo -e "${RED}✗ No device detected${NC}"
        return 1
    fi
}

# Function to upload firmware
upload_firmware() {
    local device_name=$1
    local firmware_dir=$2

    echo -e "${BLUE}Uploading $device_name firmware...${NC}"

    # Save current directory
    local original_dir="$(pwd)"

    # Use absolute path for safety
    local script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    local full_path="$script_dir/$firmware_dir"

    if cd "$full_path" && pio run --target upload --silent; then
        echo -e "${GREEN}✓ $device_name firmware uploaded successfully!${NC}"
        cd "$original_dir"  # Return to original directory
        return 0
    else
        echo -e "${RED}✗ Failed to upload $device_name firmware${NC}"
        cd "$original_dir"  # Return to original directory
        return 1
    fi
}

# Function to wait for device change
wait_for_device_change() {
    local step_name=$1
    local device_desc=$2

    echo -e "${YELLOW}$step_name${NC}"
    echo -e "${BLUE}$device_desc${NC}"
    echo "Press Enter when ready..."

    # Force interactive behavior
    if [ -t 0 ]; then
        echo -e "${BLUE}Waiting for you to press Enter...${NC}"
        if ! read -r -t 30; then
            echo -e "${YELLOW}Timeout waiting for input. Continuing...${NC}"
        fi
    else
        echo -e "${YELLOW}Non-interactive mode detected. Assuming device is ready...${NC}"
        sleep 2
    fi

    echo "Checking for device..."
    local attempts=0
    while ! check_device; do
        attempts=$((attempts + 1))
        if [ $attempts -ge 3 ]; then
            echo -e "${RED}Failed to detect device after 3 attempts.${NC}"
            echo "Please check USB connection and try again."
            return 1
        fi
        echo "Please ensure the device is connected and try again..."
        echo "Press Enter when device is connected..."
        if [ -t 0 ]; then
            echo -e "${BLUE}Waiting for you to press Enter...${NC}"
            if ! read -r -t 30; then
                echo -e "${YELLOW}Timeout waiting for input. Continuing...${NC}"
            fi
        else
            echo -e "${YELLOW}Non-interactive mode: waiting 5 seconds for device connection...${NC}"
            sleep 5
        fi
    done
    return 0
}

# Check command line arguments
if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    echo "ESP-NOW Controller Upload Script"
    echo ""
    echo "Usage:"
    echo "  ./upload.sh              - Interactive upload (recommended)"
    echo "  ./upload.sh --auto       - Automatic upload (for CI/testing)"
    echo "  ./upload.sh --controller - Upload only controller firmware"
    echo "  ./upload.sh --receiver   - Upload only receiver firmware"
    echo "  ./upload.sh --help       - Show this help"
    exit 0
fi

if [ "$1" = "--controller" ]; then
    echo "Uploading only controller firmware..."
    if check_device; then
        upload_firmware "Controller" "firmware/controller"
    else
        echo -e "${RED}No device detected. Please connect the controller.${NC}"
        exit 1
    fi
    exit 0
fi

if [ "$1" = "--receiver" ]; then
    echo "Uploading only receiver firmware..."
    if check_device; then
        upload_firmware "Receiver" "firmware/receiver"
    else
        echo -e "${RED}No device detected. Please connect the receiver.${NC}"
        exit 1
    fi
    exit 0
fi

# Main upload process (always interactive)
if [ ! -t 0 ]; then
    echo -e "${RED}Warning: Script is running in non-interactive mode.${NC}"
    echo "The interactive upload process requires user input between device connections."
    echo ""
    echo "For non-interactive use, please use one of these options:"
    echo "  ./upload.sh --controller    - Upload only controller firmware"
    echo "  ./upload.sh --receiver      - Upload only receiver firmware"
    echo ""
    echo -e "${YELLOW}Continuing in non-interactive mode...${NC}"
    echo ""
fi

echo "========================================"
echo "ESP-NOW Controller - Simple Upload Tool"
echo "========================================"
echo "Please connect your ESP32 devices one at a time:"
echo ""

# Upload controller
if wait_for_device_change "Step 1: Connect the CONTROLLER device" "(with OLED display and joysticks)"; then
    upload_firmware "Controller" "firmware/controller"
else
    exit 1
fi

# Upload receiver
echo ""
if wait_for_device_change "Step 2: Disconnect controller and connect the RECEIVER device" "(standalone ESP32)"; then
    upload_firmware "Receiver" "firmware/receiver"
else
    exit 1
fi

echo ""
echo -e "${GREEN}========================================"
echo "🎉 UPLOAD COMPLETE!"
echo "========================================"
echo ""
echo "Testing Instructions:"
echo "1. Power on both devices"
echo "2. Controller will show joystick data on OLED"
echo "3. Receiver will show received data in serial monitor"
echo ""
echo -e "Use: ${BLUE}pio device monitor${NC} to view serial output"
echo -e "${GREEN}=======================================${NC}"