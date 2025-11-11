import { NextRequest, NextResponse } from 'next/server';
import { AdminService } from '@/lib/admin-service';
import { addDoc, collection, serverTimestamp, doc, updateDoc } from 'firebase/firestore';
import { db } from '@/lib/firebase';

// Generate a unique session ID
function generateSessionId(): string {
  return `session_${Date.now()}_${Math.random().toString(36).substring(2, 15)}`;
}

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { binId, userId, scannedAt, location } = body;

    // Validate required fields
    if (!binId || !userId) {
      return NextResponse.json(
        { 
          success: false, 
          error: 'Missing required fields: binId, userId',
          code: 'INVALID_REQUEST'
        },
        { status: 400 }
      );
    }

    const adminService = new AdminService();

    // Get bin details
    const bin = await adminService.getBinById(binId);
    
    if (!bin) {
      return NextResponse.json(
        { 
          success: false, 
          error: 'Bin not found',
          code: 'BIN_NOT_FOUND'
        },
        { status: 404 }
      );
    }

    // Check if bin is already active with another user
    if (bin.status === 'active' && bin.currentUser && bin.currentUser !== userId) {
      return NextResponse.json(
        { 
          success: false, 
          error: 'Bin is currently in use by another user',
          code: 'BIN_ALREADY_ACTIVE',
          data: {
            currentUser: bin.currentUser,
            estimatedAvailableAt: new Date(Date.now() + 5 * 60 * 1000).toISOString() // 5 minutes estimate
          }
        },
        { status: 409 } // Conflict
      );
    }

    // Check if user is already using this bin
    if (bin.status === 'active' && bin.currentUser === userId) {
      // Still send the activation command to ESP32 even if already active
      const binRef = doc(db, 'bins', binId);
      const existingSessionId = `existing_session_${bin.id}`;
      
      await updateDoc(binRef, {
        lastHeartbeat: serverTimestamp(),
        onlineStatus: 'online',
        deviceStatus: 'active',
        // Store pending command for ESP32 to retrieve
        pendingCommand: `ACTIVATE_BIN:${userId}:${existingSessionId}`,
        pendingCommandTimestamp: serverTimestamp()
      });
      
      return NextResponse.json(
        { 
          success: true, 
          message: 'Bin is already activated for you',
          data: {
            binId: bin.id,
            name: bin.name,
            status: 'active',
            currentUser: userId,
            activatedAt: scannedAt || new Date().toISOString(),
            sessionId: existingSessionId,
            expiresAt: new Date(Date.now() + 5 * 60 * 1000).toISOString()
          }
        },
        { status: 200 }
      );
    }

    // Generate session ID and calculate expiry
    const sessionId = generateSessionId();
    const activatedAt = scannedAt || new Date().toISOString();
    const expiresAt = new Date(Date.now() + 5 * 60 * 1000).toISOString(); // 5 minutes from now

    // Activate bin for this user
    await adminService.activateBin(binId, userId);

    // Send initial heartbeat (for testing without physical device)
    const binRef = doc(db, 'bins', binId);
    await updateDoc(binRef, {
      lastHeartbeat: serverTimestamp(),
      onlineStatus: 'online',
      deviceStatus: 'active',
      // Store pending command for ESP32 to retrieve
      pendingCommand: `ACTIVATE_BIN:${userId}:${sessionId}`,
      pendingCommandTimestamp: serverTimestamp()
    });

    // Log the activation activity with location
    await addDoc(collection(db, 'userActivities'), {
      userId: userId,
      action: 'Bin Activated',
      description: `Activated bin: ${bin.name}`,
      binId: binId,
      binName: bin.name,
      sessionId: sessionId,
      location: location || null,
      date: new Date().toLocaleDateString(),
      time: new Date().toLocaleTimeString(),
      timestamp: serverTimestamp()
    });

    // Store session information in Firestore
    await addDoc(collection(db, 'binSessions'), {
      sessionId: sessionId,
      binId: binId,
      binName: bin.name,
      userId: userId,
      activatedAt: new Date(activatedAt),
      expiresAt: new Date(expiresAt),
      location: location || null,
      status: 'active',
      createdAt: serverTimestamp()
    });

    return NextResponse.json({
      success: true,
      message: 'Bin activated successfully',
      data: {
        binId: bin.id,
        name: bin.name,
        status: 'active',
        currentUser: userId,
        activatedAt: activatedAt,
        sessionId: sessionId,
        expiresAt: expiresAt
      }
    });

  } catch (error) {
    console.error('Error activating bin:', error);
    return NextResponse.json(
      { 
        success: false, 
        error: 'Internal server error',
        code: 'SERVER_ERROR'
      },
      { status: 500 }
    );
  }
}

// Handle GET requests to check if the endpoint is working
export async function GET() {
  return NextResponse.json({
    message: 'Bin Activation Endpoint',
    method: 'POST',
    requiredFields: ['binId', 'userId'],
    optionalFields: ['scannedAt', 'location'],
    description: 'Activate a bin for a user and generate a session ID'
  });
}
