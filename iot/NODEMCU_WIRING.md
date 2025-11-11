# NodeMCU ESP8266 Wiring Guide for EcoEarn Bin Tracker

## NodeMCU Pin Layout Reference

```
                    NodeMCU v1.0 (ESP-12E)
                    
         ┌─────────────────────────┐
         │                         │
     RST │o                       o│ A0
      A0 │o                       o│ RST (Reserved)
     RSV │o                       o│ CH_PD
      SD3│o                       o│ D0  (GPIO16)
      SD2│o                       o│ D1  (GPIO5)  ← GPS TX
      SD1│o                       o│ D2  (GPIO4)  ← GPS RX
      CMD│o                       o│ D3  (GPIO0)
     SD0 │o                       o│ D4  (GPIO2)  ← Built-in LED
      CLK│o                       o│ 3V3
     GND │o                       o│ GND
     3V3 │o                       o│ D5  (GPIO14)
      EN │o                       o│ D6  (GPIO12)
     RST │o                       o│ D7  (GPIO13)
     GND │o                       o│ D8  (GPIO15)
     VIN │o                       o│ RX  (GPIO3)
         │                         │
         │       [USB Port]        │
         │                         │
         └─────────────────────────┘
```

## Wiring Connections for GY-GPS6MV2

### Simple Wiring Diagram

```
┌────────────────────┐              ┌─────────────────────┐
│  GY-GPS6MV2 GPS    │              │  NodeMCU ESP8266    │
│                    │              │                     │
│  [VCC] ───────────────────────────► [3.3V or VIN]      │
│                    │              │                     │
│  [GND] ───────────────────────────► [GND]              │
│                    │              │                     │
│  [TX]  ───────────────────────────► [D2] (GPIO4)       │
│                    │              │                     │
│  [RX]  ◄───────────────────────────[D1] (GPIO5)       │
│                    │              │                     │
│  [Antenna]         │              │  [Built-in LED D4]  │
│     ↑              │              │                     │
└────────────────────┘              └─────────────────────┘
```

### Detailed Pin Connections

| GPS Module Pin | Wire Color | NodeMCU Pin | Pin Name | Description |
|----------------|------------|-------------|----------|-------------|
| **VCC** | Red | VIN or 3.3V | Power | 3.3V-5V power supply |
| **GND** | Black | GND | Ground | Common ground |
| **TX** | Yellow/Green | D2 | GPIO4 | GPS transmit → ESP receive |
| **RX** | Blue/White | D1 | GPIO5 | GPS receive ← ESP transmit |

## Power Supply Options

### Option 1: USB Power (Recommended for Testing)
```
Computer USB ──► NodeMCU USB Port
                    │
                    ├──► NodeMCU: 5V via USB
                    └──► GPS: 3.3V from NodeMCU 3.3V pin
```

### Option 2: External 5V Power Supply
```
5V Power Supply ──► NodeMCU VIN Pin
                        │
                        ├──► NodeMCU: 5V regulated internally
                        └──► GPS: 3.3V from NodeMCU 3.3V pin
```

### Option 3: Power Bank (For Portable Deployment)
```
Power Bank (5V USB) ──► NodeMCU Micro USB
                            │
                            └──► Powers both NodeMCU and GPS
```

## Important Notes

### ⚠️ GPS Module Power

**Best Practice**: Connect GPS VCC to **3.3V** pin on NodeMCU
- GPS module typically operates at 3.3V
- NodeMCU has onboard 3.3V regulator
- More stable than using VIN

**Alternative**: You can use **VIN** (5V) if your GPS module supports it
- Some GPS modules have built-in voltage regulators
- Check your GPS module specifications

### 📌 Pin Mapping in Code

The Arduino code uses these definitions:
```cpp
const int GPS_RX_PIN = D2;  // GPIO4 - connects to GPS TX
const int GPS_TX_PIN = D1;  // GPIO5 - connects to GPS RX
const int LED_PIN = D4;     // GPIO2 - built-in LED
```

