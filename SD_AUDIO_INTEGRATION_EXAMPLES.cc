/**
 * @file sd_audio_integration_examples.cc
 * @brief Example integration of SDMp3Player with CAN bus callbacks
 * 
 * This file shows how to wire the audio player to CAN bus events
 * from the Kia Morning 2017 OBD-II protocol.
 * 
 * Usage: Copy these functions into vehicle_assistant.cc or
 * create appropriate callback handlers in your CAN event system.
 */

#include "offline/sd_audio_player.h"
#include "esp_log.h"

static const char* TAG = "CAN_Audio_Integration";

// ============================================================================
// BATTERY MONITORING - Play battery alerts when voltage drops
// ============================================================================

/**
 * Called when vehicle battery voltage changes
 * @param voltage_x10: Battery voltage in 0.1V units (e.g., 140 = 14.0V)
 * 
 * Typical Kia voltage ranges:
 *   - Normal: 130-150 (13.0-15.0V, engine running)
 *   - Idle: 120-135 (12.0-13.5V, engine off)
 *   - Warning: <100 (<10.0V, critical)
 *   - Low: <200 (<20.0V, needs charging)
 */
void OnBatteryVoltageChange(uint16_t voltage_x10) {
    static uint16_t last_alert_voltage = 0;
    static int64_t last_alert_time = 0;
    
    // Throttle alerts (max once per 10 seconds)
    int64_t now = esp_timer_get_time() / 1000000;
    if (now - last_alert_time < 10) {
        return;  // Too soon, skip
    }
    
    if (voltage_x10 < 100) {  // <10V - CRITICAL
        // Only alert if voltage dropped significantly
        if (voltage_x10 < (last_alert_voltage - 5)) {
            ESP_LOGW(TAG, "🚨 Battery CRITICAL: %.1fV", voltage_x10 / 10.0f);
            offline::SDMp3Player::GetInstance().PlayBatteryWarning(true);  // battery_critical.mp3
            last_alert_voltage = voltage_x10;
            last_alert_time = now;
        }
    } 
    else if (voltage_x10 < 200) {  // <20V - LOW
        // Alert when first drops below 20V
        if (last_alert_voltage >= 200) {
            ESP_LOGW(TAG, "⚠️ Battery LOW: %.1fV", voltage_x10 / 10.0f);
            offline::SDMp3Player::GetInstance().PlayBatteryWarning(false);  // battery_low.mp3
            last_alert_voltage = voltage_x10;
            last_alert_time = now;
        }
    }
    else {
        // Voltage recovered, reset alert
        last_alert_voltage = voltage_x10;
    }
}

// ============================================================================
// FUEL LEVEL MONITORING - Play fuel warnings
// ============================================================================

/**
 * Called when fuel tank level changes
 * @param fuel_percent: Fuel remaining as percentage (0-100%)
 * 
 * Typical Kia fuel reserve:
 *   - Full: 95-100%
 *   - Low: <15% (warning light)
 *   - Critical: <5% (urgent warning, ~5-10km range)
 */
void OnFuelLevelChange(uint8_t fuel_percent) {
    static uint8_t last_alert_level = 100;
    static int64_t last_alert_time = 0;
    
    // Throttle alerts (max once per 30 seconds)
    int64_t now = esp_timer_get_time() / 1000000;
    if (now - last_alert_time < 30) {
        return;
    }
    
    if (fuel_percent < 5) {  // CRITICAL - <5%
        if (last_alert_level >= 5) {
            ESP_LOGW(TAG, "🚨 Fuel CRITICAL: %d%%", fuel_percent);
            offline::SDMp3Player::GetInstance().PlayFuelWarning(true);  // fuel_critical.mp3
            last_alert_level = fuel_percent;
            last_alert_time = now;
        }
    }
    else if (fuel_percent < 15) {  // LOW - <15%
        if (last_alert_level >= 15) {
            ESP_LOGW(TAG, "⚠️ Fuel LOW: %d%%", fuel_percent);
            offline::SDMp3Player::GetInstance().PlayFuelWarning(false);  // fuel_low.mp3
            last_alert_level = fuel_percent;
            last_alert_time = now;
        }
    }
    else {
        // Fuel level recovered
        last_alert_level = fuel_percent;
    }
}

