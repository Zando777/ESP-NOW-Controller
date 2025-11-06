#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>

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
#define LEFT_VRY 3  // ADC1_CH3 (was aux switch)
#define LEFT_SW 5

// Right Joystick pins
#define RIGHT_VRX 2  // ADC1_CH2
#define RIGHT_VRY 1  // ADC1_CH1
#define RIGHT_SW 0

// Aux toggle switch - Moved to non-ADC pin
#define AUX_SWITCH 7  // Digital input only (not ADC)

// Create display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Preferences object for storing calibration
Preferences preferences;

// Function prototypes
void saveCalibration();
void loadCalibration();
void handleCalibration();

// Variables to store joystick values
int leftX, leftY, rightX, rightY;
bool leftButton, rightButton, auxSwitch;

// Calibration values
int leftXMin = 0, leftXMax = 4095, leftXCenter = 2048;
int leftYMin = 0, leftYMax = 4095, leftYCenter = 2048;
int rightXMin = 0, rightXMax = 4095, rightXCenter = 2048;
int rightYMin = 0, rightYMax = 4095, rightYCenter = 2048;

// Calibration mode
bool inCalibrationMode = false;
bool waitingForButtonRelease = false;
int calibrationStep = 0;
unsigned long calibrationStepStart = 0;
unsigned long bothButtonsPressedStart = 0;
bool bothButtonsWerePressed = false;

// Welcome screen timing
unsigned long welcomeStartTime = 0;
bool welcomeShown = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C with custom pins
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  // Rotate display 180 degrees (upside down)
  display.setRotation(2);
  
  // Configure joystick pins
  pinMode(LEFT_VRX, INPUT);
  pinMode(LEFT_VRY, INPUT);
  pinMode(LEFT_SW, INPUT_PULLUP);
  
  pinMode(RIGHT_VRX, INPUT);
  pinMode(RIGHT_VRY, INPUT);
  pinMode(RIGHT_SW, INPUT_PULLUP);
  
  // Configure aux switch
  pinMode(AUX_SWITCH, INPUT_PULLUP);
  
  // Show welcome message
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println(F("ESP-NOW"));
  display.setCursor(5, 35);
  display.println(F("Controller"));
  display.display();
  
  welcomeStartTime = millis();
  welcomeShown = true;
  
  Serial.println("========================================");
  Serial.println("ESP-NOW Controller Started");
  Serial.println("========================================");
  Serial.println("Pin Configuration:");
  Serial.println("  OLED: SDA=GPIO8, SCL=GPIO9");
  Serial.println("  Left Joystick: VRX=GPIO4, VRY=GPIO3, SW=GPIO5");
  Serial.println("  Right Joystick: VRX=GPIO2, VRY=GPIO1, SW=GPIO0");
  Serial.println("  Aux Switch: GPIO6");
  Serial.println("  Note: GPIO 0-4 are the only reliable ADC pins on ESP32-C3");
  Serial.println("========================================");
  
  // Load saved calibration from NVS
  loadCalibration();
}

void saveCalibration() {
  preferences.begin("joystick", false); // false = read/write mode
  
  preferences.putInt("leftXMin", leftXMin);
  preferences.putInt("leftXMax", leftXMax);
  preferences.putInt("leftXCenter", leftXCenter);
  preferences.putInt("leftYMin", leftYMin);
  preferences.putInt("leftYMax", leftYMax);
  preferences.putInt("leftYCenter", leftYCenter);
  
  preferences.putInt("rightXMin", rightXMin);
  preferences.putInt("rightXMax", rightXMax);
  preferences.putInt("rightXCenter", rightXCenter);
  preferences.putInt("rightYMin", rightYMin);
  preferences.putInt("rightYMax", rightYMax);
  preferences.putInt("rightYCenter", rightYCenter);
  
  preferences.putBool("calibrated", true);
  
  preferences.end();
  
  Serial.println("Calibration saved to NVS!");
}

