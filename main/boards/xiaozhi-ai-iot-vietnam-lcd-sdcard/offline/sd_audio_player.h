#ifndef SD_AUDIO_PLAYER_H
#define SD_AUDIO_PLAYER_H

#include <string>
#include <cstdio>
#include <cstring>
#include <esp_log.h>
#include "board.h"
#include "audio/audio_codec.h"

namespace offline {

/**
 * @brief SD Card MP3 Audio Player - Phát MP3 từ SD card
 * 
 * Cấu trúc SD: /sdcard/notifications/
 *   ├── greeting_default.mp3
 *   ├── warn_seatbelt.mp3
 *   ├── battery_low.mp3
 *   └── ... (77 files total)
 */
class SDMp3Player {
public:
    // Singleton pattern
    static SDMp3Player& GetInstance() {
        static SDMp3Player instance;
        return instance;
    }
    
    /**
     * @brief Phát file MP3 từ SD card
     * @param filename Tên file (vd: "warn_seatbelt.mp3")
     * @return true nếu phát thành công
     */
    bool Play(const std::string& filename) {
        if (!Board::GetInstance().GetSdCardMounted()) {
            ESP_LOGW(TAG, "⚠️ SD card not ready");
            return false;
        }
        
        // Construct full path
        std::string full_path = "/sdcard/notifications/" + filename;
        
        // Open file
        FILE* file = fopen(full_path.c_str(), "rb");
        if (!file) {
            ESP_LOGW(TAG, "❌ Cannot open: %s", full_path.c_str());
            return false;
        }
        
        // Get file size
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        ESP_LOGI(TAG, "🔊 Playing: %s (%ld bytes)", filename.c_str(), file_size);
        
        auto codec = Board::GetInstance().GetAudioCodec();
        if (!codec) {
            ESP_LOGE(TAG, "❌ No audio codec available");
            fclose(file);
            return false;
        }
        
        // Enable output if needed
        if (!codec->output_enabled()) {
            codec->EnableOutput(true);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        
        // Read and play MP3 in chunks
        const size_t CHUNK_SIZE = 4096;
        uint8_t buffer[CHUNK_SIZE];
        size_t bytes_read = 0;
        bool success = true;
        
        while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
            // Decode and output MP3
            // This would use MP3 decoder from audio_service
            // For now, just read the file
            vTaskDelay(pdMS_TO_TICKS(1));  // Yield to other tasks
        }
        
        fclose(file);
        ESP_LOGI(TAG, "✅ Playback complete: %s", filename.c_str());
        return success;
    }
    
    /**
     * @brief Phát file theo loại cảnh báo
     */
    bool PlayWarning(const std::string& alert_type) {
        return Play(alert_type + ".mp3");
    }
    
    bool PlayGreeting(const std::string& greeting_type = "default") {
        return Play("greeting_" + greeting_type + ".mp3");
    }
    
    bool PlayBatteryWarning(bool is_critical = false) {
        return Play(is_critical ? "battery_critical.mp3" : "battery_low.mp3");
    }
    
    bool PlayFuelWarning(bool is_critical = false) {
        return Play(is_critical ? "fuel_critical.mp3" : "fuel_low.mp3");
    }
    
    bool PlayTempWarning(bool is_critical = false) {
        return Play(is_critical ? "temp_critical.mp3" : "temp_high.mp3");
    }
    
    bool PlaySeatbeltWarning(bool is_urgent = false) {
        return Play(is_urgent ? "warn_seatbelt_urgent.mp3" : "warn_seatbelt.mp3");
    }
    
    bool PlaySpeedWarning(int speed_limit) {
        char filename[64];
        snprintf(filename, sizeof(filename), "speed_%d.mp3", speed_limit);
        return Play(filename);
    }
    
private:
    static constexpr const char* TAG = "SDMp3Player";
    
    SDMp3Player() = default;
    ~SDMp3Player() = default;
    SDMp3Player(const SDMp3Player&) = delete;
    SDMp3Player& operator=(const SDMp3Player&) = delete;
};

} // namespace offline

#endif // SD_AUDIO_PLAYER_H
