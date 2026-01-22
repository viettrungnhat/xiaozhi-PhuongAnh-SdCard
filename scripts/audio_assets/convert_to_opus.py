#!/usr/bin/env python3
"""
Convert audio files (WAV/MP3/FLAC) sang Opus/OGG cho ESP32

Sử dụng:
    python convert_to_opus.py input.wav output.opus
    python convert_to_opus.py --input-dir ./wav_files --output-dir ./audio_files
    python convert_to_opus.py input.mp3  # Output: input.opus
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path


# Thông số audio cho ESP32
AUDIO_CONFIG = {
    "sample_rate": 16000,
    "channels": 1,
    "bitrate": "24k",
    "codec": "libopus"
}

# Các định dạng input được hỗ trợ
SUPPORTED_FORMATS = ['.wav', '.mp3', '.flac', '.m4a', '.aac', '.ogg', '.wma']


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


def convert_to_opus(input_file: str, output_file: str, normalize: bool = True):
    """
    Chuyển file audio sang Opus/OGG
    
    Args:
        input_file: Đường dẫn file input
        output_file: Đường dẫn file output
        normalize: Có normalize âm lượng không
    """
    # Tạo thư mục output nếu chưa có
    os.makedirs(os.path.dirname(output_file) or ".", exist_ok=True)
    
    # Build FFmpeg command
    cmd = [
        "ffmpeg", "-y",
        "-i", input_file,
    ]
    
    # Thêm audio filter nếu cần normalize
    if normalize:
        cmd.extend([
            "-af", "loudnorm=I=-16:TP=-1.5:LRA=11"
        ])
    
    cmd.extend([
        "-ar", str(AUDIO_CONFIG["sample_rate"]),
        "-ac", str(AUDIO_CONFIG["channels"]),
        "-c:a", AUDIO_CONFIG["codec"],
        "-b:a", AUDIO_CONFIG["bitrate"],
        "-vbr", "on",
        "-compression_level", "10",
        output_file
    ])
    
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(f"FFmpeg error: {result.stderr}")
    
    return os.path.getsize(output_file)


def convert_directory(input_dir: str, output_dir: str, normalize: bool = True):
    """
    Chuyển đổi tất cả file audio trong thư mục
    
    Args:
        input_dir: Thư mục input
        output_dir: Thư mục output
        normalize: Có normalize âm lượng không
    """
    input_path = Path(input_dir)
    output_path = Path(output_dir)
    
    success_count = 0
    error_count = 0
    total_size = 0
    
    for input_file in input_path.rglob("*"):
        if input_file.suffix.lower() not in SUPPORTED_FORMATS:
            continue
        
        # Tính đường dẫn output, giữ nguyên cấu trúc thư mục
        relative_path = input_file.relative_to(input_path)
        output_file = output_path / relative_path.with_suffix('.opus')
        
        try:
            size = convert_to_opus(str(input_file), str(output_file), normalize)
            print(f"  ✓ {relative_path} -> {output_file.name} ({size / 1024:.1f} KB)")
            success_count += 1
            total_size += size
        except Exception as e:
            print(f"  ✗ {relative_path}: {e}")
            error_count += 1
    
    return success_count, error_count, total_size


def main():
    parser = argparse.ArgumentParser(
        description="Convert audio files sang Opus/OGG cho ESP32"
    )
    
    # Positional arguments cho single file conversion
    parser.add_argument(
        "input",
        nargs="?",
        help="File input (WAV/MP3/FLAC)"
    )
    parser.add_argument(
        "output",
        nargs="?",
        help="File output (.opus) - tùy chọn, mặc định dùng tên file input"
    )
    
    # Optional arguments cho batch conversion
    parser.add_argument(
        "--input-dir",
        type=str,
        help="Thư mục chứa các file audio cần convert"
    )
    parser.add_argument(
        "--output-dir",
        type=str,
        default="audio_files",
        help="Thư mục output (mặc định: audio_files)"
    )
    parser.add_argument(
        "--no-normalize",
        action="store_true",
        help="Không normalize âm lượng"
    )
    
    args = parser.parse_args()
    
    # Kiểm tra FFmpeg
    if not check_ffmpeg():
        print("❌ Lỗi: FFmpeg chưa được cài đặt hoặc không có trong PATH")
        print("   Windows: choco install ffmpeg")
        print("   hoặc download từ https://ffmpeg.org/download.html")
        sys.exit(1)
    
    normalize = not args.no_normalize
    
    # Batch conversion
    if args.input_dir:
        print(f"\n🔄 Convert thư mục: {args.input_dir}")
        print(f"   Output: {args.output_dir}/")
        print(f"   Normalize: {'Có' if normalize else 'Không'}")
        print("-" * 50)
        
        success, error, total_size = convert_directory(
            args.input_dir, args.output_dir, normalize
        )
        
        print("-" * 50)
        print(f"✅ Hoàn thành: {success} file")
        if error > 0:
            print(f"❌ Lỗi: {error} file")
        print(f"📦 Tổng kích thước: {total_size / 1024:.1f} KB")
        
    # Single file conversion
    elif args.input:
        input_file = args.input
        
        if not os.path.exists(input_file):
            print(f"❌ Không tìm thấy file: {input_file}")
            sys.exit(1)
        
        # Tự động tạo tên output nếu không được chỉ định
        if args.output:
            output_file = args.output
        else:
            output_file = Path(input_file).with_suffix('.opus')
        
        print(f"\n🔄 Convert: {input_file}")
        print(f"   Output: {output_file}")
        print(f"   Normalize: {'Có' if normalize else 'Không'}")
        
        try:
            size = convert_to_opus(input_file, str(output_file), normalize)
            print(f"✅ Hoàn thành: {output_file} ({size / 1024:.1f} KB)")
        except Exception as e:
            print(f"❌ Lỗi: {e}")
            sys.exit(1)
    
    else:
        parser.print_help()
        print("\n📝 Ví dụ:")
        print("   python convert_to_opus.py music.wav")
        print("   python convert_to_opus.py music.mp3 output.opus")
        print("   python convert_to_opus.py --input-dir ./recordings --output-dir ./audio_files")


if __name__ == "__main__":
    main()
