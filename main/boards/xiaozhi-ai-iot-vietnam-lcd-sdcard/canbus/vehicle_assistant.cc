/**
 * @file vehicle_assistant.cc
 * @brief Vehicle Assistant Implementation for Kia Morning 2017 Si
 * @author Xiaozhi AI IoT Vietnam
 * @date 2024
 * 
 * Implementation of the intelligent vehicle assistant with
 * greeting, safety alerts, and voice interaction features.
 */

#include "vehicle_assistant.h"
#include "relay_controller.h"
#include "../config.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <cstring>
#include <algorithm>
#include <cctype>

static const char* TAG = "Vehicle_Assistant";

namespace vehicle {

// ============================================================================
// Singleton Instance
// ============================================================================

VehicleAssistant& VehicleAssistant::GetInstance() {
    static VehicleAssistant instance;
    return instance;
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

VehicleAssistant::VehicleAssistant()
    : state_(AssistantState::IDLE)
    , is_running_(false)
    , highway_mode_(false)
    , monitoring_task_handle_(nullptr)
    , data_mutex_(nullptr)
    , speak_callback_(nullptr)
    , listen_callback_(nullptr)
    , display_callback_(nullptr)
    , sound_callback_(nullptr)
    , last_speed_announce_(0)
    , last_drive_time_check_(0)
    , greeting_done_(false)
    , last_oil_change_km_(0)
    , last_tire_check_km_(0)
    , last_major_service_km_(0)
    , is_initialized_(false)
{
    data_mutex_ = xSemaphoreCreateMutex();
    if (!data_mutex_) {
        ESP_LOGE(TAG, "Failed to create data mutex");
    }
    
    ESP_LOGI(TAG, "Vehicle Assistant created");
}

VehicleAssistant::~VehicleAssistant() {
    Stop();
    
    if (data_mutex_) {
        vSemaphoreDelete(data_mutex_);
        data_mutex_ = nullptr;
    }
}

// ============================================================================
// Initialization
// ============================================================================

bool VehicleAssistant::Initialize() {
    ESP_LOGI(TAG, "Initializing Vehicle Assistant");
    
    if (is_initialized_) {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }
    
    // Load maintenance data from NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("vehicle", NVS_READONLY, &nvs_handle);
    if (err == ESP_OK) {
        nvs_get_u32(nvs_handle, "oil_km", &last_oil_change_km_);
        nvs_get_u32(nvs_handle, "tire_km", &last_tire_check_km_);
        nvs_get_u32(nvs_handle, "major_km", &last_major_service_km_);
        nvs_close(nvs_handle);
        ESP_LOGI(TAG, "Loaded maintenance data: Oil=%lu, Tire=%lu, Major=%lu",
                 last_oil_change_km_, last_tire_check_km_, last_major_service_km_);
    } else {
        ESP_LOGW(TAG, "No saved maintenance data found");
    }
    
    // Initialize Kia protocol parser
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    if (!protocol.Initialize()) {
        ESP_LOGE(TAG, "Failed to initialize Kia protocol");
        return false;
    }
    
    // Register callbacks with protocol parser
    protocol.RegisterDataCallback([this](const kia::VehicleData& data) {
        OnVehicleDataUpdate(data);
    });
    
    protocol.RegisterDoorCallback([this](const kia::DoorStatus& old_s, const kia::DoorStatus& new_s) {
        OnDoorEvent(old_s, new_s);
    });
    
    protocol.RegisterAlertCallback([this](const char* msg, int priority) {
        OnAlert(msg, priority);
    });
    
    // Register CAN bus callback
    canbus::CanBusDriver& can = canbus::CanBusDriver::GetInstance();
    can.RegisterCallback([this](const canbus::CanMessage& msg) {
        OnCanMessage(msg);
    });
    
    // Register default smart scenarios
    RegisterScenario({
        "Bo chuan bi ve",
        "bố chuẩn bị về",
        [this]() {
            Speak("Vâng, em đã chuẩn bị sẵn sàng để bố về!");
#ifdef CONFIG_ENABLE_RELAY_CONTROL
            // Mở cốp để bố cất đồ
            relay::VehicleRelayManager::GetInstance().OpenTrunk();
            Speak("Em đã mở cốp để bố cất đồ.");
#endif
            Speak("Chúc bố có chuyến đi an toàn!");
        },
        true
    });
    
    // Scenario: Mở cốp
    RegisterScenario({
        "Mo cop",
        "mở cốp",
        [this]() {
#ifdef CONFIG_ENABLE_RELAY_CONTROL
            std::string result = relay::VehicleRelayManager::GetInstance().OpenTrunk();
            Speak(result);
#else
            Speak("Chức năng mở cốp chưa được kích hoạt.");
#endif
        },
        true
    });
    
    // Scenario: Bật điều hòa
    RegisterScenario({
        "Bat dieu hoa",
        "bật điều hòa",
        [this]() {
#ifdef CONFIG_ENABLE_RELAY_CONTROL
            std::string result = relay::VehicleRelayManager::GetInstance().TurnOnAC();
            Speak(result);
#else
            Speak("Chức năng điều khiển điều hòa chưa được kích hoạt.");
#endif
        },
        true
    });
    
    // Scenario: Tắt điều hòa
    RegisterScenario({
        "Tat dieu hoa",
        "tắt điều hòa",
        [this]() {
#ifdef CONFIG_ENABLE_RELAY_CONTROL
            std::string result = relay::VehicleRelayManager::GetInstance().TurnOffAC();
            Speak(result);
#else
            Speak("Chức năng điều khiển điều hòa chưa được kích hoạt.");
#endif
        },
        true
    });
    
    RegisterScenario({
        "Che do duong truong",
        "chế độ đường trường",
        [this]() {
            SetHighwayMode(true);
            Speak("Đã bật chế độ đường trường. Em sẽ đọc tốc độ định kỳ và nhắc bố nghỉ ngơi sau mỗi 2 tiếng lái xe.");
        },
        true
    });
    
    RegisterScenario({
        "Tat che do duong truong",
        "tắt chế độ đường trường",
        [this]() {
            SetHighwayMode(false);
            Speak("Đã tắt chế độ đường trường.");
        },
        true
    });
    
    is_initialized_ = true;
    state_.store(AssistantState::IDLE);
    
    ESP_LOGI(TAG, "Vehicle Assistant initialized successfully");
    return true;
}

// ============================================================================
// Start / Stop
// ============================================================================

bool VehicleAssistant::Start() {
    ESP_LOGI(TAG, "Starting Vehicle Assistant");
    
    if (!is_initialized_) {
        ESP_LOGE(TAG, "Not initialized");
        return false;
    }
    
    if (is_running_.load()) {
        ESP_LOGW(TAG, "Already running");
        return true;
    }
    
    is_running_.store(true);
    greeting_done_ = false;
    
    // Create monitoring task
    BaseType_t result = xTaskCreatePinnedToCore(
        MonitoringTaskWrapper,
        "vehicle_monitor",
        4096,
        this,
        CAN_TASK_PRIORITY - 1,  // Slightly lower priority than CAN
        &monitoring_task_handle_,
        CAN_TASK_CORE
    );
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create monitoring task");
        is_running_.store(false);
        return false;
    }
    
    state_.store(AssistantState::MONITORING);
    ESP_LOGI(TAG, "Vehicle Assistant started");
    
    return true;
}

void VehicleAssistant::Stop() {
    ESP_LOGI(TAG, "Stopping Vehicle Assistant");
    
    is_running_.store(false);
    
    if (monitoring_task_handle_) {
        vTaskDelay(pdMS_TO_TICKS(100));
        if (eTaskGetState(monitoring_task_handle_) != eDeleted) {
            vTaskDelete(monitoring_task_handle_);
        }
        monitoring_task_handle_ = nullptr;
    }
    
    state_.store(AssistantState::IDLE);
    ESP_LOGI(TAG, "Vehicle Assistant stopped");
}

// ============================================================================
// Callback Setters
// ============================================================================

void VehicleAssistant::SetSpeakCallback(SpeakCallback callback) {
    speak_callback_ = callback;
    ESP_LOGI(TAG, "Speak callback set");
}

void VehicleAssistant::SetListenCallback(ListenCallback callback) {
    listen_callback_ = callback;
    ESP_LOGI(TAG, "Listen callback set");
}

void VehicleAssistant::SetDisplayCallback(DisplayCallback callback) {
    display_callback_ = callback;
    ESP_LOGI(TAG, "Display callback set");
}

void VehicleAssistant::SetSoundCallback(SoundCallback callback) {
    sound_callback_ = callback;
    ESP_LOGI(TAG, "Sound callback set");
}

// ============================================================================
// CAN Data Handlers
// ============================================================================

void VehicleAssistant::OnCanMessage(const canbus::CanMessage& msg) {
    // Forward to protocol parser
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    protocol.ProcessMessage(msg);
}

void VehicleAssistant::OnVehicleDataUpdate(const kia::VehicleData& data) {
    // Update display periodically
    static int64_t last_display_update = 0;
    int64_t now = esp_timer_get_time() / 1000;
    
    if (now - last_display_update > 1000) {  // Update every second
        last_display_update = now;
        UpdateDisplayStatus();
    }
}

void VehicleAssistant::OnDoorEvent(const kia::DoorStatus& old_status, const kia::DoorStatus& new_status) {
    ESP_LOGI(TAG, "Door event: Driver door %s", 
             new_status.driver_door_open ? "OPENED" : "closed");
    
    // Check if driver door just opened (vehicle entry)
    if (!old_status.driver_door_open && new_status.driver_door_open) {
        OnDriverDoorOpened();
    }
}

void VehicleAssistant::OnAlert(const char* message, int priority) {
    ESP_LOGI(TAG, "Alert (priority %d): %s", priority, message);
    
    // Map priority to alert type for cooldown (simplified)
    AlertType type = (priority == 0) ? AlertType::CRITICAL_OVERHEAT : AlertType::LOW_BATTERY;
    
    if (ShouldSendAlert(type)) {
        if (priority == 0) {
            // Critical alert - play warning sound first
            PlaySound("warning");
        }
        Speak(message);
        RecordAlertSent(type);
    }
}

// ============================================================================
// Event Handlers
// ============================================================================

void VehicleAssistant::OnDriverDoorOpened() {
    ESP_LOGI(TAG, "Driver door opened - preparing greeting");
    
    if (!greeting_done_) {
        PerformGreeting();
    }
}

void VehicleAssistant::OnIgnitionOn() {
    ESP_LOGI(TAG, "Ignition turned ON");
    
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    const kia::VehicleData& data = protocol.GetVehicleData();
    
    // Initial safety check
    vTaskDelay(pdMS_TO_TICKS(2000));  // Wait for data to stabilize
    
    CheckSafetyConditions();
    
    // Health summary
    std::string health_msg = "Mọi hệ thống đều ổn định.";
    
    if (data.check_engine) {
        health_msg = "Lưu ý: Đèn check engine đang sáng.";
    } else if (data.tpms_warning) {
        health_msg = "Lưu ý: Cần kiểm tra áp suất lốp.";
    }
    
    // Check maintenance
    std::string maint_status = GetMaintenanceStatus();
    if (!maint_status.empty()) {
        health_msg += " " + maint_status;
    }
    
    Speak(health_msg);
}

void VehicleAssistant::OnIgnitionOff() {
    ESP_LOGI(TAG, "Ignition turned OFF");
    
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    const kia::VehicleData& data = protocol.GetVehicleData();
    
    // Check for lights left on
    if (data.lights.headlights_on || data.lights.parking_lights_on) {
        if (ShouldSendAlert(AlertType::LIGHTS_ON_ENGINE_OFF)) {
            Speak("Bố ơi, đèn vẫn đang bật. Bố nhớ tắt đèn nhé!");
            RecordAlertSent(AlertType::LIGHTS_ON_ENGINE_OFF);
        }
    }
    
    // Check for doors unlocked
    if (data.doors.any_door_unlocked) {
        Speak("Bố nhớ khóa cửa xe nhé!");
    }
    
    // Disable highway mode
    if (highway_mode_.load()) {
        SetHighwayMode(false);
    }
    
    // Reset greeting flag for next entry
    greeting_done_ = false;
    
    // Enter power save mode
    state_.store(AssistantState::POWER_SAVE);
}

// ============================================================================
// Greeting
// ============================================================================

void VehicleAssistant::PerformGreeting() {
    ESP_LOGI(TAG, "Performing greeting");
    
    state_.store(AssistantState::GREETING);
    
    // Build greeting message
    std::string greeting = "Chào bố! Hôm nay mình đi đâu thế ạ?";
    
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    const kia::VehicleData& data = protocol.GetVehicleData();
    
    // Add safety reminders if needed
    if (!data.seatbelt_driver) {
        greeting += " Bố nhớ thắt dây an toàn";
    }
    if (data.parking_brake_on) {
        greeting += " và hạ phanh tay";
    }
    greeting += " nhé! Chúc chuyến đi an toàn!";
    
    Speak(greeting);
    
    greeting_done_ = true;
    
    // Enter listening mode after greeting
    vTaskDelay(pdMS_TO_TICKS(3000));  // Wait for TTS to finish
    
    if (listen_callback_) {
        state_.store(AssistantState::LISTENING);
        listen_callback_();
    } else {
        state_.store(AssistantState::MONITORING);
    }
}

// ============================================================================
// Safety Monitoring
// ============================================================================

void VehicleAssistant::CheckSafetyConditions() {
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    const kia::VehicleData& data = protocol.GetVehicleData();
    
    // Seatbelt while moving
    if (!data.seatbelt_driver && data.vehicle_speed > 10) {
        if (ShouldSendAlert(AlertType::SEATBELT)) {
            Speak("Bố ơi, bố chưa thắt dây an toàn!");
            RecordAlertSent(AlertType::SEATBELT);
        }
    }
    
    // Parking brake while moving
    if (data.parking_brake_on && data.vehicle_speed > 5) {
        if (ShouldSendAlert(AlertType::PARKING_BRAKE)) {
            Speak("Bố ơi, phanh tay vẫn đang kéo!");
            RecordAlertSent(AlertType::PARKING_BRAKE);
        }
    }
    
    // Door ajar while moving
    if (data.door_ajar && data.vehicle_speed > 10) {
        if (ShouldSendAlert(AlertType::DOOR_OPEN)) {
            PlaySound("warning");
            Speak("CẢNH BÁO! Có cửa chưa đóng kín!");
            RecordAlertSent(AlertType::DOOR_OPEN);
        }
    }
}

void VehicleAssistant::CheckDriveTime() {
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    int drive_time = protocol.GetDrivingTimeMinutes();
    
    if (drive_time >= VEHICLE_MAX_DRIVE_TIME_MINUTES) {
        if (ShouldSendAlert(AlertType::LONG_DRIVE)) {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), 
                     "Bố ơi, bố đã lái xe %d tiếng rồi. Nên nghỉ ngơi một chút nhé!",
                     drive_time / 60);
            Speak(buffer);
            RecordAlertSent(AlertType::LONG_DRIVE);
        }
    }
}

