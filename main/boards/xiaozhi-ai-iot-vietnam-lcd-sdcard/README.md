# Xiaozhi AI IoT Vietnam - LCD + SDCard (Board README)

Tài liệu ngắn cho bo `xiaozhi-ai-iot-vietnam-lcd-sdcard` về cách đấu dây, chức năng các nút và cách sử dụng module thẻ SD qua SPI.

**Tham khảo mã nguồn:** [main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/config.h](main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/config.h) và [main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/xiaozhi_ai_iot_vietnam_board_lcd_sdcard.cc](main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/xiaozhi_ai_iot_vietnam_board_lcd_sdcard.cc#L1-L400)

**Tổng quan**
- Bo hỗ trợ màn hình SPI (ST7789/ILI9341/GC9A01 tùy cấu hình), âm thanh I2S (simplex hoặc duplex), 3 nút vật lý và hỗ trợ thẻ SD bằng SDMMC hoặc SPI.
- **Hỗ trợ CAN Bus** cho giao tiếp với xe Kia Morning 2017 Si qua module SN65HVD230.
- **Chế độ Offline** - Hoạt động cơ bản khi không có WiFi, phát âm thanh cảnh báo và nhạc từ thẻ SD.
- **Nút phát nhạc riêng** - GPIO3 để điều khiển phát nhạc MP3 từ thẻ SD.
- README này tập trung hướng dẫn khi bạn sử dụng module thẻ SD qua giao tiếp SPI (module SD SPI kiểu breakout hoặc module microSD thứ nhất).

---

## 🎵 Chế độ Offline & Phát nhạc SD

Bo mạch hỗ trợ hoạt động cơ bản khi không có WiFi và phát nhạc/âm thanh từ thẻ SD.

### Tính năng Offline Mode

| Tính năng | Mô tả | Yêu cầu WiFi |
|-----------|-------|--------------|
| **Phát nhạc SD** | Phát file MP3 từ thẻ nhớ | ❌ Không |
| **Cảnh báo offline** | Phát âm thanh opus đã ghi sẵn | ❌ Không |
| **Đọc thông số xe** | Phát thông tin từ CAN bus | ❌ Không |
| **Trợ lý AI đầy đủ** | Nhận lệnh giọng nói, TTS | ✅ Cần WiFi |

### Cấu hình Offline Mode trong config.h

```c
// Bật chế độ offline
#define CONFIG_ENABLE_OFFLINE_MODE

// Bỏ qua kiểm tra cập nhật khi khởi động
#define CONFIG_SKIP_OTA_CHECK_AT_STARTUP

// Chọn nguồn âm thanh offline
#define CONFIG_OFFLINE_AUDIO_FROM_FLASH     // Ưu tiên: Flash assets
// #define CONFIG_OFFLINE_AUDIO_FROM_SD     // Phụ: SD card (fallback)
```

### 2 Cách cài đặt Audio Offline

| | **Flash Assets** | **SD Card** |
|---|---|---|
| **Ưu điểm** | ✅ Đáng tin cậy<br>✅ Nhanh hơn<br>✅ Không phụ thuộc SD | ✅ Dễ sửa<br>✅ Không chiếm Flash |
| **Nhược điểm** | ⚠️ Phải flash lại để sửa<br>⚠️ Chiếm ~600KB flash | ⚠️ Phụ thuộc SD<br>⚠️ Chậm hơn |
| **Cách build** | `idf.py build` (tự động)<br>`python scripts/flash_audio_assets.py` | Copy `audio_opus/` vào SD |

### Cách 1: Build vào Flash Assets (Khuyến nghị cho xe)

```bash
# 1. Build firmware (audio tự động được build)
idf.py build

# 2. Flash firmware
idf.py flash

# 3. Flash audio assets riêng
cd main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard
python scripts/flash_audio_assets.py <PORT>

# Hoặc flash manual với offset
esptool.py --port <PORT> write_flash 0x800000 scripts/assets/audio_assets.bin
```

### Cách 2: Copy vào SD Card (Fallback)

```bash
# Copy thư mục audio_opus vào root của SD
copy /audio_opus/* /path/to/sd/audio_opus/
```

### Cấu trúc thư mục trên thẻ SD

```
📁 SD Card Root
├── 📁 music/              ← Nhạc MP3 (phát bằng nút GPIO3)
│   ├── 🎵 bai_hat_1.mp3
│   ├── 🎵 bai_hat_2.mp3
│   └── ...
├── 📁 audio_opus/         ← Âm thanh offline cho trợ lý xe
│   ├── 📁 greetings/      ← Lời chào
│   ├── 📁 warnings/       ← Cảnh báo
│   ├── 📁 highway/        ← Chế độ đường trường
│   ├── 📁 control/        ← Điều khiển (cốp, AC)
│   ├── 📁 info/           ← Thông tin xe
│   └── 📁 numbers/        ← Số đọc
└── ...
```

---

## 🎹 Nút phát nhạc SD (GPIO3)

Thêm một nút nhấn riêng để điều khiển phát nhạc từ thẻ SD mà không cần WiFi.

### Sơ đồ đấu nối

```
┌─────────────────────────────────────────────────────────────────┐
│                      ESP32-S3 (Xiaozhi)                         │
│                                                                 │
│     GPIO3 (Music Button) ───────────┐                           │
│     GND ──────────────────────┐     │                           │
│                               │     │                           │
└───────────────────────────────│─────│───────────────────────────┘
                                │     │
                           ┌────┴─────┴────┐
                           │    Nút nhấn   │
                           │  (Push Button)│
                           └───────────────┘

Hoặc dùng công tắc có sẵn trên xe:
GPIO3 ──────── Công tắc ──────── GND
```

### Chức năng nút nhấn

| Thao tác | Chức năng |
|----------|-----------|
| **Nhấn 1 lần** | Play/Pause nhạc |
| **Nhấn 2 lần nhanh** | Chuyển bài tiếp theo ⏭ |
| **Nhấn giữ 1 giây** | Quay lại bài trước ⏮ |
| **Nhấn giữ 3 giây** | Bật/Tắt Shuffle 🔀 |

### Cấu hình trong config.h

```c
// GPIO cho nút phát nhạc
#define MUSIC_BUTTON_GPIO           GPIO_NUM_3
#define MUSIC_BUTTON_ACTIVE_LOW     true    // Nút nhấn nối GND

// Chế độ phát nhạc mặc định
#define MUSIC_AUTO_PLAY_ON_BOOT     false   // Tự động phát khi khởi động
#define MUSIC_SHUFFLE_DEFAULT       true    // Mặc định bật shuffle
#define MUSIC_REPEAT_ALL_DEFAULT    true    // Mặc định lặp lại toàn bộ

// Thời gian phát hiện thao tác (ms)
#define MUSIC_BUTTON_DEBOUNCE_MS    50
#define MUSIC_BUTTON_DOUBLE_CLICK_MS 300
#define MUSIC_BUTTON_LONG_PRESS_MS  1000
```

### Tóm tắt tất cả nút nhấn

| Nút | GPIO | Chức năng chính |
|-----|------|-----------------|
| **BOOT** | GPIO0 | Toggle trò chuyện AI / Dừng nhạc (giữ) |
| **Volume Up** | GPIO2 | Tăng âm lượng / Next track (khi đang phát) |
| **Volume Down** | GPIO1 | Giảm âm lượng / Pause (khi đang phát) |
| **Music** | GPIO3 | Play/Pause/Next/Prev nhạc SD (mới) |

---

## 🚗 CAN Bus - Kết nối với xe Kia Morning 2017 Si

Bo mạch hỗ trợ kết nối CAN Bus để đọc dữ liệu từ xe và cung cấp các tính năng trợ lý thông minh.

### Sơ đồ đấu nối Module SN65HVD230

```
┌─────────────────────────────────────────────────────────────────┐
│                      ESP32-S3 (Xiaozhi)                         │
│                                                                 │
│     GPIO17 (CAN TX) ──────────────────────┐                     │
│     GPIO8  (CAN RX) ─────────────────┐    │                     │
│     3.3V ─────────────────────┐      │    │                     │
│     GND ───────────────┐      │      │    │                     │
│                        │      │      │    │                     │
└────────────────────────│──────│──────│────│─────────────────────┘
                         │      │      │    │
                         │      │      │    │
┌────────────────────────│──────│──────│────│─────────────────────┐
│   Module SN65HVD230    │      │      │    │                     │
│                        │      │      │    │                     │
│      GND ◄─────────────┘      │      │    │                     │
│      VCC ◄────────────────────┘      │    │                     │
│      CRX ◄───────────────────────────┘    │                     │
│      CTX ◄────────────────────────────────┘                     │
│                                                                 │
│      CANH ─────────────────────────────────► OBD-II Pin 6       │
│      CANL ─────────────────────────────────► OBD-II Pin 14      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘


                     ┌───────────────────────────────────────┐
                     │    Cổng OBD-II trên xe Kia Morning    │
                     │                                       │
                     │    1   2   3   4   5   6   7   8      │
                     │    ●   ●   ●   ●   ●   ●   ●   ●      │
                     │                        ▲              │
                     │    ●   ●   ●   ●   ●   ●   ●   ●      │
                     │    9  10  11  12  13  14  15  16      │
                     │                        ▲              │
                     │                        │              │
                     │    Pin 6:  CAN High (CANH)            │
                     │    Pin 14: CAN Low (CANL)             │
                     │    Pin 4:  Chassis Ground (GND)       │
                     │    Pin 16: Battery Power (+12V)       │
                     └───────────────────────────────────────┘
```

### Bảng đấu nối chi tiết

| ESP32-S3 GPIO | SN65HVD230 Pin | Mô tả |
|---------------|----------------|-------|
| GPIO17 | CTX | CAN Transmit |
| GPIO8 | CRX | CAN Receive |
| 3.3V | VCC | Nguồn 3.3V |
| GND | GND | Mass chung |

| SN65HVD230 Pin | OBD-II Pin | Mô tả |
|----------------|------------|-------|
| CANH | Pin 6 | CAN High |
| CANL | Pin 14 | CAN Low |

### Tính năng CAN Bus cho Kia Morning 2017

#### 1. Nhóm Chức năng Chào hỏi & Cá nhân hóa
- **Lời chào tự động**: Khi cảm biến CAN Bus báo cửa tài xế mở, Xiaozhi tự động đánh thức và chào: "Chào bố, hôm nay mình đi đâu thế ạ? Bố nhớ thắt dây an toàn và hạ phanh tay nhé!"
- **Chế độ lắng nghe chủ động**: Sau lời chào, chatbot tự động mở Micro để nhận lệnh

#### 2. Nhóm Cảnh báo An toàn
- **Cảnh báo phanh tay/Dây an toàn**: Nhắc nhở nếu xe di chuyển nhưng chưa hạ phanh tay hoặc thắt dây an toàn
- **Giám sát ắc quy**: Cảnh báo khi điện áp bình xuống dưới 11.8V
- **Cảnh báo nhiệt độ máy**: Phát cảnh báo khẩn cấp nếu nước làm mát quá 100°C
- **Cảnh báo quên khóa cửa/tắt đèn**: Khi tắt máy xe mà các thông số này vẫn bật

#### 3. Nhóm Thông tin Xe (Dashboard Voice)
- **Đọc thông số hành trình**: Xăng còn lại, số km đã đi, mức tiêu hao nhiên liệu
- **Báo cáo sức khỏe xe**: Tóm tắt trạng thái xe khi lên xe
- **Nhắc lịch bảo dưỡng**: Dựa trên số Odo để nhắc thay dầu (5k km), kiểm tra lốp (10k km)

#### 4. Lệnh giọng nói hỗ trợ
- "Tốc độ bao nhiêu?" - Đọc tốc độ hiện tại
- "Xăng còn bao nhiêu?" - Đọc mức nhiên liệu và quãng đường còn đi được
- "Nhiệt độ máy bao nhiêu?" - Đọc nhiệt độ nước làm mát
- "Bật chế độ đường trường" - Bật chế độ thông báo tốc độ định kỳ
- "Bố chuẩn bị về" - Kích hoạt kịch bản về nhà

#### 5. Chế độ đường trường (Highway Mode)
- Đọc tốc độ hiện tại sau mỗi 5 phút
- Cảnh báo nếu lái xe liên tục quá 2 tiếng
- Tự động tắt khi xe dừng

### Cấu hình CAN Bus

Các thông số cấu hình trong `config.h`:

```c
// CAN Bus GPIO Pins
#define CAN_TX_GPIO         GPIO_NUM_17
#define CAN_RX_GPIO         GPIO_NUM_8

// CAN Bus Speed (Kia Morning 2017 uses 500kbps)
#define CAN_SPEED_KBPS      500

// Power Saving - tự động ngủ sau 5 phút không có dữ liệu CAN
#define CAN_IDLE_TIMEOUT_MS         (5 * 60 * 1000)

// Task Priority - ưu tiên trung bình để không ảnh hưởng audio
#define CAN_TASK_PRIORITY           5
#define CAN_TASK_CORE               1
```

### Ngưỡng cảnh báo

```c
// Battery
#define VEHICLE_BATTERY_LOW_VOLTAGE     11.8f   // Cảnh báo
#define VEHICLE_BATTERY_CRITICAL_VOLTAGE 11.0f  // Khẩn cấp

// Engine Temperature
#define VEHICLE_COOLANT_WARN_TEMP       100.0f  // Cảnh báo
#define VEHICLE_COOLANT_CRITICAL_TEMP   105.0f  // Khẩn cấp

// Driving
#define VEHICLE_MAX_DRIVE_TIME_MINUTES  120     // Nhắc nghỉ sau 2 tiếng
```

### Debug CAN Bus

Để xem log CAN bus chi tiết, chạy:
```bash
idf.py -p <PORT> monitor
```

Các log sẽ hiển thị:
- `CAN_Driver`: Trạng thái driver, lỗi, thống kê
- `Kia_CAN`: Dữ liệu xe được parse (speed, fuel, temp...)
- `Vehicle_Assistant`: Các sự kiện và cảnh báo

---

## 🔌 Relay Module - Điều khiển Cốp điện & Điều hòa

Bo mạch hỗ trợ điều khiển relay để mở cốp điện và bật/tắt điều hòa qua giọng nói.

### Sơ đồ đấu nối Module Relay

```
┌─────────────────────────────────────────────────────────────────┐
│                      ESP32-S3 (Xiaozhi)                         │
│                                                                 │
│     GPIO9  (Relay Cốp) ──────────────────────┐                  │
│     GPIO47 (Relay AC)  ─────────────────┐    │                  │
│     3.3V hoặc 5V ─────────────────┐     │    │                  │
│     GND ───────────────┐          │     │    │                  │
│                        │          │     │    │                  │
└────────────────────────│──────────│─────│────│──────────────────┘
                         │          │     │    │
                         │          │     │    │
┌────────────────────────│──────────│─────│────│──────────────────┐
│   Module Relay 2CH     │          │     │    │                  │
│   (Active LOW)         │          │     │    │                  │
│                        │          │     │    │                  │
│      GND ◄─────────────┘          │     │    │                  │
│      VCC ◄────────────────────────┘     │    │                  │
│      IN2 (AC) ◄─────────────────────────┘    │                  │
│      IN1 (Cốp) ◄─────────────────────────────┘                  │
│                                                                 │
│      NO1 ──────────────────────────────────► Cốp điện (+)       │
│      COM1 ─────────────────────────────────► +12V (ACC)         │
│      NC1 ──────────────────────────────────► Không dùng         │
│                                                                 │
│      NO2 ──────────────────────────────────► Nút điều hòa       │
│      COM2 ─────────────────────────────────► +12V               │
│      NC2 ──────────────────────────────────► Không dùng         │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘


              ┌───────────────────────────────────────────────┐
              │       Sơ đồ đấu nối Cốp điện Kia Morning      │
              │                                               │
              │   [Bộ mở cốp điện aftermarket]                │
              │                                               │
              │   Motor cốp (+) ◄───── NO1 (Relay)            │
              │   Motor cốp (-) ◄───── GND (xe)               │
              │                                               │
              │   Khi relay đóng (GPIO9 = LOW):               │
              │   +12V ──► COM1 ──► NO1 ──► Motor = MỞ CỐP   │
              │                                               │
              │   ⚠️ LƯU Ý: Cần thêm relay phụ 12V nếu       │
              │   dòng motor > 10A                            │
              └───────────────────────────────────────────────┘
```

### Bảng đấu nối Relay

| ESP32-S3 GPIO | Relay Pin | Chức năng |
|---------------|-----------|-----------|
| GPIO9 | IN1 | Điều khiển cốp điện |
| GPIO47 | IN2 | Điều khiển điều hòa (tùy chọn) |
| 3.3V/5V | VCC | Nguồn relay module |
| GND | GND | Mass chung |

### Lệnh giọng nói điều khiển Relay

| Lệnh | Chức năng |
|------|-----------|
| "Mở cốp" | Kích relay GPIO9 trong 1 giây để mở cốp |
| "Bật điều hòa" | Bật relay GPIO47 để kích điều hòa |
| "Tắt điều hòa" | Tắt relay GPIO47 |
| "Bố chuẩn bị về" | Mở cốp + thông báo sẵn sàng |

### Cấu hình Relay trong config.h

```c
// Relay Control - Cho cốp điện và điều hòa
#define CONFIG_ENABLE_RELAY_CONTROL     // Comment để tắt

#define RELAY_TRUNK_GPIO        GPIO_NUM_9   // Relay mở cốp
#define RELAY_AC_GPIO           GPIO_NUM_47  // Relay điều hòa (tùy chọn)
#define RELAY_ACTIVE_LOW        true         // Module relay Active LOW
#define RELAY_TRUNK_PULSE_MS    1000         // Thời gian xung mở cốp (1 giây)
```

### Lưu ý khi đấu Relay

⚠️ **QUAN TRỌNG**:
- Dùng module relay có optocoupler để cách ly ESP32 khỏi mạch 12V
- Relay 5V/3.3V tùy module, kiểm tra VCC của module relay
- Motor cốp điện thường cần dòng 5-15A, relay module nhỏ chỉ chịu được 10A
- Nên dùng relay module điều khiển relay công suất 12V/30A nếu dòng lớn
- Test trên xe đỗ trước khi chạy thực tế

---

### Build thử không cần SN65HVD230

Nếu chưa nối module SN65HVD230, bạn vẫn có thể build và flash để test các tính năng khác:

1. **Tắt CAN Bus** (tùy chọn): Mở file `config.h` và comment dòng sau:
   ```c
   // #define CONFIG_ENABLE_CAN_BUS  // Comment để tắt CAN bus
   ```

2. **Build và flash bình thường**:
   ```bash
   idf.py build
   idf.py -p <PORT> flash monitor
   ```

3. **Log khi CAN bị tắt**:
   ```
   CAN Bus DISABLED (SN65HVD230 not connected)
   To enable: uncomment CONFIG_ENABLE_CAN_BUS in config.h
   ```

Khi CAN bus được bật nhưng không có module, driver sẽ khởi tạo nhưng không nhận dữ liệu. Điều này không gây crash.

### Lưu ý quan trọng

⚠️ **CẢNH BÁO AN TOÀN**:
- KHÔNG can thiệp CAN bus khi xe đang chạy ở tốc độ cao
- KHÔNG gửi lệnh CAN không xác định
- Luôn test trên xe đỗ trước khi chạy thực tế
- Module SN65HVD230 phải được cấp nguồn 3.3V (KHÔNG dùng 5V)

⚠️ **LƯU Ý KỸ THUẬT**:
- CAN IDs có thể khác nhau giữa các phiên bản xe và vùng địa lý
- Cần sniff CAN bus thực tế để xác định đúng CAN IDs cho xe của bạn
- File `kia_can_protocol.cc` có thể cần điều chỉnh CAN IDs

---

## 🤖 Tính năng Chatbot trên xe Kia Morning 2017

Dưới đây là danh sách đầy đủ các tính năng của trợ lý ảo khi tích hợp với xe.

### 📢 Nhóm 1: Chào hỏi & Cá nhân hóa

| Tính năng | Mô tả | Trigger |
|-----------|-------|---------|
| **Lời chào tự động** | Khi cửa tài xế mở, chatbot tự động chào: "Chào bố, hôm nay mình đi đâu thế ạ? Bố nhớ thắt dây an toàn và hạ phanh tay nhé! Chúc chuyến đi an toàn!" | CAN: Cửa tài xế mở |
| **Chế độ lắng nghe chủ động** | Sau lời chào, tự động mở Micro để nhận lệnh mà không cần gọi "Xiaozhi" | Sau greeting |
| **Báo cáo sức khỏe xe** | Tóm tắt trạng thái xe khi mới lên: "Mọi hệ thống đều ổn định, nhiệt độ trong xe là 28°C" | CAN: Ignition ON |

### ⚠️ Nhóm 2: Cảnh báo An toàn

| Cảnh báo | Điều kiện | Mức độ | Cooldown |
|----------|-----------|--------|----------|
| **Dây an toàn** | Xe chạy > 10 km/h, tài xế chưa thắt dây | 🔴 Cao | 30 giây |
| **Phanh tay** | Xe chạy > 5 km/h, phanh tay vẫn kéo | 🔴 Cao | 30 giây |
| **Cửa mở** | Xe chạy > 10 km/h, có cửa chưa đóng | 🔴 Khẩn cấp | 30 giây |
| **Điện bình yếu** | Điện áp < 11.8V | 🟡 Trung bình | 30 giây |
| **Điện bình nguy hiểm** | Điện áp < 11.0V | 🔴 Khẩn cấp | 30 giây |
| **Nhiệt độ máy cao** | Nước làm mát > 100°C | 🟡 Cảnh báo | 30 giây |
| **Nhiệt độ máy nguy hiểm** | Nước làm mát > 105°C | 🔴 Khẩn cấp + Âm thanh | 30 giây |
| **Xăng sắp hết** | Mức xăng < 10% | 🟡 Trung bình | 30 giây |
| **Đèn còn bật** | Tắt máy nhưng đèn vẫn sáng | 🟡 Nhắc nhở | 1 lần |
| **Quên khóa cửa** | Tắt máy nhưng cửa chưa khóa | 🟡 Nhắc nhở | 1 lần |
| **Lái xe quá lâu** | Lái liên tục > 2 tiếng | 🟡 Nhắc nghỉ | 10 phút |

### 📊 Nhóm 3: Thông tin xe (Dashboard Voice)

Bạn có thể hỏi chatbot bất cứ lúc nào:

| Câu hỏi mẫu | Thông tin trả về |
|-------------|------------------|
| "Tốc độ bao nhiêu?" | "Tốc độ hiện tại là 65 km/h" |
| "Xăng còn bao nhiêu?" | "Xăng còn 45%, đủ đi khoảng 180 km" |
| "Nhiệt độ máy bao nhiêu?" | "Nhiệt độ máy bình thường, 85 độ" |
| "Xe đi được bao xa?" | "Xe đã đi tổng cộng 45000 km. Chuyến này đã đi 25 km" |
| "Điện bình bao nhiêu?" | "Điện bình 12.6 volt, ở mức tốt" |
| "Có cảnh báo gì không?" | "Không có cảnh báo nào. Xe hoạt động bình thường" |
| "Sức khỏe xe thế nào?" | Tóm tắt đầy đủ: tốc độ, xăng, nhiệt độ, điện, odo |
| "Lái được bao lâu rồi?" | "Bố đã lái xe được 1 tiếng 25 phút" |

### 🛣️ Nhóm 4: Chế độ Đường trường (Highway Mode)

Kích hoạt bằng lệnh: **"Bật chế độ đường trường"**

| Tính năng | Mô tả |
|-----------|-------|
| **Thông báo tốc độ định kỳ** | Đọc tốc độ hiện tại mỗi 5 phút |
| **Nhắc nghỉ ngơi** | Cảnh báo sau 2 tiếng lái liên tục |
| **Tự động tắt** | Khi tắt máy xe |

Tắt bằng lệnh: **"Tắt chế độ đường trường"**

### 🎬 Nhóm 5: Kịch bản thông minh (Smart Scenarios)

| Kịch bản | Lệnh kích hoạt | Hành động |
|----------|----------------|-----------|
| **Bố chuẩn bị về** | "Bố chuẩn bị về" | Thông báo sẵn sàng, (tương lai: mở cốp, bật AC, phát nhạc) |
| **Chế độ đường trường** | "Chế độ đường trường" | Bật highway mode |

### 🔧 Nhóm 6: Nhắc bảo dưỡng

Dựa trên số Odo đọc được từ CAN bus:

| Hạng mục | Chu kỳ | Thông báo |
|----------|--------|-----------|
| **Thay dầu máy** | Mỗi 5,000 km | "Đã đi X km từ lần thay dầu, nên thay dầu" |
| **Kiểm tra lốp** | Mỗi 10,000 km | "Nên kiểm tra lốp" |
| **Bảo dưỡng lớn** | Mỗi 30,000 km | "Nên đưa xe đi bảo dưỡng định kỳ" |

### 🎤 Danh sách lệnh giọng nói

```
┌─────────────────────────────────────────────────────────────────┐
│                    LỆNH GIỌNG NÓI HỖ TRỢ                        │
├─────────────────────────────────────────────────────────────────┤
│  📊 THÔNG TIN XE                                                │
│  • "Tốc độ bao nhiêu?"                                          │
│  • "Xăng còn bao nhiêu?" / "Còn đi được bao xa?"                │
│  • "Nhiệt độ máy bao nhiêu?"                                    │
│  • "Odo bao nhiêu?" / "Xe đi được bao nhiêu km?"                │
│  • "Điện bình thế nào?"                                         │
│  • "Có cảnh báo gì không?"                                      │
│  • "Sức khỏe xe thế nào?"                                       │
│  • "Lái được bao lâu rồi?"                                      │
├─────────────────────────────────────────────────────────────────┤
│  🛣️ CHẾ ĐỘ LÁI XE                                               │
│  • "Bật chế độ đường trường"                                    │
│  • "Tắt chế độ đường trường"                                    │
├─────────────────────────────────────────────────────────────────┤
│  🎬 KỊCH BẢN                                                    │
│  • "Bố chuẩn bị về"                                             │
└─────────────────────────────────────────────────────────────────┘
```

### 📁 Cấu trúc file CAN Bus

```
main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/
├── config.h                    # Cấu hình GPIO, thresholds
├── xiaozhi_ai_iot_vietnam_board_lcd_sdcard.cc  # Board init
└── canbus/
    ├── canbus_driver.h         # TWAI driver interface
    ├── canbus_driver.cc        # TWAI driver implementation
    ├── kia_can_protocol.h      # Kia CAN IDs & data structures
    ├── kia_can_protocol.cc     # CAN message parser
    ├── vehicle_assistant.h     # Assistant logic interface
    └── vehicle_assistant.cc    # Assistant implementation
```

### 🔮 Tính năng phát triển tương lai

| Tính năng | Mô tả | Yêu cầu phần cứng |
|-----------|-------|-------------------|
| **Mở cốp** | Điều khiển bằng giọng nói | Relay + Xilanh điện |
| **Điều khiển AC** | Bật/tắt, điều chỉnh nhiệt độ | Relay hoặc CAN command |
| **Điều khiển kính** | Lên/xuống kính | Relay vào cụm công tắc |
| **Bật/tắt đèn** | Điều khiển đèn chiếu | Gửi CAN command đến BCM |
| **Điều khiển gạt mưa** | Bật/tắt gạt mưa | CAN command |
| **Hiển thị HUD** | Hiện thông tin trên kính lái | Module HUD riêng |

---

**Chân (GPIO) chính**
- **SD (SPI)**: (khi kích hoạt `CONFIG_SD_CARD_SPI_INTERFACE`)
  - MOSI: GPIO39
  - MISO: GPIO41
  - SCLK: GPIO40
  - CS:   GPIO38

- **Màn hình SPI**
  - MOSI: GPIO12
  - CLK:  GPIO11
  - DC:   GPIO13
  - RST:  GPIO14
  - CS:   GPIO21
  - Backlight PWM: GPIO10

- **CAN Bus (SN65HVD230)**
  - TX: GPIO17
  - RX: GPIO8

- **Nút & LED**
  - `BOOT_BUTTON_GPIO`: GPIO0 (nút chính/boot)
  - `VOLUME_UP_BUTTON_GPIO`: GPIO2
  - `VOLUME_DOWN_BUTTON_GPIO`: GPIO1
  - LED tích hợp (`BUILTIN_LED_GPIO`): GPIO48

- **Âm thanh I2S (giá trị mặc định trong `config.h`)**
  - Speaker DOUT: GPIO7
  - Speaker BCLK: GPIO15
  - Speaker LRCK: GPIO16
  - Microphone WS/SCK/DIN: GPIO4/5/6 (tùy cấu hình)

Chi tiết các giá trị trên nằm trong [config.h](main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/config.h).

**Sử dụng module thẻ SD (SPI)**
- Kết nối module SD SPI tới các chân tương ứng: MOSI→GPIO39, MISO→GPIO41, SCLK→GPIO40, CS→GPIO38, VCC→3.3V, GND→GND.
- Trong dự án: nếu dùng SPI, `SdSPI` được khởi tạo với `SPI2_HOST` và tốc độ 15MHz (xem mã khởi tạo trong `xiaozhi_ai_iot_vietnam_board_lcd_sdcard.cc`).
- Để bật SPI SD: chạy `idf.py menuconfig` và chọn phần cấu hình thẻ SD (hoặc bật `CONFIG_SD_CARD_SPI_INTERFACE`) — đảm bảo không bật đồng thời chế độ SDMMC nếu bạn muốn dùng module SPI.

**Hành vi các nút (đã implement trong code)**
- `BOOT_BUTTON` (GPIO0)
  - Click: nếu thiết bị đang khởi động và Wi‑Fi chưa kết nối → gọi `ResetWifiConfiguration()`; sau đó `Application::ToggleChatState()` (chuyển trạng thái chat/hoạt động).
  - Long press: dừng phát nhạc từ SD hoặc dừng radio nếu đang phát, hiển thị thông báo trên màn hình.

- `VOLUME_UP` (GPIO2)
  - Click: nếu `Esp32SdMusic` đang Playing/Paused → chuyển sang bài tiếp theo (`next()`); ngược lại tăng âm lượng +10 (tối đa 100).
  - Long press: tăng âm lượng +10.

- `VOLUME_DOWN` (GPIO1)
  - Click: nếu SD music đang Playing → `pause()`; nếu Paused → `play()` (resume).
  - Nếu đang phát online music hoặc radio: click sẽ toggle “mute” bằng cách lưu volume hiện tại và đặt volume = 0, click tiếp sẽ khôi phục.
  - Nếu không thuộc các trường hợp trên: giảm âm lượng -10 (tối thiểu 0).
  - Long press: giảm âm lượng -10.

Thông tin hành vi chi tiết xem mã: [xiaozhi_ai_iot_vietnam_board_lcd_sdcard.cc](main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/xiaozhi_ai_iot_vietnam_board_lcd_sdcard.cc#L1-L200)

**Lưu ý & gợi ý**
- Một số chân (GPIO39..48) là chân đặc biệt trên vài biến thể ESP — kiểm tra sơ đồ phần cứng của module/SoC bạn đang dùng.
- Nếu dùng thẻ SD SPI đồng thời với LCD trên SPI, đảm bảo bạn bố trí CS riêng và chọn tốc độ SPI phù hợp để tránh xung nhiễu; mã hiện sử dụng SPI3 cho LCD và SPI2 cho SD để tránh xung đột.
- Nếu gặp lỗi mount SD, thử giảm tốc độ SPI CS hoặc chuyển sang SDMMC (nếu bo/hệ thống hỗ trợ khe SD trực tiếp).

**Các bước nhanh để thử**
1. Kết nối module SD như trên.
2. Bật `CONFIG_SD_CARD_SPI_INTERFACE` trong `menuconfig` hoặc trong sdkconfig.
3. Build và flash:

```bash
idf.py build
idf.py -p <PORT> flash monitor
```

4. Trên khởi động, gắn thẻ SD chứa thư mục nhạc theo định dạng dự án (thường `music` hoặc `sdcard` tùy cấu hình). Kiểm tra log để xác nhận mount thành công.

Nếu muốn, tôi có thể bổ sung sơ đồ đấu dây hình ảnh hoặc kiểm tra/ghi chú cho từng biến thể LCD cụ thể.

---

## 🚀 Hướng dẫn Build & Flash

### Khởi Động Lần Đầu

```bash
# 1. Cấu hình build (chọn board: xiaozhi-ai-iot-vietnam-lcd-sdcard)
idf.py menuconfig

# 2. Build firmware (audio assets sẽ tự động được build)
idf.py build

# 3. Flash firmware
idf.py -p COM3 flash

# 4. Monitor log
idf.py -p COM3 monitor
```

### Build Audio Assets

```bash
# Tự động build khi chạy idf.py build
# (CMakeLists.txt sẽ gọi scripts/build_audio_assets.py)

# Hoặc build thủ công
cd main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/scripts
python build_audio_assets.py
```

### Flash Audio Assets

⚠️ **CHỈ FLASH LẦN ĐẦU hoặc khi cập nhật âm thanh**

```bash
# Flash audio assets riêng
cd main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/scripts
python flash_audio_assets.py <PORT>

# Hoặc dùng esptool trực tiếp
esptool.py --port COM3 write_flash 0x800000 assets/audio_assets.bin

# Kiểm tra audio assets đã flash
python check_flash_audio.py <PORT>
```

### Quy Trình Flash Định Kỳ

**Lần đầu:**
```bash
idf.py build flash
cd scripts && python flash_audio_assets.py
```

**Những lần sau (update code):**
```bash
idf.py build flash  # Audio assets vẫn giữ nguyên!
```

**Khi update audio:**
```bash
cd scripts
python build_audio_assets.py
python flash_audio_assets.py
```

---

## 📡 WiFi & Chế độ Kết Nối

### Các chế độ hoạt động

| Trạng thái | Chatbot | Tính năng |
|-----------|---------|----------|
| **Không WiFi** | ✅ Offline | Nghe/phát audio offline, phát nhạc SD, CAN bus |
| **WiFi có nhưng chưa kết nối** | ⚠️ Transitional | Tự động kết nối nếu đã lưu WiFi |
| **WiFi kết nối thành công** | ✅ Online | Chat AI, voice recognition, OTA updates |

### Setup WiFi Lần Đầu

```
1. Nhấn nút BOOT (GPIO0) lúc khởi động
   └─> LED bắt đầu nhấp nháy (WiFi Config Mode)

2. Từ điện thoại, kết nối WiFi:
   ├─ SSID: "xiaozhi-xxxxxx"
   └─ Password: (không có)

3. Trình duyệt tự động mở: http://192.168.4.1
   ├─ Chọn WiFi muốn kết nối
   ├─ Nhập mật khẩu
   └─ Nhấn "Connect"

4. Chờ 5-10 giây
   ├─ LED xanh ✅ = Thành công → Online Mode
   └─ LED đỏ ❌ = Thất bại → Offline Mode (quay lại bước 1)
```

### Thay Đổi WiFi Mới

```
1. Nhấn nút BOOT để vào WiFi Config Mode
   └─> LED nhấp nháy

2. Từ điện thoại, kết nối "xiaozhi-xxxxxx"
   └─> Trình duyệt → http://192.168.4.1

3. Chọn WiFi mới, nhập mật khẩu

4. Chờ kết nối
   ├─ Nếu thành công → LED xanh → Next lần boot sẽ tự động kết nối
   └─ Nếu thất bại → LED đỏ → Offline Mode
```

### Reset WiFi Hoàn Toàn

Nếu cấu hình cũ bị lỗi:

```bash
# Cách 1: Dùng Boot button
1. Nhấn nút BOOT ngay khi cắm điện (first 3 seconds)
   └─> Xóa tất cả WiFi cũ

# Cách 2: Dùng esptool (Hard reset)
esptool.py --port COM3 erase_flash
idf.py flash
```

---

## ⚙️ Cấu Hình Offline Mode

Tất cả cấu hình nằm trong file `config.h`:

```c
// ============================================================================
// OFFLINE MODE Configuration
// ============================================================================

// Enable Offline Mode
#define CONFIG_ENABLE_OFFLINE_MODE

// Skip OTA check at startup (không kiểm tra cập nhật khi boot)
#define CONFIG_SKIP_OTA_CHECK_AT_STARTUP

// Offline audio source
#define CONFIG_OFFLINE_AUDIO_FROM_FLASH     // Ưu tiên: Flash (bảo đảm)
// #define CONFIG_OFFLINE_AUDIO_FROM_SD     // Fallback: SD card

// ============================================================================
// Music Button Configuration (GPIO3)
// ============================================================================

#define MUSIC_BUTTON_GPIO           GPIO_NUM_3
#define MUSIC_BUTTON_ACTIVE_LOW     true

#define MUSIC_AUTO_PLAY_ON_BOOT     false
#define MUSIC_SHUFFLE_DEFAULT       true
#define MUSIC_REPEAT_ALL_DEFAULT    true

// ============================================================================
// CAN Bus Configuration for Kia Morning 2017 Si
// ============================================================================

#define CONFIG_ENABLE_CAN_BUS           // Comment để tắt CAN bus
#define CAN_TX_GPIO         GPIO_NUM_17
#define CAN_RX_GPIO         GPIO_NUM_8
#define CAN_SPEED_KBPS      500

// ============================================================================
// Relay Control Configuration
// ============================================================================

#define CONFIG_ENABLE_RELAY_CONTROL
#define RELAY_TRUNK_GPIO        GPIO_NUM_9
#define RELAY_AC_GPIO           GPIO_NUM_47
```

### Tắt/Bật Tính Năng

**Tắt CAN Bus** (nếu không có SN65HVD230):
```c
// #define CONFIG_ENABLE_CAN_BUS  // Comment dòng này
```

**Tắt Music Button** (nếu không cần):
```c
// #define MUSIC_BUTTON_GPIO GPIO_NUM_3  // Comment dòng này
```

**Tắt Relay** (nếu không có module relay):
```c
// #define CONFIG_ENABLE_RELAY_CONTROL  // Comment dòng này
```

Sau khi thay đổi cấu hình:
```bash
idf.py build
idf.py -p COM3 flash monitor
```

---

## 🔧 Xử Sự Cố

### Vấn đề: Audio offline không phát

**Kiểm tra:**
```bash
cd scripts
python check_flash_audio.py <PORT>

# Output nên hiển thị:
# ✅ Audio assets đã được flash!
# 📊 Số file audio: 77
```

**Giải pháp:**
```bash
cd scripts
python build_audio_assets.py
python flash_audio_assets.py <PORT>
```

### Vấn đề: SD card không mount được

**Nguyên nhân**: Tốc độ SPI quá cao hoặc chân nối lỏng

**Kiểm tra:**
1. Chạy `idf.py -p COM3 monitor` và tìm log SD
2. Thử dùng thẻ SD khác
3. Kiểm tra kết nối chân (GPIO39, GPIO41, GPIO40, GPIO38)

**Giải pháp:**
```bash
# Giảm tốc độ SPI trong config.h (nếu có)
# Hoặc chuyển từ SPI sang SDMMC
idf.py menuconfig
# Tìm: Component config → SD/MMC
```

### Vấn đề: CAN Bus không nhận dữ liệu

**Kiểm tra:**
1. Kiểm tra cắm SN65HVD230
2. Kiểm tra chân GPIO17 (TX), GPIO8 (RX)
3. Kiểm tra cổng OBD-II trên xe

**Log kiểm tra:**
```bash
idf.py -p COM3 monitor

# Tìm dòng: "CAN Bus initialized" để xác nhận
```

### Vấn đề: Firmware quá lớn, không flash được

**Giải pháp**: Tắt tính năng không cần

```c
// Tắt trong config.h
// #define CONFIG_ENABLE_CAN_BUS
// #define CONFIG_ENABLE_RELAY_CONTROL
// #define MUSIC_BUTTON_GPIO GPIO_NUM_3
```

---

## 📚 Tài Liệu Liên Quan

- [WIFI_WORKFLOW_GUIDE.md](../../WIFI_WORKFLOW_GUIDE.md) - Chi tiết WiFi workflow
- [config.h](config.h) - Tất cả cấu hình GPIO & thresholds
- [xiaozhi_ai_iot_vietnam_board_lcd_sdcard.cc](xiaozhi_ai_iot_vietnam_board_lcd_sdcard.cc) - Board initialization
- [scripts/build_audio_assets.py](scripts/build_audio_assets.py) - Build audio binary
- [scripts/flash_audio_assets.py](scripts/flash_audio_assets.py) - Flash audio assets
- [canbus/](canbus/) - CAN Bus driver & Kia protocol
- [offline/](offline/) - Offline audio & music player

---

## 💡 Mẹo & Best Practices

### 1. Kiểm tra phần cứng trước khi flash

```bash
# Chạy diagnostic
idf.py -p COM3 monitor

# Bạn sẽ thấy:
# ✅ Display initialized
# ✅ SD Card mounted
# ✅ CAN Bus initialized
# ✅ Audio Codec ready
# ✅ Offline Audio loaded
```

### 2. Backup cấu hình WiFi

NVS (Non-Volatile Storage) trên ESP32 lưu WiFi. Khi reset flash:
- Cấu hình WiFi sẽ **MẤT**
- Audio assets trong Flash sẽ **LƯU GIỮ** (riêng partition)

### 3. Tối ưu hóa pin

```c
// Trong config.h, tăng timeout để tiết kiệm pin:
#define CAN_IDLE_TIMEOUT_MS         (10 * 60 * 1000)  // 10 phút thay vì 5
```

### 4. Test trên xe an toàn

- **Luôn test trên xe đỗ trước**
- **KHÔNG** can thiệp CAN Bus khi xe đang chạy
- **KHÔNG** gửi lệnh CAN không xác định

---

## 📞 Liên Hệ & Support

Nếu gặp vấn đề:
1. Kiểm tra log: `idf.py -p COM3 monitor`
2. Xem [WIFI_WORKFLOW_GUIDE.md](../../WIFI_WORKFLOW_GUIDE.md)
3. Kiểm tra cấu hình trong [config.h](config.h)
4. Report issue trên GitHub
