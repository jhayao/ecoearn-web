# Mobile App QR Code Scanning Workflow

This document describes the complete workflow for scanning QR codes from the mobile app to activate bins for users.

## 🔄 Workflow Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    QR CODE SCAN WORKFLOW                         │
└─────────────────────────────────────────────────────────────────┘

1. User opens mobile app
2. User taps "Scan Bin QR Code"
3. Camera opens, user scans QR code on bin
4. App sends QR data + user info to server
5. Server validates QR code and checks bin status
6. If available: Bin is locked to user (status = 'active')
7. User can now use the bin for recycling
8. When done: User taps "Finish Recycling"
9. Server deactivates bin (status = 'inactive')
10. Bin becomes available for other users

```

## 📱 API Endpoints

### 1. Scan QR Code (Activate Bin)

**Endpoint**: `POST /api/bins/scan-qr`

**Purpose**: Scan QR code and activate bin for user

**Request Body**:
```json
{
  "qrData": "{\"binId\":\"abc123\",\"type\":\"bin_activation\",\"timestamp\":\"2025-11-06T10:00:00.000Z\"}",
  "userId": "user123",
  "userName": "John Doe",
  "userEmail": "john@example.com"
}
```

**Success Response (200)**:
```json
{
  "success": true,
  "message": "Bin activated successfully",
  "binStatus": "activated",
  "bin": {
    "id": "abc123",
    "name": "Mobod Main St Bin",
    "status": "active",
    "level": 45,
    "lat": 8.476876,
    "lng": 123.799913,
    "currentUser": "user123"
  }
}
```

**Already Active Response (200)**:
```json
{
  "success": true,
  "message": "Bin is already activated for you",
  "binStatus": "already_active",
  "bin": {
    "id": "abc123",
    "name": "Mobod Main St Bin",
    "status": "active",
    "level": 45,
    "lat": 8.476876,
    "lng": 123.799913
  }
}
```

**Occupied Response (409 Conflict)**:
```json
{
  "success": false,
  "error": "Bin is currently in use by another user",
  "binStatus": "occupied",
  "binName": "Mobod Main St Bin"
}
```

**Error Responses**:
- `400`: Invalid QR code or missing fields
- `404`: Bin not found
- `409`: Bin occupied by another user
- `500`: Server error

---

### 2. Deactivate Bin (Finish Recycling)

**Endpoint**: `POST /api/bins/deactivate`

**Purpose**: Deactivate bin after user finishes recycling

**Request Body**:
```json
{
  "binId": "abc123",
  "userId": "user123",
  "userEmail": "john@example.com"
}
```

**Success Response (200)**:
```json
{
  "success": true,
  "message": "Bin deactivated successfully",
  "bin": {
    "id": "abc123",
    "name": "Mobod Main St Bin",
    "status": "inactive"
  }
}
```

**Error Responses**:
- `400`: Missing required fields
- `403`: User not authorized (not the current user)
- `404`: Bin not found
- `500`: Server error

---

### 3. Check Bin Status

**Endpoint**: `POST /api/bins/check-status` or `GET /api/bins/check-status?binId=abc123`

**Purpose**: Check if bin is available before attempting to use

**Request Body (POST)**:
```json
{
  "binId": "abc123",
  "userId": "user123"
}
```

**Success Response (200)**:
```json
{
  "success": true,
  "bin": {
    "id": "abc123",
    "name": "Mobod Main St Bin",
    "status": "active",
    "level": 45,
    "lat": 8.476876,
    "lng": 123.799913,
    "currentUser": "user123",
    "availability": "active_by_you",
    "canUse": true
  }
}
```

**Availability States**:
- `available` - Bin is free to use
- `active_by_you` - You have already activated this bin
- `occupied` - Another user is using this bin

---

## 📱 Mobile App Implementation Guide

### React Native Example

```jsx
import React, { useState } from 'react';
import { View, Text, Button, Alert } from 'react-native';
import { Camera } from 'expo-camera';
// or import { RNCamera } from 'react-native-camera';

