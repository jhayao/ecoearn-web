import { NextRequest, NextResponse } from 'next/server';
import { AdminService } from '@/lib/admin-service';

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { apiKey, comp1Capacity, comp2Capacity } = body;

    // Validate required fields
    if (!apiKey) {
      return NextResponse.json(
        { error: 'API key is required' },
        { status: 400 }
      );
    }

    if (typeof comp1Capacity !== 'number' || typeof comp2Capacity !== 'number') {
      return NextResponse.json(
        { error: 'Capacity values must be numbers' },
        { status: 400 }
      );
    }

    // Validate capacity ranges
    if (comp1Capacity < 0 || comp1Capacity > 100 || comp2Capacity < 0 || comp2Capacity > 100) {
      return NextResponse.json(
        { error: 'Capacity values must be between 0 and 100' },
        { status: 400 }
      );
    }

    const adminService = new AdminService();

    // Verify API key and update bin capacity
    const result = await adminService.updateBinCapacity(apiKey, comp1Capacity, comp2Capacity);

    if (!result.success) {
      return NextResponse.json(
        { error: result.error },
        { status: 404 }
      );
    }

    return NextResponse.json({
      success: true,
      message: 'Bin capacity updated successfully',
      binId: result.binId,
      comp1Capacity: comp1Capacity,
      comp2Capacity: comp2Capacity
    });

  } catch (error) {
    console.error('Error updating bin capacity:', error);
    return NextResponse.json(
      { error: 'Internal server error' },
      { status: 500 }
    );
  }
}