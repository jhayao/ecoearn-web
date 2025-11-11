# QR-First Workflow: Scan Before Use

## 🔐 Security Model: Bin Locked Until QR Scan

Your EcoEarn smart bin now requires **QR code authentication FIRST** before any operations can occur.

---

## 🚫 Without QR Scan (Default State)

**All bin functions are LOCKED:**

```
❌ IR Sensor - Disabled (not monitoring)
❌ Servos - Locked (no movement)
❌ User Detection - Disabled
❌ Material Classification - Blocked
❌ Trash Disposal - Not allowed
🔴 LED - OFF (bin inactive)
```

**What the bin does:**
- Only listens for `ACTIVATE_BIN` command
- Ignores all other commands
- Returns `BIN_LOCKED` response if commands are attempted

---

## ✅ With QR Scan (Activated State)

**After mobile app scans QR and activates:**

```
✅ IR Sensor - Active (monitoring for users)
✅ Servos - Unlocked (ready to operate)
✅ User Detection - Enabled
✅ Material Classification - Allowed
✅ Trash Disposal - Fully functional
🟢 LED - Blinking (bin active)
```

**What the bin does:**
- Monitors IR sensor for user presence
- Processes classification commands from ESP32-CAM
- Opens appropriate compartments
- Records transactions with user ID
- Auto-locks after 5 minutes or manual deactivation

---

## 📱 Complete User Flow

### Step 1: User Scans QR Code
```
User → Opens Mobile App
     → Scans QR on Bin
     → Mobile App → POST /bins/activate
```

**Mobile App Request:**
```json
POST https://your-domain.com/api/bins/activate
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

**Server Response:**
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

---

### Step 2: Mobile App Activates Bin Hardware
```
Mobile App → Sends via Bluetooth/WiFi to ESP32-CAM
          → Serial Command: "ACTIVATE_BIN:abc123xyz789:session_abc123"
```

**ESP32-CAM forwards to ESP32 Control Board:**
```
ESP32-CAM → Serial → ESP32
Command: ACTIVATE_BIN:abc123xyz789:session_abc123
```

**ESP32 Response:**
```
BIN_ACTIVATED
```

**What happens internally:**
```cpp
// ESP32 code activates the bin
binActivatedByQR = true;
currentUserId = "abc123xyz789";
currentSessionId = "session_abc123";
binActivationTime = millis();

// LED blinks rapidly to confirm activation
// All sensors and servos are now unlocked
```

---

### Step 3: User Approaches Bin
```
User → Approaches bin (within 20-50cm)
     → Sharp IR Sensor detects presence
     → ESP32 → Serial → ESP32-CAM: "CHECK_USER"
     → ESP32-CAM → Captures image
     → ESP32-CAM → POST /iot/check-user
```

**API Request:**
```json
POST https://your-domain.com/api/iot/check-user
{
  "binId": "bin_001",
  "image": "base64_encoded_image_data",
  "timestamp": "2025-11-10T08:30:05Z",
  "sensorData": {
    "irDistance": 25.5,
    "irDetected": true
  }
}
```

**Server Response:**
```json
{
  "success": true,
  "userPresent": true,
  "confidence": 0.95,
  "message": "User detected",
  "allowAccess": true
}
```

---

### Step 4: User Inserts Trash
```
User → Inserts trash item
     → ESP32-CAM → Captures image
     → ESP32-CAM → POST /iot/classify
```

**API Request:**
```json
POST https://your-domain.com/api/iot/classify
{
  "binId": "bin_001",
  "image": "base64_encoded_image_data",
  "timestamp": "2025-11-10T08:30:10Z",
  "sessionId": "session_abc123"
}
```

**Server Response:**
```json
{
  "success": true,
  "classification": {
    "material": "plastic",
    "confidence": 0.92,
    "materialId": "PET_1"
  },
  "action": "OPEN_PLASTIC",
  "message": "Plastic bottle detected"
}
```

---

### Step 5: ESP32-CAM Commands ESP32
```
ESP32-CAM → Serial → ESP32: "OPEN_PLASTIC"
```

**ESP32 Executes Sequence:**
```cpp
// Only executes if binActivatedByQR == true
1. Open bin lid (90°)
2. Close lid (0°) - user already dropped trash
3. Rotate platform to plastic position (0°)
4. Drop trash (release dropper)
5. Return dropper to hold position
```

**ESP32 Response:**
```
PLASTIC_OPENED
```

---

### Step 6: ESP32 Records Transaction
```
ESP32 → POST /iot/recycle
```

**API Request:**
```json
POST https://your-domain.com/api/iot/recycle
{
  "binId": "bin_001",
  "userId": "abc123xyz789",  // From QR scan
  "materialType": "plastic",
  "weight": 0.5,
  "quantity": 1,
  "location": {
    "latitude": 14.5995,
    "longitude": 120.9842
  },
  "timestamp": "2025-11-10T08:30:15Z",
  "sessionId": "session_abc123",
  "deviceData": {
    "compartment": "plastic",
    "fillLevel": 46,
    "temperature": 28.5,
    "humidity": 65
  }
}
```

**Server Response:**
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
  }
}
```

