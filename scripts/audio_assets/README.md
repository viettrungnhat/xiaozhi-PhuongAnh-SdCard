# 🔊 Offline Audio Assets - Hướng Dẫn Tạo và Flash

Hướng dẫn chi tiết cách tạo file audio Opus/OGG cho chế độ offline và flash vào ESP32.

## 📋 Mục Lục

1. [Yêu Cầu](#yêu-cầu)
2. [Cấu Trúc File Audio](#cấu-trúc-file-audio)
3. [Cách 1: Tạo Audio Từ Text (TTS)](#cách-1-tạo-audio-từ-text-tts)
4. [Cách 2: Convert Audio Có Sẵn](#cách-2-convert-audio-có-sẵn)
5. [Build Assets.bin](#build-assetsbin)
6. [Flash Riêng Partition Assets](#flash-riêng-partition-assets)
7. [Kiểm Tra Kết Quả](#kiểm-tra-kết-quả)

---

## Yêu Cầu

### Phần mềm cần cài đặt:

```bash
# Python 3.8+
python --version

# Cài đặt các thư viện cần thiết
pip install ffmpeg-python edge-tts pydub

# FFmpeg (cần cài riêng)
# Windows: Download từ https://ffmpeg.org/download.html và thêm vào PATH
# hoặc dùng chocolatey: choco install ffmpeg
```

### Thông số kỹ thuật audio:
- **Codec**: Opus
- **Container**: OGG
- **Sample Rate**: 16000 Hz (16kHz)
- **Channels**: Mono (1 channel)
- **Bitrate**: 16-32 kbps

---

## Cấu Trúc File Audio

Các file audio cần được đặt trong thư mục `audio_files/` với cấu trúc sau:

```
audio_files/
├── greetings/
│   ├── greeting_morning.opus      # Chào buổi sáng
│   ├── greeting_afternoon.opus    # Chào buổi chiều
│   ├── greeting_evening.opus      # Chào buổi tối
│   └── greeting_default.opus      # Chào mặc định
├── warnings/
│   ├── warn_seatbelt.opus         # Nhắc thắt dây an toàn
│   ├── warn_seatbelt_urgent.opus  # Cảnh báo dây an toàn khẩn cấp
│   ├── battery_low.opus           # Pin yếu
│   └── fuel_low.opus              # Nhiên liệu thấp
├── control/
│   ├── trunk_opened.opus          # Cốp đã mở
│   ├── trunk_closed.opus          # Cốp đã đóng
│   ├── door_locked.opus           # Khóa cửa
│   └── door_unlocked.opus         # Mở khóa cửa
└── system/
    ├── wifi_connected.opus        # WiFi đã kết nối
    ├── wifi_disconnected.opus     # Mất kết nối WiFi
    ├── offline_mode.opus          # Đã chuyển sang chế độ offline
    └── online_mode.opus           # Đã kết nối online
```

---

## Cách 1: Tạo Audio Từ Text (TTS)

### Bước 1: Chạy script tạo audio từ văn bản

```bash
cd scripts/audio_assets
python create_audio_from_text.py
```

Script sẽ tự động tạo các file audio từ danh sách văn bản được định nghĩa sẵn.

### Bước 2: Tùy chỉnh văn bản (tùy chọn)

Mở file `audio_text_config.json` để chỉnh sửa nội dung:

```json
{
    "greetings/greeting_morning.opus": "Xin chào buổi sáng! Chúc bạn một ngày tốt lành!",
    "greetings/greeting_afternoon.opus": "Xin chào buổi chiều! Lái xe an toàn nhé!",
    "warnings/warn_seatbelt.opus": "Xin vui lòng thắt dây an toàn trước khi khởi hành."
}
```

---

## Cách 2: Convert Audio Có Sẵn

### Từ file WAV/MP3:

```bash
# Chuyển đổi 1 file
python convert_to_opus.py input.wav output.opus

# Chuyển đổi toàn bộ thư mục
python convert_to_opus.py --input-dir ./wav_files --output-dir ./audio_files
```

### Sử dụng FFmpeg trực tiếp:

```bash
# Convert WAV sang Opus/OGG
ffmpeg -i input.wav -ar 16000 -ac 1 -c:a libopus -b:a 24k output.opus

# Convert MP3 sang Opus/OGG
ffmpeg -i input.mp3 -ar 16000 -ac 1 -c:a libopus -b:a 24k output.opus
```

---

## Build Assets.bin

### Bước 1: Copy audio files vào assets

```bash
cd scripts/audio_assets
python build_audio_assets.py
```

Script này sẽ:
1. Đọc tất cả file `.opus` từ `audio_files/`
2. Thêm vào assets hiện có
3. Tạo file `assets.bin` mới

### Bước 2: Kiểm tra assets.bin

```bash
python verify_assets.py build/assets.bin
```

Output:
```
Assets Summary:
  Total files: 45
  Audio files: 16
  Font files: 1
  Model files: 1
  Image files: 27
  
Audio files:
  ✓ greetings/greeting_morning.opus (12.5 KB)
  ✓ greetings/greeting_afternoon.opus (11.2 KB)
  ✓ warnings/warn_seatbelt.opus (14.8 KB)
  ...
```

---

## Flash Riêng Partition Assets

### Phương pháp 1: Dùng esptool.py

```bash
# Tìm offset của assets partition (xem partitions.csv)
# Thường là 0x610000 cho ESP32-S3

# Flash assets.bin riêng (không flash firmware)
esptool.py --chip esp32s3 --port COM5 --baud 921600 write_flash 0x610000 build/assets.bin
```

### Phương pháp 2: Dùng idf.py flash riêng partition

```bash
cd e:\xiaozhi-PhuongAnh-SdCard

# Flash chỉ partition assets
idf.py -p COM5 partition-table-flash  # Flash partition table trước
idf.py -p COM5 app-flash              # Flash app (nếu cần)

# Hoặc flash trực tiếp file assets.bin
python -m esptool --chip esp32s3 --port COM5 write_flash 0x610000 scripts/audio_assets/build/assets.bin
```

### Phương pháp 3: Script tự động

```bash
cd scripts/audio_assets
python flash_assets.py --port COM5
```

---

## Kiểm Tra Kết Quả

### 1. Xem log khởi động ESP32:

Kết nối Serial Monitor và restart thiết bị:

```
I (1234) OfflineAudioAssets: Đang khởi tạo offline audio assets...
I (1235) OfflineAudioAssets: Assets header: 45 files, checksum=0x00001234, length=1500000
I (1240) OfflineAudioAssets: ✓ greetings/greeting_morning.opus (12800 bytes)
I (1245) OfflineAudioAssets: ✓ greetings/greeting_afternoon.opus (11500 bytes)
I (1250) OfflineAudioAssets: ✓ warnings/warn_seatbelt.opus (15200 bytes)
...
I (1300) OfflineAudioAssets: Đã tải 16 file audio thành công
```

### 2. Test phát audio offline:

Nhấn giữ nút Boot 3 giây để chuyển sang chế độ offline, sau đó kiểm tra các tính năng:
- Khởi động lại: Nghe lời chào tự động
- Cắm dây an toàn: Nghe cảnh báo

---

## 🔧 Khắc Phục Sự Cố

### Lỗi "File not found"
- Kiểm tra tên file chính xác (phân biệt hoa/thường)
- Đảm bảo file có đuôi `.opus`

### Lỗi "Invalid audio format"
- Kiểm tra sample rate = 16000 Hz
- Kiểm tra codec = Opus
- Dùng `ffprobe output.opus` để kiểm tra

### Lỗi flash
- Kiểm tra đúng COM port
- Kiểm tra offset partition đúng
- Thử giảm baud rate xuống 115200

---

## 📝 Lưu Ý Quan Trọng

1. **Kích thước partition**: Assets partition có khoảng 2.2MB. Đảm bảo tổng kích thước không vượt quá.

2. **Định dạng audio**: Phải dùng Opus trong container OGG, không phải MP3 hay WAV.

3. **Sample rate**: Phải là 16000 Hz để khớp với TTS và audio decoder.

4. **Backup**: Luôn backup assets.bin trước khi flash mới.

5. **Đồng bộ code**: Tên file trong assets phải khớp với tên trong code C++ (`offline_audio_assets.h`).
