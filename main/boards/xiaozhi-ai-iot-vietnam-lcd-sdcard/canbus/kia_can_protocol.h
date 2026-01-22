#ifndef _KIA_CAN_PROTOCOL_H_
#define _KIA_CAN_PROTOCOL_H_

/**
 * @file kia_can_protocol.h
 * @brief Kia Morning 2017 Si CAN Bus Protocol Parser
 * @author Xiaozhi AI IoT Vietnam
 * @date 2024
 * 
 * This file defines CAN IDs and data structures for parsing CAN bus messages
 * from the Kia Morning 2017 Si (also known as Kia Picanto in some markets).
 * 
 * Note: CAN IDs may vary between vehicle models and regions.
 * These are common IDs for Hyundai/Kia vehicles but may need adjustment.
 */

#include "canbus_driver.h"
#include <cstdint>
#include <functional>

namespace kia {

// ============================================================================
// Kia Morning 2017 CAN Bus IDs (Actual values for this vehicle)
// Verified on: Kia Morning 2017 with OBD-II connector
// See docs/KIA_MORNING_2017_CAN_PROTOCOL_GUIDE.md for detailed format info
// ============================================================================

// Engine & Powertrain (ECM - Engine Control Module)
// ✓ VERIFIED ON ACTUAL VEHICLE: 0x316 contains RPM, Coolant, Throttle
constexpr uint32_t CAN_ID_ENGINE_DATA_1         = 0x316;    // ✓ RPM (B2-3 /4), Coolant (B1-40), Throttle (B4)
constexpr uint32_t CAN_ID_ENGINE_TEMPS          = 0x316;    // Same ID: Coolant in B1
constexpr uint32_t CAN_ID_ENGINE_DATA_2         = 0x316;    // Consolidated to 0x316
constexpr uint32_t CAN_ID_TRANSMISSION          = 0x43F;    // ✓ VERIFIED: Gear position (B0: 0=P, 1=R, 2=N, 3=D)
constexpr uint32_t CAN_ID_VEHICLE_SPEED         = 0x386;    // ✓ Vehicle speed (wheel speeds)

// Body Control Module (BCM)
constexpr uint32_t CAN_ID_DOORS_BRAKE           = 0x15F;    // ✓ Doors, Trunk, Parking Brake, Door Locks
constexpr uint32_t CAN_ID_SEATBELT              = 0x0A1;    // ✓ Seatbelt status (driver, passenger, rear)
constexpr uint32_t CAN_ID_LIGHTS_WIPER          = 0x680;    // ✓ Lights (headlights, fog, turn), Wiper
constexpr uint32_t CAN_ID_ODOMETER              = 0x4F0;    // Odometer reading (if available)

// Backward-compatible aliases for old constant names
constexpr uint32_t CAN_ID_DOORS                 = CAN_ID_DOORS_BRAKE;    // Alias (use CAN_ID_DOORS_BRAKE)
constexpr uint32_t CAN_ID_PARKING_BRAKE         = CAN_ID_DOORS_BRAKE;    // Alias (brake data in CAN_ID_DOORS_BRAKE)
constexpr uint32_t CAN_ID_LIGHTS                = CAN_ID_LIGHTS_WIPER;   // Alias (use CAN_ID_LIGHTS_WIPER)

// Electrical System
constexpr uint32_t CAN_ID_BATTERY               = 0x5A0;    // Battery voltage
constexpr uint32_t CAN_ID_IGNITION              = 0x5B0;    // Ignition status

// Climate Control
constexpr uint32_t CAN_ID_CLIMATE               = 0x7A0;    // AC status, Fan speed, Cabin temp

// Fuel System
constexpr uint32_t CAN_ID_FUEL                  = 0x545;    // ✓ Fuel level %, Estimated range (km)

// Instrument Cluster
constexpr uint32_t CAN_ID_CLUSTER_1             = 0x580;    // Warning indicators, ABS, Check engine
constexpr uint32_t CAN_ID_CLUSTER_2             = 0x581;    // Trip computer data

// ============================================================================
// Data Structures for Vehicle Status
// ============================================================================

// Door status flags
struct DoorStatus {
    bool driver_door_open : 1;
    bool passenger_door_open : 1;
    bool rear_left_open : 1;
    bool rear_right_open : 1;
    bool trunk_open : 1;
    bool hood_open : 1;
    bool any_door_unlocked : 1;
    bool reserved : 1;
};

// Lights status
struct LightsStatus {
    bool headlights_on : 1;
    bool high_beam_on : 1;
    bool fog_lights_on : 1;
    bool parking_lights_on : 1;
    bool turn_left_on : 1;
    bool turn_right_on : 1;
    bool hazard_on : 1;
    bool interior_light_on : 1;
};

// Ignition status
enum class IgnitionState : uint8_t {
    OFF = 0,
    ACC = 1,         // Accessory
    ON = 2,          // Ignition on
    START = 3        // Cranking
};

// Gear position
enum class GearPosition : uint8_t {
    PARK = 0,
    REVERSE = 1,
    NEUTRAL = 2,
    DRIVE = 3,
    SPORT = 4,       // If available
    LOW = 5,         // Low gear
    UNKNOWN = 255
};

// Climate control mode
enum class ClimateMode : uint8_t {
    OFF = 0,
    COOL = 1,
    HEAT = 2,
    AUTO = 3,
    DEFROST = 4
};

// Complete vehicle data structure
struct VehicleData {
    // Engine data
    float engine_rpm;           // RPM
    float throttle_position;    // 0-100%
    float coolant_temp;         // Celsius
    float oil_temp;             // Celsius (if available)
    
