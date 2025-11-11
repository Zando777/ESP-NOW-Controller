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
  delay(100);
  Serial.write(0xFF); // Marker to verify serial works
  
  // Now wait 10 seconds for serial monitor to connect
  Serial.println("\n\n🔧 STARTUP: Waiting 10 seconds for serial monitor...");
  Serial.flush();
  for (int i = 0; i < 10; i++) {
    Serial.print(".");
    Serial.flush();
    delay(1000);
  }
  Serial.println(" STARTING!\n");
  Serial.flush();
  
  Serial.println("DEBUG: About to initialize I2C");
  Serial.flush();
  
  // Initialize I2C with custom pins
  Wire.begin(SDA_PIN, SCL_PIN);
  
  Serial.println("DEBUG: I2C initialized, about to init display");
  Serial.flush();
  
  // Initialize display
  initDisplay();
  
  Serial.println("DEBUG: Display initialized, showing welcome screen");
  Serial.flush();
  
  showWelcomeScreen();
  
  unsigned long welcomeStartTime = millis();
  
  Serial.println("DEBUG: About to init joystick");
  Serial.flush();
  
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
  
  Serial.println("DEBUG: About to load calibration");
  Serial.flush();
  
  // Load saved calibration from NVS
  loadCalibration();
  
  // Initialize ESP-NOW with explicit debug output
  Serial.println("\n🔧 About to call initESPNow()...");
  Serial.flush();
  
  // TEMPORARILY SKIP ESPNOW TO DEBUG SETUP
  // initESPNow();
  
  Serial.println("✅ initESPNow() skipped for debugging\n");
  Serial.flush();
  
  Serial.println("Setup complete. Ready for operation.");
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
