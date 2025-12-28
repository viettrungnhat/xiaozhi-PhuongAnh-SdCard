# Xiaozhi ESP32 - Phiên bản Việt Nam -  Series Đồ chơi cho Phương Anh 098683806

<div style="display: flex; justify-content: space-between;>

**Chatbot AI Giọng Nói Tiếng Việt Trên Nền Tảng ESP32**
  <a href="docs/images/01_avata.jpg" target="_blank" title="Xingzhi Cube 1.54tft Board">
    <img src="docs/images/01_avata.jpg" width="480" />
  </a>
</div>

---

## 🌐 Cộng Đồng & Hỗ Trợ

Tham gia cộng đồng Xiaozhi AI-IoT Vietnam để nhận hỗ trợ, chia sẻ kinh nghiệm và cập nhật tính năng mới:

|
## 📖 Giới Thiệu

**Xiaozhi ESP32 Vietnam** là phiên bản tùy chỉnh của dự án Xiaozhi AI Chatbot, được phát triển đặc biệt cho cộng đồng Việt Nam với nhiều tính năng bổ sung và tối ưu hóa cho thị trường Việt.

Dự án sử dụng chip ESP32 kết hợp với các mô hình AI lớn (Qwen, DeepSeek) để tạo ra một chatbot tương tác bằng giọng nói thông minh, hỗ trợ điều khiển IoT thông qua giao thức MCP.

### 🎯 Dự Án Gốc

Dự án này được fork và phát triển từ [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) của tác giả Xiage.

---

## ✨ Tính Năng Đã Phát Triển

### 🎵 Giải Trí & Âm Nhạc

#### Nghe Nhạc Việt Nam
- Phát nhạc từ server music với khả năng cấu hình linh hoạt
- **Không cần build lại ROM** khi thay đổi link server music
- Hỗ trợ streaming chất lượng cao

#### Radio Việt Nam & Quốc Tế
- 📻 Radio VOV (Đài Tiếng nói Việt Nam)
- 🌍 Kênh radio tiếng Anh
- Thêm nhiều kênh radio khác

#### Hiển Thị Phổ Nhạc
- 📊 Hiển thị phổ âm thanh trực quan trên màn hình **LCD**
- 📊 Hiển thị phổ âm thanh trên màn hình **OLED**
- Giao diện trực quan, đẹp mắt khi phát nhạc và Radio

### 🔄 Cập Nhật & Triển Khai





### 🤖 Hỗ Trợ Phần Cứng:

#### Xingzhi Cube:
<div style="display: flex; justify-content: space-between;">
  <a href="docs/images/02_Xingzhi_Cube.jpg" target="_blank" title="Xingzhi Cube 1.54tft Board">
    <img src="docs/images/02_Xingzhi_Cube.jpg" width="480" />
  </a>
</div>

#### Tự lắp theo sơ đồ kết nối: 
<div style="display: flex; justify-content: space-between;">
  <a href="docs/images/03_diy_01.jpg" target="_blank" title="Sơ đồ kết nối mạch ĐEN">
    <img src="docs/images/03_diy_01.jpg" width="480" />
  </a>
  <a href="docs/images/03_diy_02.jpg" target="_blank" title="Sơ đồ kết nối mạch TÍM">
    <img src="docs/images/03_diy_02.jpg" width="480" />
  </a>
  <a href="docs/images/03_diy_03.jpg" target="_blank" title="Sơ đồ kết nối mạch TÍM">
    <img src="docs/images/03_diy_03.jpg" width="480" />
  </a>
  <a href="docs/images/03_diy_04.jpg" target="_blank" title="Sơ đồ kết nối mạch TÍM">
    <img src="docs/images/03_diy_04.jpg" width="480" />
  </a>
</div>

#### Otto Robot Board
- Hỗ trợ new partition để build firmware cho board Otto Robot
- Tích hợp điều khiển động cơ servo
- Phù hợp cho các dự án robot giáo dục

---

## 🚀 Tính Năng Đang Phát Triển

Các tính năng sau đây đang được phát triển tích cực và sẽ được phát hành trong các phiên bản tương lai:

### 🎵 Đa Phương Tiện Nâng Cao

