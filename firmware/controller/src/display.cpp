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

// Draw one joystick as a crosshair box with a dot at the stick position.
// xbar/ybar are 0..ADC_MAP_MAX (centre = ADC_MAP_CENTER).
static void drawStick(int x0, int y0, int sz, int xbar, int ybar, const char* label) {
  display.drawRect(x0, y0, sz, sz, SSD1306_WHITE);
  int cx = x0 + sz / 2;
  int cy = y0 + sz / 2;
  // Centre crosshair (reference)
  display.drawLine(cx, y0 + 2, cx, y0 + sz - 2, SSD1306_WHITE);
  display.drawLine(x0 + 2, cy, x0 + sz - 2, cy, SSD1306_WHITE);
  // Dot position (invert Y so stick-up is up on screen)
  int px = x0 + 3 + (int)((long)xbar * (sz - 6) / ADC_MAP_MAX);
  int py = y0 + 3 + (int)((long)(ADC_MAP_MAX - ybar) * (sz - 6) / ADC_MAP_MAX);
  display.fillCircle(px, py, 3, SSD1306_WHITE);
  // Label centred under the box
  int len = strlen(label) * 6;
  display.setCursor(x0 + (sz - len) / 2, y0 + sz + 2);
  display.print(label);
}

void drawJoystickBars() {
  int leftXBar, leftYBar, rightXBar, rightYBar;
  mapJoystickValues(leftXBar, leftYBar, rightXBar, rightYBar);

  // Left stick = translation, right stick = rotation
  drawStick(4, 13, 38, leftXBar, leftYBar, "MOVE");
  drawStick(86, 13, 38, rightXBar, rightYBar, "TURN");
}

void drawButtonStatus() {
  extern bool leftButton, rightButton, auxSwitch;

  // Button indicators in the middle column, between the two stick boxes.
  struct Btn { const char* l; bool on; } btns[3] = {
    {"L", leftButton}, {"R", rightButton}, {"A", auxSwitch}
  };
  for (int i = 0; i < 3; i++) {
    int y = 16 + i * 13;
    display.setCursor(50, y + 1);
    display.print(btns[i].l);
    display.drawRect(60, y, 9, 9, SSD1306_WHITE);
    if (btns[i].on) display.fillRect(62, y + 2, 5, 5, SSD1306_WHITE);
  }
}

void drawESPNowStatus() {
  // Top status bar: device name on the left, link indicator on the right.
  ControlDevice &dev = devices[selectedDevice];

  display.setCursor(0, 0);
  display.print(dev.name);

  // Link indicator: filled circle = linked, hollow = no link.
  if (dev.linkOk) {
    display.fillCircle(122, 3, 3, SSD1306_WHITE);
  } else {
    display.drawCircle(122, 3, 3, SSD1306_WHITE);
  }

  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
}

void displayDeviceMenu(int highlight) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println(F("SELECT DEVICE"));
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  for (int i = 0; i < numDevices && i < 4; i++) {
    int yPos = 14 + i * 10;
    display.setCursor(0, yPos);
    display.print(i == highlight ? F(">") : F(" "));
    display.print(F(" "));
    display.print(devices[i].name);
    if (i == selectedDevice) display.print(F(" *"));
    if (devices[i].linkOk) {
      display.setCursor(110, yPos);
      display.print(F("OK"));
    }
  }

  display.setCursor(0, 56);
  display.print(F("release = pick"));
  display.display();
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
