import { NextRequest, NextResponse } from 'next/server';
import { AdminService } from '@/lib/admin-service';

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { qrData, userId, userName, userEmail } = body;

    // Validate required fields
    if (!qrData || !userId || !userName) {
      return NextResponse.json(
        { 
          success: false, 
          error: 'Missing required fields: qrData, userId, userName' 
        },
        { status: 400 }
      );
    }

    // Parse QR data
    let parsedQrData;
    try {
      parsedQrData = JSON.parse(qrData);
    } catch (error) {
      return NextResponse.json(
        { 
          success: false, 
          error: 'Invalid QR code format' 
        },
        { status: 400 }
      );
    }

    const { binId, type } = parsedQrData;

    // Validate QR code type
    if (type !== 'bin_activation') {
      return NextResponse.json(
        { 
          success: false, 
          error: 'Invalid QR code type' 
        },
        { status: 400 }
      );
    }

    if (!binId) {
      return NextResponse.json(
        { 
          success: false, 
          error: 'Invalid QR code: missing bin ID' 
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

    // Check if bin is already active with another user
    if (bin.status === 'active' && bin.currentUser && bin.currentUser !== userId) {
      return NextResponse.json(
        { 
          success: false, 
          error: 'Bin is currently in use by another user',
          binStatus: 'occupied',
          binName: bin.name
        },
        { status: 409 } // Conflict
      );
    }

    // Check if user is already using this bin
    if (bin.status === 'active' && bin.currentUser === userId) {
      return NextResponse.json(
        { 
          success: true, 
          message: 'Bin is already activated for you',
          binStatus: 'already_active',
          bin: {
            id: bin.id,
            name: bin.name,
            status: bin.status,
            level: bin.level,
            lat: bin.lat,
            lng: bin.lng
          }
        },
        { status: 200 }
      );
    }

    // Activate bin for this user
    await adminService.activateBin(binId, userId);

    // Log the activity
    const { addDoc, collection, serverTimestamp } = await import('firebase/firestore');
    const { db } = await import('@/lib/firebase');
    
    await addDoc(collection(db, 'userActivities'), {
      userId: userId,
      email: userEmail || userName,
      action: 'Bin Scan',
      description: `Scanned and activated bin: ${bin.name}`,
      binId: binId,
      binName: bin.name,
      date: new Date().toLocaleDateString(),
      time: new Date().toLocaleTimeString(),
      timestamp: serverTimestamp()
    });

    return NextResponse.json({
      success: true,
      message: 'Bin activated successfully',
      binStatus: 'activated',
      bin: {
        id: bin.id,
        name: bin.name,
        status: 'active',
        level: bin.level,
        lat: bin.lat,
        lng: bin.lng,
        currentUser: userId
      }
    });

  } catch (error) {
    console.error('Error scanning QR code:', error);
    return NextResponse.json(
      { 
        success: false, 
        error: 'Internal server error' 
      },
      { status: 500 }
    );
  }
}

// Handle GET requests to check if the endpoint is working
export async function GET() {
  return NextResponse.json({
    message: 'Bin QR Scan Endpoint',
    method: 'POST',
    requiredFields: ['qrData', 'userId', 'userName'],
    optionalFields: ['userEmail']
  });
}
