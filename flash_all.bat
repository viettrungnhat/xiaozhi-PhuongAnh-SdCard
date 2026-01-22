@echo off
echo ========================================
echo 1. Flash Assets Partition with MP3 files
echo ========================================
python %IDF_PATH%\components\esptool_py\esptool\esptool.py -p COM12 write_flash 0x7D0000 scripts\spiffs_assets\build\assets.bin
if errorlevel 1 (
    echo Assets flash FAILED!
    exit /b 1
)

echo.
echo ========================================
echo 2. Build Firmware with MP3 Decoder
echo ========================================
idf.py build
if errorlevel 1 (
    echo Build FAILED!
    exit /b 1
)

echo.
echo ========================================
echo 3. Flash Firmware
echo ========================================
idf.py -p COM12 flash

echo.
echo ========================================
echo 4. Monitor Output
echo ========================================
idf.py -p COM12 monitor
