# Points Calculation System

## Overview

The EcoEarn system awards points to users based on the **material type** and **weight** of items recycled. Points are calculated server-side and credited to the user's account.

---

## 📊 Point Calculation Formula

### Basic Formula
```
Points = (Weight in KG) × (Points per KG for Material Type)
```

### Material Pricing Table

Based on your current system:

| Material Type | Price per KG (PHP) | Points per KG | Points per 100g |
|---------------|-------------------|---------------|-----------------|
| **Plastic** | ₱25.00 | 100 points | 10 points |
| **Tin/Aluminum** | ₱45.00 | 150 points | 15 points |
| **Rejected** | ₱0.00 | 0 points | 0 points |

### Conversion Rate
```
1 Point = ₱0.25
400 Points = ₱100.00
```

---

## 🔢 Calculation Examples

### Example 1: Plastic Bottle (500g = 0.5kg)
```
Weight: 0.5 kg
Material: Plastic
Points per KG: 100

Calculation:
Points = 0.5 kg × 100 points/kg = 50 points
Value = 50 × ₱0.25 = ₱12.50
```

### Example 2: Aluminum Can (30g = 0.03kg)
```
Weight: 0.03 kg
Material: Tin
Points per KG: 150

Calculation:
Points = 0.03 kg × 150 points/kg = 4.5 points
Rounded = 5 points
Value = 5 × ₱0.25 = ₱1.25
```

### Example 3: Multiple Items (3 plastic bottles)
```
Item 1: 0.5 kg × 100 = 50 points
Item 2: 0.45 kg × 100 = 45 points
Item 3: 0.6 kg × 100 = 60 points

Total: 155 points
Value: ₱38.75
```

### Example 4: Rejected Item
```
Weight: 0.5 kg
Material: Rejected
Points per KG: 0

Calculation:
Points = 0.5 kg × 0 points/kg = 0 points
Value = ₱0.00

Note: Item is disposed but user gets no points
```

---

## 🎯 How Points are Calculated in the System

### Step 1: ESP32 Submits Transaction

After trash is deposited, ESP32 sends transaction data:

**Endpoint:** `POST /iot/recycle`

**Request:**
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
  "sessionId": "session_abc123"
}
```

### Step 2: Server Calculates Points

**Backend Logic (Node.js/Firebase Example):**

```javascript
// Material pricing from Firestore or config
const MATERIAL_PRICING = {
  plastic: {
    pricePerKg: 25.00,
    pointsPerKg: 100
  },
  tin: {
    pricePerKg: 45.00,
    pointsPerKg: 150
  },
  rejected: {
    pricePerKg: 0.00,
    pointsPerKg: 0
  }
};

async function calculatePoints(materialType, weight) {
  const pricing = MATERIAL_PRICING[materialType.toLowerCase()];
  
  if (!pricing) {
    throw new Error('Invalid material type');
  }
  
  // Calculate base points
  const basePoints = weight * pricing.pointsPerKg;
  
  // Round to nearest integer
  let totalPoints = Math.round(basePoints);
  
  // Apply bonuses (optional)
  const bonuses = await calculateBonuses(materialType, weight);
  totalPoints += bonuses.total;
  
  return {
    basePoints: Math.round(basePoints),
    bonusPoints: bonuses.total,
    totalPoints: totalPoints,
    cashValue: totalPoints * 0.25,
    breakdown: bonuses.breakdown
  };
}

