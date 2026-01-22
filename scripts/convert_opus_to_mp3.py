#!/usr/bin/env python3
"""
Convert Ogg Opus files to MP3 for ESP32 playback
Uses MP3 decoder which is already available in the project
"""

import os
import sys
import subprocess
from pathlib import Path

def convert_opus_to_mp3(opus_path, mp3_path):
    """Convert Ogg Opus to MP3 using ffmpeg"""
    print(f"Converting: {opus_path.name}")
    
    # Convert to MP3: 24kHz mono, 64kbps (matches ES8311 codec rate)
    ffmpeg_path = r"C:\Program Files\AppSmartLearning\ffmpeg_bin\bin\ffmpeg.exe"
    cmd = [
        ffmpeg_path, "-i", str(opus_path),
        "-ar", "24000",      # 24kHz sample rate (matches codec default)
        "-ac", "1",          # Mono
        "-b:a", "64k",       # 64kbps bitrate
        "-y",                # Overwrite
        str(mp3_path)
    ]
    
    try:
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        print(f"  ✓ {mp3_path.name}")
        return True
    except FileNotFoundError:
        print("\nERROR: ffmpeg not found!")
        print("Install ffmpeg:")
        print("  Windows: winget install ffmpeg")
        print("  Or download from: https://ffmpeg.org/download.html")
        return False
    except subprocess.CalledProcessError as e:
        print(f"  ✗ Failed: {e.stderr}")
        return False

def process_directory(input_dir, output_dir):
    """Process all .opus files in directory tree"""
    input_path = Path(input_dir)
    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)
    
    # Find all .opus files
    opus_files = list(input_path.rglob("*.opus"))
    if not opus_files:
        print(f"No .opus files found in {input_dir}")
        return
    
    print(f"Found {len(opus_files)} opus files")
    print("=" * 60)
    
    success_count = 0
    for opus_file in opus_files:
        # Preserve directory structure
        rel_path = opus_file.relative_to(input_path)
        mp3_file = output_path / rel_path.with_suffix('.mp3')
        mp3_file.parent.mkdir(parents=True, exist_ok=True)
        
        if convert_opus_to_mp3(opus_file, mp3_file):
            success_count += 1
    
    print("=" * 60)
    print(f"✅ Converted {success_count}/{len(opus_files)} files successfully")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python convert_opus_to_mp3.py <input_dir> <output_dir>")
        print("\nExample:")
        print("  python convert_opus_to_mp3.py")
        print("    main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/audio_opus")
        print("    main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/audio_mp3")
        sys.exit(1)
    
    process_directory(sys.argv[1], sys.argv[2])
