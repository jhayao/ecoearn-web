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
Authorization: Bearer {user_token}
```

**Payload:**
```json
{
  "binId": "bin_001",
  "userId": "abc123xyz789",
  "sessionId": "session_abc123"
}
```

### Response

**Success (200):**
```json
{
  "success": true,
  "message": "Bin deactivated successfully",
  "data": {
    "binId": "bin_001",
    "status": "inactive",
    "sessionDuration": 180,
    "totalPoints": 150,
    "itemsRecycled": 3,
    "deactivatedAt": "2025-11-10T08:33:00Z"
  }
}
```

**Error (403 - Unauthorized):**
```json
{
  "success": false,
  "error": "Unauthorized to deactivate this bin",
  "code": "UNAUTHORIZED_DEACTIVATION"
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
export const deactivateBin = async (binId, userId, sessionId) => {
  try {
    const response = await axios.post(`${API_BASE_URL}/bins/deactivate`, {
      binId: binId,
      userId: userId,
      sessionId: sessionId
    }, {
      headers: {
        'Authorization': `Bearer ${getAuthToken()}`,
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
    required String binId,
    required String userId,
    required String sessionId,
  }) async {
    final response = await http.post(
      Uri.parse('$baseUrl/bins/deactivate'),
      headers: _headers,
      body: jsonEncode({
        'binId': binId,
        'userId': userId,
        'sessionId': sessionId,
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
| 7 | User recycles | - | - | IR active, servos ready |
| 8 | User finishes | `POST /bins/deactivate` | - | - |
| 9 | App sends to ESP32-CAM | - | `DEACTIVATE_BIN` | - |
| 10 | ESP32 locks | - | - | 🔴 **BIN LOCKED** |

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
- ✅ Returns session summary (points, items)
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
  -H "Authorization: Bearer YOUR_TOKEN" \
  -d '{
    "binId": "bin_001",
    "userId": "test_user_123",
    "sessionId": "session_abc123"
  }'
```

---

## 📝 Notes for Backend Implementation

When implementing these endpoints, ensure:

1. **Firestore Update** on activation:
   ```javascript
   await updateDoc(doc(db, 'bins', binId), {
     status: 'active',
     currentUser: userId,
     sessionId: sessionId,
     activatedAt: serverTimestamp()
   });
   ```

2. **Firestore Update** on deactivation:
   ```javascript
   await updateDoc(doc(db, 'bins', binId), {
     status: 'inactive',
     currentUser: null,
     sessionId: null,
     deactivatedAt: serverTimestamp()
   });
   ```

3. **Session Tracking**:
   ```javascript
   await addDoc(collection(db, 'sessions'), {
     binId: binId,
     userId: userId,
     sessionId: sessionId,
     startTime: activatedAt,
     endTime: deactivatedAt,
     duration: durationSeconds,
     totalPoints: pointsEarned,
     itemsRecycled: itemCount
   });
   ```

---

## ✅ Conclusion

These **2 simple APIs** control the entire bin access system:
- `POST /bins/activate` - Connect user to bin
- `POST /bins/deactivate` - Disconnect user from bin

Combined with serial commands to ESP32, they provide complete session management for your smart recycling bin system.
