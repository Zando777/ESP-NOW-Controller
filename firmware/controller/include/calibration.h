#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <Preferences.h>
#include "config.h"

// ============================================
// CALIBRATION DATA STRUCTURE
// ============================================

struct CalibrationData {
  int leftXMin = 0;
  int leftXMax = ADC_MAX;
  int leftXCenter = ADC_CENTER;
  int leftYMin = 0;
  int leftYMax = ADC_MAX;
  int leftYCenter = ADC_CENTER;
  
  int rightXMin = 0;
  int rightXMax = ADC_MAX;
  int rightXCenter = ADC_CENTER;
  int rightYMin = 0;
  int rightYMax = ADC_MAX;
  int rightYCenter = ADC_CENTER;
};

// ============================================
// GLOBAL VARIABLES
// ============================================

extern CalibrationData calibration;
extern Preferences preferences;
extern bool inCalibrationMode;
extern bool waitingForButtonRelease;
extern int calibrationStep;
extern unsigned long calibrationStepStart;
extern unsigned long bothButtonsPressedStart;
extern bool bothButtonsWerePressed;

// ============================================
// FUNCTION PROTOTYPES
// ============================================

void saveCalibration();
void loadCalibration();
void handleCalibration();
void validateCalibration();

#endif // CALIBRATION_H
