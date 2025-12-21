/**
 * @file kia_can_protocol.cc
 * @brief Kia Morning 2017 Si CAN Bus Protocol Parser Implementation
 * @author Xiaozhi AI IoT Vietnam
 * @date 2024
 * 
 * Implementation of CAN message parsing for Kia Morning 2017 Si.
 * Converts raw CAN data into meaningful vehicle information.
 */

#include "kia_can_protocol.h"
#include "config.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <cstring>
#include <cstdio>

static const char* TAG = "Kia_CAN";

namespace kia {

// ============================================================================
// Singleton Instance
// ============================================================================

KiaCanProtocol& KiaCanProtocol::GetInstance() {
    static KiaCanProtocol instance;
    return instance;
}

// ============================================================================
// Constructor
// ============================================================================

KiaCanProtocol::KiaCanProtocol()
    : data_mutex_(nullptr)
    , callback_mutex_(nullptr)
    , engine_start_time_(0)
    , trip_start_odo_(0)
    , is_initialized_(false)
{
    memset(&vehicle_data_, 0, sizeof(vehicle_data_));
    vehicle_data_.ignition = IgnitionState::OFF;
    vehicle_data_.gear = GearPosition::UNKNOWN;
    vehicle_data_.climate_mode = ClimateMode::OFF;
    
    data_mutex_ = xSemaphoreCreateMutex();
    callback_mutex_ = xSemaphoreCreateMutex();
    
    if (!data_mutex_ || !callback_mutex_) {
        ESP_LOGE(TAG, "Failed to create mutexes");
    }
    
    ESP_LOGI(TAG, "Kia CAN Protocol parser created");
}

// ============================================================================
// Initialization
// ============================================================================

bool KiaCanProtocol::Initialize() {
    ESP_LOGI(TAG, "Initializing Kia CAN Protocol parser");
    
    if (is_initialized_) {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }
    
    // Reset vehicle data
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        memset(&vehicle_data_, 0, sizeof(vehicle_data_));
        vehicle_data_.ignition = IgnitionState::OFF;
        vehicle_data_.gear = GearPosition::PARK;
        vehicle_data_.data_valid = false;
        xSemaphoreGive(data_mutex_);
    }
    
    is_initialized_ = true;
    ESP_LOGI(TAG, "Kia CAN Protocol parser initialized");
    
    return true;
}

// ============================================================================
// Message Processing
// ============================================================================

void KiaCanProtocol::ProcessMessage(const canbus::CanMessage& msg) {
    if (!is_initialized_) return;
    
    ESP_LOGD(TAG, "Processing CAN ID: 0x%03lX", msg.id);
    
    switch (msg.id) {
        case CAN_ID_ENGINE_DATA_1:
            ParseEngineData1(msg);
            break;
        case CAN_ID_ENGINE_DATA_2:
            ParseEngineData2(msg);
            break;
        case CAN_ID_VEHICLE_SPEED:
            ParseVehicleSpeed(msg);
            break;
        case CAN_ID_ODOMETER:
            ParseOdometer(msg);
            break;
        case CAN_ID_DOORS:
            ParseDoors(msg);
            break;
        case CAN_ID_SEATBELT:
            ParseSeatbelt(msg);
            break;
        case CAN_ID_PARKING_BRAKE:
            ParseParkingBrake(msg);
            break;
        case CAN_ID_LIGHTS:
            ParseLights(msg);
            break;
        case CAN_ID_BATTERY:
            ParseBattery(msg);
            break;
        case CAN_ID_IGNITION:
            ParseIgnition(msg);
            break;
        case CAN_ID_FUEL:
            ParseFuel(msg);
            break;
        case CAN_ID_CLIMATE:
            ParseClimate(msg);
            break;
        case CAN_ID_CLUSTER_1:
            ParseCluster1(msg);
            break;
        case CAN_ID_TRANSMISSION:
            ParseTransmission(msg);
            break;
        default:
            // Unknown CAN ID - log for debugging new messages
            ESP_LOGD(TAG, "Unknown CAN ID 0x%03lX: [%02X %02X %02X %02X %02X %02X %02X %02X]",
                     msg.id, msg.data[0], msg.data[1], msg.data[2], msg.data[3],
                     msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
            break;
    }
    
    // Update timestamp and validity
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
        vehicle_data_.last_update_ms = esp_timer_get_time() / 1000;
        vehicle_data_.data_valid = true;
        xSemaphoreGive(data_mutex_);
    }
    
    // Check for alert conditions
    CheckForAlerts();
    
    // Notify callbacks
    NotifyDataCallbacks();
}

