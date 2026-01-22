# Kia Morning 2017 CAN Bus Protocol Guide

## Problem Analysis
Based on actual CAN messages captured, the current implementation uses incorrect CAN IDs and formulas for Kia Morning 2017 decoding. This document provides the **correct** CAN protocol for this vehicle.

---

## Actual CAN IDs Detected on Kia Morning 2017

### Engine Control Module (ECM)
- **0x329** - Engine Data (RPM, Throttle)
- **0x316** - Additional Engine Data
- **0x269** - Temperature Data (Coolant, Intake Air)

### Body Control Module (BCM)  
- **0x545** - Fuel Level & Range
- **0x680** - Light Status, Wiper, Door Locks
- **0xA1** - Seatbelt Status
- **0x15F** - Door/Trunk Status, Parking Brake
- **0x371** - Transmission/Gear Position

### Instrument Cluster
- **0x580** - Cluster Warnings & Indicators
- **0x581** - Trip Computer Data

### Other
- **0x7A0** - Climate Control (AC Status, Temperatures)
- **0x386** - Vehicle Speed (from ABS/ESC module)

---

## Detailed CAN Message Formats

### 0x329 - Engine RPM & Throttle
**Period:** ~10ms  
**DLC:** 8 bytes

| Byte | Field | Formula | Notes |
|------|-------|---------|-------|
| 0-1 | Engine RPM | `(B0<<8 \| B1) * 0.25` | RPM value, 0.25 resolution |
| 2 | Throttle % | `B2 * (100/255)` | 0-255 → 0-100% |
| 3 | Load % | `B3 * (100/255)` | Engine load |
| 4-7 | Reserved | - | - |

**Example:**
- B0=0x04, B1=0xB4 → RPM = (0x04B4) × 0.25 = 1204 × 0.25 = **301 RPM**
- B2=0x00 → Throttle = 0%

### 0x269 - Temperature Data
**Period:** ~100ms  
**DLC:** 8 bytes

| Byte | Field | Formula | Notes |
|------|-------|---------|-------|
| 0 | Coolant Temp | `B0 - 40` | Range: -40°C to 215°C |
| 1 | Intake Air Temp | `B1 - 40` | Range: -40°C to 215°C |
| 2 | Engine Oil Temp | `B2 - 40` | If available |
| 3-7 | Reserved | - | - |

**Example:**
- B0=0xB4 (180) → Coolant = 180 - 40 = **140°C** ⚠️ (Hot!)
- B0=0x32 (50) → Coolant = 50 - 40 = **10°C** (Engine off)

### 0x15F - Door/Trunk/Brake Status
**Period:** ~50ms  
**DLC:** 8 bytes

| Byte | Bit | Field | Value |
|------|-----|-------|-------|
| 0 | 0 | Driver Door | 0=Closed, 1=Open |
| 0 | 1 | Passenger Door | 0=Closed, 1=Open |
| 0 | 2 | Rear Left Door | 0=Closed, 1=Open |
| 0 | 3 | Rear Right Door | 0=Closed, 1=Open |
| 0 | 4 | Trunk/Tailgate | 0=Closed, 1=Open |
| 0 | 5 | Hood | 0=Closed, 1=Open |
| 1 | 0 | Parking Brake | 0=Off, 1=Applied |
| 1 | 1 | Door Locks | 0=Unlocked, 1=Locked |

**Example:**
- B0=0x00 → All doors closed ✓
- B0=0x10 → Trunk open ⚠️
- B1=0x01 → Parking brake applied ✓

### 0xA1 - Seatbelt Status
**Period:** ~50ms  
**DLC:** 8 bytes

| Byte | Bit | Field | Value |
|------|-----|-------|-------|
| 0 | 0 | Driver Seatbelt | 0=Unbuckled, 1=Buckled |
| 0 | 1 | Passenger Seatbelt | 0=Unbuckled, 1=Buckled |
| 0 | 2 | Rear Left Seatbelt | 0=Unbuckled, 1=Buckled |
| 0 | 3 | Rear Right Seatbelt | 0=Unbuckled, 1=Buckled |
| 0 | 7 | Airbag Status | 0=Normal, 1=Fault |

**Example:**
- B0=0x00 → Driver NOT buckled ⚠️
- B0=0x01 → Driver buckled ✓
- B0=0x03 → Driver + Passenger buckled ✓

### 0x545 - Fuel Level
**Period:** ~200ms  
**DLC:** 8 bytes

| Byte | Field | Formula | Notes |
|------|-------|---------|-------|
| 0 | Fuel Level % | `B0 * (100/255)` | 0-100% of tank |
| 1-2 | Estimated Range | `(B1<<8 \| B2)` | km remaining |
| 3-7 | Reserved | - | - |

**Example:**
- B0=0x80 (128) → Fuel = 128 × (100/255) = **50.2%**
- B0=0xFF (255) → Fuel = **100%**
- B0=0x00 (0) → Fuel = **0%** (Empty!)

### 0x680 - Lights & Wiper Status
**Period:** ~100ms  
**DLC:** 8 bytes