async function calculateBonuses(materialType, weight) {
  let bonuses = {
    total: 0,
    breakdown: []
  };
  
  // Bonus 1: First recycling of the day
  const isFirstToday = await checkFirstRecyclingToday(userId);
  if (isFirstToday) {
    bonuses.total += 10;
    bonuses.breakdown.push({
      type: 'first_of_day',
      points: 10,
      description: 'First recycling of the day'
    });
  }
  
  // Bonus 2: Large item bonus (>1kg)
  if (weight >= 1.0) {
    const bonus = Math.floor(weight) * 5;
    bonuses.total += bonus;
    bonuses.breakdown.push({
      type: 'large_item',
      points: bonus,
      description: 'Large item bonus'
    });
  }
  
  // Bonus 3: Streak bonus (consecutive days)
  const streakDays = await getUserStreakDays(userId);
  if (streakDays >= 7) {
    const streakBonus = Math.min(streakDays, 30) * 2;
    bonuses.total += streakBonus;
    bonuses.breakdown.push({
      type: 'streak_bonus',
      points: streakBonus,
      description: `${streakDays}-day streak bonus`
    });
  }
  
  // Bonus 4: High-value material bonus (tin)
  if (materialType === 'tin') {
    bonuses.total += 5;
    bonuses.breakdown.push({
      type: 'premium_material',
      points: 5,
      description: 'Premium material bonus'
    });
  }
  
  return bonuses;
}
```

### Step 3: Update User Points

```javascript
async function recordTransaction(transactionData) {
  const { userId, materialType, weight, binId, sessionId } = transactionData;
  
  // Calculate points
  const pointsCalculation = await calculatePoints(materialType, weight);
  
  // Get current user points
  const userDoc = await getDoc(doc(db, 'users', userId));
  const currentPoints = userDoc.data().points || 0;
  const newTotalPoints = currentPoints + pointsCalculation.totalPoints;
  
  // Update user document
  await updateDoc(doc(db, 'users', userId), {
    points: newTotalPoints,
    totalRecycled: increment(1),
    [`recyclingStats.${materialType}`]: increment(1),
    lastRecycled: serverTimestamp()
  });
  
  // Record transaction
  const transactionRef = await addDoc(collection(db, 'transactions'), {
    userId: userId,
    binId: binId,
    sessionId: sessionId,
    materialType: materialType,
    weight: weight,
    quantity: 1,
    basePoints: pointsCalculation.basePoints,
    bonusPoints: pointsCalculation.bonusPoints,
    totalPoints: pointsCalculation.totalPoints,
    cashValue: pointsCalculation.cashValue,
    bonusBreakdown: pointsCalculation.breakdown,
    timestamp: serverTimestamp(),
    status: 'completed'
  });
  
  return {
    transactionId: transactionRef.id,
    points: pointsCalculation.totalPoints,
    newTotalPoints: newTotalPoints,
    breakdown: pointsCalculation
  };
}
```

### Step 4: Server Response

**Response to ESP32:**
```json
{
  "success": true,
  "message": "Transaction recorded successfully",
  "data": {
    "transactionId": "txn_001",
    "points": 50,
    "newTotalPoints": 1300,
    "breakdown": {
      "basePoints": 50,
      "bonusPoints": 0,
      "totalPoints": 50,
      "cashValue": 12.50,
      "bonuses": []
    },
    "compartmentStatus": {
      "plastic": 46,
      "tin": 35,
      "rejected": 60
    }
  },
  "timestamp": "2025-11-10T08:30:20Z"
}
```

### Step 5: Mobile App Notification

Mobile app receives real-time update via Firestore listener:

```javascript
// Firestore listener on user document
onSnapshot(doc(db, 'users', userId), (doc) => {
  const data = doc.data();
  
  // Show notification
  showNotification({
    title: '🎉 Points Earned!',
    message: `+${lastTransaction.points} points`,
    total: data.points
  });
  
  // Update UI
  updatePointsDisplay(data.points);
});
```

---

## 💰 Bonus Points System

### Available Bonuses

| Bonus Type | Criteria | Points | Description |
|------------|----------|--------|-------------|
| **First of Day** | First recycling today | +10 | Encourages daily usage |
| **Large Item** | Item ≥ 1kg | +5 per kg | Rewards bulk recycling |
| **Streak Bonus** | 7+ consecutive days | +2 per day | Max 60 points (30 days) |
| **Premium Material** | Tin/Aluminum | +5 | Higher value material |
| **Weekend Warrior** | Recycle on weekend | +15 | Bonus for weekend activity |
| **Early Bird** | Recycle before 9 AM | +5 | Morning recycling bonus |
| **Bulk Recycler** | 5+ items in session | +20 | Encourages multiple items |

### Example with Bonuses

```
Base Transaction:
- Material: Plastic
- Weight: 0.5 kg
- Base Points: 50

