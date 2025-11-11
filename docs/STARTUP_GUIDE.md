# ESP-NOW Controller & Receiver Startup Guide

This guide will help you set up and test the ESP-NOW wireless controller system using two ESP32 devices.

## 📋 Hardware Requirements

### Required Hardware
- **2x ESP32-C3 DevKitM-1** (or compatible ESP32 boards)
- **Controller Device**: KY-023 joystick module + OLED display (SSD1306)
- **Receiver Device**: ESP32 board only (no additional hardware needed)
- **2x USB cables** for programming and power
- **Power supplies** or USB ports for both devices

### Optional Hardware
- **Breadboard and jumper wires** (for prototyping)
- **External antennas** (for extended range)

## 🔧 Software Setup

### Prerequisites
1. **PlatformIO** installed (VS Code extension or CLI)
2. **Git** for cloning repositories
3. **Two ESP32 boards** ready for programming

### Step 1: Clone the Repository
```bash
git clone https://github.com/alex-algo-maker/ESP-NOW-Controller.git
cd ESP-NOW-Controller
```

## 🎮 Controller Setup (Sender)

### Hardware Connections
Connect the KY-023 joystick and OLED to your first ESP32:

| Component | ESP32 Pin | Description |
|-----------|-----------|-------------|
| **OLED SDA** | GPIO 8 | I2C Data |
| **OLED SCL** | GPIO 9 | I2C Clock |
| **OLED VCC** | 3.3V | Power |
| **OLED GND** | GND | Ground |
| **Left Joystick VRX** | GPIO 4 | Left X-axis |
| **Left Joystick VRY** | GPIO 3 | Left Y-axis |
| **Left Joystick SW** | GPIO 5 | Left button |
| **Right Joystick VRX** | GPIO 2 | Right X-axis |
| **Right Joystick VRY** | GPIO 1 | Right Y-axis |
| **Right Joystick SW** | GPIO 0 | Right button |
| **Aux Switch** | GPIO 7 | Auxiliary button |

### Upload Controller Code
```bash
# Build and upload to first ESP32
pio run --target upload --environment esp32-c3-devkitm-1
```

### Controller Startup
1. **Open Serial Monitor:**
   ```bash
   pio device monitor
   ```

2. **Expected Output:**
   ```
   ========================================
   ESP-NOW Controller Started
   ========================================
   Pin Configuration:
     OLED: SDA=GPIO8, SCL=GPIO9
     Left Joystick: VRX=GPIO4, VRY=GPIO3, SW=GPIO5
     Right Joystick: VRX=GPIO2, VRY=GPIO1, SW=GPIO0
     Aux Switch: GPIO7
   ========================================
   No calibration found in NVS. Using defaults.
   Hold both joystick buttons for 5 seconds to calibrate.
   ESP-NOW initialized successfully
   Receiver MAC: FF:FF:FF:FF:FF:FF
   Setup complete. Ready for operation.
   Type 'HELP' for available commands.
   ```

## 📡 Receiver Setup

### Upload Receiver Code
```bash
# Navigate to receiver directory
cd receiver

# Build and upload to second ESP32
pio run --target upload --environment esp32-c3-devkitm-1
```

### Receiver Startup
1. **Open Serial Monitor for Receiver:**
   ```bash
   pio device monitor
   ```

2. **Expected Output:**
   ```
   ========================================
   ESP-NOW Receiver Started
   ========================================
   Receiver MAC Address: XX:XX:XX:XX:XX:XX
   ESP-NOW initialized successfully
   Waiting for joystick data...
   ========================================
   ```

## 🧪 Testing & Calibration

### Step 1: Basic Connection Test
1. **Power on both devices**
2. **Move joysticks on controller** - you should see data appear on receiver serial monitor
3. **Press buttons** - button states should update on receiver

### Step 2: Controller Calibration (Important!)
The joysticks need calibration for accurate readings:

1. **Hold both joystick buttons** for 5 seconds until calibration starts
2. **Follow OLED instructions:**
   - Move LEFT joystick fully UP, press RIGHT button
   - Move LEFT joystick fully DOWN, press RIGHT button
   - Move LEFT joystick fully LEFT, press RIGHT button
   - Move LEFT joystick fully RIGHT, press RIGHT button
   - Move RIGHT joystick fully UP, press LEFT button
   - Move RIGHT joystick fully DOWN, press LEFT button
   - Move RIGHT joystick fully LEFT, press LEFT button
   - Move RIGHT joystick fully RIGHT, press LEFT button
   - Center BOTH joysticks, press AUX switch

3. **Calibration saves automatically** to ESP32 flash memory

### Step 3: Verify Data Transmission
Move joysticks and observe receiver output:
```
----------------------------------------
Packet #45 - Time: 12345 ms

[Left Joystick]
  X: 2048
  Y: 1024
  Button: RELEASED

[Right Joystick]
  X: 3072
  Y: 512
  Button: PRESSED

[Aux Switch]
  State: OFF

Last packet received 23 ms ago
✓ Connection OK
```

## 🔧 Advanced Configuration

### Setting Specific Receiver MAC
If you want the controller to send to a specific receiver:

1. **Get receiver MAC** from receiver startup message
2. **Send command to controller:**
   ```
   MAC:XX:XX:XX:XX:XX:XX
   ```
   Replace `XX:XX:XX:XX:XX:XX` with actual receiver MAC

3. **Controller response:**
   ```
   Receiver MAC set to: XX:XX:XX:XX:XX:XX
   ```

### Available Serial Commands (Controller)
```
MAC:AA:BB:CC:DD:EE:FF - Set receiver MAC address
GETMAC                 - Print current receiver MAC
BROADCAST              - Enable broadcast mode (default)
HELP                   - Show command help
```

## 🚨 Troubleshooting

### No Data on Receiver
1. **Check power** - Both devices powered on?
2. **Check range** - Devices within ~30m of each other?
3. **Check MAC addresses** - Controller broadcasting or set to correct receiver MAC?
4. **Check serial output** - Any ESP-NOW initialization errors?

### Joystick Values Stuck at 4095
1. **Calibrate joysticks** - Required for proper ADC readings
2. **Check connections** - Joystick pins correctly wired?
3. **Check power** - Joystick getting 3.3V?

### OLED Not Working
1. **Check I2C address** - Should be 0x3C (default)
2. **Check connections** - SDA/SCL pins correct?
3. **Check power** - OLED getting 3.3V?

### Compilation Errors
1. **Check PlatformIO** - Properly installed?
2. **Check board** - ESP32-C3 selected in platformio.ini?
3. **Check libraries** - Adafruit libraries installed?

### Connection Lost Warnings
- **High latency**: Move devices closer together
- **Connection lost**: Check power, range, or restart devices

## 📊 Expected Performance

- **Update Rate**: 50Hz (20ms intervals)
- **Latency**: <50ms typical
- **Range**: ~30m indoor, ~100m line-of-sight
- **Power**: ~80mA per device
- **Data**: 7 values (4 analog + 3 digital)

## 🎯 Usage Examples

### Robot Control
Use receiver data to control motors/servos:
```cpp
// In receiver loop()
if (receivedData.leftY > 3000) {
  // Move forward
} else if (receivedData.leftY < 1000) {
  // Move backward
}
```

### Game Controller
Send data to PC via serial for gaming:
```python
# Python script to read serial data
import serial
ser = serial.Serial('/dev/ttyUSB0', 115200)
while True:
    line = ser.readline().decode().strip()
    if 'X:' in line:
        # Parse and use joystick data
```

### Data Logging
Log controller inputs to SD card for analysis.

## 🔄 Next Steps

1. **Test basic functionality** ✓
2. **Calibrate joysticks** ✓
3. **Verify reliable connection** ✓
4. **Integrate with your project** (robot, game, etc.)

## 📞 Support

If you encounter issues:
1. Check serial output for error messages
2. Verify hardware connections
3. Test with known working ESP32 boards
4. Check ESP-NOW range and interference

The system is now ready for wireless control applications!