    // Speed & Odometer
    float vehicle_speed;        // km/h
    uint32_t odometer_km;       // Total kilometers
    float trip_km;              // Trip distance
    
    // Fuel
    float fuel_level_percent;   // 0-100%
    float fuel_consumption;     // L/100km (average)
    float range_km;             // Estimated range
    
    // Electrical
    float battery_voltage;      // Volts (12V system)
    IgnitionState ignition;     // Ignition status
    
    // Body
    DoorStatus doors;           // Door status
    bool seatbelt_driver;       // Driver seatbelt
    bool seatbelt_passenger;    // Passenger seatbelt
    bool parking_brake_on;      // Parking brake engaged
    LightsStatus lights;        // Lights status
    bool wiper_on;              // Wiper active
    
    // Transmission
    GearPosition gear;          // Current gear
    
    // Climate
    bool ac_on;                 // AC compressor on
    float cabin_temp;           // Interior temperature (if available)
    float set_temp;             // Set temperature
    uint8_t fan_speed;          // Fan speed (0-7)
    ClimateMode climate_mode;   // Climate mode
    
    // Warning flags
    bool check_engine;          // Check engine light
    bool low_fuel;              // Low fuel warning
    bool low_oil;               // Low oil pressure
    bool battery_warning;       // Battery/charging issue
    bool door_ajar;             // Door ajar warning
    bool airbag_warning;        // Airbag system warning
    bool abs_warning;           // ABS warning
    bool tpms_warning;          // Tire pressure warning
    
    // Timestamps
    int64_t last_update_ms;     // Last data update timestamp
    bool data_valid;            // True if receiving valid data
};

// ============================================================================
// Callback types
// ============================================================================

using VehicleDataCallback = std::function<void(const VehicleData&)>;
using DoorEventCallback = std::function<void(const DoorStatus&, const DoorStatus&)>;  // old, new
using AlertCallback = std::function<void(const char* message, int priority)>;

// ============================================================================
// Kia CAN Protocol Parser Class
// ============================================================================

/**
 * @class KiaCanProtocol
 * @brief Parser for Kia Morning 2017 Si CAN bus messages
 * 
 * This class interprets raw CAN messages and extracts meaningful vehicle data.
 * It maintains the current vehicle state and provides callbacks for changes.
 */
class KiaCanProtocol {
public:
    /**
     * @brief Get singleton instance
     */
    static KiaCanProtocol& GetInstance();

    /**
     * @brief Initialize the protocol parser
     * @return true if initialized successfully
     */
    bool Initialize();

    /**
     * @brief Process a received CAN message
     * @param msg The CAN message to process
     */
    void ProcessMessage(const canbus::CanMessage& msg);

    /**
     * @brief Get current vehicle data
     * @return Reference to current vehicle data
     */
    const VehicleData& GetVehicleData() const;

