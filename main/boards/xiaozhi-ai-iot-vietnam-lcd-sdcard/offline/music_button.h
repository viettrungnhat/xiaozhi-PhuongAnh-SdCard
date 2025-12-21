#ifndef MUSIC_BUTTON_H
#define MUSIC_BUTTON_H

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <atomic>
#include <functional>

#include "config.h"

namespace music {

/**
 * @brief Music Button Controller - Điều khiển phát nhạc bằng nút nhấn
 * 
 * Chức năng nút nhấn:
 * - Nhấn 1 lần: Play/Pause
 * - Nhấn 2 lần nhanh: Next track
 * - Nhấn giữ 1 giây: Previous track / Stop
 * - Nhấn giữ 3 giây: Shuffle on/off
 */
class MusicButtonController {
public:
    // Callback types
    using OnPlayPauseCallback = std::function<void()>;
    using OnNextTrackCallback = std::function<void()>;
    using OnPrevTrackCallback = std::function<void()>;
    using OnShuffleToggleCallback = std::function<void()>;
    using OnStopCallback = std::function<void()>;
    
    // Singleton
    static MusicButtonController& GetInstance() {
        static MusicButtonController instance;
        return instance;
    }
    
    /**
     * @brief Khởi tạo nút nhấn
     * @return true nếu thành công
     */
    bool Initialize() {
        if (is_initialized_) {
            return true;
        }
        
#ifndef MUSIC_BUTTON_GPIO
        ESP_LOGW(TAG, "MUSIC_BUTTON_GPIO not defined");
        return false;
#else
        // Cấu hình GPIO
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_ANYEDGE;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = (1ULL << MUSIC_BUTTON_GPIO);
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        
        esp_err_t err = gpio_config(&io_conf);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure music button GPIO: %s", esp_err_to_name(err));
            return false;
        }
        
        // Tạo event queue
        button_event_queue_ = xQueueCreate(10, sizeof(ButtonEvent));
        if (button_event_queue_ == nullptr) {
            ESP_LOGE(TAG, "Failed to create button event queue");
            return false;
        }
        
        // Cài đặt ISR handler
        gpio_install_isr_service(0);
        gpio_isr_handler_add(MUSIC_BUTTON_GPIO, ButtonISRHandler, this);
        
        // Tạo task xử lý nút nhấn
        xTaskCreate(
            ButtonTaskWrapper,
            "music_btn",
            2048,
            this,
            4,
            &button_task_handle_
        );
        
        is_initialized_ = true;
        ESP_LOGI(TAG, "Music button initialized on GPIO%d", MUSIC_BUTTON_GPIO);
        return true;
#endif
    }
    
    /**
     * @brief Dừng và giải phóng tài nguyên
     */
    void Deinitialize() {
        if (!is_initialized_) return;
        
#ifdef MUSIC_BUTTON_GPIO
        gpio_isr_handler_remove(MUSIC_BUTTON_GPIO);
#endif
        
        if (button_task_handle_) {
            vTaskDelete(button_task_handle_);
            button_task_handle_ = nullptr;
        }
        
        if (button_event_queue_) {
            vQueueDelete(button_event_queue_);
            button_event_queue_ = nullptr;
        }
        
        is_initialized_ = false;
    }
    
    // Set callbacks
    void SetOnPlayPause(OnPlayPauseCallback callback) { on_play_pause_ = callback; }
    void SetOnNextTrack(OnNextTrackCallback callback) { on_next_track_ = callback; }
    void SetOnPrevTrack(OnPrevTrackCallback callback) { on_prev_track_ = callback; }
    void SetOnShuffleToggle(OnShuffleToggleCallback callback) { on_shuffle_toggle_ = callback; }
    void SetOnStop(OnStopCallback callback) { on_stop_ = callback; }
    
    bool IsInitialized() const { return is_initialized_; }
    
private:
    static constexpr const char* TAG = "MusicButton";
    
    // Button event types
    enum class ButtonEvent {
        PRESSED,
        RELEASED
    };
    
