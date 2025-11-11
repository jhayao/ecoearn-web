# ESP32-CAM Cloudflare R2 Upload System

Automatically upload every captured image to Cloudflare R2 cloud storage.

## 🎯 Why Use R2?

- ☁️ **Cloud Storage** - Access images from anywhere
- 💰 **Cheap** - $0.015/GB/month (10GB free)
- 🚀 **Fast** - Cloudflare's global network
- 📦 **Unlimited** - No storage limits
- 🔒 **Secure** - Private or public buckets
- 📊 **Dataset Collection** - Perfect for AI training data
- 🔍 **Audit Trail** - Every detection saved permanently

## 📋 Setup Instructions

### Step 1: Create Cloudflare R2 Bucket

1. **Sign up** for Cloudflare (free tier available)
2. Go to **R2 Object Storage** in dashboard
3. Click **Create bucket**
4. Name it: `ecoearn-captures` (or your choice)
5. Click **Create bucket**

### Step 2: Generate R2 API Tokens

1. In R2 dashboard, click **Manage R2 API Tokens**
2. Click **Create API token**
3. Set permissions:
   - **Object Read & Write** (or Admin for testing)
4. Click **Create API Token**
5. **Save these credentials** (you'll only see them once):
   - Access Key ID
   - Secret Access Key
   - Account ID

### Step 3: Update ESP32-CAM Code

Open `esp32_cam_r2_upload.ino` and update:

```cpp
// Cloudflare R2 Configuration
const char* R2_ACCOUNT_ID = "abc123def456";  // From step 2
const char* R2_BUCKET_NAME = "ecoearn-captures";  // From step 1
const char* R2_ACCESS_KEY_ID = "your_access_key_id";  // From step 2
const char* R2_SECRET_ACCESS_KEY = "your_secret_key";  // From step 2
```

### Step 4: Upload to ESP32-CAM

1. Open in Arduino IDE
2. Select board: **AI Thinker ESP32-CAM**
3. Upload code
4. Open Serial Monitor @ 115200 baud

### Step 5: Test Upload

1. Type `capture` in Serial Monitor
2. Watch the upload process
3. Check R2 dashboard for uploaded image

## 🚀 Quick Start

```
1. Power on ESP32-CAM
2. Open Serial Monitor @ 115200
3. Type: capture
4. Wait for upload to complete
5. Check R2 dashboard for image
```

## 🎮 Commands

```
capture  - Capture image, identify, upload to R2
test     - Same as 'capture'
start    - Same as 'capture'
stats    - Show upload statistics
help     - Show available commands
```

## 📊 What Gets Uploaded

Each image is uploaded with metadata in the filename:

```
Format: YYYYMMDD_HHMMSS_<material>_<count>.jpg

Examples:
captures/20251111_143052_plastic_1.jpg
captures/20251111_143105_tin_2.jpg
captures/20251111_143120_rejected_3.jpg
```

## 🌐 File Structure in R2

```
ecoearn-captures/
├── captures/
│   ├── 20251111_143052_plastic_1.jpg
│   ├── 20251111_143105_tin_2.jpg
│   ├── 20251111_143120_rejected_3.jpg
│   ├── 20251111_143135_plastic_4.jpg
│   └── ...
```

## 📈 Upload Statistics

View in Serial Monitor:
```
═══════════════════════════════════════════════
           UPLOAD STATISTICS
═══════════════════════════════════════════════
  Total captures: 42
  Successful uploads: 40
  Failed uploads: 2
  Success rate: 95.2%
  Last material: plastic (92.5% confidence)
  Last R2 URL: https://...r2.cloudflarestorage.com/...
═══════════════════════════════════════════════
```

## 💡 Web Interface

Access at `http://<ESP32_IP>/`:

```
┌─────────────────────────────────────┐
│  📸 ESP32-CAM → ☁️ R2 Upload         │
│                                     │
│  [🚀 Capture & Upload] [🔄 Refresh] │
│                                     │
│  📊 Statistics                      │
│  ├─ Total: 42                       │
│  ├─ Success: 40                     │
│  ├─ Failed: 2                       │
│  └─ Last: plastic (92.5%)           │
└─────────────────────────────────────┘
```

## 🔧 Configuration Options

### Image Quality Settings

```cpp
#define FRAME_SIZE FRAMESIZE_SVGA    // 800x600 (default)
#define JPEG_QUALITY 10              // 0-63, lower = better
```

### Available Frame Sizes

| Setting | Resolution | File Size | Use Case |
|---------|------------|-----------|----------|
| `FRAMESIZE_QVGA` | 320x240 | ~10KB | Low bandwidth |
| `FRAMESIZE_VGA` | 640x480 | ~30KB | Standard |
| `FRAMESIZE_SVGA` | 800x600 | ~50KB | **Recommended** |
| `FRAMESIZE_XGA` | 1024x768 | ~80KB | High quality |
| `FRAMESIZE_UXGA` | 1600x1200 | ~150KB | Max quality |

### WiFi Settings

```cpp
const char* WIFI_SSID = "Your_WiFi_Name";
const char* WIFI_PASSWORD = "Your_Password";
```

### Backend API URL

```cpp
const char* BACKEND_API_URL = "http://192.168.1.x:5001/identify/material";
```

## 📦 Storage Costs

Cloudflare R2 Pricing (very cheap!):

| Usage | Cost | Example |
|-------|------|---------|
| First 10GB | **FREE** | ~10,000 images |
| Storage | $0.015/GB/month | 100GB = $1.50/month |
| Downloads (Class A) | $4.50/million | Very cheap |
| Downloads (Class B) | $0.36/million | Almost free |

### Example: 1 Year of Operation

```
Assumptions:
- 100 captures per day
- ~50KB per image
- 36,500 images total
- ~1.8GB storage

Cost breakdown:
- Storage: 1.8GB × $0.015 = $0.027/month
- Annual cost: $0.32/year 🎉

Incredibly cheap!
```

## 🔒 Security Options

### Option 1: Public Bucket (Easy)
- Anyone with URL can view images
- Good for: Testing, public datasets
- Setup: Enable "Public Access" in R2 bucket settings

### Option 2: Private Bucket (Secure)
- Images require authentication
- Good for: Production, sensitive data
- Setup: Use presigned URLs or implement AWS Signature V4

### Current Implementation

⚠️ **NOTE**: The current code uses a **simplified upload method**. For production, you should implement:

1. **AWS Signature V4** for authenticated requests
2. **Presigned URLs** generated by your backend
3. **Token rotation** for security

## 🔄 Integration with Existing System

### Add to esp32_cam_controller.ino

You can easily add R2 upload to your existing ESP32-CAM controller:

```cpp
// Add after material identification
bool identified = captureAndIdentify();
if (identified) {
    // Upload to R2
    uploadToR2(framebuffer);
    
    // Then send result to ESP32 main board
    sendResultToESP32();
}
```

## 🌟 Benefits for Your Project

1. **Audit Trail** - Every detection is saved
2. **Dataset Building** - Collect training data automatically
3. **Quality Control** - Review misclassifications later
4. **Analytics** - Analyze usage patterns
5. **Debugging** - See what camera actually captured
6. **Compliance** - Proof of proper recycling
7. **User Transparency** - Show users what was detected

## 📱 Access Images

### Method 1: Cloudflare Dashboard
1. Go to R2 bucket
2. Browse `captures/` folder
3. Click on image to view
4. Download if needed

### Method 2: R2 Public URL (if public)
```
https://ecoearn-captures.r2.dev/captures/20251111_143052_plastic_1.jpg
```

### Method 3: Custom Domain (Advanced)
Map your own domain to R2 bucket:
```
https://images.ecoearn.com/captures/...
```

## 🚨 Troubleshooting

### Upload Failed
```
❌ Upload failed!
```

**Solutions:**
1. Check WiFi connection
2. Verify R2 credentials
3. Check bucket name spelling
4. Ensure time is synced (required for AWS signatures)
5. Check R2 bucket permissions

### Time Sync Failed
```
⚠️  Time sync failed - R2 uploads may not work!
```

**Solutions:**
1. Check WiFi connection to NTP server
2. Try different NTP server (pool.ntp.org, time.google.com)
3. Increase sync timeout

### WiFi Not Connected
```
⚠️  WiFi not connected
```

**Solutions:**
1. Check SSID and password
2. Move closer to router
3. Check 2.4GHz WiFi (ESP32 doesn't support 5GHz)

## 📊 Serial Monitor Output

```
╔════════════════════════════════════════════════════════╗
║       ESP32-CAM Cloudflare R2 Upload System            ║
╚════════════════════════════════════════════════════════╝

✅ Camera initialized successfully
Connecting to WiFi: Xiaomi_53DE
✅ WiFi connected!
   IP address: 192.168.1.123
🕐 Syncing time with NTP server...
✅ Time synchronized
   Current time: Mon Nov 11 14:30:45 2025
✅ Web server started on port 80

════════════════════════════════════════════════════════
Ready! Type 'help' for available commands
Web interface: http://192.168.1.123
════════════════════════════════════════════════════════

> capture

╔════════════════════════════════════════════════════════╗
║         CAPTURE → IDENTIFY → UPLOAD TO R2              ║
╚════════════════════════════════════════════════════════╝

📸 Step 1/3: Capturing image...
   ✅ Captured 45632 bytes

🔍 Step 2/3: Identifying material...
   ✅ Identified: plastic (92.5% confidence)

☁️  Step 3/3: Uploading to Cloudflare R2...
   Uploading to: https://...r2.cloudflarestorage.com/...
   ✅ Upload successful!
   📦 R2 URL: https://...r2.cloudflarestorage.com/...

════════════════════════════════════════════════════════
✅ Process complete!
════════════════════════════════════════════════════════
```

## 🎯 Next Steps

1. **Upload code** to ESP32-CAM
2. **Configure R2 credentials**
3. **Test with `capture` command**
4. **Check R2 dashboard** for uploaded image
5. **Integrate with main system** for automatic uploads
6. **Implement AWS Signature V4** for production security
7. **Set up custom domain** (optional)
8. **Build analytics dashboard** (optional)

## 💡 Pro Tips

1. **Batch Uploads** - For high-frequency captures, consider batching uploads
2. **Compression** - Images are already JPEG compressed, but you can adjust quality
3. **Metadata** - Add custom metadata to filenames (user ID, bin ID, location)
4. **Lifecycle Rules** - Set R2 to auto-delete old images after X days
5. **CDN** - Use Cloudflare CDN for fast image delivery worldwide
6. **Webhooks** - Trigger actions when new images uploaded (via Cloudflare Workers)

## 🌟 Advanced Features (Optional)

### 1. Automatic Cleanup
Delete images older than 30 days to save storage

### 2. Image Thumbnails
Generate and store thumbnails for faster loading

### 3. Real-time Dashboard
Build web dashboard showing latest uploads

### 4. Analytics
Track upload success rates, material distribution, peak times

### 5. Alerts
Email/SMS notifications on upload failures

---

## 📞 Support

For R2-specific issues:
- Cloudflare R2 Documentation: https://developers.cloudflare.com/r2/
- Cloudflare Community: https://community.cloudflare.com/

Happy uploading! 🚀☁️