// ============================================================================
// Parsing Methods
// ============================================================================

void KiaCanProtocol::ParseEngineData1(const canbus::CanMessage& msg) {
    if (msg.length < 4) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        // RPM: typically bytes 0-1, scaled by 0.25 or similar
        uint16_t raw_rpm = (msg.data[0] << 8) | msg.data[1];
        vehicle_data_.engine_rpm = raw_rpm * 0.25f;
        
        // Throttle position: typically byte 2, 0-100%
        vehicle_data_.throttle_position = msg.data[2] * 100.0f / 255.0f;
        
        ESP_LOGD(TAG, "Engine: RPM=%.0f, Throttle=%.1f%%", 
                 vehicle_data_.engine_rpm, vehicle_data_.throttle_position);
        
        xSemaphoreGive(data_mutex_);
    }
}

void KiaCanProtocol::ParseEngineData2(const canbus::CanMessage& msg) {
    if (msg.length < 4) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        // Coolant temperature: typically byte 0, offset by -40
        vehicle_data_.coolant_temp = (float)msg.data[0] - 40.0f;
        
        // Oil temperature: typically byte 1, offset by -40
        vehicle_data_.oil_temp = (float)msg.data[1] - 40.0f;
        
        ESP_LOGD(TAG, "Engine Temp: Coolant=%.1f°C, Oil=%.1f°C", 
                 vehicle_data_.coolant_temp, vehicle_data_.oil_temp);
        
        xSemaphoreGive(data_mutex_);
    }
}

void KiaCanProtocol::ParseVehicleSpeed(const canbus::CanMessage& msg) {
    if (msg.length < 2) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        // Speed: typically bytes 0-1, may need scaling
        uint16_t raw_speed = (msg.data[0] << 8) | msg.data[1];
        vehicle_data_.vehicle_speed = raw_speed * 0.01f;  // Adjust scaling as needed
        
        ESP_LOGD(TAG, "Speed: %.1f km/h", vehicle_data_.vehicle_speed);
        
        xSemaphoreGive(data_mutex_);
    }
}

void KiaCanProtocol::ParseOdometer(const canbus::CanMessage& msg) {
    if (msg.length < 4) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        // Odometer: typically bytes 0-3, big endian
        vehicle_data_.odometer_km = 
            (msg.data[0] << 24) | (msg.data[1] << 16) | 
            (msg.data[2] << 8) | msg.data[3];
        
        // Calculate trip distance if trip start was recorded
        if (trip_start_odo_ > 0) {
            vehicle_data_.trip_km = vehicle_data_.odometer_km - trip_start_odo_;
        }
        
        ESP_LOGD(TAG, "Odometer: %lu km, Trip: %.1f km", 
                 vehicle_data_.odometer_km, vehicle_data_.trip_km);
        
        xSemaphoreGive(data_mutex_);
    }
}

void KiaCanProtocol::ParseDoors(const canbus::CanMessage& msg) {
    if (msg.length < 1) return;
    
    DoorStatus old_status;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        old_status = vehicle_data_.doors;
        
        // Parse door bits
        uint8_t door_byte = msg.data[0];
        vehicle_data_.doors.driver_door_open = (door_byte & 0x01) != 0;
        vehicle_data_.doors.passenger_door_open = (door_byte & 0x02) != 0;
        vehicle_data_.doors.rear_left_open = (door_byte & 0x04) != 0;
        vehicle_data_.doors.rear_right_open = (door_byte & 0x08) != 0;
        vehicle_data_.doors.trunk_open = (door_byte & 0x10) != 0;
        vehicle_data_.doors.hood_open = (door_byte & 0x20) != 0;
        
        if (msg.length >= 2) {
            vehicle_data_.doors.any_door_unlocked = (msg.data[1] & 0x01) != 0;
        }
        
        // Update door ajar warning
        vehicle_data_.door_ajar = 
            vehicle_data_.doors.driver_door_open ||
            vehicle_data_.doors.passenger_door_open ||
            vehicle_data_.doors.rear_left_open ||
            vehicle_data_.doors.rear_right_open ||
            vehicle_data_.doors.trunk_open ||
            vehicle_data_.doors.hood_open;
        
        ESP_LOGD(TAG, "Doors: Driver=%d, Pass=%d, Trunk=%d", 
                 vehicle_data_.doors.driver_door_open,
                 vehicle_data_.doors.passenger_door_open,
                 vehicle_data_.doors.trunk_open);
        
        xSemaphoreGive(data_mutex_);
    }
    
    // Notify door callbacks if status changed
    NotifyDoorCallbacks(old_status);
}

