#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "sdmmc.h"
#include "sdspi.h"
#include "system_reset.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"
#include "lamp_controller.h"
#include "settings.h"
#include "led/single_led.h"
#include "assets/lang_config.h"
#include "music/esp32_sd_music.h"
#include "music/esp32_radio.h"
#include "music/esp32_music.h"

// CAN Bus integration for Kia Morning 2017 Si
#include "canbus/canbus_driver.h"
#include "canbus/kia_can_protocol.h"
#include "canbus/vehicle_assistant.h"
#include "canbus/relay_controller.h"

// SD Card MP3 Player for 77 Vietnamese alerts
#include "offline/sd_audio_player.h"

// Music button for SD card control
#include "offline/music_button.h"

#include <wifi_station.h>
#include <esp_log.h>
#include <driver/i2c_master.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <driver/spi_common.h>

#if defined(LCD_TYPE_ILI9341_SERIAL)
#include "esp_lcd_ili9341.h"
#endif

#if defined(LCD_TYPE_GC9A01_SERIAL)
#include "esp_lcd_gc9a01.h"
static const gc9a01_lcd_init_cmd_t gc9107_lcd_init_cmds[] = {
    //  {cmd, { data }, data_size, delay_ms}
    {0xfe, (uint8_t[]){0x00}, 0, 0},
    {0xef, (uint8_t[]){0x00}, 0, 0},
    {0xb0, (uint8_t[]){0xc0}, 1, 0},
    {0xb1, (uint8_t[]){0x80}, 1, 0},
    {0xb2, (uint8_t[]){0x27}, 1, 0},
    {0xb3, (uint8_t[]){0x13}, 1, 0},
    {0xb6, (uint8_t[]){0x19}, 1, 0},
    {0xb7, (uint8_t[]){0x05}, 1, 0},
    {0xac, (uint8_t[]){0xc8}, 1, 0},
    {0xab, (uint8_t[]){0x0f}, 1, 0},
    {0x3a, (uint8_t[]){0x05}, 1, 0},
    {0xb4, (uint8_t[]){0x04}, 1, 0},
    {0xa8, (uint8_t[]){0x08}, 1, 0},
    {0xb8, (uint8_t[]){0x08}, 1, 0},
    {0xea, (uint8_t[]){0x02}, 1, 0},
    {0xe8, (uint8_t[]){0x2A}, 1, 0},
    {0xe9, (uint8_t[]){0x47}, 1, 0},
    {0xe7, (uint8_t[]){0x5f}, 1, 0},
    {0xc6, (uint8_t[]){0x21}, 1, 0},
    {0xc7, (uint8_t[]){0x15}, 1, 0},
    {0xf0,
    (uint8_t[]){0x1D, 0x38, 0x09, 0x4D, 0x92, 0x2F, 0x35, 0x52, 0x1E, 0x0C,
                0x04, 0x12, 0x14, 0x1f},
    14, 0},
    {0xf1,
    (uint8_t[]){0x16, 0x40, 0x1C, 0x54, 0xA9, 0x2D, 0x2E, 0x56, 0x10, 0x0D,
                0x0C, 0x1A, 0x14, 0x1E},
    14, 0},
    {0xf4, (uint8_t[]){0x00, 0x00, 0xFF}, 3, 0},
    {0xba, (uint8_t[]){0xFF, 0xFF}, 2, 0},
};
#endif
 
#define TAG "XiaozhiAiIotVietnamBoardLcdSdcard"

// Static variable to store SD card mount status
static bool sd_card_mounted = false;

class XiaozhiAiIotVietnamBoardLcdSdcard : public WifiBoard {
private:
 
    Button boot_button_;
    Button volume_up_button_;
    Button volume_down_button_;
    LcdDisplay* display_;
    bool offline_mode_ = false;  // Chế độ offline - không cần WiFi

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_MOSI_PIN;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_CLK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    void InitializeLcdDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;
        // 液晶屏控制IO初始化
        ESP_LOGD(TAG, "Install panel IO");
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = DISPLAY_SPI_MODE;
        io_config.pclk_hz = 30 * 1000 * 1000;  // 30MHz for crisp display (SD card on separate SPI2_HOST, no conflict)
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI3_HOST, &io_config, &panel_io));

        // 初始化液晶屏驱动芯片
        ESP_LOGD(TAG, "Install LCD driver");
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RST_PIN;
        panel_config.rgb_ele_order = DISPLAY_RGB_ORDER;
        panel_config.bits_per_pixel = 16;
#if defined(LCD_TYPE_ILI9341_SERIAL)
        ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(panel_io, &panel_config, &panel));
