# Dual Microcontroller Smart Bin Setup Guide

## Overview

This guide covers the setup for the dual microcontroller smart recycling bin system with ESP32-CAM for AI material detection and ESP32 38-pin board for servo control and capacity monitoring.

## System Architecture

```
ESP32-CAM (AI Vision) ↔ Serial Communication ↔ ESP32 38-pin (Hardware Control)
     ↓                                                            ↓
AI Classification                                          Servo Motors
Web Streaming                                             Sharp IR Sensor
Material Routing                                          Ultrasonic Sensors
                                                          GPS Tracking
                                                          System Standby Mode
```

## User Presence Detection System

### How It Works
- **On-Demand Detection**: Sharp IR sensor only activates when ESP32-CAM requests user detection
- **Power Efficient**: Sensor remains idle until identification is requested
- **Active Mode**: System activates only when user is detected during requested checks
- **Standby Mode**: System enters standby after user leaves or no user is detected

### Detection Logic
1. **Request Trigger**: ESP32-CAM sends `CHECK_USER` command when "Identify Material" is clicked
2. **Sensor Activation**: ESP32 activates Sharp IR sensor for 3 seconds
3. **User Check**: If user detected within 20-50cm (optimal range), system activates and proceeds with identification
4. **No User**: If no user detected, request is rejected with error message
5. **Timeout**: System returns to idle state after check completes

### System States
- **🔋 ACTIVE**: User detected during requested check, full functionality, LED on
- **💤 STANDBY/IDLE**: No user detection request active, sensor idle, LED off

### ESP32-CAM Module
- **ESP32-CAM AI Thinker** - Camera and AI processing
- **Power Supply** - 5V USB or external supply
- **Antenna** - For WiFi connectivity

### ESP32 38-pin Development Board
- **ESP32-DEVKIT-V1** - Hardware control and monitoring
- **Sharp 2Y0A21 IR Sensor** - User presence detection (analog, 10-80cm range)
- **GY-GPS6MV2 GPS Module** - Location tracking (optional)
- **2x HC-SR04 Ultrasonic Sensors** - Capacity monitoring
- **3x Servo Motors (MG90S)** - Compartment control

### Shared Components
- **Power Supply** - Stable 5V, 3A minimum (5A recommended)
- **Jumper Wires** - For serial communication
- **Plastic Recycling Bin** - With 3 separate compartments

## Serial Communication Setup

### ESP32-CAM to NodeMCU Connection

```
ESP32-CAM          NodeMCU
==========         =======
GPIO16 (RX)  ────  TX (GPIO1)
GPIO17 (TX)  ────  RX (GPIO3)
GND         ────   GND
```

**Important Notes:**
- Use hardware serial pins on both boards
- ESP32-CAM GPIO16/17 are dedicated serial pins
- NodeMCU uses standard RX/TX pins
- Ensure common ground connection
- Baud rate: 9600

### Communication Protocol

#### Commands from ESP32-CAM to NodeMCU
- `OPEN_PLASTIC` - Open plastic compartment
- `OPEN_TIN` - Open tin/metal compartment
- `OPEN_REJECTED` - Open rejected items compartment
- `CLOSE_ALL` - Close all compartments
- `STATUS` - Request compartment status

#### Responses from NodeMCU to ESP32-CAM
- `PLASTIC_OPENED` - Plastic compartment opened
- `TIN_OPENED` - Tin compartment opened
- `REJECTED_OPENED` - Rejected compartment opened
- `ALL_CLOSED` - All compartments closed
- `STATUS:Plastic=OPEN,Tin=CLOSED,Rejected=CLOSED` - Status response

## Pin Configurations

### ESP32-CAM Pins
```
Pin   Function          Purpose
===   ========          =======
GPIO0  U0RX           Serial RX from NodeMCU
GPIO1  U0TX           Serial TX to NodeMCU
GPIO2  -              (Reserved)
GPIO3  U0RX           (Alternative serial)
GPIO4  -              (Available)
GPIO5  -              (Available)
GPIO12 -              (Available)
GPIO13 -              (Available)
GPIO14 -              (Available)
GPIO15 -              (Available)
GPIO16 Serial RX      Connected to NodeMCU TX
GPIO17 Serial TX      Connected to NodeMCU RX
```

### NodeMCU Pins (Updated for 3 Compartments)
```
Pin   GPIO   Function          Purpose
===   ====   ========          =======
D0    16     Serial TX        To ESP32-CAM RX
D1    5      GPS RX           GPS Module RX
D2    4      GPS TX           GPS Module TX
D3    0      Plastic Servo    Plastic compartment control
D4    2      Tin Servo        Tin compartment control
D5    14     Ultrasonic       (Available for expansion)
D6    12     Ultrasonic       (Available for expansion)
D7    13     Plastic Ultra    Plastic capacity sensor Trig
D8    15     Plastic Ultra    Plastic capacity sensor Echo
A0    ADC    Tin Ultra        Tin capacity sensor Echo
```

## Hardware Assembly

### Step 1: ESP32-CAM Setup
1. **Camera Module**: Ensure ESP32-CAM has camera attached
2. **Power Supply**: Connect stable 5V power
3. **WiFi Antenna**: Attach antenna for better connectivity
4. **Serial Pins**: Connect GPIO16/17 to NodeMCU

### Step 2: NodeMCU Setup
1. **GPS Module** (optional): Connect to D1/D2
2. **Ultrasonic Sensors**:
   - Plastic compartment: Trig=D7, Echo=D8
   - Tin compartment: Trig=D0, Echo=A0
