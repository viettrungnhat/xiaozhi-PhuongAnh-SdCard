"""
Configuration Module
Manages application configuration and settings.
"""

import json
import os
from pathlib import Path
from typing import Dict, Optional


class Config:
    """
    Configuration manager for the virtual assistant.
    """

    DEFAULT_CONFIG = {
        "assistant_name": "XiaoZhi",
        "owner_name": "Phuong Anh",
        "storage_path": "./sd_card_storage",
        "language": "vi",
        "auto_backup": True,
        "max_storage_mb": 1024
    }

    def __init__(self, config_file: str = "config.json"):
        """
        Initialize configuration.
        
        Args:
            config_file: Path to the configuration file.
        """
        self.config_file = config_file
        self.config = self._load_config()

    def _load_config(self) -> Dict:
        """
        Load configuration from file or create with defaults.
        
        Returns:
            Configuration dictionary.
        """
        if os.path.exists(self.config_file):
            try:
                with open(self.config_file, 'r', encoding='utf-8') as f:
                    config = json.load(f)
                # Merge with defaults for any missing keys
                return {**self.DEFAULT_CONFIG, **config}
            except Exception as e:
                print(f"Error loading config: {e}. Using defaults.")
                return self.DEFAULT_CONFIG.copy()
        else:
            # Create default config file
            self.save_config(self.DEFAULT_CONFIG)
            return self.DEFAULT_CONFIG.copy()

    def save_config(self, config: Dict = None) -> bool:
        """
        Save configuration to file.
        
        Args:
            config: Configuration dictionary to save. If None, saves current config.
        
        Returns:
            True if successful, False otherwise.
        """
        if config is not None:
            self.config = config

        try:
            with open(self.config_file, 'w', encoding='utf-8') as f:
                json.dump(self.config, f, indent=2, ensure_ascii=False)
            return True
        except Exception as e:
            print(f"Error saving config: {e}")
            return False

    def get(self, key: str, default=None):
        """
        Get a configuration value.
        
        Args:
            key: Configuration key.
            default: Default value if key doesn't exist.
        
        Returns:
            Configuration value.
        """
        return self.config.get(key, default)

    def set(self, key: str, value) -> bool:
        """
        Set a configuration value and save.
        
        Args:
            key: Configuration key.
            value: Value to set.
        
        Returns:
            True if successful, False otherwise.
        """
        self.config[key] = value
        return self.save_config()

    def get_all(self) -> Dict:
        """
        Get all configuration values.
        
        Returns:
            Configuration dictionary.
        """
        return self.config.copy()
