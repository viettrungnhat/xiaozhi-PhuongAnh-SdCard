#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

#ifdef CONFIG_SD_CARD_MMC_INTERFACE
// Define to use 4-bit SDMMC bus width; comment out to use 1-bit bus width
#define CARD_SDMMC_BUS_WIDTH_4BIT

#ifdef CARD_SDMMC_BUS_WIDTH_4BIT
#define CARD_SDMMC_CLK_GPIO GPIO_NUM_40 // CLK pin
#define CARD_SDMMC_CMD_GPIO GPIO_NUM_39 // MISO pin
#define CARD_SDMMC_D0_GPIO GPIO_NUM_41  // MOSI pin
#define CARD_SDMMC_D1_GPIO GPIO_NUM_42
#define CARD_SDMMC_D2_GPIO GPIO_NUM_45
#define CARD_SDMMC_D3_GPIO GPIO_NUM_38  // CS pin
#else
#define CARD_SDMMC_CLK_GPIO GPIO_NUM_40
#define CARD_SDMMC_CMD_GPIO GPIO_NUM_39
#define CARD_SDMMC_D0_GPIO GPIO_NUM_41
#endif
#endif // CONFIG_SD_CARD_MMC_INTERFACE

#ifdef CONFIG_SD_CARD_SPI_INTERFACE
#define CARD_SPI_MOSI_GPIO GPIO_NUM_39
#define CARD_SPI_MISO_GPIO GPIO_NUM_41
#define CARD_SPI_SCLK_GPIO GPIO_NUM_40
#define CARD_SPI_CS_GPIO   GPIO_NUM_38
#endif // CONFIG_SD_CARD_SPI_INTERFACE

#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// If using Duplex I2S mode, please comment out the following line
#define AUDIO_I2S_METHOD_SIMPLEX

#ifdef AUDIO_I2S_METHOD_SIMPLEX

#define AUDIO_I2S_MIC_GPIO_WS   GPIO_NUM_4
#define AUDIO_I2S_MIC_GPIO_SCK  GPIO_NUM_5
#define AUDIO_I2S_MIC_GPIO_DIN  GPIO_NUM_6
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_7
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_15
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_16

#else

#define AUDIO_I2S_GPIO_WS GPIO_NUM_4
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_5
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_6
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_7

#endif

#define BUILTIN_LED_GPIO        GPIO_NUM_48
#define BOOT_BUTTON_GPIO        GPIO_NUM_0
#define TOUCH_BUTTON_GPIO       GPIO_NUM_NC
#define VOLUME_UP_BUTTON_GPIO   GPIO_NUM_2
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_1


#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_10
#define DISPLAY_MOSI_PIN      GPIO_NUM_12
#define DISPLAY_CLK_PIN       GPIO_NUM_11
#define DISPLAY_DC_PIN        GPIO_NUM_13
#define DISPLAY_RST_PIN       GPIO_NUM_14
#define DISPLAY_CS_PIN        GPIO_NUM_21


#ifdef CONFIG_LCD_ST7789_240X320
#define LCD_TYPE_ST7789_SERIAL
#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  320
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_INVERT_COLOR    true
#define DISPLAY_RGB_ORDER  LCD_RGB_ELEMENT_ORDER_RGB
#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE 0
#endif

#ifdef CONFIG_LCD_ST7789_240X320_NO_IPS
#define LCD_TYPE_ST7789_SERIAL
#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  320
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_INVERT_COLOR    false
#define DISPLAY_RGB_ORDER  LCD_RGB_ELEMENT_ORDER_RGB
#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE 0
#endif

#ifdef CONFIG_LCD_ST7789_170X320
#define LCD_TYPE_ST7789_SERIAL
#define DISPLAY_WIDTH   170
#define DISPLAY_HEIGHT  320
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_INVERT_COLOR    true
#define DISPLAY_RGB_ORDER  LCD_RGB_ELEMENT_ORDER_RGB
#define DISPLAY_OFFSET_X  35
#define DISPLAY_OFFSET_Y  0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE 0
#endif

