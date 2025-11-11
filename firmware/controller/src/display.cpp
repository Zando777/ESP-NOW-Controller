#include "display.h"
#include "config.h"
#include "joystick.h"
#include "calibration.h"
#include "espnow.h"
#include <Arduino.h>

// ============================================
// GLOBAL VARIABLES
// ============================================

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ============================================
// FUNCTION IMPLEMENTATIONS
// ============================================

void initDisplay() {
  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  // Rotate display 180 degrees (upside down)
  display.setRotation(2);
}

void showWelcomeScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println(F("ESP-NOW"));
  display.setCursor(5, 35);
  display.println(F("Controller"));
  display.display();
}

void displayCalibrationScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Read current joystick values for debugging
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
}

void drawJoystickBars() {
  extern int leftX, leftY, rightX, rightY;
  extern CalibrationData calibration;
  
  int leftXBar, leftYBar, rightXBar, rightYBar;
  mapJoystickValues(leftXBar, leftYBar, rightXBar, rightYBar);
  
  // Left Joystick Section
  display.setCursor(0, 0);
  display.print(F("L-X"));
  display.drawRect(20, 0, 60, 8, SSD1306_WHITE);
  display.drawLine(50, 0, 50, 8, SSD1306_WHITE);
  display.fillRect(21, 1, leftXBar, 6, SSD1306_WHITE);
  display.setCursor(82, 0);
  display.print(leftX);
  
  display.setCursor(0, 10);
  display.print(F("L-Y"));
  display.drawRect(20, 10, 60, 8, SSD1306_WHITE);
  display.drawLine(50, 10, 50, 18, SSD1306_WHITE);
  display.fillRect(21, 11, leftYBar, 6, SSD1306_WHITE);
  display.setCursor(82, 10);
  display.print(leftY);
  
  // Right Joystick Section
  display.setCursor(0, 22);
  display.print(F("R-X"));
  display.drawRect(20, 22, 60, 8, SSD1306_WHITE);
  display.drawLine(50, 22, 50, 30, SSD1306_WHITE);
  display.fillRect(21, 23, rightXBar, 6, SSD1306_WHITE);
  display.setCursor(82, 22);
  display.print(rightX);
  
  display.setCursor(0, 32);
  display.print(F("R-Y"));
  display.drawRect(20, 32, 60, 8, SSD1306_WHITE);
  display.drawLine(50, 32, 50, 40, SSD1306_WHITE);
  display.fillRect(21, 33, rightYBar, 6, SSD1306_WHITE);
  display.setCursor(82, 32);
  display.print(rightY);
  
  // Divider line
  display.drawLine(0, 44, 127, 44, SSD1306_WHITE);
}

void drawButtonStatus() {
  extern bool leftButton, rightButton, auxSwitch;
  
  // Checkboxes for buttons at the bottom
  display.setCursor(0, 48);
  display.print(F("L"));
  display.drawRect(10, 48, 10, 10, SSD1306_WHITE);
  if (leftButton) {
    display.fillRect(12, 50, 6, 6, SSD1306_WHITE);
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
}

void drawESPNowStatus() {
  extern bool espNowReady;
  extern String lastSendStatus;
  
  // ESP-NOW Status Indicator (top right corner)
  if (espNowReady) {
    display.setCursor(100, 0);
    display.setTextSize(1);
    display.print(F("ESP"));
    if (lastSendStatus == "Delivery Success") {
      display.drawCircle(120, 4, 2, SSD1306_WHITE);
      display.fillCircle(120, 4, 1, SSD1306_WHITE);
    } else {
      display.drawCircle(120, 4, 2, SSD1306_WHITE);
    }
  }
}

void updateMainDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  drawJoystickBars();
  drawButtonStatus();
  drawESPNowStatus();
  
  display.display();
}
