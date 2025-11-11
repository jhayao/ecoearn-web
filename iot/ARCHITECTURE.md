# System Architecture Diagram

## Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                        EcoEarn IoT System                        │
└─────────────────────────────────────────────────────────────────┘

┌──────────────────┐      ┌──────────────────┐      ┌──────────────────┐
│  Admin Panel     │      │   API Server     │      │   IoT Device     │
│  (Web Browser)   │◄────►│   (Next.js)      │◄────►│   (ESP + GPS)    │
└──────────────────┘      └──────────────────┘      └──────────────────┘
        │                          │                          │
        │                          │                          │
        ▼                          ▼                          ▼
┌──────────────────┐      ┌──────────────────┐      ┌──────────────────┐
│   Create Bin     │      │   Firestore      │      │   GPS Module     │
│   Get API Key    │      │   Database       │      │   (GY-GPS6MV2)   │
└──────────────────┘      └──────────────────┘      └──────────────────┘
```

## Data Flow

### 1. Bin Creation Flow
```
Admin → Admin Panel → Create Bin Form
                     ↓
              Generate API Key
                     ↓
              Save to Firestore
                     ↓
         Display API Key (One Time!)
                     ↓
         Admin Copies API Key
                     ↓
         Configure Arduino Code
```

### 2. Location Update Flow
```
GPS Module → Get Coordinates → Arduino Code
                               ↓
                    Format JSON Payload
                               ↓
                    Send HTTP POST Request
                               ↓
API Endpoint → Validate API Key → Firestore
                               ↓
                    Update Bin Location
                               ↓
                    Return Success Response
                               ↓
Admin Panel → Fetch Updated Data → Display on Map
```

## Component Architecture

```
┌────────────────────────────────────────────────────────────────────┐
│                          Frontend (React)                           │
├────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐            │
│  │  BinForm     │  │  BinList     │  │   BinMap     │            │
│  │              │  │              │  │              │            │
│  │ - Create Bin │  │ - Show Bins  │  │ - Show Map   │            │
│  │ - Show API   │  │ - Display    │  │ - GPS Points │            │
│  │   Key        │  │   Details    │  │              │            │
│  └──────────────┘  └──────────────┘  └──────────────┘            │
│         │                  │                  │                    │
│         └──────────────────┴──────────────────┘                    │
│                            │                                       │
└────────────────────────────┼───────────────────────────────────────┘
                             │
                             ▼
┌────────────────────────────────────────────────────────────────────┐
│                        Backend (Next.js)                            │
├────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────────────────────────────────────────────────┐      │
│  │              admin-service.ts                            │      │
│  ├─────────────────────────────────────────────────────────┤      │
│  │                                                          │      │
│  │  - generateApiKey()                                      │      │
│  │  - addBin()                                              │      │
│  │  - verifyApiKey()                                        │      │
│  │  - updateBinLocation()                                   │      │
│  │  - getBins()                                             │      │
│  │                                                          │      │
│  └─────────────────────────────────────────────────────────┘      │
│                            │                                       │
│                            ▼                                       │
│  ┌─────────────────────────────────────────────────────────┐      │
│  │        /api/iot/update-location/route.ts                │      │
│  ├─────────────────────────────────────────────────────────┤      │
│  │                                                          │      │
│  │  POST /api/iot/update-location                          │      │
│  │                                                          │      │
│  │  Request: { apiKey, latitude, longitude }               │      │
│  │  Response: { success, message, timestamp }              │      │
│  │                                                          │      │
│  │  - Validate API key                                      │      │
│  │  - Validate coordinates                                  │      │
│  │  - Update Firestore                                      │      │
│  │                                                          │      │
│  └─────────────────────────────────────────────────────────┘      │
│                            │                                       │
└────────────────────────────┼───────────────────────────────────────┘
                             │
                             ▼
┌────────────────────────────────────────────────────────────────────┐
│                      Firestore Database                             │
├────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  Collection: bins                                                   │
│  ┌────────────────────────────────────────────────────────┐        │
│  │  Document ID                                            │        │
│  ├────────────────────────────────────────────────────────┤        │
│  │  {                                                      │        │
│  │    name: "Mobod Main St",                              │        │
│  │    apiKey: "BIN_LK3M9Q_...",     ← Generated           │        │
│  │    lat: 8.476876,                 ← Updated by IoT     │        │
│  │    lng: 123.799913,               ← Updated by IoT     │        │
│  │    status: "active",                                    │        │
│  │    level: 45,                                           │        │
│  │    image: "base64...",                                  │        │
│  │    qrData: "...",                                       │        │
│  │    createdAt: Timestamp,                                │        │
│  │    currentUser: "userId"                                │        │
│  │  }                                                      │        │
│  └────────────────────────────────────────────────────────┘        │
│                                                                     │
└────────────────────────────────────────────────────────────────────┘
                             ▲
                             │
                             │ HTTP POST
                             │
