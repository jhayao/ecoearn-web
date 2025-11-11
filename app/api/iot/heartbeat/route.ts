import { NextRequest, NextResponse } from 'next/server';
import { AdminService } from '@/lib/admin-service';
import { doc, updateDoc, serverTimestamp } from 'firebase/firestore';
import { db } from '@/lib/firebase';

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { apiKey, status } = body;

    // Validate required fields
    if (!apiKey) {
      return NextResponse.json(
        { 
          success: false, 
          error: 'Missing required field: apiKey' 
        },
        { status: 400 }
      );
    }

    const adminService = new AdminService();

    // Verify API key and get bin ID
    const binId = await adminService.verifyApiKey(apiKey);
    
    if (!binId) {
      return NextResponse.json(
        { 
          success: false, 
          error: 'Invalid API key' 
        },
        { status: 401 }
      );
    }

    // Update bin with heartbeat timestamp and online status
    const binRef = doc(db, 'bins', binId);
    await updateDoc(binRef, {
      lastHeartbeat: serverTimestamp(),
      onlineStatus: 'online',
      deviceStatus: status || 'active', // Optional: 'active', 'idle', 'error', etc.
    });

    return NextResponse.json({
      success: true,
      message: 'Heartbeat received',
      binId: binId,
      timestamp: new Date().toISOString()
    });

  } catch (error) {
    console.error('Error processing heartbeat:', error);
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
    message: 'IoT Heartbeat Endpoint',
    method: 'POST',
    requiredFields: ['apiKey'],
    optionalFields: ['status'],
    description: 'Send heartbeat from IoT device to update online status'
  });
}
