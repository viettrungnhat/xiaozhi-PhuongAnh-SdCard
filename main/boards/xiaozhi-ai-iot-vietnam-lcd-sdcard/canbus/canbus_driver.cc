/**
 * @file canbus_driver.cc
 * @brief CAN Bus Driver implementation for Kia Morning 2017 Si
 * @author Xiaozhi AI IoT Vietnam
 * @date 2024
 * 
 * Implementation of the TWAI-based CAN bus driver with:
 * - Error handling and automatic recovery
 * - Power saving mode for battery conservation
 * - Thread-safe message handling
 * - Detailed logging for debugging
 */

#include "canbus_driver.h"
#include "../config.h"
#include <esp_timer.h>
#include <cstring>

static const char* TAG = "CAN_Driver";

namespace canbus {

// ============================================================================
// Singleton Instance
// ============================================================================

CanBusDriver& CanBusDriver::GetInstance() {
    static CanBusDriver instance;
    return instance;
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

CanBusDriver::CanBusDriver()
    : state_(CanDriverState::UNINITIALIZED)
    , callbacks_mutex_(nullptr)
    , stats_mutex_(nullptr)
    , receive_task_handle_(nullptr)
    , idle_monitor_task_handle_(nullptr)
    , tx_gpio_(GPIO_NUM_NC)
    , rx_gpio_(GPIO_NUM_NC)
    , speed_kbps_(500)
    , stop_requested_(false)
    , last_rx_timestamp_(0)
    , is_initialized_(false)
{
    memset(&stats_, 0, sizeof(stats_));
    
    // Create mutexes
    callbacks_mutex_ = xSemaphoreCreateMutex();
    stats_mutex_ = xSemaphoreCreateMutex();
    
    if (!callbacks_mutex_ || !stats_mutex_) {
        ESP_LOGE(TAG, "Failed to create mutexes");
    }
    
    ESP_LOGI(TAG, "CAN Bus Driver created");
}

CanBusDriver::~CanBusDriver() {
    Deinitialize();
    
    if (callbacks_mutex_) {
        vSemaphoreDelete(callbacks_mutex_);
        callbacks_mutex_ = nullptr;
    }
    if (stats_mutex_) {
        vSemaphoreDelete(stats_mutex_);
        stats_mutex_ = nullptr;
    }
    
    ESP_LOGI(TAG, "CAN Bus Driver destroyed");
}

// ============================================================================
// Initialization
// ============================================================================

bool CanBusDriver::Initialize(gpio_num_t tx_gpio, gpio_num_t rx_gpio, uint32_t speed_kbps) {
    ESP_LOGI(TAG, "Initializing CAN Bus Driver - TX: GPIO%d, RX: GPIO%d, Speed: %lukbps", 
             tx_gpio, rx_gpio, speed_kbps);
    
    if (is_initialized_) {
        ESP_LOGW(TAG, "Driver already initialized, deinitializing first");
        Deinitialize();
    }
    
    tx_gpio_ = tx_gpio;
    rx_gpio_ = rx_gpio;
    speed_kbps_ = speed_kbps;
    
    if (!ConfigureTwai(tx_gpio, rx_gpio, speed_kbps)) {
        ESP_LOGE(TAG, "Failed to configure TWAI controller");
        state_.store(CanDriverState::ERROR);
        return false;
    }
    
    is_initialized_ = true;
    state_.store(CanDriverState::STOPPED);
    
    ESP_LOGI(TAG, "CAN Bus Driver initialized successfully");
    return true;
}

bool CanBusDriver::ConfigureTwai(gpio_num_t tx_gpio, gpio_num_t rx_gpio, uint32_t speed_kbps) {
    ESP_LOGI(TAG, "Configuring TWAI controller");
    
    // General configuration
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(tx_gpio, rx_gpio, TWAI_MODE_NORMAL);
    g_config.rx_queue_len = CAN_RX_QUEUE_SIZE;
    g_config.tx_queue_len = 10;
    g_config.alerts_enabled = TWAI_ALERT_ALL;  // Enable all alerts for monitoring
    g_config.clkout_divider = 0;  // Disable clock output
    
    // Timing configuration based on speed
    twai_timing_config_t t_config = GetTimingConfig(speed_kbps);
    
    // Filter configuration - accept all messages initially
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    // Install TWAI driver
    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install TWAI driver: %s (0x%x)", esp_err_to_name(err), err);
        return false;
    }
    
    ESP_LOGI(TAG, "TWAI driver installed successfully");
    return true;
}

twai_timing_config_t CanBusDriver::GetTimingConfig(uint32_t speed_kbps) {
    ESP_LOGD(TAG, "Getting timing config for %lukbps", speed_kbps);
    
    switch (speed_kbps) {
        case 1000:
            return (twai_timing_config_t)TWAI_TIMING_CONFIG_1MBITS();
        case 800:
            return (twai_timing_config_t)TWAI_TIMING_CONFIG_800KBITS();
        case 500:
            return (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS();
        case 250:
            return (twai_timing_config_t)TWAI_TIMING_CONFIG_250KBITS();
        case 125:
            return (twai_timing_config_t)TWAI_TIMING_CONFIG_125KBITS();
        case 100:
            return (twai_timing_config_t)TWAI_TIMING_CONFIG_100KBITS();
        case 50:
            return (twai_timing_config_t)TWAI_TIMING_CONFIG_50KBITS();
        case 25:
            return (twai_timing_config_t)TWAI_TIMING_CONFIG_25KBITS();
        default:
            ESP_LOGW(TAG, "Unsupported speed %lukbps, defaulting to 500kbps", speed_kbps);
            return (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS();
    }
}

// ============================================================================
// Start / Stop
// ============================================================================

bool CanBusDriver::Start() {
    ESP_LOGI(TAG, "Starting CAN Bus Driver");
    
    if (!is_initialized_) {
        ESP_LOGE(TAG, "Driver not initialized");
        return false;
    }
    
    CanDriverState current = state_.load();
    if (current == CanDriverState::RUNNING) {
        ESP_LOGW(TAG, "Driver already running");
        return true;
    }
    
    // Start TWAI driver
    esp_err_t err = twai_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start TWAI driver: %s (0x%x)", esp_err_to_name(err), err);
        state_.store(CanDriverState::ERROR);
        return false;
    }
    
    stop_requested_.store(false);
    
    // Create receive task
    BaseType_t result = xTaskCreatePinnedToCore(
        ReceiveTaskWrapper,
        "can_rx_task",
        CAN_TASK_STACK_SIZE,
        this,
        CAN_TASK_PRIORITY,
        &receive_task_handle_,
        CAN_TASK_CORE
    );
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create receive task");
        twai_stop();
        state_.store(CanDriverState::ERROR);
        return false;
    }
    
    // Create idle monitor task (for power saving)
    result = xTaskCreatePinnedToCore(
        IdleMonitorTaskWrapper,
        "can_idle_task",
        2048,
        this,
        CAN_TASK_PRIORITY - 2,  // Lower priority than receive task
        &idle_monitor_task_handle_,
        CAN_TASK_CORE
    );
    
    if (result != pdPASS) {
        ESP_LOGW(TAG, "Failed to create idle monitor task (power saving disabled)");
        // Continue without idle monitoring
    }
    
    state_.store(CanDriverState::RUNNING);
    ESP_LOGI(TAG, "CAN Bus Driver started successfully");
    
    return true;
}

bool CanBusDriver::Stop() {
    ESP_LOGI(TAG, "Stopping CAN Bus Driver");
    
    stop_requested_.store(true);
    
    // Wait for tasks to finish
    if (receive_task_handle_) {
        // Give task time to clean up
        vTaskDelay(pdMS_TO_TICKS(100));
        if (eTaskGetState(receive_task_handle_) != eDeleted) {
            vTaskDelete(receive_task_handle_);
        }
        receive_task_handle_ = nullptr;
    }
    
    if (idle_monitor_task_handle_) {
        vTaskDelay(pdMS_TO_TICKS(50));
        if (eTaskGetState(idle_monitor_task_handle_) != eDeleted) {
            vTaskDelete(idle_monitor_task_handle_);
        }
        idle_monitor_task_handle_ = nullptr;
    }
    
    // Stop TWAI driver
    esp_err_t err = twai_stop();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error stopping TWAI: %s", esp_err_to_name(err));
    }
    
