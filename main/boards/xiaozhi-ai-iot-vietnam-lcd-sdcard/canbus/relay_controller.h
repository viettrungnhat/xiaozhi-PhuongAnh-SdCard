#ifndef _RELAY_CONTROLLER_H_
#define _RELAY_CONTROLLER_H_

/**
 * @file relay_controller.h
 * @brief Relay Controller for Kia Morning 2017 Si - Vehicle Control
 * @author Xiaozhi AI IoT Vietnam
 * @date 2024
 * 
 * Điều khiển relay để mở cốp điện, điều hòa, v.v.
 * Tích hợp với MCP để điều khiển bằng giọng nói.
 */

#include "../config.h"
#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string>

namespace relay {

/**
 * @class RelayController
 * @brief Controller cho một relay đơn lẻ
 */
class RelayController {
public:
    /**
     * @brief Constructor
     * @param gpio_num GPIO pin kết nối với relay
     * @param active_level Mức kích hoạt (0 = LOW, 1 = HIGH)
     * @param name Tên relay (để log)
     */
    RelayController(gpio_num_t gpio_num, int active_level, const char* name)
        : gpio_num_(gpio_num)
        , active_level_(active_level)
        , name_(name)
        , is_initialized_(false)
        , is_on_(false)
    {
        Initialize();
    }

    /**
     * @brief Khởi tạo GPIO cho relay
     */
    bool Initialize() {
        if (gpio_num_ == GPIO_NUM_NC) {
            ESP_LOGW("Relay", "%s: GPIO not configured (NC)", name_);
            return false;
        }

        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = (1ULL << gpio_num_);
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

        esp_err_t err = gpio_config(&io_conf);
        if (err != ESP_OK) {
            ESP_LOGE("Relay", "%s: Failed to configure GPIO%d: %s", 
                     name_, gpio_num_, esp_err_to_name(err));
            return false;
        }

        // Đặt relay ở trạng thái tắt
        TurnOff();
        is_initialized_ = true;

        ESP_LOGI("Relay", "%s initialized on GPIO%d (active %s)", 
                 name_, gpio_num_, active_level_ ? "HIGH" : "LOW");
        return true;
    }

    /**
     * @brief Bật relay
     */
    void TurnOn() {
        if (!is_initialized_ || gpio_num_ == GPIO_NUM_NC) return;
        
        gpio_set_level(gpio_num_, active_level_ ? 1 : 0);
        is_on_ = true;
        ESP_LOGI("Relay", "%s: ON", name_);
    }

    /**
     * @brief Tắt relay
     */
    void TurnOff() {
        if (gpio_num_ == GPIO_NUM_NC) return;
        
        gpio_set_level(gpio_num_, active_level_ ? 0 : 1);
        is_on_ = false;
        ESP_LOGI("Relay", "%s: OFF", name_);
    }

    /**
     * @brief Toggle relay
     */
    void Toggle() {
        if (is_on_) {
            TurnOff();
        } else {
            TurnOn();
        }
    }

    /**
     * @brief Kích relay trong khoảng thời gian (pulse)
     * @param duration_ms Thời gian kích (ms)
     */
    void Pulse(uint32_t duration_ms) {
        if (!is_initialized_ || gpio_num_ == GPIO_NUM_NC) return;
        
        ESP_LOGI("Relay", "%s: PULSE %lu ms", name_, duration_ms);
        TurnOn();
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        TurnOff();
    }

    /**
     * @brief Kích relay trong khoảng thời gian (async - không block)
     * @param duration_ms Thời gian kích (ms)
     */
    void PulseAsync(uint32_t duration_ms) {
        if (!is_initialized_ || gpio_num_ == GPIO_NUM_NC) return;
        
        // Tạo task để pulse mà không block
        struct PulseParams {
            RelayController* controller;
            uint32_t duration_ms;
        };
        
        PulseParams* params = new PulseParams{this, duration_ms};
        
        xTaskCreate([](void* arg) {
            PulseParams* p = static_cast<PulseParams*>(arg);
            p->controller->Pulse(p->duration_ms);
            delete p;
            vTaskDelete(NULL);
        }, "relay_pulse", 2048, params, 5, NULL);
    }

    /**
     * @brief Kiểm tra relay có đang bật không
     */
    bool IsOn() const { return is_on_; }

    /**
     * @brief Lấy tên relay
     */
    const char* GetName() const { return name_; }

    /**
     * @brief Lấy GPIO number
     */
    gpio_num_t GetGpio() const { return gpio_num_; }

private:
    gpio_num_t gpio_num_;
    int active_level_;
    const char* name_;
    bool is_initialized_;
    bool is_on_;
};

// ============================================================================
// Vehicle Relay Manager - Quản lý tất cả relay trên xe
// ============================================================================

/**
 * @class VehicleRelayManager
 * @brief Quản lý tất cả relay điều khiển xe
 */
class VehicleRelayManager {
public:
    static VehicleRelayManager& GetInstance() {
        static VehicleRelayManager instance;
        return instance;
    }