#elif defined(LCD_TYPE_GC9A01_SERIAL)
        ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(panel_io, &panel_config, &panel));
        gc9a01_vendor_config_t gc9107_vendor_config = {
            .init_cmds = gc9107_lcd_init_cmds,
            .init_cmds_size = sizeof(gc9107_lcd_init_cmds) / sizeof(gc9a01_lcd_init_cmd_t),
        };        
#else
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));
#endif
        
        esp_lcd_panel_reset(panel);

        esp_lcd_panel_init(panel);
        esp_lcd_panel_invert_color(panel, DISPLAY_INVERT_COLOR);
        esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
#ifdef  LCD_TYPE_GC9A01_SERIAL
        panel_config.vendor_config = &gc9107_vendor_config;
#endif
        display_ = new SpiLcdDisplay(panel_io, panel,
                                    DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                // Đang ở màn hình cấu hình WiFi - nhấn để vào chế độ OFFLINE
                ESP_LOGW(TAG, "Boot button pressed during WiFi config - switching to OFFLINE mode");
                Settings offline_settings("offline", true);
                offline_settings.SetInt("enabled", 1);
                GetDisplay()->ShowNotification("📴 Bật OFFLINE mode\nKhởi động lại...");
                vTaskDelay(pdMS_TO_TICKS(2000));
                esp_restart();
                return;
            }
            app.ToggleChatState();
        });

        // Boot button long press (>2s): Toggle offline/online mode
        boot_button_.OnLongPress([this]() {
            auto& app = Application::GetInstance();
            auto sd_music = app.GetSdMusic();
            auto radio = app.GetRadio();
            
            // Nếu đang phát nhạc, dừng trước
            if (sd_music && sd_music->getState() == Esp32SdMusic::PlayerState::Playing) {
                sd_music->stop();
                GetDisplay()->ShowNotification("Đã dừng nhạc SD");
                return;
            }
            if (radio && radio->IsPlaying()) {
                radio->Stop();
                GetDisplay()->ShowNotification("Đã dừng radio");
                return;
            }
            
            // Không có nhạc đang phát - toggle offline/online mode
            {
                Settings offline_settings("offline", false);
                int current_offline = offline_settings.GetInt("enabled", 0);
                
                if (current_offline == 1) {
                    // Đang offline → chuyển sang online
                    ESP_LOGI(TAG, "🔌 Boot long press: Switching to ONLINE mode");
                    {
                        Settings write_settings("offline", true);
                        write_settings.SetInt("enabled", 0);
                    }  // Destructor → nvs_commit()
                    GetDisplay()->ShowNotification("📶 Chế độ ONLINE\nKhởi động lại...");
                } else {
                    // Đang online → chuyển sang offline
                    ESP_LOGW(TAG, "📴 Boot long press: Switching to OFFLINE mode");
                    {
                        Settings write_settings("offline", true);
                        write_settings.SetInt("enabled", 1);
                    }  // Destructor → nvs_commit()
                    GetDisplay()->ShowNotification("📴 Chế độ OFFLINE\nKhởi động lại...");
                }
            }
            
            vTaskDelay(pdMS_TO_TICKS(2000));
            esp_restart();
        });

        volume_up_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            auto sd_music = app.GetSdMusic();
            
            // If SD music is playing/paused, next track
            if (sd_music && (sd_music->getState() == Esp32SdMusic::PlayerState::Playing ||
                             sd_music->getState() == Esp32SdMusic::PlayerState::Paused)) {
                sd_music->next();
                GetDisplay()->ShowNotification("Bài tiếp theo ⏭");
                return;
            }
            
            // Otherwise, adjust volume
            auto codec = GetAudioCodec();
            auto volume = codec->output_volume() + 10;
            if (volume > 100) {
                volume = 100;
            }
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification(Lang::Strings::VOLUME + std::to_string(volume));
        });

        // Volume Up Long Press: Increase volume
        volume_up_button_.OnLongPress([this]() {
            auto codec = GetAudioCodec();
            auto volume = codec->output_volume() + 10;
            if (volume > 100) {
                volume = 100;
            }
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification(Lang::Strings::VOLUME + std::to_string(volume));
        });

        volume_down_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            auto sd_music = app.GetSdMusic();
            auto music = app.GetMusic();
            auto radio = app.GetRadio();
            
            // If SD music is playing/paused, toggle pause
            if (sd_music) {
                auto state = sd_music->getState();
                if (state == Esp32SdMusic::PlayerState::Playing) {
                    sd_music->pause();
                    GetDisplay()->ShowNotification("Tạm dừng ⏸");
                    return;
                } else if (state == Esp32SdMusic::PlayerState::Paused) {
                    sd_music->play();  // play() resumes if paused
                    GetDisplay()->ShowNotification("Tiếp tục ▶");
                    return;
                }
            }
            
            // If online music or radio is playing, toggle mute (simulate pause)
            static int saved_volume = -1;  // Save volume before muting
            auto codec = GetAudioCodec();
            
            if ((music && music->IsPlaying()) || (radio && radio->IsPlaying())) {
                if (saved_volume < 0) {
                    // Currently playing -> Mute (simulate pause)
                    saved_volume = codec->output_volume();
                    codec->SetOutputVolume(0);
                    GetDisplay()->ShowNotification("Tạm dừng ⏸ (mute)");
                } else {
                    // Currently muted -> Restore volume (simulate resume)
                    codec->SetOutputVolume(saved_volume);
                    GetDisplay()->ShowNotification("Tiếp tục ▶");
                    saved_volume = -1;
                }
                return;
            }
            
            // Otherwise, adjust volume
            auto volume = codec->output_volume() - 10;
            if (volume < 0) {
                volume = 0;
            }
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification(Lang::Strings::VOLUME + std::to_string(volume));
        });

        // Volume Down Long Press: Decrease volume
        volume_down_button_.OnLongPress([this]() {
            auto codec = GetAudioCodec();
            auto volume = codec->output_volume() - 10;
            if (volume < 0) {
                volume = 0;
            }
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification(Lang::Strings::VOLUME + std::to_string(volume));
        });
    }

    // 物联网初始化，添加对 AI 可见设备
    void InitializeTools() {
        static LampController lamp(LAMP_GPIO);
        
        auto& mcp_server = McpServer::GetInstance();
        
        // Tool 1: Chế độ Offline - thiết bị hoạt động không cần internet
        // Các cách gọi: "bật offline", "chế độ offline", "tắt wifi", "ngắt mạng"
        mcp_server.AddTool("self.system.offline_mode",
            "Chuyển sang chế độ OFFLINE (không cần wifi/internet). Khi người dùng nói 'bật offline', 'chế độ offline', 'tắt wifi', 'ngắt kết nối mạng', hoặc 'không cần internet' thì gọi tool này. Thiết bị sẽ restart và hoạt động offline với CAN bus, nhạc SD, điều khiển local.",
            PropertyList(), [this](const PropertyList& properties) {
                ESP_LOGW(TAG, "Enabling OFFLINE MODE by user request");
                
                // Lưu flag offline mode vào NVS - dùng scoped block để đảm bảo nvs_commit chạy
                {
                    Settings offline_settings("offline", true);
                    offline_settings.SetInt("enabled", 1);
                    ESP_LOGI(TAG, "✅ Đã set offline flag = 1, waiting for destructor to commit...");
                }  // Destructor chạy ở đây → nvs_commit()
                ESP_LOGI(TAG, "✅ NVS committed, preparing restart...");
                
                GetDisplay()->ShowNotification("📴 Chế độ OFFLINE\nKhởi động lại...");
                vTaskDelay(pdMS_TO_TICKS(2000));
                esp_restart();
                return true;
            });
        
        // Tool 2: Bật lại WiFi và kết nối internet
        mcp_server.AddTool("self.system.online_mode",
            "Bật chế độ online. Thiết bị sẽ kết nối WiFi và sử dụng cloud AI",
            PropertyList(), [this](const PropertyList& properties) {
                ESP_LOGI(TAG, "Enabling ONLINE MODE by user request");
                
                // Xóa flag offline mode - dùng scoped block để đảm bảo nvs_commit chạy
                {
                    Settings offline_settings("offline", true);
                    offline_settings.SetInt("enabled", 0);
                    ESP_LOGI(TAG, "✅ Đã set offline flag = 0, waiting for destructor to commit...");
                }  // Destructor chạy ở đây → nvs_commit()
                ESP_LOGI(TAG, "✅ NVS committed");
                
                // Kiểm tra có WiFi cũ không
                Settings wifi_settings("wifi", false);
                std::string ssid = wifi_settings.GetString("ssid");
                
                if (ssid.empty()) {
                    // Chưa có WiFi → vào chế độ cấu hình
                    GetDisplay()->ShowNotification("📶 Cấu hình WiFi...");
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    ResetWifiConfiguration();
                } else {
                    // Có WiFi cũ → restart và tự kết nối
                    GetDisplay()->ShowNotification("📶 Kết nối WiFi: " + ssid);
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    esp_restart();
                }
                return true;
            });
        
        // Tool 3: Xóa WiFi và cấu hình lại từ đầu
        mcp_server.AddTool("self.system.reset_wifi",
            "Xóa WiFi cũ và cấu hình WiFi mới. Hệ thống sẽ tạo hotspot để bạn kết nối và nhập thông tin WiFi",
            PropertyList(), [this](const PropertyList& properties) {
                ESP_LOGW(TAG, "Resetting WiFi configuration by user request");
                GetDisplay()->ShowNotification("🔄 Cấu hình WiFi mới...");
                vTaskDelay(pdMS_TO_TICKS(1000));
                ResetWifiConfiguration();
                return true;
            });
        
        // Tool 4: Test phát âm thanh từ thư mục notifications (cho CAN alerts)
        // NOTE: Disabled - 77 notification files already on SD card
        // When connected to Kia Morning 2017 via OBD-II CAN bus, notifications will play AUTOMATICALLY
        // No MCP tool needed - just connect the vehicle!
        // TODO: Fix file name matching with FatFS 8.3 format if needed later
        /*
        PropertyList notification_props;
        notification_props.AddProperty(Property("alert_type", kPropertyTypeString, "battery_low"));
        mcp_server.AddTool("self.audio.test_notification", "Test notifications", notification_props, 
            [this](const PropertyList& properties) -> bool { return false; });
        */
    }

    // ========================================================================
    // Relay Initialization for Vehicle Control (Trunk, AC, etc.)
    // ========================================================================
    void InitializeRelays() {
#ifdef CONFIG_ENABLE_RELAY_CONTROL
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "Initializing Vehicle Relay Control");
        ESP_LOGI(TAG, "Trunk Relay: GPIO%d", RELAY_TRUNK_GPIO);
