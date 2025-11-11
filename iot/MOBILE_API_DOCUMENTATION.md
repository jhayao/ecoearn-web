# Mobile App API Documentation

## Base URL
```
https://your-domain.com/api
```

---

## 1. User Authentication

### 1.1 Sign Up
**Endpoint:** `POST /auth/signup`

**Request Payload:**
```json
{
  "email": "user@example.com",
  "password": "securePassword123",
  "displayName": "John Doe",
  "phoneNumber": "+1234567890"
}
```

**Success Response (201):**
```json
{
  "success": true,
  "message": "User created successfully",
  "data": {
    "userId": "abc123xyz789",
    "email": "user@example.com",
    "displayName": "John Doe",
    "createdAt": "2025-11-10T08:30:00Z"
  },
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
}
```

**Error Response (400):**
```json
{
  "success": false,
  "error": "Email already exists",
  "code": "AUTH_EMAIL_EXISTS"
}
```

---

### 1.2 Login
**Endpoint:** `POST /auth/login`

**Request Payload:**
```json
{
  "email": "user@example.com",
  "password": "securePassword123"
}
```

**Success Response (200):**
```json
{
  "success": true,
  "message": "Login successful",
  "data": {
    "userId": "abc123xyz789",
    "email": "user@example.com",
    "displayName": "John Doe",
    "profilePicture": "https://storage.googleapis.com/...",
    "points": 1250,
    "totalRecycled": 45
  },
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
}
```

**Error Response (401):**
```json
{
  "success": false,
  "error": "Invalid email or password",
  "code": "AUTH_INVALID_CREDENTIALS"
}
```

---

### 1.3 Get User Profile
**Endpoint:** `GET /users/{userId}`

**Headers:**
```
Authorization: Bearer {token}
```

**Success Response (200):**
```json
{
  "success": true,
  "data": {
    "userId": "abc123xyz789",
    "email": "user@example.com",
    "displayName": "John Doe",
    "phoneNumber": "+1234567890",
    "profilePicture": "https://storage.googleapis.com/...",
    "points": 1250,
    "totalRecycled": 45,
    "recyclingStats": {
      "plastic": 28,
      "tin": 17
    },
    "createdAt": "2025-01-15T10:20:30Z",
    "lastActive": "2025-11-10T08:30:00Z"
  }
}
```

---

### 1.4 Update User Profile
**Endpoint:** `PUT /users/{userId}`

**Headers:**
```
Authorization: Bearer {token}
Content-Type: application/json
```

**Request Payload:**
```json
{
  "displayName": "Jane Doe",
  "phoneNumber": "+1234567890",
  "profilePicture": "base64_encoded_image_data_or_url"
}
```

**Success Response (200):**
```json
{
  "success": true,
  "message": "Profile updated successfully",
  "data": {
    "userId": "abc123xyz789",
    "displayName": "Jane Doe",
    "phoneNumber": "+1234567890",
    "profilePicture": "https://storage.googleapis.com/...",
    "updatedAt": "2025-11-10T08:35:00Z"
  }
}
```

---

## 2. Bin Management

### 2.1 Get Nearby Bins
**Endpoint:** `GET /bins/nearby`

**Query Parameters:**
```
latitude: 14.5995
longitude: 120.9842
radius: 5000 (meters, default: 5000)
```

**Example:**
```
GET /bins/nearby?latitude=14.5995&longitude=120.9842&radius=5000
```

**Headers:**
```
Authorization: Bearer {token}
```

