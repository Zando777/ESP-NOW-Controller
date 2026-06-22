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
// ESP-NOW CONTROL
// ============================================

#define CONTROL_PROTOCOL_VERSION 1     // must match receiver firmware
#define CONTROL_DEFAULT_SPEED 200      // master speed sent to robot (0-255)
#define HOLD_TO_MENU_MS 600            // hold right button this long to open device menu
#define MENU_TILT_REPEAT_MS 250        // min time between highlight steps in menu
#define LINK_OK_MS    600              // link shown OK if an ACK was seen within this
#define LINK_DEAD_MS  800              // re-acquire channel after this much ACK silence

// Joystick -> command axis polarity (flip to +1/-1 if a direction is reversed)
#define SIGN_X   (+1)                  // left stick X  -> strafe
#define SIGN_Y   (-1)                  // left stick Y  -> forward (inverted to match stick)
#define SIGN_ROT (+1)                  // right stick X -> rotation

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