#ifdef RELAY_AC_GPIO
        ESP_LOGI(TAG, "AC Relay: GPIO%d", RELAY_AC_GPIO);
#endif
        ESP_LOGI(TAG, "========================================");
        
        // Khởi tạo VehicleRelayManager (singleton)
        relay::VehicleRelayManager::GetInstance();
        
        ESP_LOGI(TAG, "Vehicle Relay Control initialized successfully!");
#else
        ESP_LOGI(TAG, "Vehicle Relay Control DISABLED");
#endif
    }

    // ========================================================================
    // CAN Bus Initialization for Kia Morning 2017 Si
    // Using SN65HVD230 module connected to GPIO17 (TX) and GPIO8 (RX)
    // ========================================================================
    // MADE PUBLIC for delayed initialization after SD card mount
    void InitializeCanBus() {
#ifdef CONFIG_ENABLE_CAN_BUS
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "Initializing CAN Bus for Kia Morning 2017");
        ESP_LOGI(TAG, "TX: GPIO%d, RX: GPIO%d, Speed: %d kbps", 
                 CAN_TX_GPIO, CAN_RX_GPIO, CAN_SPEED_KBPS);
        ESP_LOGI(TAG, "========================================");
        
        // Initialize CAN bus driver
        canbus::CanBusDriver& can_driver = canbus::CanBusDriver::GetInstance();
        if (!can_driver.Initialize(CAN_TX_GPIO, CAN_RX_GPIO, CAN_SPEED_KBPS)) {
            ESP_LOGE(TAG, "Failed to initialize CAN bus driver!");
            ESP_LOGE(TAG, "Check SN65HVD230 wiring: CTX->GPIO%d, CRX->GPIO%d", 
                     CAN_TX_GPIO, CAN_RX_GPIO);
            return;
        }
        
        // Initialize vehicle assistant
        vehicle::VehicleAssistant& assistant = vehicle::VehicleAssistant::GetInstance();
        if (!assistant.Initialize()) {
            ESP_LOGE(TAG, "Failed to initialize Vehicle Assistant!");
            return;
        }
        
        // Set up callbacks for TTS and display
        // Hoạt động cả khi ONLINE và OFFLINE
        assistant.SetSpeakCallback([](const std::string& message) {
            ESP_LOGI(TAG, "🔊 Vehicle says: %s", message.c_str());
            auto& app = Application::GetInstance();
            
            // Luôn phát beep thông báo trước (hoạt động cả offline)
            app.PlaySound(Lang::Sounds::OGG_POPUP);
            
            // TODO: Nếu có file opus tương ứng trong assets, phát file đó
            // Ví dụ: message chứa "cảnh báo" → phát warning.opus
            //        message chứa "chào" → phát greeting.opus
        });
        
        assistant.SetDisplayCallback([this](const std::string& text, int line) {
            // Display vehicle info on LCD (could use notification or dedicated area)
            ESP_LOGD(TAG, "Display L%d: %s", line, text.c_str());
        });
        
        // ⏳ DELAY CAN START - Schedule to start ~15 seconds after boot
        // This gives the system time to stabilize and SD card to mount
        xTaskCreate([](void* param) {
            auto* can_driver_ptr = static_cast<canbus::CanBusDriver*>(param);
            ESP_LOGI(TAG, "⏳ Waiting 15 seconds before starting CAN bus scanning...");
            vTaskDelay(pdMS_TO_TICKS(15000));  // Wait ~15 seconds
            
            // Start CAN bus driver (begin receiving messages)
            if (!can_driver_ptr->Start()) {
                ESP_LOGE(TAG, "Failed to start CAN bus driver!");
                vTaskDelete(nullptr);
                return;
            }
            
            ESP_LOGI(TAG, "✅ CAN Bus started! Now listening for vehicle messages...");
            vTaskDelete(nullptr);
        }, "can_start_delay", 4096, &can_driver, 3, nullptr);
        
        // Start vehicle assistant (keep services ready)
        if (!assistant.Start()) {
            ESP_LOGE(TAG, "Failed to start Vehicle Assistant!");
            return;
        }
        
        ESP_LOGI(TAG, "Vehicle Assistant initialized (CAN scanning will start in ~15s)");
        
        // Start vehicle data display task
        StartVehicleDataDisplayTask();

        // ========================================================================
        // WiFi Disconnect Monitor - Tự động chuyển offline khi mất WiFi
        // ========================================================================
        xTaskCreate([](void* param) {
            auto* board = static_cast<XiaozhiAiIotVietnamBoardLcdSdcard*>(param);
            
            // Wait for initial WiFi connection attempt
            vTaskDelay(pdMS_TO_TICKS(30000));
            
            bool was_connected = WifiStation::GetInstance().IsConnected();
            int disconnect_count = 0;
            
            while (true) {
                bool is_connected = WifiStation::GetInstance().IsConnected();
                
                if (was_connected && !is_connected) {
                    // Vừa mất kết nối WiFi
                    disconnect_count++;
                    ESP_LOGW("WIFI_MONITOR", "📴 WiFi disconnected (count=%d)", disconnect_count);
                    
                    // Nếu mất kết nối liên tục 3 lần (6 giây), thông báo
                    if (disconnect_count >= 3) {
                        board->GetDisplay()->ShowNotification("📴 Mất kết nối WiFi\nChế độ offline tự động");
                        
                        // Phát beep cảnh báo
                        Application::GetInstance().PlaySound(Lang::Sounds::OGG_EXCLAMATION);
                        
                        // Set flag offline (không restart, tiếp tục hoạt động)
                        board->offline_mode_ = true;
                        ESP_LOGW("WIFI_MONITOR", "🔄 Auto-switched to OFFLINE mode");
                        
                        disconnect_count = 0;
                    }
                } else if (!was_connected && is_connected) {
                    // Vừa kết nối lại WiFi
                    disconnect_count = 0;
                    board->offline_mode_ = false;
                    ESP_LOGI("WIFI_MONITOR", "📶 WiFi reconnected - back to ONLINE mode");
                    board->GetDisplay()->ShowNotification("📶 Đã kết nối lại WiFi");
                } else if (is_connected) {
                    disconnect_count = 0;  // Reset counter khi đang connected
                }
                
                was_connected = is_connected;
                vTaskDelay(pdMS_TO_TICKS(2000));  // Check every 2 seconds
            }
        }, "wifi_monitor", 3072, this, 3, nullptr);
