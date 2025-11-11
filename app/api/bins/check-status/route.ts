import { NextRequest, NextResponse } from 'next/server';
import { AdminService } from '@/lib/admin-service';

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { binId, userId } = body;

    // Validate required fields
    if (!binId) {
      return NextResponse.json(
        { 
          success: false, 
          error: 'Missing required field: binId' 
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

    // Determine bin availability for the user
    let availability = 'available';
    let canUse = true;

    if (bin.status === 'active') {
      if (bin.currentUser === userId) {
        availability = 'active_by_you';
        canUse = true;
      } else if (bin.currentUser) {
        availability = 'occupied';
        canUse = false;
      }
    }

    return NextResponse.json({
      success: true,
      bin: {
        id: bin.id,
        name: bin.name,
        status: bin.status,
        level: bin.level,
        lat: bin.lat,
        lng: bin.lng,
        currentUser: bin.currentUser,
        availability: availability,
        canUse: canUse
      }
    });

  } catch (error) {
    console.error('Error checking bin status:', error);
    return NextResponse.json(
      { 
        success: false, 
        error: 'Internal server error' 
      },
      { status: 500 }
    );
  }
}

// Handle GET requests with query parameters
export async function GET(request: NextRequest) {
  try {
    const searchParams = request.nextUrl.searchParams;
    const binId = searchParams.get('binId');

    if (!binId) {
      return NextResponse.json({
        message: 'Bin Status Check Endpoint',
        method: 'POST or GET with query parameter',
        requiredFields: ['binId'],
        optionalFields: ['userId']
      });
    }

    const adminService = new AdminService();
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

    return NextResponse.json({
      success: true,
      bin: {
        id: bin.id,
        name: bin.name,
        status: bin.status,
        level: bin.level,
        currentUser: bin.currentUser
      }
    });

  } catch (error) {
    console.error('Error checking bin status:', error);
    return NextResponse.json(
      { 
        success: false, 
        error: 'Internal server error' 
      },
      { status: 500 }
    );
  }
}
