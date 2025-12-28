#!/usr/bin/env python3
"""
Script tạo file âm thanh Opus từ text tiếng Việt
Sử dụng Google TTS hoặc có thể thay bằng FPT.AI/Zalo TTS

Cài đặt:
    pip install gtts pydub

Yêu cầu: ffmpeg phải được cài đặt trong hệ thống
"""

import os
import subprocess
from pathlib import Path

# Thử import gtts, nếu không có thì thông báo
try:
    from gtts import gTTS
    HAS_GTTS = True
except ImportError:
    HAS_GTTS = False
    print("⚠️ Chưa cài gtts. Chạy: pip install gtts")

# Thư mục output
OUTPUT_DIR = Path(__file__).parent

# Danh sách tất cả các file âm thanh cần tạo
# Giọng điệu: Cô gái nhỏ nhắc bố, dễ thương và quan tâm
AUDIO_FILES = {
    # === GREETINGS === (Lời chào thân thương)
    "greetings/greeting_morning.opus": "Bố ơi, buổi sáng tốt lành ạ! Chúc bố có một ngày làm việc thật hiệu quả nha!",
    "greetings/greeting_afternoon.opus": "Bố ơi, buổi chiều vui vẻ nha! Bố nhớ uống nước đủ nhé!",
    "greetings/greeting_evening.opus": "Bố ơi, buổi tối an lành ạ! Lái xe cẩn thận nha bố!",
    "greetings/greeting_default.opus": "Bố ơi, hôm nay mình đi đâu thế ạ? Con đã sẵn sàng rồi!",
    "greetings/goodbye.opus": "Bố ơi, tạm biệt bố nha! Hẹn gặp lại bố!",
    
    # === WARNINGS - Safety === (Nhắc nhở an toàn)
    "warnings/warn_seatbelt.opus": "Bố ơi, bố nhớ thắt dây an toàn nha! Con lo lắm!",
    "warnings/warn_seatbelt_urgent.opus": "Bố ơi bố ơi! Xe đang chạy mà bố chưa thắt dây an toàn! Bố thắt đi ạ!",
    "warnings/warn_parking_brake.opus": "Bố ơi, bố nhớ hạ phanh tay trước khi đi nha!",
    "warnings/warn_parking_brake_urgent.opus": "Bố ơi! Xe đang chạy mà phanh tay chưa hạ! Bố hạ phanh tay đi ạ!",
    "warnings/warn_door_open.opus": "Bố ơi, cửa xe chưa đóng kín nè! Bố đóng lại giúp con nha!",
    "warnings/warn_lights_on.opus": "Bố ơi, đèn xe vẫn đang bật nè! Bố tắt đi kẻo hết ắc quy nha!",
    
    # === WARNINGS - Battery === (Cảnh báo ắc quy)
    "warnings/battery_low.opus": "Bố ơi, điện ắc quy đang yếu hơn bình thường rồi ạ. Bố để ý nha!",
    "warnings/battery_critical.opus": "Bố ơi, ắc quy yếu lắm rồi! Bố kiểm tra sớm nha, con lo quá!",
    
    # === WARNINGS - Temperature === (Cảnh báo nhiệt độ)
    "warnings/temp_high.opus": "Bố ơi, nhiệt độ máy đang hơi cao nè! Bố để ý giúp con nha!",
    "warnings/temp_critical.opus": "Bố ơi bố ơi! Nhiệt độ máy cao quá rồi! Bố dừng xe kiểm tra ngay nha!",
    "warnings/temp_normal.opus": "Bố ơi, nhiệt độ máy đã bình thường rồi ạ! Bố yên tâm nha!",
    
    # === WARNINGS - Fuel === (Cảnh báo xăng)
    "warnings/fuel_low.opus": "Bố ơi, xăng còn ít rồi nè! Bố đổ xăng sớm nha!",
    "warnings/fuel_critical.opus": "Bố ơi, xăng sắp hết rồi! Còn được vài cây số thôi! Bố đổ xăng đi ạ!",
    "warnings/fuel_reserve.opus": "Bố ơi, xe đang chạy xăng dự trữ rồi ạ!",
    
    # === HIGHWAY MODE === (Chế độ đường trường)
    "highway/highway_mode_on.opus": "Bố ơi, con đã bật chế độ đường trường rồi ạ! Con sẽ đọc tốc độ định kỳ cho bố nha!",
    "highway/highway_mode_off.opus": "Bố ơi, con đã tắt chế độ đường trường rồi ạ!",
    "highway/speed_60.opus": "Bố ơi, đang đi 60 cây số ạ!",
    "highway/speed_70.opus": "Bố ơi, đang đi 70 cây số ạ!",
    "highway/speed_80.opus": "Bố ơi, đang đi 80 cây số ạ!",
    "highway/speed_90.opus": "Bố ơi, đang đi 90 cây số ạ!",
    "highway/speed_100.opus": "Bố ơi, đang đi 100 cây số ạ!",
    "highway/speed_110.opus": "Bố ơi, đang đi 110 cây số rồi ạ!",
    "highway/speed_120.opus": "Bố ơi, đang đi 120 cây số rồi ạ! Nhanh quá!",
    "highway/speed_over_limit.opus": "Bố ơi, bố đang đi hơi nhanh rồi! Bố chậm lại chút nha, con lo lắm!",
    "highway/rest_reminder.opus": "Bố ơi, bố lái xe hơn 2 tiếng rồi đó! Bố nghỉ ngơi một chút nha! Con thương bố!",
    
    # === CONTROL === (Điều khiển)
    "control/trunk_opening.opus": "Bố ơi, con đang mở cốp cho bố nha!",
    "control/trunk_opened.opus": "Bố ơi, cốp đã mở rồi ạ!",
    "control/ac_on.opus": "Bố ơi, con đã bật điều hòa cho bố rồi ạ!",
    "control/ac_off.opus": "Bố ơi, con đã tắt điều hòa rồi ạ!",
    "control/ready_to_go.opus": "Bố ơi, con đã chuẩn bị sẵn sàng để bố về rồi ạ! Bố về nha!",
    
    # === INFO === (Thông tin)
    "info/info_speed_prefix.opus": "Bố ơi, tốc độ hiện tại là",
    "info/info_fuel_prefix.opus": "Bố ơi, xăng còn khoảng",
    "info/info_temp_prefix.opus": "Bố ơi, nhiệt độ nước làm mát là",
    "info/info_battery_prefix.opus": "Bố ơi, điện áp ắc quy là",
    "info/info_km.opus": "cây số ạ",
    "info/info_percent.opus": "phần trăm ạ",
    "info/info_degrees.opus": "độ C ạ",
    "info/info_volts.opus": "vôn ạ",
    
    # === MAINTENANCE === (Bảo dưỡng)
    "warnings/maint_oil_change.opus": "Bố ơi, xe đã đi được 5000 km rồi ạ! Đến lúc thay dầu rồi nha bố!",
    "warnings/maint_tire_check.opus": "Bố ơi, xe đã đi được 10000 km rồi ạ! Bố kiểm tra lốp xe nha!",
    "warnings/maint_general.opus": "Bố ơi, đến lúc bảo dưỡng xe rồi ạ! Bố nhớ nha!",
}

