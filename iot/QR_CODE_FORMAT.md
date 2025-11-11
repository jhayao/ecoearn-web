# QR Code Format - EcoEarn Smart Bin System

## 📱 QR Code Data Structure

### Generated QR Code Format

When a bin is created in the admin panel, a QR code is automatically generated with the following JSON structure:

```json
{
  "binId": "generated-bin-id",
  "type": "bin_activation",
  "timestamp": "2025-11-10T12:34:56.789Z"
}
```

### Field Descriptions

| Field | Type | Description | Example |
|-------|------|-------------|---------|
| `binId` | string | Unique identifier for the bin (auto-generated UUID) | `"f7b9c4e2-8a1d-4f6e-9b2a-3c5d8e1a9f7b"` |
| `type` | string | Always `"bin_activation"` to identify QR purpose | `"bin_activation"` |
| `timestamp` | string | ISO 8601 timestamp of when QR was generated | `"2025-11-10T08:30:45.123Z"` |

---

## 🎯 Mobile App Integration

### Scanning Flow

```
User opens mobile app
      ↓
Scan QR code on bin
      ↓
App reads JSON data
      ↓
Extract binId
      ↓
Send to backend:
  POST /api/bins/activate
  {
    "binId": "f7b9c4e2-...",
    "userId": "user-123"
  }
      ↓
Backend validates & activates bin
      ↓
Success: User can now use bin
```

### Mobile App Code Example

#### React Native / Expo

```typescript
import { Camera } from 'expo-camera';
import { BarCodeScanner } from 'expo-barcode-scanner';

const BinQRScanner = () => {
  const [hasPermission, setHasPermission] = useState(null);
  const [scanned, setScanned] = useState(false);

  useEffect(() => {
    (async () => {
      const { status } = await Camera.requestCameraPermissionsAsync();
      setHasPermission(status === 'granted');
    })();
  }, []);

  const handleBarCodeScanned = async ({ type, data }) => {
    setScanned(true);
    
    try {
      // Parse QR code data
      const qrData = JSON.parse(data);
      
      // Validate QR code type
      if (qrData.type !== 'bin_activation') {
        alert('Invalid QR code. Please scan an EcoEarn bin QR code.');
        setScanned(false);
        return;
      }
      
      // Extract bin ID
      const binId = qrData.binId;
      
      // Activate bin via API
      const response = await fetch('https://your-api.com/api/bins/activate', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${userToken}`,
        },
        body: JSON.stringify({
          binId: binId,
          userId: currentUserId,
        }),
      });
      
      const result = await response.json();
      
      if (response.ok) {
        alert(`Bin ${binId} activated successfully!`);
        // Navigate to bin usage screen
        navigation.navigate('BinUsage', { binId });
      } else {
        alert(`Activation failed: ${result.message}`);
        setScanned(false);
      }
      
    } catch (error) {
      console.error('QR scan error:', error);
      alert('Invalid QR code format');
      setScanned(false);
    }
  };

  if (hasPermission === null) {
    return <Text>Requesting camera permission...</Text>;
  }
  
  if (hasPermission === false) {
    return <Text>No access to camera</Text>;
  }

  return (
    <View style={styles.container}>
      <BarCodeScanner
        onBarCodeScanned={scanned ? undefined : handleBarCodeScanned}
        style={StyleSheet.absoluteFillObject}
      />
      {scanned && (
        <Button title="Tap to Scan Again" onPress={() => setScanned(false)} />
      )}
    </View>
  );
};
```

#### Flutter

```dart
import 'package:qr_code_scanner/qr_code_scanner.dart';
import 'dart:convert';
import 'package:http/http.dart' as http;

class BinQRScanner extends StatefulWidget {
  @override
  _BinQRScannerState createState() => _BinQRScannerState();
}

class _BinQRScannerState extends State<BinQRScanner> {
  final GlobalKey qrKey = GlobalKey(debugLabel: 'QR');
  QRViewController? controller;
  bool scanned = false;

  void _onQRViewCreated(QRViewController controller) {
    this.controller = controller;
    controller.scannedDataStream.listen((scanData) async {
      if (!scanned) {
        scanned = true;
        await _handleQRCode(scanData.code ?? '');
      }
    });
  }

  Future<void> _handleQRCode(String qrCode) async {
    try {
      // Parse QR code JSON
      final qrData = jsonDecode(qrCode);
      
      // Validate QR type
      if (qrData['type'] != 'bin_activation') {
        _showError('Invalid QR code. Please scan an EcoEarn bin.');
        scanned = false;
        return;
      }
      
      // Extract bin ID
      final binId = qrData['binId'];
      
      // Call activation API
      final response = await http.post(
        Uri.parse('https://your-api.com/api/bins/activate'),
        headers: {
          'Content-Type': 'application/json',
          'Authorization': 'Bearer $userToken',
        },
        body: jsonEncode({
          'binId': binId,
          'userId': currentUserId,
        }),
      );
      
      if (response.statusCode == 200) {
        final result = jsonDecode(response.body);
        _showSuccess('Bin activated successfully!');
        // Navigate to bin usage
        Navigator.pushNamed(context, '/bin-usage', arguments: binId);
      } else {
        _showError('Activation failed');
        scanned = false;
      }
      
    } catch (e) {
      print('QR scan error: $e');
      _showError('Invalid QR code format');
      scanned = false;
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: QRView(
        key: qrKey,
        onQRViewCreated: _onQRViewCreated,
      ),
    );
  }

