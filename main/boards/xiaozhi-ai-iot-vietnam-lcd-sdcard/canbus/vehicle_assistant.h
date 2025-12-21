#ifndef _VEHICLE_ASSISTANT_H_
#define _VEHICLE_ASSISTANT_H_

/**
 * @file vehicle_assistant.h
 * @brief Vehicle Assistant for Kia Morning 2017 Si - Voice AI Integration
 * @author Xiaozhi AI IoT Vietnam
 * @date 2024
 * 
 * This module provides intelligent vehicle assistant features:
 * - Greeting and personalization (welcome messages when entering vehicle)
 * - Safety warnings (seatbelt, parking brake, battery, temperature)
 * - Vehicle information queries (fuel, speed, odometer)
 * - Smart scenarios (highway mode, long drive reminders)
 * - MCP integration for voice commands
 */

#include "canbus_driver.h"
#include "kia_can_protocol.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <functional>
#include <string>
#include <atomic>

namespace vehicle {

// ============================================================================
// Assistant Configuration
// ============================================================================

// Alert intervals (to avoid spamming same warnings)
constexpr int64_t ALERT_COOLDOWN_MS = 30000;        // 30 seconds between same alert
constexpr int64_t SPEED_ANNOUNCE_INTERVAL_MS = 300000; // 5 minutes for speed announcements
constexpr int64_t DRIVE_TIME_CHECK_INTERVAL_MS = 600000; // 10 minutes for drive time checks

// ============================================================================
// Callback Types
// ============================================================================

// Callback for speaking messages (TTS)
using SpeakCallback = std::function<void(const std::string& message)>;

// Callback for starting voice listening mode
using ListenCallback = std::function<void()>;

// Callback for displaying text on LCD
using DisplayCallback = std::function<void(const std::string& text, int line)>;

// Callback for playing sounds (alerts, music)
using SoundCallback = std::function<void(const std::string& sound_name)>;

// ============================================================================
// Assistant States
// ============================================================================

enum class AssistantState {
    IDLE,               // Waiting for vehicle entry
    GREETING,           // Saying welcome message
    LISTENING,          // Listening for commands
    PROCESSING,         // Processing command
    SPEAKING,           // Speaking response
    MONITORING,         // Passive monitoring (during drive)
    HIGHWAY_MODE,       // Highway driving mode with periodic updates
    POWER_SAVE          // Power saving mode (vehicle off)
};

// ============================================================================
// Alert Types
// ============================================================================

enum class AlertType {
    SEATBELT,
    PARKING_BRAKE,
    DOOR_OPEN,
    LOW_BATTERY,
    CRITICAL_BATTERY,
    ENGINE_OVERHEAT,
    CRITICAL_OVERHEAT,
    LOW_FUEL,
    LONG_DRIVE,
    SPEED_WARNING,
    LIGHTS_ON_ENGINE_OFF,
    MAINTENANCE_DUE
};

// ============================================================================
// Smart Scenario Definitions
// ============================================================================

struct SmartScenario {
    std::string name;
    std::string trigger_phrase;     // Voice command to trigger
    std::function<void()> action;   // Action to perform
    bool enabled;
};

// ============================================================================
// Vehicle Assistant Class
// ============================================================================

/**
 * @class VehicleAssistant
 * @brief Main vehicle assistant controller
 * 
 * Integrates CAN bus data with voice AI to provide intelligent
 * vehicle assistant features.
 */
class VehicleAssistant {
public:
    /**
     * @brief Get singleton instance
     */
    static VehicleAssistant& GetInstance();

    /**
     * @brief Initialize the vehicle assistant
     * @return true if initialized successfully
     */
    bool Initialize();

    /**
     * @brief Start the vehicle assistant
     * @return true if started successfully
     */
    bool Start();

    /**
     * @brief Stop the vehicle assistant
     */
    void Stop();

    /**
     * @brief Set callback for speaking (TTS)
     * @param callback Function to call when assistant wants to speak
     */
    void SetSpeakCallback(SpeakCallback callback);

    /**
     * @brief Set callback for listening mode
     * @param callback Function to call when assistant should listen
     */
    void SetListenCallback(ListenCallback callback);

    /**
     * @brief Set callback for display updates
     * @param callback Function to call to update LCD
     */
    void SetDisplayCallback(DisplayCallback callback);

    /**
     * @brief Set callback for sound effects
     * @param callback Function to call to play sounds
     */
    void SetSoundCallback(SoundCallback callback);

    /**
     * @brief Get current assistant state
     */
    AssistantState GetState() const;

    /**
     * @brief Process voice command (called from chat module)
     * @param command The voice command text
     * @return true if command was handled, false to pass to chatbot
     */
    bool ProcessVoiceCommand(const std::string& command);

    /**
     * @brief Get vehicle status summary for chatbot context
     * @return Status summary string
     */
    std::string GetVehicleStatusSummary() const;

