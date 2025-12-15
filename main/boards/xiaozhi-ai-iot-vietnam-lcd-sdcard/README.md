# Xiaozhi AI IoT Vietnam - LCD + SDCard (Board README)

Tài liệu ngắn cho bo `xiaozhi-ai-iot-vietnam-lcd-sdcard` về cách đấu dây, chức năng các nút và cách sử dụng module thẻ SD qua SPI.

**Tham khảo mã nguồn:** [main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/config.h](main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/config.h) và [main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/xiaozhi_ai_iot_vietnam_board_lcd_sdcard.cc](main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/xiaozhi_ai_iot_vietnam_board_lcd_sdcard.cc#L1-L400)

**Tổng quan**
- Bo hỗ trợ màn hình SPI (ST7789/ILI9341/GC9A01 tùy cấu hình), âm thanh I2S (simplex hoặc duplex), 3 nút vật lý và hỗ trợ thẻ SD bằng SDMMC hoặc SPI.
- README này tập trung hướng dẫn khi bạn sử dụng module thẻ SD qua giao tiếp SPI (module SD SPI kiểu breakout hoặc module microSD thứ nhất).

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
