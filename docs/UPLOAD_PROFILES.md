# ESP-NOW Controller - Simple Upload Setup

## 🎯 Simplified Upload Process

**Good news!** The upload process is now much simpler with broadcast mode. No more MAC address configuration or device switching complexity.

### What's Changed:
- **Broadcast Mode**: Controller sends to all devices automatically
- **Simple Script**: One command uploads both devices
- **No MAC Configuration**: Works out of the box

## 🚀 Quick Start Upload

### Step 1: Run the Simple Upload Script

```bash
./upload.sh
```

**Interactive Mode** (recommended):
- Guides you through connecting devices one at a time
- Shows clear status messages and device detection
- Allows you to change devices between uploads

**Command Line Options:**
```bash
./upload.sh --help          # Show all options
./upload.sh --controller    # Upload only controller
./upload.sh --receiver      # Upload only receiver  
./upload.sh --auto          # Non-interactive mode
```

The script will guide you through:
1. Connect controller device → Press Enter → Uploads automatically
2. Connect receiver device → Press Enter → Uploads automatically
3. Done! 🎉

### Step 2: Test the System

1. **Power on both devices**
2. **Controller** shows joystick data on OLED display
3. **Receiver** shows received data in serial monitor

## 📋 Manual Upload (if needed)

If the script doesn't work:

```bash
# Upload only controller
./upload.sh --controller

# Upload only receiver  
./upload.sh --receiver

# Or manual commands:
cd firmware/controller && pio run --target upload
cd firmware/receiver && pio run --target upload
```

## � Troubleshooting

### "No device detected"
- Ensure ESP32 is properly connected via USB
- Try a different USB port
- Check USB cable

### "Upload failed"
- Make sure the correct device is connected for each step
- Try power-cycling the ESP32
- Check serial monitor for error messages

### Communication not working
- Both devices must be powered on and in range
- Check serial output for ESP-NOW status
- Use `GETSTATUS` command in controller serial monitor

## 📡 ESP-NOW Details

- **Mode**: Broadcast (FF:FF:FF:FF:FF:FF)
- **Channel**: 1
- **Frequency**: 50Hz (20ms intervals)
- **Range**: ~100-200 meters (line of sight)

## 🎯 Next Steps

1. Run `./upload.sh`
2. Power on both devices
3. Move joysticks and see data transfer!
4. Use serial monitor to debug if needed