Bonuses Applied:
+ First of Day: 10 points
+ 7-Day Streak: 14 points (7 days × 2)

Total: 50 + 10 + 14 = 74 points
Value: ₱18.50
```

---

## 📱 Points Display in Mobile App

### Transaction Screen

```
┌──────────────────────────────────┐
│  🎉 Transaction Complete!        │
├──────────────────────────────────┤
│                                  │
│  Material: Plastic Bottle        │
│  Weight: 0.5 kg                  │
│                                  │
│  Base Points:        50 pts      │
│  First of Day:      +10 pts      │
│  Streak Bonus:      +14 pts      │
│  ─────────────────────────────   │
│  Total Earned:       74 pts      │
│  Cash Value:        ₱18.50       │
│                                  │
│  Your Total:      1,374 pts      │
│  (₱343.50)                       │
│                                  │
│  [Continue Recycling]            │
│  [Finish Session]                │
└──────────────────────────────────┘
```

---

## 🔧 Backend Implementation Example

### Firebase Cloud Function

```javascript
// functions/src/index.ts
import * as functions from 'firebase-functions';
import * as admin from 'firebase-admin';

const db = admin.firestore();

export const submitRecycling = functions.https.onRequest(async (req, res) => {
  try {
    const { userId, materialType, weight, binId, sessionId } = req.body;
    
    // Validate input
    if (!userId || !materialType || !weight) {
      return res.status(400).json({ 
        success: false, 
        error: 'Missing required fields' 
      });
    }
    
    // Get material pricing
    const pricingDoc = await db.collection('config').doc('pricing').get();
    const pricing = pricingDoc.data()?.[materialType];
    
    if (!pricing) {
      return res.status(400).json({ 
        success: false, 
        error: 'Invalid material type' 
      });
    }
    
    // Calculate base points
    const basePoints = Math.round(weight * pricing.pointsPerKg);
    
    // Calculate bonuses
    const bonuses = await calculateBonuses(userId, materialType, weight);
    const totalPoints = basePoints + bonuses.total;
    
    // Update user points
    const userRef = db.collection('users').doc(userId);
    await userRef.update({
      points: admin.firestore.FieldValue.increment(totalPoints),
      totalRecycled: admin.firestore.FieldValue.increment(1),
      [`recyclingStats.${materialType}`]: admin.firestore.FieldValue.increment(1),
      lastRecycled: admin.firestore.FieldValue.serverTimestamp()
    });
    
    // Record transaction
    const transactionRef = await db.collection('transactions').add({
      userId,
      binId,
      sessionId,
      materialType,
      weight,
      basePoints,
      bonusPoints: bonuses.total,
      totalPoints,
      cashValue: totalPoints * 0.25,
      bonusBreakdown: bonuses.breakdown,
      timestamp: admin.firestore.FieldValue.serverTimestamp(),
      status: 'completed'
    });
    
    // Get new total
    const userDoc = await userRef.get();
    const newTotalPoints = userDoc.data()?.points || 0;
    
    // Return response
    res.status(201).json({
      success: true,
      message: 'Transaction recorded successfully',
      data: {
        transactionId: transactionRef.id,
        points: totalPoints,
        newTotalPoints: newTotalPoints,
        breakdown: {
          basePoints,
          bonusPoints: bonuses.total,
          totalPoints,
          cashValue: totalPoints * 0.25,
          bonuses: bonuses.breakdown
        }
      }
    });
    
  } catch (error) {
    console.error('Error processing transaction:', error);
    res.status(500).json({ 
      success: false, 
      error: 'Internal server error' 
    });
  }
});
```

---

## 📊 Firestore Data Structure

### Users Collection

```javascript
// users/{userId}
{
  "userId": "abc123xyz789",
  "email": "user@example.com",
  "displayName": "John Doe",
  "points": 1374,
  "totalRecycled": 42,
  "recyclingStats": {
    "plastic": 28,
    "tin": 14,
    "rejected": 0
  },
  "streakDays": 7,
  "lastRecycled": Timestamp,
  "createdAt": Timestamp
}
```

### Transactions Collection

```javascript
// transactions/{transactionId}
{
  "transactionId": "txn_001",
  "userId": "abc123xyz789",
  "binId": "bin_001",
  "sessionId": "session_abc123",
  "materialType": "plastic",
  "weight": 0.5,
  "quantity": 1,
  "basePoints": 50,
  "bonusPoints": 24,
  "totalPoints": 74,
  "cashValue": 18.50,
  "bonusBreakdown": [
    {
      "type": "first_of_day",
      "points": 10,
      "description": "First recycling of the day"
    },
    {
      "type": "streak_bonus",
      "points": 14,
      "description": "7-day streak bonus"
    }
  ],
  "timestamp": Timestamp,
  "status": "completed"
}
```

### Pricing Configuration

```javascript
// config/pricing
{
  "plastic": {
    "pricePerKg": 25.00,
    "pointsPerKg": 100,
    "enabled": true
  },
  "tin": {
    "pricePerKg": 45.00,
    "pointsPerKg": 150,
    "enabled": true
  },
  "rejected": {
    "pricePerKg": 0.00,
    "pointsPerKg": 0,
    "enabled": true
  },
  "conversionRate": 0.25,
  "lastUpdated": Timestamp
}
```

---

## ⚙️ Weight Measurement

### How Weight is Determined

**Option 1: Load Cell (Recommended)**
```cpp
// ESP32 with HX711 load cell
#include <HX711.h>

