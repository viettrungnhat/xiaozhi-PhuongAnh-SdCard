# Audio Opus - Âm thanh Offline cho Trợ lý xe Kia Morning

Thư mục chứa các file âm thanh Opus để phát cảnh báo/nhắc nhở **OFFLINE** khi không có WiFi.

## 📁 Danh sách file âm thanh cần tạo

### 🎵 Âm thanh hệ thống (System Sounds)
| File | Mô tả | Thời lượng |
|------|-------|------------|
| `beep_short.opus` | Beep ngắn - xác nhận lệnh | 0.2s |
| `beep_double.opus` | Beep đôi - hoàn thành | 0.4s |
| `beep_warning.opus` | Beep cảnh báo (cao hơn) | 0.5s |
| `beep_error.opus` | Beep lỗi (thấp hơn) | 0.5s |
| `chime_startup.opus` | Nhạc khởi động | 2s |
| `chime_shutdown.opus` | Nhạc tắt máy | 1.5s |

### 👋 Lời chào (Greetings)
| File | Nội dung | Thời lượng |
|------|----------|------------|
| `greeting_morning.opus` | "Chào bố, buổi sáng tốt lành! Chúc bố có một ngày làm việc hiệu quả." | 4s |
| `greeting_afternoon.opus` | "Chào bố, buổi chiều vui vẻ!" | 2s |
| `greeting_evening.opus` | "Chào bố, buổi tối an lành! Lái xe cẩn thận nhé." | 3s |
| `greeting_default.opus` | "Chào bố, hôm nay mình đi đâu thế ạ?" | 2.5s |
| `goodbye.opus` | "Tạm biệt bố, hẹn gặp lại!" | 2s |

### ⚠️ Cảnh báo an toàn (Safety Warnings)
| File | Nội dung | Mức độ |
|------|----------|--------|
| `warn_seatbelt.opus` | "Bố ơi, nhớ thắt dây an toàn nhé!" | Nhắc nhở |
| `warn_seatbelt_urgent.opus` | "Cảnh báo! Xe đang chạy nhưng bố chưa thắt dây an toàn!" | Khẩn cấp |
| `warn_parking_brake.opus` | "Bố ơi, nhớ hạ phanh tay trước khi đi nhé!" | Nhắc nhở |
| `warn_parking_brake_urgent.opus` | "Cảnh báo! Xe đang chạy nhưng phanh tay chưa được hạ!" | Khẩn cấp |
| `warn_door_open.opus` | "Cảnh báo! Cửa xe chưa đóng kín." | Cảnh báo |
| `warn_lights_on.opus` | "Bố ơi, đèn xe vẫn đang bật. Nhớ tắt để tiết kiệm ắc quy nhé!" | Nhắc nhở |

### 🔋 Cảnh báo ắc quy (Battery Warnings)
| File | Nội dung | Mức độ |
|------|----------|--------|
| `battery_low.opus` | "Bố ơi, điện áp ắc quy đang thấp hơn bình thường." | Cảnh báo |
| `battery_critical.opus` | "Cảnh báo khẩn cấp! Ắc quy rất yếu, bố nên kiểm tra sớm!" | Khẩn cấp |

### 🌡️ Cảnh báo nhiệt độ (Temperature Warnings)
| File | Nội dung | Mức độ |
|------|----------|--------|
| `temp_high.opus` | "Bố ơi, nhiệt độ nước làm mát đang cao hơn bình thường." | Cảnh báo |
| `temp_critical.opus` | "Cảnh báo khẩn cấp! Nhiệt độ máy quá cao! Bố nên dừng xe kiểm tra ngay!" | Khẩn cấp |
| `temp_normal.opus` | "Nhiệt độ máy đã trở lại bình thường." | Thông báo |

### ⛽ Cảnh báo nhiên liệu (Fuel Warnings)
| File | Nội dung | Mức độ |
|------|----------|--------|
| `fuel_low.opus` | "Bố ơi, xăng còn ít. Bố nên đổ xăng sớm nhé!" | Cảnh báo |
| `fuel_critical.opus` | "Cảnh báo! Xăng sắp hết, còn khoảng vài cây số nữa thôi!" | Khẩn cấp |
| `fuel_reserve.opus` | "Xe đang chạy xăng dự trữ." | Thông báo |

### 🛣️ Chế độ đường trường (Highway Mode)
| File | Nội dung | Thời lượng |
|------|----------|------------|
| `highway_mode_on.opus` | "Đã bật chế độ đường trường. Em sẽ đọc tốc độ định kỳ." | 3s |
| `highway_mode_off.opus` | "Đã tắt chế độ đường trường." | 2s |
| `speed_60.opus` | "Tốc độ 60 cây số." | 1.5s |
| `speed_70.opus` | "Tốc độ 70 cây số." | 1.5s |
| `speed_80.opus` | "Tốc độ 80 cây số." | 1.5s |
| `speed_90.opus` | "Tốc độ 90 cây số." | 1.5s |
| `speed_100.opus` | "Tốc độ 100 cây số." | 1.5s |
| `speed_over_limit.opus` | "Bố ơi, tốc độ đang hơi cao. Lái chậm lại chút nhé!" | 3s |
| `rest_reminder.opus` | "Bố ơi, bố đã lái xe hơn 2 tiếng rồi. Nên nghỉ ngơi một chút nhé!" | 4s |

### 🚗 Điều khiển xe (Vehicle Control)
| File | Nội dung | Thời lượng |
|------|----------|------------|
| `trunk_opening.opus` | "Em đang mở cốp." | 1.5s |
| `trunk_opened.opus` | "Cốp đã mở." | 1s |
| `ac_on.opus` | "Đã bật điều hòa." | 1.5s |
| `ac_off.opus` | "Đã tắt điều hòa." | 1.5s |
| `ready_to_go.opus` | "Vâng, em đã chuẩn bị sẵn sàng để bố về!" | 2.5s |