**Success Response (200):**
```json
{
  "success": true,
  "data": {
    "bins": [
      {
        "binId": "bin_001",
        "name": "Mall Entrance Bin",
        "status": "inactive",
        "location": {
          "latitude": 14.5995,
          "longitude": 120.9842,
          "address": "SM City Manila, Philippines"
        },
        "fillLevel": 45,
        "distance": 250.5,
        "image": "https://storage.googleapis.com/...",
        "currentUser": null,
        "createdAt": "2025-10-01T12:00:00Z"
      },
      {
        "binId": "bin_002",
        "name": "Park Recycling Station",
        "status": "active",
        "location": {
          "latitude": 14.6010,
          "longitude": 120.9850,
          "address": "Rizal Park, Manila"
        },
        "fillLevel": 78,
        "distance": 1200.3,
        "image": "https://storage.googleapis.com/...",
        "currentUser": "xyz456abc789",
        "createdAt": "2025-09-15T14:30:00Z"
      }
    ],
    "totalCount": 2,
    "timestamp": "2025-11-10T08:30:00Z"
  }
}
```

---

### 2.2 Get All Bins
**Endpoint:** `GET /bins`

**Headers:**
```
Authorization: Bearer {token}
```

**Success Response (200):**
```json
{
  "success": true,
  "data": {
    "bins": [
      {
        "binId": "bin_001",
        "name": "Mall Entrance Bin",
        "status": "inactive",
        "location": {
          "latitude": 14.5995,
          "longitude": 120.9842
        },
        "fillLevel": 45,
        "image": "https://storage.googleapis.com/...",
        "qrData": "{\"binId\":\"bin_001\",\"type\":\"bin_activation\",\"timestamp\":\"2025-11-10T08:30:00Z\"}",
        "currentUser": null
      }
    ],
    "totalCount": 15
  }
}
```

---

### 2.3 Get Bin Details
**Endpoint:** `GET /bins/{binId}`

**Headers:**
```
Authorization: Bearer {token}
```

**Success Response (200):**
```json
{
  "success": true,
  "data": {
    "binId": "bin_001",
    "name": "Mall Entrance Bin",
    "status": "inactive",
    "location": {
      "latitude": 14.5995,
      "longitude": 120.9842,
      "address": "SM City Manila, Philippines"
    },
    "fillLevel": 45,
    "compartments": {
      "plastic": {
        "level": 40,
        "capacity": 100,
        "lastUpdated": "2025-11-10T07:15:00Z"
      },
      "tin": {
        "level": 35,
        "capacity": 100,
        "lastUpdated": "2025-11-10T07:15:00Z"
      },
      "rejected": {
        "level": 60,
        "capacity": 100,
        "lastUpdated": "2025-11-10T07:15:00Z"
      }
    },
    "image": "https://storage.googleapis.com/...",
    "qrCodePhoto": "https://storage.googleapis.com/...",
    "currentUser": null,
    "lastMaintenance": "2025-11-05T10:00:00Z",
    "createdAt": "2025-10-01T12:00:00Z"
  }
}
```

---

### 2.4 Scan QR Code & Activate Bin
**Endpoint:** `POST /bins/activate`

