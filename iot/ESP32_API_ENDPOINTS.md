# ESP32 IoT Device API Endpoints

## ⚠️ IMPORTANT: QR Scan Required First

**All bin operations are LOCKED until a valid QR code is scanned by the mobile app.**

Without QR activation:
- ❌ IR sensor is disabled
- ❌ Servos are locked
- ❌ User detection disabled
- ❌ Material classification disabled

After QR activation:
- ✅ IR sensor activated
- ✅ Servos unlocked
- ✅ User detection enabled
- ✅ Full recycling functionality

---

## Base URL
```
https://your-domain.com/api
```

---

## 1. Bin Activation via QR Scan (Mobile App → Server → ESP32)

**This MUST happen first before any bin operations**

### Step 1: Mobile App Scans QR & Activates Bin
**Endpoint:** `POST /bins/activate` (Mobile App calls this)

**Request Payload:**
```json
{
  "binId": "bin_001",
  "userId": "abc123xyz789",
  "scannedAt": "2025-11-10T08:30:00Z",
  "location": {
    "latitude": 14.5995,
    "longitude": 120.9842
  }
}
```

**Success Response (200):**
```json
{
  "success": true,
  "message": "Bin activated successfully",
  "data": {
    "binId": "bin_001",
    "sessionId": "session_abc123",
    "userId": "abc123xyz789",
    "activatedAt": "2025-11-10T08:30:00Z",
    "expiresAt": "2025-11-10T09:30:00Z"
  }
}
```

### Step 2: Mobile App Sends Activation to ESP32-CAM

**Mobile app communicates with ESP32-CAM via Bluetooth/WiFi and sends:**

**Serial Command:**
```
ACTIVATE_BIN:abc123xyz789:session_abc123
```

**Format:** `ACTIVATE_BIN:{userId}:{sessionId}`

### Step 3: ESP32-CAM Forwards to ESP32 Control Board

**ESP32-CAM sends via Serial to ESP32:**
```
ACTIVATE_BIN:abc123xyz789:session_abc123
```

### Step 4: ESP32 Unlocks All Functions

**ESP32 Response:**
```
BIN_ACTIVATED
```

Now the bin is ready to operate! IR sensor, servos, and all functions are enabled.

---

## 2. User Presence Check (ESP32-CAM → Server)

**Endpoint:** `POST /iot/check-user`

**Purpose:** ESP32-CAM sends captured image to verify if a user is present before allowing bin access

**⚠️ Note:** This endpoint only works AFTER bin is activated via QR scan

**Request Headers:**
```
Content-Type: application/json
X-Device-ID: bin_001
X-Device-Secret: your_device_secret_key
```

**Request Payload:**
```json
{
  "binId": "bin_001",
  "image": "base64_encoded_image_data",
  "timestamp": "2025-11-10T08:30:00Z",
  "sensorData": {
    "irDistance": 25.5,
    "irDetected": true
  }
}
```

**Success Response (200):**
```json
{
  "success": true,
  "userPresent": true,
  "confidence": 0.95,
  "message": "User detected",
  "allowAccess": true,
  "timestamp": "2025-11-10T08:30:00Z"
}
```

**No User Response (200):**
```json
{
  "success": true,
  "userPresent": false,
  "confidence": 0.98,
  "message": "No user detected",
  "allowAccess": false,
  "timestamp": "2025-11-10T08:30:00Z"
}
```

**Error Response (400):**
```json
{
  "success": false,
  "error": "Invalid image data",
  "code": "INVALID_IMAGE"
}
```

---

## 2. Material Classification (ESP32-CAM → Server)

**Endpoint:** `POST /iot/classify`

**Purpose:** ESP32-CAM sends image of trash for AI classification (plastic/tin/rejected)

**⚠️ Note:** This endpoint only works AFTER bin is activated via QR scan

**Request Headers:**
```
Content-Type: application/json
X-Device-ID: bin_001
X-Device-Secret: your_device_secret_key
```

**Request Payload:**
```json
{
  "binId": "bin_001",
  "image": "base64_encoded_image_data",
  "timestamp": "2025-11-10T08:30:15Z",
  "sessionId": "session_abc123"
}
```

**Success Response (200):**
```json
{
  "success": true,
  "classification": {
    "material": "plastic",
    "confidence": 0.92,
    "materialId": "PET_1"
  },
  "action": "OPEN_PLASTIC",
  "message": "Plastic bottle detected",
  "timestamp": "2025-11-10T08:30:15Z"
}
```