| Tính năng | Mô tả | Trạng thái |
|-----------|-------|------------|
| 💾 **Play music from SD card** | Phát nhạc trực tiếp từ thẻ nhớ SD | 🔨 Đang phát triển |
| 🎬 **Play video from SD** | Phát video từ thẻ nhớ SD trên màn hình LCD | 🔨 Đang phát triển |
| 🔊 **Phát nhạc qua Bluetooth** | Kết nối và phát nhạc qua loa Bluetooth | 🔨 Đang phát triển |

### 💰 Tích Hợp Thanh Toán & Tiện Ích Tin Tức

| Tính năng | Mô tả | Trạng thái |
|-----------|-------|------------|
| 💳 **QR Code Ngân Hàng** | Hiển thị mã QR thanh toán ngân hàng Việt Nam | 📋 Kế hoạch |
| 📈 **Giá Vàng** | Cập nhật giá vàng trong nước theo thời gian thực | 📋 Kế hoạch |
| 📊 **Giá Chứng Khoán** | Hiển thị giá cổ phiếu VN-Index, HNX-Index | 📋 Kế hoạch |
| 📰 **Tin Tức Tài Chính** | Cập nhật tin tức kinh tế Việt Nam | 📋 Kế hoạch |

### 📱 Kết Nối Di Động

| Tính năng | Mô tả | Trạng thái |
|-----------|-------|------------|
| 🍎 **ANCS Bluetooth iPhone** | Kết nối iPhone nhận thông báo chỉ đường, tin nhắn | 🔨 Đang phát triển |
| 📲 **Thông Báo Cuộc Gọi** | Hiển thị thông tin người gọi từ điện thoại | 📋 Kế hoạch |

### ⚙️ Hệ Thống & Cấu Hình

| Tính năng | Mô tả | Trạng thái |
|-----------|-------|------------|
| 🌐 **OTA qua Webserver Nhúng** | Cập nhật firmware qua webserver tích hợp trong chip | 🔨 Đang phát triển |
| 🔧 **Web Server Cấu Hình GPIO** | Giao diện web để cấu hình chân GPIO | 🔨 Đang phát triển |

---

## 📶 Quản Lý WiFi & Chế Độ Offline

### 🎤 Điều Khiển Bằng Giọng Nói

#### 📴 Chế Độ OFFLINE (Không cần WiFi)
```
Nói: "Bật offline" hoặc "Chế độ offline"
```
- ✅ Thiết bị hoạt động **không cần internet**
- ✅ CAN bus đọc dữ liệu xe OK
- ✅ Phát nhạc từ SD card OK
- ✅ Điều khiển relay (cốp, điều hòa) OK
- ❌ Không có cloud AI, TTS online

#### 📶 Chế Độ ONLINE (Kết nối WiFi)
```
Nói: "Bật online" hoặc "Chế độ online"
```
- Nếu có WiFi cũ đã lưu → Tự động kết nối
- Nếu chưa có WiFi → Tạo hotspot để cấu hình

#### 🔄 Reset WiFi (Cấu hình lại từ đầu)
```
Nói: "Reset WiFi" hoặc "Cấu hình WiFi mới"
```
- Hệ thống tạo **WiFi hotspot** tên: `0986183806-XXXX`
- Dùng điện thoại kết nối vào hotspot
- Trình duyệt tự động mở trang cấu hình (captive portal)
- Chọn WiFi mới và nhập mật khẩu
- Lưu → Thiết bị tự động kết nối

### 🔘 Điều Khiển Bằng Phím Cứng

#### Boot Button (GPIO0)
**Khi thiết bị đang khởi động (LED nhấp nháy):**
- Nhấn **Boot button 1 lần** → Kích hoạt WiFi configuration portal

**Sau khi đã khởi động:**
- Nhấn **1 lần**: Bật/Tắt chatbot
- Nhấn **giữ lâu**: Dừng nhạc/radio đang phát

### 🔄 Quy Trình Test Offline Trên Xe

#### Bước 1: Bật chế độ Offline (tại nhà có WiFi)
```
1. Nói: "Bật offline"
2. Thiết bị restart tự động
3. Màn hình hiển thị "CHẾ ĐỘ OFFLINE"
4. Thiết bị hoạt động không cần WiFi
```

#### Bước 2: Test trên xe
```
5. Cắm OBD-II vào xe
6. Bật máy xe
7. Test CAN bus, điều khiển cốp, AC...
8. Phát nhạc từ SD card
```