**Headers:**
```
Authorization: Bearer {token}
Content-Type: application/json
```

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
    "name": "Mall Entrance Bin",
    "status": "active",
    "currentUser": "abc123xyz789",
    "activatedAt": "2025-11-10T08:30:00Z",
    "sessionId": "session_abc123",
    "expiresAt": "2025-11-10T09:30:00Z"
  }
}
```

**Error Response (409):**
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

**Error Response (404):**
```json
{
  "success": false,
  "error": "Bin not found",
  "code": "BIN_NOT_FOUND"
}
```

---

### 2.5 Deactivate Bin
**Endpoint:** `POST /bins/deactivate`

**Headers:**
```
Authorization: Bearer {token}
Content-Type: application/json
```

**Request Payload:**
```json
{
  "binId": "bin_001",
  "userId": "abc123xyz789",
  "sessionId": "session_abc123"
}
```

**Success Response (200):**
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

**Error Response (403):**
```json
{
  "success": false,
  "error": "Unauthorized to deactivate this bin",
  "code": "UNAUTHORIZED_DEACTIVATION"
}
```

---

## 3. Recycling Transactions

### 3.1 Submit Recycling Transaction
**Endpoint:** `POST /recycle`

**Headers:**
```
Authorization: Bearer {token}
Content-Type: application/json
```

**Request Payload:**
```json
{
  "userId": "abc123xyz789",
  "binId": "bin_001",
  "materialType": "plastic",
  "weight": 0.5,
  "quantity": 2,
  "location": {
    "latitude": 14.5995,
    "longitude": 120.9842
  },
  "timestamp": "2025-11-10T08:30:00Z",
  "sessionId": "session_abc123"
}
```

**Success Response (201):**
```json
{
  "success": true,
  "message": "Recycling transaction recorded successfully",
  "data": {
    "transactionId": "txn_001",
    "userId": "abc123xyz789",
    "binId": "bin_001",
    "materialType": "plastic",
    "weight": 0.5,
    "quantity": 2,
    "points": 50,
    "rewards": {
      "basePoints": 40,
      "bonusPoints": 10,
      "bonusReason": "First recycling of the day"
    },
    "location": {
      "latitude": 14.5995,
      "longitude": 120.9842
    },
    "timestamp": "2025-11-10T08:30:00Z",
    "status": "completed"
  }
}
```

**Error Response (400):**
```json
{
  "success": false,
  "error": "Invalid material type",
  "code": "INVALID_MATERIAL",
  "validMaterials": ["plastic", "tin", "rejected"]
}
```

---

### 3.2 Get User Recycling History
**Endpoint:** `GET /recycle/history/{userId}`

**Query Parameters:**
```
page: 1 (default: 1)
limit: 20 (default: 20)
materialType: plastic (optional, filter by material)
startDate: 2025-11-01T00:00:00Z (optional)
endDate: 2025-11-10T23:59:59Z (optional)
```

**Headers:**
```
Authorization: Bearer {token}
```

**Success Response (200):**
```json
{
  "success": true,
  "data": {
    "transactions": [
      {
        "transactionId": "txn_001",
        "binId": "bin_001",
        "binName": "Mall Entrance Bin",
        "materialType": "plastic",
        "weight": 0.5,
        "quantity": 2,
        "points": 50,
        "location": {
          "latitude": 14.5995,
          "longitude": 120.9842,
          "address": "SM City Manila"
        },
        "timestamp": "2025-11-10T08:30:00Z",
        "status": "completed"
      },
      {
        "transactionId": "txn_002",
        "binId": "bin_002",
        "binName": "Park Recycling Station",
        "materialType": "tin",
        "weight": 0.3,
        "quantity": 5,
        "points": 75,
        "location": {
          "latitude": 14.6010,
          "longitude": 120.9850,
          "address": "Rizal Park"
        },
        "timestamp": "2025-11-09T15:20:00Z",
        "status": "completed"
      }
    ],
    "pagination": {
      "currentPage": 1,
      "totalPages": 3,
      "totalItems": 45,
      "itemsPerPage": 20
    },
    "summary": {
      "totalPoints": 2250,
      "totalTransactions": 45,
      "totalWeight": 22.5,
      "materials": {
        "plastic": 28,
        "tin": 17
      }
    }
  }
}
```

---

### 3.3 Get Recycling Stats
**Endpoint:** `GET /recycle/stats/{userId}`

**Query Parameters:**
```
period: month (options: day, week, month, year, all)
```

**Headers:**
```
Authorization: Bearer {token}
```

**Success Response (200):**
```json
{
  "success": true,
  "data": {
    "period": "month",
    "userId": "abc123xyz789",
    "stats": {
      "totalPoints": 1250,
      "totalTransactions": 45,
      "totalWeight": 22.5,
      "averageWeightPerTransaction": 0.5,
      "materials": {
        "plastic": {
          "count": 28,
          "weight": 14.0,
          "points": 700
        },
        "tin": {
          "count": 17,
          "weight": 8.5,
          "points": 550
        }
      },
      "dailyAverage": {
        "transactions": 1.5,
        "weight": 0.75,
        "points": 41.67
      },
      "streakDays": 7,
      "lastRecycled": "2025-11-10T08:30:00Z"
    },
    "chartData": [
      {
        "date": "2025-11-01",
        "plastic": 2,
        "tin": 1,
        "points": 75
      },
      {
        "date": "2025-11-02",
        "plastic": 1,
        "tin": 0,
        "points": 25
      }
    ],
    "timestamp": "2025-11-10T08:30:00Z"
  }
}
```

---

## 4. Reports

### 4.1 Submit User Report
**Endpoint:** `POST /reports`

**Headers:**
```
Authorization: Bearer {token}
Content-Type: application/json
```

**Request Payload:**
```json
{
  "userId": "abc123xyz789",
  "userName": "John Doe",
  "description": "Bin is full and needs maintenance",
  "location": "SM City Manila, Main Entrance",
  "coordinates": {
    "latitude": 14.5995,
    "longitude": 120.9842
  },
  "image": "base64_encoded_image_data",
  "binId": "bin_001",
  "reportType": "maintenance",
  "priority": "high"
}
```

**Success Response (201):**
```json
{
  "success": true,
  "message": "Report submitted successfully",
  "data": {
    "reportId": "report_001",
    "userId": "abc123xyz789",
    "userName": "John Doe",
    "description": "Bin is full and needs maintenance",
    "location": "SM City Manila, Main Entrance",
    "coordinates": {
      "latitude": 14.5995,
      "longitude": 120.9842
    },
    "image": "https://storage.googleapis.com/...",
    "binId": "bin_001",
    "reportType": "maintenance",
    "priority": "high",
    "status": "pending",
    "timestamp": "2025-11-10T08:30:00Z"
  }
}
```

---

### 4.2 Get User Reports
**Endpoint:** `GET /reports/user/{userId}`

**Headers:**
```
Authorization: Bearer {token}
```

**Success Response (200):**
```json
{
  "success": true,
  "data": {
    "reports": [
      {
        "reportId": "report_001",
        "description": "Bin is full and needs maintenance",
        "location": "SM City Manila",
        "image": "https://storage.googleapis.com/...",
        "binId": "bin_001",
        "reportType": "maintenance",
        "priority": "high",
        "status": "pending",
        "timestamp": "2025-11-10T08:30:00Z",
        "adminResponse": null
      },
      {
        "reportId": "report_002",
        "description": "Bin malfunction",
        "location": "Rizal Park",
        "image": "https://storage.googleapis.com/...",
        "binId": "bin_002",
        "reportType": "technical",
        "priority": "medium",
        "status": "resolved",
        "timestamp": "2025-11-08T14:20:00Z",
        "adminResponse": "Issue has been fixed. Thank you for reporting!",
        "resolvedAt": "2025-11-09T10:00:00Z"
      }
    ],
    "totalCount": 2
  }
}
```

---

## 5. Points & Rewards

### 5.1 Get User Points
**Endpoint:** `GET /points/{userId}`

**Headers:**
```
Authorization: Bearer {token}
```

**Success Response (200):**
```json
{
  "success": true,
  "data": {
    "userId": "abc123xyz789",
    "currentPoints": 1250,
    "lifetimePoints": 3450,
    "pointsThisMonth": 450,
    "rank": 42,
    "nextRankPoints": 1500,
    "pointsToNextRank": 250,
    "badges": [
      {
        "badgeId": "eco_warrior",
        "name": "Eco Warrior",
        "description": "Recycled 100 items",
        "icon": "https://storage.googleapis.com/...",
        "earnedAt": "2025-10-15T12:00:00Z"
      }
    ]
  }
}
```

---

### 5.2 Redeem Points
**Endpoint:** `POST /points/redeem`

**Headers:**
```
Authorization: Bearer {token}
Content-Type: application/json
```

**Request Payload:**
```json
{
  "userId": "abc123xyz789",
  "rewardId": "reward_001",
  "pointsCost": 500
}
```

**Success Response (200):**
```json
{
  "success": true,
  "message": "Reward redeemed successfully",
  "data": {
    "redemptionId": "redemption_001",
    "userId": "abc123xyz789",
    "rewardId": "reward_001",
    "rewardName": "10% Store Discount",
    "pointsCost": 500,
    "remainingPoints": 750,
    "voucherCode": "ECOEARNS-ABC123",
    "expiresAt": "2025-12-10T23:59:59Z",
    "redeemedAt": "2025-11-10T08:30:00Z"
  }
}
```

**Error Response (400):**
```json
{
  "success": false,
  "error": "Insufficient points",
  "code": "INSUFFICIENT_POINTS",
  "data": {
    "currentPoints": 300,
    "required": 500,
    "shortage": 200
  }
}
```

---

### 5.3 Get Available Rewards
**Endpoint:** `GET /rewards`

**Headers:**
```
Authorization: Bearer {token}
```

**Success Response (200):**
```json
{
  "success": true,
  "data": {
    "rewards": [
      {
        "rewardId": "reward_001",
        "name": "10% Store Discount",
        "description": "Get 10% off your next purchase",
        "pointsCost": 500,
        "category": "discount",
        "image": "https://storage.googleapis.com/...",
        "available": true,
        "stock": 50,
        "expiryDays": 30
      },
      {
        "rewardId": "reward_002",
        "name": "Eco-Friendly Water Bottle",
        "description": "Reusable stainless steel water bottle",
        "pointsCost": 1500,
        "category": "merchandise",
        "image": "https://storage.googleapis.com/...",
        "available": true,
        "stock": 10,
        "expiryDays": null
      }
    ]
  }
}
```

---

## 6. Leaderboard

### 6.1 Get Leaderboard
**Endpoint:** `GET /leaderboard`

**Query Parameters:**
```
period: month (options: day, week, month, all)
limit: 10 (default: 10)
```

**Headers:**
```
Authorization: Bearer {token}
```

**Success Response (200):**
```json
{
  "success": true,
  "data": {
    "period": "month",
    "leaderboard": [
      {
        "rank": 1,
        "userId": "user_top1",
        "displayName": "EcoChampion",
        "profilePicture": "https://storage.googleapis.com/...",
        "points": 5420,
        "totalRecycled": 180,
        "badges": 12
      },
      {
        "rank": 2,
        "userId": "user_top2",
        "displayName": "GreenWarrior",
        "profilePicture": "https://storage.googleapis.com/...",
        "points": 4890,
        "totalRecycled": 165,
        "badges": 10
      }
    ],
    "userRank": {
      "rank": 42,
      "userId": "abc123xyz789",
      "displayName": "John Doe",
      "profilePicture": "https://storage.googleapis.com/...",
      "points": 1250,
      "totalRecycled": 45,
      "badges": 3
    },
    "timestamp": "2025-11-10T08:30:00Z"
  }
}
```

---

## 7. Notifications

### 7.1 Get User Notifications
**Endpoint:** `GET /notifications/{userId}`

**Query Parameters:**
```
unreadOnly: true (default: false)
limit: 20 (default: 20)
```

**Headers:**
```
Authorization: Bearer {token}
```

**Success Response (200):**
```json
{
  "success": true,
  "data": {
    "notifications": [
      {
        "notificationId": "notif_001",
        "type": "transaction",
        "title": "Points Earned!",
        "message": "You earned 50 points for recycling plastic",
        "data": {
          "transactionId": "txn_001",
          "points": 50
        },
        "read": false,
        "timestamp": "2025-11-10T08:30:00Z"
      },
      {
        "notificationId": "notif_002",
        "type": "achievement",
        "title": "New Badge Unlocked!",
        "message": "You've earned the 'Eco Warrior' badge",
        "data": {
          "badgeId": "eco_warrior",
          "badgeName": "Eco Warrior"
        },
        "read": true,
        "timestamp": "2025-11-09T15:20:00Z"
      }
    ],
    "unreadCount": 3,
    "totalCount": 25
  }
}
```

---

### 7.2 Mark Notification as Read
**Endpoint:** `PUT /notifications/{notificationId}/read`

**Headers:**
```
Authorization: Bearer {token}
```

**Success Response (200):**
```json
{
  "success": true,
  "message": "Notification marked as read",
  "data": {
    "notificationId": "notif_001",
    "read": true,
    "readAt": "2025-11-10T08:35:00Z"
  }
}
```

---

## 8. Material Pricing

### 8.1 Get Current Prices
**Endpoint:** `GET /pricing`

**Success Response (200):**
```json
{
  "success": true,
  "data": {
    "pricing": {
      "plastic": {
        "pricePerKg": 25.00,
        "pointsPerKg": 100,
        "currency": "PHP",
        "lastUpdated": "2025-11-01T00:00:00Z"
      },
      "tin": {
        "pricePerKg": 45.00,
        "pointsPerKg": 150,
        "currency": "PHP",
        "lastUpdated": "2025-11-01T00:00:00Z"
      }
    },
    "conversionRate": {
      "pointsToPhp": 0.25,
      "description": "1 point = ₱0.25"
    }
  }
}
```

---

## Error Codes Reference

| Code | HTTP Status | Description |
|------|-------------|-------------|
| `AUTH_INVALID_CREDENTIALS` | 401 | Invalid email or password |
| `AUTH_EMAIL_EXISTS` | 400 | Email already registered |
| `AUTH_TOKEN_EXPIRED` | 401 | Authentication token expired |
| `AUTH_TOKEN_INVALID` | 401 | Invalid authentication token |
| `BIN_NOT_FOUND` | 404 | Bin ID does not exist |
| `BIN_ALREADY_ACTIVE` | 409 | Bin is currently in use |
| `UNAUTHORIZED_DEACTIVATION` | 403 | User not authorized to deactivate bin |
| `INVALID_MATERIAL` | 400 | Invalid material type provided |
| `INSUFFICIENT_POINTS` | 400 | Not enough points for redemption |
| `REWARD_OUT_OF_STOCK` | 400 | Reward is no longer available |
| `INVALID_REQUEST` | 400 | Request payload validation failed |
| `SERVER_ERROR` | 500 | Internal server error |

---

## Rate Limits

- **Authentication endpoints:** 10 requests per minute
- **Transaction endpoints:** 30 requests per minute
- **General endpoints:** 60 requests per minute

**Rate Limit Headers:**
```
X-RateLimit-Limit: 60
X-RateLimit-Remaining: 45
X-RateLimit-Reset: 1699608000
```

---

## WebSocket Events (Optional - for Real-time Features)

If you decide to implement WebSocket for real-time updates:

**Connection URL:**
```
wss://your-domain.com/ws?token={auth_token}
```

**Events to Listen:**
```json
{
  "event": "bin_status_changed",
  "data": {
    "binId": "bin_001",
    "status": "inactive",
    "timestamp": "2025-11-10T08:30:00Z"
  }
}