    state_.store(CanDriverState::STOPPED);
    ESP_LOGI(TAG, "CAN Bus Driver stopped");
    
    return true;
}

void CanBusDriver::Deinitialize() {
    ESP_LOGI(TAG, "Deinitializing CAN Bus Driver");
    
    if (state_.load() == CanDriverState::RUNNING || state_.load() == CanDriverState::POWER_SAVE) {
        Stop();
    }
    
    if (is_initialized_) {
        esp_err_t err = twai_driver_uninstall();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Error uninstalling TWAI driver: %s", esp_err_to_name(err));
        }
        is_initialized_ = false;
    }
    
    ClearCallbacks();
    
    state_.store(CanDriverState::UNINITIALIZED);
    ESP_LOGI(TAG, "CAN Bus Driver deinitialized");
}

// ============================================================================
// Send Messages
// ============================================================================

bool CanBusDriver::SendMessage(const CanMessage& msg, uint32_t timeout_ms) {
    if (!IsReady()) {
        ESP_LOGW(TAG, "Cannot send - driver not ready");
        return false;
    }
    
    twai_message_t twai_msg;
    memset(&twai_msg, 0, sizeof(twai_msg));
    
    twai_msg.identifier = msg.id;
    twai_msg.data_length_code = msg.length;
    twai_msg.extd = msg.is_extended ? 1 : 0;
    twai_msg.rtr = msg.is_rtr ? 1 : 0;
    memcpy(twai_msg.data, msg.data, msg.length);
    
    esp_err_t err = twai_transmit(&twai_msg, pdMS_TO_TICKS(timeout_ms));
    
    if (err == ESP_OK) {
        if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
            stats_.tx_count++;
            xSemaphoreGive(stats_mutex_);
        }
        ESP_LOGD(TAG, "Message sent: ID=0x%03lX, Len=%d", msg.id, msg.length);
        return true;
    } else {
        ESP_LOGE(TAG, "Failed to send message: %s", esp_err_to_name(err));
        if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
            stats_.error_count++;
            xSemaphoreGive(stats_mutex_);
        }
        return false;
    }
}