#else
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "CAN Bus DISABLED (SN65HVD230 not connected)");
        ESP_LOGI(TAG, "To enable: uncomment CONFIG_ENABLE_CAN_BUS in config.h");
        ESP_LOGI(TAG, "========================================");
#endif  // CONFIG_ENABLE_CAN_BUS
    }

    // ========================================================================
    // Music Button Initialization - Nút nhấn phát nhạc SD (GPIO3)
    // ========================================================================
    void InitializeMusicButton() {
#ifdef MUSIC_BUTTON_GPIO
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "Initializing Music Button on GPIO%d", MUSIC_BUTTON_GPIO);
        ESP_LOGI(TAG, "========================================");
        
        auto& music_btn = music::MusicButtonController::GetInstance();
        if (!music_btn.Initialize()) {
            ESP_LOGW(TAG, "Failed to initialize music button");
            return;
        }
        
        // Set callbacks for music control
        music_btn.SetOnPlayPause([this]() {
            auto& app = Application::GetInstance();
            auto sd_music = app.GetSdMusic();
            
            if (sd_music) {
                auto state = sd_music->getState();
                if (state == Esp32SdMusic::PlayerState::Playing) {
                    sd_music->pause();
                    GetDisplay()->ShowNotification("Tạm dừng ⏸");
                } else if (state == Esp32SdMusic::PlayerState::Paused) {
                    sd_music->play();
                    GetDisplay()->ShowNotification("Tiếp tục ▶");
                } else {
                    // Stopped - start playing from beginning
                    if (sd_music->getTotalTracks() > 0) {
#ifdef MUSIC_SHUFFLE_DEFAULT
                        sd_music->shuffle(true);
#endif
                        sd_music->play();
                        auto track = sd_music->getCurrentTrack();
                        GetDisplay()->ShowNotification("▶ " + track);
                    } else {
                        GetDisplay()->ShowNotification("Không có nhạc trong thẻ SD");
                    }
                }
            }
        });
        
        music_btn.SetOnNextTrack([this]() {
            auto& app = Application::GetInstance();
            auto sd_music = app.GetSdMusic();
            
            if (sd_music && sd_music->getTotalTracks() > 0) {
                sd_music->next();
                auto track = sd_music->getCurrentTrack();
                GetDisplay()->ShowNotification("⏭ " + track);
            }
        });
        
        music_btn.SetOnPrevTrack([this]() {
            auto& app = Application::GetInstance();
            auto sd_music = app.GetSdMusic();
            
            if (sd_music && sd_music->getTotalTracks() > 0) {
                sd_music->prev();
                auto track = sd_music->getCurrentTrack();
                GetDisplay()->ShowNotification("⏮ " + track);
            }
        });
        
        music_btn.SetOnShuffleToggle([this]() {
            auto& app = Application::GetInstance();
            auto sd_music = app.GetSdMusic();
            
            if (sd_music) {
                static bool shuffle_enabled = MUSIC_SHUFFLE_DEFAULT;
                shuffle_enabled = !shuffle_enabled;
                sd_music->shuffle(shuffle_enabled);
                GetDisplay()->ShowNotification(shuffle_enabled ? "Shuffle: BẬT 🔀" : "Shuffle: TẮT");
            }
        });
        
        ESP_LOGI(TAG, "Music button initialized!");
        ESP_LOGI(TAG, "- 1 nhấn: Play/Pause");
        ESP_LOGI(TAG, "- 2 nhấn nhanh: Bài tiếp theo");
        ESP_LOGI(TAG, "- Giữ 1s: Bài trước");
        ESP_LOGI(TAG, "- Giữ 3s: Bật/Tắt Shuffle");
