#!/usr/bin/env python3
"""
Build Audio Assets cho ESP32

Script này đóng gói các file audio .opus vào assets.bin và có thể
merge với assets.bin hiện có (chứa fonts, models, emojis).

Sử dụng:
    python build_audio_assets.py
    python build_audio_assets.py --audio-dir ./audio_files --output ./build
    python build_audio_assets.py --merge-with ../../build/assets.bin
"""

import argparse
import json
import os
import shutil
import struct
import sys
from datetime import datetime
from pathlib import Path


# Cấu hình mặc định
DEFAULT_AUDIO_DIR = "../../main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/audio_opus"
DEFAULT_OUTPUT_DIR = "build"
MAX_NAME_LENGTH = 48  # Độ dài tối đa của tên file trong assets


def compute_checksum(data):
    """Tính checksum của data"""
    checksum = sum(data) & 0xFFFF
    return checksum


def sort_key(filename):
    """Sắp xếp theo extension rồi tên file"""
    basename, extension = os.path.splitext(filename)
    return extension, basename


def read_existing_assets(assets_bin_path):
    """
    Đọc assets.bin hiện có và trả về danh sách file info
    
    Returns:
        list of tuples: (filename, data)
    """
    files = []
    
    if not os.path.exists(assets_bin_path):
        return files
    
    with open(assets_bin_path, 'rb') as f:
        # Header: total_files (4), checksum (4), data_length (4)
        header = f.read(12)
        if len(header) < 12:
            return files
        
        total_files = struct.unpack('<I', header[0:4])[0]
        checksum = struct.unpack('<I', header[4:8])[0]
        data_length = struct.unpack('<I', header[8:12])[0]
        
        print(f"  Đọc assets.bin: {total_files} files, checksum=0x{checksum:04X}")
        
        # Đọc mmap_table
        # Mỗi entry: name (MAX_NAME_LENGTH), size (4), offset (4), width (2), height (2)
        entry_size = MAX_NAME_LENGTH + 4 + 4 + 2 + 2
        
        entries = []
        for i in range(total_files):
            entry_data = f.read(entry_size)
            if len(entry_data) < entry_size:
                break
            
            name = entry_data[:MAX_NAME_LENGTH].rstrip(b'\x00').decode('utf-8')
            size = struct.unpack('<I', entry_data[MAX_NAME_LENGTH:MAX_NAME_LENGTH+4])[0]
            offset = struct.unpack('<I', entry_data[MAX_NAME_LENGTH+4:MAX_NAME_LENGTH+8])[0]
            
            entries.append((name, size, offset))
        
        # Đọc merged_data (bắt đầu sau mmap_table)
        table_size = entry_size * total_files
        merged_start = 12 + table_size
        
        for name, size, offset in entries:
            # Offset là từ đầu merged_data, thêm 2 byte prefix 0x5A5A
            file_start = merged_start + offset + 2  # Skip 0x5A5A prefix
            f.seek(file_start)
            file_data = f.read(size)
            
            if len(file_data) == size:
                files.append((name, file_data))
                # print(f"    - {name}: {size} bytes")
    
    return files


def collect_audio_files(audio_dir):
    """
    Thu thập tất cả file .opus từ thư mục audio
    
    Returns:
        list of tuples: (relative_filename, data)
    """
    files = []
    audio_path = Path(audio_dir)
    
    if not audio_path.exists():
        print(f"❌ Không tìm thấy thư mục: {audio_dir}")
        return files
    
    for opus_file in sorted(audio_path.rglob("*.opus")):
        relative_name = str(opus_file.relative_to(audio_path))
        # Thay thế \ bằng / cho đồng nhất
        relative_name = relative_name.replace("\\", "/")
        
        with open(opus_file, 'rb') as f:
            data = f.read()
        
        files.append((relative_name, data))
        print(f"  + {relative_name}: {len(data)} bytes")
    
    return files


