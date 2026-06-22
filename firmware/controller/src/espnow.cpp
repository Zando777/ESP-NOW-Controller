#include "espnow.h"
#include "config.h"
#include "joystick.h"
#include "calibration.h"
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// ============================================
// DEVICE LIST (static)
// ============================================
// Add controllable devices here. channel 0 = unknown (found by sweep).
ControlDevice devices[] = {
  {"Mecanum", {0x00, 0x70, 0x07, 0x84, 0x9E, 0xB0}, 0, false},
  {"Test Rx", {0x88, 0x56, 0xA6, 0x64, 0xA1, 0xE8}, 0, false},
};
int numDevices = sizeof(devices) / sizeof(devices[0]);
int selectedDevice = 0;

// ============================================
// GLOBAL STATE
// ============================================
bool espNowReady = false;
String lastSendStatus = "Not sent";

static unsigned long lastSendTime = 0;
static uint8_t txSeq = 0;

// Time of the most recent successful delivery (ACK). Used to decide when the
// link is genuinely dead vs. just dropping the odd ACK.
static volatile unsigned long lastSuccessMs = 0;

// Probe result for channel sweep, written by the send callback.
static volatile bool probeDone = false;
static volatile bool probeOk = false;

// ============================================
// SEND CALLBACK
// ============================================
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  bool ok = (status == ESP_NOW_SEND_SUCCESS);
  probeOk = ok;
  probeDone = true;
  lastSendStatus = ok ? "Delivery Success" : "Delivery Failed";
  if (ok) lastSuccessMs = millis();
}

// ============================================
// PEER MANAGEMENT
// ============================================
// Peer is registered with channel 0 (use current radio channel); the radio
// channel is set explicitly with esp_wifi_set_channel before sending.
static void setPeer(const uint8_t *mac) {
  if (esp_now_is_peer_exist(mac)) return;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

// Set the radio channel and wait until it actually takes effect. Sending a
// probe before the channel switch completes makes the sweep lock the wrong
// channel (off by one), so confirm via read-back.
static void setRadioChannel(uint8_t ch) {
  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
  uint8_t cur = 0;
  wifi_second_chan_t sc;
  for (int i = 0; i < 20; i++) {
    esp_wifi_get_channel(&cur, &sc);
    if (cur == ch) break;
    delay(5);
  }
  delay(10);  // small extra settle for the PHY
}

static bool sendProbe(const uint8_t *mac) {
  ControlCommand cmd = {};
  cmd.version = CONTROL_PROTOCOL_VERSION;
  cmd.seq = txSeq++;
  cmd.speed = 0;  // zeroed motion
  probeDone = false;
  probeOk = false;
  if (esp_now_send(mac, (uint8_t *)&cmd, sizeof(cmd)) != ESP_OK) return false;
  unsigned long start = millis();
  while (!probeDone && millis() - start < 50) {
    delay(1);
  }
  return probeDone && probeOk;
}

// Sweep channels 1..13 to find the one the device is reachable on.
// Returns the channel, or 0 if not found.
// Note: allow the radio to settle on the new channel before probing, otherwise
// the probe transmits on the previous channel and the ACK is missed.
static uint8_t sweepChannel(const uint8_t *mac) {
  for (uint8_t ch = 1; ch <= 13; ch++) {
    setRadioChannel(ch);
    // Two attempts per channel for robustness against a single dropped frame.
    if (sendProbe(mac) || sendProbe(mac)) {
      Serial.printf("[ESP-NOW] Found device on channel %d\n", ch);
      return ch;
    }
  }
  Serial.println("[ESP-NOW] Channel sweep found no device");
  return 0;
}

// ============================================
// PUBLIC API
// ============================================
bool selectDevice(int index) {
  if (index < 0 || index >= numDevices) return false;

  // Remove the previous peer so only the active device is registered.
  if (selectedDevice >= 0 && selectedDevice < numDevices) {
    esp_now_del_peer(devices[selectedDevice].mac);
  }
  selectedDevice = index;

  if (!espNowReady) return false;

  ControlDevice &dev = devices[index];
  setPeer(dev.mac);

  // Use cached channel if known, else sweep.
  if (dev.channel != 0) {
    setRadioChannel(dev.channel);
    if (sendProbe(dev.mac) || sendProbe(dev.mac)) {
      dev.linkOk = true;
      lastSuccessMs = millis();
      Serial.printf("[ESP-NOW] Selected %s on cached channel %d\n", dev.name, dev.channel);
      return true;
    }
  }

  uint8_t ch = sweepChannel(dev.mac);
  if (ch != 0) {
    dev.channel = ch;
    dev.linkOk = true;
    lastSuccessMs = millis();
    Serial.printf("[ESP-NOW] Selected %s, locked channel %d\n", dev.name, ch);
    return true;
  }
  dev.linkOk = false;
  Serial.printf("[ESP-NOW] Selected %s but link not found\n", dev.name);
  return false;
}

void initESPNow() {
  // STA mode but NOT associated to any AP, so we can freely set the radio
  // channel to match whichever device we are talking to.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  esp_wifi_set_ps(WIFI_PS_NONE);

  Serial.print("[ESP-NOW] Controller MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] init FAILED");
    espNowReady = false;
    return;
  }
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
  espNowReady = true;
  Serial.println("[ESP-NOW] Ready");

  // Lock onto the default device.
  selectDevice(selectedDevice);
}