#else
        ESP_LOGI(TAG, "Music Button DISABLED (MUSIC_BUTTON_GPIO not defined)");
#endif
    }

    // ========================================================================
    // Vehicle Data Display Task - show engine info when CAN connected
    // ========================================================================
    void StartVehicleDataDisplayTask() {
        xTaskCreate([](void* param) {
            auto* board = static_cast<XiaozhiAiIotVietnamBoardLcdSdcard*>(param);
            auto& can = canbus::CanBusDriver::GetInstance();
            
            // Wait for CAN status to finish (30s)
            vTaskDelay(pdMS_TO_TICKS(35000));
            
            while (true) {
                auto stats = can.GetStats();
                
                // Only display vehicle data if CAN is connected and receiving messages
                if (stats.rx_count > 0) {
                    char vehicle_info[256];
                    snprintf(vehicle_info, sizeof(vehicle_info),
                        "🚗 THÔNG TIN XE\n\n"
                        "📊 CAN: %u tin/s\n"
                        "🔌 Trạng thái: Hoạt động\n"
                        "💬 Thử lệnh: bật điều hoà",
                        (unsigned int)stats.rx_count);
                    board->GetDisplay()->SetChatMessage("system", vehicle_info);
                }
                
                vTaskDelay(pdMS_TO_TICKS(5000));  // Update every 5 seconds
            }
        }, "vehicle_data_display", 3072, this, 2, nullptr);
    }