bool CanBusDriver::SendMessage(uint32_t id, const uint8_t* data, uint8_t length, uint32_t timeout_ms) {
    CanMessage msg;
    msg.id = id;
    msg.length = (length > 8) ? 8 : length;
    msg.is_extended = false;
    msg.is_rtr = false;
    msg.timestamp_ms = esp_timer_get_time() / 1000;
    
    if (data && length > 0) {
        memcpy(msg.data, data, msg.length);
    }
    
    return SendMessage(msg, timeout_ms);
}

// ============================================================================
// Callbacks
// ============================================================================

void CanBusDriver::RegisterCallback(CanMessageCallback callback) {
    if (!callback) return;
    
    if (xSemaphoreTake(callbacks_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        callbacks_.push_back(callback);
        ESP_LOGI(TAG, "Callback registered, total callbacks: %d", (int)callbacks_.size());
        xSemaphoreGive(callbacks_mutex_);
    }
}

void CanBusDriver::ClearCallbacks() {
    if (xSemaphoreTake(callbacks_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        callbacks_.clear();
        ESP_LOGI(TAG, "All callbacks cleared");
        xSemaphoreGive(callbacks_mutex_);
    }
}

// ============================================================================
// Filters
// ============================================================================

bool CanBusDriver::AddFilter(uint32_t id, uint32_t mask) {
    ESP_LOGI(TAG, "Adding filter: ID=0x%03lX, Mask=0x%03lX", id, mask);
    // Note: ESP32 TWAI only supports one hardware filter
    // For multiple filters, implement software filtering in ProcessReceivedMessage
    // This would require restarting the driver with new filter config
    ESP_LOGW(TAG, "Hardware filter update requires driver restart");
    return true;
}

void CanBusDriver::ClearFilters() {
    ESP_LOGI(TAG, "Filters cleared (accepting all messages)");
}

// ============================================================================
// Power Saving
// ============================================================================

void CanBusDriver::EnterPowerSaveMode() {
    if (state_.load() != CanDriverState::RUNNING) {
        ESP_LOGW(TAG, "Cannot enter power save - not running");
        return;
    }
    
    ESP_LOGI(TAG, "Entering power save mode");
    
    // Stop current operation
    twai_stop();
    
    // Reconfigure for listen-only mode (lower power)
    twai_driver_uninstall();
    
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(tx_gpio_, rx_gpio_, TWAI_MODE_LISTEN_ONLY);
    g_config.rx_queue_len = CAN_RX_QUEUE_SIZE;
    twai_timing_config_t t_config = GetTimingConfig(speed_kbps_);
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err == ESP_OK) {
        twai_start();
        state_.store(CanDriverState::POWER_SAVE);
        ESP_LOGI(TAG, "Power save mode activated (listen-only)");
    } else {
        ESP_LOGE(TAG, "Failed to enter power save mode: %s", esp_err_to_name(err));
        // Try to recover
        ConfigureTwai(tx_gpio_, rx_gpio_, speed_kbps_);
        twai_start();
    }
}

void CanBusDriver::ExitPowerSaveMode() {
    if (state_.load() != CanDriverState::POWER_SAVE) {
        return;
    }
    
    ESP_LOGI(TAG, "Exiting power save mode");
    
    // Stop and reconfigure for normal mode
    twai_stop();
    twai_driver_uninstall();
    
    if (ConfigureTwai(tx_gpio_, rx_gpio_, speed_kbps_)) {
        twai_start();
        state_.store(CanDriverState::RUNNING);
        ESP_LOGI(TAG, "Normal mode restored");
    } else {
        ESP_LOGE(TAG, "Failed to exit power save mode");
        state_.store(CanDriverState::ERROR);
    }
}

// ============================================================================
// Status & Statistics
// ============================================================================

CanDriverState CanBusDriver::GetState() const {
    return state_.load();
}

CanStats CanBusDriver::GetStats() const {
    CanStats stats_copy;
    
    if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        stats_copy = stats_;
        stats_copy.state = state_.load();
        xSemaphoreGive(stats_mutex_);
    } else {
        memset(&stats_copy, 0, sizeof(stats_copy));
        stats_copy.state = state_.load();
    }
    
    return stats_copy;
}

