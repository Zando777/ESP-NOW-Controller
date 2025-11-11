#include "calibration.h"
#include "config.h"
#include "display.h"
#include <Arduino.h>

// ============================================
// GLOBAL VARIABLES
// ============================================

CalibrationData calibration;
Preferences preferences;
bool inCalibrationMode = false;
bool waitingForButtonRelease = false;
int calibrationStep = 0;
unsigned long calibrationStepStart = 0;
unsigned long bothButtonsPressedStart = 0;
bool bothButtonsWerePressed = false;

// ============================================
// FUNCTION IMPLEMENTATIONS
// ============================================

void saveCalibration() {
  preferences.begin("joystick", false);
  
  preferences.putInt("leftXMin", calibration.leftXMin);
  preferences.putInt("leftXMax", calibration.leftXMax);
  preferences.putInt("leftXCenter", calibration.leftXCenter);
  preferences.putInt("leftYMin", calibration.leftYMin);
  preferences.putInt("leftYMax", calibration.leftYMax);
  preferences.putInt("leftYCenter", calibration.leftYCenter);
  
  preferences.putInt("rightXMin", calibration.rightXMin);
  preferences.putInt("rightXMax", calibration.rightXMax);
  preferences.putInt("rightXCenter", calibration.rightXCenter);
  preferences.putInt("rightYMin", calibration.rightYMin);
  preferences.putInt("rightYMax", calibration.rightYMax);
  preferences.putInt("rightYCenter", calibration.rightYCenter);
  
  preferences.putBool("calibrated", true);
  preferences.end();
  
  Serial.println("Calibration saved to NVS!");
}

void loadCalibration() {
  preferences.begin("joystick", true);
  
  bool isCalibrated = preferences.getBool("calibrated", false);
  
  if (isCalibrated) {
    calibration.leftXMin = preferences.getInt("leftXMin", 0);
    calibration.leftXMax = preferences.getInt("leftXMax", ADC_MAX);
    calibration.leftXCenter = preferences.getInt("leftXCenter", ADC_CENTER);
    calibration.leftYMin = preferences.getInt("leftYMin", 0);
    calibration.leftYMax = preferences.getInt("leftYMax", ADC_MAX);
    calibration.leftYCenter = preferences.getInt("leftYCenter", ADC_CENTER);
    
    calibration.rightXMin = preferences.getInt("rightXMin", 0);
    calibration.rightXMax = preferences.getInt("rightXMax", ADC_MAX);
    calibration.rightXCenter = preferences.getInt("rightXCenter", ADC_CENTER);
    calibration.rightYMin = preferences.getInt("rightYMin", 0);
    calibration.rightYMax = preferences.getInt("rightYMax", ADC_MAX);
    calibration.rightYCenter = preferences.getInt("rightYCenter", ADC_CENTER);
    
    Serial.println("\n>>> Calibration loaded from NVS <<<");
    Serial.println("Left Joystick:");
    Serial.print("  X: Min="); Serial.print(calibration.leftXMin); 
    Serial.print(" Center="); Serial.print(calibration.leftXCenter);
    Serial.print(" Max="); Serial.println(calibration.leftXMax);
    Serial.print("  Y: Min="); Serial.print(calibration.leftYMin);
    Serial.print(" Center="); Serial.print(calibration.leftYCenter);
    Serial.print(" Max="); Serial.println(calibration.leftYMax);
    Serial.println("Right Joystick:");
    Serial.print("  X: Min="); Serial.print(calibration.rightXMin);
    Serial.print(" Center="); Serial.print(calibration.rightXCenter);
    Serial.print(" Max="); Serial.println(calibration.rightXMax);
    Serial.print("  Y: Min="); Serial.print(calibration.rightYMin);
    Serial.print(" Center="); Serial.print(calibration.rightYCenter);
    Serial.print(" Max="); Serial.println(calibration.rightYMax);
  } else {
    Serial.println("\nNo calibration found in NVS. Using defaults.");
    Serial.println("Hold both joystick buttons for 5 seconds to calibrate.");
  }
  
  preferences.end();
}