{
  "event": "points_awarded",
  "data": {
    "userId": "abc123xyz789",
    "points": 50,
    "newTotal": 1300,
    "transactionId": "txn_001"
  }
}

{
  "event": "notification",
  "data": {
    "notificationId": "notif_003",
    "type": "achievement",
    "title": "Streak Milestone!",
    "message": "You've recycled for 7 days in a row!"
  }
}
```

---

## Testing Credentials (Development Only)

**Test User:**
```
Email: test@ecoearn.com
Password: Test123456!
UserId: test_user_001
```

**Test Bin:**
```
BinId: bin_test_001
QR Data: {"binId":"bin_test_001","type":"bin_activation","timestamp":"2025-11-10T08:30:00Z"}
```

---

## Implementation Notes

1. **Authentication:** Use Firebase Authentication tokens in the `Authorization` header
2. **Image Upload:** Send images as base64 encoded strings or use multipart/form-data
3. **Timestamps:** All timestamps are in ISO 8601 format (UTC)
4. **Pagination:** Use `page` and `limit` query parameters
5. **Error Handling:** Always check the `success` field in responses
6. **Retry Logic:** Implement exponential backoff for failed requests
7. **Offline Support:** Cache responses locally and sync when online

---

## Mobile SDK Examples

### React Native (Axios)

```javascript
import axios from 'axios';

