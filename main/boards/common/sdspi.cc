#include "sdspi.h"
#include "esp_log.h"
#include "driver/gpio.h"

SdSPI::SdSPI() : card_(nullptr), spi_bus_initialized_(false) {
  config_ = Config();
}

SdSPI::SdSPI(const Config& config)
    : config_(config), card_(nullptr), spi_bus_initialized_(false) {}

SdSPI::SdSPI(gpio_num_t mosi_pin,
             gpio_num_t miso_pin,
             gpio_num_t sclk_pin,
             gpio_num_t cs_pin,
             spi_host_device_t host_id,
             int freq_khz,
             const char* mount_point,
             bool format_if_mount_failed,
             int max_files,
             size_t allocation_unit_size)
    : config_{mount_point, format_if_mount_failed, max_files, allocation_unit_size, mosi_pin, miso_pin, sclk_pin, cs_pin, host_id, freq_khz},
      card_(nullptr),
      spi_bus_initialized_(false) {}

SdSPI::~SdSPI() {
  if (is_mounted_) {
    Deinitialize();
  }
}

esp_err_t SdSPI::Initialize() {
  if (is_mounted_) {
    ESP_LOGW(kTag, "SD card already mounted");
    return ESP_OK;
  }

  ESP_LOGI(kTag, "Initializing SD card (SPI mode)");
  ESP_LOGI(kTag, "SPI Config - MOSI: %d, MISO: %d, SCLK: %d, CS: %d", 
           config_.mosi_pin, config_.miso_pin, config_.sclk_pin, config_.cs_pin);
  ESP_LOGI(kTag, "SPI Freq: %d kHz, Host ID: %d", config_.freq_khz, config_.host_id);

  // Longer delay for SD card power stabilization - some cards need 200-500ms
  ESP_LOGI(kTag, "Waiting for SD card power stabilization...");
  vTaskDelay(pdMS_TO_TICKS(300));

  // Configure GPIO pins with pull-up resistors for better signal integrity
  ESP_LOGI(kTag, "Configuring GPIO pull-ups for SD card pins");
  gpio_config_t gpio_cfg = {
      .pin_bit_mask = (1ULL << config_.miso_pin) | (1ULL << config_.cs_pin),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&gpio_cfg);
  ESP_LOGI(kTag, "GPIO pull-ups configured: MISO (pin %d) and CS (pin %d)", 
           config_.miso_pin, config_.cs_pin);

  // Initialize SPI bus if not already done
  if (!spi_bus_initialized_) {
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = config_.mosi_pin,
        .miso_io_num = config_.miso_pin,
        .sclk_io_num = config_.sclk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ESP_LOGI(kTag, "Initializing SPI bus with host %d", config_.host_id);
    esp_err_t ret = spi_bus_initialize((spi_host_device_t)config_.host_id, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
      ESP_LOGE(kTag, "Failed to initialize SPI bus: %s (0x%x)", esp_err_to_name(ret), ret);
      return ret;
    }
    ESP_LOGI(kTag, "SPI bus initialized successfully");
    spi_bus_initialized_ = true;
  }

  // Mount configuration
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = config_.format_if_mount_failed,
      .max_files = config_.max_files,
      .allocation_unit_size = config_.allocation_unit_size,
      .disk_status_check_enable = false};

  // Slot configuration for SPI
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = config_.cs_pin;
  slot_config.host_id = (spi_host_device_t)config_.host_id;

  // Retry loop for SD card mount - handles intermittent detection issues
  // Start with lower frequency and increase if successful
  const int max_retries = 5;
  const int init_frequencies[] = {5000, 5000, 10000, 10000, config_.freq_khz};  // Start slow, then normal
  esp_err_t ret = ESP_FAIL;
  
  for (int attempt = 1; attempt <= max_retries; attempt++) {
    // Use lower frequency for first attempts to improve reliability
    int freq_khz = init_frequencies[attempt - 1];
    
    // Host configuration for SPI - recreate for each attempt with different frequency
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = freq_khz;
    
    // Toggle CS pin to reset SD card before each attempt
    gpio_set_direction(config_.cs_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(config_.cs_pin, 1);  // Deselect
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(config_.cs_pin, 0);  // Select
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(config_.cs_pin, 1);  // Deselect again
    vTaskDelay(pdMS_TO_TICKS(50));
    
    ESP_LOGI(kTag, "Attempting to mount SD card at %s (attempt %d/%d, freq=%dkHz)", 
             config_.mount_point, attempt, max_retries, freq_khz);
    
    ret = esp_vfs_fat_sdspi_mount(config_.mount_point, &host,
                                   &slot_config, &mount_config, &card_);

    if (ret == ESP_OK) {
      ESP_LOGI(kTag, "SD card mounted successfully at %d kHz", freq_khz);
      break;  // Success!
    }
    
    // Log warning and retry
    ESP_LOGW(kTag, "SD card mount attempt %d failed: %s (0x%x)", 
             attempt, esp_err_to_name(ret), ret);
    
    if (attempt < max_retries) {
      int delay_ms = 300 + (attempt * 200);  // Increasing delay: 500, 700, 900, 1100ms
      ESP_LOGI(kTag, "Waiting %dms before retry...", delay_ms);
      vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
  }

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(kTag,
               "Failed to mount filesystem (ESP_FAIL) after %d attempts. "
               "Consider setting format_if_mount_failed option.", max_retries);
    } else {
      ESP_LOGE(kTag,
               "Failed to initialize the card after %d attempts (Error: %s, 0x%x). "
               "Make sure: SD card is inserted, "
               "pins are correct (MOSI:%d, MISO:%d, SCLK:%d, CS:%d), "
               "pull-up resistors in place.",
               max_retries, esp_err_to_name(ret), ret,
               config_.mosi_pin, config_.miso_pin, config_.sclk_pin, config_.cs_pin);
    }
    card_ = nullptr;
    return ret;
  }

  is_mounted_ = true;
  ESP_LOGI(kTag, "Filesystem mounted successfully!");
  ESP_LOGI(kTag, "SD Card Info:");
  PrintCardInfo();

  return ESP_OK;
}

