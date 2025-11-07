#ifndef JOYSTICK_H
#define JOYSTICK_H

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

#endif // JOYSTICK_H
