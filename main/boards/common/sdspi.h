#ifndef XIAOZHI_SDSPI_H_
#define XIAOZHI_SDSPI_H_

#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include "sd_card.h"
#include "sd_card_common.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

// Default GPIO pins for SPI SD card interface
#ifndef DEFAULT_SPI_MOSI_GPIO
#define DEFAULT_SPI_MOSI_GPIO GPIO_NUM_39
#endif
#ifndef DEFAULT_SPI_MISO_GPIO
#define DEFAULT_SPI_MISO_GPIO GPIO_NUM_41
#endif
#ifndef DEFAULT_SPI_SCLK_GPIO
#define DEFAULT_SPI_SCLK_GPIO GPIO_NUM_40
#endif
#ifndef DEFAULT_SPI_CS_GPIO
#define DEFAULT_SPI_CS_GPIO GPIO_NUM_38
#endif

class SdSPI : public SdCard {
 public:
  struct Config {
    const char* mount_point = kSdCardMountPoint;
    bool format_if_mount_failed = false;
    int max_files = kSdCardMaxFiles;
    size_t allocation_unit_size = kSdCardAllocationUnitSize;
    gpio_num_t mosi_pin = DEFAULT_SPI_MOSI_GPIO;
    gpio_num_t miso_pin = DEFAULT_SPI_MISO_GPIO;
    gpio_num_t sclk_pin = DEFAULT_SPI_SCLK_GPIO;
    gpio_num_t cs_pin = DEFAULT_SPI_CS_GPIO;
    spi_host_device_t host_id = SPI2_HOST;
    int freq_khz = 10000;  // 10 MHz - reduced for better stability
  };

  SdSPI();
  explicit SdSPI(const Config& config);
  explicit SdSPI(gpio_num_t mosi_pin,
                 gpio_num_t miso_pin,
                 gpio_num_t sclk_pin,
                 gpio_num_t cs_pin,
                 spi_host_device_t host_id = SPI2_HOST,
                 int freq_khz = 10000,
                 const char* mount_point = kSdCardMountPoint,
                 bool format_if_mount_failed = false,
                 int max_files = kSdCardMaxFiles,
                 size_t allocation_unit_size = kSdCardAllocationUnitSize);
  ~SdSPI();

  // Disable copy and assign
  SdSPI(const SdSPI&) = delete;
  SdSPI& operator=(const SdSPI&) = delete;

  // Initialize and mount the SD card
  esp_err_t Initialize() override;

  // Unmount and deinitialize the SD card
  esp_err_t Deinitialize() override;

  // Get mount point path
  const char* GetMountPoint() const override { return config_.mount_point; }

  // Get card information
  const sdmmc_card_t* GetCardInfo() const { return card_; }

  // Print card information to stdout
  void PrintCardInfo() const override;

  // File operations
  esp_err_t WriteFile(const char* path, const char* data) override;
  esp_err_t ReadFile(const char* path, char* buffer, size_t buffer_size) override;
  esp_err_t DeleteFile(const char* path) override;
  esp_err_t RenameFile(const char* old_path, const char* new_path) override;
  bool FileExists(const char* path) override;
  // Format the SD card
  esp_err_t Format() override;

 private:
  Config config_;
  sdmmc_card_t* card_;
  bool spi_bus_initialized_;

  static constexpr const char* kTag = "SdSPI";
};

#endif  // XIAOZHI_SDSPI_H_
