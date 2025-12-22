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

// Offline mode and music button
#include "offline/offline_audio_player.h"
#include "offline/offline_audio_assets.h"
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

class XiaozhiAiIotVietnamBoardLcdSdcard : public WifiBoard {
private:
 
    Button boot_button_;
    Button volume_up_button_;
    Button volume_down_button_;
    LcdDisplay* display_;

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
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });

        // Boot button long press: Stop music/radio playback
        boot_button_.OnLongPress([this]() {
            auto& app = Application::GetInstance();
            auto sd_music = app.GetSdMusic();
            auto radio = app.GetRadio();
            
            // Check if any music is playing and stop it
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
        assistant.SetSpeakCallback([](const std::string& message) {
            ESP_LOGI(TAG, "Vehicle says: %s", message.c_str());
            // TODO: Integrate with Xiaozhi TTS when ready
            // Application::GetInstance().Speak(message);
        });
        
        assistant.SetDisplayCallback([this](const std::string& text, int line) {
            // Display vehicle info on LCD (could use notification or dedicated area)
            ESP_LOGD(TAG, "Display L%d: %s", line, text.c_str());
        });
        
        // Start CAN bus driver
        if (!can_driver.Start()) {
            ESP_LOGE(TAG, "Failed to start CAN bus driver!");
            return;
        }
        
        // Start vehicle assistant
        if (!assistant.Start()) {
            ESP_LOGE(TAG, "Failed to start Vehicle Assistant!");
            return;
        }
        
        ESP_LOGI(TAG, "CAN Bus and Vehicle Assistant started successfully!");
        ESP_LOGI(TAG, "Listening for Kia Morning 2017 CAN messages...");
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
    // Offline Audio Player Initialization - Load từ Flash Assets
    // ========================================================================
    void InitializeOfflineAudio() {
#ifdef CONFIG_ENABLE_OFFLINE_MODE
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "Initializing Offline Audio Assets (Flash)");
        ESP_LOGI(TAG, "========================================");
        
        // Khởi tạo offline audio player từ flash assets
        auto& offline_assets = offline::OfflineAudioAssets::GetInstance();
        if (offline_assets.Initialize()) {
            ESP_LOGI(TAG, "Offline Audio Assets ready! %d files from Flash", 
                     (int)offline_assets.GetAudioFileCount());
        } else {
            ESP_LOGW(TAG, "Offline Audio Assets failed to initialize");
            ESP_LOGI(TAG, "Run: python scripts/flash_audio_assets.py");
        }
        
        // Fallback: thử khởi tạo từ SD card nếu flash không có
        auto sd_card = GetSdCard();
        if (!offline_assets.IsInitialized() && sd_card) {
            ESP_LOGI(TAG, "Trying SD card fallback...");
            auto& offline_player = offline::OfflineAudioPlayer::GetInstance();
            if (offline_player.Initialize("/sdcard")) {
                ESP_LOGI(TAG, "Offline Audio Player (SD) ready! %d files", 
                         (int)offline_player.GetAudioFileCount());
            }
        }
#endif
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
        InitializeCanBus();  // Initialize CAN bus for Kia Morning 2017
        InitializeMusicButton();  // Initialize music button GPIO3
        InitializeOfflineAudio(); // Initialize offline audio player
        if (DISPLAY_BACKLIGHT_PIN != GPIO_NUM_NC) {
            GetBacklight()->RestoreBrightness();
        }
    }

    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
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