void VehicleAssistant::AnnounceSpeed() {
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    const kia::VehicleData& data = protocol.GetVehicleData();
    
    char buffer[64];
    kia::FormatSpeedForVoice(data.vehicle_speed, buffer, sizeof(buffer));
    Speak(buffer);
}

void VehicleAssistant::UpdateDisplayStatus() {
    if (!display_callback_) return;
    
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    const kia::VehicleData& data = protocol.GetVehicleData();
    
    // Line 1: Speed and RPM
    char line1[32];
    snprintf(line1, sizeof(line1), "%.0f km/h | %.0f RPM", 
             data.vehicle_speed, data.engine_rpm);
    display_callback_(line1, 1);
    
    // Line 2: Fuel and Temp
    char line2[32];
    snprintf(line2, sizeof(line2), "Fuel: %.0f%% | %.0f°C", 
             data.fuel_level_percent, data.coolant_temp);
    display_callback_(line2, 2);
}

// ============================================================================
// Alert Management
// ============================================================================

bool VehicleAssistant::ShouldSendAlert(AlertType type) {
    int64_t now = esp_timer_get_time() / 1000;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        auto it = last_alert_time_.find(type);
        if (it == last_alert_time_.end()) {
            xSemaphoreGive(data_mutex_);
            return true;
        }
        
        bool should_send = (now - it->second) > ALERT_COOLDOWN_MS;
        xSemaphoreGive(data_mutex_);
        return should_send;
    }
    
    return false;
}

