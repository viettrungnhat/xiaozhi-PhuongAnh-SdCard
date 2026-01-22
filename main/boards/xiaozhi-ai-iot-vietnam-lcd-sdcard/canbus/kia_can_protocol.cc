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
#include "../config.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <cstring>
#include <cstdio>
#include <cmath>

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
    
    // Diagnostic logging disabled - uncomment to debug
    // if (msg.id == 0x316 || msg.id == 0x4B0 || msg.id == 0x610 || msg.id == 0x43F) {
    //     ESP_LOGD(TAG, "📨 CAN 0x%03lX RAW [%02X %02X %02X %02X %02X %02X %02X %02X]",
    //              msg.id, msg.data[0], msg.data[1], msg.data[2], msg.data[3],
    //              msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
    // }
    
    switch (msg.id) {
        case CAN_ID_ENGINE_DATA_1:  // 0x316 - Consolidated engine data
            ParseEngineData1(msg);
            break;
        case CAN_ID_ENGINE_TEMPS:   // 0x316 (same ID, already handled)
            // Data handled by ParseEngineData1
            break;
        case CAN_ID_VEHICLE_SPEED:
            ParseVehicleSpeed(msg);
            break;
        case CAN_ID_ODOMETER:
            ParseOdometer(msg);
            break;
        case CAN_ID_DOORS_BRAKE:    // 0x15F
            ParseDoors(msg);
            ParseParkingBrake(msg);
            break;
        case CAN_ID_SEATBELT:       // 0x0A1
            ParseSeatbelt(msg);
            break;
        case CAN_ID_LIGHTS_WIPER:   // 0x680
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
    // CAN_ID_ENGINE_DATA_1 = 0x316 (verified on actual Kia Morning 2017)
    // Byte 0: Counter (0-15, ignore)
    // Byte 1: Coolant temp = B1 - 40°C
    // Byte 2-3: RPM = (B3<<8 | B2) / 4
    // Byte 4: Throttle position (0-255 → 0-100%)
    
    if (msg.length < 5) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        // Coolant: byte 1 (offset -40)
        float new_coolant = (float)msg.data[1] - 40.0f;
        
        // RPM: bytes 2-3 (big endian), scale /4
        uint16_t raw_rpm = (msg.data[3] << 8) | msg.data[2];
        float new_rpm = raw_rpm / 4.0f;
        
        // Throttle position: byte 4, 0-255 → 0-100%
        float new_throttle = msg.data[4] * 100.0f / 255.0f;
        
        // Validate: reject impossible RPM
        if (new_rpm > 8000.0f) {
            ESP_LOGW(TAG, "⚠️ Invalid RPM: %.0f (raw=0x%04X)", new_rpm, raw_rpm);
            xSemaphoreGive(data_mutex_);
            return;
        }
        
        // Log ONLY if values changed significantly (reduce spam)
        bool rpm_changed = (fabs(vehicle_data_.engine_rpm - new_rpm) >= 10.0f);
        bool coolant_changed = (fabs(vehicle_data_.coolant_temp - new_coolant) >= 2.0f);
        
        vehicle_data_.engine_rpm = new_rpm;
        vehicle_data_.coolant_temp = new_coolant;
        vehicle_data_.throttle_position = new_throttle;
        
        if (rpm_changed || coolant_changed) {
            ESP_LOGI(TAG, "✓ Engine: RPM=%.0f, Coolant=%.0f°C, Throttle=%.0f%%", 
                     new_rpm, new_coolant, new_throttle);
        }
        
        xSemaphoreGive(data_mutex_);
    }
}

