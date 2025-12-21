# 📡 WiFi & Chatbot Workflow Guide

## 📋 Tóm Tắt Nhanh

| Trạng Thái | Chatbot Hoạt Động | Chức Năng |
|----------|------------------|----------|
| **Không WiFi** | ✅ Có (Offline Mode) | Nghe/trả lời bằng audio offline, phát nhạc SD, kiểm soát xe CAN |
| **Có WiFi mà không kết nối** | ⚠️ Một phần (Transitional) | Tự động kết nối nếu đã lưu WiFi, không bắt buộc |
| **WiFi kết nối thành công** | ✅✅ Đầy đủ (Online Mode) | Chat với AI, voice assistant, tất cả tính năng |

---

## 🔌 TRường Hợp 1: Không Có WiFi (Offline Mode)

### Khởi Động Chatbot Như Thế Nào?

```
1. ESP32 khởi động
2. Kiểm tra cấu hình WiFi đã lưu
3. Nếu không có WiFi:
   ├─ CONFIG_SKIP_OTA_CHECK_AT_STARTUP = true
   │  └─> Bỏ qua kiểm tra cập nhật (tiết kiệm thời gian)
   ├─ CONFIG_ENABLE_OFFLINE_MODE = true
   │  └─> Bật chế độ offline
   └─> Hiển thị "OFFLINE MODE" trên màn hình
```

### Chatbot Hoạt Động Những Gì?

✅ **Có thể làm:**
- 🎤 **Nghe lệnh**: Sử dụng wake word nhận diện cục bộ (không cần cloud)
- 🔊 **Phát âm thanh offline**: Các file audio Opus được build trong Flash (77 files)
  - "Bố ơi, xin lỗi em không kết nối được"
  - "Em là trợ lý ảo, em không thể..."
  - Các phản hồi cơ bản khác
- 🎵 **Phát nhạc**: Từ thẻ SD card (`/music` folder)
  - Nút GPIO3: Play/Pause
  - Vol+: Bài tiếp theo
  - Vol-: Bài trước
- 🚗 **Điều khiển xe (CAN Bus)**:
  - Nút GPIO9: Bật/tắt cốp điện (Trunk)
  - Nút GPIO47: Bật/tắt điều hòa
  - Đọc dữ liệu từ Kia Morning 2017 (500kbps CAN)
- 🎨 **Hiển thị UI**: Màn hình LCD vẫn hoạt động bình thường

❌ **Không thể làm:**
- 💬 Chat với AI (cần server)
- 🗣️ Voice-to-text (cần cloud recognition)
- 📊 Truy vấn thông tin từ internet
- 🔄 Cập nhật firmware OTA

### Hành Động Sử Dụng:

```cpp
// Nút Power (GPIO0)
- Click 1 lần  : Bật/tắt nghe chỉ thị
- Giữ 1 giây   : Dừng phát nhạc/radio (nếu có)

// Nút Vol+ (GPIO2)
- Click 1 lần  : Bài nhạc tiếp theo (khi phát nhạc)
- Giữ 1 giây   : Tăng âm lượng

// Nút Vol- (GPIO1)
- Click 1 lần  : Bài nhạc trước (khi phát nhạc)
- Giữ 1 giây   : Giảm âm lượng

// Nút Nhạc (GPIO3)
- Click 1 lần    : Play/Pause
- Click 2 lần    : Bài tiếp theo
- Giữ 1 giây     : Bài trước
- Giữ 3 giây     : Bật/tắt Shuffle
```

### Khởi Động Offline Mode:
```bash
# Lần đầu build và flash
idf.py build flash

# Chỉ flash lần đầu
cd scripts
python flash_audio_assets.py
```

### Kiểm Tra Offline Mode:
```bash
# Kiểm tra audio assets đã flash
cd scripts
python check_flash_audio.py

# Nếu lỗi, flash lại
python flash_audio_assets.py
```

---

## 🌐 TRường Hợp 2: Có WiFi nhưng Chưa Kết Nối

### Khởi Động Chatbot Như Thế Nào?

```
1. ESP32 khởi động
2. Kiểm tra cấu hình WiFi đã lưu
3. Nếu có WiFi trong bộ nhớ:
   ├─> Tự động kết nối (SSID: Your_WiFi, Password: xxxx)
   └─> Hiển thị "Connecting..." trên màn hình
4. Nếu kết nối thành công:
   ├─> Chuyển sang Online Mode
   └─> Chatbot đầy đủ chức năng
5. Nếu kết nối thất bại:
   ├─> Quay về Offline Mode
   └─> Vẫn hoạt động bình thường (nhưng không AI)
```

