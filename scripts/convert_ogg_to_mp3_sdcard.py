#!/usr/bin/env python3
"""
Convert Ogg Opus files to MP3 and prepare for SD card
Usage: python convert_ogg_to_mp3_sdcard.py <input_dir> <output_dir>
"""

import os
import subprocess
import shutil
from pathlib import Path

def convert_ogg_to_mp3(input_file, output_file):
    """Convert OGG Opus to MP3 using FFmpeg"""
    try:
        # Convert to 24kHz mono MP3, 64kbps
        cmd = [
            'ffmpeg',
            '-i', input_file,
            '-acodec', 'libmp3lame',
            '-ar', '24000',
            '-ac', '1',
            '-b:a', '64k',
            '-y',  # Overwrite output file
            output_file
        ]
        
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        return result.returncode == 0
    except Exception as e:
        print(f"❌ Error converting {input_file}: {e}")
        return False

def main():
    import sys
    
    if len(sys.argv) < 2:
        input_dir = "E:\\xiaozhi-PhuongAnh-SdCard\\main\\boards\\xiaozhi-ai-iot-vietnam-lcd-sdcard\\audio_ogg"
        output_dir = "E:\\xiaozhi-PhuongAnh-SdCard\\sdcard_notifications"
    else:
        input_dir = sys.argv[1]
        output_dir = sys.argv[2] if len(sys.argv) > 2 else "sdcard_notifications"
    
    input_path = Path(input_dir)
    output_path = Path(output_dir)
    
    # Create output directory
    output_path.mkdir(parents=True, exist_ok=True)
    
    print(f"📁 Input:  {input_path}")
    print(f"📁 Output: {output_path}")
    print("=" * 60)
    
    # Find all .ogg files
    ogg_files = list(input_path.rglob("*.ogg"))
    total = len(ogg_files)
    
    if total == 0:
        print("❌ No .ogg files found!")
        return
    
    print(f"🎵 Found {total} OGG files to convert")
    print("=" * 60)
    
    success_count = 0
    error_count = 0
    total_size = 0
    
    for idx, ogg_file in enumerate(ogg_files, 1):
        # Get just the filename without path
        filename = ogg_file.stem + ".mp3"
        output_file = output_path / filename
        
        file_size = ogg_file.stat().st_size
        
        print(f"[{idx:2d}/{total}] {filename:40s} ({file_size:8d} bytes) ... ", end="", flush=True)
        
        if convert_ogg_to_mp3(str(ogg_file), str(output_file)):
            output_size = output_file.stat().st_size
            total_size += output_size
            print(f"✅ ({output_size:8d} bytes)")
            success_count += 1
        else:
            print("❌")
            error_count += 1
    
    print("=" * 60)
    print(f"\n📊 Results:")
    print(f"  ✅ Converted: {success_count}/{total} files")
    if error_count > 0:
        print(f"  ❌ Failed: {error_count} files")
    print(f"  💾 Total size: {total_size / 1024 / 1024:.2f} MB")
    print(f"  📂 Output: {output_path}")
    
    print("\n📝 Next steps:")
    print(f"  1. Copy all files from '{output_path}' to SD card")
    print(f"  2. Create folder: /sdcard/notifications/")
    print(f"  3. Paste MP3 files there")
    print(f"  4. Insert SD card and restart device")

if __name__ == "__main__":
    main()
