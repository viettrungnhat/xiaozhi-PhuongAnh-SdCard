"""
Main Application Entry Point
Starts the virtual assistant with SD card support.
"""

import sys
from .assistant import VirtualAssistant
from .config import Config


def interactive_mode():
    """
    Run the assistant in interactive mode.
    """
    print("=" * 60)
    print("XiaoZhi - Trợ Lý Ảo với Hỗ Trợ SD Card")
    print("Phiên bản 0.1.0")
    print("=" * 60)
    print()
    
    # Initialize assistant
    config = Config()
    assistant = VirtualAssistant(config)
    
    # Greet user
    print(assistant.greet())
    print()
    print("Gõ 'help' để xem danh sách lệnh, hoặc 'exit' để thoát.")
    print()
    
    # Main loop
    while True:
        try:
            command = input(f"{config.get('owner_name', 'User')} > ").strip()
            
            if not command:
                continue
            
            if command.lower() in ["exit", "quit", "thoát"]:
                print(f"Tạm biệt {config.get('owner_name', 'User')}! Hẹn gặp lại!")
                break
            
            # Process command
            result = assistant.process_command(command)
            print()
            print(result["message"])
            print()
            
        except KeyboardInterrupt:
            print("\n\nTạm biệt! Hẹn gặp lại!")
            break
        except Exception as e:
            print(f"\nLỗi: {e}\n")


def demo_mode():
    """
    Run a demo of the assistant capabilities.
    """
    print("=" * 60)
    print("XiaoZhi - Demo Mode")
    print("=" * 60)
    print()
    
    # Initialize assistant
    config = Config()
    assistant = VirtualAssistant(config)
    
    # Demo commands
    demo_commands = [
        ("Greeting", ""),
        ("Help", "help"),
        ("Storage Info", "storage"),
        ("Write File", "write demo.txt Đây là file demo từ XiaoZhi!"),
        ("Write Another File", "write test.txt Hello from XiaoZhi virtual assistant"),
        ("List Files", "list"),
        ("Read File", "read demo.txt"),
        ("Save Note", ""),
        ("Storage Info", "storage"),
    ]
    
    print(assistant.greet())
    print()
    
    for title, command in demo_commands:
        if title == "Greeting":
            continue
        
        print(f">>> {title}")
        if command:
            print(f"Command: {command}")
            result = assistant.process_command(command)
            print(result["message"])
        elif title == "Save Note":
            print("Saving a note...")
            success = assistant.save_note("My First Note", "This is my first note in XiaoZhi!")
            if success:
                print("✓ Note saved successfully!")
            else:
                print("✗ Failed to save note.")
        print()
    
    print("Demo completed!")
    print()


def main():
    """
    Main entry point for the application.
    """
    if len(sys.argv) > 1:
        if sys.argv[1] == "--demo":
            demo_mode()
        elif sys.argv[1] == "--help":
            print("XiaoZhi - Virtual Assistant with SD Card Support")
            print()
            print("Usage:")
            print("  xiaozhi            Run in interactive mode")
            print("  xiaozhi --demo     Run demo mode")
            print("  xiaozhi --help     Show this help message")
            print()
        else:
            print(f"Unknown option: {sys.argv[1]}")
            print("Use --help for usage information.")
    else:
        interactive_mode()


if __name__ == "__main__":
    main()
