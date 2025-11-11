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
extern String lastSendStatus;

// ============================================
// FUNCTION PROTOTYPES
// ============================================

void initESPNow();
void sendJoystickData();
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
bool parseMAC(const char* str, uint8_t* macAddr);
void handleSerialCommands();

#endif // ESPNOW_H