public:
    XiaozhiAiIotVietnamBoardLcdSdcard() :
        boot_button_(BOOT_BUTTON_GPIO),
        volume_up_button_(VOLUME_UP_BUTTON_GPIO),
        volume_down_button_(VOLUME_DOWN_BUTTON_GPIO) {
        InitializeSpi();
        InitializeLcdDisplay();
        InitializeButtons();
        InitializeTools();
        InitializeRelays();  // Initialize relay control for trunk, AC
        // ⚠️ DELAY CAN initialization - SD card needs to mount first (11-14s)
        // Starting CAN too early causes CPU overload and breaks SD card mount
        // InitializeCanBus();  // Will be called later after SD card ready
        InitializeMusicButton();  // Initialize music button GPIO3
        if (DISPLAY_BACKLIGHT_PIN != GPIO_NUM_NC) {
            GetBacklight()->RestoreBrightness();
        }
        
        // Schedule CAN initialization after SD card mount (~15 seconds)
        xTaskCreate([](void* param) {
            auto* board = static_cast<XiaozhiAiIotVietnamBoardLcdSdcard*>(param);
            ESP_LOGI(TAG, "⏳ Waiting for SD card to mount before starting CAN...");
            vTaskDelay(pdMS_TO_TICKS(15000));  // Wait ~15s for SD card mount + buffer
            board->InitializeCanBus();  // Now safe to start CAN
            ESP_LOGI(TAG, "✅ CAN Bus initialization complete");
            vTaskDelete(nullptr);
        }, "can_init_delay", 4096, this, 3, nullptr);

        // Schedule SD card mount status display (~13s after boot, show briefly)
        xTaskCreate([](void* param) {
            auto* board = static_cast<XiaozhiAiIotVietnamBoardLcdSdcard*>(param);

            vTaskDelay(pdMS_TO_TICKS(13000));

            if (board->IsSdCardMounted()) {
                ESP_LOGI("SD_STATUS", "✓ SD card mounted successfully");
                board->GetDisplay()->SetChatMessage("system", "✅ Thẻ nhớ OK\n📁 Sẵn sàng phát nhạc");
            } else {
                ESP_LOGW("SD_STATUS", "✗ SD card mount failed");
                board->GetDisplay()->SetChatMessage("system", "❌ Thẻ nhớ lỗi\n💡 Kiểm tra khe cắm\n🔌 Thử lại sau");
            }

            vTaskDelay(pdMS_TO_TICKS(3000));
            board->GetDisplay()->SetChatMessage("system", "");
            vTaskDelete(nullptr);
        }, "sd_status", 4096, this, 2, nullptr);
        
        // Schedule CAN status display ~15s after startup to allow CAN bus to scan properly.
        xTaskCreate([](void* param) {
            auto* board = static_cast<XiaozhiAiIotVietnamBoardLcdSdcard*>(param);
            auto& app = Application::GetInstance();

            const TickType_t poll_ticks = pdMS_TO_TICKS(200) > 0 ? pdMS_TO_TICKS(200) : 1;
            int waited_ms = 0;
            while (!board->IsSdCardMounted() && waited_ms < 30000) {
                vTaskDelay(poll_ticks);
                waited_ms += 200;
            }
            vTaskDelay(pdMS_TO_TICKS(15000));  // Wait ~15s for CAN bus to start receiving messages
            
            // Check CAN connection status
            auto& can = canbus::CanBusDriver::GetInstance();
            auto stats = can.GetStats();

            char msg[256];
            bool connected = (stats.rx_count > 0);
            if (connected) {
                ESP_LOGI("CAN_STATUS", "✅ CAN kết nối! Nhận %lu messages", stats.rx_count);
                app.PlaySound(Lang::Sounds::OGG_SUCCESS);
                vTaskDelay(pdMS_TO_TICKS(500));
                snprintf(msg, sizeof(msg),
                    "✅ ĐÃ KẾT NỐI VỚI XE\n\n"
                    "🚗 Kia Morning 2017\n"
                    "📊 Nhận: %lu tin nhắn\n"
                    "💬 Thử nói lệnh...",
                    stats.rx_count);
            } else {
                ESP_LOGW("CAN_STATUS", "❌ CAN chưa kết nối - Kiểm tra OBD-II");
                app.PlaySound(Lang::Sounds::OGG_EXCLAMATION);
                vTaskDelay(pdMS_TO_TICKS(300));
                app.PlaySound(Lang::Sounds::OGG_EXCLAMATION);
                vTaskDelay(pdMS_TO_TICKS(500));
                snprintf(msg, sizeof(msg),
                    "❌ CHƯA KẾT NỐI VỚI XE\n\n"
                    "🔌 Kiểm tra OBD-II:\n"
                    "  • Pin 6: CANH (Dây đỏ)\n"
                    "  • Pin 14: CANL (Dây đen)\n"
                    "  • Pin 4/5: GND (Đất)\n\n"
                    "🚗 Bật xe (ACC/ON)");
            }

            // Keep the message visible for 30 seconds.
            // Some other subsystems may call SetChatMessage("system", "") during boot
            // (which clears the whole chat on LCD), so we refresh once per second.
            for (int i = 0; i < 30; i++) {
                // Update stats in real-time to show latest rx_count
                auto fresh_stats = can.GetStats();
                if (connected && fresh_stats.rx_count > stats.rx_count) {
                    snprintf(msg, sizeof(msg),
                        "✅ ĐÃ KẾT NỐI VỚI XE\n\n"
                        "🚗 Kia Morning 2017\n"
                        "📊 Nhận: %lu tin nhắn\n"
                        "💬 Thử nói lệnh...",
                        fresh_stats.rx_count);
                    stats.rx_count = fresh_stats.rx_count;
                }
                board->GetDisplay()->SetChatMessage("system", msg);
                vTaskDelay(pdMS_TO_TICKS(1000));
            }

            board->GetDisplay()->SetChatMessage("system", "");
            vTaskDelete(nullptr);
        }, "can_status_display", 4096, this, 2, nullptr);
    }

    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    // SD card status management
    void SetSdCardMounted(bool mounted) {
        sd_card_mounted = mounted;
    }

    bool IsSdCardMounted() const {
        return sd_card_mounted;
    }

    virtual AudioCodec* GetAudioCodec() override {
#ifdef AUDIO_I2S_METHOD_SIMPLEX
        static NoAudioCodecSimplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT, AUDIO_I2S_MIC_GPIO_SCK, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
#else
        static NoAudioCodecDuplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN);