void KiaCanProtocol::ParseSeatbelt(const canbus::CanMessage& msg) {
    if (msg.length < 1) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        vehicle_data_.seatbelt_driver = (msg.data[0] & 0x01) != 0;
        vehicle_data_.seatbelt_passenger = (msg.data[0] & 0x02) != 0;
        
        ESP_LOGD(TAG, "Seatbelt: Driver=%d, Passenger=%d", 
                 vehicle_data_.seatbelt_driver, vehicle_data_.seatbelt_passenger);
        
        xSemaphoreGive(data_mutex_);
    }
}

void KiaCanProtocol::ParseParkingBrake(const canbus::CanMessage& msg) {
    if (msg.length < 1) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        vehicle_data_.parking_brake_on = (msg.data[0] & 0x01) != 0;
        
        ESP_LOGD(TAG, "Parking Brake: %s", 
                 vehicle_data_.parking_brake_on ? "ON" : "OFF");
        
        xSemaphoreGive(data_mutex_);
    }
}

void KiaCanProtocol::ParseLights(const canbus::CanMessage& msg) {
    if (msg.length < 1) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        uint8_t light_byte = msg.data[0];
        vehicle_data_.lights.headlights_on = (light_byte & 0x01) != 0;
        vehicle_data_.lights.high_beam_on = (light_byte & 0x02) != 0;
        vehicle_data_.lights.fog_lights_on = (light_byte & 0x04) != 0;
        vehicle_data_.lights.parking_lights_on = (light_byte & 0x08) != 0;
        vehicle_data_.lights.turn_left_on = (light_byte & 0x10) != 0;
        vehicle_data_.lights.turn_right_on = (light_byte & 0x20) != 0;
        vehicle_data_.lights.hazard_on = (light_byte & 0x40) != 0;
        
        if (msg.length >= 2) {
            vehicle_data_.lights.interior_light_on = (msg.data[1] & 0x01) != 0;
        }
        
        ESP_LOGD(TAG, "Lights: Head=%d, High=%d, Hazard=%d", 
                 vehicle_data_.lights.headlights_on,
                 vehicle_data_.lights.high_beam_on,
                 vehicle_data_.lights.hazard_on);
        
        xSemaphoreGive(data_mutex_);
    }
}

void KiaCanProtocol::ParseBattery(const canbus::CanMessage& msg) {
    if (msg.length < 2) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        // Battery voltage: typically bytes 0-1, scaled
        uint16_t raw_voltage = (msg.data[0] << 8) | msg.data[1];
        vehicle_data_.battery_voltage = raw_voltage * 0.01f;  // Adjust scaling
        
        // Check battery warning threshold
        vehicle_data_.battery_warning = 
            vehicle_data_.battery_voltage < VEHICLE_BATTERY_LOW_VOLTAGE;
        
        ESP_LOGD(TAG, "Battery: %.2fV (Warning=%d)", 
                 vehicle_data_.battery_voltage, vehicle_data_.battery_warning);
        
        xSemaphoreGive(data_mutex_);
    }
}

void KiaCanProtocol::ParseIgnition(const canbus::CanMessage& msg) {
    if (msg.length < 1) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        IgnitionState old_state = vehicle_data_.ignition;
        
        uint8_t ign_byte = msg.data[0] & 0x03;
        vehicle_data_.ignition = static_cast<IgnitionState>(ign_byte);
        
        // Track engine start time
        if (old_state != IgnitionState::ON && vehicle_data_.ignition == IgnitionState::ON) {
            engine_start_time_ = esp_timer_get_time() / 1000;
            if (vehicle_data_.odometer_km > 0 && trip_start_odo_ == 0) {
                trip_start_odo_ = vehicle_data_.odometer_km;
            }
            ESP_LOGI(TAG, "Engine started");
        } else if (old_state == IgnitionState::ON && vehicle_data_.ignition == IgnitionState::OFF) {
            engine_start_time_ = 0;
            ESP_LOGI(TAG, "Engine stopped");
        }
        
        ESP_LOGD(TAG, "Ignition: %s", IgnitionToString(vehicle_data_.ignition));
        
        xSemaphoreGive(data_mutex_);
    }
}