const API_BASE_URL = 'https://your-domain.com/api';

// Initialize API client
const apiClient = axios.create({
  baseURL: API_BASE_URL,
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json',
  },
});

// Add auth token to requests
apiClient.interceptors.request.use((config) => {
  const token = getAuthToken(); // Get from AsyncStorage
  if (token) {
    config.headers.Authorization = `Bearer ${token}`;
  }
  return config;
});

// Activate bin after QR scan
export const activateBin = async (binId, userId, location) => {
  try {
    const response = await apiClient.post('/bins/activate', {
      binId,
      userId,
      scannedAt: new Date().toISOString(),
      location,
    });
    return response.data;
  } catch (error) {
    if (error.response?.status === 409) {
      throw new Error('Bin is already in use');
    }
    throw error;
  }
};

// Submit recycling transaction
export const submitRecycling = async (data) => {
  try {
    const response = await apiClient.post('/recycle', {
      ...data,
      timestamp: new Date().toISOString(),
    });
    return response.data;
  } catch (error) {
    throw error;
  }
};

// Get nearby bins
export const getNearbyBins = async (latitude, longitude, radius = 5000) => {
  try {
    const response = await apiClient.get('/bins/nearby', {
      params: { latitude, longitude, radius },
    });
    return response.data;
  } catch (error) {
    throw error;
  }
};
```

### Flutter (http package)

```dart
import 'dart:convert';
import 'package:http/http.dart' as http;

