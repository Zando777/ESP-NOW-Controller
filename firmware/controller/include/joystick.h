#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <Arduino.h>
#include "config.h"

// ============================================
// GLOBAL VARIABLES
// ============================================

extern int leftX;
extern int leftY;
extern int rightX;
extern int rightY;
extern bool leftButton;
extern bool rightButton;
extern bool auxSwitch;

// ============================================
// FUNCTION PROTOTYPES
// ============================================

void initJoystick();
void readJoystickInputs();
void checkCalibrationTrigger();
void mapJoystickValues(int& leftXBar, int& leftYBar, int& rightXBar, int& rightYBar);
void printJoystickDebug(int leftXBar, int leftYBar, int rightXBar, int rightYBar);

// Map a raw ADC reading to a signed -100..100 value using calibration,
// with a deadzone around centre. Used to build the ESP-NOW command.
int8_t mapAxisSigned(int raw, int mn, int ctr, int mx);

#endif // JOYSTICK_H