void loadCalibration() {
  preferences.begin("joystick", true); // true = read-only mode
  
  bool isCalibrated = preferences.getBool("calibrated", false);
  
  if (isCalibrated) {
    leftXMin = preferences.getInt("leftXMin", 0);
    leftXMax = preferences.getInt("leftXMax", 4095);
    leftXCenter = preferences.getInt("leftXCenter", 2048);
    leftYMin = preferences.getInt("leftYMin", 0);
    leftYMax = preferences.getInt("leftYMax", 4095);
    leftYCenter = preferences.getInt("leftYCenter", 2048);
    
    rightXMin = preferences.getInt("rightXMin", 0);
    rightXMax = preferences.getInt("rightXMax", 4095);
    rightXCenter = preferences.getInt("rightXCenter", 2048);
    rightYMin = preferences.getInt("rightYMin", 0);
    rightYMax = preferences.getInt("rightYMax", 4095);
    rightYCenter = preferences.getInt("rightYCenter", 2048);
    
    Serial.println("\n>>> Calibration loaded from NVS <<<");
    Serial.println("Left Joystick:");
    Serial.print("  X: Min="); Serial.print(leftXMin); 
    Serial.print(" Center="); Serial.print(leftXCenter);
    Serial.print(" Max="); Serial.println(leftXMax);
    Serial.print("  Y: Min="); Serial.print(leftYMin);
    Serial.print(" Center="); Serial.print(leftYCenter);
    Serial.print(" Max="); Serial.println(leftYMax);
    Serial.println("Right Joystick:");
    Serial.print("  X: Min="); Serial.print(rightXMin);
    Serial.print(" Center="); Serial.print(rightXCenter);
    Serial.print(" Max="); Serial.println(rightXMax);
    Serial.print("  Y: Min="); Serial.print(rightYMin);
    Serial.print(" Center="); Serial.print(rightYCenter);
    Serial.print(" Max="); Serial.println(rightYMax);
  } else {
    Serial.println("\nNo calibration found in NVS. Using defaults.");
    Serial.println("Hold both joystick buttons for 5 seconds to calibrate.");
  }
  
  preferences.end();
}

