#ifndef ESPNOW_H
#define ESPNOW_H

#include <esp_now.h>
#include <WiFi.h>
#include <cstring>

// ============================================
// ESP-NOW DATA STRUCTURE
// ============================================

typedef struct {
  int leftX;
  int leftY;
  int rightX;
  int rightY;
  bool leftButton;
  bool rightButton;
  bool auxSwitch;
} JoystickData;

// ============================================
// GLOBAL VARIABLES
// ============================================

extern uint8_t receiverMAC[6];
extern JoystickData joystickData;
extern bool espNowReady;
extern unsigned long lastSendTime;
extern int lastSendStatus;  // -1: not sent, 0: success, 1: failed

// ============================================
// FUNCTION PROTOTYPES
// ============================================

void initESPNow();
void sendJoystickData();
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
bool parseMAC(const char* str, uint8_t* macAddr);
void handleSerialCommands();
void printESPNowStatus();

#endif // ESPNOW_H