void validateCalibration() {
  // Ensure min is actually less than max
  if (calibration.leftXMin > calibration.leftXMax) { 
    int temp = calibration.leftXMin; 
    calibration.leftXMin = calibration.leftXMax; 
    calibration.leftXMax = temp; 
  }
  if (calibration.leftYMin > calibration.leftYMax) { 
    int temp = calibration.leftYMin; 
    calibration.leftYMin = calibration.leftYMax; 
    calibration.leftYMax = temp; 
  }
  if (calibration.rightXMin > calibration.rightXMax) { 
    int temp = calibration.rightXMin; 
    calibration.rightXMin = calibration.rightXMax; 
    calibration.rightXMax = temp; 
  }
  if (calibration.rightYMin > calibration.rightYMax) { 
    int temp = calibration.rightYMin; 
    calibration.rightYMin = calibration.rightYMax; 
    calibration.rightYMax = temp; 
  }
  
  // Ensure center is within min/max range
  calibration.leftXCenter = constrain(calibration.leftXCenter, calibration.leftXMin, calibration.leftXMax);
  calibration.leftYCenter = constrain(calibration.leftYCenter, calibration.leftYMin, calibration.leftYMax);
  calibration.rightXCenter = constrain(calibration.rightXCenter, calibration.rightXMin, calibration.rightXMax);
  calibration.rightYCenter = constrain(calibration.rightYCenter, calibration.rightYMin, calibration.rightYMax);
  
  Serial.println("Validated calibration values.");
}

