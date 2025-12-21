#!/usr/bin/env python3
"""
Script kiểm tra audio assets binary
"""

import struct
from pathlib import Path

def check_audio_assets():
    script_dir = Path(__file__).parent
    assets_bin = script_dir / "assets" / "audio_assets.bin"
    
    if not assets_bin.exists():
        print(f"❌ File không tồn tại: {assets_bin}")
        return False
    
    size_kb = assets_bin.stat().st_size / 1024
    print(f"📦 Audio Assets Binary: {assets_bin}")
    print(f"📏 Size: {size_kb:.1f} KB")
    print("=" * 50)
    
    with open(assets_bin, 'rb') as f:
        # Read header
        file_count = struct.unpack('<I', f.read(4))[0]
        checksum = struct.unpack('<I', f.read(4))[0]
        data_length = struct.unpack('<I', f.read(4))[0]
        
        print(f"📂 File count: {file_count}")
        print(f"🔐 Checksum: 0x{checksum:08X}")
        print(f"📊 Data length: {data_length} bytes")
        print("=" * 50)
        
        # Read some file entries
        print("📄 First 10 files:")
        for i in range(min(10, file_count)):
            # Asset entry: name(32) + size(4) + offset(4) + width(2) + height(2)
            name_bytes = f.read(32)
            size, offset, width, height = struct.unpack('<IIHH', f.read(12))
            
            name = name_bytes.decode('utf-8').rstrip('\x00')
            print(f"  {i+1:2d}. {name:<35} ({size:>6} bytes)")
    
    print("=" * 50)
    print("✅ Audio assets binary OK!")
    return True

if __name__ == "__main__":
    check_audio_assets()