# Getting Started — The Genesis Radar

## What You Need

### Hardware
See [parts_list.md](../hardware/parts_list.md) for complete BOM.

Minimum to get started:
- ESP32-S3 JC3248W535C 3.5" touchscreen — ~$33 AUD
- DW3000 UWB modules × 5 — ~$225 AUD
- Vivaldi antennas × 5 — ~$60 AUD
- 74HC138 decoder — ~$2 AUD
- Wiring, power bank, mounting — ~$80 AUD

**Total: ~$400 AUD**

### Software
- Arduino IDE 2.x
- ESP32 board support (Espressif)
- LovyanGFX library
- Arduino IDE libraries: ArduinoJson

## Step 1 — Arduino IDE Setup

1. Download Arduino IDE from https://arduino.cc
2. File → Preferences → Additional Board Manager URLs:
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```
3. Tools → Board Manager → search "esp32" → Install Espressif Systems ESP32
4. Tools → Library Manager → Install: **LovyanGFX**

## Step 2 — Board Settings

Tools menu:
- Board: **ESP32S3 Dev Module**
- Flash Size: **16MB**
- PSRAM: **OPI PSRAM**
- USB CDC On Boot: **Enabled**

## Step 3 — Wire the Hardware

See [wiring_diagram.html](../hardware/wiring_diagram.html) for interactive diagram.

Key connections:
```
ESP32-S3 GPIO11 → SPI CLK  (all DW3000 modules)
ESP32-S3 GPIO13 → SPI MOSI (all DW3000 modules)
ESP32-S3 GPIO12 → SPI MISO (all DW3000 modules)
ESP32-S3 GPIO4  → 74HC138 A0
ESP32-S3 GPIO5  → 74HC138 A1
ESP32-S3 GPIO6  → 74HC138 A2
74HC138 Y0-Y4   → DW3000 CS pins (Tx, Rx1, Rx2, Rx3, Rx4)
ESP32-S3 GPIO7  → DW3000 Tx IRQ
ESP32-S3 GPIO8  → DW3000 Rx1 IRQ
ESP32-S3 GPIO9  → DW3000 Rx2 IRQ
ESP32-S3 GPIO10 → DW3000 Rx3 IRQ
ESP32-S3 GPIO14 → DW3000 Rx4 IRQ
```

## Step 4 — Upload Firmware

1. Create a new Arduino sketch folder called **uwb_radar**
2. Copy both firmware files into the folder:
   - uwb_radar_stage1.ino
   - uwb_radar_stage2.ino
3. Open uwb_radar_stage1.ino in Arduino IDE
4. Connect ESP32-S3 via USB-C
5. Select correct COM port
6. Click Upload

## Step 5 — First Boot

On first boot:
1. Splash screen appears
2. Modules initialise (watch Serial Monitor at 115200 baud)
3. **CRITICAL: Clear the area behind the wall**
4. Calibration runs automatically — 50 frames, ~3 seconds
5. Radar goes live

## Step 6 — Calibration

Calibration removes static reflections (wall, furniture).
- Run calibration in an empty room with no people behind wall
- Tap top-right corner of touchscreen to re-calibrate anytime
- Re-calibrate if you move to a new location

## Array Geometry

Mount your 5 DW3000 modules in a cross pattern:
```
        [Rx3]
          |
        40cm
          |
[Rx1]─40cm─[Tx]─40cm─[Rx2]
          |
        40cm
          |
        [Rx4]
```

All Vivaldi antennas pointing forward (toward wall).
Screen on rear face pointing toward operator.

## Troubleshooting

See [troubleshooting.md](troubleshooting.md) for common issues.

Most common issue: **Module not responding**
- Check 3.3V power to module
- Check 100nF decoupling cap on VCC/GND
- Check SPI wiring — CLK, MOSI, MISO
- Check 74HC138 address lines
- Verify correct CS address for module