bool CanBusDriver::IsReady() const {
    CanDriverState current = state_.load();
    return current == CanDriverState::RUNNING || current == CanDriverState::POWER_SAVE;
}

int64_t CanBusDriver::GetTimeSinceLastMessage() const {
    int64_t last_rx = last_rx_timestamp_.load();
    if (last_rx == 0) return -1;
    
    int64_t now = esp_timer_get_time() / 1000;
    return now - last_rx;
}

// ============================================================================
// Error Recovery
// ============================================================================

bool CanBusDriver::RecoverFromError() {
    ESP_LOGI(TAG, "Attempting to recover from error state");
    
    state_.store(CanDriverState::RECOVERING);
    
    // Stop everything
    if (receive_task_handle_) {
        stop_requested_.store(true);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    twai_stop();
    twai_driver_uninstall();
    
    // Reinitialize
    if (ConfigureTwai(tx_gpio_, rx_gpio_, speed_kbps_)) {
        if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
            stats_.error_count++;
            stats_.last_error_timestamp = esp_timer_get_time() / 1000;
            xSemaphoreGive(stats_mutex_);
        }
        
        if (Start()) {
            ESP_LOGI(TAG, "Recovery successful");
            return true;
        }
    }
    
    ESP_LOGE(TAG, "Recovery failed");
    state_.store(CanDriverState::ERROR);
    return false;
}

// ============================================================================
// Tasks
// ============================================================================

void CanBusDriver::ReceiveTaskWrapper(void* arg) {
    CanBusDriver* driver = static_cast<CanBusDriver*>(arg);
    driver->ReceiveTask();
}

void CanBusDriver::ReceiveTask() {
    ESP_LOGI(TAG, "Receive task started");
    
    twai_message_t twai_msg;
    uint32_t alerts;
    
    while (!stop_requested_.load()) {
        // Check for alerts (errors, bus events)
        if (twai_read_alerts(&alerts, pdMS_TO_TICKS(10)) == ESP_OK) {
            if (alerts != 0) {
                HandleAlerts(alerts);
            }
        }
        
        // Try to receive a message
        esp_err_t err = twai_receive(&twai_msg, pdMS_TO_TICKS(50));
        
        if (err == ESP_OK) {
            ProcessReceivedMessage(twai_msg);
        } else if (err != ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "Receive error: %s", esp_err_to_name(err));
        }
        
        // Update status periodically
        static int update_counter = 0;
        if (++update_counter >= 100) {  // Every ~5 seconds
            update_counter = 0;
            twai_status_info_t status;
            if (twai_get_status_info(&status) == ESP_OK) {
                UpdateStats(status);
            }
        }
    }
    
    ESP_LOGI(TAG, "Receive task ended");
    vTaskDelete(NULL);
}

void CanBusDriver::IdleMonitorTaskWrapper(void* arg) {
    CanBusDriver* driver = static_cast<CanBusDriver*>(arg);
    driver->IdleMonitorTask();
}