    MusicButtonController() = default;
    ~MusicButtonController() { Deinitialize(); }
    MusicButtonController(const MusicButtonController&) = delete;
    MusicButtonController& operator=(const MusicButtonController&) = delete;
    
    static void IRAM_ATTR ButtonISRHandler(void* arg) {
        auto* self = static_cast<MusicButtonController*>(arg);
#ifdef MUSIC_BUTTON_GPIO
        int level = gpio_get_level(MUSIC_BUTTON_GPIO);
#ifdef MUSIC_BUTTON_ACTIVE_LOW
        ButtonEvent event = (level == 0) ? ButtonEvent::PRESSED : ButtonEvent::RELEASED;
#else
        ButtonEvent event = (level == 1) ? ButtonEvent::PRESSED : ButtonEvent::RELEASED;
#endif
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(self->button_event_queue_, &event, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
#endif
    }
    
    static void ButtonTaskWrapper(void* arg) {
        auto* self = static_cast<MusicButtonController*>(arg);
        self->ButtonTask();
    }
    
    void ButtonTask() {
        ButtonEvent event;
        int64_t press_time = 0;
        int click_count = 0;
        int64_t last_click_time = 0;
        
        while (true) {
            if (xQueueReceive(button_event_queue_, &event, pdMS_TO_TICKS(50)) == pdTRUE) {
                int64_t now = esp_timer_get_time() / 1000; // Convert to ms
                
                if (event == ButtonEvent::PRESSED) {
                    press_time = now;
                    ESP_LOGD(TAG, "Button pressed");
                } else if (event == ButtonEvent::RELEASED) {
                    int64_t press_duration = now - press_time;
                    ESP_LOGD(TAG, "Button released, duration: %lld ms", press_duration);
                    
                    // Xử lý theo thời gian nhấn
                    if (press_duration >= 3000) {
                        // Nhấn giữ 3 giây: Toggle shuffle
                        ESP_LOGI(TAG, "Long press (3s): Toggle shuffle");
                        if (on_shuffle_toggle_) {
                            on_shuffle_toggle_();
                        }
                    } else if (press_duration >= MUSIC_BUTTON_LONG_PRESS_MS) {
                        // Nhấn giữ 1 giây: Previous track hoặc Stop
                        ESP_LOGI(TAG, "Long press (1s): Previous track");
                        if (on_prev_track_) {
                            on_prev_track_();
                        }
                    } else if (press_duration >= MUSIC_BUTTON_DEBOUNCE_MS) {
                        // Click bình thường
                        if (now - last_click_time < MUSIC_BUTTON_DOUBLE_CLICK_MS) {
                            // Double click: Next track
                            click_count++;
                            if (click_count >= 2) {
                                ESP_LOGI(TAG, "Double click: Next track");
                                if (on_next_track_) {
                                    on_next_track_();
                                }
                                click_count = 0;
                            }
                        } else {
                            // Single click (sẽ được xác nhận sau timeout)
                            click_count = 1;
                        }
                        last_click_time = now;
                    }
                }
            } else {
                // Timeout - kiểm tra single click
                if (click_count == 1) {
                    int64_t now = esp_timer_get_time() / 1000;
                    if (now - last_click_time >= MUSIC_BUTTON_DOUBLE_CLICK_MS) {
                        // Confirmed single click: Play/Pause
                        ESP_LOGI(TAG, "Single click: Play/Pause");
                        if (on_play_pause_) {
                            on_play_pause_();
                        }
                        click_count = 0;
                    }
                }
            }
        }
    }
    
    std::atomic<bool> is_initialized_{false};
    
    TaskHandle_t button_task_handle_ = nullptr;
    QueueHandle_t button_event_queue_ = nullptr;
    
    // Callbacks
    OnPlayPauseCallback on_play_pause_;
    OnNextTrackCallback on_next_track_;
    OnPrevTrackCallback on_prev_track_;
    OnShuffleToggleCallback on_shuffle_toggle_;
    OnStopCallback on_stop_;
};

} // namespace music

#endif // MUSIC_BUTTON_H