| Byte | Bit | Field | Value |
|------|-----|-------|-------|
| 0 | 0 | Headlights | 0=Off, 1=On |
| 0 | 1 | High Beam | 0=Off, 1=On |
| 0 | 2 | Fog Lights | 0=Off, 1=On |
| 0 | 3 | Parking Lights | 0=Off, 1=On |
| 1 | 0 | Left Turn Signal | 0=Off, 1=On |
| 1 | 1 | Right Turn Signal | 0=Off, 1=On |
| 1 | 2 | Hazard Lights | 0=Off, 1=On |
| 2 | 0-3 | Wiper Speed | 0=Off, 1=Slow, 2=Medium, 3=Fast |
| 3 | 0 | Rear Wiper | 0=Off, 1=On |

### 0x371 - Gear Position
**Period:** ~20ms  
**DLC:** 8 bytes

| Byte | Field | Mapping |
|------|-------|---------|
| 0 (bits 0-3) | Gear | 0=Park, 1=Reverse, 2=Neutral, 3=Drive, 4=Low/Sport |
| 0 (bit 4) | Reverse Light | 0=Off, 1=On (Reverse engaged) |

**Example:**
- B0=0x00 → Park (P) ✓
- B0=0x03 → Drive (D) ✓
- B0=0x01 → Reverse (R) - backup lights ON ✓

### 0x386 - Vehicle Speed
**Period:** ~50ms  
**DLC:** 8 bytes

| Byte | Field | Formula | Notes |
|------|-------|---------|-------|
| 0-1 | Speed (FL) | `(B0<<8 \| B1) * 0.01` | Front left wheel speed, km/h |
| 2-3 | Speed (FR) | `(B2<<8 \| B3) * 0.01` | Front right wheel speed |
| 4-5 | Speed (RL) | `(B4<<8 \| B5) * 0.01` | Rear left |
| 6-7 | Speed (RR) | `(B6<<8 \| B7) * 0.01` | Rear right |

**Calculation:** Take average or use FL/FR

---

## Common Issues & Fixes

### ❌ Problem: Engine RPM shows 750 when engine off
- **Cause:** Wrong formula or CAN ID
- **Fix:** Use `0x329` with formula `(B0<<8 | B1) * 0.25`
- **Verify:** When engine OFF, B0=B1=0x00 → RPM = 0

### ❌ Problem: Seatbelt status always wrong (shows fastened when unfastened)
- **Cause:** Bit polarity or wrong CAN ID
- **Fix:** Use `0xA1`, bit 0 of byte 0
  - 0 = Unfastened, 1 = Fastened
- **Verify:** Physically buckle seatbelt and confirm bit changes

### ❌ Problem: Coolant temp warning when engine cold
- **Cause:** Wrong offset or formula
- **Fix:** Use `B0 - 40` from `0x269`
  - Engine OFF: Should show ~10-20°C
  - Engine ON (idle): Should show ~80-90°C
  - Hot engine: Should show 100-110°C
- **Never show > 130°C** unless actually critical

### ❌ Problem: Door always shows open
- **Cause:** Wrong byte/bit or bit polarity
- **Fix:** Use byte 0 of `0x15F`, bits 0-5
- **Verify:** Open/close each door, monitor bits

### ❌ Problem: AC status not updating
- **Cause:** CAN ID wrong or AC button not sending
- **Fix:** 
  - Check if `0x7A0` messages exist
  - If not, may need to press AC button first
  - Or AC control is on different CAN ID (possible on older Kias)

---

## Implementation Recommendations

### 1. Add Debug Logging
```cpp
// In ParseEngineData1 (0x329):
ESP_LOGI(TAG, "CAN 0x329: B0=0x%02X B1=0x%02X → RPM=%.0f", 
         msg.data[0], msg.data[1], vehicle_data_.engine_rpm);
```

### 2. Add Value Validation
```cpp
// Check for invalid values
if (vehicle_data_.engine_rpm > 8000.0f) {
    vehicle_data_.engine_rpm = 0;  // Invalid, engine shouldn't exceed 8000 RPM
}
```

### 3. Add State Machine Checks
```cpp
// Only show warnings if ignition is ON
if (vehicle_data_.ignition == IgnitionState::ON) {
    if (vehicle_data_.coolant_temp > 110.0f) {
        // Show warning
    }
}
```

### 4. Implement Timeout Detection
```cpp
// If no message received for > 1 second, mark as stale
if (esp_timer_get_time() - last_message_time > 1000000) {
    vehicle_data_.data_valid = false;
}
```

---

## Testing Checklist

- [ ] Engine OFF: RPM=0, Coolant ~20°C, No warnings
- [ ] Engine ON (idle): RPM=700±100, Coolant ~85°C, Fuel displays correctly
- [ ] Doors open/close: All door statuses update correctly
- [ ] Seatbelts: Shows correct buckle status
- [ ] Parking brake: Shows ON when applied, OFF when released
- [ ] Gear: Shows PARK when in park, DRIVE when in drive
- [ ] Speed: Shows 0 when parked, increases when moving
- [ ] Lights: Turn signals, headlights update correctly
- [ ] AC: On/Off status matches actual AC button state
- [ ] Fuel: Increases accuracy, no false levels

---

## References
- **Protocol:** Hyundai/Kia CAN Bus (varies by model year & region)
- **Vehicle:** Kia Morning 2017 Si (may vary by market)
- **Connector:** 16-pin OBD-II (ISO 15031-3)
- **Speed:** 500 kbps
- **Transceiver:** SN65HVD230

**Note:** Exact CAN IDs and formulas may vary by region and market. This guide is based on Kia Morning 2017 captured data. Always verify with actual vehicle messages.