def build_assets_bin(file_list, output_path, max_name_len=MAX_NAME_LENGTH):
    """
    Build assets.bin từ danh sách file
    
    Args:
        file_list: list of (filename, data) tuples
        output_path: đường dẫn file output
        max_name_len: độ dài tối đa của tên file
    """
    # Sắp xếp file theo tên
    file_list = sorted(file_list, key=lambda x: sort_key(x[0]))
    
    # Build merged_data với prefix 0x5A5A cho mỗi file
    merged_data = bytearray()
    file_info_list = []
    
    for filename, data in file_list:
        offset = len(merged_data)
        
        # Thêm prefix 0x5A5A
        merged_data.extend(b'\x5A\x5A')
        merged_data.extend(data)
        
        # Lưu info: (name, offset, size, width, height)
        # width/height = 0 cho audio files
        file_info_list.append((filename, offset, len(data), 0, 0))
    
    # Build mmap_table
    mmap_table = bytearray()
    for filename, offset, size, width, height in file_info_list:
        # Truncate tên nếu quá dài
        if len(filename) > max_name_len:
            print(f"⚠️ Tên file quá dài, sẽ bị cắt: {filename}")
        
        # Pad tên file với null bytes
        name_bytes = filename.encode('utf-8')[:max_name_len]
        name_bytes = name_bytes.ljust(max_name_len, b'\x00')
        
        mmap_table.extend(name_bytes)
        mmap_table.extend(struct.pack('<I', size))
        mmap_table.extend(struct.pack('<I', offset))
        mmap_table.extend(struct.pack('<H', width))
        mmap_table.extend(struct.pack('<H', height))
    
    # Combine mmap_table + merged_data
    combined_data = mmap_table + merged_data
    
    # Tính checksum
    combined_checksum = compute_checksum(combined_data)
    combined_length = len(combined_data)
    total_files = len(file_info_list)
    
    # Build header: total_files (4), checksum (4), length (4)
    header = struct.pack('<I', total_files)
    header += struct.pack('<I', combined_checksum)
    header += struct.pack('<I', combined_length)
    
    # Write output file
    final_data = header + combined_data
    
    os.makedirs(os.path.dirname(output_path) or ".", exist_ok=True)
    with open(output_path, 'wb') as f:
        f.write(final_data)
    
    return total_files, combined_checksum, len(final_data)


def main():
    parser = argparse.ArgumentParser(
        description="Build Audio Assets cho ESP32"
    )
    parser.add_argument(
        "--audio-dir",
        type=str,
        default=DEFAULT_AUDIO_DIR,
        help=f"Thư mục chứa file audio (mặc định: {DEFAULT_AUDIO_DIR})"
    )
    parser.add_argument(
        "--output",
        type=str,
        default=DEFAULT_OUTPUT_DIR,
        help=f"Thư mục output (mặc định: {DEFAULT_OUTPUT_DIR})"
    )
    parser.add_argument(
        "--merge-with",
        type=str,
        help="Merge với assets.bin hiện có (path đến assets.bin)"
    )
    parser.add_argument(
        "--output-name",
        type=str,
        default="assets.bin",
        help="Tên file output (mặc định: assets.bin)"
    )
    
    args = parser.parse_args()
    
    print("\n🔊 Build Audio Assets")
    print("=" * 50)
    
    all_files = []
    
    # Merge với assets.bin hiện có nếu được chỉ định
    if args.merge_with:
        print(f"\n📂 Merge với: {args.merge_with}")
        existing_files = read_existing_assets(args.merge_with)
        
        # Lọc bỏ các file audio cũ (sẽ được thay thế)
        for name, data in existing_files:
            if not name.endswith('.opus'):
                all_files.append((name, data))
                print(f"  ✓ Giữ lại: {name}")
            else:
                print(f"  ✗ Thay thế: {name}")
    
    # Thu thập file audio mới
    print(f"\n📂 Thu thập audio từ: {args.audio_dir}")
    audio_files = collect_audio_files(args.audio_dir)
    
    if not audio_files:
        print("❌ Không tìm thấy file audio .opus nào!")
        print(f"   Vui lòng tạo file audio trong thư mục: {args.audio_dir}/")
        sys.exit(1)
    
    all_files.extend(audio_files)
    
    # Build assets.bin
    output_path = os.path.join(args.output, args.output_name)
    print(f"\n📦 Building: {output_path}")
    
    total_files, checksum, total_size = build_assets_bin(all_files, output_path)
    
    print("\n" + "=" * 50)
    print(f"✅ Build thành công!")
    print(f"   Tổng số file: {total_files}")
    print(f"   Checksum: 0x{checksum:04X}")
    print(f"   Kích thước: {total_size / 1024:.1f} KB")
    print(f"   Output: {output_path}")
    
    # Tạo file summary
    summary_path = os.path.join(args.output, "assets_summary.json")
    summary = {
        "total_files": total_files,
        "checksum": f"0x{checksum:04X}",
        "size_bytes": total_size,
        "files": [{"name": name, "size": len(data)} for name, data in all_files],
        "build_time": datetime.now().isoformat()
    }
    
    with open(summary_path, 'w', encoding='utf-8') as f:
        json.dump(summary, f, indent=2, ensure_ascii=False)
    
    print(f"   Summary: {summary_path}")


if __name__ == "__main__":
    main()
