# Servo Motor Configuration - EcoEarn Smart Bin

## ⚙️ Actual Servo Setup

### Hardware Components
Your system uses **3 servo motors** with a **rotating platform design**:

1. **MG90S #1** - Bin Lid Opener
2. **MG90S #2** - Trash Dropper
3. **MG995** - Rotating Platform

---

## 🔄 System Architecture

```
                    [User Approach]
                          ↓
                  ┌───────────────┐
                  │  Sharp IR     │ ← Detects user presence
                  │  Sensor       │
                  └───────┬───────┘
                          ↓
                  ┌───────────────┐
                  │  Bin Lid      │ ← MG90S #1 opens
                  │  (Servo 1)    │
                  └───────┬───────┘
                          ↓
                  [User drops trash]
                          ↓
                  ┌───────────────┐
                  │  Rotating     │ ← MG995 rotates to
                  │  Platform     │   selected compartment
                  │  (Servo 3)    │   (0°/90°/180°)
                  └───────┬───────┘
                          ↓
                  ┌───────────────┐
                  │  Trash        │ ← MG90S #2 releases
                  │  Dropper      │   trash
                  │  (Servo 2)    │
                  └───────┬───────┘
                          ↓
              ┌───────────┴───────────┐
              ↓           ↓           ↓
         [Plastic]    [Tin]    [Rejected]
```

---

## 🎯 Servo Details

### 1. Bin Lid Servo (MG90S #1)
**Pin:** GPIO12  
**Function:** Opens and closes the main bin lid when user is detected

**Positions:**
- `LID_CLOSED = 0°` - Lid is closed (default state)
- `LID_OPEN = 90°` - Lid is open (user can deposit trash)

**Workflow:**
1. Sharp IR sensor detects user
2. Bin lid opens (0° → 90°)
3. User deposits trash
4. Bin lid closes (90° → 0°)

**Specifications:**
- Voltage: 4.8-6V
- Torque: 1.8kg/cm @ 4.8V
- Speed: 0.1s/60° @ 4.8V
- Weight: 9g

---

### 2. Trash Dropper Servo (MG90S #2)
**Pin:** GPIO13  
**Function:** Holds trash, then releases it into selected compartment

**Positions:**
- `DROPPER_HOLD = 0°` - Holding trash (default state)
- `DROPPER_RELEASE = 90°` - Trash is dropped

**Workflow:**
1. After bin lid closes, dropper holds trash at 0°
2. Wait for platform to rotate to correct position
3. Dropper releases trash (0° → 90°)
4. Trash falls into compartment
5. Dropper returns to hold position (90° → 0°)

**Specifications:**
- Voltage: 4.8-6V
- Torque: 1.8kg/cm @ 4.8V
- Speed: 0.1s/60° @ 4.8V
- Weight: 9g

---

### 3. Rotating Platform Servo (MG995)
**Pin:** GPIO15  
**Function:** Rotates platform to position trash above correct compartment

**Positions:**
- `ROTATE_PLASTIC = 0°` - Platform positioned above plastic compartment
- `ROTATE_TIN = 90°` - Platform positioned above tin compartment
- `ROTATE_REJECTED = 180°` - Platform positioned above rejected compartment

**Workflow:**
1. ESP32-CAM identifies material type
2. Platform rotates to corresponding position:
   - Plastic detected → Rotate to 0°
   - Tin detected → Rotate to 90°
   - Rejected/Unknown → Rotate to 180°
3. Wait for platform to stabilize (0.5-1 second)
4. Dropper releases trash

**Specifications:**
- Voltage: 4.8-7.2V
- Torque: 9.4kg/cm @ 4.8V, 11kg/cm @ 6V
- Speed: 0.2s/60° @ 4.8V, 0.16s/60° @ 6V
- Weight: 55g
- **Higher torque** - Can handle rotating platform with trash weight

---

## 📊 Pin Assignment

| Servo | Model | Pin | GPIO | Function | Angle Range |
|-------|-------|-----|------|----------|-------------|
| Servo 1 | MG90S | BIN_LID_SERVO_PIN | GPIO12 | Bin Lid | 0° - 90° |
| Servo 2 | MG90S | DROPPER_SERVO_PIN | GPIO13 | Trash Dropper | 0° - 90° |
| Servo 3 | MG995 | ROTATOR_SERVO_PIN | GPIO15 | Platform Rotator | 0° - 180° |

---

## 🔌 Wiring Diagram

```
ESP32 38-Pin Board
┌─────────────────────┐
│                     │
│  GPIO12 ────────────┼──── Bin Lid Servo Signal (Orange/Yellow)
│  GPIO13 ────────────┼──── Dropper Servo Signal (Orange/Yellow)
│  GPIO15 ────────────┼──── Rotator Servo Signal (Orange/Yellow)
│                     │
│  5V     ────────────┼──── All Servo VCC (Red) via external supply
│  GND    ────────────┼──── All Servo GND (Brown/Black)
│                     │
└─────────────────────┘

External 5V Power Supply (3A minimum, 5A recommended)
│
├── Servo 1 VCC (MG90S)
├── Servo 2 VCC (MG90S)
├── Servo 3 VCC (MG995) ← Higher current draw!
│
└── Common GND ←──────────┐
                          │
                    ESP32 GND
```

