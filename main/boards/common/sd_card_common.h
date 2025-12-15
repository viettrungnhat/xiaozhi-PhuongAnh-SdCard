#ifndef XIAOZHI_SD_CARD_COMMON_H_
#define XIAOZHI_SD_CARD_COMMON_H_

// Common constants for SD card interfaces
inline constexpr const char* kSdCardMountPoint = "/sdcard";
inline constexpr int kSdCardMaxFiles = 5;
inline constexpr size_t kSdCardAllocationUnitSize = 16 * 1024;

#endif  // XIAOZHI_SD_CARD_COMMON_H_