**User sees points on mobile app! 🎉**

---

### Step 7: User Finishes & Deactivates
```
User → Finishes recycling
     → Clicks "Finish" in mobile app
     → Mobile App → POST /bins/deactivate
```

**Mobile App Request:**
```json
POST https://your-domain.com/api/bins/deactivate
{
  "binId": "bin_001",
  "userId": "abc123xyz789",
  "sessionId": "session_abc123"
}
```

**Server Response:**
```json
{
  "success": true,
  "message": "Bin deactivated successfully",
  "data": {
    "binId": "bin_001",
    "status": "inactive",
    "sessionDuration": 180,
    "deactivatedAt": "2025-11-10T08:33:00Z"
  }
}
```

**Mobile sends to ESP32-CAM:**
```
ESP32-CAM → Serial → ESP32: "DEACTIVATE_BIN"
```

**ESP32 Locks Everything:**
```cpp
binActivatedByQR = false;
currentUserId = "";
currentSessionId = "";
userPresent = false;
systemActive = false;

// Close all compartments
closeAllCompartments();

// LED turns off
digitalWrite(LED_PIN, LOW);
```

**ESP32 Response:**
```
BIN_DEACTIVATED
```

**Bin is now locked again - waiting for next QR scan 🔒**

---

## 🔄 Auto-Timeout Feature

**Session expires after 5 minutes of inactivity:**

```cpp
// ESP32 loop() continuously checks
if (binActivatedByQR && (millis() - binActivationTime >= 300000)) {
  Serial.println("Session timeout - deactivating bin");
  deactivateBin();
}
```

**What happens:**
- After 5 minutes, bin automatically locks
- User must scan QR again for next use
- Prevents unauthorized use if user forgets to deactivate

---

## ⚠️ Security Features

### 1. QR Required First
```cpp
// All commands check this first
if (!binActivatedByQR) {
  Serial.println("BIN LOCKED - Scan QR first!");
  Serial.println("BIN_LOCKED");
  return;  // Command ignored
}
```

### 2. User ID Tracking
```cpp
// Every transaction records the authenticated user
String currentUserId = "abc123xyz789";  // From QR scan
String currentSessionId = "session_abc123";  // From server

// Sent with every transaction
POST /iot/recycle {
  "userId": currentUserId,
  "sessionId": currentSessionId
}
```

### 3. Session Expiry
```cpp
// Maximum 5 minutes per session
const unsigned long BIN_SESSION_TIMEOUT = 300000;

// Auto-locks if exceeded
if (currentTime - binActivationTime >= BIN_SESSION_TIMEOUT) {
  deactivateBin();
}
```

### 4. Manual Deactivation
```cpp
// User can manually end session via mobile app
if (command == "DEACTIVATE_BIN") {
  deactivateBin();
}
```

---

## 🎯 Benefits of QR-First Approach

### 1. **User Authentication**
- ✅ Only authenticated users can use the bin
- ✅ Points awarded to correct user account
- ✅ Prevents unauthorized access

### 2. **Usage Tracking**
- ✅ Every transaction linked to a user ID
- ✅ Complete audit trail
- ✅ Analytics per user

### 3. **Security**
- ✅ Bin locked when not in use
- ✅ Prevents vandalism/misuse
- ✅ Session timeout for safety

### 4. **User Experience**
- ✅ Clear start/end of session
- ✅ Mobile app shows real-time status
- ✅ Points immediately credited

### 5. **Power Efficiency**
- ✅ IR sensor off when bin locked
- ✅ Servos only active during session
- ✅ Lower power consumption

---

## 🛠️ Arduino Code Changes Summary

### Global Variables Added
```cpp
bool binActivatedByQR = false;        // Bin activation state
String currentUserId = "";            // User from QR scan
String currentSessionId = "";         // Session from server
unsigned long binActivationTime = 0;  // Activation timestamp
const unsigned long BIN_SESSION_TIMEOUT = 300000;  // 5 minutes
```

### New Serial Commands
```cpp
#define CMD_ACTIVATE_BIN "ACTIVATE_BIN:"      // Format: ACTIVATE_BIN:userId:sessionId
#define CMD_DEACTIVATE_BIN "DEACTIVATE_BIN"   // Deactivate bin
```