class ApiService {
  static const String baseUrl = 'https://your-domain.com/api';
  String? _authToken;

  void setAuthToken(String token) {
    _authToken = token;
  }

  Map<String, String> get _headers => {
    'Content-Type': 'application/json',
    if (_authToken != null) 'Authorization': 'Bearer $_authToken',
  };

  // Activate bin
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
      return jsonDecode(response.body);
    } else if (response.statusCode == 409) {
      throw Exception('Bin is already in use');
    } else {
      throw Exception('Failed to activate bin');
    }
  }

  // Submit recycling transaction
  Future<Map<String, dynamic>> submitRecycling({
    required String userId,
    required String binId,
    required String materialType,
    required double weight,
    required int quantity,
    required double latitude,
    required double longitude,
    required String sessionId,
  }) async {
    final response = await http.post(
      Uri.parse('$baseUrl/recycle'),
      headers: _headers,
      body: jsonEncode({
        'userId': userId,
        'binId': binId,
        'materialType': materialType,
        'weight': weight,
        'quantity': quantity,
        'location': {
          'latitude': latitude,
          'longitude': longitude,
        },
        'timestamp': DateTime.now().toIso8601String(),
        'sessionId': sessionId,
      }),
    );

    if (response.statusCode == 201) {
      return jsonDecode(response.body);
    } else {
      throw Exception('Failed to submit transaction');
    }
  }

  // Get nearby bins
  Future<Map<String, dynamic>> getNearbyBins({
    required double latitude,
    required double longitude,
    int radius = 5000,
  }) async {
    final uri = Uri.parse('$baseUrl/bins/nearby').replace(
      queryParameters: {
        'latitude': latitude.toString(),
        'longitude': longitude.toString(),
        'radius': radius.toString(),
      },
    );

    final response = await http.get(uri, headers: _headers);

    if (response.statusCode == 200) {
      return jsonDecode(response.body);
    } else {
      throw Exception('Failed to load nearby bins');
    }
  }
}
```

---

## Next Steps for Backend Implementation

To implement these APIs, you'll need to create:

1. **Firebase Cloud Functions** or **Next.js API Routes**
2. **Firestore Security Rules** for data access control
3. **Firebase Storage Rules** for image uploads
4. **Server-side validation** for all endpoints
5. **Rate limiting middleware**
6. **Error handling middleware**
7. **Logging and monitoring**

Would you like me to help you implement any specific endpoint?