void KiaCanProtocol::ParseFuel(const canbus::CanMessage& msg) {
    if (msg.length < 2) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        // Fuel level: typically byte 0, 0-100%
        vehicle_data_.fuel_level_percent = msg.data[0] * 100.0f / 255.0f;
        
        // Fuel consumption: typically byte 1
        if (msg.length >= 4) {
            uint16_t raw_consumption = (msg.data[2] << 8) | msg.data[3];
            vehicle_data_.fuel_consumption = raw_consumption * 0.01f;
        }
        
        // Low fuel warning (below 10%)
        vehicle_data_.low_fuel = vehicle_data_.fuel_level_percent < 10.0f;
        
        // Estimate range (assuming ~35L tank and avg 7L/100km)
        float fuel_liters = vehicle_data_.fuel_level_percent * 0.35f;  // 35L tank
        if (vehicle_data_.fuel_consumption > 0) {
            vehicle_data_.range_km = fuel_liters / vehicle_data_.fuel_consumption * 100.0f;
        } else {
            vehicle_data_.range_km = fuel_liters / 7.0f * 100.0f;  // Default 7L/100km
        }
        
        ESP_LOGD(TAG, "Fuel: %.1f%%, Consumption: %.1f L/100km, Range: %.0f km", 
                 vehicle_data_.fuel_level_percent, 
                 vehicle_data_.fuel_consumption,
                 vehicle_data_.range_km);
        
        xSemaphoreGive(data_mutex_);
    }
}

void KiaCanProtocol::ParseClimate(const canbus::CanMessage& msg) {
    if (msg.length < 4) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        vehicle_data_.ac_on = (msg.data[0] & 0x01) != 0;
        vehicle_data_.fan_speed = msg.data[1] & 0x0F;
        vehicle_data_.set_temp = msg.data[2] * 0.5f + 15.0f;  // 15-30°C range
        vehicle_data_.climate_mode = static_cast<ClimateMode>(msg.data[3] & 0x07);
        
        ESP_LOGD(TAG, "Climate: AC=%d, Fan=%d, SetTemp=%.1f°C", 
                 vehicle_data_.ac_on, vehicle_data_.fan_speed, vehicle_data_.set_temp);
        
        xSemaphoreGive(data_mutex_);
    }
}

void KiaCanProtocol::ParseCluster1(const canbus::CanMessage& msg) {
    if (msg.length < 2) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        // Warning lights from instrument cluster
        uint8_t warn1 = msg.data[0];
        uint8_t warn2 = msg.data[1];
        
        vehicle_data_.check_engine = (warn1 & 0x01) != 0;
        vehicle_data_.low_oil = (warn1 & 0x02) != 0;
        vehicle_data_.airbag_warning = (warn1 & 0x04) != 0;
        vehicle_data_.abs_warning = (warn1 & 0x08) != 0;
        vehicle_data_.tpms_warning = (warn2 & 0x01) != 0;
        
        ESP_LOGD(TAG, "Warnings: CheckEngine=%d, Oil=%d, ABS=%d, TPMS=%d", 
                 vehicle_data_.check_engine, vehicle_data_.low_oil,
                 vehicle_data_.abs_warning, vehicle_data_.tpms_warning);
        
        xSemaphoreGive(data_mutex_);
    }
}

void KiaCanProtocol::ParseTransmission(const canbus::CanMessage& msg) {
    if (msg.length < 1) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        uint8_t gear_byte = msg.data[0] & 0x0F;
        
        switch (gear_byte) {
            case 0: vehicle_data_.gear = GearPosition::PARK; break;
            case 1: vehicle_data_.gear = GearPosition::REVERSE; break;
            case 2: vehicle_data_.gear = GearPosition::NEUTRAL; break;
            case 3: vehicle_data_.gear = GearPosition::DRIVE; break;
            case 4: vehicle_data_.gear = GearPosition::SPORT; break;
            case 5: vehicle_data_.gear = GearPosition::LOW; break;
            default: vehicle_data_.gear = GearPosition::UNKNOWN; break;
        }
        
        ESP_LOGD(TAG, "Gear: %s", GearToString(vehicle_data_.gear));
        
        xSemaphoreGive(data_mutex_);
    }
}

// ============================================================================
// Alert Checking
// ============================================================================

