# ESP-NOW Multi-Device Control — Design

Date: 21/06/2026

## Goal

Extend the ESP-NOW Controller (ESP32-C3, OLED + dual joysticks) so it can:

1. Drive the Mini Mecanum ESP32 robot over ESP-NOW.
2. Hold a static list of controllable devices and let the operator cycle through
   and select which one to drive, from the controller's OLED.

The Mecanum robot keeps its existing WiFi STA + web server + OTA so it can still
be flashed over the air; ESP-NOW runs alongside on the router-assigned channel.

## Affected projects

- `ESP-NOW Controller/firmware/controller/` — controller firmware (main work).
- `ESP-NOW Controller/firmware/shared/espnow_data.h` — shared protocol struct.
- `ESP-NOW Controller/firmware/receiver/` — bench test target, updated to the new packet.
- `Mini Mecanum ESP32/src/main.cpp` — robot, additive ESP-NOW receive path + failsafe.

## 1. Protocol

A single device-agnostic command packet replaces the raw `JoystickData` packet.
Defined in `firmware/shared/espnow_data.h` and copied byte-identically into the
Mecanum project (the projects are separate repos and cannot share an include path).

```c
typedef struct __attribute__((packed)) {
  uint8_t version;   // protocol version, starts at 1
  uint8_t seq;       // rolling counter, wraps at 255; for debug / loss detection
  int8_t  x;         // strafe    -100..100  (left .. right)
  int8_t  y;         // forward   -100..100  (back .. forward)
  int8_t  rot;       // rotation  -100..100  (CCW .. CW)
  uint8_t speed;     // master speed 0..255
  uint8_t buttons;   // bit0=leftBtn, bit1=rightBtn, bit2=aux
} ControlCommand;     // 8 bytes
```

`version` lets receivers reject mismatched firmware. The robot converts
`x/y/rot` to its float control inputs by dividing by 100.0 and clamping to
[-1.0, 1.0], and uses `speed` as `robotSpeed`.

### Joystick mapping (controller)

Translation and rotation are split across the two sticks, following the standard
twin-stick mecanum convention used in FRC/FTC and most game-pad robot control:

- **Left stick = translation:** `y` = left-stick Y (forward/back), `x` = left-stick
  X (strafe left/right).
- **Right stick = rotation:** `rot` = right-stick X (yaw turn). Right-stick Y
  reserved (unused in v1).
- `speed`: fixed at 200 in v1. Proportional control already comes from stick
  magnitude (x/y/rot are continuous -100..100), so a fixed master cap is enough.
  `speed` is a named constant so it is trivial to change or wire to an axis later.
- `buttons`: left SW, right SW, aux toggle packed into the bitfield.

This matches the Mecanum robot's existing wheel-mixing in `calculateMotorSpeeds()`,
which is the standard mecanum formula, so the controller sends `x/y/rot` and the
robot's kinematics is unchanged:

```
FL = y + x + rot     FR = y - x - rot
BL = y - x + rot     BR = y + x - rot
```

Existing calibration and ADC→range mapping in the controller is reused to produce
the -100..100 values (currently it maps to 0..58 bar units; a -100..100 signed
mapping helper is added alongside, the existing bar mapping is left for the UI).

Reference: standard two-joystick mecanum control — left stick translates, right
stick turns (gm0.org Mecanum drive tutorial; Chief Delphi).

## 2. Multi-device list and selection (controller)

Static table compiled into the controller firmware:

```c
struct ControlDevice {
  const char* name;     // shown on OLED
  uint8_t     mac[6];   // peer MAC
  uint8_t     channel;  // last-known WiFi channel; 0 = unknown -> auto-find (sect. 3)
};
```

Initial entries:

- `Mecanum` — MAC captured from the robot's boot log (the robot prints
  `WiFi.macAddress()` in setup; filled into the table during implementation).
- `Test Rx` — `88:56:A6:64:A1:E8` (existing receiver board, for bench testing).

`selectedDevice` index defaults to 0. Cached channels may optionally be persisted
to NVS later; v1 keeps them in RAM (re-found on boot via sweep).

