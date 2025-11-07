#include "joystick.h"
#include "calibration.h"
#include "config.h"
#include <Arduino.h>

// ============================================
// GLOBAL VARIABLES
// ============================================

int leftX = ADC_CENTER;
int leftY = ADC_CENTER;
int rightX = ADC_CENTER;
int rightY = ADC_CENTER;
bool leftButton = false;
bool rightButton = false;
bool auxSwitch = false;

// ============================================
// FUNCTION IMPLEMENTATIONS
// ============================================

void initJoystick() {
  pinMode(LEFT_VRX, INPUT);
  pinMode(LEFT_VRY, INPUT);
  pinMode(LEFT_SW, INPUT_PULLUP);
  
  pinMode(RIGHT_VRX, INPUT);
  pinMode(RIGHT_VRY, INPUT);
  pinMode(RIGHT_SW, INPUT_PULLUP);
  
  pinMode(AUX_SWITCH, INPUT_PULLUP);
}

void readJoystickInputs() {
  // Read all analog inputs
  leftX = analogRead(LEFT_VRX);
  leftY = analogRead(LEFT_VRY);
  rightX = analogRead(RIGHT_VRX);
  rightY = analogRead(RIGHT_VRY);
  
  // Read digital inputs (active LOW with pullup)
  leftButton = !digitalRead(LEFT_SW);
  rightButton = !digitalRead(RIGHT_SW);
  auxSwitch = !digitalRead(AUX_SWITCH);
}

void checkCalibrationTrigger() {
  extern unsigned long bothButtonsPressedStart;
  extern bool bothButtonsWerePressed;
  extern bool inCalibrationMode;
  extern bool waitingForButtonRelease;
  extern int calibrationStep;
  extern unsigned long calibrationStepStart;
  
  // Check for calibration mode trigger (both buttons held for 5 seconds)
  if (leftButton && rightButton && !inCalibrationMode) {
    if (!bothButtonsWerePressed) {
      bothButtonsPressedStart = millis();
      bothButtonsWerePressed = true;
    } else if (millis() - bothButtonsPressedStart >= CALIBRATION_TRIGGER_TIME) {
      // Enter calibration mode but wait for button release
      inCalibrationMode = true;
      waitingForButtonRelease = true;
      calibrationStep = 0;
      calibrationStepStart = millis();
      Serial.println("\n>>> ENTERING CALIBRATION MODE <<<");
      Serial.println("Release both buttons to begin...");
    }
  } else if (!leftButton || !rightButton) {
    bothButtonsWerePressed = false;
  }
}

void mapJoystickValues(int& leftXBar, int& leftYBar, int& rightXBar, int& rightYBar) {
  extern CalibrationData calibration;
  
  // Left X mapping with deadzone around center
  if (abs(leftX - calibration.leftXCenter) < DEADZONE_THRESHOLD) {
    leftXBar = ADC_MAP_CENTER;
  } else if (leftX < calibration.leftXCenter) {
    leftXBar = map(constrain(leftX, calibration.leftXMin, calibration.leftXCenter), 
                   calibration.leftXMin, calibration.leftXCenter, ADC_MAP_MIN, ADC_MAP_CENTER);
  } else {
    leftXBar = map(constrain(leftX, calibration.leftXCenter, calibration.leftXMax), 
                   calibration.leftXCenter, calibration.leftXMax, ADC_MAP_CENTER, ADC_MAP_MAX);
  }
  
  // Left Y mapping with deadzone
  if (abs(leftY - calibration.leftYCenter) < DEADZONE_THRESHOLD) {
    leftYBar = ADC_MAP_CENTER;
  } else if (leftY < calibration.leftYCenter) {
    leftYBar = map(constrain(leftY, calibration.leftYMin, calibration.leftYCenter), 
                   calibration.leftYMin, calibration.leftYCenter, ADC_MAP_MIN, ADC_MAP_CENTER);
  } else {
    leftYBar = map(constrain(leftY, calibration.leftYCenter, calibration.leftYMax), 
                   calibration.leftYCenter, calibration.leftYMax, ADC_MAP_CENTER, ADC_MAP_MAX);
  }
  
  // Right X mapping with deadzone
  if (abs(rightX - calibration.rightXCenter) < DEADZONE_THRESHOLD) {
    rightXBar = ADC_MAP_CENTER;
  } else if (rightX < calibration.rightXCenter) {
    rightXBar = map(constrain(rightX, calibration.rightXMin, calibration.rightXCenter), 
                    calibration.rightXMin, calibration.rightXCenter, ADC_MAP_MIN, ADC_MAP_CENTER);
  } else {
    rightXBar = map(constrain(rightX, calibration.rightXCenter, calibration.rightXMax), 
                    calibration.rightXCenter, calibration.rightXMax, ADC_MAP_CENTER, ADC_MAP_MAX);
  }
  
  // Right Y mapping with deadzone
  if (abs(rightY - calibration.rightYCenter) < DEADZONE_THRESHOLD) {
    rightYBar = ADC_MAP_CENTER;
  } else if (rightY < calibration.rightYCenter) {
    rightYBar = map(constrain(rightY, calibration.rightYMin, calibration.rightYCenter), 
                    calibration.rightYMin, calibration.rightYCenter, ADC_MAP_MIN, ADC_MAP_CENTER);
  } else {
    rightYBar = map(constrain(rightY, calibration.rightYCenter, calibration.rightYMax), 
                    calibration.rightYCenter, calibration.rightYMax, ADC_MAP_CENTER, ADC_MAP_MAX);
  }
  
  // Constrain bars to valid range (safety net)
  leftXBar = constrain(leftXBar, ADC_MAP_MIN, ADC_MAP_MAX);
  leftYBar = constrain(leftYBar, ADC_MAP_MIN, ADC_MAP_MAX);
  rightXBar = constrain(rightXBar, ADC_MAP_MIN, ADC_MAP_MAX);
  rightYBar = constrain(rightYBar, ADC_MAP_MIN, ADC_MAP_MAX);
}

