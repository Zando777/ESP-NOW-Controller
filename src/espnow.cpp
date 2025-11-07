#include "espnow.h"
#include "config.h"
#include <Arduino.h>

// ============================================
// GLOBAL VARIABLES
// ============================================

uint8_t receiverMAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast by default
JoystickData joystickData;
bool espNowReady = false;
unsigned long lastSendTime = 0;
int lastSendStatus = -1;

// ============================================
// FUNCTION IMPLEMENTATIONS
// ============================================

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  lastSendStatus = (status == ESP_NOW_SEND_SUCCESS) ? 0 : 1;
}

void initESPNow() {
  // Set device as WiFi station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    espNowReady = false;
    return;
  }
  
  // Register send callback
  esp_now_register_send_cb(onDataSent);
  
  // Add peer (broadcast or specific MAC)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    espNowReady = false;
    return;
  }
  
  espNowReady = true;
  Serial.println("ESP-NOW initialized successfully");
  printESPNowStatus();
}

void sendJoystickData() {
  if (!espNowReady) return;
  
  unsigned long now = millis();
  if (now - lastSendTime < SEND_INTERVAL) return;
  lastSendTime = now;
  
  extern int leftX, leftY, rightX, rightY;
  extern bool leftButton, rightButton, auxSwitch;
  
  // Update data structure
  joystickData.leftX = leftX;
  joystickData.leftY = leftY;
  joystickData.rightX = rightX;
  joystickData.rightY = rightY;
  joystickData.leftButton = leftButton;
  joystickData.rightButton = rightButton;
  joystickData.auxSwitch = auxSwitch;
  
  // Send data
  esp_now_send(receiverMAC, (uint8_t *) &joystickData, sizeof(joystickData));
}

bool parseMAC(const char* str, uint8_t* macAddr) {
  int parts[6];
  if (sscanf(str, "%02x:%02x:%02x:%02x:%02x:%02x", 
             &parts[0], &parts[1], &parts[2], 
             &parts[3], &parts[4], &parts[5]) != 6) {
    return false;
  }
  for (int i = 0; i < 6; i++) {
    macAddr[i] = parts[i];
  }
  return true;
}

void printESPNowStatus() {
  Serial.print("Receiver MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", receiverMAC[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

void handleSerialCommands() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd.startsWith("MAC:")) {
      // Set receiver MAC address: MAC:AA:BB:CC:DD:EE:FF
      String macStr = cmd.substring(4);
      if (parseMAC(macStr.c_str(), receiverMAC)) {
        Serial.print("Receiver MAC set to: ");
        for (int i = 0; i < 6; i++) {
          Serial.printf("%02X", receiverMAC[i]);
          if (i < 5) Serial.print(":");
        }
        Serial.println();
        
        // Re-initialize ESP-NOW with new MAC
        if (espNowReady) {
          esp_now_deinit();
        }
        initESPNow();
      } else {
        Serial.println("Invalid MAC format. Use: MAC:AA:BB:CC:DD:EE:FF");
      }
    } 
    else if (cmd == "GETMAC") {
      // Print current MAC address
      Serial.print("Current Receiver MAC: ");
      for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", receiverMAC[i]);
        if (i < 5) Serial.print(":");
      }
      Serial.println();
    }
    else if (cmd == "BROADCAST") {
      // Set to broadcast mode
      receiverMAC[0] = 0xFF;
      receiverMAC[1] = 0xFF;
      receiverMAC[2] = 0xFF;
      receiverMAC[3] = 0xFF;
      receiverMAC[4] = 0xFF;
      receiverMAC[5] = 0xFF;
      Serial.println("Broadcasting enabled (FF:FF:FF:FF:FF:FF)");
      
      // Re-initialize ESP-NOW
      if (espNowReady) {
        esp_now_deinit();
      }
      initESPNow();
    }
    else if (cmd == "HELP") {
      Serial.println("\n=== ESP-NOW Controller Commands ===");
      Serial.println("MAC:AA:BB:CC:DD:EE:FF - Set receiver MAC address");
      Serial.println("GETMAC                 - Print current receiver MAC");
      Serial.println("BROADCAST              - Enable broadcast mode");
      Serial.println("HELP                   - Show this help message");
      Serial.println("===================================\n");
    }
  }
}