void CanBusDriver::IdleMonitorTask() {
    ESP_LOGI(TAG, "Idle monitor task started");
    
    while (!stop_requested_.load()) {
        vTaskDelay(pdMS_TO_TICKS(CAN_POWER_SAVE_CHECK_MS));
        
        if (state_.load() == CanDriverState::RUNNING) {
            int64_t idle_time = GetTimeSinceLastMessage();
            
            if (idle_time > 0 && idle_time >= CAN_IDLE_TIMEOUT_MS) {
                ESP_LOGI(TAG, "No CAN traffic for %lld ms, entering power save mode", idle_time);
                EnterPowerSaveMode();
            }
        } else if (state_.load() == CanDriverState::POWER_SAVE) {
            int64_t idle_time = GetTimeSinceLastMessage();
            
            // If we receive a message in power save mode, exit it
            if (idle_time >= 0 && idle_time < CAN_POWER_SAVE_CHECK_MS * 2) {
                ESP_LOGI(TAG, "CAN traffic detected, exiting power save mode");
                ExitPowerSaveMode();
            }
        }
    }
    
    ESP_LOGI(TAG, "Idle monitor task ended");
    vTaskDelete(NULL);
}

void CanBusDriver::ProcessReceivedMessage(const twai_message_t& twai_msg) {
    // Create our message structure
    CanMessage msg;
    msg.id = twai_msg.identifier;
    msg.length = twai_msg.data_length_code;
    msg.is_extended = twai_msg.extd;
    msg.is_rtr = twai_msg.rtr;
    msg.timestamp_ms = esp_timer_get_time() / 1000;
    memcpy(msg.data, twai_msg.data, 8);
    
    // Update timestamp
    last_rx_timestamp_.store(msg.timestamp_ms);
    
    // Update stats
    if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
        stats_.rx_count++;
        stats_.last_rx_timestamp = msg.timestamp_ms;
        xSemaphoreGive(stats_mutex_);
    }
    
    // Log message (debug level)
    ESP_LOGD(TAG, "RX: ID=0x%03lX Len=%d Data=[%02X %02X %02X %02X %02X %02X %02X %02X]",
             msg.id, msg.length,
             msg.data[0], msg.data[1], msg.data[2], msg.data[3],
             msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
    
    // Call registered callbacks
    if (xSemaphoreTake(callbacks_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        for (auto& callback : callbacks_) {
            if (callback) {
                callback(msg);
            }
        }
        xSemaphoreGive(callbacks_mutex_);
    }
}

void CanBusDriver::HandleAlerts(uint32_t alerts) {
    if (alerts & TWAI_ALERT_ERR_PASS) {
        ESP_LOGW(TAG, "Alert: Error passive state entered");
    }
    if (alerts & TWAI_ALERT_BUS_ERROR) {
        ESP_LOGE(TAG, "Alert: Bus error occurred");
        if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
            stats_.error_count++;
            stats_.last_error_timestamp = esp_timer_get_time() / 1000;
            xSemaphoreGive(stats_mutex_);
        }
    }
    if (alerts & TWAI_ALERT_BUS_OFF) {
        ESP_LOGE(TAG, "Alert: Bus-off state! Attempting recovery...");
        if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
            stats_.bus_off_count++;
            xSemaphoreGive(stats_mutex_);
        }
        
        // Initiate bus recovery
        twai_initiate_recovery();
    }
    if (alerts & TWAI_ALERT_BUS_RECOVERED) {
        ESP_LOGI(TAG, "Alert: Bus recovered from bus-off state");
        twai_start();
    }
    if (alerts & TWAI_ALERT_ARB_LOST) {
        ESP_LOGD(TAG, "Alert: Arbitration lost");
        if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
            stats_.arb_lost_count++;
            xSemaphoreGive(stats_mutex_);
        }
    }
    if (alerts & TWAI_ALERT_RX_QUEUE_FULL) {
        ESP_LOGW(TAG, "Alert: RX queue full, messages may be lost");
    }
}

void CanBusDriver::UpdateStats(const twai_status_info_t& status) {
    ESP_LOGD(TAG, "Status: state=%d, tx_err=%ld, rx_err=%ld, tx_failed=%ld, rx_miss=%ld",
             status.state, status.tx_error_counter, status.rx_error_counter,
             status.tx_failed_count, status.rx_missed_count);
}

} // namespace canbus