┌────────────────────────────┼───────────────────────────────────────┐
│                      IoT Device (Arduino)                           │
├────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌──────────────────┐           ┌──────────────────┐              │
│  │   ESP8266/32     │◄─────────►│   GPS Module     │              │
│  │   Microcontroller│   Serial   │   GY-GPS6MV2     │              │
│  └──────────────────┘   (UART)   └──────────────────┘              │
│          │                                  │                       │
│          │                                  │                       │
│   ┌──────▼──────┐                  ┌───────▼────────┐             │
│   │ WiFi Module │                  │  NEO-6M Chip   │             │
│   │             │                  │                 │             │
│   │ - Connect   │                  │ - Get GPS Fix  │             │
│   │ - HTTP POST │                  │ - Read NMEA    │             │
│   │             │                  │ - Track Sats   │             │
│   └─────────────┘                  └────────────────┘             │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────┐      │
│  │              Arduino Sketch Logic                        │      │
│  ├─────────────────────────────────────────────────────────┤      │
│  │                                                          │      │
│  │  1. Connect to WiFi                                      │      │
│  │  2. Initialize GPS Serial                                │      │
│  │  3. Wait for GPS Fix (4+ satellites)                     │      │
│  │  4. Read Latitude/Longitude                              │      │
│  │  5. Create JSON Payload                                  │      │
│  │  6. Send HTTP POST to Server                             │      │
│  │  7. Wait for Response                                     │      │
│  │  8. Sleep/Wait for Next Update                           │      │
│  │  9. Repeat from Step 4                                   │      │
│  │                                                          │      │
│  └─────────────────────────────────────────────────────────┘      │
│                                                                     │
└────────────────────────────────────────────────────────────────────┘
```

## Hardware Connections

```
┌─────────────────────────────────────────────────┐
│            ESP8266 (NodeMCU)                    │
├─────────────────────────────────────────────────┤
│                                                 │
│   [3.3V] ─────────────────► [VCC] GPS Module   │
│   [GND]  ─────────────────► [GND]              │
│   [D2]   ─────────────────► [RX]  (GPS TX→D2)  │
│   [D1]   ─────────────────► [TX]  (GPS RX←D1)  │
│                                                 │
│   [USB] ◄────► Computer (for programming)      │
│                                                 │
│   [LED] Built-in LED for status indicators     │
│                                                 │
└─────────────────────────────────────────────────┘
```

## Network Communication

```
IoT Device (192.168.1.100)
    │
    │ WiFi 2.4GHz
    ▼
WiFi Router
    │
    │ Internet
    ▼
Firewall/NAT
    │
    │ HTTPS
    ▼
EcoEarn Server (your-domain.com)
    │
    │ API Request
    ▼
/api/iot/update-location
    │
    │ Validate & Update
    ▼
Firestore Database
    │
    │ Real-time Sync
    ▼
Admin Panel (Shows updated location)
```

## Security Layers

```
┌──────────────────────────────────────────┐
│         Security Implementation          │
├──────────────────────────────────────────┤
│                                          │
│  Layer 1: API Key Authentication         │
│  ├─ Unique per bin                       │
│  ├─ Validated on each request            │
│  └─ Stored securely in Firestore         │
│                                          │
│  Layer 2: HTTPS Encryption               │
│  ├─ TLS/SSL for data transmission        │
│  ├─ Certificate validation               │
│  └─ Encrypted payload                    │
│                                          │
│  Layer 3: Server-Side Validation         │
│  ├─ Input sanitization                   │
│  ├─ Coordinate range checking            │
│  └─ Rate limiting (future)               │
│                                          │
│  Layer 4: Firebase Security Rules        │
│  ├─ Authentication required              │
│  ├─ Read/write permissions               │
│  └─ Data validation rules                │
│                                          │
└──────────────────────────────────────────┘
```

## Timing Diagram

```
Time ─────────────────────────────────────────────────────►

Admin Panel:
  │
  ├─► Create Bin (t=0)
  │
  ├─► Generate & Display API Key (t=1s)
  │
  └─► View Updated Location (t=5min+)

IoT Device:
      │
      ├─► Boot & Connect WiFi (t=0)
      │
      ├─► Initialize GPS (t=5s)
      │
      ├─► Wait for GPS Fix (t=30s-120s)
      │
      ├─► Send Location Update (t=2min)
      │
      ├─► Wait 5 minutes (t=2min-7min)
      │
      └─► Send Next Update (t=7min)
           │
           └─► Repeat...

Server:
           │
           ├─► Receive Request (t=2min)
           │
           ├─► Validate API Key (t=2min+50ms)
           │
           ├─► Update Database (t=2min+100ms)
           │
           └─► Send Response (t=2min+200ms)
```

## State Machine (IoT Device)

```
        START
          │
          ▼
    ┌──────────┐
    │   INIT   │  Initialize hardware, WiFi, GPS
    └─────┬────┘
          │
          ▼
    ┌──────────┐
    │  CONNECT │  Connect to WiFi network
    └─────┬────┘
          │
          ▼
    ┌──────────┐
    │ GPS_WAIT │  Wait for GPS fix (4+ satellites)
    └─────┬────┘
          │
          ▼
    ┌──────────┐
    │ GPS_READ │  Read latitude/longitude
    └─────┬────┘
          │
          ▼
    ┌──────────┐
    │   SEND   │  Send HTTP POST to server
    └─────┬────┘
          │
          ▼
    ┌──────────┐
    │  SLEEP   │  Wait for UPDATE_INTERVAL
    └─────┬────┘
          │
          └──────► Back to GPS_WAIT

    Error States:
    ├─ WIFI_ERROR → Retry CONNECT
    ├─ GPS_ERROR → Retry GPS_WAIT
    └─ SEND_ERROR → Retry SEND
```

## Error Handling Flow

```
Request Sent
    │
    ├─► Success (200 OK)
    │   └─► Update complete, sleep until next cycle
    │
    ├─► Client Error (4xx)
    │   ├─► 400 Bad Request → Check payload format
    │   ├─► 401 Unauthorized → Check API key
    │   └─► 404 Not Found → Check endpoint URL
    │
    ├─► Server Error (5xx)
    │   └─► Retry with exponential backoff
    │
    └─► Network Error
        ├─► Check WiFi connection
        ├─► Reconnect if needed
        └─► Retry request
```

## Legend

```
→   Data flow
◄►  Bidirectional communication
├─  Component relationship
▼   Sequential step
│   Connection/dependency
```

---

**Version:** 1.0.0  
**Last Updated:** November 6, 2025
