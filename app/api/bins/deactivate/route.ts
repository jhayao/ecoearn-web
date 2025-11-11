import { NextRequest, NextResponse } from 'next/server';
import { AdminService } from '@/lib/admin-service';

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { binId, userId, userEmail } = body;

    // Validate required fields
    if (!binId || !userId) {
      return NextResponse.json(
        { 
          success: false, 
          error: 'Missing required fields: binId, userId' 
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
          error: 'Bin not found' 
        },
        { status: 404 }
      );
    }

    // Check if the user is authorized to deactivate this bin
    if (bin.currentUser !== userId) {
      return NextResponse.json(
        { 
          success: false, 
          error: 'You are not authorized to deactivate this bin',
          currentUser: bin.currentUser
        },
        { status: 403 } // Forbidden
      );
    }

    // Deactivate bin
    console.log(`[Deactivate API] User ${userId} disconnecting from bin ${binId}`);
    await adminService.deactivateBinForUser(binId, userId);
    console.log(`[Deactivate API] DEACTIVATE_BIN command queued for ESP32`);

    // Log the activity
    const { addDoc, collection, serverTimestamp } = await import('firebase/firestore');
    const { db } = await import('@/lib/firebase');
    
    await addDoc(collection(db, 'userActivities'), {
      userId: userId,
      email: userEmail || userId,
      action: 'Bin Deactivate',
      description: `Deactivated bin: ${bin.name}`,
      binId: binId,
      binName: bin.name,
      date: new Date().toLocaleDateString(),
      time: new Date().toLocaleTimeString(),
      timestamp: serverTimestamp()
    });

    return NextResponse.json({
      success: true,
      message: 'Bin deactivated successfully',
      bin: {
        id: bin.id,
        name: bin.name,
        status: 'inactive'
      }
    });

  } catch (error) {
    console.error('Error deactivating bin:', error);
    return NextResponse.json(
      { 
        success: false, 
        error: 'Internal server error' 
      },
      { status: 500 }
    );
  }
}

// Handle GET requests
export async function GET() {
  return NextResponse.json({
    message: 'Bin Deactivate Endpoint',
    method: 'POST',
    requiredFields: ['binId', 'userId'],
    optionalFields: ['userEmail']
  });
}
