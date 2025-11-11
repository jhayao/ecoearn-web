# IoT Smart Bin Hardware Setup Guide

## Overview

This guide covers the hardware setup for the EcoEarn IoT smart trash bin with ultrasonic sensors and servo motors.

## Components Required

### Core Components
- **NodeMCU ESP8266** - Main microcontroller board
- **GY-GPS6MV2 GPS Module** - For location tracking (optional for testing)
- **3x HC-SR04 Ultrasonic Sensors** - For hand detection and capacity monitoring
- **3x Servo Motors (MG90S or SG90)** - For bin and compartment control

### Power Supply
- **5V Power Supply** - For NodeMCU and servos (USB or external)
- **Power Bank/Battery** - For portable operation

### Additional Materials
- **Jumper Wires** - Male-to-male and male-to-female
- **Breadboard** - For prototyping (optional)
- **Plastic Bin/Container** - With separate compartments
- **Mounting Hardware** - Screws, brackets for sensors and servos

## NodeMCU Pin Configuration

| Component | Pin | GPIO | Purpose |
|-----------|-----|------|---------|
| **Hand Detection Ultrasonic** | | | |
| | D5 | GPIO14 | Trigger |
| | D6 | GPIO12 | Echo |
| **Compartment 1 Ultrasonic** | | | |
| | D7 | GPIO13 | Trigger |
| | D8 | GPIO15 | Echo |
| **Compartment 2 Ultrasonic** | | | |
| | D0 | GPIO16 | Trigger |
| | A0 | ADC0 | Echo |
| **Servo Motors** | | | |
| | D3 | GPIO0 | Bin Opening Servo |
| | D4 | GPIO2 | Compartment 1 Servo |
| | D8 | GPIO15 | Compartment 2 Servo |
| **GPS Module** (optional) | | | |
| | D1 | GPIO5 | GPS TX → NodeMCU RX |
| | D2 | GPIO4 | GPS RX → NodeMCU TX |
| **Status LED** | | | |
| | D0 | GPIO16 | Status Indicator |

## Wiring Diagrams

### Ultrasonic Sensor (HC-SR04) Wiring

```
HC-SR04     NodeMCU
========     =======
VCC    ──── 3.3V/5V
GND    ──── GND
Trig   ──── [Trig Pin]
Echo   ──── [Echo Pin]
```

**Important Notes:**
- HC-SR04 can use either 3.3V or 5V
- Each sensor needs separate Trig and Echo pins
- Maximum range: ~400cm
- Minimum range: ~2cm

### Servo Motor Wiring

```
Servo       NodeMCU
=====       =======
Brown  ──── GND
Red    ──── 3.3V/5V
Orange ──── [Signal Pin]
```

**Important Notes:**
- Servos require stable 5V power
- Signal pin connects to PWM-capable GPIO pins
- MG90S servo: 180° rotation, ~9g weight
- Current draw: ~100mA when moving

### GPS Module (GY-GPS6MV2) Wiring

```
GY-GPS6MV2  NodeMCU
==========  =======
VCC    ──── 3.3V/5V
GND    ──── GND
TX     ──── D2 (GPIO4)
RX     ──── D1 (GPIO5)
```

## Physical Setup Instructions

### 1. Bin Structure
```
+-------------------+
|                   |
|    Bin Opening    | ← Servo controls lid
|                   |
+---------+---------+
| Comp 1  | Comp 2  | ← Separate compartments
|         |         |
| Sensor  | Sensor  | ← Ultrasonic sensors
+---------+---------+
```

### 2. Sensor Placement

#### Hand Detection Sensor
- **Position**: Above the bin opening, facing downward
- **Height**: 15-20cm above the expected hand position
- **Angle**: Pointing straight down or slightly angled
- **Purpose**: Detects when someone approaches to use the bin

#### Capacity Sensors
- **Position**: Inside each compartment, mounted on the lid or side
- **Height**: Aligned with the top of the compartment
- **Angle**: Pointing downward into the compartment
- **Purpose**: Measures fill level by distance to waste surface

### 3. Servo Motor Installation

#### Bin Opening Servo
- **Mounting**: Attached to the bin lid hinge mechanism
- **Function**: Opens/closes the main bin lid when hand is detected

#### Compartment Servos
- **Mounting**: Inside each compartment or on sorting mechanism
- **Function**: Controls access to specific compartments (optional feature)

## Power Considerations

### Current Requirements
- **NodeMCU ESP8266**: 200-300mA (peaks during WiFi transmission)
- **HC-SR04 Ultrasonic**: 15mA per sensor (peaks during measurement)
- **MG90S Servo**: 100mA per servo (during movement)
- **GPS Module**: 45mA continuous

### Total Current Calculation
```
Base load: NodeMCU + GPS = 300mA
3 Ultrasonic sensors: 45mA
3 Servos (moving): 300mA
Peak total: ~645mA
```

### Recommended Power Supply
- **Voltage**: 5V stable
- **Current**: 1A minimum, 2A recommended
- **Type**: USB power bank or wall adapter with noise filtering

## Testing Checklist

### Hardware Testing
- [ ] All connections secure and correct
- [ ] Power supply stable (no voltage drops)
- [ ] Ultrasonic sensors not obstructed
- [ ] Servo motors move freely (no binding)
- [ ] GPS antenna has clear sky view (if used)

### Software Testing
- [ ] NodeMCU connects to WiFi
- [ ] Ultrasonic sensors return valid readings
- [ ] Servo motors respond to commands
- [ ] Hand detection triggers bin opening
- [ ] Capacity readings update correctly
- [ ] API calls succeed (check server logs)

### Functional Testing
- [ ] Hand detection opens bin within 15cm
- [ ] Bin auto-closes after 5 seconds
- [ ] Capacity measurements accurate
- [ ] LED indicators work correctly
- [ ] GPS coordinates update (if equipped)

## Troubleshooting

### Common Issues

#### Ultrasonic Sensors
- **No readings**: Check VCC/GND connections, try different pins
- **Inconsistent readings**: Add delay between measurements, check for interference
- **False triggers**: Adjust detection thresholds, check sensor placement

#### Servo Motors
- **No movement**: Verify power supply, check signal pin connections
- **Jittery movement**: Ensure stable power, add decoupling capacitors
- **Overheating**: Check current draw, ensure proper ventilation

#### NodeMCU Issues
- **WiFi connection fails**: Check credentials, try different channels
- **Random resets**: Check power supply stability, reduce servo current draw
- **Pin conflicts**: Verify pin assignments don't conflict with boot pins

### Debug Tips
1. Test each component individually before full integration
2. Use serial monitor to check sensor readings and debug messages
3. Verify API endpoints with test requests before hardware testing
4. Monitor power consumption during operation

## Safety Notes

- **Electrical Safety**: Use appropriate voltage levels, avoid short circuits
- **Mechanical Safety**: Ensure servo movements don't pinch or trap
- **Environmental**: Protect electronics from moisture and extreme temperatures
- **User Safety**: Avoid placing sensors where they could cause injury

## Next Steps

1. **Upload Test Code**: Start with the test version (no GPS required)
2. **Verify Basic Functionality**: Test ultrasonic sensors and servos
3. **Add GPS Module**: Integrate location tracking
4. **Field Testing**: Deploy in real environment
5. **Monitor Performance**: Track capacity data and system reliability