#endif
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }

    virtual Backlight* GetBacklight() override {
        if (DISPLAY_BACKLIGHT_PIN != GPIO_NUM_NC) {
            static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
            return &backlight;
        }
        return nullptr;
    }

    // Override StartNetwork để hỗ trợ chế độ OFFLINE
    virtual void StartNetwork() override {
        // Kiểm tra flag offline mode từ NVS
        Settings offline_settings("offline", false);
        
        // GetInt() trả về giá trị trực tiếp, không phải qua reference
        int offline_enabled = offline_settings.GetInt("enabled", 0);
        ESP_LOGI(TAG, "🔍 Checking offline flag: value=%d", offline_enabled);
        
        if (offline_enabled == 1) {
            offline_mode_ = true;
            ESP_LOGW(TAG, "========================================");
            ESP_LOGW(TAG, "📴 CHẾ ĐỘ OFFLINE - Không cần WiFi");
            ESP_LOGW(TAG, "   CAN bus, SD music, local control OK");
            ESP_LOGW(TAG, "   Nói 'Bật online' để kết nối WiFi");
            ESP_LOGW(TAG, "========================================");
            
            GetDisplay()->SetChatMessage("system", "📴 CHẾ ĐỘ OFFLINE\n✅ CAN bus OK\n✅ Nhạc SD OK\n💬 Nói 'Bật online'");
            
            // Không gọi WifiBoard::StartNetwork() - skip WiFi hoàn toàn
            return;
        }
        
        // Chế độ bình thường - kết nối WiFi
        ESP_LOGI(TAG, "📶 CHẾ ĐỘ ONLINE - Kết nối WiFi...");
        WifiBoard::StartNetwork();
    }
    // Kiểm tra đang ở chế độ offline không
    bool IsOfflineMode() const {
        return offline_mode_;
    }