**Rejected Material Response (200):**
```json
{
  "success": true,
  "classification": {
    "material": "rejected",
    "confidence": 0.88,
    "reason": "Not recyclable"
  },
  "action": "OPEN_REJECTED",
  "message": "Non-recyclable item detected",
  "timestamp": "2025-11-10T08:30:15Z"
}
```

**Possible Actions:**
- `OPEN_PLASTIC` - Route to plastic compartment
- `OPEN_TIN` - Route to tin compartment
- `OPEN_REJECTED` - Route to rejected compartment
- `DENY_ACCESS` - Item cannot be processed

---

## 3. Submit Recycling Transaction (ESP32 → Server)

**Endpoint:** `POST /iot/recycle`

**Purpose:** ESP32 reports successful recycling transaction after trash is deposited

**Request Headers:**
```
Content-Type: application/json
X-Device-ID: bin_001
X-Device-Secret: your_device_secret_key
```

**Request Payload:**
```json
{
  "binId": "bin_001",
  "userId": "abc123xyz789",
  "materialType": "plastic",
  "weight": 0.5,
  "quantity": 1,
  "location": {
    "latitude": 14.5995,
    "longitude": 120.9842
  },
  "timestamp": "2025-11-10T08:30:20Z",
  "sessionId": "session_abc123",
  "deviceData": {
    "compartment": "plastic",
    "fillLevel": 45,
    "temperature": 28.5,
    "humidity": 65
  }
}
```

**Success Response (201):**
```json
{
  "success": true,
  "message": "Transaction recorded successfully",
  "data": {
    "transactionId": "txn_001",
    "points": 50,
    "newTotalPoints": 1300,
    "compartmentStatus": {
      "plastic": 46,
      "tin": 35,
      "rejected": 60
    }
  },
  "timestamp": "2025-11-10T08:30:20Z"
}
```

**Error Response (400):**
```json
{
  "success": false,
  "error": "Invalid material type",
  "code": "INVALID_MATERIAL"
}
```

---

## 4. Update Bin Status (ESP32 → Server)

**Endpoint:** `POST /iot/bin/status`

**Purpose:** ESP32 sends periodic status updates (fill levels, sensor readings, health check)

**Request Headers:**
```
Content-Type: application/json
X-Device-ID: bin_001
X-Device-Secret: your_device_secret_key
```

**Request Payload:**
```json
{
  "binId": "bin_001",
  "status": "active",
  "compartments": {
    "plastic": {
      "fillLevel": 45,
      "capacity": 100,
      "ultrasonicDistance": 15.5
    },
    "tin": {
      "fillLevel": 35,
      "capacity": 100,
      "ultrasonicDistance": 22.3
    },
    "rejected": {
      "fillLevel": 60,
      "capacity": 100,
      "ultrasonicDistance": 12.8
    }
  },
  "sensors": {
    "irSensor": {
      "value": 1650,
      "status": "ok"
    },
    "gps": {
      "latitude": 14.5995,
      "longitude": 120.9842,
      "satellites": 8,
      "fix": true
    },
    "temperature": 28.5,
    "humidity": 65
  },
  "currentUser": "abc123xyz789",
  "timestamp": "2025-11-10T08:30:25Z",
  "uptime": 86400,
  "wifiSignal": -45
}
```

**Success Response (200):**
```json
{
  "success": true,
  "message": "Status updated successfully",
  "alerts": [
    {
      "type": "warning",
      "message": "Rejected compartment is 60% full",
      "action": "schedule_maintenance"
    }
  ],
  "commands": [],
  "timestamp": "2025-11-10T08:30:25Z"
}
```

**With Commands Response (200):**
```json
{
  "success": true,
  "message": "Status updated successfully",
  "alerts": [
    {
      "type": "critical",
      "message": "Plastic compartment is 90% full",
      "action": "urgent_maintenance"
    }
  ],
  "commands": [
    {
      "command": "DISABLE_BIN",
      "reason": "Maintenance required",
      "executeAt": "immediate"
    }
  ],
  "timestamp": "2025-11-10T08:30:25Z"
}
```

---

## 5. Report Error (ESP32 → Server)

**Endpoint:** `POST /iot/error`

**Purpose:** ESP32 reports system errors or malfunctions

**Request Headers:**
```
Content-Type: application/json
X-Device-ID: bin_001
X-Device-Secret: your_device_secret_key
```

**Request Payload:**
```json
{
  "binId": "bin_001",
  "errorType": "servo_malfunction",
  "errorCode": "SERVO_TIMEOUT",
  "severity": "high",
  "description": "Plastic servo failed to respond within timeout",
  "timestamp": "2025-11-10T08:30:30Z",
  "deviceState": {
    "status": "error",
    "affectedComponent": "plastic_servo",
    "lastSuccessfulOperation": "2025-11-10T08:25:00Z"
  },
  "diagnostics": {
    "servoPosition": 0,
    "targetPosition": 90,
    "voltage": 4.8,
    "current": 0.5
  }
}
```

