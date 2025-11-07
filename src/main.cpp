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
  
  // Initialize I2C with custom pins
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Initialize display
  initDisplay();
  showWelcomeScreen();
  
  unsigned long welcomeStartTime = millis();
  
  // Configure joystick pins
  initJoystick();
  
  // Show welcome message
  Serial.println("========================================");
  Serial.println("ESP-NOW Controller Started");
  Serial.println("========================================");
  Serial.println("Pin Configuration:");
  Serial.println("  OLED: SDA=GPIO8, SCL=GPIO9");
  Serial.println("  Left Joystick: VRX=GPIO4, VRY=GPIO3, SW=GPIO5");
  Serial.println("  Right Joystick: VRX=GPIO2, VRY=GPIO1, SW=GPIO0");
  Serial.println("  Aux Switch: GPIO7");
  Serial.println("========================================");
  
  // Load saved calibration from NVS
  loadCalibration();
  
  // Initialize ESP-NOW
  initESPNow();
  
  Serial.println("\nSetup complete. Ready for operation.");
  Serial.println("Type 'HELP' for available commands.\n");
  
  // Wait for welcome screen to finish
  while (millis() - welcomeStartTime < WELCOME_SCREEN_DURATION) {
    delay(10);
  }
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {
  // Handle serial commands (for setting MAC address, etc.)
  handleSerialCommands();
  
  // Read all joystick inputs
  readJoystickInputs();
  
  // Send joystick data via ESP-NOW
  sendJoystickData();
  
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
  
  // Update main display
  updateMainDisplay();
  
  // Serial debugging output
  int leftXBar, leftYBar, rightXBar, rightYBar;
  mapJoystickValues(leftXBar, leftYBar, rightXBar, rightYBar);
  printJoystickDebug(leftXBar, leftYBar, rightXBar, rightYBar);
  
  delay(50); // Small delay to avoid flickering
}