#ifdef CONFIG_SD_CARD_DISABLED
    virtual SdCard* GetSdCard() override {
        return nullptr;
    }
#else
    virtual SdCard* GetSdCard() override {
#ifdef CONFIG_SD_CARD_MMC_INTERFACE
#ifdef CARD_SDMMC_BUS_WIDTH_4BIT
        static SdMMC sdmmc(CARD_SDMMC_CLK_GPIO,
                           CARD_SDMMC_CMD_GPIO,
                           CARD_SDMMC_D0_GPIO,
                           CARD_SDMMC_D1_GPIO,
                           CARD_SDMMC_D2_GPIO,
                           CARD_SDMMC_D3_GPIO);
#else
        static SdMMC sdmmc(CARD_SDMMC_CLK_GPIO,
                           CARD_SDMMC_CMD_GPIO,
                           CARD_SDMMC_D0_GPIO);
#endif
        return &sdmmc;
#elif defined(CONFIG_SD_CARD_SPI_INTERFACE)
        static SdSPI sdspi(CARD_SPI_MOSI_GPIO,
                           CARD_SPI_MISO_GPIO,
                           CARD_SPI_SCLK_GPIO,
                           CARD_SPI_CS_GPIO,
                           SPI2_HOST,
                           15000);  // Optimized to 15MHz - fast enough for SD card, reduces EMI with LCD at 10MHz
        return &sdspi;
#else
        return nullptr;
#endif
    }
#endif
};

DECLARE_BOARD(XiaozhiAiIotVietnamBoardLcdSdcard);