3. **Servo Motors**:
   - Plastic servo: D3
   - Tin servo: D4
   - Rejected servo: D8
4. **Serial Connection**: RX=D0, TX to ESP32-CAM

### Step 3: Bin Mechanical Setup
```
+-------------------+
|                   |
|   ESP32-CAM       | ← Camera positioned to view items
|   (Top View)      |
+-------------------+
|                   |
| Plastic | Tin | Rejected | ← Three compartments
| Sensor  | Sensor | Servo   | ← Ultrasonic sensors
| Servo   | Servo  |         | ← Servo motors
+-------------------+
```

## Software Configuration

### ESP32-CAM Code Updates
```cpp
// Serial communication pins
#define NODEMCU_RX 16
#define NODEMCU_TX 17

// Initialize serial
NODEMCU_SERIAL.begin(9600, SERIAL_8N1, NODEMCU_RX, NODEMCU_TX);
```

### NodeMCU Code Updates
```cpp
// Serial communication (uses hardware Serial)
#define ESP32_SERIAL Serial
#define ESP32_BAUD 9600

// Servo pins for 3 compartments
#define PLASTIC_SERVO_PIN D3
#define TIN_SERVO_PIN D4
#define REJECTED_SERVO_PIN D8
```

## Workflow Sequence

### Material Detection Process
1. **Button Click**: User clicks "Identify Material" on ESP32-CAM web interface
2. **User Check Request**: ESP32-CAM sends `CHECK_USER` command to NodeMCU
3. **Sensor Activation**: NodeMCU activates ultrasonic sensor for user detection
4. **User Verification**: System checks if user is within 50cm range
5. **User Detected**: System activates and proceeds with material identification
6. **No User**: Request rejected with "No user detected" error message
7. **Item Placed**: User places item in front of ESP32-CAM
8. **Image Capture**: ESP32-CAM captures image via web interface
9. **AI Classification**: Sends image to backend server for identification
10. **Material Routing**: Opens appropriate compartment based on material type
11. **Auto-Close**: Compartment closes after 5 seconds
12. **Standby**: System returns to idle state after operation completes

### Capacity Monitoring
- NodeMCU continuously monitors compartment levels
- Sends capacity data to server every 1 minute (active) or 3 minutes (standby)
- Admin dashboard shows real-time fill levels

## Testing Procedure

### Hardware Testing
1. **Power Supply**: Verify stable 5V to both boards
2. **Serial Connection**: Test communication with `STATUS` command
3. **Servo Movement**: Test each compartment individually
4. **Ultrasonic Sensors**: Verify distance readings
5. **Camera**: Check video streaming at port 81

### Software Testing
1. **ESP32-CAM Web Interface**: Access `http://esp32-ip:81`
2. **Material Identification**: Click "Identify Material" button
3. **Serial Commands**: Monitor commands sent to NodeMCU
4. **Compartment Response**: Verify correct servo activation
5. **Capacity Updates**: Check server receives capacity data

### Integration Testing
1. **System Idle**: Verify ultrasonic sensor is not continuously active
2. **Button Click**: Click "Identify Material" without standing in front of sensor
3. **No User Error**: Should receive "No user detected" error message
4. **User Present**: Stand within 50cm and click "Identify Material"
5. **User Detected**: System should proceed with identification
6. **Place Plastic Item**: Should open plastic compartment
7. **Place Tin Item**: Should open tin compartment
8. **Place Other Item**: Should open rejected compartment
9. **Auto-Close**: Ensure compartments close after timeout
10. **Return to Idle**: System should return to idle state after operation

## Troubleshooting

### Serial Communication Issues
- **No Response**: Check wiring and baud rates
- **Garbled Data**: Verify voltage levels and ground connection
- **ESP32 Reset**: Check power supply stability

### Servo Control Issues
- **No Movement**: Verify power supply and signal pins
- **Erratic Movement**: Check for PWM conflicts
- **Overheating**: Ensure proper current rating

### Camera Issues
- **No Stream**: Check camera connection and power
- **Poor Quality**: Adjust frame size and quality settings
- **WiFi Disconnect**: Improve antenna placement

### Ultrasonic Sensor Issues
- **No Readings**: Check trigger/echo pin connections
- **Inconsistent**: Add delay between measurements
- **False Readings**: Calibrate distance thresholds

## Power Considerations

### Current Requirements
- **ESP32-CAM**: 200-300mA (peaks during WiFi/image processing)
- **NodeMCU**: 150-200mA (with GPS and sensors)
- **3 Servos**: 300mA peak (during movement)
- **3 Ultrasonic**: 45mA (during measurement)

### Recommended Power Supply
- **Voltage**: 5V stable DC
- **Current**: 2A minimum, 3A recommended
- **Type**: USB power bank or regulated DC supply
- **Backup**: Battery pack for portable operation

## Safety Notes

- **Electrical Safety**: Use appropriate voltage levels
- **Mechanical Safety**: Ensure servo movements don't pinch
- **User Safety**: Avoid placing cameras where privacy is concern
- **Environmental**: Protect electronics from moisture

## Next Steps

1. **Individual Testing**: Test each microcontroller separately
2. **Serial Communication**: Verify command/response system
3. **Material Classification**: Train AI model for better accuracy
4. **Mechanical Integration**: Mount components in recycling bin
5. **Field Testing**: Deploy in real environment
6. **Performance Monitoring**: Track accuracy and reliability