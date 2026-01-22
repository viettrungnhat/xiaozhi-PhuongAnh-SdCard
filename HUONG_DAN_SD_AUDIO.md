# 🎵 Hướng Dẫn Setup Audio từ Thẻ SD - Hệ Thống 77 Cảnh Báo Xe Kia Morning 2017

**Dự Án**: Xe Kia Morning 2017 - Hệ Thống Thông Báo OBD-II  
**Mục Đích**: Phát 77 cảnh báo tiếng Việt từ thẻ SD card qua loa xe  
**Trạng Thái**: ✅ Sẵn sàng thiết lập

---

## 📋 Mục Lục

1. [Bắt Đầu Nhanh (5 phút)](#bắt-đầu-nhanh)
2. [Yêu Cầu Phần Cứng](#yêu-cầu-phần-cứng)
3. [Chuẩn Bị File Audio (Chuyển Đổi Định Dạng)](#chuẩn-bị-file-audio)
4. [Thiết Lập Thẻ SD](#thiết-lập-thẻ-sd)
5. [Build & Nạp Firmware](#build--nạp-firmware)
6. [Kiểm Tra & Test](#kiểm-tra--test)
7. [Tích Hợp Với CAN Bus](#tích-hợp-với-can-bus)
8. [Xử Lý Sự Cố](#xử-lý-sự-cố)

---

## 🚀 Bắt Đầu Nhanh

### 4 Bước Cơ Bản (30-60 phút)

#### **Bước 1️⃣: Chuyển Đổi File Audio** (~20 phút)

```bash
cd e:\xiaozhi-PhuongAnh-SdCard
python scripts\convert_ogg_to_mp3_sdcard.py
```

**Kết Quả**:
- ✅ 77 file MP3 được tạo ra
- ✅ Kích thước: ~2.3 MB
- ✅ Lưu tại: `sdcard_notifications/`

**Nếu FFmpeg chưa cài đặt**:

```powershell
# Windows - Cài qua Chocolatey
choco install ffmpeg

# Hoặc download từ: https://ffmpeg.org/download.html
```

#### **Bước 2️⃣: Chuẩn Bị Thẻ SD** (~15 phút)

```powershell
# 1. Cắm thẻ SD vào máy (giả sử là ổ D:)

# 2. Format thẻ (FAT32)
Format-Volume -DriveLetter D -FileSystem FAT32 -Confirm:$false

# 3. Tạo thư mục notifications
New-Item -Path "D:\notifications" -ItemType Directory -Force

# 4. Copy toàn bộ file MP3
Copy-Item -Path ".\sdcard_notifications\*" -Destination "D:\notifications\" -Force -Verbose

# 5. Kiểm tra (phải hiển thị 77 file)
(Get-ChildItem -Path "D:\notifications").Count
```

#### **Bước 3️⃣: Build & Nạp Firmware** (~10 phút)

```bash
# Build (biên dịch)
idf.py build

# Nạp vào ESP32-S3
idf.py app-flash

# Đợi xong: "✅ Flashing complete"
```

#### **Bước 4️⃣: Test Boot** (~5 phút)

```bash
# Mở monitor để xem log
idf.py monitor

# Tìm các dòng:
# ✅ SD Card initialized successfully
# ✅ SD Card MP3 Player Ready
# ✅ CAN Bus State: RUNNING
```

🎉 **Xong! Hệ thống sẵn sàng!**

---

## 🛠️ Yêu Cầu Phần Cứng

### ESP32-S3 Board
- **Chip**: ESP32-S3 (N16R8 hoặc tương đương, 8MB PSRAM tối thiểu)
- **SPI SD Card**: GPIO11(MOSI), GPIO13(MISO), GPIO14(CLK), GPIO12(CS), GPIO42(DETECT)
- **I2S Audio Out**: GPIO16(BCLK), GPIO17(LRCK), GPIO18(DOUT), GPIO39(MCLK)

### Loa/DAC
| Model | Bit | Tần số | Ghi Chú |
|-------|-----|--------|--------|
| **PPCM5102** | 16-bit | 24 kHz | ✅ **Khuyên dùng** |
| **MAX98357** | 16-bit | 24 kHz | ✅ Tốt |

### Thẻ SD
- **Dung Lượng**: ≥1GB (3.8GB khuyên dùng)
- **Định Dạng**: FAT32
- **Tốc Độ**: Class 6 trở lên
- ⚠️ **Quan Trọng**: GPIO39 không được dùng cho SD (dùng cho MCLK của I2S)

---

## 📁 Chuẩn Bị File Audio

### Yêu Cầu Cài Đặt

**Windows PC**:
```powershell
# 1. Cài Python 3.8+
python --version  # Kiểm tra
# Nếu chưa: Download từ python.org

# 2. Cài FFmpeg
choco install ffmpeg
# Hoặc: Download từ ffmpeg.org

# 3. Kiểm tra lại
ffmpeg -version
```

**Linux (Ubuntu/Debian)**:
```bash
sudo apt-get update
sudo apt-get install python3 ffmpeg
```

### Chuyển Đổi File Audio

#### **Cách 1: Tự Động (Khuyên Dùng)** ✅

```bash
cd e:\xiaozhi-PhuongAnh-SdCard
python scripts\convert_ogg_to_mp3_sdcard.py
```

**Kết Quả**:
```
🎵 Found 77 OGG files to convert
============================================================
[01/77] greeting_default.mp3 ... ✅
[02/77] greeting_morning.mp3 ... ✅
...
[77/77] speed_150.mp3 ... ✅

📊 Results:
  ✅ Converted: 77/77 files
  💾 Total size: 2.3 MB
  📂 Output: e:\xiaozhi-PhuongAnh-SdCard\sdcard_notifications
```

#### **Cách 2: Thủ Công (Nếu Script Lỗi)**

```bash
# Một file:
ffmpeg -i input.ogg -acodec libmp3lame -ar 24000 -ac 1 -b:a 64k output.mp3

# Toàn bộ (Windows PowerShell):
Get-ChildItem "audio_ogg" -Recurse -Filter "*.ogg" | ForEach-Object {
    $outfile = $_.BaseName + ".mp3"
    ffmpeg -i $_.FullName -acodec libmp3lame -ar 24000 -ac 1 -b:a 64k $outfile
}
```

### Danh Sách 77 File Audio

**Lời Chào (4 file)**
```
greeting_default.mp3      - Lời chào mặc định
greeting_morning.mp3      - Chào buổi sáng (06:00-11:59)
greeting_afternoon.mp3    - Chào chiều (12:00-17:59)
greeting_evening.mp3      - Chào tối (18:00-23:59)
```

**Pin/Xăng/Nhiệt Độ (6 file)**
```
battery_low.mp3           - ⚠️ Pin yếu (<20V)
battery_critical.mp3      - 🚨 Pin nguy hiểm (<10V)
fuel_low.mp3              - ⚠️ Xăng cạn (<15%)
fuel_critical.mp3         - 🚨 Xăng sắp hết (<5%)
temp_high.mp3             - ⚠️ Nước mát nóng (>95°C)
temp_critical.mp3         - 🚨 Nước mát quá nóng (>105°C)
```

**Cảnh Báo Khác (5 file)**
```
warn_seatbelt.mp3         - Nhắc nhở dây an toàn
warn_seatbelt_urgent.mp3  - Nhắc gấp (tốc độ >80 km/h)
warn_door_open.mp3        - ⚠️ Cửa mở khi chạy
warn_lights_on.mp3        - ⚠️ Đèn vẫn sáng (tắt máy)
warn_parking_brake.mp3    - ⚠️ Phanh tay còn kéo (khi chạy)
```

**Tốc Độ Giới Hạn (8 file)**
```
speed_40.mp3, speed_60.mp3, speed_80.mp3, speed_100.mp3
speed_120.mp3, speed_150.mp3, ...
```

**File Khác (54 file)**
```
Bảo dưỡng, lỗi kỹ thuật, thông tin hệ thống, ...
```

---

## 💾 Thiết Lập Thẻ SD

### Bước 1: Format Thẻ

```powershell
# Cắm thẻ SD → Xuất hiện ổ (ví dụ: D:)

# Format FAT32
Format-Volume -DriveLetter D -FileSystem FAT32 -Confirm:$false

# Kiểm tra
Get-Volume -DriveLetter D
# FileSystem: NTFS  ← Nên là FAT32
```

### Bước 2: Tạo Thư Mục

```powershell
# Tạo thư mục notifications
New-Item -Path "D:\notifications" -ItemType Directory -Force

# Kiểm tra
Get-ChildItem -Path "D:\"
# Sẽ thấy: notifications (folder)
```

### Bước 3: Copy File MP3

```powershell
# Copy toàn bộ 77 file
Copy-Item -Path ".\sdcard_notifications\*" `
          -Destination "D:\notifications\" `
          -Force -Verbose

# Output:
# Copying "greeting_default.mp3"
# Copying "greeting_morning.mp3"
# ... (77 file)
```

### Bước 4: Kiểm Tra

```powershell
# Đếm file (phải hiển thị 77)
(Get-ChildItem -Path "D:\notifications").Count
# Kết quả: 77

# Xem chi tiết size
Get-ChildItem -Path "D:\notifications" | Measure-Object -Property Length -Sum
# TotalSize: 2.3 MB
```

### Bước 5: Eject Thẻ Safely

```powershell
# Eject an toàn
$volume = Get-Volume -DriveLetter D
$volume | Remove-Volume -Confirm:$false

# Hoặc: Click chuột phải → Eject

# Cắm vào ESP32-S3 board
```

### 📂 Cấu Trúc Thẻ SD

```
D: (thẻ SD)
├── music/
│   ├── track1.mp3 (hiện tại)
│   ├── track2.mp3 (hiện tại)
│   └── ... (6 bài)
└── notifications/          ← ✨ TỪ MỚI
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
    └── ... (77 tổng cộng)
```

---

## 🏗️ Build & Nạp Firmware

### Bước 1: Update Cấu Hình (Nếu Cần)

```bash
# Set target là ESP32-S3
idf.py set-target esp32s3

# Mở menu cấu hình
idf.py menuconfig
# → Component config
# → ESP-ML307 MP3 Decoder
# → ☑ Enable (đã bật sẵn)
```

### Bước 2: Build (Biên Dịch)

```bash
# Biên dịch toàn bộ
idf.py build

# Output sẽ tương tự:
# [0%] Creating directories and files.
# [1%] Generating esp_idf_version.h
# ...
# [100%] Built target app
# Took 12.34 seconds
# ✅ Build succeeded
```

**Nếu lỗi compilation**:
```bash
# Clean toàn bộ và build lại
idf.py fullclean
idf.py build
```

### Bước 3: Nạp Firmware

```bash
# Cắm cáp USB → ESP32-S3

# Nạp firmware
idf.py app-flash

# Output:
# Serial port COM7
# Chip is ESP32-S3 in download mode
# ...
# Wrote 1953792 bytes to file ...
# Wrote successfully
# ✅ Flashing complete
```

**Nếu không nhận cổng COM**:
```bash
# Kiểm tra cổng COM
idf.py monitor --port list

# Nạp vào cổng cụ thể
idf.py app-flash --port COM7
```

### Bước 4: Monitor Boot Log

```bash
# Mở serial monitor để xem log
idf.py monitor

# Tìm những dòng như:
# [0.100] ✅ SD Card initialized successfully
# [0.850] SD Card Status: Ready, Total: 3847.7 MB, Free: 3815.2 MB
# [1.200] ✅ SD Card MP3 Player Ready
# [1.250] 🎵 Playing: greeting_default.mp3
# [1.400] ✅ Playback complete: greeting_default.mp3
# [2.100] ✅ CAN Bus State: RUNNING
# [2.150] Application: STATE: idle

# Bấm Ctrl+] để thoát
```

---

## 🧪 Kiểm Tra & Test

### Kiểm Tra Boot

```
Khi khởi động, bạn sẽ thấy:

1. ✅ SD Card initialized successfully
   → Thẻ SD đã được nhận diện

2. ✅ SD Card MP3 Player Ready
   → Hệ thống âm thanh sẵn sàng

3. 🔊 Playing: greeting_default.mp3
   → Bắt đầu phát lời chào

4. ✅ Playback complete: greeting_default.mp3
   → Phát xong

5. ✅ CAN Bus State: RUNNING
   → Xe đã kết nối

6. Application: STATE: idle
   → Chương trình chạy bình thường
```

### Test Thủ Công

Thêm code này để test các file audio:

```cpp
// Trong application.cc hoặc shell
void TestAudioFiles() {
    auto& player = offline::SDMp3Player::GetInstance();
    
    // Test lời chào
    player.PlayGreeting("morning");
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // Test cảnh báo
    player.PlayBatteryWarning(false);      // battery_low.mp3
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    player.PlayBatteryWarning(true);       // battery_critical.mp3
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    player.PlayFuelWarning(false);         // fuel_low.mp3
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    player.PlaySpeedWarning(80);           // speed_80.mp3
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    player.PlaySeatbeltWarning(true);      // warn_seatbelt_urgent.mp3
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    ESP_LOGI(TAG, "✅ Test hoàn tất!");
}
```

---

## 🔗 Tích Hợp Với CAN Bus

### Khi Nào Phát Âm Thanh?

| Sự Kiện | File Audio | Điều Kiện |
|--------|-----------|----------|
| 🔋 Pin yếu | battery_low.mp3 | Điện áp < 20V |
| 🔋 Pin nguy hiểm | battery_critical.mp3 | Điện áp < 10V |
| ⛽ Xăng cạn | fuel_low.mp3 | Xăng < 15% |
| ⛽ Xăng sắp hết | fuel_critical.mp3 | Xăng < 5% |
| 🌡️ Nước mát nóng | temp_high.mp3 | Nhiệt độ > 95°C |
| 🌡️ Nước mát quá nóng | temp_critical.mp3 | Nhiệt độ > 105°C |
| 🪑 Dây an toàn | warn_seatbelt.mp3 | Không thắt, tốc độ < 80 km/h |
| 🪑 Dây an toàn gấp | warn_seatbelt_urgent.mp3 | Không thắt, tốc độ > 80 km/h |
| 🚪 Cửa mở | warn_door_open.mp3 | Cửa mở khi chạy |
| 💡 Đèn sáng | warn_lights_on.mp3 | Đèn bật, tắt máy |
| 🅿️ Phanh tay | warn_parking_brake.mp3 | Phanh tay kéo khi chạy |
| 🛑 Giới hạn tốc độ | speed_XX.mp3 | Phát hiện biển báo |

### Ví Dụ Code Tích Hợp

Xem file: `SD_AUDIO_INTEGRATION_EXAMPLES.cc`

```cpp
#include "offline/sd_audio_player.h"

// Khi pin thay đổi
void OnBatteryVoltageChange(uint16_t voltage_x10) {
    if (voltage_x10 < 100) {  // <10V
        offline::SDMp3Player::GetInstance().PlayBatteryWarning(true);
    } else if (voltage_x10 < 200) {  // <20V
        offline::SDMp3Player::GetInstance().PlayBatteryWarning(false);
    }
}

// Khi xăng thay đổi
void OnFuelLevelChange(uint8_t fuel_percent) {
    if (fuel_percent < 5) {
        offline::SDMp3Player::GetInstance().PlayFuelWarning(true);
    } else if (fuel_percent < 15) {
        offline::SDMp3Player::GetInstance().PlayFuelWarning(false);
    }
}

// Khi mở dây an toàn
void OnSeatbeltStatusChange(bool fastened, uint16_t speed) {
    if (!fastened) {
        bool urgent = (speed > 80);
        offline::SDMp3Player::GetInstance().PlaySeatbeltWarning(urgent);
    }
}

// Khi phát hiện tốc độ giới hạn
void OnSpeedLimitDetected(int speed_kmh) {
    offline::SDMp3Player::GetInstance().PlaySpeedWarning(speed_kmh);
}
```

---

## 🔧 Xử Lý Sự Cố

### Sự Cố 1: "SD Card Không Nhận Diện"

**Triệu Chứng**:
- Log: `❌ SD card not ready`
- Không phát âm thanh

**Nguyên Nhân & Cách Khắc Phục**:
```
1. ❌ Thẻ SD không cắm vào
   ✅ Kiểm tra khe cắm, cắm lại

2. ❌ GPIO sai
   ✅ Kiểm tra: GPIO11(MOSI), GPIO13(MISO), GPIO14(CLK), GPIO12(CS)

3. ❌ Thẻ SD bị hỏng
   ✅ Thử thẻ khác

4. ❌ Định dạng sai (NTFS thay vì FAT32)
   ✅ Format lại: Format-Volume -DriveLetter D -FileSystem FAT32
```

### Sự Cố 2: "File Không Tìm Thấy"

**Triệu Chứng**:
- Log: `❌ Cannot open: /sdcard/notifications/audio.mp3`

**Nguyên Nhân & Cách Khắc Phục**:
```
1. ❌ File chưa copy vào thẻ
   ✅ Copy lại:
      Copy-Item -Path ".\sdcard_notifications\*" `
                -Destination "D:\notifications\" -Force

2. ❌ Tên file sai (phải chính xác)
   ✅ Kiểm tra tên file: greeting_default.mp3 (không phải greeting_default.OGG)

3. ❌ Thư mục sai (không phải /sdcard/notifications)
   ✅ Đảm bảo thư mục là: /sdcard/notifications/ (chữ thường)

4. ❌ Thẻ SD bị eject trước khi copy xong
   ✅ Copy lại cẩn thận, kiểm tra 77 file
```

### Sự Cố 3: "Âm Thanh Rất Yếu Hoặc Méo"

**Triệu Chứng**:
- Phát được nhưng âm rất nhỏ
- Hoặc âm bị đưa

**Nguyên Nhân & Cách Khắc Phục**:
```
1. ❌ GPIO I2S sai
   ✅ Kiểm tra: GPIO16(BCLK), GPIO17(LRCK), GPIO18(DOUT), GPIO39(MCLK)

2. ❌ DAC không được cấp điện
   ✅ Kiểm tra 3.3V trên chân AVDD của DAC

3. ❌ File MP3 chất lượng thấp
   ✅ Re-convert với bitrate cao hơn:
      ffmpeg -i input.ogg -acodec libmp3lame -b:a 128k output.mp3

4. ❌ Volume module bị mute
   ✅ Tăng volume từ code hoặc nút ấn
```

### Sự Cố 4: "FFmpeg Không Tìm Thấy"

**Triệu Chứng**:
- Lỗi: `ffmpeg: command not found`

**Cách Khắc Phục**:
```powershell
# Cài FFmpeg
choco install ffmpeg

# Hoặc download: https://ffmpeg.org/download.html

# Kiểm tra lại
ffmpeg -version
```

### Sự Cố 5: "Thẻ SD Chậm"

**Triệu Chứng**:
- Chơi nhạc bị lag
- File mở chậm

**Nguyên Nhân & Cách Khắc Phục**:
```
1. ❌ SPI speed quá cao
   ✅ Giảm từ 15MHz xuống 5MHz trong cấu hình

2. ❌ Thẻ SD class thấp
   ✅ Dùng thẻ Class 10 trở lên

3. ❌ Xung đột với LCD
   ✅ Đảm bảo LCD & SD card dùng SPI host khác nhau
      LCD: SPI3_HOST (30MHz)
      SD: SPI2_HOST (5-15MHz)
```

### Sự Cố 6: "Xung Đột GPIO GPIO39"

**Triệu Chứng**:
- Boot lỗi
- I2S không hoạt động

**Cách Khắc Phục**:
```
GPIO39 không được dùng cho SD card (dùng cho I2S MCLK)

Kiểm tra cấu hình:
- SD: GPIO11, GPIO13, GPIO14, GPIO12 ✅
- I2S: GPIO16, GPIO17, GPIO18, GPIO39 ✅
```

---

## 📊 Thông Số Kỹ Thuật

### Audio MP3
| Thông Số | Giá Trị |
|---------|--------|
| **Định Dạng** | MP3 (MPEG-1 Layer III) |
| **Tần số** | 24 kHz |
| **Kênh** | Mono |
| **Bitrate** | 64 kbps |
| **Độ Sâu** | 16-bit |
| **Độ Trễ** | 100-200ms (chấp nhận được cho cảnh báo) |

### I2S DAC
| GPIO | Chức Năng | Ghi Chú |
|-----|---------|--------|
| GPIO16 | BCLK | Bit clock |
| GPIO17 | LRCK | Word select |
| GPIO18 | DOUT | Data out |
| GPIO39 | MCLK | Master clock |

### SD Card SPI
| GPIO | Chức Năng | Ghi Chú |
|-----|---------|--------|
| GPIO11 | MOSI | Data out từ ESP |
| GPIO13 | MISO | Data in vào ESP |
| GPIO14 | CLK | Clock |
| GPIO12 | CS | Chip select |
| GPIO42 | DETECT | Phát hiện card |

### Hiệu Suất
| Chỉ Số | Giá Trị |
|-------|--------|
| **Thời Gian Setup** | 30-60 phút |
| **Số File Audio** | 77 cảnh báo |
| **Dung Lượng** | 2.3 MB |
| **Độ Trễ** | 100-200ms |
| **Độ Tin Cậy** | Cao (MP3 đơn giản) |

---

## ✅ Danh Sách Kiểm Tra

Sau khi setup xong, kiểm tra:

- [ ] FFmpeg đã cài đặt
- [ ] Python 3.8+ sẵn sàng
- [ ] 77 file OGG nằm trong `audio_ogg/`
- [ ] Script `convert_ogg_to_mp3_sdcard.py` có tại `scripts/`
- [ ] Chạy script → 77 file MP3 được tạo ra
- [ ] Thẻ SD format FAT32
- [ ] Thư mục `/notifications/` được tạo trên thẻ
- [ ] 77 file MP3 copy vào `/notifications/`
- [ ] Cắm thẻ SD vào ESP32-S3
- [ ] Build firmware: `idf.py build`
- [ ] Nạp firmware: `idf.py app-flash`
- [ ] Monitor boot log: `idf.py monitor`
- [ ] Thấy "✅ SD Card MP3 Player Ready"
- [ ] Thấy "✅ CAN Bus State: RUNNING"
- [ ] Phát lời chào "greeting_default.mp3" thành công

---

## 🎯 Tiếp Theo

### Ngắn Hạn (Setup)
- ✅ Chuẩn bị PC (FFmpeg, Python)
- ✅ Convert audio (~20 phút)
- ✅ Thiết lập thẻ SD (~15 phút)
- ✅ Build & nạp (~10 phút)

### Trung Hạn (Tích Hợp)
- [ ] Thêm callback CAN bus
- [ ] Test từng loại cảnh báo
- [ ] Điều chỉnh ngưỡng cảnh báo
- [ ] Kiểm tra độ trễ

### Dài Hạn (Sản Xuất)
- [ ] Test với xe thực (Kia Morning 2017)
- [ ] Cải thiện file audio
- [ ] Deploy vào xe
- [ ] Bảo dưỡng & cập nhật

---

## 📞 Tài Liệu Khác

| Tài Liệu | Nội Dung |
|---------|---------|
| [QUICK_REFERENCE.md](QUICK_REFERENCE.md) | Tham khảo nhanh |
| [SD_AUDIO_SETUP_GUIDE.md](SD_AUDIO_SETUP_GUIDE.md) | Hướng dẫn tiếng Anh chi tiết |
| [CODE_CHANGES_SUMMARY.md](CODE_CHANGES_SUMMARY.md) | Tóm tắt code |
| [SD_AUDIO_INTEGRATION_EXAMPLES.cc](SD_AUDIO_INTEGRATION_EXAMPLES.cc) | Ví dụ code C++ |
| [HUONG_DAN_SD_AUDIO.md](HUONG_DAN_SD_AUDIO.md) | **Hướng dẫn tiếng Việt (File này)** |

---

## 🎵 Kết Luận

**Hệ thống SD MP3 đơn giản, đáng tin cậy, dễ sửa đổi.**

So với cách khác (Ogg Opus từ Flash):
- ✅ Không bị stack overflow
- ✅ Dễ cập nhật file (không cần recompile)
- ✅ Độ trễ chấp nhận được (100-200ms)
- ✅ Âm thanh rõ ràng, tự nhiên

🚗 **Sẵn sàng lắp vào Kia Morning 2017 của bạn!**

---

**Phiên Bản**: 1.0  
**Cập Nhật Lần Cuối**: 2024  
**Trạng Thái**: ✅ Sẵn Sàng