    /**
     * @brief Check if receiving valid CAN data
     * @return true if data is fresh and valid
     */
    bool IsDataValid() const;

    /**
     * @brief Get time since last valid data
     * @return Time in milliseconds, or -1 if no data received
     */
    int64_t GetTimeSinceLastData() const;

    /**
     * @brief Register callback for vehicle data updates
     * @param callback Function to call when data changes
     */
    void RegisterDataCallback(VehicleDataCallback callback);

    /**
     * @brief Register callback for door events
     * @param callback Function to call when door status changes
     */
    void RegisterDoorCallback(DoorEventCallback callback);

    /**
     * @brief Register callback for alerts/warnings
     * @param callback Function to call for alerts
     */
    void RegisterAlertCallback(AlertCallback callback);

    /**
     * @brief Clear all callbacks
     */
    void ClearCallbacks();

    /**
     * @brief Request specific PID via OBD-II (if supported)
     * @param pid The OBD-II PID to request
     * @return true if request sent successfully
     */
    bool RequestOBDPid(uint8_t pid);

    /**
     * @brief Get estimated fuel range in km
     * @return Estimated range, or -1 if cannot calculate
     */
    float GetEstimatedRange() const;

    /**
     * @brief Get driving time since engine start
     * @return Driving time in minutes
     */
    int GetDrivingTimeMinutes() const;

    /**
     * @brief Reset trip computer values
     */
    void ResetTrip();

private:
    KiaCanProtocol();
    ~KiaCanProtocol() = default;
    
    KiaCanProtocol(const KiaCanProtocol&) = delete;
    KiaCanProtocol& operator=(const KiaCanProtocol&) = delete;

    // Parsing methods for different CAN IDs
    void ParseEngineData1(const canbus::CanMessage& msg);
    void ParseEngineTemps(const canbus::CanMessage& msg);  // Engine temperatures (Coolant, Oil, Intake)
    void ParseVehicleSpeed(const canbus::CanMessage& msg);
    void ParseOdometer(const canbus::CanMessage& msg);
    void ParseDoors(const canbus::CanMessage& msg);
    void ParseSeatbelt(const canbus::CanMessage& msg);
    void ParseParkingBrake(const canbus::CanMessage& msg);
    void ParseLights(const canbus::CanMessage& msg);
    void ParseBattery(const canbus::CanMessage& msg);
    void ParseIgnition(const canbus::CanMessage& msg);
    void ParseFuel(const canbus::CanMessage& msg);
    void ParseClimate(const canbus::CanMessage& msg);
    void ParseCluster1(const canbus::CanMessage& msg);
    void ParseTransmission(const canbus::CanMessage& msg);

    // Helper methods
    void CheckForAlerts();
    void NotifyDataCallbacks();
    void NotifyDoorCallbacks(const DoorStatus& old_status);

    // Member variables
    VehicleData vehicle_data_;
    std::vector<VehicleDataCallback> data_callbacks_;
    std::vector<DoorEventCallback> door_callbacks_;
    std::vector<AlertCallback> alert_callbacks_;
    
    SemaphoreHandle_t data_mutex_;
    SemaphoreHandle_t callback_mutex_;
    
    int64_t engine_start_time_;     // When engine was started
    float trip_start_odo_;          // Odometer at trip start
    bool is_initialized_;
};

// ============================================================================
// Helper functions
// ============================================================================

/**
 * @brief Convert gear position to string
 */
const char* GearToString(GearPosition gear);

/**
 * @brief Convert ignition state to string
 */
const char* IgnitionToString(IgnitionState state);

/**
 * @brief Format speed for voice output
 * @param speed_kmh Speed in km/h
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 */
void FormatSpeedForVoice(float speed_kmh, char* buffer, size_t buffer_size);

/**
 * @brief Format fuel level for voice output
 */
void FormatFuelForVoice(float fuel_percent, float range_km, char* buffer, size_t buffer_size);

/**
 * @brief Format temperature for voice output
 */
void FormatTempForVoice(float temp_celsius, char* buffer, size_t buffer_size);

} // namespace kia

#endif // _KIA_CAN_PROTOCOL_H_