#### Bước 3: Quay lại Online (khi về nhà)
```
9. Nói: "Bật online"
10. Nếu có WiFi cũ → Tự động kết nối
11. Nếu chưa có → Tạo hotspot để cấu hình
```

### ⚙️ Lưu Ý Kỹ Thuật

- **Offline mode**: Thiết bị hoạt động đầy đủ tính năng local (CAN, SD, relay)
- **Online mode**: Thêm cloud AI, TTS, OTA update
- **Flag offline**: Lưu trong NVS, giữ qua lần khởi động
- **Boot button**: Luôn có thể vào WiFi configuration khi khởi động

---

## 🔊 Tạo và Flash Offline Audio Assets

Để chế độ offline hoạt động với âm thanh (lời chào, cảnh báo), cần tạo và flash các file audio vào partition assets.

### 📋 Yêu Cầu

```bash
# Cài đặt Python dependencies
pip install edge-tts ffmpeg-python esptool

# Cài đặt FFmpeg (Windows)
choco install ffmpeg
# hoặc download từ https://ffmpeg.org/download.html
```

### 🎵 Bước 1: Tạo File Audio Từ Text

```bash
cd scripts/audio_assets

# Tạo audio từ danh sách văn bản tiếng Việt
python create_audio_from_text.py

# Hoặc dùng giọng nam
python create_audio_from_text.py --voice vi-VN-NamMinhNeural
```

Các file audio sẽ được tạo trong thư mục `scripts/audio_assets/audio_files/`:
- `greetings/greeting_morning.opus` - Chào buổi sáng
- `greetings/greeting_default.opus` - Chào mặc định
- `warnings/warn_seatbelt.opus` - Nhắc thắt dây an toàn
- `system/offline_mode.opus` - Thông báo offline
- ... và nhiều file khác

### 📝 Tùy Chỉnh Nội Dung Audio

Chỉnh sửa file `scripts/audio_assets/audio_text_config.json`:

```json
{
    "greetings/greeting_morning.opus": "Xin chào buổi sáng! Chúc bạn một ngày tốt lành!",
    "warnings/warn_seatbelt.opus": "Xin vui lòng thắt dây an toàn trước khi khởi hành."
}
```

### 📦 Bước 2: Build Assets.bin

```bash
cd scripts/audio_assets

# Build assets mới (chỉ audio)
python build_audio_assets.py

# Hoặc merge với assets.bin hiện có (giữ fonts, models, emojis)
python build_audio_assets.py --merge-with ../../build/assets.bin
```

### ✅ Bước 3: Xác Minh Assets

```bash
python verify_assets.py build/assets.bin
```

Output:
```
📦 File: build/assets.bin
   Kích thước: 1500.5 KB
📊 Tổng kết:
   🔊 Audio files: 16
   🔤 Font files: 1
   🧠 Model files: 1
   🖼️ Image files: 27
```

### ⚡ Bước 4: Flash Assets (Không Flash Firmware)

```bash
cd scripts/audio_assets

# Flash assets.bin vào partition assets
python flash_assets.py --port COM5

# Hoặc với tốc độ chậm hơn nếu bị lỗi
python flash_assets.py --port COM5 --baud 115200
```

**Offset mặc định:**
- 8MB flash: `0x600000`
- 16MB flash: `0x9F0000`
- 4MB flash: `0x300000`

### 🔄 Quy Trình Đầy Đủ

```bash
# 1. Tạo audio từ text
cd scripts/audio_assets
python create_audio_from_text.py

# 2. Build assets (merge với assets hiện có)
python build_audio_assets.py --merge-with ../../build/assets.bin

# 3. Xác minh
python verify_assets.py build/assets.bin

# 4. Flash
python flash_assets.py --port COM5

# 5. Restart thiết bị và kiểm tra log
```

Xem chi tiết đầy đủ tại: [scripts/audio_assets/README.md](scripts/audio_assets/README.md)

| 🎚️ **Tăng Mic Gain với UI** | Điều chỉnh độ nhạy microphone qua giao diện | 🔨 Đang phát triển |
| 🔄 **Update V1 lên V2** | Hỗ trợ nâng cấp từ phiên bản V1 lên V2 | 📋 Kế hoạch |
| 🖥️ **Hỗ Trợ Màn Hình Mới** | Build firmware cho các loại màn hình mới | 🔨 Đang phát triển |

### ⏰ Tiện Ích Thông Minh

