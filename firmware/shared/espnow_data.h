#ifndef ESPNOW_DATA_H
#define ESPNOW_DATA_H

// MAC Addresses
// Receiver (cu.usbmodem141201): 88:56:a6:64:a1:e8
// Controller (cu.usbmodem141401): ec:da:3b:bd:cd:74

// Joystick data structure - MUST be identical on both boards
typedef struct {
  int leftX, leftY, rightX, rightY;
  bool leftButton, rightButton, auxSwitch;
} JoystickData;

#endif