### 📊 Thông tin xe (Vehicle Info)
| File | Nội dung | Thời lượng |
|------|----------|------------|
| `info_speed_prefix.opus` | "Tốc độ hiện tại là" | 1s |
| `info_fuel_prefix.opus` | "Xăng còn khoảng" | 1s |
| `info_temp_prefix.opus` | "Nhiệt độ nước làm mát là" | 1.5s |
| `info_battery_prefix.opus` | "Điện áp ắc quy là" | 1.5s |
| `info_km.opus` | "cây số" | 0.5s |
| `info_percent.opus` | "phần trăm" | 0.5s |
| `info_degrees.opus` | "độ C" | 0.5s |
| `info_volts.opus` | "vôn" | 0.5s |

### 🔢 Số đọc (Number Audio)
| File | Nội dung |
|------|----------|
| `num_0.opus` đến `num_9.opus` | Đọc số 0-9 |
| `num_10.opus` đến `num_19.opus` | Đọc số 10-19 |
| `num_20.opus`, `num_30.opus`... `num_90.opus` | Đọc số chục |
| `num_100.opus` | "một trăm" |
| `num_thousand.opus` | "nghìn" |

### 🔧 Bảo dưỡng (Maintenance)
| File | Nội dung | Thời lượng |
|------|----------|------------|
| `maint_oil_change.opus` | "Bố ơi, xe đã đi được 5000 km. Đến lúc thay dầu rồi ạ!" | 4s |
| `maint_tire_check.opus` | "Bố ơi, xe đã đi được 10000 km. Nên kiểm tra lốp xe nhé!" | 4s |
| `maint_general.opus` | "Đến lúc bảo dưỡng định kỳ rồi bố ơi!" | 2.5s |

---

## 🎙️ Hướng dẫn tạo file âm thanh

### Cách 1: Thu âm trực tiếp
1. Dùng điện thoại hoặc micro thu âm giọng nói
2. Lưu file WAV/MP3
3. Convert sang Opus bằng ffmpeg:
   ```bash
   ffmpeg -i input.wav -c:a libopus -b:a 24k -ar 16000 output.opus
   ```

### Cách 2: Dùng Text-to-Speech (TTS)
1. Dùng Google TTS, FPT.AI hoặc Zalo TTS
2. Chọn giọng nữ miền Bắc cho tự nhiên
3. Export và convert sang Opus

### Cách 3: Dùng script Python
```python
# Cần cài đặt: pip install gtts pydub
from gtts import gTTS
from pydub import AudioSegment
import os

text = "Chào bố, hôm nay mình đi đâu thế ạ?"
tts = gTTS(text=text, lang='vi')
tts.save("temp.mp3")

# Convert to opus
os.system("ffmpeg -i temp.mp3 -c:a libopus -b:a 24k -ar 16000 greeting_default.opus")
```

---

## 📝 Thông số kỹ thuật

| Thông số | Giá trị |
|----------|---------|
| Format | Opus |
| Sample Rate | 16000 Hz |
| Bitrate | 24 kbps |
| Channels | Mono |
| Max file size | < 50KB mỗi file |

---

## 📂 Cấu trúc thư mục

```
audio_opus/
├── README.md           # File này
├── system/             # Âm thanh hệ thống
│   ├── beep_short.opus
│   ├── beep_warning.opus
│   └── ...
├── greetings/          # Lời chào
│   ├── greeting_morning.opus
│   ├── greeting_default.opus
│   └── ...
├── warnings/           # Cảnh báo
│   ├── warn_seatbelt.opus
│   ├── battery_low.opus
│   └── ...
├── highway/            # Chế độ đường trường
│   ├── speed_60.opus
│   ├── rest_reminder.opus
│   └── ...
├── control/            # Điều khiển
│   ├── trunk_opening.opus
│   ├── ac_on.opus
│   └── ...
├── info/               # Thông tin
│   ├── info_speed_prefix.opus
│   └── ...
└── numbers/            # Số đọc
    ├── num_0.opus
    ├── num_1.opus
    └── ...
```

---

## 🔊 Tổng số file cần tạo

| Nhóm | Số file |
|------|---------|
| Hệ thống | 6 |
| Lời chào | 5 |
| Cảnh báo an toàn | 6 |
| Ắc quy | 2 |
| Nhiệt độ | 3 |
| Nhiên liệu | 3 |
| Đường trường | 9 |
| Điều khiển | 5 |
| Thông tin | 8 |
| Số (0-100, chục, trăm, nghìn) | ~35 |
| Bảo dưỡng | 3 |
| **TỔNG** | **~85 file** |

---

## ⚡ Ưu tiên tạo trước

Các file quan trọng nhất cần tạo trước:

1. ✅ `greeting_default.opus` - Lời chào
2. ✅ `warn_seatbelt.opus` - Nhắc dây an toàn
3. ✅ `warn_parking_brake.opus` - Nhắc phanh tay
4. ✅ `battery_critical.opus` - Cảnh báo ắc quy
5. ✅ `temp_critical.opus` - Cảnh báo nhiệt độ
6. ✅ `fuel_low.opus` - Cảnh báo xăng
7. ✅ `trunk_opened.opus` - Mở cốp
8. ✅ `beep_short.opus` - Beep xác nhận
9. ✅ `beep_warning.opus` - Beep cảnh báo
10. ✅ `rest_reminder.opus` - Nhắc nghỉ ngơi
