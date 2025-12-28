#!/usr/bin/env python3
"""
Tạo Audio Opus từ Text bằng Edge TTS (Microsoft TTS)

Script này sử dụng Edge TTS để tạo file audio tiếng Việt từ văn bản,
sau đó convert sang định dạng Opus/OGG phù hợp với ESP32.

Yêu cầu:
    pip install edge-tts pydub

FFmpeg cũng cần được cài đặt và có trong PATH.

Sử dụng:
    python create_audio_from_text.py
    python create_audio_from_text.py --config custom_config.json
    python create_audio_from_text.py --voice vi-VN-NamMinhNeural
"""

import asyncio
import json
import os
import sys
import argparse
import subprocess
import tempfile
from pathlib import Path

# Kiểm tra và cài đặt thư viện cần thiết
try:
    import edge_tts
except ImportError:
    print("Đang cài đặt edge-tts...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "edge-tts"])
    import edge_tts


# Danh sách voice tiếng Việt có sẵn
VIETNAMESE_VOICES = {
    "nam_minh": "vi-VN-NamMinhNeural",      # Giọng nam
    "hoai_my": "vi-VN-HoaiMyNeural",         # Giọng nữ
}

# Voice mặc định
DEFAULT_VOICE = "vi-VN-HoaiMyNeural"  # Giọng nữ dễ nghe

# Thông số audio cho ESP32
AUDIO_CONFIG = {
    "sample_rate": 16000,
    "channels": 1,
    "bitrate": "24k",
    "codec": "libopus"
}


def check_ffmpeg():
    """Kiểm tra FFmpeg đã được cài đặt chưa"""
    try:
        result = subprocess.run(
            ["ffmpeg", "-version"],
            capture_output=True,
            text=True
        )
        return result.returncode == 0
    except FileNotFoundError:
        return False


async def text_to_mp3(text: str, output_mp3: str, voice: str = DEFAULT_VOICE):
    """
    Chuyển văn bản thành file MP3 bằng Edge TTS
    
    Args:
        text: Văn bản cần chuyển đổi
        output_mp3: Đường dẫn file MP3 output
        voice: Giọng đọc (mặc định là giọng nữ tiếng Việt)
    """
    communicate = edge_tts.Communicate(text, voice)
    await communicate.save(output_mp3)


def mp3_to_opus(input_mp3: str, output_opus: str):
    """
    Chuyển file MP3 sang Opus/OGG với thông số phù hợp ESP32
    
    Args:
        input_mp3: Đường dẫn file MP3 input
        output_opus: Đường dẫn file Opus output
    """
    cmd = [
        "ffmpeg", "-y",
        "-i", input_mp3,
        "-ar", str(AUDIO_CONFIG["sample_rate"]),
        "-ac", str(AUDIO_CONFIG["channels"]),
        "-c:a", AUDIO_CONFIG["codec"],
        "-b:a", AUDIO_CONFIG["bitrate"],
        "-vbr", "on",
        "-compression_level", "10",
        output_opus
    ]
    
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"FFmpeg error: {result.stderr}")
        raise RuntimeError(f"Failed to convert {input_mp3} to {output_opus}")


async def create_audio(text: str, output_opus: str, voice: str = DEFAULT_VOICE):
    """
    Tạo file audio Opus từ văn bản
    
    Args:
        text: Văn bản cần chuyển đổi
        output_opus: Đường dẫn file Opus output
        voice: Giọng đọc
    """
    # Tạo thư mục nếu chưa có
    os.makedirs(os.path.dirname(output_opus) or ".", exist_ok=True)
    
    # Tạo file MP3 tạm thời
    with tempfile.NamedTemporaryFile(suffix=".mp3", delete=False) as tmp:
        tmp_mp3 = tmp.name
    
    try:
        # Bước 1: Text -> MP3
        await text_to_mp3(text, tmp_mp3, voice)
        
        # Bước 2: MP3 -> Opus
        mp3_to_opus(tmp_mp3, output_opus)
        
        # Lấy kích thước file
        size = os.path.getsize(output_opus)
        print(f"  ✓ {output_opus} ({size / 1024:.1f} KB)")
        
    finally:
        # Xóa file tạm
        if os.path.exists(tmp_mp3):
            os.remove(tmp_mp3)


async def main():
    parser = argparse.ArgumentParser(
        description="Tạo Audio Opus từ Text bằng Edge TTS"
    )
    parser.add_argument(
        "--config", 
        type=str, 
        default="audio_text_config.json",
        help="File JSON chứa danh sách văn bản (mặc định: audio_text_config.json)"
    )
    parser.add_argument(
        "--voice",
        type=str,
        default=DEFAULT_VOICE,
        choices=list(VIETNAMESE_VOICES.values()) + list(VIETNAMESE_VOICES.keys()),
        help=f"Giọng đọc (mặc định: {DEFAULT_VOICE})"
    )
    parser.add_argument(
        "--output-dir",
        type=str,
        default="audio_files",
        help="Thư mục output (mặc định: audio_files)"
    )
    parser.add_argument(
        "--list-voices",
        action="store_true",
        help="Liệt kê tất cả giọng đọc tiếng Việt có sẵn"
    )
    
    args = parser.parse_args()
    
    # Liệt kê voices
    if args.list_voices:
        print("\n🎤 Các giọng đọc tiếng Việt có sẵn:")
        for name, voice_id in VIETNAMESE_VOICES.items():
            print(f"  - {name}: {voice_id}")
        print(f"\nVí dụ: python {sys.argv[0]} --voice {DEFAULT_VOICE}")
        return
    
    # Kiểm tra FFmpeg
    if not check_ffmpeg():
        print("❌ Lỗi: FFmpeg chưa được cài đặt hoặc không có trong PATH")
        print("   Windows: choco install ffmpeg")
        print("   hoặc download từ https://ffmpeg.org/download.html")
        sys.exit(1)
    
    # Xử lý voice argument
    voice = args.voice
    if voice in VIETNAMESE_VOICES:
        voice = VIETNAMESE_VOICES[voice]
    
    # Đọc config
    config_path = Path(__file__).parent / args.config
    if not config_path.exists():
        print(f"❌ Không tìm thấy file config: {config_path}")
        sys.exit(1)
    
    with open(config_path, 'r', encoding='utf-8') as f:
        audio_config = json.load(f)
    
    print(f"\n🔊 Tạo Audio từ Text")
    print(f"   Voice: {voice}")
    print(f"   Config: {config_path}")
    print(f"   Output: {args.output_dir}/")
    print(f"   Tổng số file: {len(audio_config)}")
    print("-" * 50)
    
    # Tạo từng file audio
    output_dir = Path(__file__).parent / args.output_dir
    
    success_count = 0
    error_count = 0
    
    for filename, text in audio_config.items():
        output_path = output_dir / filename
        try:
            await create_audio(text, str(output_path), voice)
            success_count += 1
        except Exception as e:
            print(f"  ✗ {filename}: {e}")
            error_count += 1
    
    print("-" * 50)
    print(f"✅ Hoàn thành: {success_count} file")
    if error_count > 0:
        print(f"❌ Lỗi: {error_count} file")
    
    # Tính tổng kích thước
    total_size = 0
    for filename in audio_config.keys():
        file_path = output_dir / filename
        if file_path.exists():
            total_size += file_path.stat().st_size
    
    print(f"📦 Tổng kích thước: {total_size / 1024:.1f} KB")


if __name__ == "__main__":
    asyncio.run(main())