  @override
  void dispose() {
    controller?.dispose();
    super.dispose();
  }
}
```

---

## 🔍 QR Code Validation

### Client-Side Validation (Mobile App)

```typescript
function validateQRCode(qrData: string): { valid: boolean; binId?: string; error?: string } {
  try {
    // Parse JSON
    const data = JSON.parse(qrData);
    
    // Check required fields
    if (!data.binId || !data.type || !data.timestamp) {
      return { valid: false, error: 'Missing required fields' };
    }
    
    // Validate type
    if (data.type !== 'bin_activation') {
      return { valid: false, error: 'Invalid QR code type' };
    }
    
    // Validate binId format (UUID)
    const uuidRegex = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i;
    if (!uuidRegex.test(data.binId)) {
      return { valid: false, error: 'Invalid bin ID format' };
    }
    
    // Optional: Check if QR is not too old (e.g., generated within last year)
    const qrDate = new Date(data.timestamp);
    const now = new Date();
    const daysSinceGeneration = (now.getTime() - qrDate.getTime()) / (1000 * 60 * 60 * 24);
    
    if (daysSinceGeneration > 365) {
      console.warn('QR code is very old, may be outdated');
      // Don't reject, just warn
    }
    
    return { valid: true, binId: data.binId };
    
  } catch (error) {
    return { valid: false, error: 'Invalid JSON format' };
  }
}

// Usage
const result = validateQRCode(scannedData);
if (result.valid) {
  activateBin(result.binId);
} else {
  alert(result.error);
}
```

### Server-Side Validation (Backend API)

```typescript
// API endpoint: POST /api/bins/activate
app.post('/api/bins/activate', async (req, res) => {
  const { binId, userId } = req.body;
  
  // Validate request
  if (!binId || !userId) {
    return res.status(400).json({ error: 'binId and userId required' });
  }
  
  try {
    // Check if bin exists
    const bin = await db.collection('bins').doc(binId).get();
    
    if (!bin.exists) {
      return res.status(404).json({ error: 'Bin not found' });
    }
    
    const binData = bin.data();
    
    // Check if bin is already activated
    if (binData.status === 'active') {
      return res.status(400).json({ error: 'Bin already activated' });
    }
    
    // Check if bin is available
    if (binData.status !== 'available') {
      return res.status(400).json({ error: 'Bin not available for activation' });
    }
    
    // Activate bin
    await db.collection('bins').doc(binId).update({
      status: 'active',
      activatedBy: userId,
      activatedAt: new Date().toISOString(),
      lastUsed: new Date().toISOString(),
    });
    
    // Log activation
    await db.collection('bin_activations').add({
      binId: binId,
      userId: userId,
      timestamp: new Date().toISOString(),
      method: 'qr_scan',
    });
    
    res.json({
      success: true,
      binId: binId,
      message: 'Bin activated successfully',
    });
    
  } catch (error) {
    console.error('Activation error:', error);
    res.status(500).json({ error: 'Server error' });
  }
});
```

---

## 📊 QR Code Examples

### Example 1: Standard Bin QR Code

```json
{
  "binId": "f7b9c4e2-8a1d-4f6e-9b2a-3c5d8e1a9f7b",
  "type": "bin_activation",
  "timestamp": "2025-11-10T08:30:45.123Z"
}
```

**What mobile app should do:**
1. Parse JSON
2. Extract `binId`: `"f7b9c4e2-8a1d-4f6e-9b2a-3c5d8e1a9f7b"`
3. Send POST request to activate bin
4. Navigate user to bin usage screen

### Example 2: Test QR Code (from test page)

```json
{
  "binId": "test-bin-001",
  "type": "bin_activation",
  "timestamp": "2025-11-10T14:22:10.456Z"
}
```

**What mobile app should do:**
1. Same as Example 1
2. Recognize `test-bin-001` as test bin
3. May show "TEST MODE" indicator in app

---

## 🎨 QR Code Appearance

### Visual Specifications

```javascript
QRCode.toDataURL(qrData, {
  width: 200,        // 200x200 pixels
  margin: 2,         // 2-unit quiet zone
  color: {
    dark: '#000000', // Black modules
    light: '#FFFFFF' // White background
  }
})
```

### Printed QR Code Recommendations

- **Minimum size:** 3cm × 3cm (for scanning from 10-20cm distance)
- **Recommended size:** 5cm × 5cm (for easier scanning)
- **Material:** Waterproof sticker or laminated paper
- **Placement:** On flat surface of bin, eye-level height
- **Protection:** Indoor use or weatherproof coating for outdoor bins

---

## 🔐 Security Considerations

### Current Security

✅ **Type validation:** Ensures QR is for bin activation  
✅ **UUID format:** Makes binId unpredictable  
✅ **Timestamp:** Allows tracking when QR was generated  
✅ **Server validation:** Backend verifies bin exists and is available  

### Potential Enhancements

1. **Add cryptographic signature:**
```json
{
  "binId": "f7b9c4e2-...",
  "type": "bin_activation",
  "timestamp": "2025-11-10T08:30:45.123Z",
  "signature": "hmac-sha256-signature"
}
```

2. **Single-use activation codes:**
```json
{
  "binId": "f7b9c4e2-...",
  "type": "bin_activation",
  "timestamp": "2025-11-10T08:30:45.123Z",
  "activationCode": "ABC123XYZ"
}
```
Backend invalidates code after first use.

3. **Expiration time:**
```json
{
  "binId": "f7b9c4e2-...",
  "type": "bin_activation",
  "timestamp": "2025-11-10T08:30:45.123Z",
  "expiresAt": "2026-11-10T08:30:45.123Z"
}
```

---

## 📱 Mobile App API Requirements

### 1. Bin Activation Endpoint

```typescript
// POST /api/bins/activate
{
  "binId": "f7b9c4e2-8a1d-4f6e-9b2a-3c5d8e1a9f7b",
  "userId": "user-123"
}

