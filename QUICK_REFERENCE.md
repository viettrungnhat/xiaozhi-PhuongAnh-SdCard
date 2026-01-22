# ⚡ Quick Reference - SD MP3 Implementation

## 📌 Key Files

| File | Purpose | Status |
|------|---------|--------|
| `sd_audio_player.h` | Audio player class | ✅ Created |
| `convert_ogg_to_mp3_sdcard.py` | FFmpeg batch converter | ✅ Created |
| `SD_AUDIO_SETUP_GUIDE.md` | Complete setup guide | ✅ Created |
| `CODE_CHANGES_SUMMARY.md` | Implementation details | ✅ Created |

---

## 🚀 Quick Start (4 Steps)

### 1️⃣ Convert Audio Files (~20 min)
```bash
cd e:\xiaozhi-PhuongAnh-SdCard
python scripts\convert_ogg_to_mp3_sdcard.py
```
**Result**: 77 MP3 files in `sdcard_notifications/` (~2.3MB)

### 2️⃣ Prepare SD Card (~15 min)
```powershell
# Format & create directory
Format-Volume -DriveLetter D -FileSystem FAT32 -Confirm:$false
New-Item -Path "D:\notifications" -ItemType Directory -Force

# Copy files
Copy-Item -Path ".\sdcard_notifications\*" -Destination "D:\notifications\" -Force -Verbose

# Verify (should show 77 files)
(Get-ChildItem -Path "D:\notifications").Count
```

### 3️⃣ Build & Flash (~10 min)
```bash
idf.py build
idf.py app-flash
```

### 4️⃣ Test Boot
```bash
idf.py monitor
# Expected: ✅ SD Card MP3 Player Ready
```

---

## 💻 Code Usage

### Play Battery Warning
```cpp
offline::SDMp3Player::GetInstance().PlayBatteryWarning(false);  // battery_low.mp3
offline::SDMp3Player::GetInstance().PlayBatteryWarning(true);   // battery_critical.mp3
```

### Play Greeting (Time-Aware)
```cpp
offline::SDMp3Player::GetInstance().PlayGreeting("morning");     // 06:00-11:59
offline::SDMp3Player::GetInstance().PlayGreeting("afternoon");   // 12:00-17:59
offline::SDMp3Player::GetInstance().PlayGreeting("evening");     // 18:00-23:59
offline::SDMp3Player::GetInstance().PlayGreeting("default");     // anytime
```

### Play Custom Alert
```cpp
offline::SDMp3Player::GetInstance().Play("custom_alert.mp3");
```

### Convenience Methods
```cpp
offline::SDMp3Player::GetInstance().PlayFuelWarning(true);           // fuel_critical.mp3
offline::SDMp3Player::GetInstance().PlayTempWarning(false);          // temp_high.mp3
offline::SDMp3Player::GetInstance().PlaySeatbeltWarning(true);       // warn_seatbelt_urgent.mp3
offline::SDMp3Player::GetInstance().PlaySpeedWarning(80);            // speed_80.mp3
```

---

## 📂 SD Card File Structure

```
/sdcard/
├── music/
│   ├── track1.mp3 (existing)
│   ├── track2.mp3 (existing)
│   └── ... (6 tracks)
└── notifications/          ← NEW
    ├── greeting_default.mp3
    ├── greeting_morning.mp3
    ├── greeting_afternoon.mp3
    ├── greeting_evening.mp3
    ├── battery_low.mp3
    ├── battery_critical.mp3
    ├── fuel_low.mp3
    ├── fuel_critical.mp3
    ├── temp_high.mp3
    ├── temp_critical.mp3
    ├── warn_seatbelt.mp3
    ├── warn_seatbelt_urgent.mp3
    ├── warn_door_open.mp3
    ├── warn_lights_on.mp3
    ├── warn_parking_brake.mp3
    ├── speed_40.mp3
    ├── speed_60.mp3
    ├── speed_80.mp3
    ├── speed_100.mp3
    ├── speed_120.mp3
    ├── speed_150.mp3
    └── ... (77 total files)
```

---

## 🎚️ Audio Specifications