### Main Loop Changes
```cpp
void loop() {
  // Check commands first (includes activation)
  checkESP32Commands();
  
  // Check session timeout
  if (binActivatedByQR && (millis() - binActivationTime >= BIN_SESSION_TIMEOUT)) {
    deactivateBin();
  }
  
  // ONLY proceed if bin is activated
  if (!binActivatedByQR) {
    digitalWrite(LED_PIN, LOW);
    delay(100);
    return;  // Skip all operations
  }
  
  // Normal operations only run if activated...
}
```

### New Functions
```cpp
void activateBin() {
  binActivatedByQR = true;
  binActivationTime = millis();
  closeAllCompartments();
  Serial.println("BIN_ACTIVATED");
}

void deactivateBin() {
  binActivatedByQR = false;
  currentUserId = "";
  currentSessionId = "";
  closeAllCompartments();
  Serial.println("BIN_DEACTIVATED");
}
```

---

## 📊 State Diagram

```
┌─────────────────┐
│  BIN LOCKED     │ ◄─── Default State
│  (No QR Scan)   │
│                 │
│  IR: OFF        │
│  Servos: LOCKED │
│  LED: OFF       │
└────────┬────────┘
         │
         │ QR Code Scanned
         │ Mobile → ACTIVATE_BIN
         ▼
┌─────────────────┐
│  BIN ACTIVATED  │
│  (QR Scanned)   │
│                 │
│  IR: ACTIVE     │
│  Servos: READY  │
│  LED: BLINKING  │
└────────┬────────┘
         │
         │ User Approaches
         │ IR Sensor Detects
         ▼
┌─────────────────┐
│  USER DETECTED  │
│  (Ready for use)│
│                 │
│  Waiting for    │
│  trash insert   │
└────────┬────────┘
         │
         │ Trash Inserted
         │ AI Classifies
         ▼
┌─────────────────┐
│  SORTING ACTIVE │
│  (Servos moving)│
│                 │
│  Lid → Open     │
│  Rotate → Comp  │
│  Drop → Trash   │
└────────┬────────┘
         │
         │ Transaction Complete
         │ Points Awarded
         ▼
┌─────────────────┐
│  READY FOR NEXT │
│  (Session active)│
│                 │
│  User can insert│
│  more trash     │
└────────┬────────┘
         │
         │ User Finishes
         │ Mobile → DEACTIVATE_BIN
         │ OR 5 min timeout
         ▼
┌─────────────────┐
│  BIN LOCKED     │ ◄─── Back to start
│  (Session ended)│
└─────────────────┘
```

---

## 🔍 Testing Checklist

### Test 1: Locked State
```
✓ Without QR scan, IR sensor should do nothing
✓ Send "OPEN_PLASTIC" → Should respond "BIN_LOCKED"
✓ LED should be OFF
✓ Servos should not move
```

### Test 2: Activation
```
✓ Send "ACTIVATE_BIN:testUser:testSession"
✓ Should respond "BIN_ACTIVATED"
✓ LED should start blinking
✓ IR sensor should become active
```

### Test 3: Normal Operation
```
✓ Approach bin → IR sensor triggers
✓ Send "OPEN_PLASTIC" → Servos should execute sequence
✓ Transaction should include userId in POST request
```

### Test 4: Deactivation
```
✓ Send "DEACTIVATE_BIN"
✓ Should respond "BIN_DEACTIVATED"
✓ LED should turn OFF
✓ IR sensor should become inactive
✓ Servos should lock
```

### Test 5: Timeout
```
✓ Activate bin
✓ Wait 5 minutes
✓ Bin should auto-deactivate
✓ Should print "Session timeout - deactivating bin"
```

---

## 📱 Mobile App Integration Notes

Your mobile app needs to:

1. **Scan QR Code** using camera
2. **Parse QR Data** to get binId
3. **Call API** POST /bins/activate
4. **Get sessionId** from response
5. **Send to ESP32-CAM** via Bluetooth/WiFi:
   ```
   ACTIVATE_BIN:{userId}:{sessionId}
   ```
6. **Show "Bin Active"** UI to user
7. **Monitor session** (show timer, points earned)
8. **Deactivate** when user finishes:
   ```
   POST /bins/deactivate
   Send: DEACTIVATE_BIN
   ```

---

## ✅ Conclusion

Your bin is now **secure and user-authenticated**! 

**Key Points:**
- 🔐 QR scan is REQUIRED before any operations
- 👤 Every transaction is linked to authenticated user
- ⏱️ Auto-timeout prevents unauthorized prolonged use
- 🎯 Points awarded to correct user account
- 🔋 Power-efficient when locked

**Next step:** Test the complete workflow with your hardware!
