#ifndef OFFLINE_AUDIO_PLAYER_H
#define OFFLINE_AUDIO_PLAYER_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <esp_log.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../config.h"

namespace offline {

/**
 * @brief Offline Audio Player - Phát âm thanh Opus từ thẻ SD khi không có WiFi
 * 
 * Cho phép chatbot phát các file âm thanh đã ghi sẵn để:
 * - Chào hỏi, cảnh báo, thông báo khi offline
 * - Đọc thông số xe từ CAN bus mà không cần TTS online
 * - Phát nhạc nền từ thẻ SD
 */
class OfflineAudioPlayer {
public:
    // Các category âm thanh
    enum class AudioCategory {
        SYSTEM,         // Beep, chime
        GREETINGS,      // Lời chào
        WARNINGS,       // Cảnh báo
        HIGHWAY,        // Chế độ đường trường
        CONTROL,        // Điều khiển (cốp, AC)
        INFO,           // Thông tin xe
        NUMBERS,        // Số đọc
        CUSTOM          // Tùy chỉnh
    };
    
    // Singleton pattern
    static OfflineAudioPlayer& GetInstance() {
        static OfflineAudioPlayer instance;
        return instance;
    }
    
    /**
     * @brief Khởi tạo player với đường dẫn thẻ SD
     * @param sd_mount_point Điểm mount của thẻ SD (vd: "/sdcard")
     * @return true nếu thành công
     */
    bool Initialize(const std::string& sd_mount_point) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        sd_mount_point_ = sd_mount_point;
        audio_base_path_ = sd_mount_point + OFFLINE_AUDIO_PATH;
        
        // Kiểm tra thư mục audio tồn tại
        struct stat st;
        if (stat(audio_base_path_.c_str(), &st) != 0) {
            ESP_LOGW(TAG, "Audio folder not found: %s", audio_base_path_.c_str());
            ESP_LOGI(TAG, "Please copy audio_opus folder to SD card");
            is_initialized_ = false;
            return false;
        }
        
        // Load danh sách file
        LoadAudioFiles();
        
        is_initialized_ = true;
        ESP_LOGI(TAG, "Offline Audio Player initialized, found %d audio files", 
                 (int)audio_files_.size());
        return true;
    }
    
    /**
     * @brief Phát file âm thanh theo tên
     * @param filename Tên file (không cần path đầy đủ)
     * @return true nếu thành công
     */
    bool Play(const std::string& filename) {
        if (!is_initialized_) {
            ESP_LOGW(TAG, "Player not initialized");
            return false;
        }
        
        std::string full_path = FindAudioFile(filename);
        if (full_path.empty()) {
            ESP_LOGW(TAG, "Audio file not found: %s", filename.c_str());
            return false;
        }
        
        // TODO: Integrate with AudioService to play opus file
        ESP_LOGI(TAG, "Playing: %s", full_path.c_str());
        current_file_ = filename;
        
        return true;
    }
    
    /**
     * @brief Phát file âm thanh theo category và tên
     */
    bool PlayFromCategory(AudioCategory category, const std::string& filename) {
        std::string folder = GetCategoryFolder(category);
        std::string full_name = folder + "/" + filename;
        return Play(full_name);
    }
    
    // === Convenience methods cho các âm thanh thường dùng ===
    
    // Lời chào
    bool PlayGreetingMorning() { return PlayFromCategory(AudioCategory::GREETINGS, "greeting_morning.opus"); }
    bool PlayGreetingAfternoon() { return PlayFromCategory(AudioCategory::GREETINGS, "greeting_afternoon.opus"); }
    bool PlayGreetingEvening() { return PlayFromCategory(AudioCategory::GREETINGS, "greeting_evening.opus"); }
    bool PlayGreetingDefault() { return PlayFromCategory(AudioCategory::GREETINGS, "greeting_default.opus"); }
    bool PlayGoodbye() { return PlayFromCategory(AudioCategory::GREETINGS, "goodbye.opus"); }
    
    // Cảnh báo an toàn
    bool PlayWarnSeatbelt() { return PlayFromCategory(AudioCategory::WARNINGS, "warn_seatbelt.opus"); }
    bool PlayWarnSeatbeltUrgent() { return PlayFromCategory(AudioCategory::WARNINGS, "warn_seatbelt_urgent.opus"); }
    bool PlayWarnParkingBrake() { return PlayFromCategory(AudioCategory::WARNINGS, "warn_parking_brake.opus"); }
    bool PlayWarnParkingBrakeUrgent() { return PlayFromCategory(AudioCategory::WARNINGS, "warn_parking_brake_urgent.opus"); }
    bool PlayWarnDoorOpen() { return PlayFromCategory(AudioCategory::WARNINGS, "warn_door_open.opus"); }
    bool PlayWarnLightsOn() { return PlayFromCategory(AudioCategory::WARNINGS, "warn_lights_on.opus"); }
    
