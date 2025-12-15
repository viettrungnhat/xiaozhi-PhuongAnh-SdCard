"""
Unit tests for Virtual Assistant
"""

import unittest
import tempfile
import shutil
import sys
import os

# Add src to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))

from xiaozhi.assistant import VirtualAssistant
from xiaozhi.config import Config


class TestVirtualAssistant(unittest.TestCase):
    """Test cases for VirtualAssistant class."""

    def setUp(self):
        """Set up test fixtures."""
        # Create a temporary directory for testing
        self.test_dir = tempfile.mkdtemp()
        
        # Create config with test directory
        self.config = Config.__new__(Config)
        self.config.config_file = os.path.join(self.test_dir, "config.json")
        self.config.config = {
            "assistant_name": "TestAssistant",
            "owner_name": "Test User",
            "storage_path": os.path.join(self.test_dir, "storage"),
            "language": "vi"
        }
        
        self.assistant = VirtualAssistant(self.config)

    def tearDown(self):
        """Clean up test fixtures."""
        if os.path.exists(self.test_dir):
            shutil.rmtree(self.test_dir)

    def test_greet(self):
        """Test greeting message."""
        greeting = self.assistant.greet()
        self.assertIn("Test User", greeting)
        self.assertIn("TestAssistant", greeting)

    def test_help_command(self):
        """Test help command."""
        result = self.assistant.process_command("help")
        self.assertEqual(result["status"], "success")
        self.assertIn("Lệnh có sẵn", result["message"])

    def test_storage_info_command(self):
        """Test storage info command."""
        result = self.assistant.process_command("storage")
        self.assertEqual(result["status"], "success")
        self.assertIn("data", result)
        self.assertIn("total_gb", result["data"])

    def test_write_and_read_file_command(self):
        """Test write and read file commands."""
        # Write file
        write_result = self.assistant.process_command("write test.txt Hello World")
        self.assertEqual(write_result["status"], "success")
        
        # Read file
        read_result = self.assistant.process_command("read test.txt")
        self.assertEqual(read_result["status"], "success")
        self.assertIn("Hello World", read_result["message"])

    def test_list_files_command(self):
        """Test list files command."""
        # Create some files
        self.assistant.process_command("write file1.txt content1")
        self.assistant.process_command("write file2.txt content2")
        
        # List files
        result = self.assistant.process_command("list")
        self.assertEqual(result["status"], "success")
        self.assertEqual(len(result["data"]), 2)

    def test_delete_file_command(self):
        """Test delete file command."""
        # Create file
        self.assistant.process_command("write delete_me.txt temporary")
        
        # Delete file
        result = self.assistant.process_command("delete delete_me.txt")
        self.assertEqual(result["status"], "success")
        
        # Verify file is gone
        read_result = self.assistant.process_command("read delete_me.txt")
        self.assertEqual(read_result["status"], "error")

    def test_save_note(self):
        """Test saving a note."""
        success = self.assistant.save_note("Test Note", "This is a test note.")
        self.assertTrue(success)

    def test_get_notes(self):
        """Test getting notes."""
        # Save some notes
        self.assistant.save_note("Note 1", "Content 1")
        self.assistant.save_note("Note 2", "Content 2")
        
        # Get notes
        notes = self.assistant.get_notes()
        self.assertEqual(len(notes), 2)

    def test_unknown_command(self):
        """Test handling unknown command."""
        result = self.assistant.process_command("unknown_command")
        self.assertEqual(result["status"], "error")

    def test_vietnamese_commands(self):
        """Test Vietnamese command variants."""
        # Help command in Vietnamese
        result1 = self.assistant.process_command("trợ giúp")
        self.assertEqual(result1["status"], "success")
        
        # Storage command in Vietnamese
        result2 = self.assistant.process_command("bộ nhớ")
        self.assertEqual(result2["status"], "success")
        
        # List command in Vietnamese
        result3 = self.assistant.process_command("danh sách")
        self.assertEqual(result3["status"], "success")


if __name__ == '__main__':
    unittest.main()