**Success Response (200):**
```json
{
  "success": true,
  "message": "Error logged successfully",
  "errorId": "error_001",
  "actions": [
    {
      "action": "DISABLE_COMPARTMENT",
      "compartment": "plastic",
      "message": "Plastic compartment disabled until repair"
    },
    {
      "action": "NOTIFY_ADMIN",
      "priority": "high"
    }
  ],
  "timestamp": "2025-11-10T08:30:30Z"
}
```

---

## 6. Heartbeat / Health Check (ESP32 → Server)

**Endpoint:** `POST /iot/heartbeat`

**Purpose:** Periodic health check to confirm device is online

**Request Headers:**
```
Content-Type: application/json
X-Device-ID: bin_001
X-Device-Secret: your_device_secret_key
```

**Request Payload:**
```json
{
  "binId": "bin_001",
  "timestamp": "2025-11-10T08:30:35Z",
  "uptime": 86400,
  "freeMemory": 156000,
  "wifiSignal": -45,
  "firmwareVersion": "1.0.0"
}
```

**Success Response (200):**
```json
{
  "success": true,
  "message": "Heartbeat received",
  "serverTime": "2025-11-10T08:30:35Z",
  "commands": [],
  "configUpdates": null
}
```

**With Firmware Update Response (200):**
```json
{
  "success": true,
  "message": "Heartbeat received",
  "serverTime": "2025-11-10T08:30:35Z",
  "commands": [
    {
      "command": "UPDATE_FIRMWARE",
      "version": "1.1.0",
      "url": "https://storage.googleapis.com/firmware/v1.1.0.bin",
      "checksum": "abc123def456"
    }
  ],
  "configUpdates": {
    "heartbeatInterval": 60000,
    "statusUpdateInterval": 300000
  }
}
```

---

## 7. Get Active Session (ESP32 → Server)

**Endpoint:** `GET /iot/bin/{binId}/session`

**Purpose:** Check if bin has an active user session (after power loss recovery)

**Request Headers:**
```
X-Device-ID: bin_001
X-Device-Secret: your_device_secret_key
```

**Success Response - Active Session (200):**
```json
{
  "success": true,
  "hasActiveSession": true,
  "session": {
    "sessionId": "session_abc123",
    "userId": "abc123xyz789",
    "activatedAt": "2025-11-10T08:25:00Z",
    "expiresAt": "2025-11-10T09:25:00Z"
  }
}
```

**Success Response - No Session (200):**
```json
{
  "success": true,
  "hasActiveSession": false,
  "session": null
}
```

---

## Arduino ESP32 Implementation Examples

### Example 1: Check User Presence
```cpp
void checkUserPresence() {
  if (irSensorDetected()) {
    Serial.println("IR sensor triggered, checking user presence...");
    
    // Request ESP32-CAM to check user
    Serial2.println("CHECK_USER");
    
    // Wait for ESP32-CAM response (it will call server)
    // ESP32-CAM handles the HTTP request internally
    delay(2000);
    
    // Response comes via Serial from ESP32-CAM
    if (Serial2.available()) {
      String response = Serial2.readStringUntil('\n');
      if (response == "USER_DETECTED") {
        Serial.println("User confirmed by server");
        // Proceed with bin operation
      } else {
        Serial.println("No user detected");
      }
    }
  }
}
```

### Example 2: Submit Recycling Transaction
```cpp
void sendRecycleData(String materialType, String userId, float weight) {
  HTTPClient http;
  
  String serverUrl = "https://your-domain.com/api/iot/recycle";
  
  // Create JSON payload
  StaticJsonDocument<512> doc;
  doc["binId"] = binId;
  doc["userId"] = userId;
  doc["materialType"] = materialType;
  doc["weight"] = weight;
  doc["quantity"] = 1;
  
  JsonObject location = doc.createNestedObject("location");
  location["latitude"] = currentLatitude;
  location["longitude"] = currentLongitude;
  
  doc["timestamp"] = getISOTimestamp();
  doc["sessionId"] = currentSessionId;
  
  JsonObject deviceData = doc.createNestedObject("deviceData");
  deviceData["compartment"] = materialType;
  deviceData["fillLevel"] = getFillLevel(materialType);
  deviceData["temperature"] = readTemperature();
  deviceData["humidity"] = readHumidity();
  
  String payload;
  serializeJson(doc, payload);
  
  // Make HTTP request
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-ID", binId);
  http.addHeader("X-Device-Secret", DEVICE_SECRET);
  
  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode == 201) {
    String response = http.getString();
    Serial.println("Transaction submitted successfully");
    Serial.println(response);
    
    // Parse response to get points earned
    StaticJsonDocument<256> responseDoc;
    deserializeJson(responseDoc, response);
    int points = responseDoc["data"]["points"];
    Serial.print("Points earned: ");
    Serial.println(points);
  } else {
    Serial.print("Error submitting transaction: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
}
```