### Controller state machine

- **DRIVE** (default): reads joysticks, builds a `ControlCommand`, sends to the
  selected device every `SEND_INTERVAL` (20 ms / 50 Hz). OLED shows the existing
  driving bars plus the selected device name and link status (OK / lost from the
  send callback).
- **SELECT**: entered by holding the right joystick button. While held, the OLED
  shows the device list with a highlight; tilting the joystick up/down moves the
  highlight (with debounce so one tilt = one step). Releasing the button selects
  the highlighted device, returns to DRIVE, and triggers a channel sync for the
  newly selected device.

While in SELECT, the controller stops sending drive commands (robot failsafe
stops it — see section 4).

## 3. Channel sync

ESP-NOW requires transmitter and receiver to be on the same WiFi channel. The
robot's channel is fixed by the router it is connected to and is not known to the
controller in advance. The controller is not connected to any AP, so it can set
its radio channel freely with `esp_wifi_set_channel()`.

Sync algorithm, run when a device is selected and when sends start failing
(N consecutive `OnDataSent` failures):

1. For `ch` in 1..13:
   - `esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE)`.
   - Update the peer entry's channel and send a probe `ControlCommand` (zeroed
     motion) to the target MAC.
   - Wait briefly for the `OnDataSent` result. On success, lock `ch`, cache it in
     the device entry, stop sweeping.
2. If no channel succeeds, mark the device link as "lost"; retry on the next
   selection or send-failure trigger.

Because the controller drives one device at a time, it simply parks its radio on
the selected device's channel. Re-selecting a device with a cached non-zero
channel sets that channel directly and only falls back to a sweep on failure.

## 4. Robot side (Mini Mecanum ESP32)

Additive change to `src/main.cpp`; WiFi STA, web server and OTA are untouched and
keep working on the same channel.

- In `setup()` (after WiFi is up): `esp_now_init()` and register a receive callback.
  No peer needs to be added for one-way receive.
- Receive callback: validate length and `version`, then set
  `moveX = x/100.0`, `moveY = y/100.0`, `moveRot = rot/100.0`, `robotSpeed = speed`,
  and record `lastPacketMs = millis()`.
- **Failsafe** in `loop()`: if `millis() - lastPacketMs > FAILSAFE_MS` (400 ms),
  force `moveX = moveY = moveRot = 0`. The robot currently has no timeout — the
  last command persists indefinitely — so this is required for safe link loss.

The existing pipeline (`calculateMotorSpeeds` → `driveMotor`, recomputed every
loop from the globals) is unchanged.

### Bench receiver

`firmware/receiver/src/main.cpp` is updated to parse `ControlCommand` and print
its fields, so the link and channel sync can be tested without the robot powered.

## Error handling

- Wrong packet size or version on a receiver: ignore the packet (no state change).
- Controller send failures: surface as "link lost" on OLED; trigger channel
  re-sync after N consecutive failures.
- Robot link loss: failsafe stop after 400 ms of no packets.
- ESP-NOW init failure on either side: logged over serial; device is non-functional
  for wireless but does not crash.

## Testing

1. Build all three firmwares.
2. Controller (flash over USB, port `/dev/cu.usbmodem101`):
   - DRIVE UI shows selected device + link status.
   - Hold right button → SELECT list appears; tilt scrolls; release picks.
   - Channel sweep finds the `Test Rx` board (delivery success); receiver prints
     decoded `ControlCommand` matching stick movement.
3. Robot (build; flash via OTA or USB):
   - Receive callback updates motion globals; wheels respond to sticks.
   - Stop sending → robot stops within ~400 ms (failsafe).
4. Multi-device: with both `Test Rx` and `Mecanum` powered on different channels,
   selecting each parks the controller on the right channel and controls only the
   selected device.

## Out of scope (v1)

- Dynamic broadcast discovery of devices.
- Two-way telemetry from robot to controller (beyond ESP-NOW send-status).
- Persisting the device list / channels to NVS.
- Per-device control profiles (all devices use the same ControlCommand mapping).
