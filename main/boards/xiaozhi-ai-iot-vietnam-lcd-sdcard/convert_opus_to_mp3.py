#!/usr/bin/env python3
"""
Convert all Opus audio files to MP3 format (24kHz mono) for ESP32 playback

Usage:
    python convert_opus_to_mp3.py
"""

import os
import subprocess
from pathlib import Path

# Directories
OPUS_DIR = Path(__file__).parent / "audio_opus"
MP3_DIR = Path(__file__).parent / "audio_mp3"

# FFmpeg path
FFMPEG_PATH = r"E:\AppSmartLearning\ffmpeg_bin\bin\ffmpeg.exe"

# Audio settings for ES8311 codec
SAMPLE_RATE = 24000
BITRATE = "64k"
CHANNELS = 1

def convert_opus_to_mp3(opus_file, mp3_file):
    """Convert single opus file to MP3 using ffmpeg"""
    try:
        # Create parent directory if needed
        mp3_file.parent.mkdir(parents=True, exist_ok=True)
        
        # ffmpeg command: decode opus -> encode MP3 at 24kHz mono
        cmd = [
            FFMPEG_PATH,
            "-i", str(opus_file),
            "-ar", str(SAMPLE_RATE),  # Sample rate
            "-ac", str(CHANNELS),     # Mono
            "-b:a", BITRATE,          # Bitrate
            "-y",                     # Overwrite
            str(mp3_file)
        ]
        
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode == 0:
            opus_size = opus_file.stat().st_size
            mp3_size = mp3_file.stat().st_size
            print(f"✅ {opus_file.name:40s} -> {mp3_file.name:40s} ({opus_size:6d} -> {mp3_size:6d} bytes)")
            return True
        else:
            print(f"❌ Failed: {opus_file.name}")
            print(f"   Error: {result.stderr}")
            return False
            
    except Exception as e:
        print(f"❌ Error converting {opus_file.name}: {e}")
        return False

def main():
    if not OPUS_DIR.exists():
        print(f"❌ Opus directory not found: {OPUS_DIR}")
        return
    
    # Create output directory
    MP3_DIR.mkdir(exist_ok=True)
    
    # Find all opus files
    opus_files = list(OPUS_DIR.rglob("*.opus"))
    
    if not opus_files:
        print(f"❌ No .opus files found in {OPUS_DIR}")
        return
    
    print(f"Found {len(opus_files)} opus files")
    print(f"Converting to MP3 (24kHz mono, {BITRATE} bitrate)...")
    print("=" * 100)
    
    success_count = 0
    
    # Convert each file, maintaining directory structure
    for opus_file in sorted(opus_files):
        # Get relative path from opus directory
        rel_path = opus_file.relative_to(OPUS_DIR)
        
        # Create MP3 path with same structure
        mp3_file = MP3_DIR / rel_path.with_suffix(".mp3")
        
        if convert_opus_to_mp3(opus_file, mp3_file):
            success_count += 1
    
    print("=" * 100)
    print(f"✅ Converted {success_count}/{len(opus_files)} files successfully")
    print(f"📁 Output directory: {MP3_DIR}")
    print(f"\n💡 Next steps:")
    print(f"   1. Rebuild assets: python scripts/spiffs_assets/build.py --audio_mp3 {MP3_DIR.relative_to(Path(__file__).parent.parent.parent.parent)} --wakenet_model ...")
    print(f"   2. Flash firmware: idf.py build flash")
    print(f"   3. Flash assets: python %IDF_PATH%\\components\\esptool_py\\esptool\\esptool.py -p COM12 write_flash 0x7D0000 scripts/spiffs_assets/build/assets.bin")

if __name__ == "__main__":
    main()