### Example 3: Update Bin Status
```cpp
void sendStatusUpdate() {
  HTTPClient http;
  
  String serverUrl = "https://your-domain.com/api/iot/bin/status";
  
  // Create JSON payload
  StaticJsonDocument<1024> doc;
  doc["binId"] = binId;
  doc["status"] = binActive ? "active" : "inactive";
  
  // Compartments
  JsonObject compartments = doc.createNestedObject("compartments");
  
  JsonObject plastic = compartments.createNestedObject("plastic");
  plastic["fillLevel"] = plasticFillLevel;
  plastic["capacity"] = 100;
  plastic["ultrasonicDistance"] = plasticDistance;
  
  JsonObject tin = compartments.createNestedObject("tin");
  tin["fillLevel"] = tinFillLevel;
  tin["capacity"] = 100;
  tin["ultrasonicDistance"] = tinDistance;
  
  JsonObject rejected = compartments.createNestedObject("rejected");
  rejected["fillLevel"] = rejectedFillLevel;
  rejected["capacity"] = 100;
  rejected["ultrasonicDistance"] = rejectedDistance;
  
  // Sensors
  JsonObject sensors = doc.createNestedObject("sensors");
  
  JsonObject irSensor = sensors.createNestedObject("irSensor");
  irSensor["value"] = analogRead(IR_SENSOR_PIN);
  irSensor["status"] = "ok";
  
  JsonObject gps = sensors.createNestedObject("gps");
  gps["latitude"] = currentLatitude;
  gps["longitude"] = currentLongitude;
  gps["satellites"] = gps.satellites.value();
  gps["fix"] = gps.location.isValid();
  
  sensors["temperature"] = readTemperature();
  sensors["humidity"] = readHumidity();
  
  doc["currentUser"] = currentUserId;
  doc["timestamp"] = getISOTimestamp();
  doc["uptime"] = millis() / 1000;
  doc["wifiSignal"] = WiFi.RSSI();
  
  String payload;
  serializeJson(doc, payload);
  
  // Make HTTP request
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-ID", binId);
  http.addHeader("X-Device-Secret", DEVICE_SECRET);
  
  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("Status updated successfully");
    
    // Check for server commands
    StaticJsonDocument<512> responseDoc;
    deserializeJson(responseDoc, response);
    
    JsonArray commands = responseDoc["commands"];
    if (commands.size() > 0) {
      for (JsonObject cmd : commands) {
        String command = cmd["command"];
        executeServerCommand(command);
      }
    }
  } else {
    Serial.print("Error updating status: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
}
```

### Example 4: Report Error
```cpp
void reportError(String errorType, String errorCode, String description) {
  HTTPClient http;
  
  String serverUrl = "https://your-domain.com/api/iot/error";
  
  StaticJsonDocument<512> doc;
  doc["binId"] = binId;
  doc["errorType"] = errorType;
  doc["errorCode"] = errorCode;
  doc["severity"] = "high";
  doc["description"] = description;
  doc["timestamp"] = getISOTimestamp();
  
  JsonObject deviceState = doc.createNestedObject("deviceState");
  deviceState["status"] = "error";
  deviceState["affectedComponent"] = errorType;
  deviceState["lastSuccessfulOperation"] = lastSuccessfulOperation;
  
  JsonObject diagnostics = doc.createNestedObject("diagnostics");
  diagnostics["voltage"] = readVoltage();
  diagnostics["current"] = readCurrent();
  
  String payload;
  serializeJson(doc, payload);
  
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-ID", binId);
  http.addHeader("X-Device-Secret", DEVICE_SECRET);
  
  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode == 200) {
    Serial.println("Error reported successfully");
  }
  
  http.end();
}
```

