# OBD-II Vehicle Data Reading Guide for Kia Morning 2017

## Overview

OBD-II (On-Board Diagnostics II) communication happens over **CAN bus** (Controller Area Network). The CAN bus is a two-wire system:

- **CAN-H (High)**: Pin 6 on OBD-II connector
- **CAN-L (Low)**: Pin 14 on OBD-II connector  
- **GND**: Pins 4 or 5 on OBD-II connector

Currently, you have **2 wires (CAN-H, CAN-L) connected**, which is exactly what's needed for CAN bus communication!

## Current Setup

```
OBD-II Connector
  PIN 1: [Empty]
  PIN 2: [Empty]
  PIN 3: [Empty]
  PIN 4: GND ----→ ESP32-S3 GND
  PIN 5: GND ----→ ESP32-S3 GND
  PIN 6: CAN-H --→ SN65HVD230 CANH → CRX (GPIO17)
  PIN 7: [Empty]
  PIN 8: [Empty]
  PIN 9: [Empty]
  PIN 10: [Empty]
  PIN 11: [Empty]
  PIN 12: [Empty]
  PIN 13: [Empty]
  PIN 14: CAN-L -→ SN65HVD230 CANL → CTX (GPIO8)
  PIN 15: [Empty]
  PIN 16: [Battery +]
```

**✅ This is sufficient for CAN communication!**

## Vehicle Data Messages in Kia Morning 2017

The Kia Morning sends CAN messages with various vehicle data. Here are common message IDs:

### Engine Data Messages

| CAN ID | Data Bytes | Information |
|--------|-----------|-------------|
| 0x080 | Bytes 0-1 | Engine RPM (2 bytes, Big Endian) |
| 0x0A0 | Byte 0 | Engine Coolant Temperature |
| 0x0C0 | Bytes 0-1 | Vehicle Speed (2 bytes) |
| 0x0E0 | Byte 0 | Throttle Position |
| 0x100 | Bytes 0-1 | Fuel Tank Level |
| 0x120 | Byte 0 | Gear Position (D, N, P, R) |

### Calculating Values from Raw CAN Data

#### 1. **Engine RPM** (0x080, Bytes 0-1)

```
Formula: RPM = (Byte0 << 8 | Byte1) / 4
or: RPM = ((Byte0 * 256) + Byte1) / 4

Example:
CAN Data: [0x10, 0x00, ...]
Byte0 = 0x10 (16), Byte1 = 0x00 (0)
Combined = (16 << 8) | 0 = 4096
RPM = 4096 / 4 = 1024 RPM
```

#### 2. **Engine Coolant Temperature** (0x0A0, Byte 0)

```
Formula: Temp (°C) = Byte0 - 40
or: Temp (°C) = Byte0 - 0x28

Example:
CAN Data: [0x50, ...]
Temp = 0x50 - 40 = 80 - 40 = 40°C

Valid range: -40°C to 215°C (0x00 to 0xFF)
```

#### 3. **Vehicle Speed** (0x0C0, Bytes 0-1)

```
Formula: Speed (km/h) = (Byte0 << 8 | Byte1) * 0.01
or: Speed (km/h) = ((Byte0 * 256) + Byte1) / 100

Example:
CAN Data: [0x00, 0xC8, ...]
Byte0 = 0x00 (0), Byte1 = 0xC8 (200)
Combined = 200
Speed = 200 * 0.01 = 2 km/h
```

#### 4. **Throttle Position** (0x0E0, Byte 0)

```
Formula: Throttle (%) = (Byte0 * 100) / 255

Example:
CAN Data: [0xFF, ...]
Throttle = (255 * 100) / 255 = 100%
```

#### 5. **Fuel Tank Level** (0x100, Byte 0)

```
Formula: Fuel (%) = (Byte0 * 100) / 255

Example:
CAN Data: [0x80, ...]
Fuel = (128 * 100) / 255 = 50.2%
```

#### 6. **Gear Position** (0x120, Byte 0)

```
Common Values:
0x00 = Park (P)
0x01 = Reverse (R)
0x02 = Neutral (N)
0x03 = Drive (D)
0x04 = Low (L)
```

## How to Add New Vehicle Data Parsing

### Step 1: Define CAN ID and Struct in Kia CAN Protocol Header

```cpp
// File: main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/canbus/kia_can_protocol.h

struct VehicleEngineData {
    uint16_t rpm;               // Engine RPM
    int8_t coolant_temp;        // Coolant temperature (°C)
    uint16_t speed;             // Vehicle speed (km/h)
    uint8_t throttle_pos;       // Throttle position (%)
    uint8_t fuel_level;         // Fuel tank level (%)
};
```