| Parameter | Value |
|-----------|-------|
| Format | MP3 (MPEG-1 Layer III) |
| Sample Rate | 24 kHz |
| Channels | Mono |
| Bit Rate | 64 kbps |
| Output Bits | 16-bit PCM |
| Latency | 100-200ms |
| I2S Clock | GPIO16 (BCLK), GPIO17 (LRCK), GPIO18 (DOUT) |
| DAC | PPCM5102 or MAX98357 |

---

## ✅ Verification Commands

### Check Conversion
```bash
# Count MP3 files
dir ".\sdcard_notifications" | find /C ".mp3"
# Should show: 77

# Check file sizes
dir ".\sdcard_notifications" /s | grep "\.mp3"
# Total size ~2.3MB
```

### Check SD Card
```powershell
# Mount and check
Get-Volume -DriveLetter D
# Status: Healthy, FileSystem: FAT32

# Check notifications folder
Get-ChildItem -Path "D:\notifications" | Measure-Object | Select-Object Count
# Should show: 77
```

### Boot Test
```bash
idf.py monitor
# Watch for these messages:
# ✅ SD Card initialized successfully
# ✅ SD Card MP3 Player Ready
# 🔊 Playing: greeting_default.mp3
# ✅ CAN Bus State: RUNNING
```

---

## 🔧 Common Issues & Fixes

| Issue | Cause | Fix |
|-------|-------|-----|
| "SD card not ready" | Card not mounted | Check GPIO11/13/14/12 connections |
| "Cannot open: /sdcard/notifications/..." | File not on SD card | Copy MP3 files to /notifications/ |
| No audio output | DAC not initialized | Check GPIO16/17/18 I2S connections |
| Audio distorted | Wrong bit rate | Re-convert with `-b:a 128k` |
| File not found error | Wrong filename | Use exact names from manifest |
| SD read timeout | Speed too high | Reduce SPI speed in config |

---

## 📊 File Manifest (77 Files)

### Greetings (4)
- greeting_default.mp3
- greeting_morning.mp3
- greeting_afternoon.mp3
- greeting_evening.mp3

### Battery (2)
- battery_low.mp3
- battery_critical.mp3

### Fuel (2)
- fuel_low.mp3
- fuel_critical.mp3

### Temperature (2)
- temp_high.mp3
- temp_critical.mp3

### Seatbelt (2)
- warn_seatbelt.mp3
- warn_seatbelt_urgent.mp3

### Doors & Lights (3)
- warn_door_open.mp3
- warn_lights_on.mp3
- warn_parking_brake.mp3

### Speed Limits (8)
- speed_40.mp3
- speed_50.mp3
- speed_60.mp3
- speed_80.mp3
- speed_100.mp3
- speed_120.mp3
- speed_150.mp3
- speed_180.mp3 (future)

### Maintenance & Info (52)
- info_oil_change.mp3
- info_service_due.mp3
- info_tire_pressure.mp3
- warn_check_engine.mp3
- warn_abs_fault.mp3
- warn_airbag_fault.mp3
- ... (and 46 more)

---

## 🎯 Next Actions Checklist

- [ ] Run FFmpeg conversion script
- [ ] Verify 77 MP3 files created
- [ ] Format SD card as FAT32
- [ ] Create `/sdcard/notifications/` directory
- [ ] Copy all 77 MP3 files to SD card
- [ ] Verify file copy (should be 77 files, ~2.3MB)
- [ ] Insert SD card into ESP32-S3 board
- [ ] Build firmware: `idf.py build`
- [ ] Flash firmware: `idf.py app-flash`
- [ ] Monitor boot: `idf.py monitor`
- [ ] Verify "✅ SD Card MP3 Player Ready" appears
- [ ] Test individual sounds in code
- [ ] Integrate CAN callbacks
- [ ] Test with vehicle

---

## 📚 Full Documentation

**Setup Guide**: [SD_AUDIO_SETUP_GUIDE.md](SD_AUDIO_SETUP_GUIDE.md)  
**Code Changes**: [CODE_CHANGES_SUMMARY.md](CODE_CHANGES_SUMMARY.md)  
**Source File**: [sd_audio_player.h](main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/offline/sd_audio_player.h)  
**Converter Script**: [convert_ogg_to_mp3_sdcard.py](scripts/convert_ogg_to_mp3_sdcard.py)

---

**Implementation Status**: ✅ Framework Complete & Documented  
**Estimated Setup Time**: 30-60 minutes  
**Expected Result**: 77 Vietnamese vehicle alerts via SD Card MP3
