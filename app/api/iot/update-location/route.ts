import { NextRequest, NextResponse } from 'next/server';
import { AdminService } from '@/lib/admin-service';

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { apiKey, latitude, longitude } = body;

    // Validate required fields
    if (!apiKey || latitude === undefined || longitude === undefined) {
      return NextResponse.json(
        { 
          success: false, 
          error: 'Missing required fields: apiKey, latitude, longitude' 
        },
        { status: 400 }
      );
    }

    // Validate latitude and longitude ranges
    if (latitude < -90 || latitude > 90 || longitude < -180 || longitude > 180) {
      return NextResponse.json(
        { 
          success: false, 
          error: 'Invalid latitude or longitude values' 
        },
        { status: 400 }
      );
    }

    const adminService = new AdminService();
    const success = await adminService.updateBinLocation(
      apiKey,
      parseFloat(latitude),
      parseFloat(longitude)
    );

    if (success) {
      return NextResponse.json({
        success: true,
        message: 'Bin location updated successfully',
        timestamp: new Date().toISOString()
      });
    } else {
      return NextResponse.json(
        { 
          success: false, 
          error: 'Invalid API key or failed to update location' 
        },
        { status: 401 }
      );
    }
  } catch (error) {
    console.error('Error updating bin location:', error);
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
    message: 'IoT Update Location Endpoint',
    method: 'POST',
    requiredFields: ['apiKey', 'latitude', 'longitude']
  });
}
