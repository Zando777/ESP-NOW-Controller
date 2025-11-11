#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

typedef struct {
  int leftX, leftY, rightX, rightY;
  bool leftButton, rightButton, auxSwitch;
} JoystickData;

JoystickData receivedData = {0};
bool dataReceived = false;
int packetCount = 0;

// Controller MAC address (to send data back)
uint8_t controllerMAC[6] = {0xEC, 0xDA, 0x3B, 0xBD, 0xCD, 0x74};

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  Serial.printf("\n🔔 [CALLBACK] Received %d bytes from controller\n", len);
  Serial.flush();
  
  if (len == sizeof(JoystickData)) {
    memcpy(&receivedData, incomingData, sizeof(JoystickData));
    dataReceived = true;
    packetCount++;
    
    Serial.printf("✅ [PACKET #%d] L(%d,%d) R(%d,%d) Btn(%d,%d,%d)\n", 
      packetCount,
      receivedData.leftX, receivedData.leftY, 
      receivedData.rightX, receivedData.rightY,
      receivedData.leftButton, receivedData.rightButton, receivedData.auxSwitch);
    Serial.flush();
  } else {
    Serial.printf("❌ [ERROR] Wrong packet size: got %d, expected %zu\n", len, sizeof(JoystickData));
    Serial.flush();
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.printf("[SENT] ✅ Sent successfully to controller\n");
  } else {
    Serial.printf("[SENT] ❌ Failed to send to controller\n");
  }
}

void setup() {
  // 10-second delay to allow serial monitor to connect
  delay(10000);
  
  Serial.begin(115200);
  delay(500);
  
  while (Serial.available()) Serial.read();
  
  Serial.println("\n\n========================================");
  Serial.println("ESP-NOW RECEIVER - Starting Setup");
  Serial.println("========================================\n");
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  Serial.print("[INIT] Receiver MAC: ");
  Serial.println(WiFi.macAddress());
  
  Serial.println("[INIT] Initializing ESP-NOW...");
  esp_err_t initErr = esp_now_init();
  Serial.printf("[INIT] esp_now_init() result: %d (0 = success)\n", initErr);
  
  if (initErr != ESP_OK) {
    Serial.println("[ERROR] ❌ ESP-NOW initialization failed!");
    return;
  }
  
  Serial.println("[INIT] ✅ ESP-NOW core initialized successfully");
  
  esp_err_t recvErr = esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  Serial.printf("[INIT] Register recv callback result: %d (0 = success)\n", recvErr);
  
  if (recvErr != ESP_OK) {
    Serial.println("[ERROR] ❌ Failed to register receive callback");
    return;
  }
  
  Serial.println("[INIT] ✅ Receive callback registered");
  
  esp_err_t sendErr = esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
  Serial.printf("[INIT] Register send callback result: %d (0 = success)\n", sendErr);
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, controllerMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  Serial.printf("[INIT] Adding controller peer: %02X:%02X:%02X:%02X:%02X:%02X\n",
    controllerMAC[0], controllerMAC[1], controllerMAC[2],
    controllerMAC[3], controllerMAC[4], controllerMAC[5]);
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[INIT] ⚠️  Note: Peer already exists or other issue");
  } else {
    Serial.println("[INIT] ✅ Controller peer added");
  }
  
  Serial.println("\n========================================");
  Serial.println("✅ RECEIVER READY");
  Serial.println("Waiting for joystick data from controller...");
  Serial.println("========================================\n");
}

void loop() {
  static unsigned long lastPrint = 0;
  
  if (!dataReceived) {
    if (millis() - lastPrint > 5000) {
      Serial.println("⏳ [STATUS] No packets received yet - waiting...");
      lastPrint = millis();
    }
  } else {
    dataReceived = false;
  }
  
  delay(100);
}