void VehicleAssistant::RecordAlertSent(AlertType type) {
    int64_t now = esp_timer_get_time() / 1000;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        last_alert_time_[type] = now;
        xSemaphoreGive(data_mutex_);
    }
}

void VehicleAssistant::Speak(const std::string& message) {
    ESP_LOGI(TAG, "Speaking: %s", message.c_str());
    
    if (speak_callback_) {
        speak_callback_(message);
    }
}

void VehicleAssistant::PlaySound(const std::string& sound) {
    ESP_LOGI(TAG, "Playing sound: %s", sound.c_str());
    
    if (sound_callback_) {
        sound_callback_(sound);
    }
}

// ============================================================================
// Monitoring Task
// ============================================================================

void VehicleAssistant::MonitoringTaskWrapper(void* arg) {
    VehicleAssistant* assistant = static_cast<VehicleAssistant*>(arg);
    assistant->MonitoringTask();
}

void VehicleAssistant::MonitoringTask() {
    ESP_LOGI(TAG, "Monitoring task started");
    
    while (is_running_.load()) {
        int64_t now = esp_timer_get_time() / 1000;
        
        // Periodic safety checks
        if (state_.load() == AssistantState::MONITORING || 
            state_.load() == AssistantState::HIGHWAY_MODE) {
            
            CheckSafetyConditions();
            
            // Drive time check every 10 minutes
            if (now - last_drive_time_check_ > DRIVE_TIME_CHECK_INTERVAL_MS) {
                last_drive_time_check_ = now;
                CheckDriveTime();
            }
            
            // Highway mode speed announcements
            if (highway_mode_.load()) {
                if (now - last_speed_announce_ > SPEED_ANNOUNCE_INTERVAL_MS) {
                    last_speed_announce_ = now;
                    AnnounceSpeed();
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));  // Check every second
    }
    
    ESP_LOGI(TAG, "Monitoring task ended");
    vTaskDelete(NULL);
}

// ============================================================================
// Voice Command Processing
// ============================================================================

bool VehicleAssistant::ProcessVoiceCommand(const std::string& command) {
    ESP_LOGI(TAG, "Processing command: %s", command.c_str());
    
    // Convert to lowercase for matching
    std::string lower_cmd = command;
    std::transform(lower_cmd.begin(), lower_cmd.end(), lower_cmd.begin(), ::tolower);
    
    // Check for scenario triggers
    for (const auto& scenario : scenarios_) {
        if (scenario.enabled && lower_cmd.find(scenario.trigger_phrase) != std::string::npos) {
            ESP_LOGI(TAG, "Triggering scenario: %s", scenario.name.c_str());
            scenario.action();
            return true;
        }
    }
    
    // Vehicle info queries
    if (ParseSpeedQuery(lower_cmd) ||
        ParseFuelQuery(lower_cmd) ||
        ParseTemperatureQuery(lower_cmd) ||
        ParseOdometerQuery(lower_cmd)) {
        return true;
    }
    
    return false;  // Let chatbot handle
}

bool VehicleAssistant::ParseSpeedQuery(const std::string& command) const {
    if (command.find("tốc độ") != std::string::npos ||
        command.find("nhanh") != std::string::npos ||
        command.find("speed") != std::string::npos) {
        
        std::string response = MCP_GetVehicleSpeed();
        if (speak_callback_) {
            speak_callback_(response);
        }
        return true;
    }
    return false;
}

bool VehicleAssistant::ParseFuelQuery(const std::string& command) const {
    if (command.find("xăng") != std::string::npos ||
        command.find("nhiên liệu") != std::string::npos ||
        command.find("fuel") != std::string::npos ||
        command.find("đi được bao xa") != std::string::npos) {
        
        std::string response = MCP_GetFuelInfo();
        if (speak_callback_) {
            speak_callback_(response);
        }
        return true;
    }
    return false;
}

bool VehicleAssistant::ParseTemperatureQuery(const std::string& command) const {
    if (command.find("nhiệt độ") != std::string::npos ||
        command.find("máy nóng") != std::string::npos ||
        command.find("temperature") != std::string::npos) {
        
        std::string response = MCP_GetEngineTemp();
        if (speak_callback_) {
            speak_callback_(response);
        }
        return true;
    }
    return false;
}

bool VehicleAssistant::ParseOdometerQuery(const std::string& command) const {
    if (command.find("odo") != std::string::npos ||
        command.find("km") != std::string::npos ||
        command.find("đi được") != std::string::npos ||
        command.find("quãng đường") != std::string::npos) {
        
        std::string response = MCP_GetOdometer();
        if (speak_callback_) {
            speak_callback_(response);
        }
        return true;
    }
    return false;
}

// ============================================================================
// Queries
// ============================================================================

std::string VehicleAssistant::GetVehicleStatusSummary() const {
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    const kia::VehicleData& data = protocol.GetVehicleData();
    
    char buffer[512];
    snprintf(buffer, sizeof(buffer),
             "Trạng thái xe: Tốc độ %.0f km/h, Xăng %.0f%%, Nhiệt độ máy %.0f°C, "
             "Điện bình %.1fV, Odo %lu km. %s",
             data.vehicle_speed, data.fuel_level_percent, data.coolant_temp,
             data.battery_voltage, data.odometer_km,
             data.check_engine ? "Lưu ý: Đèn check engine đang sáng." : "Xe hoạt động bình thường.");
    
    return std::string(buffer);
}

std::string VehicleAssistant::QueryVehicleInfo(const std::string& query) const {
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    if (lower_query.find("speed") != std::string::npos || lower_query.find("tốc độ") != std::string::npos) {
        return MCP_GetVehicleSpeed();
    }
    if (lower_query.find("fuel") != std::string::npos || lower_query.find("xăng") != std::string::npos) {
        return MCP_GetFuelInfo();
    }
    if (lower_query.find("temp") != std::string::npos || lower_query.find("nhiệt") != std::string::npos) {
        return MCP_GetEngineTemp();
    }
    if (lower_query.find("odo") != std::string::npos || lower_query.find("km") != std::string::npos) {
        return MCP_GetOdometer();
    }
    if (lower_query.find("battery") != std::string::npos || lower_query.find("điện") != std::string::npos) {
        return MCP_GetBatteryVoltage();
    }
    if (lower_query.find("warning") != std::string::npos || lower_query.find("cảnh báo") != std::string::npos) {
        return MCP_GetVehicleWarnings();
    }
    if (lower_query.find("health") != std::string::npos || lower_query.find("sức khỏe") != std::string::npos) {
        return MCP_GetVehicleHealthReport();
    }
    
    return GetVehicleStatusSummary();
}

// ============================================================================
// Highway Mode
// ============================================================================

void VehicleAssistant::SetHighwayMode(bool enable) {
    highway_mode_.store(enable);
    
    if (enable) {
        state_.store(AssistantState::HIGHWAY_MODE);
        last_speed_announce_ = esp_timer_get_time() / 1000;
        last_drive_time_check_ = last_speed_announce_;
        ESP_LOGI(TAG, "Highway mode ENABLED");
    } else {
        if (state_.load() == AssistantState::HIGHWAY_MODE) {
            state_.store(AssistantState::MONITORING);
        }
        ESP_LOGI(TAG, "Highway mode DISABLED");
    }
}

bool VehicleAssistant::IsHighwayMode() const {
    return highway_mode_.load();
}

// ============================================================================
// Scenarios
// ============================================================================

bool VehicleAssistant::TriggerScenario(const std::string& name) {
    for (const auto& scenario : scenarios_) {
        if (scenario.name == name && scenario.enabled) {
            ESP_LOGI(TAG, "Triggering scenario: %s", name.c_str());
            scenario.action();
            return true;
        }
    }
    
    ESP_LOGW(TAG, "Scenario not found: %s", name.c_str());
    return false;
}

void VehicleAssistant::RegisterScenario(const SmartScenario& scenario) {
    scenarios_.push_back(scenario);
    ESP_LOGI(TAG, "Scenario registered: %s", scenario.name.c_str());
}

// ============================================================================
// Maintenance
// ============================================================================

std::string VehicleAssistant::GetMaintenanceStatus() const {
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    const kia::VehicleData& data = protocol.GetVehicleData();
    
    std::string status;
    
    if (last_oil_change_km_ > 0) {
        uint32_t km_since_oil = data.odometer_km - last_oil_change_km_;
        if (km_since_oil >= MAINTENANCE_OIL_CHANGE_KM) {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "Đã đi %lu km từ lần thay dầu, nên thay dầu.", km_since_oil);
            status += buffer;
        }
    }
    
    if (last_tire_check_km_ > 0) {
        uint32_t km_since_tire = data.odometer_km - last_tire_check_km_;
        if (km_since_tire >= MAINTENANCE_TIRE_CHECK_KM) {
            if (!status.empty()) status += " ";
            status += "Nên kiểm tra lốp.";
        }
    }
    
    return status;
}

