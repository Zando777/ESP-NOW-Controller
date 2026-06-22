#include <Arduino.h>
#include <Wire.h>

// Include all modular headers
#include "config.h"
#include "calibration.h"
#include "espnow.h"
#include "display.h"
#include "joystick.h"

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  delay(200);

  // Initialize I2C with custom pins
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize display
  initDisplay();
  showWelcomeScreen();
  unsigned long welcomeStartTime = millis();

  // Configure joystick pins
  initJoystick();

  Serial.println("========================================");
  Serial.println("ESP-NOW Controller Started");
  Serial.println("========================================");

  // Load saved calibration from NVS
  loadCalibration();

  // Initialize ESP-NOW (sets WiFi STA, finds the default device's channel)
  initESPNow();

  Serial.println("Setup complete. Type 'HELP' for commands.\n");

  // Wait for welcome screen to finish
  while (millis() - welcomeStartTime < WELCOME_SCREEN_DURATION) {
    delay(10);
  }
}

// ============================================
// MAIN LOOP
// ============================================

// Controller operating mode
enum ControllerMode { MODE_DRIVE, MODE_SELECT };
static ControllerMode mode = MODE_DRIVE;

// Device-select menu state machine. Hold the right button alone to open the
// menu, tilt the left stick up/down to move the highlight, release to pick.
static void updateDeviceSelection() {
  extern int leftY;
  extern CalibrationData calibration;
  static unsigned long rightHoldStart = 0;
  static bool rightWasHeld = false;
  static unsigned long lastTiltMs = 0;
  static int highlight = 0;

  if (mode == MODE_DRIVE) {
    if (rightButton && !leftButton) {
      if (!rightWasHeld) {
        rightWasHeld = true;
        rightHoldStart = millis();
      } else if (millis() - rightHoldStart > HOLD_TO_MENU_MS) {
        mode = MODE_SELECT;
        highlight = selectedDevice;
      }
    } else {
      rightWasHeld = false;
    }
  } else {  // MODE_SELECT
    int8_t y = mapAxisSigned(leftY, calibration.leftYMin,
                             calibration.leftYCenter, calibration.leftYMax);
    if (millis() - lastTiltMs > MENU_TILT_REPEAT_MS) {
      if (y > 50) {  // up = previous
        highlight = (highlight - 1 + numDevices) % numDevices;
        lastTiltMs = millis();
      } else if (y < -50) {  // down = next
        highlight = (highlight + 1) % numDevices;
        lastTiltMs = millis();
      }
    }
    // Release the right button to confirm the highlighted device.
    if (!rightButton) {
      mode = MODE_DRIVE;
      rightWasHeld = false;
      selectDevice(highlight);
    }
    displayDeviceMenu(highlight);
  }
}

void loop() {
  // Handle serial commands (for setting MAC address, etc.)
  handleSerialCommands();

  // Read all joystick inputs
  readJoystickInputs();

  // Check for calibration mode trigger
  checkCalibrationTrigger();

  // Handle calibration mode
  if (inCalibrationMode) {
    // If waiting for button release, show message and wait
    if (waitingForButtonRelease) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println(F("CALIBRATION MODE"));
      display.println();
      display.println(F("Release both"));
      display.println(F("joystick buttons"));
      display.println(F("to begin..."));
      display.display();
      
      // Check if both buttons are released
      if (!leftButton && !rightButton) {
        waitingForButtonRelease = false;
        Serial.println("Buttons released. Starting calibration...");
        delay(500);
      }
      return;
    }
    
    handleCalibration();
    return;
  }
  
  // Device-select state machine (handles its own display when in the menu)
  updateDeviceSelection();
  if (mode == MODE_SELECT) {
    delay(20);
    return;  // do not drive while selecting; robot failsafe stops it
  }

  // DRIVE: stream control command at 50 Hz (sendControlCommand rate-limits
  // internally). The OLED refresh is slow over I2C, so throttle it to ~10 Hz
  // to avoid capping the command rate.
  sendControlCommand();

  static unsigned long lastDisplayMs = 0;
  if (millis() - lastDisplayMs > 100) {
    updateMainDisplay();
    lastDisplayMs = millis();
  }

  delay(5);
}