void KiaCanProtocol::ParseEngineTemps(const canbus::CanMessage& msg) {
    // CAN_ID_ENGINE_TEMPS = 0x269 (low frequency message, safe to log)
    // Byte 0: Coolant temperature (°C = value - 40)
    // Byte 1: Intake air temperature (°C = value - 40)
    
    if (msg.length < 2) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        float new_coolant = (float)msg.data[0] - 40.0f;
        float new_oil = (float)msg.data[1] - 40.0f;
        
        // Validate
        if (new_coolant < -50.0f || new_coolant > 150.0f) {
            ESP_LOGW(TAG, "Invalid coolant: %.1f°C (raw=0x%02X)", new_coolant, msg.data[0]);
            if (new_coolant < -50.0f) new_coolant = 0.0f;
        }
        
        // Log only if temperature changed by >1°C
        bool coolant_changed = (fabs(vehicle_data_.coolant_temp - new_coolant) >= 1.0f);
        bool oil_changed = (fabs(vehicle_data_.oil_temp - new_oil) >= 1.0f);
        
        vehicle_data_.coolant_temp = new_coolant;
        vehicle_data_.oil_temp = new_oil;
        
        if (coolant_changed || oil_changed) {
            ESP_LOGI(TAG, "Temps: Coolant=%.1f°C, Oil=%.1f°C (0x%02X 0x%02X)", 
                     new_coolant, new_oil, msg.data[0], msg.data[1]);
        }
        
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
    // CAN_ID_DOORS_BRAKE = 0x15F
    // Byte 0, bits 0-5: Door/Trunk/Hood status (1=Open, 0=Closed)
    // Byte 1, bit 0: Parking brake (1=Applied, 0=Released)
    // Byte 1, bit 1: Door locks (1=Locked, 0=Unlocked)
    
    if (msg.length < 2) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        // Parse door bits from byte 0
        uint8_t door_byte = msg.data[0];
        vehicle_data_.doors.driver_door_open = (door_byte & 0x01) != 0;      // Bit 0
        vehicle_data_.doors.passenger_door_open = (door_byte & 0x02) != 0;   // Bit 1
        vehicle_data_.doors.rear_left_open = (door_byte & 0x04) != 0;        // Bit 2
        vehicle_data_.doors.rear_right_open = (door_byte & 0x08) != 0;       // Bit 3
        vehicle_data_.doors.trunk_open = (door_byte & 0x10) != 0;            // Bit 4
        vehicle_data_.doors.hood_open = (door_byte & 0x20) != 0;             // Bit 5
        
        // Parse brake and locks from byte 1
        vehicle_data_.parking_brake_on = (msg.data[1] & 0x01) != 0;          // Bit 0
        vehicle_data_.doors.any_door_unlocked = (msg.data[1] & 0x02) == 0;   // Bit 1 (inverted logic)
        
        // Update door ajar warning
        vehicle_data_.door_ajar = 
            vehicle_data_.doors.driver_door_open ||
            vehicle_data_.doors.passenger_door_open ||
            vehicle_data_.doors.rear_left_open ||
            vehicle_data_.doors.rear_right_open ||
            vehicle_data_.doors.trunk_open ||
            vehicle_data_.doors.hood_open;
        
        ESP_LOGI(TAG, "✓ CAN 0x15F: Doors=[0x%02X] Driver=%d, Pass=%d, Trunk=%d, Brake=%d", 
                 door_byte,
                 vehicle_data_.doors.driver_door_open,
                 vehicle_data_.doors.passenger_door_open,
                 vehicle_data_.doors.trunk_open,
                 vehicle_data_.parking_brake_on);
        
        xSemaphoreGive(data_mutex_);
    }
}

void KiaCanProtocol::ParseSeatbelt(const canbus::CanMessage& msg) {
    // CAN_ID_SEATBELT = 0x0A1
    // Byte 0, bit 0: Driver seatbelt (1=Buckled, 0=Unbuckled)
    // Byte 0, bit 1: Passenger seatbelt (1=Buckled, 0=Unbuckled)
    // Byte 0, bit 2: Rear left seatbelt (1=Buckled, 0=Unbuckled)
    // Byte 0, bit 3: Rear right seatbelt (1=Buckled, 0=Unbuckled)
    
    if (msg.length < 1) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        uint8_t belt_byte = msg.data[0];
        
        bool old_driver = vehicle_data_.seatbelt_driver;
        bool old_passenger = vehicle_data_.seatbelt_passenger;
        
        // Bit 0: Driver, Bit 1: Passenger (1=fastened, 0=unfastened)
        vehicle_data_.seatbelt_driver = (belt_byte & 0x01) != 0;
        vehicle_data_.seatbelt_passenger = (belt_byte & 0x02) != 0;
        
        // Log only if changed to reduce spam
        if (vehicle_data_.seatbelt_driver != old_driver || 
            vehicle_data_.seatbelt_passenger != old_passenger) {
            ESP_LOGI(TAG, "⚠️  CAN 0x0A1: Seatbelt: Driver=%s, Passenger=%s (raw=0x%02X)",
                     vehicle_data_.seatbelt_driver ? "✓" : "✗",
                     vehicle_data_.seatbelt_passenger ? "✓" : "✗",
                     belt_byte);
        }
        
        xSemaphoreGive(data_mutex_);
    }
}

