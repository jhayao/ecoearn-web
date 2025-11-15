import { NextRequest, NextResponse } from 'next/server';
import { AdminService } from '@/lib/admin-service';

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const {
      binId,
      userId,
      materialType,
      weight,
      quantity,
      location,
      timestamp,
      sessionId,
      deviceData
    } = body;

    // Validate required fields
    if (!binId || !userId || !materialType) {
      return NextResponse.json(
        {
          success: false,
          error: 'Missing required fields: binId, userId, materialType'
        },
        { status: 400 }
      );
    }

    // Validate material type
    const validMaterials = ['plastic', 'tin', 'rejected'];
    if (!validMaterials.includes(materialType)) {
      return NextResponse.json(
        {
          success: false,
          error: 'Invalid material type. Must be: plastic, tin, or rejected'
        },
        { status: 400 }
      );
    }

    const adminService = new AdminService();

    // Verify bin exists and is active
    const bin = await adminService.getBinById(binId);
    if (!bin) {
      return NextResponse.json(
        { success: false, error: 'Bin not found' },
        { status: 404 }
      );
    }

    if (bin.status !== 'active') {
      return NextResponse.json(
        { success: false, error: 'Bin is not active' },
        { status: 400 }
      );
    }

    // Verify user is authorized for this bin
    if (bin.currentUser !== userId) {
      return NextResponse.json(
        {
          success: false,
          error: 'User is not authorized for this bin',
          currentUser: bin.currentUser
        },
        { status: 403 }
      );
    }

    // Get current pricing configuration
    const pricing = await adminService.getCurrentPricing();
    if (!pricing) {
      return NextResponse.json(
        {
          success: false,
          error: 'Pricing configuration not found'
        },
        { status: 500 }
      );
    }

    // Calculate points based on material type and current pricing
    let points = 0;
    if (materialType === 'plastic') {
      // plastic = items per point (e.g., 50 bottles = 1 point)
      points = Math.floor(quantity / pricing.plastic);
    } else if (materialType === 'tin') {
      // glass/tin = items per point (e.g., 10 cans = 1 point)
      points = Math.floor(quantity / pricing.glass);
    }
    // Rejected items get 0 points

    console.log(`[Recycle API] Processing ${materialType} transaction: ${quantity} items, ${points} points for user ${userId}`);

    // Add points to user if any earned
    if (points > 0) {
      await adminService.addUserPoints(userId, points);
      console.log(`[Recycle API] Added ${points} points to user ${userId}`);
    }

    // Log the recycling activity
    await adminService.logRecyclingActivity({
      userId,
      userEmail: userId, // ESP32 doesn't send email, so use userId
      materialType,
      quantity: quantity || 1,
      weight: weight || 0,
      pointsEarned: points,
      binId: bin.id!,
      binName: bin.name
    });

    // Update bin compartment fill levels if deviceData provided
    if (deviceData && deviceData.fillLevel !== undefined) {
      // Note: Compartment updates should be sent via /iot/update-capacity endpoint
      console.log(`[Recycle API] Compartment ${materialType} fill level: ${deviceData.fillLevel}% (use /iot/update-capacity to update)`);
    }

    return NextResponse.json({
      success: true,
      message: 'Transaction recorded successfully',
      data: {
        transactionId: `txn_${Date.now()}`,
        points: points,
        materialType,
        quantity: quantity || 1,
        weight: weight || 0
      },
      timestamp: new Date().toISOString()
    }, { status: 201 });

  } catch (error) {
    console.error('Error processing recycle transaction:', error);
    return NextResponse.json(
      { success: false, error: 'Internal server error' },
      { status: 500 }
    );
  }
}

// Handle GET requests for documentation
export async function GET() {
  const adminService = new AdminService();
  const pricing = await adminService.getCurrentPricing();

  return NextResponse.json({
    message: 'IoT Recycle Transaction Endpoint',
    method: 'POST',
    description: 'Records individual recycling transactions from ESP32 microcontroller',
    requiredFields: ['binId', 'userId', 'materialType'],
    optionalFields: ['weight', 'quantity', 'location', 'timestamp', 'sessionId', 'deviceData'],
    materialTypes: ['plastic', 'tin', 'rejected'],
    pointsCalculation: {
      plastic: pricing ? `Dynamic: ${pricing.plastic} bottles = 1 point` : 'Dynamic pricing',
      tin: pricing ? `Dynamic: ${pricing.glass} cans = 1 point` : 'Dynamic pricing',
      rejected: '0 points'
    },
    deviceData: {
      compartment: 'plastic|tin|rejected',
      fillLevel: '0-100',
      temperature: 'number',
      humidity: 'number'
    }
  });
}