HX711 scale;
const int LOADCELL_DOUT_PIN = 32;
const int LOADCELL_SCK_PIN = 33;

void setup() {
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(2280.f);  // Calibration factor
  scale.tare();
}

float getWeight() {
  // Get weight in kg
  float weight = scale.get_units(10) / 1000.0;  // Convert g to kg
  return weight;
}
```

**Option 2: Estimated Weight (Current Implementation)**
```cpp
// Estimate based on material type and size detected by AI
float estimateWeight(String materialType, String sizeCategory) {
  // Average weights
  if (materialType == "plastic") {
    if (sizeCategory == "small") return 0.05;   // 50g
    if (sizeCategory == "medium") return 0.25;  // 250g
    if (sizeCategory == "large") return 0.5;    // 500g
  }
  else if (materialType == "tin") {
    if (sizeCategory == "small") return 0.015;  // 15g
    if (sizeCategory == "medium") return 0.03;  // 30g
    if (sizeCategory == "large") return 0.05;   // 50g
  }
  return 0.1;  // Default 100g
}
```

**Option 3: User Input (Manual Entry)**
```javascript
// Mobile app asks user to confirm weight
showWeightDialog({
  estimatedWeight: 0.5,
  allowEdit: true,
  onConfirm: (weight) => {
    submitTransaction(weight);
  }
});
```

---

## 🎯 Summary

### Points Flow

```
1. User deposits trash
   ↓
2. ESP32-CAM classifies material type
   ↓
3. Load cell measures weight (or estimated)
   ↓
4. ESP32 → POST /iot/recycle
   ↓
5. Server calculates:
   - Base points = weight × pointsPerKg
   - Bonus points (streaks, first of day, etc.)
   - Total points = base + bonuses
   ↓
6. Update user's point balance
   ↓
7. Record transaction in Firestore
   ↓
8. Send response to ESP32
   ↓
9. Mobile app updates UI in real-time
   ↓
10. User sees points earned! 🎉
```

### Key Formulas

```
Base Points = Weight (kg) × Points per KG
Total Points = Base Points + Bonus Points
Cash Value = Total Points × ₱0.25

Plastic: 100 points/kg
Tin: 150 points/kg
Rejected: 0 points/kg
```

---

## 💡 Recommendations

1. **Use Load Cell**: For accurate weight measurement
2. **Implement Bonuses**: Increase user engagement
3. **Real-time Updates**: Use Firestore listeners
4. **Admin Panel**: Allow pricing adjustments
5. **Analytics**: Track points awarded per material type
6. **Point Redemption**: Allow users to redeem points for rewards

Would you like me to help implement the backend point calculation logic or add weight sensing with load cells?
