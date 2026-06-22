#ifndef ESPNOW_H
#define ESPNOW_H

#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>
#include "config.h"

// ============================================
// ESP-NOW CONTROL PROTOCOL
// ============================================
// ControlCommand must stay byte-identical to the receiver firmware
// (Mini Mecanum ESP32). Sent controller -> device at ~50 Hz.
typedef struct __attribute__((packed)) {
  uint8_t version;   // protocol version
  uint8_t seq;       // rolling counter, for debug / loss detection
  int8_t  x;         // strafe    -100..100 (left .. right)
  int8_t  y;         // forward   -100..100 (back .. forward)
  int8_t  rot;       // rotation  -100..100 (CCW .. CW)
  uint8_t speed;     // master speed 0..255
  uint8_t buttons;   // bit0=leftBtn, bit1=rightBtn, bit2=aux
} ControlCommand;     // 7 bytes packed

// A controllable device in the static list.
struct ControlDevice {
  const char* name;     // shown on OLED
  uint8_t     mac[6];   // peer MAC
  uint8_t     channel;  // last-known WiFi channel; 0 = unknown -> sweep
  bool        linkOk;   // last send delivered
};

// ============================================
// GLOBAL VARIABLES
// ============================================

extern ControlDevice devices[];
extern int numDevices;
extern int selectedDevice;
extern bool espNowReady;
extern String lastSendStatus;

// ============================================
// FUNCTION PROTOTYPES
// ============================================

void initESPNow();
void sendControlCommand();            // build from joysticks + send to selected device
bool selectDevice(int index);         // switch peer + lock channel (sweeps if unknown)
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void handleSerialCommands();

#endif // ESPNOW_H
