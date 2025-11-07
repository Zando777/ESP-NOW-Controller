#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// PIN CONFIGURATION
// ============================================

// OLED Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// I2C Pins
#define SDA_PIN 8
#define SCL_PIN 9

// Left Joystick pins - Using available ADC pins
#define LEFT_VRX 4  // ADC1_CH4
#define LEFT_VRY 3  // ADC1_CH3
#define LEFT_SW 5

// Right Joystick pins
#define RIGHT_VRX 2  // ADC1_CH2
#define RIGHT_VRY 1  // ADC1_CH1
#define RIGHT_SW 0

// Aux toggle switch - Digital input only (not ADC)
#define AUX_SWITCH 7

// ============================================
// TIMING CONFIGURATION
// ============================================

#define WELCOME_SCREEN_DURATION 2000  // ms
#define CALIBRATION_TRIGGER_TIME 5000 // ms (both buttons held)
#define SEND_INTERVAL 20               // ms (50Hz)
#define DEADZONE_THRESHOLD 50          // ADC units around center

// ============================================
// ADC CONFIGURATION
// ============================================

#define ADC_MIN 0
#define ADC_MAX 4095
#define ADC_CENTER 2048
#define ADC_MAP_MIN 0
#define ADC_MAP_MAX 58
#define ADC_MAP_CENTER 29

// ============================================
// SERIAL COMMUNICATION
// ============================================

#define SERIAL_BAUDRATE 115200

#endif // CONFIG_H