| Tính năng | Mô tả | Trạng thái |
|-----------|-------|------------|
| ⏰ **Hẹn Giờ Báo Thức** | Thiết lập và quản lý nhiều báo thức | 🔨 Đang phát triển |
| 🎙️ **Chủ Động Wakeup & thông báo** | Tự động kích hoạt và gửi văn bản theo lịch | 📋 Kế hoạch |

### 🏭 Tự Động Hóa Công Nghiệp

| Tính năng | Mô tả | Trạng thái |
|-----------|-------|------------|
| 🏭 **ModBus RTU** | Giao thức ModBus RTU để điều khiển thiết bị công nghiệp | 📋 Kế hoạch |
| 🌐 **ModBus TCP/IP** | Giao thức ModBus TCP/IP qua mạng Ethernet/WiFi | 📋 Kế hoạch |

**Chú thích trạng thái:**
- 🔨 Đang phát triển: Đang được code và test
- 📋 Kế hoạch: Đã lên kế hoạch, chưa bắt đầu phát triển

---

## 🎯 Tính Năng Cốt Lõi (Từ Dự Án Gốc)

### Kết Nối & Mạng
- ✅ Wi-Fi
- ✅ ML307 Cat.1 4G
- ✅ Websocket hoặc MQTT+UDP
- ✅ OPUS audio codec

### AI & Giọng Nói
- ✅ Wake word offline với [ESP-SR](https://github.com/espressif/esp-sr)
- ✅ ASR + LLM + TTS streaming
- ✅ Nhận dạng giọng nói với [3D Speaker](https://github.com/modelscope/3D-Speaker)
- ✅ Đa ngôn ngữ (Tiếng Trung, Tiếng Anh, Tiếng Nhật)

### Hiển Thị & Phần Cứng
- ✅ Màn hình OLED / LCD
- ✅ Hiển thị biểu cảm
- ✅ Quản lý pin
- ✅ ESP32-C3, ESP32-S3, ESP32-P4

### Điều Khiển IoT
- ✅ MCP phía thiết bị (âm lượng, LED, motor, GPIO)
- ✅ MCP đám mây (nhà thông minh, desktop, email)
- ✅ Tùy chỉnh wake word, font, biểu cảm

---

## 🛠️ Bắt Đầu Nhanh

### Cho Người Dùng Cuối


2. **Cấu Hình WiFi**
   - Bật thiết bị
   - Kết nối vào WiFi của ESP32
   - Cấu hình WiFi nhà bạn

3. **Sử Dụng**
   - Nói "Sophia" để đánh thức
   - Bắt đầu trò chuyện

### Cho Nhà Phát Triển

#### Yêu Cầu
- **IDE**: VSCode hoặc Cursor
- **Plugin**: ESP-IDF v5.4+
- **Hệ điều hành**: Linux (khuyến nghị) hoặc Windows
- **Code Style**: Google C++ Style Guide

#### Build Từ Source



# Cài đặt ESP-IDF dependencies
# (Theo hướng dẫn của ESP-IDF)

# Build
idf.py build

# Flash
idf.py -p COM_PORT flash monitor
```

---

#
## 📄 Giấy Phép

Dự án này được phát hành dưới giấy phép **MIT License**, kế thừa từ dự án gốc xiaozhi-esp32.

Bạn có thể:
- ✅ Sử dụng miễn phí cho mục đích cá nhân
- ✅ Sử dụng cho mục đích thương mại
- ✅ Chỉnh sửa và phân phối lại

---

## 🙏 Cảm Ơn

### Dự Án Gốc
- [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) - Dự án gốc bởi Xiage

### Thư Viện & Framework
- [ESP-IDF](https://github.com/espressif/esp-idf) - Espressif IoT Development Framework
- [ESP-SR](https://github.com/espressif/esp-sr) - Speech Recognition Framework
- [3D Speaker](https://github.com/modelscope/3D-Speaker) - Speaker Recognition

### Cộng Đồng
- Cảm ơn tất cả các thành viên trong nhóm Zalo và Facebook
- Cảm ơn những người đã đóng góp code và ý tưởng

---

## 📞 Liên Hệ

098683806
<div align="center">

**Made with ❤️ by Vietnam AI-IoT Community**

⭐ Nếu project này hữu ích, hãy cho chúng tôi một star nhé! ⭐

</div>