# Bin Activation & Deactivation APIs

## Overview

These are the **only 2 APIs** needed to control bin access and connect/disconnect users from the smart bin.

---

## 🟢 API to Activate Bin (QR Scan)

**Endpoint:** `POST /bins/activate`

**Called by:** Mobile App (after scanning QR code)

**Purpose:** Unlocks the bin and associates it with a specific user session

### Request

```http
POST https://your-domain.com/api/bins/activate
Content-Type: application/json
Authorization: Bearer {user_token}
```

**Payload:**
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

### Response

**Success (200):**
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

**Error (409 - Bin Already Active):**
```json
{
  "success": false,
  "error": "Bin is currently in use by another user",
  "code": "BIN_ALREADY_ACTIVE",
  "data": {
    "currentUser": "xyz456abc789",
    "estimatedAvailableAt": "2025-11-10T08:45:00Z"
  }
}
```

**Error (404 - Bin Not Found):**
```json
{
  "success": false,
  "error": "Bin not found",
  "code": "BIN_NOT_FOUND"
}
```

### Hardware Communication

After successful API call, mobile app sends to ESP32-CAM via Bluetooth/WiFi:

**Serial Command:**
```
ACTIVATE_BIN:abc123xyz789:session_abc123
```

**Format:** `ACTIVATE_BIN:{userId}:{sessionId}`

**ESP32 Response:**
```
BIN_ACTIVATED
```

### Result

- ✅ Bin unlocks
- ✅ IR sensor activates
- ✅ Servos ready
- ✅ LED starts blinking
- ✅ User can now use the bin

---

## 🔴 API to Deactivate Bin (Disconnect User)

**Endpoint:** `POST /bins/deactivate`

**Called by:** Mobile App (when user finishes recycling)

**Purpose:** Locks the bin and ends the user session

### Request

```http
POST https://your-domain.com/api/bins/deactivate
Content-Type: application/json
```

**Payload:**
```json
{
  "userId": "abc123xyz789",
  "sessionData": {
    "plasticCount": 3,
    "tinCount": 2,
    "rejectedCount": 1,
    "sessionId": "session_abc123"
  }
}
```

**Headers:**
```
X-API-Key: BIN_LK3M9Q_H7G8J9K2L4M5N6P8Q
```

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| userId | string | Yes | The user ID who activated the bin |
| sessionData | object | No | Session recycling data from ESP32 |
| sessionData.plasticCount | number | No | Number of plastic bottles detected |
| sessionData.tinCount | number | No | Number of tin cans detected |
| sessionData.rejectedCount | number | No | Number of rejected items |
| sessionData.sessionId | string | No | Session identifier |

**Headers:**

| Header | Required | Description |
|--------|----------|-------------|
| X-API-Key | Yes | The unique API key for the bin |

### Response

**Success (200):**
```json
{
  "success": true,
  "message": "Bin deactivated successfully",
  "data": {
    "pointsAwarded": 5,
    "sessionProcessed": true
  },
  "bin": {
    "id": "uTnM9cFUDMfRVF8GbaiN",
    "name": "Test Bin",
    "status": "inactive"
  }
}
```

**Error (400 - Missing Fields):**
```json
{
  "success": false,
  "error": "Missing required fields: apiKey, userId"
}
```

**Error (403 - Unauthorized):**
```json
{
  "success": false,
  "error": "You are not authorized to deactivate this bin",
  "currentUser": "different_user_id"
}
```

**Error (404 - Bin Not Found):**
```json
{
  "success": false,
  "error": "Bin not found"
}
```

### Hardware Communication

After successful API call, mobile app sends to ESP32-CAM via Bluetooth/WiFi:

**Serial Command:**
```
DEACTIVATE_BIN
```

**ESP32 Response:**
```
BIN_DEACTIVATED
```

### Result

- 🔒 Bin locks
- ❌ IR sensor disables
- ❌ Servos lock
- ❌ LED turns off
- ❌ No operations allowed until next QR scan

---

## 🔄 Auto-Deactivation (Timeout)

The bin automatically deactivates after **5 minutes** of inactivity to prevent unauthorized prolonged use.

### ESP32 Implementation

```cpp
// Check every loop
if (binActivatedByQR && (millis() - binActivationTime >= 300000)) {
  Serial.println("Session timeout - deactivating bin");
  deactivateBin();
  
  // Optionally notify server
  sendSessionTimeoutToServer();
}
```

### Optional Server Notification

**Endpoint:** `POST /iot/session-timeout`

**Request:**
```json
{
  "binId": "bin_001",
  "sessionId": "session_abc123",
  "userId": "abc123xyz789",
  "reason": "timeout",
  "timestamp": "2025-11-10T08:35:00Z"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Session timeout recorded"
}
```

---

## 📱 Mobile App Implementation Examples

### React Native (Axios)

