#!/usr/bin/env python3
"""
Xác minh Assets.bin

Script này đọc và kiểm tra nội dung của file assets.bin

Sử dụng:
    python verify_assets.py build/assets.bin
    python verify_assets.py ../../build/assets.bin
"""

import argparse
import os
import struct
import sys
from pathlib import Path


def verify_assets(assets_path: str, verbose: bool = False):
    """
    Đọc và hiển thị nội dung của assets.bin
    """
    if not os.path.exists(assets_path):
        print(f"❌ Không tìm thấy file: {assets_path}")
        return False
    
    file_size = os.path.getsize(assets_path)
    print(f"\n📦 File: {assets_path}")
    print(f"   Kích thước: {file_size / 1024:.1f} KB ({file_size:,} bytes)")
    print("=" * 60)
    
    with open(assets_path, 'rb') as f:
        # Đọc header: total_files (4), checksum (4), data_length (4)
        header = f.read(12)
        if len(header) < 12:
            print("❌ File quá nhỏ, không hợp lệ")
            return False
        
        total_files = struct.unpack('<I', header[0:4])[0]
        checksum = struct.unpack('<I', header[4:8])[0]
        data_length = struct.unpack('<I', header[8:12])[0]
        
        print(f"\n📋 Header:")
        print(f"   Total files: {total_files}")
        print(f"   Checksum: 0x{checksum:04X}")
        print(f"   Data length: {data_length:,} bytes")
        
        # Đọc file entries
        # Entry format: name (48 bytes), size (4), offset (4), width (2), height (2)
        MAX_NAME_LEN = 48
        ENTRY_SIZE = MAX_NAME_LEN + 4 + 4 + 2 + 2
        
        entries = []
        audio_files = []
        font_files = []
        model_files = []
        image_files = []
        other_files = []
        
        print(f"\n📂 Files ({total_files}):")
        print("-" * 60)
        
        for i in range(total_files):
            entry_data = f.read(ENTRY_SIZE)
            if len(entry_data) < ENTRY_SIZE:
                print(f"⚠️ Không đủ dữ liệu cho entry {i}")
                break
            
            name = entry_data[:MAX_NAME_LEN].rstrip(b'\x00').decode('utf-8', errors='replace')
            size = struct.unpack('<I', entry_data[MAX_NAME_LEN:MAX_NAME_LEN+4])[0]
            offset = struct.unpack('<I', entry_data[MAX_NAME_LEN+4:MAX_NAME_LEN+8])[0]
            width = struct.unpack('<H', entry_data[MAX_NAME_LEN+8:MAX_NAME_LEN+10])[0]
            height = struct.unpack('<H', entry_data[MAX_NAME_LEN+10:MAX_NAME_LEN+12])[0]
            
            entries.append({
                'name': name,
                'size': size,
                'offset': offset,
                'width': width,
                'height': height
            })
            
            # Phân loại file
            ext = name.lower().split('.')[-1] if '.' in name else ''
            
            if ext in ['opus', 'ogg', 'mp3', 'wav']:
                audio_files.append(name)
                icon = "🔊"
            elif ext in ['bin'] and 'font' in name.lower():
                font_files.append(name)
                icon = "🔤"
            elif ext in ['bin'] and 'model' in name.lower():
                model_files.append(name)
                icon = "🧠"
            elif ext in ['png', 'jpg', 'gif', 'spng', 'sjpg']:
                image_files.append(name)
                icon = "🖼️"
            else:
                other_files.append(name)
                icon = "📄"
            
            if verbose or len(entries) <= 30:
                if width > 0 and height > 0:
                    print(f"   {icon} {name:<40} {size:>8} bytes  ({width}x{height})")
                else:
                    print(f"   {icon} {name:<40} {size:>8} bytes")
        
        if len(entries) > 30 and not verbose:
            print(f"   ... và {len(entries) - 30} file khác (dùng --verbose để xem tất cả)")
        
        # Tổng kết
        print("\n" + "=" * 60)
        print("📊 Tổng kết:")
        print(f"   🔊 Audio files: {len(audio_files)}")
        print(f"   🔤 Font files: {len(font_files)}")
        print(f"   🧠 Model files: {len(model_files)}")
        print(f"   🖼️ Image files: {len(image_files)}")
        print(f"   📄 Other files: {len(other_files)}")
        
        # Liệt kê audio files
        if audio_files:
            print(f"\n🔊 Audio files chi tiết:")
            for name in audio_files:
                entry = next(e for e in entries if e['name'] == name)
                print(f"   ✓ {name} ({entry['size'] / 1024:.1f} KB)")
        else:
            print(f"\n⚠️ Không có file audio .opus nào trong assets!")
        
        # Tính checksum thực tế
        f.seek(12)  # Skip header
        remaining_data = f.read()
        actual_checksum = sum(remaining_data) & 0xFFFF
        
        if actual_checksum == checksum:
            print(f"\n✅ Checksum hợp lệ")
        else:
            print(f"\n⚠️ Checksum không khớp!")
            print(f"   Expected: 0x{checksum:04X}")
            print(f"   Actual: 0x{actual_checksum:04X}")
    
    return True


def main():
    parser = argparse.ArgumentParser(
        description="Xác minh Assets.bin"
    )
    parser.add_argument(
        "assets_bin",
        type=str,
        help="Đường dẫn đến file assets.bin"
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Hiển thị tất cả các file"
    )
    
    args = parser.parse_args()
    
    verify_assets(args.assets_bin, args.verbose)


if __name__ == "__main__":
    main()