// ============================================================================
// ENGINE TEMPERATURE MONITORING - Play temperature alerts
// ============================================================================

/**
 * Called when engine coolant temperature changes
 * @param temp_celsius: Coolant temperature in degrees Celsius
 * 
 * Typical Kia temperature ranges:
 *   - Normal: 80-95°C (running)
 *   - High: 95-105°C (cooling fan active)
 *   - Critical: >105°C (warning, engine protection mode)
 *   - Cold start: <60°C (warming up)
 */
void OnEngineTemperatureChange(uint8_t temp_celsius) {
    static uint8_t last_alert_temp = 0;
    static int64_t last_alert_time = 0;
    
    // Throttle alerts (max once per 20 seconds)
    int64_t now = esp_timer_get_time() / 1000000;
    if (now - last_alert_time < 20) {
        return;
    }
    
    if (temp_celsius > 105) {  // CRITICAL - >105°C
        if (last_alert_temp <= 105) {
            ESP_LOGE(TAG, "🚨 Engine OVERHEATING: %d°C", temp_celsius);
            offline::SDMp3Player::GetInstance().PlayTempWarning(true);  // temp_critical.mp3
            last_alert_temp = temp_celsius;
            last_alert_time = now;
        }
    }
    else if (temp_celsius > 95) {  // HIGH - >95°C
        if (last_alert_temp <= 95) {
            ESP_LOGW(TAG, "⚠️ Engine temperature HIGH: %d°C", temp_celsius);
            offline::SDMp3Player::GetInstance().PlayTempWarning(false);  // temp_high.mp3
            last_alert_temp = temp_celsius;
            last_alert_time = now;
        }
    }
    else {
        // Temperature recovered
        last_alert_temp = temp_celsius;
    }
}

// ============================================================================
// SEATBELT MONITORING - Play seatbelt reminders
// ============================================================================

/**
 * Called when driver seatbelt status changes or vehicle speed increases
 * @param driver_fastened: true if driver seatbelt is fastened
 * @param vehicle_speed: Vehicle speed in km/h
 * 
 * Kia behavior:
 *   - Chime every 30 seconds if unfastened (stationary or slow speed)
 *   - Urgent warning if unfastened and speed >80 km/h
 *   - No warning if already playing another alert
 */
void OnSeatbeltStatusChange(bool driver_fastened, uint16_t vehicle_speed) {
    static bool last_fastened = true;
    static int64_t last_alert_time = 0;
    
    // Throttle non-urgent alerts (every 30 seconds if continuously unfastened)
    int64_t now = esp_timer_get_time() / 1000000;
    bool urgent = (!driver_fastened && vehicle_speed > 80);
    
    if (!driver_fastened) {
        if (urgent) {
            // URGENT: Speed >80 km/h + unfastened
            // Play urgent reminder more frequently
            if (now - last_alert_time > 10) {  // Every 10 seconds at high speed
                ESP_LOGW(TAG, "🚨 URGENT: Seatbelt unfastened at %.0f km/h", vehicle_speed / 10.0f);
                offline::SDMp3Player::GetInstance().PlaySeatbeltWarning(true);  // warn_seatbelt_urgent.mp3
                last_alert_time = now;
            }
        }
        else {
            // Standard reminder when unfastened at low speed
            if (last_fastened || (now - last_alert_time > 30)) {  // First time or every 30s
                ESP_LOGW(TAG, "⚠️ Seatbelt reminder: %.0f km/h", vehicle_speed / 10.0f);
                offline::SDMp3Player::GetInstance().PlaySeatbeltWarning(false);  // warn_seatbelt.mp3
                last_alert_time = now;
            }
        }
        last_fastened = false;
    }
    else {
        // Seatbelt fastened - reset alert timer
        last_fastened = true;
    }
}

// ============================================================================
// DOOR MONITORING - Play door open warnings while driving
// ============================================================================

/**
 * Called when a door is opened while vehicle is moving
 * @param door_id: 0=driver, 1=front_passenger, 2=rear_left, 3=rear_right
 * @param vehicle_speed: Vehicle speed in km/h
 */