**Remember**: RX and TX are crossed!
- NodeMCU **D2 (RX)** connects to GPS **TX**
- NodeMCU **D1 (TX)** connects to GPS **RX**

### 🔌 NodeMCU Pin Labels

NodeMCU has **TWO** pin numbering systems:

1. **Silk Screen Labels** (D0, D1, D2, etc.) - Use these when connecting wires
2. **GPIO Numbers** (GPIO0, GPIO4, GPIO5, etc.) - Used in some code

Our code uses **D-labels** which Arduino IDE understands:
```cpp
D0 = GPIO16
D1 = GPIO5   ← GPS TX pin
D2 = GPIO4   ← GPS RX pin
D3 = GPIO0
D4 = GPIO2   ← Built-in LED
D5 = GPIO14
D6 = GPIO12
D7 = GPIO13
D8 = GPIO15
```

## Physical Assembly Steps

### Step 1: Prepare Components
- [ ] NodeMCU ESP8266 board
- [ ] GY-GPS6MV2 GPS module
- [ ] 4 female-to-female jumper wires (different colors recommended)
- [ ] Breadboard (optional, for cleaner connections)
- [ ] Micro USB cable

### Step 2: Wire Connections

1. **Power Connection (Red Wire)**
   ```
   GPS VCC → NodeMCU 3.3V pin
   ```

2. **Ground Connection (Black Wire)**
   ```
   GPS GND → NodeMCU GND pin
   ```

3. **GPS TX to NodeMCU RX (Yellow/Green Wire)**
   ```
   GPS TX → NodeMCU D2 (GPIO4)
   ```

4. **GPS RX to NodeMCU TX (Blue/White Wire)**
   ```
   GPS RX → NodeMCU D1 (GPIO5)
   ```

### Step 3: Antenna Position
- GPS antenna should face **upward** (toward sky)
- Keep away from metal objects
- For indoor testing, place near window

### Step 4: USB Connection
- Connect NodeMCU to computer via Micro USB cable
- LED should light up when powered

## Breadboard Layout (Optional but Recommended)

```
                 Breadboard
     ┌────────────────────────────────┐
     │                                │
     │  [3.3V] ──┐                    │
     │  [GND]  ──┼──┐                 │
     │  [D1]   ──┼──┼──┐              │
     │  [D2]   ──┼──┼──┼──┐           │
     │           │  │  │  │           │
     │     ┌─────┴──┴──┴──┴─────┐     │
     │     │   GY-GPS6MV2       │     │
     │     │ [VCC][GND][TX][RX] │     │
     │     │                    │     │
     │     └────────────────────┘     │
     │                                │
     │     [NodeMCU on other end]    │
     │                                │
     └────────────────────────────────┘
```

## Verification Checklist

Before uploading code, verify:

- [ ] **VCC Connection**: GPS VCC to NodeMCU 3.3V
- [ ] **Ground Connection**: GPS GND to NodeMCU GND
- [ ] **TX/RX Crossed**: GPS TX to NodeMCU D2, GPS RX to NodeMCU D1
- [ ] **Secure Connections**: All wires firmly connected
- [ ] **Antenna Free**: GPS antenna not blocked
- [ ] **USB Cable**: NodeMCU connected to computer

## LED Status Indicators

The built-in LED on **D4** shows status:

| LED Pattern | Meaning |
|-------------|---------|
| Slow blink (1 sec) | Waiting for GPS fix |
| 3 quick blinks | WiFi connected successfully |
| 5 rapid blinks | Location update successful |
| Solid on | Processing/sending data |

## Troubleshooting

### Problem: GPS not getting fix

