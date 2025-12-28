#!/usr/bin/env python3
"""
Script đóng gói audio opus vào assets binary cho ESP32
Tạo file binary để flash vào partition assets
"""

import os
import sys
import struct
from pathlib import Path

def create_assets_binary(audio_dir, output_file):
    """Tạo file binary từ thư mục audio_opus"""
    
    audio_path = Path(audio_dir)
    if not audio_path.exists():
        print(f"❌ Thư mục không tồn tại: {audio_dir}")
        return False
    
    # Scan tất cả file .opus
    opus_files = []
    for opus_file in audio_path.glob("**/*.opus"):
        rel_path = opus_file.relative_to(audio_path)
        opus_files.append((str(rel_path), opus_file))
    
    if not opus_files:
        print(f"❌ Không tìm thấy file .opus trong {audio_dir}")
        return False
    
    print(f"📁 Tìm thấy {len(opus_files)} file opus")
    
    # Chuẩn bị data
    file_count = len(opus_files)
    file_table = []
    file_data = bytearray()
    
    for rel_path, file_path in opus_files:
        file_size = file_path.stat().st_size
        file_offset = len(file_data)
        
        # Đọc file content
        with open(file_path, 'rb') as f:
            content = f.read()
            file_data.extend(content)
        
        # Asset table entry (tương thích với mmap_assets_table)
        # char asset_name[32], uint32_t size, uint32_t offset, uint16_t width, uint16_t height
        asset_name = rel_path.encode('utf-8')[:31]  # Max 31 chars + null terminator
        asset_name = asset_name.ljust(32, b'\x00')  # Pad to 32 bytes
        
        entry = struct.pack('<32sIIHH', 
                           asset_name,    # 32 bytes name
                           file_size,     # 4 bytes size
                           file_offset,   # 4 bytes offset
                           0,            # 2 bytes width (không dùng cho audio)
                           0)            # 2 bytes height (không dùng cho audio)
        file_table.append(entry)
        
        print(f"  📄 {rel_path} ({file_size} bytes)")
    
    # Tính checksum cho data
    checksum = sum(file_data) & 0xFFFFFFFF
    data_length = len(file_data)
    
    # Tạo header: file_count(4) + checksum(4) + data_length(4) + table + data
    table_data = b''.join(file_table)
    total_data = table_data + file_data
    
    header = struct.pack('<III', file_count, checksum, len(total_data))
    
    # Ghi file binary
    with open(output_file, 'wb') as f:
        f.write(header)
        f.write(total_data)
    
    output_size = os.path.getsize(output_file)
    print(f"✅ Tạo thành công: {output_file}")
    print(f"📊 Tổng dung lượng: {output_size / 1024:.1f} KB")
    print(f"🗂️ Số file: {file_count}")
    print(f"🔐 Checksum: 0x{checksum:08X}")
    
    return True

if __name__ == "__main__":
    script_dir = Path(__file__).parent
    audio_dir = script_dir / "audio_opus"
    output_file = script_dir / "assets" / "audio_assets.bin"
    
    # Tạo thư mục assets nếu chưa có
    output_file.parent.mkdir(exist_ok=True)
    
    if len(sys.argv) > 1:
        audio_dir = Path(sys.argv[1])
    
    if len(sys.argv) > 2:
        output_file = Path(sys.argv[2])
    
    print("🎵 Đóng gói Audio Opus vào Assets Binary")
    print("=" * 50)
    print(f"📁 Input:  {audio_dir}")
    print(f"📦 Output: {output_file}")
    print("=" * 50)
    
    if create_assets_binary(audio_dir, output_file):
        print("🎉 Hoàn thành!")
    else:
        sys.exit(1)