### Example 5: Heartbeat
```cpp
unsigned long lastHeartbeat = 0;
const unsigned long heartbeatInterval = 60000; // 1 minute

void loop() {
  // ... other code ...
  
  // Send heartbeat every minute
  if (millis() - lastHeartbeat >= heartbeatInterval) {
    sendHeartbeat();
    lastHeartbeat = millis();
  }
}

void sendHeartbeat() {
  HTTPClient http;
  
  String serverUrl = "https://your-domain.com/api/iot/heartbeat";
  
  StaticJsonDocument<256> doc;
  doc["binId"] = binId;
  doc["timestamp"] = getISOTimestamp();
  doc["uptime"] = millis() / 1000;
  doc["freeMemory"] = ESP.getFreeHeap();
  doc["wifiSignal"] = WiFi.RSSI();
  doc["firmwareVersion"] = FIRMWARE_VERSION;
  
  String payload;
  serializeJson(doc, payload);
  
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-ID", binId);
  http.addHeader("X-Device-Secret", DEVICE_SECRET);
  
  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    
    // Check for server commands
    StaticJsonDocument<512> responseDoc;
    deserializeJson(responseDoc, response);
    
    JsonArray commands = responseDoc["commands"];
    if (commands.size() > 0) {
      for (JsonObject cmd : commands) {
        String command = cmd["command"];
        executeServerCommand(command);
      }
    }
  }
  
  http.end();
}
```

### Helper Function: Get ISO Timestamp
```cpp
String getISOTimestamp() {
  // If you have RTC or NTP sync
  time_t now = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  
  return String(buffer);
}
```

---

## Security Notes

1. **Device Authentication:**
   - Store `X-Device-Secret` securely in ESP32 EEPROM
   - Rotate secrets periodically
   - Use HTTPS for all API calls

2. **Data Validation:**
   - Server validates all incoming data
   - Check timestamp to prevent replay attacks
   - Verify binId matches device registration

3. **Rate Limiting:**
   - Heartbeat: Max 1 per minute
   - Status updates: Max 1 per 5 minutes
   - Transactions: Max 10 per minute
   - Error reports: Max 5 per minute

---

## Testing with cURL

```bash
# Test user presence check
curl -X POST https://your-domain.com/api/iot/check-user \
  -H "Content-Type: application/json" \
  -H "X-Device-ID: bin_001" \
  -H "X-Device-Secret: your_secret" \
  -d '{
    "binId": "bin_001",
    "image": "base64_data_here",
    "timestamp": "2025-11-10T08:30:00Z",
    "sensorData": {
      "irDistance": 25.5,
      "irDetected": true
    }
  }'

# Test recycling transaction
curl -X POST https://your-domain.com/api/iot/recycle \
  -H "Content-Type: application/json" \
  -H "X-Device-ID: bin_001" \
  -H "X-Device-Secret: your_secret" \
  -d '{
    "binId": "bin_001",
    "userId": "abc123xyz789",
    "materialType": "plastic",
    "weight": 0.5,
    "quantity": 1,
    "location": {
      "latitude": 14.5995,
      "longitude": 120.9842
    },
    "timestamp": "2025-11-10T08:30:20Z",
    "sessionId": "session_abc123"
  }'
```

---

## Complete ESP32 Workflow

```
1. USER SCANS QR CODE (Mobile App) ⭐ REQUIRED FIRST STEP
   └─> Mobile app → POST /bins/activate
       └─> Server validates → Response: sessionId
           └─> Mobile sends to ESP32-CAM via Bluetooth/WiFi
               └─> ESP32-CAM → Serial: "ACTIVATE_BIN:userId:sessionId"
                   └─> ESP32 unlocks all functions ✅

2. USER APPROACHES BIN (NOW IR SENSOR IS ACTIVE)
   └─> IR Sensor detects → ESP32-CAM → POST /iot/check-user
       └─> Response: user detected → Allow access

3. USER INSERTS TRASH
   └─> ESP32-CAM captures image → POST /iot/classify
       └─> Response: "plastic" → OPEN_PLASTIC
           └─> ESP32 opens lid → rotates to plastic → drops

4. ESP32 SUBMITS TRANSACTION
   └─> POST /iot/recycle (includes userId from QR scan)
       └─> Response: points awarded to user

5. PERIODIC STATUS UPDATES
   └─> Every 5 minutes → POST /iot/bin/status
       └─> Update fill levels, sensor data

6. HEARTBEAT
   └─> Every 1 minute → POST /iot/heartbeat
       └─> Confirm device is online

7. USER FINISHES RECYCLING
   └─> Mobile app → POST /bins/deactivate
       └─> ESP32-CAM → Serial: "DEACTIVATE_BIN"
           └─> ESP32 locks all functions 🔒

❌ WITHOUT QR SCAN: IR sensor disabled, servos locked, no operations allowed
✅ WITH QR SCAN: Full functionality enabled for authenticated user
```
