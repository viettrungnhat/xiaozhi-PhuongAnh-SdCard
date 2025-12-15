"""
Virtual Assistant Module
Main assistant logic and command processing.
"""

from typing import Dict, List, Optional
from .sd_card_manager import SDCardManager
from .config import Config


class VirtualAssistant:
    """
    Virtual Assistant with SD card management capabilities.
    """

    def __init__(self, config: Config = None):
        """
        Initialize the virtual assistant.
        
        Args:
            config: Configuration object. If None, creates default config.
        """
        self.config = config if config else Config()
        self.sd_manager = SDCardManager(self.config.get("storage_path"))
        self.assistant_name = self.config.get("assistant_name", "XiaoZhi")
        self.owner_name = self.config.get("owner_name", "Phuong Anh")

    def greet(self) -> str:
        """
        Greet the user.
        
        Returns:
            Greeting message.
        """
        return f"Xin chào {self.owner_name}! Tôi là {self.assistant_name}, trợ lý ảo của bạn với hỗ trợ SD card."

    def process_command(self, command: str) -> Dict:
        """
        Process a command from the user.
        
        Args:
            command: Command string.
        
        Returns:
            Dictionary with 'status', 'message', and optional 'data'.
        """
        command = command.strip()
        command_lower = command.lower()
        
        if command_lower in ["help", "trợ giúp", "giúp đỡ"]:
            return self._help_command()
        elif command_lower in ["storage", "bộ nhớ", "lưu trữ"]:
            return self._storage_info_command()
        elif command_lower in ["list", "danh sách", "files"]:
            return self._list_files_command()
        elif command_lower.startswith("read ") or command_lower.startswith("đọc "):
            return self._read_file_command(command)
        elif command_lower.startswith("write ") or command_lower.startswith("viết "):
            return self._write_file_command(command)
        elif command_lower.startswith("delete ") or command_lower.startswith("xóa "):
            return self._delete_file_command(command)
        else:
            return {
                "status": "error",
                "message": "Không hiểu lệnh. Gõ 'help' để xem danh sách lệnh."
            }

    def _help_command(self) -> Dict:
        """Show help information."""
        help_text = """
Lệnh có sẵn:
- help / trợ giúp: Hiển thị trợ giúp
- storage / bộ nhớ: Xem thông tin bộ nhớ SD card
- list / danh sách: Liệt kê tất cả files
- read <tên_file> / đọc <tên_file>: Đọc nội dung file
- write <tên_file> <nội_dung> / viết <tên_file> <nội_dung>: Ghi file
- delete <tên_file> / xóa <tên_file>: Xóa file
"""
        return {
            "status": "success",
            "message": help_text
        }

    def _storage_info_command(self) -> Dict:
        """Get storage information."""
        info = self.sd_manager.get_storage_info()
        message = f"""
Thông tin bộ nhớ SD Card:
- Tổng dung lượng: {info['total_gb']} GB
- Đã sử dụng: {info['used_gb']} GB ({info['usage_percent']}%)
- Còn trống: {info['free_gb']} GB
- Đường dẫn: {info['storage_path']}
"""
        return {
            "status": "success",
            "message": message,
            "data": info
        }

    def _list_files_command(self) -> Dict:
        """List all files in storage."""
        files = self.sd_manager.list_files()
        
        if not files:
            return {
                "status": "success",
                "message": "Không có file nào trong bộ nhớ.",
                "data": []
            }
        
        file_list = "\n".join([
            f"- {f['name']} ({f['size_kb']} KB) - {f['modified']}"
            for f in files
        ])
        
        return {
            "status": "success",
            "message": f"Tìm thấy {len(files)} file(s):\n{file_list}",
            "data": files
        }

    def _read_file_command(self, command: str) -> Dict:
        """Read a file from storage."""
        parts = command.split(maxsplit=1)
        if len(parts) < 2:
            return {
                "status": "error",
                "message": "Vui lòng chỉ định tên file. Ví dụ: read myfile.txt"
            }
        
        file_path = parts[1]
        content = self.sd_manager.read_file(file_path)
        
        if content is None:
            return {
                "status": "error",
                "message": f"Không thể đọc file '{file_path}'. File có thể không tồn tại."
            }
        
        return {
            "status": "success",
            "message": f"Nội dung của '{file_path}':\n{content}",
            "data": {"file": file_path, "content": content}
        }

    def _write_file_command(self, command: str) -> Dict:
        """Write content to a file."""
        parts = command.split(maxsplit=2)
        if len(parts) < 3:
            return {
                "status": "error",
                "message": "Vui lòng chỉ định tên file và nội dung. Ví dụ: write myfile.txt Hello World"
            }
        
        file_path = parts[1]
        content = parts[2]
        
        success = self.sd_manager.write_file(file_path, content)
        
        if success:
            return {
                "status": "success",
                "message": f"Đã ghi file '{file_path}' thành công."
            }
        else:
            return {
                "status": "error",
                "message": f"Không thể ghi file '{file_path}'."
            }

    def _delete_file_command(self, command: str) -> Dict:
        """Delete a file from storage."""
        parts = command.split(maxsplit=1)
        if len(parts) < 2:
            return {
                "status": "error",
                "message": "Vui lòng chỉ định tên file. Ví dụ: delete myfile.txt"
            }
        
        file_path = parts[1]
        success = self.sd_manager.delete_file(file_path)
        
        if success:
            return {
                "status": "success",
                "message": f"Đã xóa file '{file_path}' thành công."
            }
        else:
            return {
                "status": "error",
                "message": f"Không thể xóa file '{file_path}'. File có thể không tồn tại."
            }

    def save_note(self, title: str, content: str) -> bool:
        """
        Save a note to storage.
        
        Args:
            title: Note title.
            content: Note content.
        
        Returns:
            True if successful, False otherwise.
        """
        from datetime import datetime
        
        note_data = {
            "title": title,
            "content": content,
            "created": datetime.now().isoformat(),
            "owner": self.owner_name
        }
        
        filename = f"notes/{title.replace(' ', '_')}.json"
        return self.sd_manager.save_json(filename, note_data)

    def get_notes(self) -> List[Dict]:
        """
        Get all notes from storage.
        
        Returns:
            List of note dictionaries.
        """
        files = self.sd_manager.list_files("notes")
        notes = []
        
        for file_info in files:
            if file_info["name"].endswith(".json"):
                note = self.sd_manager.load_json(file_info["path"])
                if note:
                    notes.append(note)
        
        return notes
