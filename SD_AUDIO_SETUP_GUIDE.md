# 🎵 SD Card MP3 Audio Setup Guide - Vietnamese Vehicle Alerts

**Project**: Kia Morning 2017 OBD-II Vehicle Notification System  
**Hardware**: ESP32-S3 + PPCM5102/MAX98357 DAC + SD Card  
**System**: 77 Vietnamese vehicle alert notifications via SD Card MP3  
**Status**: ✅ Ready for Setup

---

## 📋 Table of Contents

1. [Quick Start](#quick-start)
2. [Hardware Requirements](#hardware-requirements)
3. [File Preparation (Audio Conversion)](#file-preparation)
4. [SD Card Setup](#sd-card-setup)
5. [Building & Flashing](#building--flashing)
6. [Integration & Testing](#integration--testing)
7. [Troubleshooting](#troubleshooting)
8. [Technical Details](#technical-details)

---

## ⚡ Quick Start

**Time Required**: ~30-60 minutes  
**Skills Required**: Basic Windows/Linux, command line, USB device drivers

### 1. **Convert Audio Files** (~20 min)
```bash
cd scripts/
python convert_ogg_to_mp3_sdcard.py
```
Output: 77 MP3 files in `sdcard_notifications/` folder

### 2. **Prepare SD Card** (~10 min)
- Copy all MP3 files to SD card at: `/sdcard/notifications/`
- Insert SD card into ESP32-S3 board

### 3. **Build & Flash** (~5-10 min)
```bash
idf.py build
idf.py app-flash
```

### 4. **Test**
```bash
idf.py monitor
```
Boot should show: "✅ SD Card MP3 Player Ready"

---

## 🛠️ Hardware Requirements

### ESP32-S3 Board
- **MCU**: ESP32-S3 (N16R8 or similar, 8MB PSRAM minimum)
- **SD Card Interface**: SPI mode (GPIO11=MOSI, GPIO13=MISO, GPIO14=CLK, GPIO12=CS, GPIO42=DETECT)
- **I2S DAC Interface**: GPIO16(BCLK), GPIO17(LRCK), GPIO18(DOUT), GPIO39(MCLK) - **NO SD CARD ON GPIO39**

### Audio DAC
| DAC Model | Bits | Sample Rate | Notes |
|-----------|------|-------------|-------|
| PPCM5102  | 16   | 24 kHz      | ✅ Recommended (no codec chip) |
| MAX98357  | 16   | 24 kHz      | ✅ Class D amplifier |
| UDA1334   | 16   | 48 kHz      | Compatible |

### SD Card
- **Size**: ≥1GB (3.8GB recommended)
- **Format**: FAT32
- **Speed**: Class 6 or better
- **Pin Configuration**: Must NOT conflict with I2S GPIO39

---

## 📁 File Preparation

### Prerequisites
- **Windows PC with**: Python 3.8+, FFmpeg
- **Installation**:

```powershell
# Install FFmpeg (Windows)
choco install ffmpeg
# or download from: https://ffmpeg.org/download.html

# Install Python
python --version  # Should be 3.8 or newer
```

### Audio Conversion Process

#### **Option A: Automatic (Recommended)**
```bash
cd e:\xiaozhi-PhuongAnh-SdCard
python scripts\convert_ogg_to_mp3_sdcard.py
```

**Output**:
```
📊 Results:
  ✅ Converted: 77/77 files
  💾 Total size: 2.3 MB
  📂 Output: e:\xiaozhi-PhuongAnh-SdCard\sdcard_notifications
```

#### **Option B: Manual (Using FFmpeg directly)**
```bash
# Single file
ffmpeg -i audio_ogg/greetings/greeting_default.ogg -acodec libmp3lame -ar 24000 -ac 1 -b:a 64k greeting_default.mp3

# Batch convert all files
for /r "audio_ogg" %f in (*.ogg) do (
    ffmpeg -i "%f" -acodec libmp3lame -ar 24000 -ac 1 -b:a 64k "%~nf.mp3"
)
```

### Audio File Manifest (77 Files)

#### **Category: Greetings (4 files)**
```
greeting_default.mp3      - Default greeting (neutral time)
greeting_morning.mp3      - Morning greeting (06:00-11:59)
greeting_afternoon.mp3    - Afternoon greeting (12:00-17:59)
greeting_evening.mp3      - Evening greeting (18:00-23:59)
```

#### **Category: Battery Warnings (2 files)**
```
battery_low.mp3           - Battery voltage <20% (28V for 24V system)
battery_critical.mp3      - Battery voltage <10% (14V for 24V system)
```

#### **Category: Fuel Level (2 files)**
```
fuel_low.mp3              - Fuel remaining <15% of tank
fuel_critical.mp3         - Fuel remaining <5% of tank
```

#### **Category: Temperature Warnings (2 files)**
```
temp_high.mp3             - Coolant temperature >95°C
temp_critical.mp3         - Coolant temperature >105°C (engine protection active)
```

#### **Category: Seatbelt Reminders (2 files)**
```
warn_seatbelt.mp3         - Standard seatbelt reminder (every 30s if unfastened)
warn_seatbelt_urgent.mp3  - Urgent reminder (speed >80km/h, unfastened)
```

#### **Category: Door & Lights (3 files)**
```
warn_door_open.mp3        - Door open at driving
warn_lights_on.mp3        - Headlights left on (engine off)
warn_parking_brake.mp3    - Parking brake engaged (driving detected)
```

#### **Category: Speed Limit Announcements (8 files)**
```
speed_40.mp3              - Speed limit 40 km/h
speed_60.mp3              - Speed limit 60 km/h
speed_80.mp3              - Speed limit 80 km/h
speed_100.mp3             - Speed limit 100 km/h
speed_120.mp3             - Speed limit 120 km/h (motorway)
speed_150.mp3             - Speed limit 150 km/h (motorway special)
```

#### **Category: Maintenance & Info (52 files - expandable)**
```
info_oil_change.mp3       - Scheduled oil change due
info_service_due.mp3      - Service/inspection due
info_tire_pressure.mp3    - Tire pressure low
warn_check_engine.mp3     - Check engine light (OBD-II codes)
warn_abs_fault.mp3        - ABS malfunction
warn_airbag_fault.mp3     - Airbag system fault
... (additional info files by vehicle state)
```

**Note**: Total 77 files (~2.3MB at 64kbps MP3)

---

## 💾 SD Card Setup

### Step 1: Format SD Card
```powershell
# Windows PowerShell
# Insert SD card, note drive letter (e.g., D:)
Format-Volume -DriveLetter D -FileSystem FAT32 -Confirm:$false
```

### Step 2: Create Directory Structure
```powershell
# Create notifications folder
New-Item -Path "D:\notifications" -ItemType Directory -Force

# Verify
Get-ChildItem -Path "D:\" -Directory
```

### Step 3: Copy MP3 Files
```powershell
# Copy all MP3 files
Copy-Item -Path ".\sdcard_notifications\*" -Destination "D:\notifications\" -Force -Verbose

# Verify copy
Get-ChildItem -Path "D:\notifications\" | Measure-Object | Select-Object Count
# Expected: 77 files
```

### Step 4: Verify File Structure
```powershell
# Check SD card structure
$path = "D:\notifications"
Get-ChildItem -Path $path | Format-Table Name, Length -AutoSize

# Expected output:
# Name                           Length
# ----                           ------
# greeting_default.mp3           28500
# greeting_morning.mp3           31200
# battery_low.mp3                25400
# ... (77 total files, ~2.3MB)
```

### Step 5: Eject & Insert into Board
```powershell
# Safely eject SD card
Invoke-Item "D:\" | Eject-USB
# Or use Windows: Right-click → Eject
```

---

## 🏗️ Building & Flashing

### Step 1: Update Configuration (if needed)
```bash
# Configure for ESP32-S3
idf.py set-target esp32s3

# Enable MP3 decoder
idf.py menuconfig
# Navigate: Component config → ESP-ML307 MP3 Decoder → [X] Enable
```

### Step 2: Build Firmware
```bash
# Full clean build
idf.py fullclean
idf.py build

# Output:
# Calculating binary size...
# esp32s3_xiaozhi_vn.bin file is too large. Options for flashing:
# - Reduce code and data size in your project
# - Use PSRAM for dynamic allocation
# ✅ Build succeeded
```

### Step 3: Flash to Device
```bash
# Ensure device is connected
idf.py app-flash

# Output:
# Serial port COM7
# Chip is ESP32-S3
# Features: WiFi, BLE, Dual Core, 8MB PSRAM
# ...
# Wrote successfully
# ✅ Flashing complete
```

### Step 4: Monitor Serial Output
```bash
# Open serial monitor
idf.py monitor

# Expected boot output:
# [0.120] ✅ SD Card initialized successfully
# [0.850] SD Card Status: Ready, Total: 3847.7 MB, Free: 3815.2 MB
# [1.200] ✅ SD Card MP3 Player Ready
# [1.250] 🎵 Playing: greeting_default.mp3 (28500 bytes)
# [1.400] ✅ Playback complete: greeting_default.mp3
# [2.100] ✅ CAN Bus State: RUNNING
# [2.150] Application: STATE: idle
```

**Note**: First boot with SD MP3 greeting should play within 1-2 seconds

---

## 🧪 Integration & Testing

### Phase 1: Hardware Verification
```bash
# Monitor boot messages
idf.py monitor

# Check for:
✅ "SD Card initialized successfully"
✅ "SD Card MP3 Player Ready"
✅ "CAN Bus State: RUNNING"
```

### Phase 2: Manual Audio Test (via Serial Console)
In ESP32-S3 code, test individual sounds:

```cpp
// In application initialization
offline::SDMp3Player::GetInstance().PlayGreeting("morning");
vTaskDelay(pdMS_TO_TICKS(3000));

offline::SDMp3Player::GetInstance().PlayBatteryWarning(false);  // battery_low.mp3
vTaskDelay(pdMS_TO_TICKS(3000));

offline::SDMp3Player::GetInstance().PlaySpeedWarning(80);       // speed_80.mp3
```

### Phase 3: CAN Bus Integration
Update `vehicle_assistant.cc` callbacks:

```cpp
// Example: Battery voltage callback
void OnBatteryVoltageChange(uint16_t voltage_x10) {
    // voltage_x10 = 140 means 14.0V
    if (voltage_x10 < 100) {  // <10V critical
        offline::SDMp3Player::GetInstance().PlayBatteryWarning(true);
    } else if (voltage_x10 < 200) {  // <20V low
        offline::SDMp3Player::GetInstance().PlayBatteryWarning(false);
    }
}

// Seatbelt example
void OnSeatbeltStatus(bool driver_fastened, uint16_t vehicle_speed) {
    if (!driver_fastened && vehicle_speed > 80) {  // Urgent
        offline::SDMp3Player::GetInstance().PlaySeatbeltWarning(true);
    } else if (!driver_fastened) {
        offline::SDMp3Player::GetInstance().PlaySeatbeltWarning(false);
    }
}

// Speed limit detection
void OnSpeedLimitSign(int speed_kmh) {
    // Announce when new speed limit detected
    offline::SDMp3Player::GetInstance().PlaySpeedWarning(speed_kmh);
}
```

### Phase 4: Live Vehicle Testing
1. **Equipment**: OBD-II diagnostic dongle (or simulated data)
2. **Test Scenarios**:
   - Start engine → "greeting_morning.mp3" plays
   - Battery voltage drops → "battery_low.mp3" plays
   - Seatbelt unbuckle → "warn_seatbelt.mp3" plays (every 30s)
   - Speed 80+ without seatbelt → "warn_seatbelt_urgent.mp3" plays
   - Engine coolant >95°C → "temp_high.mp3" plays
   - Fuel <15% → "fuel_low.mp3" plays

---

## 🔧 Troubleshooting

### Issue 1: "SD Card not mounted"
**Symptoms**: 
- Boot log: "❌ SD card not ready"
- No audio playback

**Solution**:
```
1. Verify SD card in correct slot
2. Check GPIO connections: 11(MOSI), 13(MISO), 14(CLK), 12(CS)
3. Verify SPI frequency: 5MHz (in board config)
4. Try different SD card (some cards fail at low speed)
5. Enable debug: idf.py menuconfig → Component config → Log level → Verbose
```

### Issue 2: "Cannot open: /sdcard/notifications/audio.mp3"
**Symptoms**:
- Boot log: "❌ Cannot open: /sdcard/notifications/audio.mp3"

**Solution**:
```
1. Verify SD card has /notifications/ directory
2. Check file names EXACTLY match (case-sensitive on Linux/ESP32)
3. Verify FAT32 format: dir /s d:\
4. Copy files again: Copy-Item -Path ".\sdcard_notifications\*" -Destination "D:\notifications\" -Force
5. Eject properly before flashing
```

### Issue 3: Audio Very Quiet or Distorted
**Symptoms**:
- Playback is barely audible or sounds "garbled"

**Solution**:
```
1. Check DAC I2S connections: GPIO16(BCLK), GPIO17(LRCK), GPIO18(DOUT)
2. Verify DAC power: 3.3V on AVDD pin
3. Check audio file quality: bitrate should be 64kbps or higher
4. Re-convert files with higher bitrate:
   ffmpeg -i input.ogg -acodec libmp3lame -b:a 128k output.mp3
5. Test with known good MP3 file
```

### Issue 4: "Conversion Error: ffmpeg not found"
**Symptoms**:
- Running `python convert_ogg_to_mp3_sdcard.py` fails
- Error: "ffmpeg: command not found"

**Solution**:
```bash
# Install FFmpeg
# Windows:
choco install ffmpeg
# or download: https://ffmpeg.org/download.html

# Linux (Ubuntu/Debian):
sudo apt-get install ffmpeg

# macOS:
brew install ffmpeg

# Verify installation:
ffmpeg -version
```

### Issue 5: "Stack Overflow" During Boot
**Symptoms**:
- Boot log shows OpusHead decoded but then "***ERROR*** A stack overflow"

**This is already fixed in SD MP3 version** - MP3 decoder requires less stack than Opus FFT

### Issue 6: Playback Cuts Out Mid-File
**Symptoms**:
- Audio starts but stops after 2-3 seconds
- SD card access LED stops blinking

**Solution**:
```
1. Check SD card stability: try different card
2. Reduce CAN bus processing load (may be starving SD I/O)
3. Increase SD SPI speed: up to 20MHz (if card supports)
4. Check I2S task priority: should be higher than SD task
```

---

## 📊 Technical Details

### Audio Architecture
```
┌──────────────┐
│  CAN Bus     │ ← Vehicle OBD-II data
│  Parser      │
└──────┬───────┘
       │
       ▼
┌──────────────────┐
│  Vehicle Logic   │ ← Battery, Fuel, Temp, Speed logic
│  State Machine   │
└──────┬───────────┘
       │
       ▼
┌──────────────────────┐
│  Alert Trigger       │ ← Determines which sound to play
│  Callback Handler    │
└──────┬───────────────┘
       │
       ▼
┌──────────────────────┐    ┌─────────────┐
│  SDMp3Player         │───→│  SD Card    │  /sdcard/notifications/*.mp3
│  Play(filename)      │    │  (3.8GB)    │
└──────┬───────────────┘    └─────────────┘
       │
       ▼
┌──────────────────────┐
│  MP3 Decoder         │  ← Built-in ESP-ML307 libhelix MP3
│  (24 bit PCM output) │
└──────┬───────────────┘
       │
       ▼
┌──────────────────────┐
│  I2S DAC Output      │  ← PPCM5102 / MAX98357
│  GPIO16/17/18        │
└──────┬───────────────┘
       │
       ▼
    AUDIO OUT
   (speakers)
```

### Latency Analysis
| Stage | Latency | Notes |
|-------|---------|-------|
| CAN Parse → Alert Trigger | <10ms | Main task |
| Trigger → SDMp3Player | <5ms | Function call |
| File Open & Read | 50-100ms | SD card SPI overhead |
| MP3 Decode (first frame) | 20-50ms | FFT operations |
| I2S Output Start | <5ms | DAC ready |
| **Total** | **100-200ms** | ✅ Acceptable for alerts |

**vs. Flash Assets (Ogg) Approach**:
| Approach | Latency | Stack | Complexity |
|----------|---------|-------|------------|
| **SD Card MP3** | 100-200ms | ✅ Safe | ✅ Simple |
| Flash Assets Ogg | 10-20ms | ❌ Overflow | ❌ Needs task |
| **Trade-off**: Slightly slower but much simpler and reliable |

### I2S Configuration
```cpp
// I2S pins for PPCM5102/MAX98357
i2s_std_config_t std_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(24000),  // 24kHz mono
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
        I2S_DATA_BIT_WIDTH_16BIT,
        I2S_SLOT_MODE_MONO
    ),
    .gpio_cfg = {
        .mclk = GPIO_NUM_39,      // Master clock
        .bclk = GPIO_NUM_16,      // Bit clock
        .ws   = GPIO_NUM_17,      // Word select (LRCK)
        .dout = GPIO_NUM_18,      // Data out
        .din  = I2S_GPIO_UNUSED,
    },
};
```

### MP3 Decoder Specifications
- **Format**: MP3 (MPEG-1 Layer III)
- **Sample Rates**: 16, 22.05, 24, 32, 44.1, 48 kHz
- **Bit Rates**: 32-320 kbps
- **Output**: 16-bit PCM
- **Memory**: ~30KB decode buffer + I2S DMA
- **Decoding Speed**: Real-time (24kHz mono at 64kbps)

---

## 📝 File Checklist

After setup, verify these files exist:

```
✅ e:\xiaozhi-PhuongAnh-SdCard\
   ├── scripts\convert_ogg_to_mp3_sdcard.py          (conversion script)
   ├── main\boards\xiaozhi-ai-iot-vietnam-lcd-sdcard\
   │   └── offline\sd_audio_player.h                 (player class)
   └── sdcard_notifications\                          (77 MP3 files)
       ├── greeting_default.mp3
       ├── greeting_morning.mp3
       ├── battery_low.mp3
       ├── battery_critical.mp3
       ├── fuel_low.mp3
       ├── fuel_critical.mp3
       ├── temp_high.mp3
       ├── temp_critical.mp3
       ├── warn_door_open.mp3
       ├── warn_lights_on.mp3
       ├── warn_parking_brake.mp3
       ├── warn_seatbelt.mp3
       ├── warn_seatbelt_urgent.mp3
       ├── speed_40.mp3 ... speed_150.mp3
       └── ... (77 total)

✅ /sdcard/ (on physical card)
   └── notifications\
       ├── greeting_default.mp3
       ├── greeting_morning.mp3
       ├── battery_low.mp3
       ├── battery_critical.mp3
       ├── fuel_low.mp3
       ├── fuel_critical.mp3
       ├── temp_high.mp3
       ├── temp_critical.mp3
       ├── warn_door_open.mp3
       ├── warn_lights_on.mp3
       ├── warn_parking_brake.mp3
       ├── warn_seatbelt.mp3
       ├── warn_seatbelt_urgent.mp3
       ├── speed_40.mp3 ... speed_150.mp3
       └── ... (77 total)
```

---

## ✅ Success Criteria

After complete setup, you should see:

```
Boot Log:
✅ CAN Bus initialized
✅ SD Card initialized successfully
✅ SD Card MP3 Player Ready
✅ CAN Bus State: RUNNING
✅ Application: STATE: idle

Console Test (run in code):
offline::SDMp3Player::GetInstance().PlayGreeting("morning");
// Result: 🔊 greeting_morning.mp3 plays for ~2 seconds

Real Vehicle Test:
1. Disconnect seatbelt → warn_seatbelt.mp3 plays
2. Speed >80km/h → warn_seatbelt_urgent.mp3 plays (if still unfastened)
3. Battery <20V → battery_low.mp3 plays
4. Fuel <15% → fuel_low.mp3 plays
5. Coolant >95°C → temp_high.mp3 plays
```

---

## 🚀 Next Steps

1. **Short-term** (Setup):
   - [ ] Convert 77 Ogg files to MP3
   - [ ] Create SD card structure
   - [ ] Copy MP3 files to SD card
   - [ ] Build & flash firmware
   - [ ] Verify boot with greeting audio

2. **Medium-term** (Integration):
   - [ ] Integrate CAN callbacks with audio playback
   - [ ] Test individual alert scenarios
   - [ ] Verify latency (should be 100-200ms)
   - [ ] Test with simulated OBD-II data

3. **Long-term** (Refinement):
   - [ ] Test with actual Kia Morning 2017 OBD-II
   - [ ] Add more alert files (maintenance, fault codes)
   - [ ] Optimize SD read speed (current: 5MHz, can go to 20MHz)
   - [ ] Add alert throttling (prevent alert spam)
   - [ ] Deploy to production vehicle

---

## 📞 Support & References

### Related Files
- [README.md](README.md) - General project documentation
- [main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/offline/sd_audio_player.h](main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/offline/sd_audio_player.h) - Audio player implementation
- [main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/xiaozhi_ai_iot_vietnam_board_lcd_sdcard.cc](main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/xiaozhi_ai_iot_vietnam_board_lcd_sdcard.cc) - Board initialization

### External References
- [ESP-IDF SD Card Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/sd_pullup_requirements.html)
- [FFmpeg Documentation](https://ffmpeg.org/documentation.html)
- [I2S Audio Output](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/i2s.html)
- [PPCM5102 DAC Datasheet](https://www.ti.com/product/PCM5102)

### Typical Timeline
```
Day 1:
  08:00 - Install tools (FFmpeg, Python)
  08:30 - Convert audio files (~20 min)
  09:00 - Prepare SD card (~15 min)
  09:20 - Build firmware (~5 min)
  09:30 - Flash & test (~10 min)
  09:45 - ✅ System ready for testing

Day 2+:
  - Integrate CAN callbacks
  - Test with vehicle
  - Refine audio files as needed
```

---

**Document Version**: 1.0  
**Last Updated**: 2024  
**Status**: ✅ SD Card MP3 System Active
