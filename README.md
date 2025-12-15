# xiaozhi-PhuongAnh-SdCard

Trợ lý ảo XiaoZhi với hỗ trợ SD Card cho Phương Anh / XiaoZhi Virtual Assistant with SD Card Support for Phuong Anh

## Tính năng / Features

- ✅ Quản lý file trên SD Card (đọc, ghi, xóa) / SD Card file management (read, write, delete)
- ✅ Giám sát bộ nhớ / Storage monitoring
- ✅ Lưu trữ ghi chú / Notes storage
- ✅ Giao diện dòng lệnh tương tác / Interactive command-line interface
- ✅ Hỗ trợ tiếng Việt / Vietnamese language support
- ✅ Quản lý file JSON / JSON file management

## Cài đặt / Installation

### Yêu cầu / Requirements

- Python 3.7 trở lên / Python 3.7 or higher
- pip

### Các bước cài đặt / Installation Steps

1. Clone repository:
```bash
git clone https://github.com/viettrungnhat/xiaozhi-PhuongAnh-SdCard.git
cd xiaozhi-PhuongAnh-SdCard
```

2. Cài đặt dependencies:
```bash
pip install -r requirements.txt
```

3. Cài đặt package (tùy chọn):
```bash
pip install -e .
```

## Sử dụng / Usage

### Chế độ tương tác / Interactive Mode

Chạy trợ lý ảo ở chế độ tương tác:

```bash
python -m src.xiaozhi.main
```

Hoặc nếu đã cài đặt package:

```bash
xiaozhi
```

### Chế độ Demo / Demo Mode

Xem demo các tính năng:

```bash
python -m src.xiaozhi.main --demo
```

Hoặc:

```bash
xiaozhi --demo
```

## Lệnh có sẵn / Available Commands

| Lệnh (Command) | Mô tả (Description) | Ví dụ (Example) |
|---------------|---------------------|-----------------|
| `help` hoặc `trợ giúp` | Hiển thị trợ giúp / Show help | `help` |
| `storage` hoặc `bộ nhớ` | Xem thông tin bộ nhớ / View storage info | `storage` |
| `list` hoặc `danh sách` | Liệt kê files / List files | `list` |
| `read <file>` hoặc `đọc <file>` | Đọc file / Read file | `read notes.txt` |
| `write <file> <content>` hoặc `viết <file> <content>` | Ghi file / Write file | `write test.txt Hello` |
| `delete <file>` hoặc `xóa <file>` | Xóa file / Delete file | `delete test.txt` |
| `exit` hoặc `thoát` | Thoát chương trình / Exit program | `exit` |

## Ví dụ sử dụng / Usage Examples

### Ghi và đọc file / Writing and Reading Files

```python
from xiaozhi.assistant import VirtualAssistant
from xiaozhi.config import Config

# Khởi tạo trợ lý / Initialize assistant
config = Config()
assistant = VirtualAssistant(config)

# Chào hỏi / Greet
print(assistant.greet())

# Ghi file / Write file
result = assistant.process_command("write myfile.txt Hello from XiaoZhi!")
print(result["message"])

# Đọc file / Read file
result = assistant.process_command("read myfile.txt")
print(result["message"])
```

### Quản lý SD Card / SD Card Management

```python
from xiaozhi.sd_card_manager import SDCardManager

# Khởi tạo manager / Initialize manager
sd_manager = SDCardManager()

# Lấy thông tin bộ nhớ / Get storage info
info = sd_manager.get_storage_info()
print(f"Dung lượng trống: {info['free_gb']} GB")

# Liệt kê files / List files
files = sd_manager.list_files()
for file in files:
    print(f"{file['name']} - {file['size_kb']} KB")

# Ghi file / Write file
sd_manager.write_file("test.txt", "Nội dung file")

# Đọc file / Read file
content = sd_manager.read_file("test.txt")
print(content)
```

### Lưu ghi chú / Saving Notes

```python
from xiaozhi.assistant import VirtualAssistant

assistant = VirtualAssistant()

# Lưu ghi chú / Save note
assistant.save_note("Shopping List", "Mua sữa, bánh mì, trứng")

# Lấy tất cả ghi chú / Get all notes
notes = assistant.get_notes()
for note in notes:
    print(f"{note['title']}: {note['content']}")
```

## Cấu trúc dự án / Project Structure

```
xiaozhi-PhuongAnh-SdCard/
├── src/
│   └── xiaozhi/
│       ├── __init__.py         # Package initialization
│       ├── sd_card_manager.py  # SD card operations
│       ├── config.py           # Configuration management
│       ├── assistant.py        # Virtual assistant logic
│       └── main.py            # Application entry point
├── tests/
│   ├── test_sd_card_manager.py # SD card tests
│   └── test_assistant.py       # Assistant tests
├── requirements.txt            # Python dependencies
├── setup.py                   # Package setup
├── .gitignore                 # Git ignore rules
└── README.md                  # This file
```

## Chạy Tests / Running Tests

Chạy tất cả tests:

```bash
python -m unittest discover tests
```

Chạy test cụ thể:

```bash
python -m unittest tests.test_sd_card_manager
python -m unittest tests.test_assistant
```

## Cấu hình / Configuration

File cấu hình `config.json` sẽ được tạo tự động khi chạy lần đầu. Bạn có thể chỉnh sửa các giá trị sau:

```json
{
  "assistant_name": "XiaoZhi",
  "owner_name": "Phuong Anh",
  "storage_path": "./sd_card_storage",
  "language": "vi",
  "auto_backup": true,
  "max_storage_mb": 1024
}
```

## API Documentation

### SDCardManager Class

Quản lý các thao tác với SD card.

#### Methods:

- `get_storage_info()` - Lấy thông tin bộ nhớ
- `list_files(subdirectory="")` - Liệt kê files
- `read_file(file_path, binary=False)` - Đọc file
- `write_file(file_path, content, binary=False)` - Ghi file
- `delete_file(file_path)` - Xóa file
- `create_directory(dir_path)` - Tạo thư mục
- `file_exists(file_path)` - Kiểm tra file tồn tại
- `save_json(file_path, data)` - Lưu JSON
- `load_json(file_path)` - Đọc JSON
- `get_file_size(file_path)` - Lấy kích thước file
- `clear_storage()` - Xóa toàn bộ bộ nhớ

### VirtualAssistant Class

Trợ lý ảo với khả năng xử lý lệnh.

#### Methods:

- `greet()` - Chào hỏi người dùng
- `process_command(command)` - Xử lý lệnh
- `save_note(title, content)` - Lưu ghi chú
- `get_notes()` - Lấy tất cả ghi chú

## Phát triển / Development

### Thêm tính năng mới / Adding New Features

1. Tạo module mới trong `src/xiaozhi/`
2. Thêm tests trong `tests/`
3. Cập nhật documentation

### Code Style

Dự án tuân theo PEP 8 Python style guide.

## Đóng góp / Contributing

Mọi đóng góp đều được chào đón! Vui lòng:

1. Fork repository
2. Tạo branch mới (`git checkout -b feature/AmazingFeature`)
3. Commit changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to branch (`git push origin feature/AmazingFeature`)
5. Mở Pull Request

## License

Dự án này được phát hành dưới giấy phép MIT.

## Tác giả / Authors

- Viet Trung Nhat Travel Trade Co., Ltd

## Liên hệ / Contact

- Email: viettrungnhattraveltradeco.ltd@gmail.com
- GitHub: [@viettrungnhat](https://github.com/viettrungnhat)

---

Made with ❤️ for Phuong Anh
