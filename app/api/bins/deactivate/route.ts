import { NextRequest, NextResponse } from 'next/server';
import { AdminService } from '@/lib/admin-service';
import { doc, updateDoc, serverTimestamp } from 'firebase/firestore';
import { db } from '@/lib/firebase';

export async function POST(request: NextRequest) {
  console.log('[Deactivate API] Mobile app deactivation request received');

  try {
    // Parse the request body
    const body = await request.json();
    const { binId, userId, sessionId } = body;

    if (!binId || !userId) {
      return NextResponse.json({
        success: false,
        error: 'binId and userId are required'
      }, { status: 400 });
    }

    console.log('[Deactivate API] Processing deactivation request:', { binId, userId, sessionId });

    const adminService = new AdminService();

    // Verify the user is authorized to deactivate this bin
    const canDeactivate = await adminService.canUserRecycleInBin(binId, userId);
    if (!canDeactivate) {
      return NextResponse.json({
        success: false,
        error: 'User is not authorized to deactivate this bin',
        code: 'UNAUTHORIZED_DEACTIVATION'
      }, { status: 403 });
    }

    // Set deactivation command for ESP32 to pick up
    const binRef = doc(db, 'bins', binId);
    await updateDoc(binRef, {
      pendingCommand: 'DEACTIVATE_BIN',
      pendingCommandTimestamp: serverTimestamp()
    });

    console.log(`[Deactivate API] Set DEACTIVATE_BIN command for bin ${binId}`);

    return NextResponse.json({
      success: true,
      message: 'Bin deactivation initiated. ESP32 will process session data and award points.',
      data: {
        binId,
        status: 'deactivation_pending',
        commandSet: true
      }
    });

  } catch (error) {
    console.error('[Deactivate API] Error processing deactivation request:', error);
    return NextResponse.json({
      success: false,
      error: 'Failed to initiate bin deactivation',
      details: error instanceof Error ? error.message : 'Unknown error'
    }, { status: 500 });
  }
}

// Handle GET requests
export async function GET() {
  return NextResponse.json({
    message: 'Bin Deactivate Endpoint - Mobile App',
    method: 'POST',
    description: 'Mobile app calls this endpoint to request bin deactivation',
    requiredFields: ['binId', 'userId'],
    optionalFields: ['sessionId'],
    response: {
      success: true,
      message: 'Bin deactivation initiated. ESP32 will process session data.',
      data: {
        binId: 'string',
        status: 'deactivation_pending',
        commandSet: true
      }
    },
    note: 'This sets a command for ESP32 to deactivate and send session data to /api/bins/session-data'
  });
}