#ifdef CONFIG_LCD_ST7789_172X320
#define LCD_TYPE_ST7789_SERIAL
#define DISPLAY_WIDTH   172
#define DISPLAY_HEIGHT  320
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_INVERT_COLOR    true
#define DISPLAY_RGB_ORDER  LCD_RGB_ELEMENT_ORDER_RGB
#define DISPLAY_OFFSET_X  34
#define DISPLAY_OFFSET_Y  0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE 0
#endif

#ifdef CONFIG_LCD_ST7789_240X280
#define LCD_TYPE_ST7789_SERIAL
#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  280
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_INVERT_COLOR    true
#define DISPLAY_RGB_ORDER  LCD_RGB_ELEMENT_ORDER_RGB
#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  20
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE 0
#endif

#ifdef CONFIG_LCD_ST7789_240X240
#define LCD_TYPE_ST7789_SERIAL
#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  240
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_INVERT_COLOR    true
#define DISPLAY_RGB_ORDER  LCD_RGB_ELEMENT_ORDER_RGB
#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE 0
#endif

#ifdef CONFIG_LCD_ST7789_240X240_7PIN
#define LCD_TYPE_ST7789_SERIAL
#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  240
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_INVERT_COLOR    true
#define DISPLAY_RGB_ORDER  LCD_RGB_ELEMENT_ORDER_RGB
#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE 3
#endif

#ifdef CONFIG_LCD_ST7789_240X135
#define LCD_TYPE_ST7789_SERIAL
#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  135
#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY true
#define DISPLAY_INVERT_COLOR    true
#define DISPLAY_RGB_ORDER  LCD_RGB_ELEMENT_ORDER_RGB
#define DISPLAY_OFFSET_X  40
#define DISPLAY_OFFSET_Y  53
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE 0
#endif

#ifdef CONFIG_LCD_ST7735_128X160
#define LCD_TYPE_ST7789_SERIAL
#define DISPLAY_WIDTH   128
#define DISPLAY_HEIGHT  160
#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y true
#define DISPLAY_SWAP_XY false
#define DISPLAY_INVERT_COLOR    false
#define DISPLAY_RGB_ORDER  LCD_RGB_ELEMENT_ORDER_RGB
#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE 0
#endif



#ifdef CONFIG_LCD_ST7735_128X128
#define LCD_TYPE_ST7789_SERIAL
#define DISPLAY_WIDTH   128
#define DISPLAY_HEIGHT  128
#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y true
#define DISPLAY_SWAP_XY false
#define DISPLAY_INVERT_COLOR  false
#define DISPLAY_RGB_ORDER  LCD_RGB_ELEMENT_ORDER_BGR
#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  32
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE 0
#endif

#ifdef CONFIG_LCD_ST7796_320X480
#define LCD_TYPE_ST7789_SERIAL
#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  480
#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_INVERT_COLOR    true
#define DISPLAY_RGB_ORDER  LCD_RGB_ELEMENT_ORDER_BGR
#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE 0
#endif

#ifdef CONFIG_LCD_ST7796_320X480_NO_IPS
#define LCD_TYPE_ST7789_SERIAL
#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  480
#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_INVERT_COLOR    false
#define DISPLAY_RGB_ORDER  LCD_RGB_ELEMENT_ORDER_BGR
#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE 0
#endif

#ifdef CONFIG_LCD_ILI9341_240X320
#define LCD_TYPE_ILI9341_SERIAL
#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  320
#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_INVERT_COLOR    true
#define DISPLAY_RGB_ORDER  LCD_RGB_ELEMENT_ORDER_BGR
#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE 0
#endif

#ifdef CONFIG_LCD_ILI9341_240X320_NO_IPS
#define LCD_TYPE_ILI9341_SERIAL
#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  320
#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_INVERT_COLOR    false
#define DISPLAY_RGB_ORDER  LCD_RGB_ELEMENT_ORDER_BGR
#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE 0
#endif

#ifdef CONFIG_LCD_GC9A01_240X240
#define LCD_TYPE_GC9A01_SERIAL
#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  240
#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_INVERT_COLOR    true
#define DISPLAY_RGB_ORDER  LCD_RGB_ELEMENT_ORDER_BGR
#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE 0
#endif