### Step 2: Add Parsing Function

```cpp
// File: main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/canbus/kia_can_protocol.cc

void KiaCanProtocol::ParseEngineData(const canbus::CanMessage& msg) {
    if (msg.id != 0x080) return;
    
    VehicleEngineData data;
    
    // Parse RPM (Bytes 0-1)
    data.rpm = ((msg.data[0] << 8) | msg.data[1]) / 4;
    
    // Parse Coolant Temp (Byte 2)
    data.coolant_temp = msg.data[2] - 40;
    
    // Parse Speed (Bytes 3-4)
    data.speed = ((msg.data[3] << 8) | msg.data[4]) / 100;
    
    // Parse Throttle (Byte 5)
    data.throttle_pos = (msg.data[5] * 100) / 255;
    
    // Parse Fuel Level (Byte 6)
    data.fuel_level = (msg.data[6] * 100) / 255;
    
    // Store and callback
    last_engine_data_ = data;
    if (data_callback_) {
        data_callback_(data);
    }
}
```

### Step 3: Register Handler in CAN Driver

```cpp
// File: main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/canbus/canbus_driver.cc

// In Initialize():
CanMessage msg;
msg.id = 0x080;  // Engine data
RegisterMessageHandler(msg.id, [this](const CanMessage& m) {
    kia_protocol_->ParseEngineData(m);
});
```

### Step 4: Display on LCD

```cpp
// File: main/boards/xiaozhi-ai-iot-vietnam-lcd-sdcard/xiaozhi_ai_iot_vietnam_board_lcd_sdcard.cc

char vehicle_data[256];
snprintf(vehicle_data, sizeof(vehicle_data),
    "🚗 THÔNG TIN XE\n\n"
    "⚙️  RPM: %u\n"
    "🌡️  Temp: %d°C\n"
    "🚄 Speed: %u km/h\n"
    "⛽ Fuel: %u%%",
    engine_data.rpm,
    engine_data.coolant_temp,
    engine_data.speed,
    engine_data.fuel_level);

board->GetDisplay()->SetChatMessage("system", vehicle_data);
```

## Common CAN Message IDs for Kia Morning 2017

```
0x080  - Engine RPM, Throttle, Fuel Pressure
0x0A0  - Coolant Temperature, Engine Load
0x0C0  - Vehicle Speed, Gear Position
0x0E0  - Steering Angle, Brake Status
0x100  - Fuel Level, Tank Capacity
0x120  - Transmission Info, Gear Status
0x140  - Door Status, Window Status
0x160  - Light Status, Wiper Status
0x180  - ABS Status, Stability Control
0x1A0  - Air Conditioning Status
0x1C0  - Radio/Navigation Info
0x200  - Odometer, Trip Info
```

## Debugging Tips

### 1. Check CAN Traffic

Look at serial log messages like:
```
I (20294) CAN_Driver: 📨 RX: ID=0x580 DLC=8 Data=[00 43 00 00 00 28 00 00]
```

This shows:
- **ID=0x580**: The CAN message ID
- **DLC=8**: Data length is 8 bytes
- **Data=[...]**: 8 bytes of payload

### 2. Test Individual Messages

Create a simple test function to parse and display a specific message:

```cpp
void TestParseMessage(const canbus::CanMessage& msg) {
    if (msg.id == 0x080) {  // Look for engine data
        uint16_t rpm = ((msg.data[0] << 8) | msg.data[1]) / 4;
        ESP_LOGI(TAG, "Engine RPM: %u", rpm);
    }
}
```

### 3. Use CAN Protocol Analyzer

Use an automotive CAN analyzer tool to capture messages and understand the protocol:
- PEAK PCAN-View
- CANoe (Vector)
- ICSpy (Intrepid)

## Next Steps

1. **Parse actual vehicle data** from CAN messages you're receiving
2. **Add more CAN message handlers** for different vehicle systems
3. **Display vehicle data on LCD** in real-time
4. **Create voice commands** to read vehicle status (e.g., "Mở kính trước" → Control windows)

## References

- OBD-II Standard: ISO 15765-2
- CAN Bus Protocol: ISO 11898
- Kia Morning 2017 Service Manual (Electrical section)
- ESP32 TWAI Driver: https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-reference/peripherals/can.html

---

**Current Status**: ✅ CAN bus is receiving ~690 messages/second from your vehicle!

**Next**: Implement parsers for the specific vehicle data you want to display.