void KiaCanProtocol::CheckForAlerts() {
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) != pdTRUE) return;
    
    if (xSemaphoreTake(callback_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        // Check for critical alerts
        
        // Battery low
        if (vehicle_data_.battery_voltage < VEHICLE_BATTERY_CRITICAL_VOLTAGE &&
            vehicle_data_.battery_voltage > 0) {
            for (auto& callback : alert_callbacks_) {
                callback("Bố ơi, điện bình rất yếu! Cần kiểm tra ngay!", 1);
            }
        } else if (vehicle_data_.battery_voltage < VEHICLE_BATTERY_LOW_VOLTAGE &&
                   vehicle_data_.battery_voltage > 0) {
            for (auto& callback : alert_callbacks_) {
                callback("Bố ơi, điện bình hơi yếu, bố nên kiểm tra để tránh khó đề máy.", 2);
            }
        }
        
        // Engine overheating
        if (vehicle_data_.coolant_temp > VEHICLE_COOLANT_CRITICAL_TEMP) {
            for (auto& callback : alert_callbacks_) {
                callback("CẢNH BÁO KHẨN CẤP! Nhiệt độ nước làm mát quá cao! Dừng xe ngay!", 0);
            }
        } else if (vehicle_data_.coolant_temp > VEHICLE_COOLANT_WARN_TEMP) {
            for (auto& callback : alert_callbacks_) {
                callback("Bố ơi, nhiệt độ máy đang cao, bố nên giảm tốc độ.", 1);
            }
        }
        
        // Parking brake warning while moving
        if (vehicle_data_.parking_brake_on && vehicle_data_.vehicle_speed > 5) {
            for (auto& callback : alert_callbacks_) {
                callback("Bố ơi, phanh tay vẫn đang kéo! Hãy hạ phanh tay nhé!", 1);
            }
        }
        
        // Seatbelt warning while moving
        if (!vehicle_data_.seatbelt_driver && vehicle_data_.vehicle_speed > 10) {
            for (auto& callback : alert_callbacks_) {
                callback("Bố ơi, bố chưa thắt dây an toàn!", 1);
            }
        }
        
        // Low fuel
        if (vehicle_data_.low_fuel && vehicle_data_.ignition == IgnitionState::ON) {
            for (auto& callback : alert_callbacks_) {
                callback("Bố ơi, xăng sắp hết rồi. Nên đổ thêm nhé!", 2);
            }
        }
        
        xSemaphoreGive(callback_mutex_);
    }
    
    xSemaphoreGive(data_mutex_);
}

// ============================================================================
// Callback Notifications
// ============================================================================