### Các Trạng Thái LED Hiển Thị:

| LED Status | Ý Nghĩa |
|----------|---------|
| 🟢 Xanh | Kết nối thành công (Online Mode) |
| 🟡 Vàng | Đang kết nối... |
| 🔴 Đỏ | Lỗi / Offline Mode |
| ⚪ Trắng | Bắt đầu setup WiFi mới |

---

## 📱 TRường Hợp 3: WiFi Kết Nối Thành Công (Online Mode)

### Khởi Động Chatbot Như Thế Nào?

```
1. ESP32 khởi động
2. Kết nối WiFi tự động (nếu đã lưu)
3. Kết nối MQTT hoặc WebSocket với server AI
4. Hiển thị "ONLINE" trên màn hình
5. Sẵn sàng chat với AI
```

### Chatbot Hoạt Động Những Gì?

✅ **Tất cả tính năng Offline Mode + thêm:**
- 🤖 **Chat với AI Server**: Gửi tin nhắn, nhận phản hồi
- 🗣️ **Voice Recognition**: Nhận diện giọng nói cloud
- 📊 **Truy vấn thông tin**: Thời tiết, tin tức, v.v.
- 🔄 **OTA Updates**: Cập nhật firmware qua WiFi
- 💾 **Cloud Sync**: Đồng bộ cấu hình

---

## 🔧 Làm Thế Nào Để Thay Đổi WiFi Mới?

### ✅ Cách 1: Nhấn Nút Setup (Khuyên Dùng)

```
1. Nhấn nút Power (GPIO0) lúc boot hoặc sau khởi động
   ├─> Nếu đang offline: Hiển thị "WiFi Config"
   ├─> LED bắt đầu nhấp nháy
2. Dùng điện thoại tìm WiFi:
   ├─> SSID: "xiaozhi-xxxxxx" (tên thiết bị)
   └─> Mật khẩu: không có (open network)
3. Kết nối vào WiFi đó
4. Trình duyệt tự động mở: http://192.168.4.1
5. Chọn WiFi mới muốn kết nối
6. Nhập mật khẩu WiFi
7. Nhấn "Connect"
8. Chờ 5-10 giây kết nối
9. Nếu thành công → LED xanh → Online Mode ✅
10. Nếu thất bại → LED đỏ → Offline Mode (quay lại bước 1)
```

### ✅ Cách 2: Xóa WiFi Cũ + Setup Mới

```
1. Cắm điện cho thiết bị
2. Nhấn nút Power (GPIO0) lúc đang khởi động (first 3 seconds)
   └─> Xóa tất cả cấu hình WiFi cũ
3. Làm lại bước 2-10 từ Cách 1
```

### ✅ Cách 3: Reset Toàn Bộ (Hard Reset)

Nếu không đủ bộ nhớ hoặc lỗi cấu hình:

```bash
# Cách 1: Dùng esptool.py
esptool.py --chip esp32s3 --port COM3 erase_flash
idf.py flash

# Cách 2: Từ Arduino IDE
- Tools → Erase All Flash Before Sketch Upload
```

### 📝 Cấu Hình WiFi Được Lưu Ở Đâu?

```
NVS Partition (Non-Volatile Storage):
├─ SSID: Your_WiFi_Name
├─ Password: Your_Password
└─ Other settings...

Nếu reset → Tất cả mất → Cần setup lại
```

---

## 🎬 Thay Đổi WiFi - Ví Dụ Thực Tế

### Kịch Bản: Đổi từ "Home_WiFi" sang "Car_WiFi_5G"

```
TRƯỚC:
  Thiết bị: Kết nối "Home_WiFi" (192.168.1.x)
  LED: Xanh (Online)

CÁC BƯỚC:
  1. Nhấn nút Power → Màn hình: "WiFi Configuration"
  2. Điện thoại → Tìm WiFi → "xiaozhi-0A1B2C"
  3. Kết nối (không cần mật khẩu)
  4. Trình duyệt → http://192.168.4.1 (tự động)
  5. Danh sách WiFi hiển thị:
     - Home_WiFi (cũ)
     - Car_WiFi_5G (mới)
     - Neighbor_WiFi
     - ...
  6. Nhấn "Car_WiFi_5G"
  7. Nhập mật khẩu: "Car12345"
  8. Nhấn "Connect"
  9. Chờ... (LED nhấp nháy vàng)
     
SAU:
  ✅ LED xanh → Kết nối thành công
  ✅ Màn hình: "ONLINE"
  ✅ Thiết bị tự động kết nối "Car_WiFi_5G" lần sau boot
```

