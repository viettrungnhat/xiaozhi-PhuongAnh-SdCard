"""
Unit tests for SD Card Manager
"""

import unittest
import tempfile
import shutil
from pathlib import Path
import sys
import os

# Add src to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))

from xiaozhi.sd_card_manager import SDCardManager


class TestSDCardManager(unittest.TestCase):
    """Test cases for SDCardManager class."""

    def setUp(self):
        """Set up test fixtures."""
        # Create a temporary directory for testing
        self.test_dir = tempfile.mkdtemp()
        self.manager = SDCardManager(self.test_dir)

    def tearDown(self):
        """Clean up test fixtures."""
        # Remove the temporary directory
        if os.path.exists(self.test_dir):
            shutil.rmtree(self.test_dir)

    def test_storage_initialization(self):
        """Test that storage directory is created."""
        self.assertTrue(os.path.exists(self.test_dir))
        self.assertTrue(os.path.isdir(self.test_dir))

    def test_get_storage_info(self):
        """Test getting storage information."""
        info = self.manager.get_storage_info()
        
        self.assertIn('total_bytes', info)
        self.assertIn('used_bytes', info)
        self.assertIn('free_bytes', info)
        self.assertIn('total_gb', info)
        self.assertIn('free_gb', info)
        self.assertIn('usage_percent', info)
        self.assertGreater(info['total_bytes'], 0)

    def test_write_and_read_file(self):
        """Test writing and reading a text file."""
        test_content = "Hello, XiaoZhi!"
        test_file = "test.txt"
        
        # Write file
        success = self.manager.write_file(test_file, test_content)
        self.assertTrue(success)
        
        # Read file
        content = self.manager.read_file(test_file)
        self.assertEqual(content, test_content)

    def test_write_and_read_binary_file(self):
        """Test writing and reading a binary file."""
        test_content = b'\x00\x01\x02\x03\x04'
        test_file = "test.bin"
        
        # Write binary file
        success = self.manager.write_file(test_file, test_content, binary=True)
        self.assertTrue(success)
        
        # Read binary file
        content = self.manager.read_file(test_file, binary=True)
        self.assertEqual(content, test_content)

    def test_file_exists(self):
        """Test checking if a file exists."""
        test_file = "exists.txt"
        
        # File doesn't exist yet
        self.assertFalse(self.manager.file_exists(test_file))
        
        # Create file
        self.manager.write_file(test_file, "content")
        
        # Now it should exist
        self.assertTrue(self.manager.file_exists(test_file))

    def test_delete_file(self):
        """Test deleting a file."""
        test_file = "delete_me.txt"
        
        # Create file
        self.manager.write_file(test_file, "temporary content")
        self.assertTrue(self.manager.file_exists(test_file))
        
        # Delete file
        success = self.manager.delete_file(test_file)
        self.assertTrue(success)
        self.assertFalse(self.manager.file_exists(test_file))

    def test_delete_nonexistent_file(self):
        """Test deleting a file that doesn't exist."""
        success = self.manager.delete_file("nonexistent.txt")
        self.assertFalse(success)

    def test_list_files(self):
        """Test listing files in storage."""
        # Create some test files
        self.manager.write_file("file1.txt", "content 1")
        self.manager.write_file("file2.txt", "content 2")
        self.manager.write_file("subdir/file3.txt", "content 3")
        
        # List files
        files = self.manager.list_files()
        
        self.assertEqual(len(files), 3)
        file_names = [f['name'] for f in files]
        self.assertIn('file1.txt', file_names)
        self.assertIn('file2.txt', file_names)
        self.assertIn('file3.txt', file_names)

    def test_create_directory(self):
        """Test creating a directory."""
        dir_path = "test_directory/nested"
        
        success = self.manager.create_directory(dir_path)
        self.assertTrue(success)
        
        full_path = Path(self.test_dir) / dir_path
        self.assertTrue(full_path.exists())
        self.assertTrue(full_path.is_dir())

    def test_save_and_load_json(self):
        """Test saving and loading JSON data."""
        test_data = {
            "name": "XiaoZhi",
            "version": "0.1.0",
            "features": ["sd_card", "notes", "assistant"]
        }
        test_file = "test_data.json"
        
        # Save JSON
        success = self.manager.save_json(test_file, test_data)
        self.assertTrue(success)
        
        # Load JSON
        loaded_data = self.manager.load_json(test_file)
        self.assertEqual(loaded_data, test_data)

    def test_get_file_size(self):
        """Test getting file size."""
        test_content = "Hello" * 100
        test_file = "size_test.txt"
        
        self.manager.write_file(test_file, test_content)
        
        size = self.manager.get_file_size(test_file)
        self.assertEqual(size, len(test_content))

    def test_read_nonexistent_file(self):
        """Test reading a file that doesn't exist."""
        content = self.manager.read_file("nonexistent.txt")
        self.assertIsNone(content)

    def test_clear_storage(self):
        """Test clearing all storage."""
        # Create some files
        self.manager.write_file("file1.txt", "content")
        self.manager.write_file("file2.txt", "content")
        self.manager.create_directory("subdir")
        
        # Clear storage
        success = self.manager.clear_storage()
        self.assertTrue(success)
        
        # Verify storage is empty
        files = self.manager.list_files()
        self.assertEqual(len(files), 0)


if __name__ == '__main__':
    unittest.main()