#ifdef CONFIG_LCD_CUSTOM
#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  320
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_INVERT_COLOR    true
#define DISPLAY_RGB_ORDER  LCD_RGB_ELEMENT_ORDER_RGB
#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE 0
#endif


// A MCP Test: Control a lamp
#define LAMP_GPIO GPIO_NUM_18

// ============================================================================
// OFFLINE MODE Configuration
// Cho phép chatbot hoạt động cơ bản khi không có WiFi
// ============================================================================

// Enable Offline Mode - Bỏ qua kiểm tra cập nhật khi không có WiFi
// #define CONFIG_ENABLE_OFFLINE_MODE  // Temporarily disabled to reduce firmware size

// Skip OTA check at startup - Không kiểm tra cập nhật khi khởi động
#define CONFIG_SKIP_OTA_CHECK_AT_STARTUP

// Offline audio source - Chọn nguồn âm thanh offline
// #define CONFIG_OFFLINE_AUDIO_FROM_FLASH     // Ưu tiên: Đọc từ Flash assets partition (disabled - too large)
#define CONFIG_OFFLINE_AUDIO_FROM_SD     // Phụ: Đọc từ SD card (fallback)

// Audio paths (chỉ dùng khi CONFIG_OFFLINE_AUDIO_FROM_SD)
#define OFFLINE_AUDIO_PATH          "/audio_opus"
#define OFFLINE_MUSIC_PATH          "/music"

// ============================================================================
// Music Button Configuration
// Nút nhấn để phát/dừng nhạc từ thẻ SD
// ============================================================================

// GPIO cho nút phát nhạc (sử dụng GPIO3)
// GPIO3 là strapping pin nhưng an toàn để dùng làm input sau khi boot
// #define MUSIC_BUTTON_GPIO           GPIO_NUM_3  // Temporarily disabled to reduce size
#define MUSIC_BUTTON_ACTIVE_LOW     true    // Nút nhấn nối GND

// Chế độ phát nhạc
#define MUSIC_AUTO_PLAY_ON_BOOT     false   // Tự động phát nhạc khi khởi động
#define MUSIC_SHUFFLE_DEFAULT       true    // Mặc định bật shuffle
#define MUSIC_REPEAT_ALL_DEFAULT    true    // Mặc định lặp lại toàn bộ

// Double-click detection (ms)
#define MUSIC_BUTTON_DEBOUNCE_MS    50
#define MUSIC_BUTTON_DOUBLE_CLICK_MS 300
#define MUSIC_BUTTON_LONG_PRESS_MS  1000

// ============================================================================
// CAN Bus Configuration for Kia Morning 2017 Si
// Module: SN65HVD230 CAN Transceiver
// ============================================================================

// Enable/Disable CAN Bus feature
// Comment out this line to disable CAN bus if SN65HVD230 is not connected
#define CONFIG_ENABLE_CAN_BUS

// CAN Bus GPIO Pins (SN65HVD230 connection)
// SN65HVD230 Pin  -> ESP32-S3 Pin
// CTX (TX)        -> GPIO17
// CRX (RX)        -> GPIO8
// VCC             -> 3.3V
// GND             -> GND
// CANH            -> Vehicle OBD-II Pin 6
// CANL            -> Vehicle OBD-II Pin 14
#define CAN_TX_GPIO         GPIO_NUM_17
#define CAN_RX_GPIO         GPIO_NUM_8

// CAN Bus Speed (Kia Morning 2017 uses 500kbps for most modules)
#define CAN_SPEED_KBPS      500

// CAN Bus Power Saving Configuration
#define CAN_IDLE_TIMEOUT_MS         (5 * 60 * 1000)  // 5 minutes before entering power save mode
#define CAN_POWER_SAVE_CHECK_MS     (1000)           // Check for idle every 1 second

// CAN Bus Task Configuration
#define CAN_TASK_STACK_SIZE         4096
#define CAN_TASK_PRIORITY           5   // Medium priority - below audio (7), above display (3)
#define CAN_TASK_CORE               1   // Run on Core 1 to not interfere with WiFi/BT on Core 0

// CAN RX Queue Size
#define CAN_RX_QUEUE_SIZE           20