---

## 📊 So Sánh Các Mode

```
┌─────────────────┬──────────────────┬──────────────────┬────────────────┐
│   Tính Năng     │  Offline Mode    │ Transitional (*) │  Online Mode   │
├─────────────────┼──────────────────┼──────────────────┼────────────────┤
│ Wake Word       │ ✅ Local         │ ⚠️ Limited        │ ✅ Cloud       │
│ Chat AI         │ ❌               │ ⚠️ Cached         │ ✅             │
│ Phát Nhạc       │ ✅               │ ✅               │ ✅             │
│ CAN Bus (Xe)    │ ✅               │ ✅               │ ✅             │
│ OTA Update      │ ❌               │ ❌               │ ✅             │
│ Internet Info   │ ❌               │ ⚠️ Cache          │ ✅             │
│ Âm Thanh Offline│ ✅ Flash/SD      │ ✅               │ ✅             │
│ NVS Settings    │ ✅               │ ✅               │ ✅             │
└─────────────────┴──────────────────┴──────────────────┴────────────────┘

(*) = WiFi có nhưng không kết nối được
```

---

## 🐛 Xử Sự Cố

### Vấn Đề: WiFi không tìm được thiết bị "xiaozhi-xxxxxx"

**Nguyên nhân**: Thiết bị chưa vào WiFi Config mode

**Giải pháp**:
1. Khởi động lại thiết bị (cắm điện)
2. Khi có LED phát sáng → Nhấn nút Power (GPIO0)
3. Đợi 2-3 giây → Nên thấy "xiaozhi-xxxxx" WiFi

### Vấn Đề: Kết nối WiFi thất bại

**Nguyên nhân**: Mật khẩu sai hoặc WiFi quá yếu

**Giải pháp**:
1. Kiểm tra mật khẩu lại
2. Đặt thiết bị gần router (WiFi 5-10m)
3. Xóa cấu hình cũ: Nhấn Power lúc boot
4. Setup lại

### Vấn Đề: Offline mode không phát được âm thanh

**Nguyên nhân**: Audio assets chưa được flash

**Giải pháp**:
```bash
cd scripts
python build_audio_assets.py
python flash_audio_assets.py
python check_flash_audio.py
```

---

## 📦 Cấu Hình Trong Code

### Tệp: `config.h`

```cpp
// Bật Offline Mode
#define CONFIG_ENABLE_OFFLINE_MODE

// Bỏ qua kiểm tra OTA khi khởi động
#define CONFIG_SKIP_OTA_CHECK_AT_STARTUP

// Ưu tiên audio từ Flash (hoặc SD fallback)
#define CONFIG_OFFLINE_AUDIO_FROM_FLASH
// #define CONFIG_OFFLINE_AUDIO_FROM_SD

// Nút GPIO cho phát nhạc
#define MUSIC_BUTTON_GPIO GPIO_NUM_3
```

### Nếu Muốn Tắt Offline Mode:

```cpp
// Trong config.h, comment out:
// #define CONFIG_ENABLE_OFFLINE_MODE

// Build lại
idf.py build flash
```

---

## 🎯 Best Practices

1. **Setup WiFi lần đầu**: 
   - Dùng WiFi mạnh (gần router)
   - WiFi 2.4GHz (ổn định hơn)

2. **Thay Đổi WiFi Thường Xuyên**:
   - Giữ config cũ (auto-connect)
   - Chỉ setup mới khi cần thay đổi

3. **Offline Mode là Safety**:
   - Chatbot luôn hoạt động, dù WiFi gặp sự cố
   - Audio assets trong Flash → bảo đảm an toàn

4. **Maintenance**:
   - Kiểm tra audio assets: `python check_flash_audio.py`
   - Update firmware thường xuyên: `idf.py build flash`
   - Audio assets vẫn lưu giữ

---

## 📞 Liên Hệ

Nếu có vấn đề:
- Kiểm tra log: `idf.py monitor`
- Check forum: Xiaozhi AI
- Issues: GitHub repository