const QRScanner = ({ userId, userName, userEmail }) => {
  const [hasPermission, setHasPermission] = useState(null);
  const [scanned, setScanned] = useState(false);
  const [activeBin, setActiveBin] = useState(null);

  // Request camera permission
  const requestPermission = async () => {
    const { status } = await Camera.requestCameraPermissionsAsync();
    setHasPermission(status === 'granted');
  };

  // Handle QR code scan
  const handleBarCodeScanned = async ({ data }) => {
    if (scanned) return;
    setScanned(true);

    try {
      // Send QR data to server
      const response = await fetch('https://your-domain.com/api/bins/scan-qr', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          qrData: data,
          userId: userId,
          userName: userName,
          userEmail: userEmail
        }),
      });

      const result = await response.json();

      if (response.ok && result.success) {
        // Success - bin activated
        setActiveBin(result.bin);
        
        if (result.binStatus === 'activated') {
          Alert.alert(
            'Success!',
            `Bin "${result.bin.name}" is now activated for you. You can start recycling!`,
            [{ text: 'OK', onPress: () => navigateToRecycling(result.bin) }]
          );
        } else if (result.binStatus === 'already_active') {
          Alert.alert(
            'Already Active',
            `You already have this bin activated.`,
            [{ text: 'OK', onPress: () => navigateToRecycling(result.bin) }]
          );
        }
      } else if (response.status === 409) {
        // Bin occupied
        Alert.alert(
          'Bin Occupied',
          `Sorry, this bin is currently being used by another user. Please try another bin.`,
          [{ text: 'OK', onPress: () => setScanned(false) }]
        );
      } else {
        // Other error
        Alert.alert(
          'Error',
          result.error || 'Failed to scan QR code',
          [{ text: 'OK', onPress: () => setScanned(false) }]
        );
      }
    } catch (error) {
      console.error('Error scanning QR code:', error);
      Alert.alert(
        'Error',
        'Failed to connect to server',
        [{ text: 'OK', onPress: () => setScanned(false) }]
      );
    }
  };

  // Deactivate bin when user finishes
  const handleFinishRecycling = async () => {
    if (!activeBin) return;

    try {
      const response = await fetch('https://your-domain.com/api/bins/deactivate', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          binId: activeBin.id,
          userId: userId,
          userEmail: userEmail
        }),
      });

      const result = await response.json();

      if (response.ok && result.success) {
        Alert.alert(
          'Thank You!',
          'Recycling session completed. The bin is now available for others.',
          [{ text: 'OK', onPress: () => navigateToHome() }]
        );
        setActiveBin(null);
      } else {
        Alert.alert('Error', result.error || 'Failed to finish recycling');
      }
    } catch (error) {
      console.error('Error deactivating bin:', error);
      Alert.alert('Error', 'Failed to connect to server');
    }
  };

  if (hasPermission === null) {
    return (
      <View>
        <Button title="Allow Camera Access" onPress={requestPermission} />
      </View>
    );
  }

  if (hasPermission === false) {
    return <Text>No access to camera</Text>;
  }

  return (
    <View style={{ flex: 1 }}>
      {!activeBin ? (
        <>
          <Camera
            style={{ flex: 1 }}
            onBarCodeScanned={scanned ? undefined : handleBarCodeScanned}
          >
            <View style={{ flex: 1, justifyContent: 'center', alignItems: 'center' }}>
              <Text style={{ color: 'white', fontSize: 18 }}>
                Scan the QR code on the bin
              </Text>
            </View>
          </Camera>
          {scanned && (
            <Button title="Tap to Scan Again" onPress={() => setScanned(false)} />
          )}
        </>
      ) : (
        <View style={{ padding: 20 }}>
          <Text style={{ fontSize: 24, fontWeight: 'bold' }}>
            Bin Activated: {activeBin.name}
          </Text>
          <Text style={{ marginTop: 10 }}>
            You can now use this bin for recycling.
          </Text>
          <Text style={{ marginTop: 5, color: 'gray' }}>
            Fill Level: {activeBin.level}%
          </Text>
          <Button
            title="Finish Recycling"
            onPress={handleFinishRecycling}
            color="green"
          />
        </View>
      )}
    </View>
  );
};

export default QRScanner;
```

### Flutter Example

```dart
import 'package:flutter/material.dart';
import 'package:qr_code_scanner/qr_code_scanner.dart';
import 'package:http/http.dart' as http;
import 'dart:convert';

class QRScannerScreen extends StatefulWidget {
  final String userId;
  final String userName;
  final String userEmail;

  QRScannerScreen({
    required this.userId,
    required this.userName,
    required this.userEmail,
  });

  @override
  _QRScannerScreenState createState() => _QRScannerScreenState();
}

class _QRScannerScreenState extends State<QRScannerScreen> {
  final GlobalKey qrKey = GlobalKey(debugLabel: 'QR');
  QRViewController? controller;
  Map<String, dynamic>? activeBin;
  bool isScanning = true;