    /**
     * @brief Mở cốp xe
     * @return Thông báo kết quả
     */
    std::string OpenTrunk() {
#ifdef CONFIG_ENABLE_RELAY_CONTROL
        if (trunk_relay_) {
            trunk_relay_->PulseAsync(RELAY_TRUNK_PULSE_MS);
            return "Đã mở cốp xe!";
        }
#endif
        return "Chức năng mở cốp chưa được cấu hình.";
    }

    /**
     * @brief Bật điều hòa
     * @return Thông báo kết quả
     */
    std::string TurnOnAC() {
#ifdef CONFIG_ENABLE_RELAY_CONTROL
#ifdef RELAY_AC_GPIO
        if (ac_relay_) {
            ac_relay_->TurnOn();
            return "Đã bật điều hòa!";
        }
#endif
#endif
        return "Chức năng điều khiển điều hòa chưa được cấu hình.";
    }

    /**
     * @brief Tắt điều hòa
     * @return Thông báo kết quả
     */
    std::string TurnOffAC() {
#ifdef CONFIG_ENABLE_RELAY_CONTROL
#ifdef RELAY_AC_GPIO
        if (ac_relay_) {
            ac_relay_->TurnOff();
            return "Đã tắt điều hòa!";
        }
#endif
#endif
        return "Chức năng điều khiển điều hòa chưa được cấu hình.";
    }

    /**
     * @brief Toggle điều hòa
     * @return Thông báo kết quả
     */
    std::string ToggleAC() {
#ifdef CONFIG_ENABLE_RELAY_CONTROL
#ifdef RELAY_AC_GPIO
        if (ac_relay_) {
            ac_relay_->Toggle();
            return ac_relay_->IsOn() ? "Đã bật điều hòa!" : "Đã tắt điều hòa!";
        }
#endif
#endif
        return "Chức năng điều khiển điều hòa chưa được cấu hình.";
    }

    /**
     * @brief Lấy trạng thái tất cả relay
     */
    std::string GetStatus() {
        std::string status = "Trạng thái relay: ";
#ifdef CONFIG_ENABLE_RELAY_CONTROL
        if (trunk_relay_) {
            status += "Cốp (GPIO" + std::to_string(trunk_relay_->GetGpio()) + ") ";
        }
#ifdef RELAY_AC_GPIO
        if (ac_relay_) {
            status += "| Điều hòa: ";
            status += ac_relay_->IsOn() ? "BẬT" : "TẮT";
        }
#endif
#else
        status += "Relay control đang tắt.";
#endif
        return status;
    }

private:
    VehicleRelayManager() {
#ifdef CONFIG_ENABLE_RELAY_CONTROL
        ESP_LOGI("VehicleRelay", "Initializing Vehicle Relay Manager...");
        
        // Khởi tạo relay cốp
        trunk_relay_ = new RelayController(RELAY_TRUNK_GPIO, RELAY_TRUNK_ACTIVE_LEVEL, "Trunk");
        
#ifdef RELAY_AC_GPIO
        // Khởi tạo relay điều hòa
        ac_relay_ = new RelayController(RELAY_AC_GPIO, RELAY_AC_ACTIVE_LEVEL, "AC");
#endif
        
        ESP_LOGI("VehicleRelay", "Vehicle Relay Manager initialized");
#else
        ESP_LOGI("VehicleRelay", "Relay control is DISABLED");
#endif
    }

    ~VehicleRelayManager() {
#ifdef CONFIG_ENABLE_RELAY_CONTROL
        delete trunk_relay_;
#ifdef RELAY_AC_GPIO
        delete ac_relay_;
#endif
#endif
    }

    VehicleRelayManager(const VehicleRelayManager&) = delete;
    VehicleRelayManager& operator=(const VehicleRelayManager&) = delete;

#ifdef CONFIG_ENABLE_RELAY_CONTROL
    RelayController* trunk_relay_ = nullptr;
#ifdef RELAY_AC_GPIO
    RelayController* ac_relay_ = nullptr;
#endif
#endif
};

// ============================================================================
// MCP Tool Functions - Điều khiển bằng giọng nói
// ============================================================================

/**
 * @brief MCP tool: Mở cốp xe
 */
inline std::string MCP_OpenTrunk() {
    return VehicleRelayManager::GetInstance().OpenTrunk();
}

/**
 * @brief MCP tool: Bật điều hòa
 */
inline std::string MCP_TurnOnAC() {
    return VehicleRelayManager::GetInstance().TurnOnAC();
}

/**
 * @brief MCP tool: Tắt điều hòa
 */
inline std::string MCP_TurnOffAC() {
    return VehicleRelayManager::GetInstance().TurnOffAC();
}

/**
 * @brief MCP tool: Toggle điều hòa
 */
inline std::string MCP_ToggleAC() {
    return VehicleRelayManager::GetInstance().ToggleAC();
}

} // namespace relay

#endif // _RELAY_CONTROLLER_H_