void KiaCanProtocol::ParseParkingBrake(const canbus::CanMessage& msg) {
    // Note: Parking brake is now parsed in ParseDoors() from CAN_ID_DOORS_BRAKE (0x15F)
    // This function kept for compatibility but data is primarily from ParseDoors
    
    if (msg.length < 1) return;
    
    if (xSemaphoreTake(data_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        // This might be redundant now, but kept as fallback
        bool old_state = vehicle_data_.parking_brake_on;
        vehicle_data_.parking_brake_on = (msg.data[0] & 0x01) != 0;
        
        if (vehicle_data_.parking_brake_on != old_state) {
            ESP_LOGI(TAG, "Parking Brake: %s", 
                     vehicle_data_.parking_brake_on ? "APPLIED ✓" : "RELEASED");
        }
        
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
        // Get current time for debouncing
        static int64_t last_overheat_critical_time = 0;
        static int64_t last_overheat_warn_time = 0;
        static int64_t last_battery_critical_time = 0;
        static int64_t last_battery_low_time = 0;
        static int64_t last_parking_brake_time = 0;
        static int64_t last_seatbelt_time = 0;
        static int64_t last_fuel_low_time = 0;
        
        int64_t now = esp_timer_get_time() / 1000;  // milliseconds
        
        // Check for critical alerts with debouncing
        
        // Battery critical (debounce: 5 seconds)
        if (vehicle_data_.battery_voltage < VEHICLE_BATTERY_CRITICAL_VOLTAGE &&
            vehicle_data_.battery_voltage > 0) {
            if (now - last_battery_critical_time > 5000) {
                for (auto& callback : alert_callbacks_) {
                    callback("Bố ơi, điện bình rất yếu! Cần kiểm tra ngay!", 1);
                }
                last_battery_critical_time = now;
            }
        } else if (vehicle_data_.battery_voltage < VEHICLE_BATTERY_LOW_VOLTAGE &&
                   vehicle_data_.battery_voltage > 0) {
            // Battery low (debounce: 30 seconds)
            if (now - last_battery_low_time > 30000) {
                for (auto& callback : alert_callbacks_) {
                    callback("Bố ơi, điện bình hơi yếu, bố nên kiểm tra để tránh khó đề máy.", 2);
                }
                last_battery_low_time = now;
            }
        }
        
        // Engine overheating - CRITICAL (debounce: 3 seconds)
        if (vehicle_data_.coolant_temp > VEHICLE_COOLANT_CRITICAL_TEMP) {
            if (now - last_overheat_critical_time > 3000) {
                for (auto& callback : alert_callbacks_) {
                    callback("CẢNH BÁO KHẨN CẤP! Nhiệt độ nước làm mát quá cao! Dừng xe ngay!", 0);
                }
                last_overheat_critical_time = now;
            }
        } else if (vehicle_data_.coolant_temp > VEHICLE_COOLANT_WARN_TEMP) {
            // Engine overheating - WARNING (debounce: 10 seconds)
            if (now - last_overheat_warn_time > 10000) {
                for (auto& callback : alert_callbacks_) {
                    callback("Bố ơi, nhiệt độ máy đang cao, bố nên giảm tốc độ.", 1);
                }
                last_overheat_warn_time = now;
            }
        }
        
        // Parking brake warning while moving (debounce: 5 seconds)
        if (vehicle_data_.parking_brake_on && vehicle_data_.vehicle_speed > 5) {
            if (now - last_parking_brake_time > 5000) {
                for (auto& callback : alert_callbacks_) {
                    callback("Bố ơi, phanh tay vẫn đang kéo! Hãy hạ phanh tay nhé!", 1);
                }
                last_parking_brake_time = now;
            }
        }
        
        // Seatbelt warning while moving (debounce: 10 seconds)
        if (!vehicle_data_.seatbelt_driver && vehicle_data_.vehicle_speed > 10) {
            if (now - last_seatbelt_time > 10000) {
                for (auto& callback : alert_callbacks_) {
                    callback("Bố ơi, bố chưa thắt dây an toàn!", 1);
                }
                last_seatbelt_time = now;
            }
        }
        
        // Low fuel (debounce: 30 seconds)
        if (vehicle_data_.low_fuel && vehicle_data_.ignition == IgnitionState::ON) {
            if (now - last_fuel_low_time > 30000) {
                for (auto& callback : alert_callbacks_) {
                    callback("Bố ơi, xăng sắp hết rồi. Nên đổ thêm nhé!", 2);
                }
                last_fuel_low_time = now;
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