// Response 200 OK
{
  "success": true,
  "binId": "f7b9c4e2-8a1d-4f6e-9b2a-3c5d8e1a9f7b",
  "message": "Bin activated successfully",
  "bin": {
    "id": "f7b9c4e2-...",
    "name": "Bin #1",
    "location": "Main Campus",
    "status": "active",
    "activatedAt": "2025-11-10T14:30:00Z"
  }
}

// Response 404 Not Found
{
  "error": "Bin not found"
}

// Response 400 Bad Request
{
  "error": "Bin already activated by another user"
}
```

### 2. Get Bin Info Endpoint (Optional)

```typescript
// GET /api/bins/:binId
// Response 200 OK
{
  "id": "f7b9c4e2-...",
  "name": "Bin #1",
  "location": "Main Campus",
  "status": "active",
  "capacity": {
    "plastic": 45.2,
    "tin": 62.8
  },
  "location": {
    "latitude": 14.6760,
    "longitude": 121.0437
  },
  "lastUsed": "2025-11-10T13:45:00Z"
}
```

---

## 🧪 Testing QR Codes

### Web Test Page

Located at: `/test-qr`

**Features:**
- Generate test QR codes with custom bin IDs
- Download QR code images
- Test scanner component
- Verify activation flow

**Usage:**
1. Open `http://localhost:3000/test-qr`
2. Enter bin ID (e.g., `test-bin-001`)
3. Click "Generate QR Code"
4. Download or display QR code
5. Use mobile app to scan and test

### Command Line QR Generation

```bash
# Install qrencode
sudo apt-get install qrencode  # Linux
brew install qrencode           # macOS

# Generate QR code
echo '{"binId":"test-bin-001","type":"bin_activation","timestamp":"2025-11-10T12:00:00Z"}' | qrencode -o test-qr.png

# Generate larger QR code
echo '{"binId":"test-bin-001","type":"bin_activation","timestamp":"2025-11-10T12:00:00Z"}' | qrencode -s 10 -o test-qr-large.png
```

---

## 📋 Summary for Mobile Developers

### What You Need to Know

1. **QR Code Contains:**
   - `binId` (string, UUID format)
   - `type` (always `"bin_activation"`)
   - `timestamp` (ISO 8601 string)

2. **What to Do After Scanning:**
   ```
   Parse JSON → Extract binId → POST to /api/bins/activate
   ```

3. **Error Handling:**
   - Invalid JSON → Show "Invalid QR code"
   - Wrong type → Show "Not an EcoEarn bin QR code"
   - Bin not found → Show "Bin not registered"
   - Already activated → Show "Bin in use by another user"

4. **Success Flow:**
   - Show success message
   - Save binId to user session
   - Navigate to bin control screen
   - Allow user to deposit trash and identify materials

5. **Libraries Needed:**
   - **React Native:** `expo-camera`, `expo-barcode-scanner`
   - **Flutter:** `qr_code_scanner`
   - **Native Android:** `com.google.zxing:core`
   - **Native iOS:** `AVFoundation` + QR metadata parsing

---

## 🔗 Related Files

- **QR Generator:** `components/bins/SimpleBinForm.tsx` (line 75-102)
- **QR Scanner:** `components/bins/QRCodeScanner.tsx`
- **Test Page:** `app/test-qr/page.tsx`
- **Backend API:** Need to create `/api/bins/activate` endpoint

---

**Last Updated:** November 2025  
**Project:** EcoEarn Smart Recycling Bin  
**QR Code Format Version:** 1.0
