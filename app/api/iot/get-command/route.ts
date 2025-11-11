import { NextRequest, NextResponse } from 'next/server';
import { AdminService } from '@/lib/admin-service';
import { doc, updateDoc, deleteField } from 'firebase/firestore';
import { db } from '@/lib/firebase';

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { apiKey } = body;

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

    // Get bin details to check for pending command
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

    // Check if there's a pending command
    const pendingCommand = (bin as any).pendingCommand;
    
    if (pendingCommand) {
      console.log(`[IoT] Sending command to bin ${binId}: ${pendingCommand}`);
      
      // Clear the pending command from Firestore
      const binRef = doc(db, 'bins', binId);
      await updateDoc(binRef, {
        pendingCommand: deleteField(),
        pendingCommandTimestamp: deleteField()
      });
      
      return NextResponse.json({
        success: true,
        hasCommand: true,
        command: pendingCommand,
        timestamp: new Date().toISOString()
      });
    }

    return NextResponse.json({
      success: true,
      hasCommand: false,
      message: 'No pending commands',
      timestamp: new Date().toISOString()
    });

  } catch (error) {
    console.error('Error getting IoT command:', error);
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
    message: 'IoT Get Command Endpoint',
    method: 'POST',
    requiredFields: ['apiKey'],
    description: 'ESP32 polls this endpoint to get pending commands (like ACTIVATE_BIN)'
  });
}