void handleCalibration() {
  extern int leftX, leftY, rightX, rightY;
  extern bool leftButton, rightButton;
  
  // Read current joystick values
  int lx = analogRead(LEFT_VRX);
  int ly = analogRead(LEFT_VRY);
  int rx = analogRead(RIGHT_VRX);
  int ry = analogRead(RIGHT_VRY);
  
  displayCalibrationScreen();
  
  switch(calibrationStep) {
    case 0:
      display.println(F("Move LEFT joystick"));
      display.println(F("fully UP"));
      display.println();
      display.println(F("Press R button"));
      display.println(F("when ready"));
      Serial.println("Step 0: Move LEFT joystick UP, press R button");
      
      if (!digitalRead(RIGHT_SW) == true) {
        calibration.leftYMax = ly;
        calibrationStep++;
        Serial.print("Captured Left Y Max: "); Serial.println(calibration.leftYMax);
        delay(500);
      }
      break;
      
    case 1:
      display.println(F("Move LEFT joystick"));
      display.println(F("fully DOWN"));
      display.println();
      display.println(F("Press R button"));
      Serial.println("Step 1: Move LEFT joystick DOWN, press R button");
      
      if (!digitalRead(RIGHT_SW) == true) {
        calibration.leftYMin = ly;
        calibrationStep++;
        Serial.print("Captured Left Y Min: "); Serial.println(calibration.leftYMin);
        delay(500);
      }
      break;
      
    case 2:
      display.println(F("Move LEFT joystick"));
      display.println(F("fully LEFT"));
      display.println();
      display.println(F("Press R button"));
      Serial.println("Step 2: Move LEFT joystick LEFT, press R button");
      
      if (!digitalRead(RIGHT_SW) == true) {
        calibration.leftXMin = lx;
        calibrationStep++;
        Serial.print("Captured Left X Min: "); Serial.println(calibration.leftXMin);
        delay(500);
      }
      break;
      
    case 3:
      display.println(F("Move LEFT joystick"));
      display.println(F("fully RIGHT"));
      display.println();
      display.println(F("Press R button"));
      Serial.println("Step 3: Move LEFT joystick RIGHT, press R button");
      
      if (!digitalRead(RIGHT_SW) == true) {
        calibration.leftXMax = lx;
        calibrationStep++;
        Serial.print("Captured Left X Max: "); Serial.println(calibration.leftXMax);
        delay(500);
      }
      break;
      
    case 4:
      display.println(F("Move RIGHT joystick"));
      display.println(F("fully UP"));
      display.println();
      display.println(F("Press L button"));
      Serial.println("Step 4: Move RIGHT joystick UP, press L button");
      
      if (!digitalRead(LEFT_SW) == true) {
        calibration.rightYMax = ry;
        calibrationStep++;
        Serial.print("Captured Right Y Max: "); Serial.println(calibration.rightYMax);
        delay(500);
      }
      break;
      
    case 5:
      display.println(F("Move RIGHT joystick"));
      display.println(F("fully DOWN"));
      display.println();
      display.println(F("Press L button"));
      Serial.println("Step 5: Move RIGHT joystick DOWN, press L button");
      
      if (!digitalRead(LEFT_SW) == true) {
        calibration.rightYMin = ry;
        calibrationStep++;
        Serial.print("Captured Right Y Min: "); Serial.println(calibration.rightYMin);
        delay(500);
      }
      break;
      
    case 6:
      display.println(F("Move RIGHT joystick"));
      display.println(F("fully LEFT"));
      display.println();
      display.println(F("Press L button"));
      Serial.println("Step 6: Move RIGHT joystick LEFT, press L button");
      
      if (!digitalRead(LEFT_SW) == true) {
        calibration.rightXMin = rx;
        calibrationStep++;
        Serial.print("Captured Right X Min: "); Serial.println(calibration.rightXMin);
        delay(500);
      }
      break;
      
    case 7:
      display.println(F("Move RIGHT joystick"));
      display.println(F("fully RIGHT"));
      display.println();
      display.println(F("Press L button"));
      Serial.println("Step 7: Move RIGHT joystick RIGHT, press L button");
      
      if (!digitalRead(LEFT_SW) == true) {
        calibration.rightXMax = rx;
        calibrationStep++;
        Serial.print("Captured Right X Max: "); Serial.println(calibration.rightXMax);
        delay(500);
      }
      break;
      
    case 8:
      display.println(F("Center BOTH"));
      display.println(F("joysticks"));
      display.println();
      display.println(F("Press AUX switch"));
      Serial.println("Step 8: Center both joysticks, press AUX switch");
      
      if (!digitalRead(AUX_SWITCH) == true) {
        calibration.leftXCenter = lx;
        calibration.leftYCenter = ly;
        calibration.rightXCenter = rx;
        calibration.rightYCenter = ry;
        calibrationStep++;
        Serial.println("Captured center positions");
        delay(500);
      }
      break;
      
    case 9:
      display.println(F("Calibration"));
      display.println(F("Complete!"));
      display.println();
      display.println(F("Returning to"));
      display.println(F("normal mode..."));
      
      Serial.println("\n>>> CALIBRATION COMPLETE <<<");
      Serial.println("Left Joystick:");
      Serial.print("  X: Min="); Serial.print(calibration.leftXMin); 
      Serial.print(" Center="); Serial.print(calibration.leftXCenter);
      Serial.print(" Max="); Serial.println(calibration.leftXMax);
      Serial.print("  Y: Min="); Serial.print(calibration.leftYMin);
      Serial.print(" Center="); Serial.print(calibration.leftYCenter);
      Serial.print(" Max="); Serial.println(calibration.leftYMax);
      Serial.println("Right Joystick:");
      Serial.print("  X: Min="); Serial.print(calibration.rightXMin);
      Serial.print(" Center="); Serial.print(calibration.rightXCenter);
      Serial.print(" Max="); Serial.println(calibration.rightXMax);
      Serial.print("  Y: Min="); Serial.print(calibration.rightYMin);
      Serial.print(" Center="); Serial.print(calibration.rightYCenter);
      Serial.print(" Max="); Serial.println(calibration.rightYMax);
      
      validateCalibration();
      Serial.println("========================================\n");
      
      saveCalibration();
      display.display();
      delay(2000);
      
      inCalibrationMode = false;
      waitingForButtonRelease = false;
      calibrationStep = 0;
      return;
  }
  
  display.display();
}
