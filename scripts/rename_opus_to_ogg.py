#!/usr/bin/env python3
"""
Rename all .opus files to .ogg (Ogg Opus format)
"""
import os
import shutil
from pathlib import Path

def rename_opus_to_ogg(source_dir, output_dir):
    """Rename all .opus files to .ogg, preserving directory structure"""
    
    if not os.path.exists(source_dir):
        print(f"Error: Source directory not found: {source_dir}")
        return
    
    # Create output directory
    os.makedirs(output_dir, exist_ok=True)
    
    renamed_count = 0
    
    # Walk through all subdirectories
    for root, dirs, files in os.walk(source_dir):
        for file in files:
            if file.lower().endswith('.opus'):
                src_file = os.path.join(root, file)
                
                # Create relative path and output directory
                rel_path = os.path.relpath(root, source_dir)
                if rel_path == '.':
                    out_dir = output_dir
                else:
                    out_dir = os.path.join(output_dir, rel_path)
                
                os.makedirs(out_dir, exist_ok=True)
                
                # Rename .opus to .ogg
                new_filename = os.path.splitext(file)[0] + '.ogg'
                dst_file = os.path.join(out_dir, new_filename)
                
                # Copy file with new name (preserve binary content)
                shutil.copy2(src_file, dst_file)
                print(f"✅ Renamed: {file} -> {new_filename}")
                renamed_count += 1
    
    print(f"\n✅ Successfully renamed {renamed_count} files from .opus to .ogg")
    return renamed_count

if __name__ == "__main__":
    # Define paths - absolute
    source_dir = r"E:\xiaozhi-PhuongAnh-SdCard\main\boards\xiaozhi-ai-iot-vietnam-lcd-sdcard\audio_opus"
    output_dir = r"E:\xiaozhi-PhuongAnh-SdCard\main\boards\xiaozhi-ai-iot-vietnam-lcd-sdcard\audio_ogg"
    
    print("Converting Opus files to OGG format...")
    print(f"Source: {source_dir}")
    print(f"Output: {output_dir}")
    print()
    
    rename_opus_to_ogg(source_dir, output_dir)
