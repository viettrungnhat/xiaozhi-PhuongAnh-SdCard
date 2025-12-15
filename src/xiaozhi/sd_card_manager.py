"""
SD Card Manager Module
Handles all SD card operations including reading, writing, and monitoring storage.
"""

import os
import shutil
import json
from pathlib import Path
from typing import List, Dict, Optional, Tuple
from datetime import datetime


class SDCardManager:
    """
    Manages SD card operations for the virtual assistant.
    Provides file management, storage monitoring, and data operations.
    """

    def __init__(self, storage_path: str = None):
        """
        Initialize SD Card Manager.
        
        Args:
            storage_path: Path to the storage directory (simulates SD card).
                         If None, uses default './sd_card_storage'
        """
        if storage_path is None:
            storage_path = os.path.join(os.getcwd(), "sd_card_storage")
        
        self.storage_path = Path(storage_path)
        self._ensure_storage_exists()

    def _ensure_storage_exists(self):
        """Ensure the storage directory exists."""
        self.storage_path.mkdir(parents=True, exist_ok=True)

    def get_storage_info(self) -> Dict[str, any]:
        """
        Get storage information including total, used, and free space.
        
        Returns:
            Dictionary containing storage statistics in bytes and human-readable format.
        """
        total, used, free = shutil.disk_usage(self.storage_path)
        
        return {
            "total_bytes": total,
            "used_bytes": used,
            "free_bytes": free,
            "total_gb": round(total / (1024 ** 3), 2),
            "used_gb": round(used / (1024 ** 3), 2),
            "free_gb": round(free / (1024 ** 3), 2),
            "usage_percent": round((used / total) * 100, 2) if total > 0 else 0,
            "storage_path": str(self.storage_path)
        }

    def list_files(self, subdirectory: str = "") -> List[Dict[str, any]]:
        """
        List all files in the storage or a specific subdirectory.
        
        Args:
            subdirectory: Optional subdirectory path relative to storage root.
        
        Returns:
            List of dictionaries containing file information.
        """
        target_path = self.storage_path / subdirectory if subdirectory else self.storage_path
        
        if not target_path.exists():
            return []
        
        files = []
        for item in target_path.rglob("*"):
            if item.is_file():
                stat = item.stat()
                files.append({
                    "name": item.name,
                    "path": str(item.relative_to(self.storage_path)),
                    "size_bytes": stat.st_size,
                    "size_kb": round(stat.st_size / 1024, 2),
                    "modified": datetime.fromtimestamp(stat.st_mtime).isoformat(),
                    "created": datetime.fromtimestamp(stat.st_ctime).isoformat()
                })
        
        return sorted(files, key=lambda x: x["modified"], reverse=True)

    def read_file(self, file_path: str, binary: bool = False) -> Optional[any]:
        """
        Read a file from storage.
        
        Args:
            file_path: Path to the file relative to storage root.
            binary: If True, read in binary mode; otherwise text mode.
        
        Returns:
            File content as string (text mode) or bytes (binary mode).
            Returns None if file doesn't exist.
        """
        full_path = self.storage_path / file_path
        
        if not full_path.exists() or not full_path.is_file():
            return None
        
        try:
            mode = "rb" if binary else "r"
            with open(full_path, mode) as f:
                return f.read()
        except Exception as e:
            print(f"Error reading file {file_path}: {e}")
            return None

    def write_file(self, file_path: str, content: any, binary: bool = False) -> bool:
        """
        Write content to a file in storage.
        
        Args:
            file_path: Path to the file relative to storage root.
            content: Content to write (string for text, bytes for binary).
            binary: If True, write in binary mode; otherwise text mode.
        
        Returns:
            True if successful, False otherwise.
        """
        full_path = self.storage_path / file_path
        
        # Create parent directories if they don't exist
        full_path.parent.mkdir(parents=True, exist_ok=True)
        
        try:
            mode = "wb" if binary else "w"
            with open(full_path, mode) as f:
                f.write(content)
            return True
        except Exception as e:
            print(f"Error writing file {file_path}: {e}")
            return False

    def delete_file(self, file_path: str) -> bool:
        """
        Delete a file from storage.
        
        Args:
            file_path: Path to the file relative to storage root.
        
        Returns:
            True if successful, False otherwise.
        """
        full_path = self.storage_path / file_path
        
        if not full_path.exists():
            return False
        
        try:
            if full_path.is_file():
                full_path.unlink()
                return True
            else:
                return False
        except Exception as e:
            print(f"Error deleting file {file_path}: {e}")
            return False

    def create_directory(self, dir_path: str) -> bool:
        """
        Create a directory in storage.
        
        Args:
            dir_path: Path to the directory relative to storage root.
        
        Returns:
            True if successful, False otherwise.
        """
        full_path = self.storage_path / dir_path
        
        try:
            full_path.mkdir(parents=True, exist_ok=True)
            return True
        except Exception as e:
            print(f"Error creating directory {dir_path}: {e}")
            return False

    def file_exists(self, file_path: str) -> bool:
        """
        Check if a file exists in storage.
        
        Args:
            file_path: Path to the file relative to storage root.
        
        Returns:
            True if file exists, False otherwise.
        """
        full_path = self.storage_path / file_path
        return full_path.exists() and full_path.is_file()

    def save_json(self, file_path: str, data: dict) -> bool:
        """
        Save data as JSON file.
        
        Args:
            file_path: Path to the file relative to storage root.
            data: Dictionary to save as JSON.
        
        Returns:
            True if successful, False otherwise.
        """
        try:
            json_content = json.dumps(data, indent=2, ensure_ascii=False)
            return self.write_file(file_path, json_content)
        except Exception as e:
            print(f"Error saving JSON to {file_path}: {e}")
            return False

    def load_json(self, file_path: str) -> Optional[dict]:
        """
        Load data from JSON file.
        
        Args:
            file_path: Path to the file relative to storage root.
        
        Returns:
            Dictionary from JSON file, or None if error occurs.
        """
        content = self.read_file(file_path)
        
        if content is None:
            return None
        
        try:
            return json.loads(content)
        except Exception as e:
            print(f"Error loading JSON from {file_path}: {e}")
            return None

    def get_file_size(self, file_path: str) -> Optional[int]:
        """
        Get the size of a file in bytes.
        
        Args:
            file_path: Path to the file relative to storage root.
        
        Returns:
            File size in bytes, or None if file doesn't exist.
        """
        full_path = self.storage_path / file_path
        
        if not full_path.exists() or not full_path.is_file():
            return None
        
        return full_path.stat().st_size

    def clear_storage(self) -> bool:
        """
        Clear all files and directories in storage.
        WARNING: This will delete all data!
        
        Returns:
            True if successful, False otherwise.
        """
        try:
            for item in self.storage_path.iterdir():
                if item.is_file():
                    item.unlink()
                elif item.is_dir():
                    shutil.rmtree(item)
            return True
        except Exception as e:
            print(f"Error clearing storage: {e}")
            return False