void VehicleAssistant::UpdateMaintenanceOdometer(const std::string& type) {
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    const kia::VehicleData& data = protocol.GetVehicleData();
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("vehicle", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS");
        return;
    }
    
    if (type == "oil") {
        last_oil_change_km_ = data.odometer_km;
        nvs_set_u32(nvs_handle, "oil_km", last_oil_change_km_);
        ESP_LOGI(TAG, "Oil change recorded at %lu km", last_oil_change_km_);
    } else if (type == "tire") {
        last_tire_check_km_ = data.odometer_km;
        nvs_set_u32(nvs_handle, "tire_km", last_tire_check_km_);
        ESP_LOGI(TAG, "Tire check recorded at %lu km", last_tire_check_km_);
    } else if (type == "major") {
        last_major_service_km_ = data.odometer_km;
        nvs_set_u32(nvs_handle, "major_km", last_major_service_km_);
        ESP_LOGI(TAG, "Major service recorded at %lu km", last_major_service_km_);
    }
    
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
}

AssistantState VehicleAssistant::GetState() const {
    return state_.load();
}

// ============================================================================
// MCP Tool Functions
// ============================================================================

std::string MCP_GetVehicleSpeed() {
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    const kia::VehicleData& data = protocol.GetVehicleData();
    
    char buffer[64];
    kia::FormatSpeedForVoice(data.vehicle_speed, buffer, sizeof(buffer));
    return std::string(buffer);
}