void KiaCanProtocol::NotifyDataCallbacks() {
    if (xSemaphoreTake(callback_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        for (auto& callback : data_callbacks_) {
            callback(vehicle_data_);
        }
        xSemaphoreGive(callback_mutex_);
    }
}

void KiaCanProtocol::NotifyDoorCallbacks(const DoorStatus& old_status) {
    bool changed = memcmp(&old_status, &vehicle_data_.doors, sizeof(DoorStatus)) != 0;
    
    if (changed && xSemaphoreTake(callback_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        for (auto& callback : door_callbacks_) {
            callback(old_status, vehicle_data_.doors);
        }
        xSemaphoreGive(callback_mutex_);
    }
}

// ============================================================================
// Getters
// ============================================================================

const VehicleData& KiaCanProtocol::GetVehicleData() const {
    return vehicle_data_;
}

bool KiaCanProtocol::IsDataValid() const {
    return vehicle_data_.data_valid;
}

int64_t KiaCanProtocol::GetTimeSinceLastData() const {
    if (vehicle_data_.last_update_ms == 0) return -1;
    return (esp_timer_get_time() / 1000) - vehicle_data_.last_update_ms;
}

float KiaCanProtocol::GetEstimatedRange() const {
    return vehicle_data_.range_km;
}

int KiaCanProtocol::GetDrivingTimeMinutes() const {
    if (engine_start_time_ == 0) return 0;
    int64_t now = esp_timer_get_time() / 1000;
    return (now - engine_start_time_) / 60000;
}

void KiaCanProtocol::ResetTrip() {
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        trip_start_odo_ = vehicle_data_.odometer_km;
        vehicle_data_.trip_km = 0;
        ESP_LOGI(TAG, "Trip reset at %lu km", vehicle_data_.odometer_km);
        xSemaphoreGive(data_mutex_);
    }
}

// ============================================================================
// Callback Registration
// ============================================================================

void KiaCanProtocol::RegisterDataCallback(VehicleDataCallback callback) {
    if (!callback) return;
    
    if (xSemaphoreTake(callback_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        data_callbacks_.push_back(callback);
        ESP_LOGI(TAG, "Data callback registered");
        xSemaphoreGive(callback_mutex_);
    }
}

void KiaCanProtocol::RegisterDoorCallback(DoorEventCallback callback) {
    if (!callback) return;
    
    if (xSemaphoreTake(callback_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        door_callbacks_.push_back(callback);
        ESP_LOGI(TAG, "Door callback registered");
        xSemaphoreGive(callback_mutex_);
    }
}

void KiaCanProtocol::RegisterAlertCallback(AlertCallback callback) {
    if (!callback) return;
    
    if (xSemaphoreTake(callback_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        alert_callbacks_.push_back(callback);
        ESP_LOGI(TAG, "Alert callback registered");
        xSemaphoreGive(callback_mutex_);
    }
}

void KiaCanProtocol::ClearCallbacks() {
    if (xSemaphoreTake(callback_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        data_callbacks_.clear();
        door_callbacks_.clear();
        alert_callbacks_.clear();
        ESP_LOGI(TAG, "All callbacks cleared");
        xSemaphoreGive(callback_mutex_);
    }
}

// ============================================================================
// OBD-II Support
// ============================================================================

bool KiaCanProtocol::RequestOBDPid(uint8_t pid) {
    // OBD-II request format: ID 0x7DF, data [02 01 PID 00 00 00 00 00]
    uint8_t data[8] = {0x02, 0x01, pid, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    canbus::CanBusDriver& can = canbus::CanBusDriver::GetInstance();
    return can.SendMessage(0x7DF, data, 8, 100);
}

// ============================================================================
// Helper Functions
// ============================================================================

const char* GearToString(GearPosition gear) {
    switch (gear) {
        case GearPosition::PARK:    return "P";
        case GearPosition::REVERSE: return "R";
        case GearPosition::NEUTRAL: return "N";
        case GearPosition::DRIVE:   return "D";
        case GearPosition::SPORT:   return "S";
        case GearPosition::LOW:     return "L";
        default:                    return "?";
    }
}

const char* IgnitionToString(IgnitionState state) {
    switch (state) {
        case IgnitionState::OFF:   return "OFF";
        case IgnitionState::ACC:   return "ACC";
        case IgnitionState::ON:    return "ON";
        case IgnitionState::START: return "START";
        default:                   return "UNKNOWN";
    }
}

void FormatSpeedForVoice(float speed_kmh, char* buffer, size_t buffer_size) {
    if (speed_kmh < 1) {
        snprintf(buffer, buffer_size, "Xe đang đứng yên");
    } else {
        snprintf(buffer, buffer_size, "Tốc độ hiện tại là %.0f km/h", speed_kmh);
    }
}

void FormatFuelForVoice(float fuel_percent, float range_km, char* buffer, size_t buffer_size) {
    if (fuel_percent < 10) {
        snprintf(buffer, buffer_size, "Xăng chỉ còn %.0f phần trăm, còn đi được khoảng %.0f km. Bố nên đổ xăng sớm nhé!", 
                 fuel_percent, range_km);
    } else if (fuel_percent < 25) {
        snprintf(buffer, buffer_size, "Xăng còn %.0f phần trăm, đi được khoảng %.0f km nữa", 
                 fuel_percent, range_km);
    } else {
        snprintf(buffer, buffer_size, "Xăng còn %.0f phần trăm, đủ đi khoảng %.0f km", 
                 fuel_percent, range_km);
    }
}

void FormatTempForVoice(float temp_celsius, char* buffer, size_t buffer_size) {
    if (temp_celsius > VEHICLE_COOLANT_CRITICAL_TEMP) {
        snprintf(buffer, buffer_size, "CẢNH BÁO! Nhiệt độ nước làm mát là %.0f độ, quá cao!", temp_celsius);
    } else if (temp_celsius > VEHICLE_COOLANT_WARN_TEMP) {
        snprintf(buffer, buffer_size, "Nhiệt độ nước làm mát là %.0f độ, đang hơi cao", temp_celsius);
    } else if (temp_celsius > 70) {
        snprintf(buffer, buffer_size, "Nhiệt độ máy bình thường, %.0f độ", temp_celsius);
    } else {
        snprintf(buffer, buffer_size, "Nhiệt độ máy là %.0f độ, máy chưa ấm hẳn", temp_celsius);
    }
}

} // namespace kia
