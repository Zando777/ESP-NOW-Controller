#include "espnow.h"
#include "config.h"
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// ============================================
// MAC ADDRESSES (confirmed via MAC reader)
// ============================================
// Controller (this device, cu.usbmodem141401): ec:da:3b:bd:cd:74
// Receiver (target device, cu.usbmodem141201): 88:56:a6:64:a1:e8

uint8_t receiverMAC[6] = {0x88, 0x56, 0xA6, 0x64, 0xA1, 0xE8};

// ============================================
// GLOBAL VARIABLES
// ============================================
JoystickData joystickData = {0};
bool espNowReady = false;
unsigned long lastSendTime = 0;
String lastSendStatus = "Not sent";

// ============================================
// CALLBACK: Data Sent
// ============================================
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    lastSendStatus = "Delivery Success";
  } else {
    lastSendStatus = "Delivery Failed";
  }
}

// ============================================
// CALLBACK: Data Received (for two-way comm)
// ============================================
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  Serial.printf("[RECV] Received %d bytes from receiver\n", len);
  if (len == sizeof(JoystickData)) {
    JoystickData receivedData;
    memcpy(&receivedData, incomingData, sizeof(JoystickData));
    Serial.printf("[RECV] L(%d,%d) R(%d,%d)\n", 
      receivedData.leftX, receivedData.leftY,
      receivedData.rightX, receivedData.rightY);
  }
}

void initESPNow() {
  Serial.println("\n\n🔧 [ESPNOW_INIT] Starting initialization...");
  Serial.flush();
  
  Serial.println("[ESPNOW_INIT] Step 1: Setting WiFi mode to STATION");
  WiFi.mode(WIFI_STA);
  Serial.println("[ESPNOW_INIT] ✅ WiFi mode set");
  Serial.flush();
  
  Serial.print("[ESPNOW_INIT] Controller MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.flush();
  
  Serial.println("[ESPNOW_INIT] Step 2: Initializing WiFi radio...");
  WiFi.begin();  // Initialize WiFi hardware
  delay(100);
  Serial.println("[ESPNOW_INIT] ✅ WiFi radio initialized");
  Serial.flush();
  
  Serial.println("[ESPNOW_INIT] Step 3: Disabling WiFi power save...");
  esp_wifi_set_ps(WIFI_PS_NONE);
  Serial.println("[ESPNOW_INIT] ✅ WiFi power save disabled");
  Serial.flush();
  
  Serial.println("[ESPNOW_INIT] Step 4: Calling esp_now_init()...");
  Serial.flush();
  delay(50);
  
  esp_err_t initErr = esp_now_init();
  
  Serial.printf("[ESPNOW_INIT] esp_now_init returned: %d\n", initErr);
  Serial.flush();
  
  if (initErr != ESP_OK) {
    Serial.printf("[ESPNOW_INIT] ❌ ESP-NOW initialization FAILED! Error code: %d\n", initErr);
    espNowReady = false;
    return;
  }
  Serial.println("[ESPNOW_INIT] ✅ ESP-NOW core initialized");
  Serial.flush();
  
  Serial.println("[ESPNOW_INIT] Step 5: Registering send callback...");
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
  Serial.println("[ESPNOW_INIT] ✅ Send callback registered");
  Serial.flush();
  
  Serial.println("[ESPNOW_INIT] Step 6: Registering receive callback...");
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  Serial.println("[ESPNOW_INIT] ✅ Receive callback registered");
  Serial.flush();
  
  Serial.println("[ESPNOW_INIT] Step 7: Adding receiver peer...");
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  Serial.printf("[ESPNOW_INIT] Peer MAC: %02X:%02X:%02X:%02X:%02X:%02X, Channel: %d\n",
    receiverMAC[0], receiverMAC[1], receiverMAC[2],
    receiverMAC[3], receiverMAC[4], receiverMAC[5],
    peerInfo.channel);
  Serial.flush();
  
  esp_err_t addPeerErr = esp_now_add_peer(&peerInfo);
  Serial.printf("[ESPNOW_INIT] esp_now_add_peer returned: %d\n", addPeerErr);
  Serial.flush();
  
  if (addPeerErr != ESP_OK) {
    Serial.println("[ESPNOW_INIT] ❌ Failed to add peer");
    espNowReady = false;
    return;
  }
  Serial.println("[ESPNOW_INIT] ✅ Peer added successfully");
  Serial.flush();
  
  espNowReady = true;
  Serial.println("\n✅ [ESPNOW_INIT] COMPLETE - Ready to send!\n");
  Serial.flush();
}

void sendJoystickData() {
  if (!espNowReady) {
    static unsigned long lastErrorPrint = 0;
    if (millis() - lastErrorPrint > 5000) {
      Serial.println("[ERROR] ESP-NOW not ready, cannot send");
      lastErrorPrint = millis();
    }
    return;
  }
  
  // Only send every SEND_INTERVAL milliseconds
  unsigned long now = millis();
  if (now - lastSendTime < SEND_INTERVAL) {
    return;
  }
  lastSendTime = now;
  
  // Get current joystick values
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
  
  // Send data to receiver
  esp_err_t result = esp_now_send(receiverMAC, (uint8_t *) &joystickData, sizeof(joystickData));
  
  // Debug output every 1 second
  static unsigned long lastDebugTime = 0;
  if (now - lastDebugTime > 1000) {
    Serial.printf("[SEND] L(%d,%d) R(%d,%d) Btn(%d,%d,%d) | Status: %s\n",
      leftX, leftY, rightX, rightY, 
      leftButton, rightButton, auxSwitch,
      lastSendStatus.c_str());
    lastDebugTime = now;
  }
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

void handleSerialCommands() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();
    
    if (cmd == "STATUS") {
      Serial.println("\n=== ESP-NOW Status ===");
      Serial.printf("Ready: %s\n", espNowReady ? "YES" : "NO");
      Serial.printf("Last Status: %s\n", lastSendStatus.c_str());
      Serial.printf("Receiver MAC: 88:56:A6:64:A1:E8\n");
      Serial.println("====================\n");
    }
    else if (cmd == "HELP") {
      Serial.println("\n=== ESP-NOW Commands ===");
      Serial.println("STATUS  - Show ESP-NOW status");
      Serial.println("HELP    - Show this help message");
      Serial.println("========================\n");
    }
  }
}