std::string MCP_GetFuelInfo() {
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    const kia::VehicleData& data = protocol.GetVehicleData();
    
    char buffer[128];
    kia::FormatFuelForVoice(data.fuel_level_percent, data.range_km, buffer, sizeof(buffer));
    return std::string(buffer);
}

std::string MCP_GetEngineTemp() {
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    const kia::VehicleData& data = protocol.GetVehicleData();
    
    char buffer[128];
    kia::FormatTempForVoice(data.coolant_temp, buffer, sizeof(buffer));
    return std::string(buffer);
}

std::string MCP_GetOdometer() {
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    const kia::VehicleData& data = protocol.GetVehicleData();
    
    char buffer[128];
    snprintf(buffer, sizeof(buffer), 
             "Xe đã đi được tổng cộng %lu km. Chuyến này đã đi %.1f km.",
             data.odometer_km, data.trip_km);
    return std::string(buffer);
}

std::string MCP_GetVehicleWarnings() {
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    const kia::VehicleData& data = protocol.GetVehicleData();
    
    std::string warnings;
    
    if (data.check_engine) warnings += "Đèn check engine đang sáng. ";
    if (data.low_fuel) warnings += "Xăng gần hết. ";
    if (data.low_oil) warnings += "Áp suất dầu thấp. ";
    if (data.battery_warning) warnings += "Điện bình yếu. ";
    if (data.abs_warning) warnings += "Lỗi hệ thống ABS. ";
    if (data.tpms_warning) warnings += "Áp suất lốp bất thường. ";
    if (data.airbag_warning) warnings += "Lỗi hệ thống túi khí. ";
    if (!data.seatbelt_driver) warnings += "Tài xế chưa thắt dây an toàn. ";
    if (data.parking_brake_on && data.vehicle_speed > 0) warnings += "Phanh tay vẫn kéo. ";
    
    if (warnings.empty()) {
        return "Không có cảnh báo nào. Xe hoạt động bình thường.";
    }
    
    return "Các cảnh báo: " + warnings;
}