  @override
  void dispose() {
    controller?.dispose();
    super.dispose();
  }

  Future<void> scanQRCode(String qrData) async {
    setState(() {
      isScanning = false;
    });

    try {
      final response = await http.post(
        Uri.parse('https://your-domain.com/api/bins/scan-qr'),
        headers: {'Content-Type': 'application/json'},
        body: jsonEncode({
          'qrData': qrData,
          'userId': widget.userId,
          'userName': widget.userName,
          'userEmail': widget.userEmail,
        }),
      );

      final result = jsonDecode(response.body);

      if (response.statusCode == 200 && result['success']) {
        setState(() {
          activeBin = result['bin'];
        });

        showDialog(
          context: context,
          builder: (context) => AlertDialog(
            title: Text('Success!'),
            content: Text(
              'Bin "${result['bin']['name']}" is now activated for you.',
            ),
            actions: [
              TextButton(
                onPressed: () {
                  Navigator.of(context).pop();
                },
                child: Text('OK'),
              ),
            ],
          ),
        );
      } else if (response.statusCode == 409) {
        showDialog(
          context: context,
          builder: (context) => AlertDialog(
            title: Text('Bin Occupied'),
            content: Text(
              'This bin is currently being used by another user.',
            ),
            actions: [
              TextButton(
                onPressed: () {
                  Navigator.of(context).pop();
                  setState(() {
                    isScanning = true;
                  });
                },
                child: Text('Try Another Bin'),
              ),
            ],
          ),
        );
      }
    } catch (e) {
      print('Error scanning QR code: $e');
      showDialog(
        context: context,
        builder: (context) => AlertDialog(
          title: Text('Error'),
          content: Text('Failed to connect to server'),
          actions: [
            TextButton(
              onPressed: () {
                Navigator.of(context).pop();
                setState(() {
                  isScanning = true;
                });
              },
              child: Text('OK'),
            ),
          ],
        ),
      );
    }
  }

  Future<void> finishRecycling() async {
    if (activeBin == null) return;

    try {
      final response = await http.post(
        Uri.parse('https://your-domain.com/api/bins/deactivate'),
        headers: {'Content-Type': 'application/json'},
        body: jsonEncode({
          'binId': activeBin!['id'],
          'userId': widget.userId,
          'userEmail': widget.userEmail,
        }),
      );

      final result = jsonDecode(response.body);

      if (response.statusCode == 200 && result['success']) {
        showDialog(
          context: context,
          builder: (context) => AlertDialog(
            title: Text('Thank You!'),
            content: Text('Recycling session completed.'),
            actions: [
              TextButton(
                onPressed: () {
                  Navigator.of(context).pop();
                  Navigator.of(context).pop(); // Go back to home
                },
                child: Text('OK'),
              ),
            ],
          ),
        );
        setState(() {
          activeBin = null;
          isScanning = true;
        });
      }
    } catch (e) {
      print('Error finishing recycling: $e');
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Scan Bin QR Code'),
      ),
      body: activeBin == null
          ? QRView(
              key: qrKey,
              onQRViewCreated: (QRViewController controller) {
                this.controller = controller;
                controller.scannedDataStream.listen((scanData) {
                  if (isScanning) {
                    scanQRCode(scanData.code!);
                  }
                });
              },
            )
          : Padding(
              padding: EdgeInsets.all(20),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    'Bin Activated',
                    style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
                  ),
                  SizedBox(height: 10),
                  Text(
                    activeBin!['name'],
                    style: TextStyle(fontSize: 18),
                  ),
                  SizedBox(height: 10),
                  Text('Fill Level: ${activeBin!['level']}%'),
                  SizedBox(height: 20),
                  ElevatedButton(
                    onPressed: finishRecycling,
                    child: Text('Finish Recycling'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.green,
                      minimumSize: Size(double.infinity, 50),
                    ),
                  ),
                ],
              ),
            ),
    );
  }
}
```

---

## 🔒 Security & Best Practices

### 1. QR Code Format

QR codes generated by admin contain:
```json
{
  "binId": "unique_bin_id",
  "type": "bin_activation",
  "timestamp": "2025-11-06T10:00:00.000Z"
}
```

### 2. Validation Rules

- ✅ QR code must have `type: "bin_activation"`
- ✅ Bin must exist in database
- ✅ User must be authenticated
- ✅ Only one user can activate a bin at a time
- ✅ User can only deactivate their own active bins

### 3. Activity Logging

Every scan and deactivation is logged with:
- User ID and email
- Action type (Bin Scan / Bin Deactivate)
- Bin ID and name
- Timestamp

### 4. Error Handling

Mobile app should handle:
- **Network errors**: Show retry option
- **Invalid QR codes**: Show error message
- **Occupied bins**: Suggest trying another bin
- **Permission denied**: Guide user to enable camera

---

## 📊 User Flow Diagram

```
┌──────────────────────────────────────────────────────────────┐
│                      USER FLOW                                │
└──────────────────────────────────────────────────────────────┘

