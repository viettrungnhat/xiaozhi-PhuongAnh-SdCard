#!/usr/bin/env python3
"""
Flash Assets Partition cho ESP32

Script này flash file assets.bin vào partition assets của ESP32
mà không cần flash lại toàn bộ firmware.

Sử dụng:
    python flash_assets.py --port COM5
    python flash_assets.py --port COM5 --baud 921600
    python flash_assets.py --port COM5 --assets-bin ./build/assets.bin
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path


# Cấu hình mặc định
DEFAULT_ASSETS_BIN = "build/assets.bin"
DEFAULT_CHIP = "esp32s3"
DEFAULT_BAUD = 921600
DEFAULT_FLASH_SIZE = "16m"  # ESP32-S3 N16R8

# Offset của assets partition (từ partitions/v2/*.csv)
# Đọc từ file partition table thực tế
PARTITION_OFFSETS = {
    "4m": 0x280000,    # partitions/v2/4m.csv: assets offset=0x280000, size=1.5MB
    "8m": 0x600000,    # partitions/v2/8m.csv: assets offset=0x600000, size=2MB
    "16m": 0x7D0000,   # partitions/v2/16m.csv: assets offset=0x7D0000, size=2.2MB
    "16m_c3": 0x800000, # partitions/v2/16m_c3.csv: assets offset=0x800000, size=4MB (ESP32-C3/C6)
    "custom": 0x800000, # Offset tùy chỉnh của user (nếu đã sửa partition table)
}

# NVS partition (chung cho tất cả)
NVS_OFFSET = 0x9000
NVS_SIZE = 0x4000  # 16KB


def check_esptool():
    """Kiểm tra esptool đã được cài đặt chưa"""
    try:
        result = subprocess.run(
            [sys.executable, "-m", "esptool", "version"],
            capture_output=True,
            text=True
        )
        return result.returncode == 0
    except Exception:
        return False


def list_serial_ports():
    """Liệt kê các cổng COM có sẵn"""
    try:
        import serial.tools.list_ports
        ports = serial.tools.list_ports.comports()
        return [(p.device, p.description) for p in ports]
    except ImportError:
        return []


def flash_assets(assets_bin: str, port: str, chip: str, baud: int, offset: int):
    """
    Flash file assets.bin vào ESP32
    
    Args:
        assets_bin: Đường dẫn đến file assets.bin
        port: Cổng COM (VD: COM5, /dev/ttyUSB0)
        chip: Loại chip (esp32, esp32s3, ...)
        baud: Tốc độ baud
        offset: Địa chỉ offset của partition
    """
    if not os.path.exists(assets_bin):
        print(f"❌ Không tìm thấy file: {assets_bin}")
        return False
    
    file_size = os.path.getsize(assets_bin)
    print(f"📦 File: {assets_bin}")
    print(f"   Kích thước: {file_size / 1024:.1f} KB")
    print(f"   Offset: 0x{offset:X}")
    print(f"   Port: {port}")
    print(f"   Chip: {chip}")
    print(f"   Baud: {baud}")
    print("-" * 50)
    
    # Build esptool command
    cmd = [
        sys.executable, "-m", "esptool",
        "--chip", chip,
        "--port", port,
        "--baud", str(baud),
        "write_flash",
        f"0x{offset:X}",
        assets_bin
    ]
    
    print(f"🚀 Đang flash...")
    print(f"   Command: {' '.join(cmd)}")
    print()
    
    try:
        result = subprocess.run(cmd, check=True)
        return result.returncode == 0
    except subprocess.CalledProcessError as e:
        print(f"❌ Flash thất bại: {e}")
        return False
    except FileNotFoundError:
        print("❌ Không tìm thấy esptool. Cài đặt với: pip install esptool")
        return False


def main():
    parser = argparse.ArgumentParser(
        description="Flash Assets Partition cho ESP32"
    )
    parser.add_argument(
        "--port", "-p",
        type=str,
        required=True,
        help="Cổng COM (VD: COM5, /dev/ttyUSB0)"
    )
    parser.add_argument(
        "--assets-bin",
        type=str,
        default=DEFAULT_ASSETS_BIN,
        help=f"Đường dẫn đến assets.bin (mặc định: {DEFAULT_ASSETS_BIN})"
    )
    parser.add_argument(
        "--chip",
        type=str,
        default=DEFAULT_CHIP,
        choices=["esp32", "esp32s2", "esp32s3", "esp32c3", "esp32c6"],
        help=f"Loại chip (mặc định: {DEFAULT_CHIP})"
    )
    parser.add_argument(
        "--baud", "-b",
        type=int,
        default=DEFAULT_BAUD,
        help=f"Tốc độ baud (mặc định: {DEFAULT_BAUD})"
    )
    parser.add_argument(
        "--flash-size",
        type=str,
        default=DEFAULT_FLASH_SIZE,
        choices=list(PARTITION_OFFSETS.keys()),
        help=f"Kích thước flash để xác định offset (mặc định: {DEFAULT_FLASH_SIZE} cho ESP32-S3 N16R8)"
    )
    parser.add_argument(
        "--offset",
        type=str,
        help="Địa chỉ offset tùy chỉnh (VD: 0x600000)"
    )
    parser.add_argument(
        "--list-ports",
        action="store_true",
        help="Liệt kê các cổng COM có sẵn"
    )
    
    args = parser.parse_args()
    
    # Liệt kê ports
    if args.list_ports:
        ports = list_serial_ports()
        if ports:
            print("\n📡 Các cổng COM có sẵn:")
            for device, desc in ports:
                print(f"   {device}: {desc}")
        else:
            print("❌ Không tìm thấy cổng COM nào")
            print("   Cài đặt pyserial: pip install pyserial")
        return
    
    # Kiểm tra esptool
    if not check_esptool():
        print("❌ esptool chưa được cài đặt")
        print("   Cài đặt với: pip install esptool")
        sys.exit(1)
    
    # Xác định offset
    if args.offset:
        offset = int(args.offset, 16)
    else:
        offset = PARTITION_OFFSETS[args.flash_size]
    
    print("\n🔌 Flash Assets Partition")
    print("=" * 50)
    
    # Flash
    success = flash_assets(
        assets_bin=args.assets_bin,
        port=args.port,
        chip=args.chip,
        baud=args.baud,
        offset=offset
    )
    
    if success:
        print("\n" + "=" * 50)
        print("✅ Flash thành công!")
        print("   Restart thiết bị để áp dụng assets mới.")
    else:
        print("\n❌ Flash thất bại!")
        print("   Kiểm tra:")
        print("   - Đúng cổng COM")
        print("   - Thiết bị đã kết nối")
        print("   - Thử giảm baud xuống 115200")
        sys.exit(1)


if __name__ == "__main__":
    main()