std::string MCP_GetBatteryVoltage() {
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    const kia::VehicleData& data = protocol.GetVehicleData();
    
    char buffer[64];
    if (data.battery_voltage < VEHICLE_BATTERY_LOW_VOLTAGE) {
        snprintf(buffer, sizeof(buffer), 
                 "Điện bình là %.1f volt, hơi yếu. Nên kiểm tra.", data.battery_voltage);
    } else {
        snprintf(buffer, sizeof(buffer), 
                 "Điện bình %.1f volt, ở mức tốt.", data.battery_voltage);
    }
    return std::string(buffer);
}

std::string MCP_EnableHighwayMode() {
    VehicleAssistant::GetInstance().SetHighwayMode(true);
    return "Đã bật chế độ đường trường. Em sẽ đọc tốc độ định kỳ và nhắc bố nghỉ ngơi.";
}

std::string MCP_DisableHighwayMode() {
    VehicleAssistant::GetInstance().SetHighwayMode(false);
    return "Đã tắt chế độ đường trường.";
}

std::string MCP_GetDrivingTime() {
    kia::KiaCanProtocol& protocol = kia::KiaCanProtocol::GetInstance();
    int minutes = protocol.GetDrivingTimeMinutes();
    
    char buffer[64];
    if (minutes < 1) {
        snprintf(buffer, sizeof(buffer), "Bố vừa mới khởi động xe.");
    } else if (minutes < 60) {
        snprintf(buffer, sizeof(buffer), "Bố đã lái xe được %d phút.", minutes);
    } else {
        snprintf(buffer, sizeof(buffer), "Bố đã lái xe được %d tiếng %d phút.", 
                 minutes / 60, minutes % 60);
    }
    return std::string(buffer);
}

std::string MCP_GetVehicleHealthReport() {
    return VehicleAssistant::GetInstance().GetVehicleStatusSummary();
}

std::string MCP_TriggerPrepareHomeScenario() {
    if (VehicleAssistant::GetInstance().TriggerScenario("Bo chuan bi ve")) {
        return "Đã kích hoạt kịch bản 'Bố chuẩn bị về'.";
    }
    return "Không thể kích hoạt kịch bản.";
}

} // namespace vehicle