void OnDoorOpenedWhileDriving(uint8_t door_id, uint16_t vehicle_speed) {
    static int64_t last_alert_time = 0;
    
    // Throttle alerts (max once per 5 seconds)
    int64_t now = esp_timer_get_time() / 1000000;
    if (now - last_alert_time < 5) {
        return;
    }
    
    if (vehicle_speed > 10) {  // Only alert if clearly moving
        const char* door_names[] = {"driver", "front_passenger", "rear_left", "rear_right"};
        const char* door_name = (door_id < 4) ? door_names[door_id] : "unknown";
        
        ESP_LOGW(TAG, "⚠️ %s door opened at %.0f km/h", door_name, vehicle_speed / 10.0f);
        offline::SDMp3Player::GetInstance().Play("warn_door_open.mp3");
        last_alert_time = now;
    }
}

// ============================================================================
// HEADLIGHTS MONITORING - Remind if lights left on
// ============================================================================

/**
 * Called when headlights are detected on but engine is off
 * @param lights_on: true if headlights are on
 * @param engine_running: true if engine is running
 */
void OnHeadlightsLeftOn(bool lights_on, bool engine_running) {
    static int64_t last_alert_time = 0;
    
    // Throttle alerts (max once per 5 minutes)
    int64_t now = esp_timer_get_time() / 1000000;
    if (now - last_alert_time < 300) {
        return;
    }
    
    if (lights_on && !engine_running) {
        ESP_LOGW(TAG, "⚠️ Headlights left on with engine off");
        offline::SDMp3Player::GetInstance().Play("warn_lights_on.mp3");
        last_alert_time = now;
    }
}

// ============================================================================
// PARKING BRAKE MONITORING - Warn if not released when driving
// ============================================================================

/**
 * Called when parking brake status changes
 * @param brake_engaged: true if parking brake is engaged
 * @param vehicle_speed: Vehicle speed in km/h
 */
void OnParkingBrakeEngaged(bool brake_engaged, uint16_t vehicle_speed) {
    static int64_t last_alert_time = 0;
    
    // Throttle alerts (max once per 10 seconds)
    int64_t now = esp_timer_get_time() / 1000000;
    if (now - last_alert_time < 10) {
        return;
    }
    
    if (brake_engaged && vehicle_speed > 20) {  // Moving with parking brake on
        ESP_LOGE(TAG, "🚨 PARKING BRAKE ENGAGED WHILE DRIVING: %.0f km/h", vehicle_speed / 10.0f);
        offline::SDMp3Player::GetInstance().Play("warn_parking_brake.mp3");
        last_alert_time = now;
    }
}

// ============================================================================
// SPEED LIMIT DETECTION - Announce detected speed limit
// ============================================================================

/**
 * Called when a speed limit sign is detected (via camera or map data)
 * @param speed_kmh: Speed limit in km/h
 */
void OnSpeedLimitDetected(uint16_t speed_kmh) {
    static uint16_t last_speed_announced = 0;
    
    // Only announce if different from last announced speed
    if (speed_kmh != last_speed_announced) {
        ESP_LOGI(TAG, "🛑 Speed limit detected: %d km/h", speed_kmh);
        
        // Map to supported speed files
        int announced_speed = speed_kmh;
        if (speed_kmh < 40) announced_speed = 40;
        else if (speed_kmh > 150) announced_speed = 150;
        
        offline::SDMp3Player::GetInstance().PlaySpeedWarning(announced_speed);
        last_speed_announced = speed_kmh;
    }
}

// ============================================================================
// GREETING ON STARTUP - Play welcome message
// ============================================================================

/**
 * Called during application initialization
 * Should play greeting appropriate to time of day
 */
void PlayStartupGreeting() {
    // Get current time
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    int hour = timeinfo->tm_hour;
    
    // Choose greeting based on time
    std::string greeting_type = "default";
    if (hour >= 6 && hour < 12) {
        greeting_type = "morning";
        ESP_LOGI(TAG, "🌅 Good morning!");
    }
    else if (hour >= 12 && hour < 18) {
        greeting_type = "afternoon";
        ESP_LOGI(TAG, "☀️ Good afternoon!");
    }
    else if (hour >= 18 && hour < 24) {
        greeting_type = "evening";
        ESP_LOGI(TAG, "🌙 Good evening!");
    }
    else {
        greeting_type = "default";
        ESP_LOGI(TAG, "👋 Welcome!");
    }
    
    offline::SDMp3Player::GetInstance().PlayGreeting(greeting_type);
}