# Số từ 0-20 và các số chục
NUMBERS = {
    "numbers/num_0.opus": "không",
    "numbers/num_1.opus": "một",
    "numbers/num_2.opus": "hai",
    "numbers/num_3.opus": "ba",
    "numbers/num_4.opus": "bốn",
    "numbers/num_5.opus": "năm",
    "numbers/num_6.opus": "sáu",
    "numbers/num_7.opus": "bảy",
    "numbers/num_8.opus": "tám",
    "numbers/num_9.opus": "chín",
    "numbers/num_10.opus": "mười",
    "numbers/num_11.opus": "mười một",
    "numbers/num_12.opus": "mười hai",
    "numbers/num_13.opus": "mười ba",
    "numbers/num_14.opus": "mười bốn",
    "numbers/num_15.opus": "mười lăm",
    "numbers/num_16.opus": "mười sáu",
    "numbers/num_17.opus": "mười bảy",
    "numbers/num_18.opus": "mười tám",
    "numbers/num_19.opus": "mười chín",
    "numbers/num_20.opus": "hai mươi",
    "numbers/num_30.opus": "ba mươi",
    "numbers/num_40.opus": "bốn mươi",
    "numbers/num_50.opus": "năm mươi",
    "numbers/num_60.opus": "sáu mươi",
    "numbers/num_70.opus": "bảy mươi",
    "numbers/num_80.opus": "tám mươi",
    "numbers/num_90.opus": "chín mươi",
    "numbers/num_100.opus": "một trăm",
    "numbers/num_thousand.opus": "nghìn",
    "numbers/num_point.opus": "phẩy",
}