void printJoystickDebug(int leftXBar, int leftYBar, int rightXBar, int rightYBar) {
  extern CalibrationData calibration;
  extern unsigned long bothButtonsPressedStart;
  extern bool bothButtonsWerePressed;
  
  Serial.println("----------------------------------------");
  Serial.print("Time: "); Serial.println(millis());
  
  Serial.println("\n[Left Joystick]");
  Serial.print("  VRX (GPIO4): "); Serial.print(leftX);
  Serial.print(" [Min:"); Serial.print(calibration.leftXMin);
  Serial.print(" Ctr:"); Serial.print(calibration.leftXCenter);
  Serial.print(" Max:"); Serial.print(calibration.leftXMax);
  Serial.print("] Bar:"); Serial.print(leftXBar);
  if (leftX < calibration.leftXCenter - 500) Serial.println(" -> LEFT");
  else if (leftX > calibration.leftXCenter + 500) Serial.println(" -> RIGHT");
  else Serial.println(" -> CENTER");
  
  Serial.print("  VRY (GPIO3):  "); Serial.print(leftY);
  Serial.print(" [Min:"); Serial.print(calibration.leftYMin);
  Serial.print(" Ctr:"); Serial.print(calibration.leftYCenter);
  Serial.print(" Max:"); Serial.print(calibration.leftYMax);
  Serial.print("] Bar:"); Serial.print(leftYBar);
  if (leftY < calibration.leftYCenter - 500) Serial.println(" -> DOWN");
  else if (leftY > calibration.leftYCenter + 500) Serial.println(" -> UP");
  else Serial.println(" -> CENTER");
  
  Serial.print("  SW (GPIO5):   "); Serial.println(leftButton ? "PRESSED" : "RELEASED");
  
  Serial.println("\n[Right Joystick]");
  Serial.print("  VRX (GPIO2):  "); Serial.print(rightX);
  Serial.print(" [Min:"); Serial.print(calibration.rightXMin);
  Serial.print(" Ctr:"); Serial.print(calibration.rightXCenter);
  Serial.print(" Max:"); Serial.print(calibration.rightXMax);
  Serial.print("] Bar:"); Serial.print(rightXBar);
  if (rightX < calibration.rightXCenter - 500) Serial.println(" -> LEFT");
  else if (rightX > calibration.rightXCenter + 500) Serial.println(" -> RIGHT");
  else Serial.println(" -> CENTER");
  
  Serial.print("  VRY (GPIO1):  "); Serial.print(rightY);
  Serial.print(" [Min:"); Serial.print(calibration.rightYMin);
  Serial.print(" Ctr:"); Serial.print(calibration.rightYCenter);
  Serial.print(" Max:"); Serial.print(calibration.rightYMax);
  Serial.print("] Bar:"); Serial.print(rightYBar);
  if (rightY < calibration.rightYCenter - 500) Serial.println(" -> DOWN");
  else if (rightY > calibration.rightYCenter + 500) Serial.println(" -> UP");
  else Serial.println(" -> CENTER");
  
  Serial.print("  SW (GPIO0):   "); Serial.println(rightButton ? "PRESSED" : "RELEASED");
  
  Serial.println("\n[Aux Switch]");
  Serial.print("  GPIO7:        "); Serial.println(auxSwitch ? "ON" : "OFF");
  
  // Show calibration trigger status
  if (leftButton && rightButton && bothButtonsWerePressed) {
    unsigned long holdTime = millis() - bothButtonsPressedStart;
    Serial.print("\n[Calibration Trigger] Holding both buttons: ");
    Serial.print(holdTime);
    Serial.println("ms / 5000ms");
  }
}