**⚠️ CRITICAL:** MG995 draws more current than MG90S!
- Use **external 5V supply** (3A minimum)
- Add **100µF capacitors** near each servo
- Use **thick wires** (22 AWG or thicker) for power

---

## 📝 Code Constants

```cpp
// Pin Definitions
const int BIN_LID_SERVO_PIN = 12;   // GPIO12 - MG90S: Main lid
const int DROPPER_SERVO_PIN = 13;   // GPIO13 - MG90S: Trash dropper
const int ROTATOR_SERVO_PIN = 15;   // GPIO15 - MG995: Platform rotator

// Servo Position Constants
// Bin Lid (MG90S)
const int LID_CLOSED = 0;           // Lid closed
const int LID_OPEN = 90;            // Lid open

// Dropper (MG90S)
const int DROPPER_HOLD = 0;         // Holding trash
const int DROPPER_RELEASE = 90;     // Releasing trash

// Rotator (MG995)
const int ROTATE_PLASTIC = 0;       // Plastic compartment position
const int ROTATE_TIN = 90;          // Tin compartment position
const int ROTATE_REJECTED = 180;    // Rejected compartment position

// Servo Objects
Servo binLidServo;
Servo dropperServo;
Servo rotatorServo;

// Initialization
void setup() {
  binLidServo.attach(BIN_LID_SERVO_PIN);
  dropperServo.attach(DROPPER_SERVO_PIN);
  rotatorServo.attach(ROTATOR_SERVO_PIN);
  
  // Set to default positions
  binLidServo.write(LID_CLOSED);
  dropperServo.write(DROPPER_HOLD);
  rotatorServo.write(ROTATE_PLASTIC);
}
```

---

## 🚀 Complete Workflow Example

### Scenario: User deposits plastic bottle

```cpp
// 1. User approaches bin
if (userDetected) {
  // 2. Open bin lid
  binLidServo.write(LID_OPEN);
  Serial.println("Bin lid opened - waiting for trash");
  delay(3000);  // Wait for user to deposit trash
  
  // 3. Close bin lid
  binLidServo.write(LID_CLOSED);
  Serial.println("Bin lid closed - identifying material");
  
  // 4. ESP32-CAM identifies material
  String material = identifyMaterial();  // Returns "PLASTIC", "TIN", or "REJECTED"
  
  // 5. Rotate platform to correct compartment
  if (material == "PLASTIC") {
    rotatorServo.write(ROTATE_PLASTIC);  // 0°
    Serial.println("Platform rotated to PLASTIC compartment");
  } else if (material == "TIN") {
    rotatorServo.write(ROTATE_TIN);      // 90°
    Serial.println("Platform rotated to TIN compartment");
  } else {
    rotatorServo.write(ROTATE_REJECTED); // 180°
    Serial.println("Platform rotated to REJECTED compartment");
  }
  
  delay(1000);  // Wait for platform to stabilize
  
  // 6. Drop trash into compartment
  dropperServo.write(DROPPER_RELEASE);
  Serial.println("Trash dropped into compartment");
  delay(1000);
  
  // 7. Return to default position
  dropperServo.write(DROPPER_HOLD);
  rotatorServo.write(ROTATE_PLASTIC);  // Default position
  Serial.println("System ready for next item");
}
```

---

## 🔄 Serial Communication Protocol

### Commands from ESP32-CAM to ESP32

```
CHECK_USER       → Check if user is present
OPEN_PLASTIC     → Rotate to plastic (0°) and drop
OPEN_TIN         → Rotate to tin (90°) and drop
OPEN_REJECTED    → Rotate to rejected (180°) and drop
CLOSE_ALL        → Return all servos to default position
STATUS           → Get current servo positions
```

### Responses from ESP32 to ESP32-CAM

```
USER_DETECTED        → User is present, system active
NO_USER_DETECTED     → No user found
USER_CHECK_STARTED   → Started checking for user
PLASTIC_OPENED       → Rotated to plastic and dropped
TIN_OPENED           → Rotated to tin and dropped
REJECTED_OPENED      → Rotated to rejected and dropped
ALL_CLOSED           → All servos at default position
STANDBY_MODE         → System in standby (no user)
```

---

## ⏱️ Timing Considerations

### Servo Movement Times (approximate)

| Movement | Time | Notes |
|----------|------|-------|
| Lid open (0°→90°) | ~0.2s | MG90S at 6V |
| Lid close (90°→0°) | ~0.2s | MG90S at 6V |
| Dropper release (0°→90°) | ~0.2s | MG90S at 6V |
| Dropper hold (90°→0°) | ~0.2s | MG90S at 6V |
| Rotate 0°→90° | ~0.3s | MG995 at 6V |
| Rotate 90°→180° | ~0.3s | MG995 at 6V |
| Rotate 180°→0° | ~0.5s | MG995 at 6V (longest) |

