#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_SSD1306.h>

// ============================================
// GLOBAL VARIABLES
// ============================================

extern Adafruit_SSD1306 display;

// ============================================
// FUNCTION PROTOTYPES
// ============================================

void initDisplay();
void showWelcomeScreen();
void updateMainDisplay();
void drawJoystickBars();
void drawButtonStatus();
void drawESPNowStatus();
void displayCalibrationScreen();

#endif // DISPLAY_H