esp_err_t SdSPI::Deinitialize() {
  if (!is_mounted_) {
    ESP_LOGW(kTag, "SD card not mounted");
    return ESP_OK;
  }

  ESP_LOGI(kTag, "Unmounting SD card");
  esp_err_t ret = esp_vfs_fat_sdcard_unmount(config_.mount_point, card_);
  if (ret != ESP_OK) {
    ESP_LOGE(kTag, "Failed to unmount SD card: %s", esp_err_to_name(ret));
    return ret;
  }

  card_ = nullptr;
  is_mounted_ = false;
  ESP_LOGI(kTag, "Card unmounted");

  // Optionally deinitialize SPI bus
  if (spi_bus_initialized_) {
    spi_bus_free((spi_host_device_t)config_.host_id);
    spi_bus_initialized_ = false;
  }

  return ESP_OK;
}

void SdSPI::PrintCardInfo() const {
  if (card_ != nullptr) {
    sdmmc_card_print_info(stdout, card_);
  } else {
    ESP_LOGW(kTag, "No card information available");
  }
}

esp_err_t SdSPI::WriteFile(const char* path, const char* data) {
  if (!is_mounted_) {
    ESP_LOGE(kTag, "SD card not mounted");
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(kTag, "Writing file: %s", path);
  FILE* f = fopen(path, "w");
  if (f == nullptr) {
    ESP_LOGE(kTag, "Failed to open file for writing: %s", path);
    return ESP_FAIL;
  }

  fprintf(f, "%s", data);
  fclose(f);
  ESP_LOGI(kTag, "File written successfully");

  return ESP_OK;
}

esp_err_t SdSPI::ReadFile(const char* path, char* buffer,
                           size_t buffer_size) {
  if (!is_mounted_) {
    ESP_LOGE(kTag, "SD card not mounted");
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(kTag, "Reading file: %s", path);
  FILE* f = fopen(path, "r");
  if (f == nullptr) {
    ESP_LOGE(kTag, "Failed to open file for reading: %s", path);
    return ESP_FAIL;
  }

  if (fgets(buffer, buffer_size, f) == nullptr) {
    ESP_LOGE(kTag, "Failed to read file: %s", path);
    fclose(f);
    return ESP_FAIL;
  }

  fclose(f);

  // Strip newline
  char* pos = strchr(buffer, '\n');
  if (pos != nullptr) {
    *pos = '\0';
  }

  ESP_LOGI(kTag, "Read from file: '%s'", buffer);
  return ESP_OK;
}

esp_err_t SdSPI::DeleteFile(const char* path) {
  if (!is_mounted_) {
    ESP_LOGE(kTag, "SD card not mounted");
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(kTag, "Deleting file: %s", path);
  if (unlink(path) != 0) {
    ESP_LOGE(kTag, "Failed to delete file: %s", path);
    return ESP_FAIL;
  }

  ESP_LOGI(kTag, "File deleted successfully");
  return ESP_OK;
}

esp_err_t SdSPI::RenameFile(const char* old_path, const char* new_path) {
  if (!is_mounted_) {
    ESP_LOGE(kTag, "SD card not mounted");
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(kTag, "Renaming file from %s to %s", old_path, new_path);

  // Check if destination file exists
  struct stat st;
  if (stat(new_path, &st) == 0) {
    ESP_LOGI(kTag, "Destination file exists, deleting it first");
    unlink(new_path);
  }

  if (rename(old_path, new_path) != 0) {
    ESP_LOGE(kTag, "Failed to rename file");
    return ESP_FAIL;
  }

  ESP_LOGI(kTag, "File renamed successfully");
  return ESP_OK;
}

bool SdSPI::FileExists(const char* path) {
  struct stat st;
  return stat(path, &st) == 0;
}

esp_err_t SdSPI::Format() {
  if (!is_mounted_) {
    ESP_LOGE(kTag, "SD card not mounted");
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(kTag, "Formatting SD card");
  esp_err_t ret = esp_vfs_fat_sdcard_format(config_.mount_point, card_);
  if (ret != ESP_OK) {
    ESP_LOGE(kTag, "Failed to format SD card: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI(kTag, "SD card formatted successfully");
  return ESP_OK;
}
