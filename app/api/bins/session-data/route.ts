import { NextRequest, NextResponse } from 'next/server';
import { AdminService } from '@/lib/admin-service';

export async function POST(request: NextRequest) {
  console.log('[Session Data API] ESP32 request received');

  try {
    // Get raw body for debugging
    const rawBody = await request.text();
    console.log('[Session Data API] Raw request body:', rawBody);

    // Parse the request body
    const body = JSON.parse(rawBody);
    const { userId, sessionData } = body;

    if (!userId) {
      return NextResponse.json({
        success: false,
        error: 'userId is required'
      }, { status: 400 });
    }

    if (!sessionData) {
      return NextResponse.json({
        success: false,
        error: 'sessionData is required'
      }, { status: 400 });
    }

    console.log('[Session Data API] Processing session data:', { userId, sessionData });

    const adminService = new AdminService();

    // Calculate points based on session data
    let totalPoints = 0;
    let totalPlastic = 0;
    let totalTin = 0;

    // Calculate plastic points (assuming each plastic bottle is worth points based on pricing)
    if (sessionData.plasticCount) {
      const plasticPoints = await adminService.calculatePoints('plastic bottle', null, 0, sessionData.plasticCount);
      totalPoints += plasticPoints;
      totalPlastic = sessionData.plasticCount;
    }

    // Calculate tin can points
    if (sessionData.tinCount) {
      const tinPoints = await adminService.calculatePoints('tin can', null, 0, sessionData.tinCount);
      totalPoints += tinPoints;
      totalTin = sessionData.tinCount;
    }

    // Add points to user account (saves to total_points)
    if (totalPoints > 0) {
      await adminService.addUserPoints(userId, totalPoints);
    }

    // Log recycling activity
    if (totalPlastic > 0 || totalTin > 0) {
      // Get user email for logging
      const { doc, getDoc } = await import('firebase/firestore');
      const { db } = await import('@/lib/firebase');
      const userDoc = await getDoc(doc(db, 'users', userId));
      const userData = userDoc.data();
      const userEmail = userData?.email || userId;

      await adminService.logRecyclingActivity({
        userId,
        userEmail,
        materialType: totalPlastic > 0 ? 'plastic bottle' : 'tin can',
        quantity: totalPlastic || totalTin,
        weight: totalPlastic > 0 ? totalPlastic * 0.5 : totalTin * 0.3, // Estimate weight
        pointsEarned: totalPoints,
        binId: 'esp32-bin', // Default bin ID for ESP32
        binName: 'Smart Bin',
        sessionData
      });
    }

    console.log(`[Session Data API] Awarded ${totalPoints} points to user ${userId}`);

    return NextResponse.json({
      success: true,
      message: 'ESP32 session data processed successfully',
      data: {
        pointsAwarded: totalPoints,
        sessionProcessed: true,
        plasticProcessed: totalPlastic,
        tinProcessed: totalTin
      }
    });

  } catch (error) {
    console.error('[Session Data API] Error processing session data:', error);
    return NextResponse.json({
      success: false,
      error: 'Failed to process session data',
      details: error instanceof Error ? error.message : 'Unknown error'
    }, { status: 500 });
  }
}

// Handle GET requests
export async function GET() {
  return NextResponse.json({
    message: 'ESP32 Session Data Endpoint',
    method: 'POST',
    description: 'ESP32 sends session data after bin deactivation for point calculation',
    requiredFields: ['userId', 'sessionData'],
    requiredHeaders: ['x-api-key'],
    sessionData: {
      plasticCount: 'number of plastic bottles detected',
      tinCount: 'number of tin cans detected',
      rejectedCount: 'number of rejected items',
      sessionId: 'session identifier'
    },
    note: 'Points are calculated and awarded based on session data and current pricing configuration'
  });
}