    // Cảnh báo ắc quy
    bool PlayBatteryLow() { return PlayFromCategory(AudioCategory::WARNINGS, "battery_low.opus"); }
    bool PlayBatteryCritical() { return PlayFromCategory(AudioCategory::WARNINGS, "battery_critical.opus"); }
    
    // Cảnh báo nhiệt độ
    bool PlayTempHigh() { return PlayFromCategory(AudioCategory::WARNINGS, "temp_high.opus"); }
    bool PlayTempCritical() { return PlayFromCategory(AudioCategory::WARNINGS, "temp_critical.opus"); }
    bool PlayTempNormal() { return PlayFromCategory(AudioCategory::WARNINGS, "temp_normal.opus"); }
    
    // Cảnh báo xăng
    bool PlayFuelLow() { return PlayFromCategory(AudioCategory::WARNINGS, "fuel_low.opus"); }
    bool PlayFuelCritical() { return PlayFromCategory(AudioCategory::WARNINGS, "fuel_critical.opus"); }
    bool PlayFuelReserve() { return PlayFromCategory(AudioCategory::WARNINGS, "fuel_reserve.opus"); }
    
    // Chế độ đường trường
    bool PlayHighwayModeOn() { return PlayFromCategory(AudioCategory::HIGHWAY, "highway_mode_on.opus"); }
    bool PlayHighwayModeOff() { return PlayFromCategory(AudioCategory::HIGHWAY, "highway_mode_off.opus"); }
    bool PlaySpeedAnnouncement(int speed) {
        // Làm tròn về bội số của 10
        int rounded = (speed / 10) * 10;
        if (rounded < 60) rounded = 60;
        if (rounded > 120) rounded = 120;
        
        char filename[32];
        snprintf(filename, sizeof(filename), "speed_%d.opus", rounded);
        return PlayFromCategory(AudioCategory::HIGHWAY, filename);
    }
    bool PlaySpeedOverLimit() { return PlayFromCategory(AudioCategory::HIGHWAY, "speed_over_limit.opus"); }
    bool PlayRestReminder() { return PlayFromCategory(AudioCategory::HIGHWAY, "rest_reminder.opus"); }
    
    // Điều khiển
    bool PlayTrunkOpening() { return PlayFromCategory(AudioCategory::CONTROL, "trunk_opening.opus"); }
    bool PlayTrunkOpened() { return PlayFromCategory(AudioCategory::CONTROL, "trunk_opened.opus"); }
    bool PlayAcOn() { return PlayFromCategory(AudioCategory::CONTROL, "ac_on.opus"); }
    bool PlayAcOff() { return PlayFromCategory(AudioCategory::CONTROL, "ac_off.opus"); }
    bool PlayReadyToGo() { return PlayFromCategory(AudioCategory::CONTROL, "ready_to_go.opus"); }
    
    // Bảo dưỡng
    bool PlayMaintOilChange() { return PlayFromCategory(AudioCategory::WARNINGS, "maint_oil_change.opus"); }
    bool PlayMaintTireCheck() { return PlayFromCategory(AudioCategory::WARNINGS, "maint_tire_check.opus"); }
    bool PlayMaintGeneral() { return PlayFromCategory(AudioCategory::WARNINGS, "maint_general.opus"); }
    
    /**
     * @brief Phát số (ghép các file số lại với nhau)
     * @param number Số cần phát
     */
    bool PlayNumber(int number) {
        if (number < 0 || number > 9999) {
            ESP_LOGW(TAG, "Number out of range: %d", number);
            return false;
        }
        
        // Đơn giản hóa: chỉ phát các số đơn hoặc số tròn chục
        if (number <= 20) {
            char filename[32];
            snprintf(filename, sizeof(filename), "num_%d.opus", number);
            return PlayFromCategory(AudioCategory::NUMBERS, filename);
        } else if (number < 100) {
            int tens = (number / 10) * 10;
            int units = number % 10;
            
            char tens_file[32];
            snprintf(tens_file, sizeof(tens_file), "num_%d.opus", tens);
            PlayFromCategory(AudioCategory::NUMBERS, tens_file);
            
            if (units > 0) {
                char units_file[32];
                snprintf(units_file, sizeof(units_file), "num_%d.opus", units);
                PlayFromCategory(AudioCategory::NUMBERS, units_file);
            }
            return true;
        } else if (number == 100) {
            return PlayFromCategory(AudioCategory::NUMBERS, "num_100.opus");
        }
        
        // Số lớn hơn - cần xử lý phức tạp hơn
        ESP_LOGW(TAG, "Number %d requires complex pronunciation", number);
        return false;
    }
    
    /**
     * @brief Phát thông tin với prefix và đơn vị
     * Ví dụ: "Tốc độ hiện tại là 80 cây số"
     */
    bool PlayInfoWithValue(const std::string& prefix_file, int value, const std::string& unit_file) {
        PlayFromCategory(AudioCategory::INFO, prefix_file);
        PlayNumber(value);
        PlayFromCategory(AudioCategory::INFO, unit_file);
        return true;
    }
    