// Vehicle Alert Thresholds for Kia Morning 2017
#define VEHICLE_BATTERY_LOW_VOLTAGE     11.8f   // Volts - warn when battery is low
#define VEHICLE_BATTERY_CRITICAL_VOLTAGE 11.0f  // Volts - critical warning
#define VEHICLE_COOLANT_WARN_TEMP       100.0f  // Celsius - engine overheating warning
#define VEHICLE_COOLANT_CRITICAL_TEMP   105.0f  // Celsius - critical overheating

// Speed thresholds
#define VEHICLE_SPEED_HIGHWAY           80      // km/h - highway driving mode
#define VEHICLE_MAX_DRIVE_TIME_MINUTES  120     // 2 hours - recommend break

// Maintenance reminders (based on odometer)
#define MAINTENANCE_OIL_CHANGE_KM       5000    // Oil change every 5000km
#define MAINTENANCE_TIRE_CHECK_KM       10000   // Tire check every 10000km
#define MAINTENANCE_MAJOR_SERVICE_KM    30000   // Major service every 30000km

// ============================================================================
// Relay GPIO for Vehicle Control (Kia Morning 2017)
// ============================================================================
// Sử dụng relay module 5V hoặc 3.3V (active LOW hoặc HIGH tùy module)
// Kết nối: GPIO -> IN của relay, relay COM/NO -> thiết bị xe
//
// GPIO còn trống trên ESP32-S3 cho relay:
// - GPIO3:  Có thể dùng (strapping pin, cần cẩn thận)
// - GPIO9:  Có thể dùng  
// - GPIO46: Có thể dùng (input only trên một số module)
// - GPIO47: Có thể dùng
// ============================================================================

// Enable/Disable Relay Control
// #define CONFIG_ENABLE_RELAY_CONTROL  // Temporarily disabled to reduce firmware size

#ifdef CONFIG_ENABLE_RELAY_CONTROL

// Trunk (Cốp xe) Relay - Điều khiển mở cốp điện
// Kết nối: GPIO9 -> Relay IN -> Relay COM/NO -> Xilanh điện cốp
#define RELAY_TRUNK_GPIO            GPIO_NUM_9
#define RELAY_TRUNK_ACTIVE_LEVEL    0           // 0 = Active LOW, 1 = Active HIGH
#define RELAY_TRUNK_PULSE_MS        500         // Thời gian kích relay (ms)

// AC (Điều hòa) Relay - Điều khiển bật/tắt điều hòa (tùy chọn)
// Kết nối: GPIO47 -> Relay IN -> Song song với nút AC trên xe
#define RELAY_AC_GPIO               GPIO_NUM_47
#define RELAY_AC_ACTIVE_LEVEL       0           // 0 = Active LOW, 1 = Active HIGH

// Horn (Còi) Relay - Điều khiển còi xe (tùy chọn, cẩn thận khi dùng)
// #define RELAY_HORN_GPIO          GPIO_NUM_NC
// #define RELAY_HORN_ACTIVE_LEVEL  0

// Hazard Lights Relay - Đèn cảnh báo nguy hiểm (tùy chọn)
// #define RELAY_HAZARD_GPIO        GPIO_NUM_NC
// #define RELAY_HAZARD_ACTIVE_LEVEL 0

#endif // CONFIG_ENABLE_RELAY_CONTROL

// ============================================================================
// Sơ đồ đấu nối Relay Module cho Cốp điện
// ============================================================================
//
//  ESP32-S3                 Relay Module              Xilanh điện cốp
//  ─────────                ────────────              ────────────────
//  GPIO9    ──────────────► IN
//  3.3V     ──────────────► VCC (hoặc 5V tùy module)
//  GND      ──────────────► GND
//                           COM ◄───────────────────► Dây + xilanh
//                           NO  ◄───────────────────► +12V từ ắc quy xe
//                           NC  (không dùng)
//
//  Lưu ý: 
//  - Dùng relay 12V/10A cho xilanh điện
//  - Thêm diode flyback (1N4007) song song với xilanh
//  - Có thể dùng relay module có opto-isolator để an toàn hơn
// ============================================================================

#endif // _BOARD_CONFIG_H_