void sendControlCommand() {
  if (!espNowReady) return;

  unsigned long now = millis();
  if (now - lastSendTime < SEND_INTERVAL) return;
  lastSendTime = now;

  extern int leftX, leftY, rightX, rightY;
  extern bool leftButton, rightButton, auxSwitch;
  extern CalibrationData calibration;

  ControlCommand cmd;
  cmd.version = CONTROL_PROTOCOL_VERSION;
  cmd.seq = txSeq++;
  cmd.x   = SIGN_X   * mapAxisSigned(leftX,  calibration.leftXMin,  calibration.leftXCenter,  calibration.leftXMax);
  cmd.y   = SIGN_Y   * mapAxisSigned(leftY,  calibration.leftYMin,  calibration.leftYCenter,  calibration.leftYMax);
  cmd.rot = SIGN_ROT * mapAxisSigned(rightX, calibration.rightXMin, calibration.rightXCenter, calibration.rightXMax);
  // AUX engaged = speed boost (double, capped at the 255 PWM ceiling).
  cmd.speed = auxSwitch ? (uint8_t)min(CONTROL_DEFAULT_SPEED * 2, 255)
                        : CONTROL_DEFAULT_SPEED;
  cmd.buttons = (leftButton ? 0x01 : 0) | (rightButton ? 0x02 : 0) | (auxSwitch ? 0x04 : 0);

  uint8_t *mac = devices[selectedDevice].mac;
  esp_now_send(mac, (uint8_t *)&cmd, sizeof(cmd));

  // Stable link indicator: linked if we have had an ACK recently. Individual
  // dropped ACKs do not mean the command was lost (the robot still receives
  // the data), so do NOT react to them.
  devices[selectedDevice].linkOk = (now - lastSuccessMs < LINK_OK_MS);

  // Re-acquire the channel ONLY after a sustained silence (the device was
  // powered off, moved channel, or went out of range). Try the current
  // channel first (instant) before falling back to a full sweep, so a brief
  // drop does not cause a multi-second blocking sweep mid-drive.
  static unsigned long lastReacquireMs = 0;
  if (now - lastSuccessMs > LINK_DEAD_MS && now - lastReacquireMs > LINK_DEAD_MS) {
    lastReacquireMs = now;
    Serial.println("[ESP-NOW] Link silent, re-acquiring...");
    if (sendProbe(mac) || sendProbe(mac)) {
      lastSuccessMs = millis();  // still here, transient drop
    } else {
      uint8_t ch = sweepChannel(mac);
      if (ch != 0) {
        devices[selectedDevice].channel = ch;
        lastSuccessMs = millis();
      }
    }
  }
}

void handleSerialCommands() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toUpperCase();

  if (cmd == "STATUS") {
    Serial.println("\n=== ESP-NOW Status ===");
    Serial.printf("Ready: %s\n", espNowReady ? "YES" : "NO");
    ControlDevice &d = devices[selectedDevice];
    Serial.printf("Device: %s  ch:%d  link:%s\n",
      d.name, d.channel, d.linkOk ? "OK" : "--");
    Serial.printf("Last: %s\n", lastSendStatus.c_str());
    Serial.println("====================\n");
  } else if (cmd == "LIST") {
    Serial.println("\n=== Devices ===");
    for (int i = 0; i < numDevices; i++) {
      Serial.printf("%s%d) %s  ch:%d  link:%s\n",
        i == selectedDevice ? "* " : "  ",
        i, devices[i].name, devices[i].channel,
        devices[i].linkOk ? "OK" : "--");
    }
    Serial.println("===============\n");
  } else if (cmd.startsWith("SELECT ")) {
    int idx = cmd.substring(7).toInt();
    if (selectDevice(idx)) Serial.printf("Selected device %d\n", idx);
    else Serial.println("Select failed");
  } else if (cmd == "HELP") {
    Serial.println("\n=== Commands ===");
    Serial.println("STATUS    - link status");
    Serial.println("LIST      - list devices");
    Serial.println("SELECT n  - select device n");
    Serial.println("================\n");
  }
}