    // Phát thông tin xe
    bool PlaySpeedInfo(int speed_kmh) {
        return PlayInfoWithValue("info_speed_prefix.opus", speed_kmh, "info_km.opus");
    }
    
    bool PlayFuelInfo(int fuel_percent) {
        return PlayInfoWithValue("info_fuel_prefix.opus", fuel_percent, "info_percent.opus");
    }
    
    bool PlayTempInfo(int temp_celsius) {
        return PlayInfoWithValue("info_temp_prefix.opus", temp_celsius, "info_degrees.opus");
    }
    
    bool PlayBatteryInfo(float voltage) {
        // Chuyển voltage thành số nguyên * 10 (vd: 12.5V -> 125)
        int v = (int)(voltage * 10);
        PlayFromCategory(AudioCategory::INFO, "info_battery_prefix.opus");
        PlayNumber(v / 10);
        PlayFromCategory(AudioCategory::NUMBERS, "num_point.opus");
        PlayNumber(v % 10);
        PlayFromCategory(AudioCategory::INFO, "info_volts.opus");
        return true;
    }
    
    /**
     * @brief Phát lời chào phù hợp với thời gian trong ngày
     */
    bool PlayTimeBasedGreeting() {
        // Lấy giờ hiện tại
        time_t now;
        time(&now);
        struct tm* timeinfo = localtime(&now);
        int hour = timeinfo->tm_hour;
        
        if (hour >= 5 && hour < 12) {
            return PlayGreetingMorning();
        } else if (hour >= 12 && hour < 18) {
            return PlayGreetingAfternoon();
        } else {
            return PlayGreetingEvening();
        }
    }
    
    // Getters
    bool IsInitialized() const { return is_initialized_; }
    std::string GetCurrentFile() const { return current_file_; }
    size_t GetAudioFileCount() const { return audio_files_.size(); }
    
    /**
     * @brief Liệt kê tất cả file audio đã tìm thấy
     */
    std::vector<std::string> ListAudioFiles() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> files;
        for (const auto& pair : audio_files_) {
            files.push_back(pair.first);
        }
        return files;
    }
    
private:
    static constexpr const char* TAG = "OfflineAudio";
    
    OfflineAudioPlayer() = default;
    ~OfflineAudioPlayer() = default;
    OfflineAudioPlayer(const OfflineAudioPlayer&) = delete;
    OfflineAudioPlayer& operator=(const OfflineAudioPlayer&) = delete;
    
    /**
     * @brief Load tất cả file audio từ thư mục
     */
    void LoadAudioFiles() {
        audio_files_.clear();
        
        // Scan các thư mục con
        std::vector<std::string> folders = {
            "system", "greetings", "warnings", "highway", "control", "info", "numbers"
        };
        
        for (const auto& folder : folders) {
            std::string folder_path = audio_base_path_ + "/" + folder;
            ScanFolder(folder_path, folder);
        }
        
        ESP_LOGI(TAG, "Loaded %d audio files from SD card", (int)audio_files_.size());
    }
    
    void ScanFolder(const std::string& folder_path, const std::string& prefix) {
        DIR* dir = opendir(folder_path.c_str());
        if (!dir) {
            ESP_LOGW(TAG, "Cannot open folder: %s", folder_path.c_str());
            return;
        }
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG) {
                std::string filename = entry->d_name;
                // Chỉ lấy file .opus
                if (filename.size() > 5 && 
                    filename.substr(filename.size() - 5) == ".opus") {
                    std::string key = prefix + "/" + filename;
                    std::string full_path = folder_path + "/" + filename;
                    audio_files_[key] = full_path;
                }
            }
        }
        
        closedir(dir);
    }
    
    /**
     * @brief Tìm file audio theo tên
     */
    std::string FindAudioFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Tìm exact match
        auto it = audio_files_.find(filename);
        if (it != audio_files_.end()) {
            return it->second;
        }
        
        // Tìm partial match
        for (const auto& pair : audio_files_) {
            if (pair.first.find(filename) != std::string::npos) {
                return pair.second;
            }
        }
        
        return "";
    }
    
    std::string GetCategoryFolder(AudioCategory category) {
        switch (category) {
            case AudioCategory::SYSTEM: return "system";
            case AudioCategory::GREETINGS: return "greetings";
            case AudioCategory::WARNINGS: return "warnings";
            case AudioCategory::HIGHWAY: return "highway";
            case AudioCategory::CONTROL: return "control";
            case AudioCategory::INFO: return "info";
            case AudioCategory::NUMBERS: return "numbers";
            default: return "";
        }
    }
    
    mutable std::mutex mutex_;
    std::atomic<bool> is_initialized_{false};
    
    std::string sd_mount_point_;
    std::string audio_base_path_;
    std::string current_file_;
    
    // Map: relative_path -> absolute_path
    std::map<std::string, std::string> audio_files_;
};

} // namespace offline

#endif // OFFLINE_AUDIO_PLAYER_H