    /**
     * @brief Get specific vehicle info (for MCP queries)
     * @param query The info requested (speed, fuel, temp, etc.)
     * @return Response string
     */
    std::string QueryVehicleInfo(const std::string& query) const;

    /**
     * @brief Enable/disable highway mode
     * @param enable true to enable
     */
    void SetHighwayMode(bool enable);

    /**
     * @brief Check if highway mode is active
     */
    bool IsHighwayMode() const;

    /**
     * @brief Trigger a smart scenario by name
     * @param name Scenario name
     * @return true if scenario found and triggered
     */
    bool TriggerScenario(const std::string& name);

    /**
     * @brief Register a custom smart scenario
     * @param scenario The scenario definition
     */
    void RegisterScenario(const SmartScenario& scenario);

    /**
     * @brief Manual trigger for door open event (for testing)
     */
    void OnDriverDoorOpened();

    /**
     * @brief Manual trigger for ignition on event
     */
    void OnIgnitionOn();

    /**
     * @brief Manual trigger for ignition off event
     */
    void OnIgnitionOff();

    /**
     * @brief Get maintenance status string
     */
    std::string GetMaintenanceStatus() const;

    /**
     * @brief Update last maintenance odometer
     * @param type Maintenance type (oil, tire, major)
     */
    void UpdateMaintenanceOdometer(const std::string& type);

private:
    VehicleAssistant();
    ~VehicleAssistant();

    VehicleAssistant(const VehicleAssistant&) = delete;
    VehicleAssistant& operator=(const VehicleAssistant&) = delete;

    // CAN bus data handlers
    void OnCanMessage(const canbus::CanMessage& msg);
    void OnVehicleDataUpdate(const kia::VehicleData& data);
    void OnDoorEvent(const kia::DoorStatus& old_status, const kia::DoorStatus& new_status);
    void OnAlert(const char* message, int priority);

    // Assistant logic
    void MonitoringTask();
    void PerformGreeting();
    void CheckSafetyConditions();
    void CheckDriveTime();
    void AnnounceSpeed();
    void UpdateDisplayStatus();

    // Alert management
    bool ShouldSendAlert(AlertType type);
    void RecordAlertSent(AlertType type);
    void Speak(const std::string& message);
    void PlaySound(const std::string& sound);

    // Command parsing
    bool ParseSpeedQuery(const std::string& command) const;
    bool ParseFuelQuery(const std::string& command) const;
    bool ParseTemperatureQuery(const std::string& command) const;
    bool ParseOdometerQuery(const std::string& command) const;

    // Static task wrapper
    static void MonitoringTaskWrapper(void* arg);

    // Member variables
    std::atomic<AssistantState> state_;
    std::atomic<bool> is_running_;
    std::atomic<bool> highway_mode_;

    TaskHandle_t monitoring_task_handle_;
    SemaphoreHandle_t data_mutex_;

    SpeakCallback speak_callback_;
    ListenCallback listen_callback_;
    DisplayCallback display_callback_;
    SoundCallback sound_callback_;

    std::vector<SmartScenario> scenarios_;

    // Alert cooldown tracking
    std::map<AlertType, int64_t> last_alert_time_;

    // Driving tracking
    int64_t last_speed_announce_;
    int64_t last_drive_time_check_;
    bool greeting_done_;

    // Maintenance tracking (stored in NVS)
    uint32_t last_oil_change_km_;
    uint32_t last_tire_check_km_;
    uint32_t last_major_service_km_;

    bool is_initialized_;
};

// ============================================================================
// MCP Tool Functions (for voice command integration)
// ============================================================================

/**
 * @brief MCP tool: Get vehicle speed
 */
std::string MCP_GetVehicleSpeed();

/**
 * @brief MCP tool: Get fuel level and range
 */
std::string MCP_GetFuelInfo();

/**
 * @brief MCP tool: Get engine temperature
 */
std::string MCP_GetEngineTemp();

/**
 * @brief MCP tool: Get odometer reading
 */
std::string MCP_GetOdometer();

/**
 * @brief MCP tool: Get all vehicle warnings
 */
std::string MCP_GetVehicleWarnings();

/**
 * @brief MCP tool: Get battery voltage
 */
std::string MCP_GetBatteryVoltage();

/**
 * @brief MCP tool: Enable highway mode
 */
std::string MCP_EnableHighwayMode();

/**
 * @brief MCP tool: Disable highway mode
 */
std::string MCP_DisableHighwayMode();

/**
 * @brief MCP tool: Get driving time
 */
std::string MCP_GetDrivingTime();

/**
 * @brief MCP tool: Get complete vehicle health report
 */
std::string MCP_GetVehicleHealthReport();

/**
 * @brief MCP tool: Trigger "Bo chuan bi ve" scenario
 */
std::string MCP_TriggerPrepareHomeScenario();

} // namespace vehicle

#endif // _VEHICLE_ASSISTANT_H_
