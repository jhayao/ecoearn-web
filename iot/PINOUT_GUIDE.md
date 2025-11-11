# Smart Recycling Bin - Complete Pinout Guide

## Table of Contents
1. [ESP32-CAM Pinout](#esp32-cam-pinout)
2. [NodeMCU ESP8266 Pinout](#nodemcu-esp8266-pinout)
3. [Serial Communication Wiring](#serial-communication-wiring)
4. [Complete Wiring Diagram](#complete-wiring-diagram)
5. [Component Connections](#component-connections)

---

## ESP32-CAM Pinout

### ESP32-CAM AI Thinker Module Pin Layout

```
                    ┌─────────────────┐
                    │   ESP32-CAM     │
                    │   AI Thinker    │
                    │                 │
        5V ─────────┤ 5V         GND  ├───────── GND
       GND ─────────┤ GND        IO0  ├───────── (BOOT - Pull LOW for programming)
       IO12 ────────┤ IO12       IO1  ├───────── (Reserved - U0TXD)
       IO13 ────────┤ IO13       IO2  ├───────── (Flash LED)
       IO15 ────────┤ IO15       IO3  ├───────── (Reserved - U0RXD)
       IO14 ────────┤ IO14       IO4  ├───────── (Available)
       IO2 ─────────┤ IO2        IO5  ├───────── (Available)
       IO4 ─────────┤ IO4     IO16(RX)├───────── NodeMCU TX (Serial Communication)
       GND ─────────┤ GND     IO17(TX)├───────── NodeMCU RX (Serial Communication)
       IO5 ─────────┤ IO5               │
                    │                   │
                    └───────────────────┘
```

### ESP32-CAM Pin Assignments

| Pin | GPIO | Function | Connection | Notes |
|-----|------|----------|------------|-------|
| 5V | - | Power Input | 5V Power Supply | Stable 5V, 300mA minimum |
| GND | - | Ground | Common Ground | Shared with NodeMCU |
| GPIO16 | 16 | Serial RX | NodeMCU TX | Hardware Serial2 RX |
| GPIO17 | 17 | Serial TX | NodeMCU RX | Hardware Serial2 TX |
| GPIO0 | 0 | Boot/Flash | Pull LOW to program | Normal: HIGH/Floating |
| GPIO1 | 1 | U0TXD | Reserved | USB Serial TX |
| GPIO3 | 3 | U0RXD | Reserved | USB Serial RX |
| GPIO2 | 2 | Flash LED | Built-in | Camera flash control |
| GPIO4-15 | 4-15 | Available | - | Can be used for expansion |

### Camera Pins (Built-in Connections)
- Camera module is pre-wired on AI Thinker board
- No external connections needed for camera functionality

---

## NodeMCU ESP8266 Pinout

### NodeMCU v1.0 Development Board Pin Layout

```
                    ┌─────────────────┐
             A0 ────┤ A0          D0  ├──── GPIO16 (Tin Ultra Echo / LED)
       RSV ─────────┤ RSV         D1  ├──── GPIO5 (GPS TX)
       RSV ─────────┤ RSV         D2  ├──── GPIO4 (GPS RX)
        SD3 ────────┤ SD3         D3  ├──── GPIO0 (Plastic Servo)
        SD2 ────────┤ SD2         D4  ├──── GPIO2 (Tin Servo)
        SD1 ────────┤ SD1         D5  ├──── GPIO14 (User Detection Trig)
        CMD ────────┤ CMD         D6  ├──── GPIO12 (User Detection Echo)
        SD0 ────────┤ SD0         D7  ├──── GPIO13 (Plastic Ultra Trig)
        CLK ────────┤ CLK         D8  ├──── GPIO15 (Rejected Servo / Plastic Ultra Echo)
        GND ────────┤ GND         3V3 ├──── 3.3V Output
        3V3 ────────┤ 3V3         RST ├──── Reset
        EN ─────────┤ EN          GND ├──── Ground
        RST ────────┤ RST         VIN ├──── 5V Power Input
        GND ────────┤ GND          RX ├──── GPIO3 (ESP32-CAM TX)
        VIN ────────┤ VIN          TX ├──── GPIO1 (ESP32-CAM RX)
                    └─────────────────┘
```

### NodeMCU Pin Assignments

#### Main Functions

| Pin Label | GPIO | Function | Connection | Notes |
|-----------|------|----------|------------|-------|
| **Power & Ground** |
| VIN | - | 5V Input | 5V Power Supply | Can power from USB or external |
| 3V3 | - | 3.3V Output | Sensors (if needed) | Max 150mA output |
| GND | - | Ground | Common Ground | Multiple GND pins available |
| **Serial Communication** |
| RX | GPIO3 | Hardware RX | ESP32-CAM GPIO17 (TX) | 9600 baud |
| TX | GPIO1 | Hardware TX | ESP32-CAM GPIO16 (RX) | 9600 baud |
| **GPS Module (Production Only)** |
| D1 | GPIO5 | GPS RX | GPS Module TX | SoftwareSerial, 9600 baud |
| D2 | GPIO4 | GPS TX | GPS Module RX | SoftwareSerial, 9600 baud |
| **User Presence Detection** |
| A0 | ADC | User Detection | Sharp 2Y0A21 Analog Out | IR distance sensor (10-80cm) |
| **Plastic Compartment** |
| D3 | GPIO0 | Plastic Servo | MG90S Servo Signal | PWM control |
| D7 | GPIO13 | Plastic Trig | HC-SR04 Trig Pin | Capacity sensor trigger |
| D8 | GPIO15 | Plastic Echo | HC-SR04 Echo Pin | Capacity sensor echo |
| **Tin Compartment** |
| D4 | GPIO2 | Tin Servo | MG90S Servo Signal | PWM control |
| D0 | GPIO16 | Tin Trig | HC-SR04 Trig Pin | Capacity sensor trigger |
| D5 | GPIO14 | Tin Echo | HC-SR04 Echo Pin | Capacity sensor echo |
| **Rejected Compartment** |
| D6 | GPIO12 | Rejected Servo | MG90S Servo Signal | PWM control |

> **Note:** Using Sharp 2Y0A21 on A0 frees up D5 and D6. D6 is now used for Rejected Servo, eliminating the previous GPIO15 conflict.

---

## Serial Communication Wiring

### ESP32-CAM ↔ NodeMCU Connection

```
┌─────────────────┐                    ┌─────────────────┐
│   ESP32-CAM     │                    │  NodeMCU ESP8266│
│                 │                    │                 │
│  GPIO17 (TX) ───┼────────────────────┼───→ RX (GPIO3)  │
│                 │                    │                 │
│  GPIO16 (RX) ←──┼────────────────────┼──── TX (GPIO1)  │
│                 │                    │                 │
│      GND ───────┼────────────────────┼──── GND         │
│                 │                    │                 │
│      5V        │                    │    VIN          │
│       ↓         │                    │     ↓           │
└───────┼─────────┘                    └─────┼───────────┘
        │                                    │
        └────────────────┬───────────────────┘
                         ↓
                   5V Power Supply
                   (2A minimum)
```

### Serial Communication Specifications
- **Protocol:** UART (Hardware Serial)
- **Baud Rate:** 9600
- **Data Bits:** 8
- **Parity:** None
- **Stop Bits:** 1
- **Voltage Level:** 3.3V (both boards are 3.3V logic)

---

## Complete Wiring Diagram

### Component-by-Component Connections

#### 1. Power Supply Connections

```
5V Power Supply (2A+)
├── ESP32-CAM 5V Pin
├── NodeMCU VIN Pin
├── All Servo Motors VCC (via common rail)
└── GPS Module VCC (if used)

Common Ground
├── ESP32-CAM GND
├── NodeMCU GND (multiple pins)
├── All Servo Motors GND
├── All HC-SR04 Sensors GND
└── GPS Module GND (if used)
```

#### 2. User Presence Detection (Sharp 2Y0A21 IR Sensor)

```
NodeMCU              Sharp 2Y0A21
A0 (ADC) ────←──── Vo (Analog Output)
    5V ──────→ VCC (Red wire)
    GND ─────→ GND (Black wire)
```

**Sharp 2Y0A21 Specifications:**
- Detection Range: 10-80cm
- Output: Analog voltage (higher voltage = closer object)
- Response Time: ~40ms
- Power: 5V, ~30mA

#### 3. Plastic Compartment

**Servo Motor:**
```
NodeMCU              MG90S Servo
D3 (GPIO0) ─────→ Signal (Orange/Yellow)
    5V ──────→ VCC (Red)
    GND ─────→ GND (Brown/Black)
```

**Capacity Sensor:**
```
NodeMCU              HC-SR04 Sensor
D7 (GPIO13) ────→ Trig
D8 (GPIO15) ←──── Echo
    5V ─────→ VCC
    GND ────→ GND
```

#### 4. Tin Compartment

**Servo Motor:**
```
NodeMCU              MG90S Servo
D4 (GPIO2) ─────→ Signal (Orange/Yellow)
    5V ──────→ VCC (Red)
    GND ─────→ GND (Brown/Black)
```

**Capacity Sensor:**
```
NodeMCU              HC-SR04 Sensor
D0 (GPIO16) ────→ Trig
D5 (GPIO14) ←──── Echo
    5V ─────→ VCC
    GND ────→ GND
```

#### 5. Rejected Compartment

**Servo Motor:**
```
NodeMCU              MG90S Servo
D6 (GPIO12) ────→ Signal (Orange/Yellow)
    5V ──────→ VCC (Red)
    GND ─────→ GND (Brown/Black)
```

#### 6. GPS Module (Production Version Only)

```
NodeMCU              GY-GPS6MV2
D1 (GPIO5) ─────→ TX (GPS receives)
D2 (GPIO4) ←───── RX (GPS transmits)
    5V ──────→ VCC
    GND ─────→ GND
```

---

## Component Connections

### Sharp 2Y0A21 IR Distance Sensor (Qty: 1)

```
┌─────────────────┐
│  Sharp 2Y0A21   │
│   IR Distance   │
│                 │
│  [Red]  VCC     │ → 5V
│  [Black] GND    │ → GND
│  [Yellow] Vo    │ → A0 (Analog)
└─────────────────┘

Wiring:
- Red Wire (VCC) → NodeMCU 5V
- Black Wire (GND) → NodeMCU GND
- Yellow Wire (Vo) → NodeMCU A0

Output Voltage vs Distance:
- 10cm: ~2.8V (ADC: ~860)
- 20cm: ~1.4V (ADC: ~430)
- 40cm: ~0.7V (ADC: ~215)
- 80cm: ~0.4V (ADC: ~120)
```

**Sensor Assignment:**
- **User Presence Detection:** A0 (Analog input)

### HC-SR04 Ultrasonic Sensor (Qty: 2)

```
┌─────────────────┐
│   HC-SR04       │
│                 │
│  [Trig] [Echo]  │
│   [VCC] [GND]   │
└─────────────────┘

Connections (each sensor):
- VCC → 5V
- GND → GND
- Trig → NodeMCU GPIO (Output)
- Echo → NodeMCU GPIO (Input)
```

**Sensor Assignments:**
1. **Plastic Capacity:** D7 (Trig), D8 (Echo)
2. **Tin Capacity:** D0 (Trig), D5 (Echo)

### MG90S Servo Motor (Qty: 3)

```
┌─────────────────┐
│   MG90S Servo   │
│                 │
│  Brown  Orange  │  Wire Colors:
│  Red    (GND)   │  - Brown/Black: GND
│  (5V)  (Signal) │  - Red: 5V
└─────────────────┘  - Orange/Yellow: Signal

Connections (each servo):
- Brown/Black → GND
- Red → 5V
- Orange/Yellow → NodeMCU GPIO (PWM)
```

**Servo Assignments:**
1. **Plastic:** D3 (GPIO0)
2. **Tin:** D4 (GPIO2)
3. **Rejected:** D8 (GPIO15) - *May conflict, see notes*

### GY-GPS6MV2 GPS Module (Optional - Production Only)

```
┌─────────────────┐
│  GY-GPS6MV2     │
│                 │
│  VCC  GND       │
│  TX   RX        │
│  PPS  (others)  │
└─────────────────┘

Connections:
- VCC → 5V
- GND → GND
- TX → NodeMCU D2 (GPIO4) - GPS sends data
- RX → NodeMCU D1 (GPIO5) - GPS receives (optional)
```

---

## Pin Conflict Resolution

### Previous Pin Conflicts - NOW RESOLVED ✅

**Sharp 2Y0A21 IR Sensor Benefits:**
- Uses only A0 (analog input) instead of 2 pins (Trig + Echo)
- Frees up D5 (GPIO14) and D6 (GPIO12)
- D6 now used for Rejected Servo - **No more conflicts!**
- More reliable than ultrasonic for close-range detection
- Faster response time (~40ms vs ~100ms)

### Updated Pin Assignments Summary

**All conflicts resolved by using Sharp 2Y0A21:**
```cpp
// User Detection - Sharp 2Y0A21
const int USER_DETECTION_PIN = A0;  // Analog input

// Plastic Compartment
const int PLASTIC_SERVO_PIN = D3;   // GPIO0
const int COMP1_TRIG_PIN = D7;      // GPIO13
const int COMP1_ECHO_PIN = D8;      // GPIO15

// Tin Compartment  
const int TIN_SERVO_PIN = D4;       // GPIO2
const int COMP2_TRIG_PIN = D0;      // GPIO16
const int COMP2_ECHO_PIN = D5;      // GPIO14 (freed from user detection)

// Rejected Compartment
const int REJECTED_SERVO_PIN = D6;  // GPIO12 (freed from user detection)
```

---

## Power Requirements

### Current Consumption Table

| Component | Quantity | Current (each) | Total Current |
|-----------|----------|----------------|---------------|
| ESP32-CAM | 1 | 200-300mA | 300mA (peak) |
| NodeMCU ESP8266 | 1 | 150-200mA | 200mA |
| MG90S Servo (idle) | 3 | 10mA | 30mA |
| MG90S Servo (moving) | 3 | 100-300mA | 900mA (peak) |
| HC-SR04 Sensor | 2 | 15mA | 30mA |
| Sharp 2Y0A21 Sensor | 1 | 30mA | 30mA |
| GPS Module | 1 | 40mA | 40mA |
| **Total Idle** | - | - | **~630mA** |
| **Total Peak** | - | - | **~1530mA** |

### Power Supply Recommendations

**Minimum:** 5V @ 2A (2000mA)
**Recommended:** 5V @ 3A (3000mA)
**Ideal:** 5V @ 5A (5000mA) - For safety margin and future expansion

**Power Source Options:**
1. USB Power Bank (5V 2.4A minimum)
2. Wall Adapter (5V 3A regulated)
3. Li-Po Battery (7.4V with 5V regulator)
4. Desktop Power Supply (5V rail)

---

## Breadboard Layout Example

```
Power Rails:
[+5V]═══════════════════════════════════════
[GND]═══════════════════════════════════════

Components Row:
ESP32-CAM    NodeMCU     Servo1  Servo2  Servo3
    │           │          │       │       │
    │           │          │       │       │
Sharp2Y0A21 HC-SR04-1  HC-SR04-2  GPS   (Power)
```

---

## Testing Checklist

### Power Testing
- [ ] Measure 5V rail voltage (should be 4.8-5.2V)
- [ ] Check current draw at idle
- [ ] Test current draw with all servos moving
- [ ] Verify no voltage drops during operation

### Continuity Testing
- [ ] Verify all GND connections are common
- [ ] Check serial RX/TX crossover (TX → RX, RX → TX)
- [ ] Test each ultrasonic sensor individually
- [ ] Verify servo signal pins are correct

### Functional Testing
- [ ] ESP32-CAM boots and connects to WiFi
- [ ] NodeMCU boots and connects to WiFi
- [ ] Serial communication between boards works
- [ ] User detection ultrasonic responds
- [ ] All servos move to open/close positions
- [ ] Capacity sensors read distances correctly
- [ ] GPS gets satellite fix (if used)

---

## Troubleshooting Guide

### Common Issues

**ESP32-CAM won't boot:**
- Ensure GPIO0 is HIGH/floating (not grounded)
- Check 5V power supply is stable
- Verify camera module is properly seated

**Serial communication not working:**
- Verify TX → RX and RX → TX crossover
- Check baud rate is 9600 on both sides
- Ensure common ground connection

**Servos jittering or not moving:**
- Check power supply can deliver enough current
- Verify servo signal pins are PWM capable
- Test servo with external power supply

**Ultrasonic sensors returning invalid readings:**
- Check 5V power supply to sensors
- Verify Trig and Echo pins not swapped
- Add 10µF capacitor across VCC/GND if noisy

**Sharp 2Y0A21 not detecting or giving wrong values:**
- Verify it's connected to A0 (analog pin)
- Clean the sensor lens (dust affects IR readings)
- Check voltage output with multimeter (should vary 0.4-2.8V)
- Read ADC value in code (should be 120-860 for 10-80cm)
- Ensure no strong IR light sources nearby (sunlight, IR remotes)

---

## Additional Notes

### Best Practices
1. Use thick wires (22-24 AWG) for power connections
2. Keep servo power wires short to minimize voltage drop
3. Add 100µF capacitor across main power rails
4. Use separate power rail for servos if possible
5. Shield serial communication wires from servo wires

### Safety Warnings
- Never exceed 5.5V on ESP32-CAM or NodeMCU
- Servos can draw high current - ensure adequate power supply
- Ultrasonic sensors should not be used in wet conditions without protection
- GPS antenna needs clear sky view for best performance

### Expansion Possibilities
- Add buzzer for audio feedback
- Connect OLED display for status
- Add relay for controlling external devices
- Integrate weight sensor for material measurement

---

## Quick Reference Card

**ESP32-CAM:**
- Serial: GPIO16(RX), GPIO17(TX)
- Power: 5V, GND

**NodeMCU:**
- Serial: RX(GPIO3), TX(GPIO1)
- User Detection: A0 (Sharp 2Y0A21 Analog)
- Plastic: D3(Servo), D7(Trig), D8(Echo)
- Tin: D4(Servo), D0(Trig), D5(Echo)
- Rejected: D6(Servo)
- GPS: D1(RX), D2(TX)

**Power:**
- 5V @ 3A minimum
- Common ground for all components