### Recommended Delays

```cpp
// After opening lid
delay(3000);  // Wait for user to deposit trash

// After rotating platform
delay(1000);  // Wait for platform to stabilize

// After dropping trash
delay(1000);  // Ensure trash has fallen

// Total cycle time: ~5-6 seconds
```

---

## ⚡ Power Consumption

### Current Draw

| Servo | Idle | Moving | Stalled |
|-------|------|--------|---------|
| MG90S (Lid) | 10mA | 220mA | 650mA |
| MG90S (Dropper) | 10mA | 220mA | 650mA |
| MG995 (Rotator) | 20mA | 500mA | 1500mA |

### Peak Power Calculation

**Worst case (all servos moving):**
- 2× MG90S: 220mA each = 440mA
- 1× MG995: 500mA
- **Total: ~940mA @ 5V**

**With ESP32 and other components:**
- ESP32: 240mA
- ESP32-CAM: 800mA
- Sharp IR: 40mA
- 2× HC-SR04: 30mA
- GPS: 67mA
- **Grand Total: ~2.1A @ 5V**

**Recommended Power Supply:**
- **5V @ 3A minimum** (for normal operation)
- **5V @ 5A recommended** (for safety margin + stall protection)

---

## 🛡️ Safety Features

### 1. Stall Detection
If servo doesn't reach target position within timeout:
```cpp
unsigned long startTime = millis();
rotatorServo.write(ROTATE_TIN);

while (millis() - startTime < 2000) {
  // Check if rotation complete
  // Add current sensing if available
}

if (millis() - startTime >= 2000) {
  Serial.println("ERROR: Rotator servo stalled!");
  // Reset to safe position
  rotatorServo.write(ROTATE_PLASTIC);
}
```

### 2. Position Verification
Before dropping trash, confirm platform is at correct angle:
```cpp
void dropTrash(int targetPosition) {
  // Ensure platform is at correct position
  rotatorServo.write(targetPosition);
  delay(1000);  // Wait for stabilization
  
  // Now safe to drop
  dropperServo.write(DROPPER_RELEASE);
  delay(1000);
  dropperServo.write(DROPPER_HOLD);
}
```

### 3. Emergency Stop
If bin lid sensor detects obstruction:
```cpp
if (lidObstructed()) {
  binLidServo.write(LID_OPEN);  // Reopen lid
  Serial.println("WARNING: Obstruction detected!");
}
```

---

## 🔧 Calibration

### Fine-tuning Servo Angles

If servos don't align perfectly, adjust constants:

```cpp
// Example: If plastic compartment is slightly off
const int ROTATE_PLASTIC = 5;   // Was 0°, now 5°
const int ROTATE_TIN = 95;      // Was 90°, now 95°
const int ROTATE_REJECTED = 185; // Was 180°, now 185°

// Example: If lid doesn't fully close
const int LID_CLOSED = -5;      // Slightly negative
const int LID_OPEN = 95;        // Slightly past 90°
```

### Testing Calibration

Upload `component_test.ino` and use menu option 6 (Test Rotator Servo) to verify angles.

---

## 📐 Physical Constraints

### Platform Design Considerations

1. **Weight Distribution**: Center trash on platform before rotating
2. **Clearance**: Ensure platform can rotate 180° without hitting bin walls
3. **Alignment**: Mark 0°/90°/180° positions on platform for visual verification
4. **Balance**: MG995 torque is sufficient for small trash items (<500g)

### Recommended Platform Specs
- **Diameter**: 10-15cm
- **Material**: Lightweight plastic or thin plywood
- **Weight**: <100g
- **Trash capacity**: 300-500g max

---

## 🎓 Advantages of This Design

### vs. 3 Separate Lids

✅ **Fewer servos needed** (3 instead of 3+ lid servos)  
✅ **Single entry point** (simpler user experience)  
✅ **More compact design** (no multiple lid mechanisms)  
✅ **Easier to seal** (one main lid only)  
✅ **Lower power consumption** (one rotating servo vs multiple lid servos)  
✅ **More reliable** (fewer moving parts)  
✅ **Easier maintenance** (centralized mechanism)

### Tradeoffs

❌ **Slightly slower** (rotation + drop vs direct lid open)  
❌ **Single point of failure** (if rotator fails, whole system stops)  
❌ **Requires calibration** (angles must be precise)

---

## 📚 Summary

Your servo system uses a **rotating platform design**:
1. **MG90S** opens/closes main bin lid (user access)
2. **MG995** rotates platform to correct compartment (0°/90°/180°)
3. **MG90S** drops trash into selected compartment

This is a **smart, efficient design** that reduces mechanical complexity while maintaining full 3-compartment sorting capability!

---

**Last Updated:** November 2025  
**Project:** EcoEarn Smart Recycling Bin  
**Board:** ESP32 38-pin Development Board