void handleCalibration() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Read current joystick values
  int lx = analogRead(LEFT_VRX);
  int ly = analogRead(LEFT_VRY);
  int rx = analogRead(RIGHT_VRX);
  int ry = analogRead(RIGHT_VRY);
  
  // Show calibration step on display
  display.setCursor(0, 0);
  display.println(F("CALIBRATION MODE"));
  display.println();
  
  // Display current raw values for debugging
  display.setTextSize(1);
  display.print(F("LX:")); display.print(lx);
  display.print(F(" LY:")); display.println(ly);
  display.print(F("RX:")); display.print(rx);
  display.print(F(" RY:")); display.println(ry);
  display.println();
  
  switch(calibrationStep) {
    case 0:
      display.println(F("Move LEFT joystick"));
      display.println(F("fully UP"));
      display.println();
      display.println(F("Press R button"));
      display.println(F("when ready"));
      Serial.println("Step 0: Move LEFT joystick UP, press R button");
      
      if (!digitalRead(RIGHT_SW) == true) {
        leftYMax = ly;
        calibrationStep++;
        Serial.print("Captured Left Y Max: "); Serial.println(leftYMax);
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
        leftYMin = ly;
        calibrationStep++;
        Serial.print("Captured Left Y Min: "); Serial.println(leftYMin);
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
        leftXMin = lx;
        calibrationStep++;
        Serial.print("Captured Left X Min: "); Serial.println(leftXMin);
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
        leftXMax = lx;
        calibrationStep++;
        Serial.print("Captured Left X Max: "); Serial.println(leftXMax);
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
        rightYMax = ry;
        calibrationStep++;
        Serial.print("Captured Right Y Max: "); Serial.println(rightYMax);
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
        rightYMin = ry;
        calibrationStep++;
        Serial.print("Captured Right Y Min: "); Serial.println(rightYMin);
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
        rightXMin = rx;
        calibrationStep++;
        Serial.print("Captured Right X Min: "); Serial.println(rightXMin);
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
        rightXMax = rx;
        calibrationStep++;
        Serial.print("Captured Right X Max: "); Serial.println(rightXMax);
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
        leftXCenter = lx;
        leftYCenter = ly;
        rightXCenter = rx;
        rightYCenter = ry;
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
      Serial.print("  X: Min="); Serial.print(leftXMin); 
      Serial.print(" Center="); Serial.print(leftXCenter);
      Serial.print(" Max="); Serial.println(leftXMax);
      Serial.print("  Y: Min="); Serial.print(leftYMin);
      Serial.print(" Center="); Serial.print(leftYCenter);
      Serial.print(" Max="); Serial.println(leftYMax);
      Serial.println("Right Joystick:");
      Serial.print("  X: Min="); Serial.print(rightXMin);
      Serial.print(" Center="); Serial.print(rightXCenter);
      Serial.print(" Max="); Serial.println(rightXMax);
      Serial.print("  Y: Min="); Serial.print(rightYMin);
      Serial.print(" Center="); Serial.print(rightYCenter);
      Serial.print(" Max="); Serial.println(rightYMax);
      
      // Validate and fix calibration values if needed
      // Ensure min is actually less than max
      if (leftXMin > leftXMax) { int temp = leftXMin; leftXMin = leftXMax; leftXMax = temp; }
      if (leftYMin > leftYMax) { int temp = leftYMin; leftYMin = leftYMax; leftYMax = temp; }
      if (rightXMin > rightXMax) { int temp = rightXMin; rightXMin = rightXMax; rightXMax = temp; }
      if (rightYMin > rightYMax) { int temp = rightYMin; rightYMin = rightYMax; rightYMax = temp; }
      
      // Ensure center is within min/max range
      leftXCenter = constrain(leftXCenter, leftXMin, leftXMax);
      leftYCenter = constrain(leftYCenter, leftYMin, leftYMax);
      rightXCenter = constrain(rightXCenter, rightXMin, rightXMax);
      rightYCenter = constrain(rightYCenter, rightYMin, rightYMax);
      
      Serial.println("Validated calibration values.");
      Serial.println("========================================\n");
      
      // Save calibration to non-volatile storage
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

void loop() {
  // Show welcome screen for 2 seconds
  if (welcomeShown && (millis() - welcomeStartTime < 2000)) {
    return;
  }
  welcomeShown = false;
  
  // Read all analog inputs
  leftX = analogRead(LEFT_VRX);
  leftY = analogRead(LEFT_VRY);
  rightX = analogRead(RIGHT_VRX);
  rightY = analogRead(RIGHT_VRY);
  
  // Read digital inputs (active LOW with pullup)
  leftButton = !digitalRead(LEFT_SW);
  rightButton = !digitalRead(RIGHT_SW);
  auxSwitch = !digitalRead(AUX_SWITCH);
  
  // Check for calibration mode trigger (both buttons held for 5 seconds)
  if (leftButton && rightButton && !inCalibrationMode) {
    if (!bothButtonsWerePressed) {
      bothButtonsPressedStart = millis();
      bothButtonsWerePressed = true;
    } else if (millis() - bothButtonsPressedStart >= 5000) {
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
        delay(500); // Small delay before starting
      }
      return;
    }
    
    handleCalibration();
    return;
  }
  
  // Clear display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Map joystick values using calibrated ranges to 0-58 pixels
  // Center position should map to 29 pixels
  int leftXBar, leftYBar, rightXBar, rightYBar;
  
  // Left X mapping with deadzone around center
  if (abs(leftX - leftXCenter) < 50) {
    leftXBar = 29; // Deadzone at center
  } else if (leftX < leftXCenter) {
    leftXBar = map(constrain(leftX, leftXMin, leftXCenter), leftXMin, leftXCenter, 0, 29);
  } else {
    leftXBar = map(constrain(leftX, leftXCenter, leftXMax), leftXCenter, leftXMax, 29, 58);
  }
  
  // Left Y mapping with deadzone
  if (abs(leftY - leftYCenter) < 50) {
    leftYBar = 29;
  } else if (leftY < leftYCenter) {
    leftYBar = map(constrain(leftY, leftYMin, leftYCenter), leftYMin, leftYCenter, 0, 29);
  } else {
    leftYBar = map(constrain(leftY, leftYCenter, leftYMax), leftYCenter, leftYMax, 29, 58);
  }
  
  // Right X mapping with deadzone
  if (abs(rightX - rightXCenter) < 50) {
    rightXBar = 29;
  } else if (rightX < rightXCenter) {
    rightXBar = map(constrain(rightX, rightXMin, rightXCenter), rightXMin, rightXCenter, 0, 29);
  } else {
    rightXBar = map(constrain(rightX, rightXCenter, rightXMax), rightXCenter, rightXMax, 29, 58);
  }
  
  // Right Y mapping with deadzone
  if (abs(rightY - rightYCenter) < 50) {
    rightYBar = 29;
  } else if (rightY < rightYCenter) {
    rightYBar = map(constrain(rightY, rightYMin, rightYCenter), rightYMin, rightYCenter, 0, 29);
  } else {
    rightYBar = map(constrain(rightY, rightYCenter, rightYMax), rightYCenter, rightYMax, 29, 58);
  }
  
  // Constrain bars to valid range (safety net)
  leftXBar = constrain(leftXBar, 0, 58);
  leftYBar = constrain(leftYBar, 0, 58);
  rightXBar = constrain(rightXBar, 0, 58);
  rightYBar = constrain(rightYBar, 0, 58);
  
  // Left Joystick Section
  display.setCursor(0, 0);
  display.print(F("L-X"));
  display.drawRect(20, 0, 60, 8, SSD1306_WHITE);  // Border
  display.drawLine(50, 0, 50, 8, SSD1306_WHITE);  // Center line marker
  display.fillRect(21, 1, leftXBar, 6, SSD1306_WHITE);  // Fill bar
  display.setCursor(82, 0);
  display.print(leftX);
  
  display.setCursor(0, 10);
  display.print(F("L-Y"));
  display.drawRect(20, 10, 60, 8, SSD1306_WHITE);
  display.drawLine(50, 10, 50, 18, SSD1306_WHITE);  // Center line marker
  display.fillRect(21, 11, leftYBar, 6, SSD1306_WHITE);
  display.setCursor(82, 10);
  display.print(leftY);
  
  // Right Joystick Section
  display.setCursor(0, 22);
  display.print(F("R-X"));
  display.drawRect(20, 22, 60, 8, SSD1306_WHITE);
  display.drawLine(50, 22, 50, 30, SSD1306_WHITE);  // Center line marker
  display.fillRect(21, 23, rightXBar, 6, SSD1306_WHITE);
  display.setCursor(82, 22);
  display.print(rightX);
  
  display.setCursor(0, 32);
  display.print(F("R-Y"));
  display.drawRect(20, 32, 60, 8, SSD1306_WHITE);
  display.drawLine(50, 32, 50, 40, SSD1306_WHITE);  // Center line marker
  display.fillRect(21, 33, rightYBar, 6, SSD1306_WHITE);
  display.setCursor(82, 32);
  display.print(rightY);
  
  // Divider line
  display.drawLine(0, 44, 127, 44, SSD1306_WHITE);
  
  // Checkboxes for buttons at the bottom
  display.setCursor(0, 48);
  display.print(F("L"));
  display.drawRect(10, 48, 10, 10, SSD1306_WHITE);
  if (leftButton) {
    display.fillRect(12, 50, 6, 6, SSD1306_WHITE);  // Filled checkbox
  }
  
  display.setCursor(25, 48);
  display.print(F("R"));
  display.drawRect(35, 48, 10, 10, SSD1306_WHITE);
  if (rightButton) {
    display.fillRect(37, 50, 6, 6, SSD1306_WHITE);
  }
  
  display.setCursor(50, 48);
  display.print(F("AUX"));
  display.drawRect(73, 48, 10, 10, SSD1306_WHITE);
  if (auxSwitch) {
    display.fillRect(75, 50, 6, 6, SSD1306_WHITE);
  }
  
  // Update display
  display.display();
  
  // Serial debugging output
  Serial.println("----------------------------------------");
  Serial.print("Time: "); Serial.println(millis());
  
  Serial.println("\n[Left Joystick]");
  Serial.print("  VRX (GPIO4): "); Serial.print(leftX);
  Serial.print(" [Min:"); Serial.print(leftXMin);
  Serial.print(" Ctr:"); Serial.print(leftXCenter);
  Serial.print(" Max:"); Serial.print(leftXMax);
  Serial.print("] Bar:"); Serial.print(leftXBar);
  if (leftX < leftXCenter - 500) Serial.println(" -> LEFT");
  else if (leftX > leftXCenter + 500) Serial.println(" -> RIGHT");
  else Serial.println(" -> CENTER");
  
  Serial.print("  VRY (GPIO3):  "); Serial.print(leftY);
  Serial.print(" [Min:"); Serial.print(leftYMin);
  Serial.print(" Ctr:"); Serial.print(leftYCenter);
  Serial.print(" Max:"); Serial.print(leftYMax);
  Serial.print("] Bar:"); Serial.print(leftYBar);
  if (leftY < leftYCenter - 500) Serial.println(" -> DOWN");
  else if (leftY > leftYCenter + 500) Serial.println(" -> UP");
  else Serial.println(" -> CENTER");
  
  Serial.print("  SW (GPIO5):   "); Serial.println(leftButton ? "PRESSED" : "RELEASED");
  
  Serial.println("\n[Right Joystick]");
  Serial.print("  VRX (GPIO2):  "); Serial.print(rightX);
  Serial.print(" [Min:"); Serial.print(rightXMin);
  Serial.print(" Ctr:"); Serial.print(rightXCenter);
  Serial.print(" Max:"); Serial.print(rightXMax);
  Serial.print("] Bar:"); Serial.print(rightXBar);
  if (rightX < rightXCenter - 500) Serial.println(" -> LEFT");
  else if (rightX > rightXCenter + 500) Serial.println(" -> RIGHT");
  else Serial.println(" -> CENTER");
  
  Serial.print("  VRY (GPIO1):  "); Serial.print(rightY);
  Serial.print(" [Min:"); Serial.print(rightYMin);
  Serial.print(" Ctr:"); Serial.print(rightYCenter);
  Serial.print(" Max:"); Serial.print(rightYMax);
  Serial.print("] Bar:"); Serial.print(rightYBar);
  if (rightY < rightYCenter - 500) Serial.println(" -> DOWN");
  else if (rightY > rightYCenter + 500) Serial.println(" -> UP");
  else Serial.println(" -> CENTER");
  
  Serial.print("  SW (GPIO0):   "); Serial.println(rightButton ? "PRESSED" : "RELEASED");
  
  Serial.println("\n[Aux Switch]");
  Serial.print("  GPIO6:        "); Serial.println(auxSwitch ? "ON" : "OFF");
  
  // Show calibration trigger status
  if (leftButton && rightButton && bothButtonsWerePressed) {
    unsigned long holdTime = millis() - bothButtonsPressedStart;
    Serial.print("\n[Calibration Trigger] Holding both buttons: ");
    Serial.print(holdTime);
    Serial.println("ms / 5000ms");
  }
  
  delay(50); // Small delay to avoid flickering
}