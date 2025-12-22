#!/usr/bin/env python3
"""
Kiểm tra audio assets đã flash trong ESP32
Check if audio assets are properly flashed in ESP32
"""

import subprocess
import sys
import os

def check_audio_assets():
    """Kiểm tra audio assets trong flash ESP32"""
    print("🔍 Đang kiểm tra audio assets trong ESP32...")
    
    try:
        # Đọc 1KB đầu của assets partition để kiểm tra header
        cmd = [
            "esptool.py",
            "--chip", "esp32s3",
            "--port", "COM3",  # Thay đổi port nếu cần
            "--baud", "921600",
            "read_flash",
            "0x800000",    # Assets partition address
            "1024",        # Đọc 1KB để kiểm tra header
            "temp_header.bin"
        ]
        
        print(f"⚡ Đang chạy: {' '.join(cmd)}")
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"❌ Lỗi khi đọc flash: {result.stderr}")
            return False
            
        # Kiểm tra file header
        if os.path.exists("temp_header.bin"):
            with open("temp_header.bin", "rb") as f:
                header = f.read(16)  # Đọc 16 bytes header
                
            if len(header) >= 8:
                magic = header[:4]
                file_count = int.from_bytes(header[4:8], 'little')
                
                if magic == b'AUD1':
                    print(f"✅ Audio assets đã được flash!")
                    print(f"📊 Số file audio: {file_count}")
                    print(f"🎵 Magic header: {magic.hex()}")
                    
                    # Cleanup
                    os.remove("temp_header.bin")
                    return True
                else:
                    print(f"❌ Magic header không đúng: {magic.hex()} (cần: 41554431)")
            else:
                print("❌ Header quá ngắn")
                
            # Cleanup
            os.remove("temp_header.bin")
        
        return False
        
    except Exception as e:
        print(f"❌ Lỗi: {e}")
        return False

def main():
    print("🎵 ESP32 Audio Assets Flash Checker")
    print("="*50)
    
    if check_audio_assets():
        print("\n✅ Audio assets đang hoạt động bình thường!")
        print("💡 Bạn có thể flash firmware mà không cần lo audio bị mất.")
    else:
        print("\n❌ Audio assets chưa được flash hoặc có lỗi!")
        print("💡 Chạy: python flash_audio_assets.py để flash audio assets.")

if __name__ == "__main__":
    main()