**Check:**
1. ✓ GPS antenna has clear view of sky
2. ✓ TX/RX wires not swapped
3. ✓ GPS module LED is blinking (indicates it's powered)
4. ✓ Wait longer (cold start can take 2 minutes)

### Problem: GPS module not powering on

**Check:**
1. ✓ VCC connected to 3.3V pin (not GND!)
2. ✓ GND connected properly
3. ✓ NodeMCU is powered (via USB)
4. ✓ Try connecting VCC to VIN instead of 3.3V

### Problem: Garbled GPS data

**Check:**
1. ✓ TX/RX are crossed correctly (TX→D2, RX→D1)
2. ✓ Baud rate is 9600 in code
3. ✓ Wires are not loose
4. ✓ No electrical interference nearby

### Problem: NodeMCU won't upload code

**Check:**
1. ✓ Correct board selected: **NodeMCU 1.0 (ESP-12E Module)**
2. ✓ Correct COM port selected
3. ✓ USB cable is data cable (not charge-only)
4. ✓ Try different USB port
5. ✓ Install CH340 USB driver if needed

## Arduino IDE Settings for NodeMCU

When uploading code, use these settings:

```
Board: "NodeMCU 1.0 (ESP-12E Module)"
Upload Speed: "115200"
CPU Frequency: "80 MHz"
Flash Size: "4MB (FS:2MB OTA:~1019KB)"
Port: [Your COM Port]
```

## Power Consumption

| Component | Current Draw |
|-----------|--------------|
| NodeMCU (WiFi active) | 70-80 mA |
| NodeMCU (sleep mode) | 20 µA |
| GPS module (acquiring) | 40-50 mA |
| GPS module (tracking) | 30 mA |
| **Total Active** | **~110-130 mA** |

### Battery Life Estimates

| Battery Capacity | Runtime (Active) | Notes |
|------------------|------------------|-------|
| 2000 mAh | ~15 hours | Small power bank |
| 5000 mAh | ~38 hours | Medium power bank |
| 10,000 mAh | ~77 hours (~3 days) | Large power bank |
| 20,000 mAh | ~154 hours (~6 days) | Extra large |

*Note: Actual runtime may vary based on update frequency and WiFi strength*

## Recommended Setup for Production

### Weatherproof Enclosure Setup

```
┌─────────────────────────────────────┐
│  Weatherproof Box (IP65 or higher) │
│                                     │
│  ┌──────────────┐                  │
│  │  NodeMCU     │                  │
│  │  ESP8266     │                  │
│  └──────┬───────┘                  │
│         │                           │
│  ┌──────▼───────┐                  │
│  │ GPS Module   │                  │
│  │ (inside)     │                  │
│  └──────────────┘                  │
│         │                           │
│    [Antenna]────────────► Outside  │
│    (mounted on top of enclosure)   │
│                                     │
│  [Power Bank or 5V adapter]        │
│                                     │
└─────────────────────────────────────┘
```

### Best Practices

1. **GPS Antenna Placement**
   - Mount on top of enclosure
   - Clear view of sky (360°)
   - Away from metal surfaces

2. **Power Supply**
   - Use regulated 5V power supply
   - Solar panel + battery for outdoor
   - Power bank for temporary deployment

3. **WiFi Signal**
   - Test WiFi strength at installation location
   - Use WiFi repeater if needed
   - Keep antenna exposed if possible

4. **Waterproofing**
   - Use IP65+ rated enclosure
   - Seal cable entry points
   - Add silica gel packets for moisture

## Next Steps

1. ✅ Complete wiring as shown above
2. ✅ Connect NodeMCU to computer via USB
3. ✅ Upload `ecoearn_bin_tracker_test.ino` first
4. ✅ Verify WiFi and API connectivity
5. ✅ Upload `ecoearn_bin_tracker.ino` (full version)
6. ✅ Test GPS fix acquisition
7. ✅ Deploy to actual bin location

## Additional Resources

- **NodeMCU Pinout**: https://circuits4you.com/2017/12/31/nodemcu-pinout/
- **GPS Module Guide**: https://lastminuteengineers.com/neo6m-gps-arduino-tutorial/
- **ESP8266 Arduino Core**: https://arduino-esp8266.readthedocs.io/

---

**For NodeMCU ESP8266**  
**Version:** 1.0.0  
**Last Updated:** November 6, 2025
