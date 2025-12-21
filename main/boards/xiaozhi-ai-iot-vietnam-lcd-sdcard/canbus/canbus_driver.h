#ifndef _CANBUS_DRIVER_H_
#define _CANBUS_DRIVER_H_

/**
 * @file canbus_driver.h
 * @brief CAN Bus Driver for Kia Morning 2017 Si - using ESP32-S3 TWAI (CAN) controller
 * @author Xiaozhi AI IoT Vietnam
 * @date 2024
 * 
 * This driver interfaces with SN65HVD230 CAN transceiver module to communicate
 * with the vehicle's OBD-II CAN bus system.
 * 
 * Features:
 * - TWAI driver initialization and configuration
 * - Power saving mode when idle (no CAN traffic for 5 minutes)
 * - Thread-safe message queue for received messages
 * - Error handling with automatic recovery
 * - Logging for debugging via Serial Monitor
 */

#include <driver/twai.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <functional>
#include <vector>
#include <atomic>

namespace canbus {

// CAN Bus driver states
enum class CanDriverState {
    UNINITIALIZED,  // Driver not yet initialized
    STOPPED,        // Driver initialized but stopped
    RUNNING,        // Driver actively receiving/transmitting
    POWER_SAVE,     // Power saving mode (TWAI in listen-only mode)
    ERROR,          // Error state, needs recovery
    RECOVERING      // Attempting to recover from error
};

// CAN message structure (wrapper around TWAI message)
struct CanMessage {
    uint32_t id;            // CAN ID (11-bit standard or 29-bit extended)
    uint8_t data[8];        // Data payload (max 8 bytes)
    uint8_t length;         // Data length (0-8)
    bool is_extended;       // True if extended CAN ID (29-bit)
    bool is_rtr;            // True if Remote Transmission Request
    int64_t timestamp_ms;   // Timestamp when message was received
};

// Callback type for received CAN messages
using CanMessageCallback = std::function<void(const CanMessage&)>;

// CAN Bus statistics for monitoring
struct CanStats {
    uint32_t rx_count;          // Total messages received
    uint32_t tx_count;          // Total messages transmitted
    uint32_t error_count;       // Total errors
    uint32_t bus_off_count;     // Bus-off events
    uint32_t arb_lost_count;    // Arbitration lost events
    int64_t last_rx_timestamp;  // Last received message timestamp
    int64_t last_error_timestamp; // Last error timestamp
    CanDriverState state;       // Current driver state
};

/**
 * @class CanBusDriver
 * @brief Main CAN Bus driver class for vehicle communication
 * 
 * This class manages the ESP32-S3 TWAI controller to communicate with the
 * vehicle's CAN bus via the SN65HVD230 transceiver.
 */
class CanBusDriver {
public:
    /**
     * @brief Get singleton instance of the CAN bus driver
     * @return Reference to the singleton instance
     */
    static CanBusDriver& GetInstance();

    /**
     * @brief Initialize the CAN bus driver
     * @param tx_gpio GPIO pin for CAN TX (connected to SN65HVD230 CTX)
     * @param rx_gpio GPIO pin for CAN RX (connected to SN65HVD230 CRX)
     * @param speed_kbps CAN bus speed in kbps (typically 500 for OBD-II)
     * @return true if initialization successful, false otherwise
     */
    bool Initialize(gpio_num_t tx_gpio, gpio_num_t rx_gpio, uint32_t speed_kbps);

    /**
     * @brief Start the CAN bus driver (begin receiving/transmitting)
     * @return true if started successfully
     */
    bool Start();

    /**
     * @brief Stop the CAN bus driver
     * @return true if stopped successfully
     */
    bool Stop();

    /**
     * @brief Deinitialize and cleanup the CAN bus driver
     */
    void Deinitialize();

    /**
     * @brief Send a CAN message
     * @param msg The message to send
     * @param timeout_ms Timeout in milliseconds (default 100ms)
     * @return true if message sent successfully
     */
    bool SendMessage(const CanMessage& msg, uint32_t timeout_ms = 100);

    /**
     * @brief Send a CAN message with standard ID
     * @param id CAN ID (11-bit)
     * @param data Pointer to data bytes
     * @param length Data length (0-8)
     * @param timeout_ms Timeout in milliseconds
     * @return true if sent successfully
     */
    bool SendMessage(uint32_t id, const uint8_t* data, uint8_t length, uint32_t timeout_ms = 100);

    /**
     * @brief Register callback for received messages
     * @param callback Function to call when message is received
     */
    void RegisterCallback(CanMessageCallback callback);

    /**
     * @brief Remove all registered callbacks
     */
    void ClearCallbacks();

    /**
     * @brief Add CAN ID filter (only receive messages matching filter)
     * @param id CAN ID to accept
     * @param mask Mask for filtering (1 = must match, 0 = don't care)
     * @return true if filter added successfully
     */
    bool AddFilter(uint32_t id, uint32_t mask);

    /**
     * @brief Clear all CAN ID filters (accept all messages)
     */
    void ClearFilters();

    /**
     * @brief Enter power saving mode
     * Reduces power consumption when no CAN activity detected
     */
    void EnterPowerSaveMode();

    /**
     * @brief Exit power saving mode
     * Resume normal operation
     */
    void ExitPowerSaveMode();

    /**
     * @brief Get current driver state
     * @return Current CanDriverState
     */
    CanDriverState GetState() const;

    /**
     * @brief Get CAN bus statistics
     * @return CanStats structure with current statistics
     */
    CanStats GetStats() const;

    /**
     * @brief Check if driver is initialized and running
     * @return true if ready to send/receive
     */
    bool IsReady() const;

    /**
     * @brief Get time since last received message
     * @return Time in milliseconds, or -1 if no messages received
     */
    int64_t GetTimeSinceLastMessage() const;

    /**
     * @brief Attempt to recover from error state
     * @return true if recovery successful
     */
    bool RecoverFromError();

private:
    // Private constructor for singleton
    CanBusDriver();
    ~CanBusDriver();

    // Delete copy constructor and assignment operator
    CanBusDriver(const CanBusDriver&) = delete;
    CanBusDriver& operator=(const CanBusDriver&) = delete;

    // Internal methods
    void ReceiveTask();                     // FreeRTOS task for receiving messages
    void IdleMonitorTask();                 // Task to monitor idle and enter power save
    void ProcessReceivedMessage(const twai_message_t& twai_msg);
    void HandleAlerts(uint32_t alerts);
    void UpdateStats(const twai_status_info_t& status);
    bool ConfigureTwai(gpio_num_t tx_gpio, gpio_num_t rx_gpio, uint32_t speed_kbps);
    twai_timing_config_t GetTimingConfig(uint32_t speed_kbps);

    // Static task wrappers for FreeRTOS
    static void ReceiveTaskWrapper(void* arg);
    static void IdleMonitorTaskWrapper(void* arg);

    // Member variables
    std::atomic<CanDriverState> state_;
    std::vector<CanMessageCallback> callbacks_;
    SemaphoreHandle_t callbacks_mutex_;
    SemaphoreHandle_t stats_mutex_;

    TaskHandle_t receive_task_handle_;
    TaskHandle_t idle_monitor_task_handle_;

    CanStats stats_;
    gpio_num_t tx_gpio_;
    gpio_num_t rx_gpio_;
    uint32_t speed_kbps_;

    std::atomic<bool> stop_requested_;
    std::atomic<int64_t> last_rx_timestamp_;

    bool is_initialized_;
};

} // namespace canbus

#endif // _CANBUS_DRIVER_H_