1. [User] Opens App
   ↓
2. [User] Taps "Scan QR Code"
   ↓
3. [App] Requests Camera Permission
   ↓
4. [User] Grants Permission
   ↓
5. [App] Opens Camera View
   ↓
6. [User] Points camera at QR code on bin
   ↓
7. [App] Detects and reads QR code
   ↓
8. [App] Sends POST /api/bins/scan-qr
   │      Body: { qrData, userId, userName, userEmail }
   ↓
9. [Server] Validates QR code
   ↓
10. [Server] Checks bin status
    ↓
    ├─ If available:
    │  ├─ [Server] Sets bin.status = 'active'
    │  ├─ [Server] Sets bin.currentUser = userId
    │  ├─ [Server] Logs activity
    │  └─ [Server] Returns success + bin details
    │     ↓
    │     [App] Shows success message
    │     [App] Navigates to recycling screen
    │     [User] Uses bin for recycling
    │
    ├─ If occupied:
    │  └─ [Server] Returns 409 error
    │     ↓
    │     [App] Shows "Bin is occupied" message
    │     [User] Tries another bin
    │
    └─ If already active by user:
       └─ [Server] Returns success (already active)
          ↓
          [App] Shows "Already active" message
          [App] Navigates to recycling screen

11. [User] Finishes recycling
    ↓
12. [User] Taps "Finish Recycling"
    ↓
13. [App] Sends POST /api/bins/deactivate
    │      Body: { binId, userId, userEmail }
    ↓
14. [Server] Verifies user is current user
    ↓
15. [Server] Sets bin.status = 'inactive'
    ├─ [Server] Removes bin.currentUser
    ├─ [Server] Logs activity
    └─ [Server] Returns success
       ↓
16. [App] Shows "Thank you" message
    ↓
17. [App] Returns to home screen
```

---

## 🧪 Testing the Workflow

### Test with cURL

**1. Scan QR Code:**
```bash
curl -X POST https://your-domain.com/api/bins/scan-qr \
  -H "Content-Type: application/json" \
  -d '{
    "qrData": "{\"binId\":\"YOUR_BIN_ID\",\"type\":\"bin_activation\",\"timestamp\":\"2025-11-06T10:00:00.000Z\"}",
    "userId": "test_user_123",
    "userName": "Test User",
    "userEmail": "test@example.com"
  }'
```

**2. Check Bin Status:**
```bash
curl -X GET "https://your-domain.com/api/bins/check-status?binId=YOUR_BIN_ID"
```

**3. Deactivate Bin:**
```bash
curl -X POST https://your-domain.com/api/bins/deactivate \
  -H "Content-Type: application/json" \
  -d '{
    "binId": "YOUR_BIN_ID",
    "userId": "test_user_123",
    "userEmail": "test@example.com"
  }'
```

---

## 📝 Database Schema Changes

The bin document in Firestore now tracks:

```typescript
{
  id: string;
  name: string;
  status: 'active' | 'inactive';  // Bin activation status
  currentUser?: string;            // User ID who activated the bin
  level: number;                   // Fill level percentage
  lat: number;                     // GPS latitude
  lng: number;                     // GPS longitude
  image: string;                   // Base64 image
  qrData: string;                  // QR code data
  qrCodePhoto: string;             // QR code image
  apiKey: string;                  // API key for IoT device
  createdAt: Timestamp;
}
```

Activity logs:
```typescript
{
  userId: string;
  email: string;
  action: 'Bin Scan' | 'Bin Deactivate';
  description: string;
  binId: string;
  binName: string;
  date: string;
  time: string;
  timestamp: Timestamp;
}
```

---

**Version**: 1.0.0  
**Last Updated**: November 6, 2025  
**Status**: Ready for Mobile App Integration ✅
