#!/usr/bin/env python3
"""
Script flash audio assets vào ESP32 assets partition
"""

import subprocess
import sys
import os
from pathlib import Path

def flash_audio_assets(port=None, baud=921600):
    """Flash audio assets binary vào partition assets"""
    
    script_dir = Path(__file__).parent
    board_dir = script_dir.parent
    
    # Đường dẫn file binary
    assets_bin = script_dir / "assets" / "audio_assets.bin"
    
    if not assets_bin.exists():
        print(f"❌ File binary không tồn tại: {assets_bin}")
        print("Chạy build_audio_assets.py trước!")
        return False
    
    # Tìm partition offset từ partition table
    partition_csv = None
    for csv_file in board_dir.parent.parent.parent.glob("partitions/**/16m.csv"):
        partition_csv = csv_file
        break
    
    if not partition_csv:
        print("❌ Không tìm thấy partition table")
        return False
    
    # Parse partition table để tìm offset của assets
    assets_offset = None
    with open(partition_csv, 'r') as f:
        for line in f:
            if 'assets' in line and 'spiffs' in line:
                parts = [p.strip() for p in line.split(',')]
                if len(parts) >= 4:
                    try:
                        # Offset có thể ở dạng hex (0x800000) hoặc decimal
                        offset_str = parts[3].strip()
                        if offset_str.startswith('0x'):
                            assets_offset = int(offset_str, 16)
                        else:
                            assets_offset = int(offset_str)
                        break
                    except ValueError:
                        continue
    
    if assets_offset is None:
        print("❌ Không tìm thấy offset partition assets")
        return False
    
    print(f"📦 Assets binary: {assets_bin}")
    print(f"📏 Size: {assets_bin.stat().st_size / 1024:.1f} KB")
    print(f"📍 Flash offset: 0x{assets_offset:X}")
    
    # Xây dựng lệnh esptool.py
    cmd = [
        "python", "-m", "esptool",
        "--chip", "esp32s3"
    ]
    
    if port:
        cmd.extend(["--port", port])
    
    cmd.extend([
        "--baud", str(baud),
        "write_flash",
        f"0x{assets_offset:X}",
        str(assets_bin)
    ])
    
    print("🔧 Lệnh flash:")
    print(" ".join(cmd))
    print()
    
    # Thực thi lệnh flash
    try:
        result = subprocess.run(cmd, check=True)
        print("✅ Flash audio assets thành công!")
        return True
    except subprocess.CalledProcessError as e:
        print(f"❌ Flash thất bại: {e}")
        return False
    except FileNotFoundError:
        print("❌ Không tìm thấy esptool.py")
        print("Cài đặt: pip install esptool")
        return False

def main():
    print("🎵 Flash Audio Assets vào ESP32")
    print("=" * 40)
    
    port = None
    if len(sys.argv) > 1:
        port = sys.argv[1]
    
    if flash_audio_assets(port):
        print("\n🎉 Hoàn thành!")
        print("⚡ Reset ESP32 để load audio assets mới")
    else:
        print("\n💥 Thất bại!")
        sys.exit(1)

if __name__ == "__main__":
    main()