```javascript
import axios from 'axios';

const API_BASE_URL = 'https://your-domain.com/api';

// ACTIVATE BIN
export const activateBin = async (binId, userId, location) => {
  try {
    const response = await axios.post(`${API_BASE_URL}/bins/activate`, {
      binId: binId,
      userId: userId,
      scannedAt: new Date().toISOString(),
      location: location
    }, {
      headers: {
        'Authorization': `Bearer ${getAuthToken()}`,
        'Content-Type': 'application/json'
      }
    });
    
    if (response.data.success) {
      const { sessionId } = response.data.data;
      
      // Send to ESP32-CAM via Bluetooth
      const command = `ACTIVATE_BIN:${userId}:${sessionId}`;
      await sendToESP32CAM(command);
      
      return response.data;
    }
  } catch (error) {
    if (error.response?.status === 409) {
      throw new Error('Bin is already in use');
    }
    throw error;
  }
};

// DEACTIVATE BIN
export const deactivateBin = async (apiKey, userId, userEmail) => {
  try {
    const response = await axios.post(`${API_BASE_URL}/bins/deactivate`, {
      apiKey: apiKey,
      userId: userId,
      userEmail: userEmail
    }, {
      headers: {
        'Content-Type': 'application/json'
      }
    });
    
    if (response.data.success) {
      // Send to ESP32-CAM via Bluetooth
      await sendToESP32CAM('DEACTIVATE_BIN');
      
      return response.data;
    }
  } catch (error) {
    throw error;
  }
};

// Helper function to send commands to ESP32-CAM via Bluetooth
async function sendToESP32CAM(command) {
  // Use react-native-ble-plx or similar
  // await BluetoothSerial.write(command + '\n');
  console.log('Sending to ESP32-CAM:', command);
}
```

### Flutter (http package)

```dart
import 'dart:convert';
import 'package:http/http.dart' as http;

class BinService {
  static const String baseUrl = 'https://your-domain.com/api';
  String? _authToken;

  void setAuthToken(String token) {
    _authToken = token;
  }

  Map<String, String> get _headers => {
    'Content-Type': 'application/json',
    if (_authToken != null) 'Authorization': 'Bearer $_authToken',
  };

  // ACTIVATE BIN
  Future<Map<String, dynamic>> activateBin({
    required String binId,
    required String userId,
    required double latitude,
    required double longitude,
  }) async {
    final response = await http.post(
      Uri.parse('$baseUrl/bins/activate'),
      headers: _headers,
      body: jsonEncode({
        'binId': binId,
        'userId': userId,
        'scannedAt': DateTime.now().toIso8601String(),
        'location': {
          'latitude': latitude,
          'longitude': longitude,
        },
      }),
    );

    if (response.statusCode == 200) {
      final data = jsonDecode(response.body);
      
      if (data['success']) {
        final sessionId = data['data']['sessionId'];
        
        // Send to ESP32-CAM via Bluetooth
        final command = 'ACTIVATE_BIN:$userId:$sessionId';
        await sendToESP32CAM(command);
        
        return data;
      }
    } else if (response.statusCode == 409) {
      throw Exception('Bin is already in use');
    }
    
    throw Exception('Failed to activate bin');
  }

  // DEACTIVATE BIN
  Future<Map<String, dynamic>> deactivateBin({
    required String apiKey,
    required String userId,
  }) async {
    final response = await http.post(
      Uri.parse('$baseUrl/bins/deactivate'),
      headers: {
        'Content-Type': 'application/json',
        'X-API-Key': apiKey,
      },
      body: jsonEncode({
        'userId': userId,
      }),
    );

    if (response.statusCode == 200) {
      final data = jsonDecode(response.body);
      
      if (data['success']) {
        // Send to ESP32-CAM via Bluetooth
        await sendToESP32CAM('DEACTIVATE_BIN');
        
        return data;
      }
    }
    
    throw Exception('Failed to deactivate bin');
  }

  // Helper function to send commands to ESP32-CAM via Bluetooth
  Future<void> sendToESP32CAM(String command) async {
    // Use flutter_blue_plus or similar
    // await characteristic.write(utf8.encode(command + '\n'));
    print('Sending to ESP32-CAM: $command');
  }
}
```

---

## 🔐 Security Considerations

### 1. Authentication Required
```javascript
// All requests must include user authentication token
headers: {
  'Authorization': `Bearer ${userToken}`
}
```

### 2. Session Validation
- Server validates user owns the session
- Only the user who activated can deactivate
- Server checks session hasn't expired

### 3. Bin State Validation
```json
// Server checks bin availability
if (bin.status === 'active' && bin.currentUser !== userId) {
  return { error: 'Bin already in use' };
}
```

### 4. Timeout Protection
```cpp
// ESP32 enforces 5-minute maximum session
const unsigned long BIN_SESSION_TIMEOUT = 300000;  // milliseconds
```

---

## 📊 Complete Flow Summary

| Step | Mobile App | API Call | Serial Command | ESP32 Result |
|------|-----------|----------|----------------|--------------|
| 1 | User scans QR | `POST /bins/activate` | - | - |
| 2 | Server validates | - | - | - |
| 3 | Server responds | Response: `sessionId` | - | - |
| 4 | App sends to ESP32-CAM | - | `ACTIVATE_BIN:userId:sessionId` | - |
| 5 | ESP32-CAM forwards | - | → ESP32 Control Board | - |
| 6 | ESP32 unlocks | - | - | 🟢 **BIN ACTIVE** |
| 7 | User inserts item | - | ESP32 counts items | IR active, servos ready |
| 8 | ESP32 processes item | - | Increments counters | Items sorted |
| 9 | User finishes | `POST /bins/deactivate` | - | - |
| 10 | ESP32 sends session data | Session data included | - | - |
| 11 | Server calculates points | Points awarded | - | - |
| 12 | ESP32-CAM | - | `DEACTIVATE_BIN` | - |
| 13 | ESP32 locks | - | - | 🔴 **BIN LOCKED** |