// ============================================================================
// CHECK ENGINE LIGHT - Play fault code alert
// ============================================================================

/**
 * Called when OBD-II fault codes are detected
 * @param fault_code: DTC (Diagnostic Trouble Code)
 * @param severity: 0=info, 1=warning, 2=critical
 */
void OnFaultCodeDetected(uint32_t fault_code, uint8_t severity) {
    static int64_t last_alert_time = 0;
    
    // Throttle alerts (max once per 30 seconds per code)
    int64_t now = esp_timer_get_time() / 1000000;
    if (now - last_alert_time < 30) {
        return;
    }
    
    // For now, play generic check engine alert
    // Could extend to play specific alerts for common faults
    if (severity >= 1) {  // Warning or critical
        ESP_LOGW(TAG, "⚠️ Fault code detected: 0x%06X (severity=%d)", fault_code, severity);
        offline::SDMp3Player::GetInstance().Play("warn_check_engine.mp3");
        last_alert_time = now;
    }
}

// ============================================================================
// INTEGRATION EXAMPLE IN MAIN APPLICATION
// ============================================================================

/**
 * Example of how to integrate all these callbacks in your main application
 * 
 * In your application initialization, wire up the CAN bus event handlers:
 * 
 *   // Register CAN callbacks
 *   VehicleAssistant vehicle;
 *   vehicle.OnBatteryVoltageChange.Register(OnBatteryVoltageChange);
 *   vehicle.OnFuelLevelChange.Register(OnFuelLevelChange);
 *   vehicle.OnEngineTemperatureChange.Register(OnEngineTemperatureChange);
 *   vehicle.OnSeatbeltStatusChange.Register(OnSeatbeltStatusChange);
 *   vehicle.OnDoorOpenedWhileDriving.Register(OnDoorOpenedWhileDriving);
 *   vehicle.OnHeadlightsLeftOn.Register(OnHeadlightsLeftOn);
 *   vehicle.OnParkingBrakeEngaged.Register(OnParkingBrakeEngaged);
 *   vehicle.OnSpeedLimitDetected.Register(OnSpeedLimitDetected);
 *   vehicle.OnFaultCodeDetected.Register(OnFaultCodeDetected);
 *   
 *   // Initialize audio system
 *   offline::SDMp3Player::GetInstance();  // Singleton initialization
 *   
 *   // Play startup greeting
 *   PlayStartupGreeting();
 * 
 * Then as CAN messages arrive from the vehicle, the callbacks will
 * automatically play the appropriate audio alerts.
 */

// ============================================================================
// TESTING EXAMPLE - Manual playback for verification
// ============================================================================

/**
 * For testing, you can call these functions from serial console
 * or create a test task that cycles through all alerts
 */
void TestAllAudioAlerts() {
    ESP_LOGI(TAG, "🧪 Testing all audio alerts...");
    
    // Greetings
    offline::SDMp3Player::GetInstance().PlayGreeting("default");
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    offline::SDMp3Player::GetInstance().PlayGreeting("morning");
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // Warnings
    offline::SDMp3Player::GetInstance().PlayBatteryWarning(false);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    offline::SDMp3Player::GetInstance().PlayBatteryWarning(true);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    offline::SDMp3Player::GetInstance().PlayFuelWarning(false);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    offline::SDMp3Player::GetInstance().PlayFuelWarning(true);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    offline::SDMp3Player::GetInstance().PlayTempWarning(false);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    offline::SDMp3Player::GetInstance().PlayTempWarning(true);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    offline::SDMp3Player::GetInstance().PlaySeatbeltWarning(false);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    offline::SDMp3Player::GetInstance().PlaySeatbeltWarning(true);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    offline::SDMp3Player::GetInstance().PlaySpeedWarning(80);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    ESP_LOGI(TAG, "✅ Test complete!");
}