def check_ffmpeg():
    """Kiểm tra ffmpeg đã được cài đặt chưa"""
    try:
        subprocess.run(["ffmpeg", "-version"], capture_output=True, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("❌ ffmpeg chưa được cài đặt!")
        print("   Windows: choco install ffmpeg hoặc tải từ https://ffmpeg.org/")
        print("   Linux: sudo apt install ffmpeg")
        print("   macOS: brew install ffmpeg")
        return False

def text_to_opus(text: str, output_path: Path, lang: str = "vi"):
    """Chuyển text thành file Opus"""
    if not HAS_GTTS:
        print(f"⏭️ Bỏ qua {output_path.name} (chưa cài gtts)")
        return False
    
    # Tạo thư mục nếu chưa có
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    # Tạo file MP3 tạm
    temp_mp3 = output_path.with_suffix(".mp3")
    
    try:
        # Tạo TTS
        tts = gTTS(text=text, lang=lang, slow=False)
        tts.save(str(temp_mp3))
        
        # Convert sang Opus với ffmpeg
        cmd = [
            "ffmpeg", "-y",
            "-i", str(temp_mp3),
            "-c:a", "libopus",
            "-b:a", "24k",
            "-ar", "16000",
            "-ac", "1",
            str(output_path)
        ]
        
        result = subprocess.run(cmd, capture_output=True)
        
        # Xóa file tạm
        if temp_mp3.exists():
            temp_mp3.unlink()
        
        if result.returncode == 0:
            size_kb = output_path.stat().st_size / 1024
            print(f"✅ {output_path.name} ({size_kb:.1f} KB)")
            return True
        else:
            print(f"❌ {output_path.name} - ffmpeg error")
            return False
            
    except Exception as e:
        print(f"❌ {output_path.name} - {e}")
        if temp_mp3.exists():
            temp_mp3.unlink()
        return False

def generate_all_audio():
    """Tạo tất cả các file âm thanh"""
    print("=" * 60)
    print("🎵 Tạo file âm thanh Opus cho Trợ lý xe Kia Morning")
    print("=" * 60)
    
    if not check_ffmpeg():
        return
    
    if not HAS_GTTS:
        print("\n⚠️ Cài gtts để tạo âm thanh: pip install gtts\n")
        return
    
    # Gộp tất cả file cần tạo
    all_files = {**AUDIO_FILES, **NUMBERS}
    
    total = len(all_files)
    success = 0
    
    print(f"\n📁 Thư mục output: {OUTPUT_DIR}")
    print(f"📊 Tổng số file cần tạo: {total}\n")
    
    for filename, text in all_files.items():
        output_path = OUTPUT_DIR / filename
        
        # Bỏ qua nếu file đã tồn tại
        if output_path.exists():
            print(f"⏭️ {filename} (đã tồn tại)")
            success += 1
            continue
        
        if text_to_opus(text, output_path):
            success += 1
    
    print("\n" + "=" * 60)
    print(f"✅ Hoàn thành: {success}/{total} file")
    print("=" * 60)

def generate_single(filename: str, text: str):
    """Tạo một file âm thanh đơn lẻ"""
    output_path = OUTPUT_DIR / filename
    text_to_opus(text, output_path)

if __name__ == "__main__":
    import sys
    
    if len(sys.argv) > 2:
        # Tạo file đơn lẻ: python generate_audio.py filename.opus "text"
        generate_single(sys.argv[1], sys.argv[2])
    else:
        # Tạo tất cả
        generate_all_audio()