---

## 🎯 Key Points

### Activation
- ✅ Creates authenticated session
- ✅ Links user to bin
- ✅ Unlocks all hardware functions
- ✅ Returns sessionId for tracking

### Deactivation
- ✅ Ends user session
- ✅ Locks all hardware functions
- ✅ Calculates and awards points based on session data
- ✅ Frees bin for next user

### Auto-Timeout
- ✅ Prevents indefinite sessions
- ✅ Automatic after 5 minutes
- ✅ Security feature
- ✅ Optional server notification

---

## 🧪 Testing

### Test Activation
```bash
curl -X POST https://your-domain.com/api/bins/activate \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -d '{
    "binId": "bin_001",
    "userId": "test_user_123",
    "scannedAt": "2025-11-10T08:30:00Z",
    "location": {
      "latitude": 14.5995,
      "longitude": 120.9842
    }
  }'
```

### Test Deactivation
```bash
curl -X POST https://your-domain.com/api/bins/deactivate \
  -H "Content-Type: application/json" \
  -d '{
    "apiKey": "BIN_LK3M9Q_H7G8J9K2L4M5N6P8Q",
    "userId": "test_user_123",
    "userEmail": "test@example.com"
  }'
```

### Test Recycling Transaction (ESP32 → Server)
```bash
curl -X POST https://your-domain.com/api/iot/recycle \
  -H "Content-Type: application/json" \
  -H "X-Device-ID: bin_001" \
  -H "X-Device-Secret: your_secret" \
  -d '{
    "binId": "bin_001",
    "userId": "test_user_123",
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

## 📝 Notes for Backend Implementation

When implementing the deactivate endpoint, ensure:

1. **API Key Validation**:
   ```javascript
   const bin = await adminService.getBinByApiKey(apiKey);
   if (!bin) {
     return { error: 'Bin not found' };
   }
   ```

2. **User Authorization Check**:
   ```javascript
   if (bin.currentUser !== userId) {
     return { error: 'You are not authorized to deactivate this bin' };
   }
   ```

3. **Points Calculation** (if sessionData provided):
   ```javascript
   // 50 plastic bottles = 1 point
   const plasticPoints = Math.floor(sessionData.plasticCount / 50);
   
   // 10 tin cans = 1 point  
   const tinPoints = Math.floor(sessionData.tinCount / 10);
   
   const totalPoints = plasticPoints + tinPoints;
   ```

**Points Calculation:** Points are calculated and awarded during deactivation based on session data sent by the ESP32.

**Session Data Processing** (`/bins/deactivate` endpoint):
```javascript
// Process session data from ESP32
if (sessionData) {
  const { plasticCount, tinCount, rejectedCount } = sessionData;
  
  // Get current pricing configuration
  const pricing = await adminService.getCurrentPricing();
  
  // Calculate points
  const plasticPoints = Math.floor(plasticCount / pricing.plastic); // items per point
  const tinPoints = Math.floor(tinCount / pricing.glass); // items per point
  const totalPoints = plasticPoints + tinPoints;
  
  // Award points to user
  if (totalPoints > 0) {
    await adminService.addUserPoints(userId, totalPoints);
  }
  
  // Log recycling activities
  // ... log each material type separately
}
```

4. **Firestore Update** on deactivation:
   ```javascript
   await updateDoc(doc(db, 'bins', bin.id), {
     status: 'inactive',
     currentUser: null,
     sessionId: null,
     deactivatedAt: serverTimestamp()
   });
   ```

5. **Activity Logging**:
   ```javascript
   await addDoc(collection(db, 'userActivities'), {
     userId: userId,
     email: userEmail,
     action: 'Bin Deactivate',
     description: `Deactivated bin: ${bin.name}`,
     binId: bin.id,
     binName: bin.name,
     timestamp: serverTimestamp()
   });
   ```

---

## ✅ Conclusion

These **2 APIs** control the entire bin access system:
- `POST /bins/activate` - Connect user to bin (returns sessionId)
- `POST /bins/deactivate` - Disconnect user from bin (processes session data and awards points)

**Key Differences:**
- **Activate**: Uses `binId`, returns `sessionId` for ESP32 communication
- **Deactivate**: Uses `X-API-Key` header and `userId`, processes session data from ESP32
- **Session-Based**: ESP32 counts materials during session, sends totals on deactivate

**Correct Architecture:**
- Mobile app handles user authentication and bin access control
- ESP32 microcontroller handles item detection, classification, and session counting
- Points are awarded when user disconnects, based on accumulated session data

Combined with serial commands to ESP32, they provide complete session management for your smart recycling bin system.
