"""
ESP32-CAM Backend API Test Script
Tests the identify/material endpoint to verify it's working correctly
"""

import requests
import sys
from pathlib import Path

# Configuration
BACKEND_URL = "http://192.168.31.196:5001/identify/material"

def test_api(image_path):
    """Test the material identification API with an image"""
    
    print("=" * 60)
    print("ESP32-CAM Backend API Test")
    print("=" * 60)
    
    # Check if image exists
    if not Path(image_path).exists():
        print(f"❌ Error: Image not found: {image_path}")
        return False
    
    print(f"📁 Image: {image_path}")
    print(f"🌐 API URL: {BACKEND_URL}")
    print("\n⏳ Sending request...")
    
    try:
        # Read image
        with open(image_path, 'rb') as f:
            image_data = f.read()
        
        print(f"📊 Image size: {len(image_data):,} bytes")
        
        # Send POST request
        headers = {'Content-Type': 'image/jpeg'}
        response = requests.post(BACKEND_URL, data=image_data, headers=headers, timeout=15)
        
        print(f"\n📡 Response Status: {response.status_code}")
        
        if response.status_code == 200:
            result = response.json()
            print("\n✅ SUCCESS! Backend Response:")
            print("-" * 60)
            print(f"  Success: {result.get('success')}")
            print(f"  Material Type: {result.get('materialType')}")
            print(f"  Confidence: {result.get('confidence', 0):.2%}")
            print(f"  Action: {result.get('action')}")
            print("-" * 60)
            
            # Show what ESP32-CAM would send
            material_type = result.get('materialType', 'unknown').lower()
            print("\n📤 ESP32-CAM would send to ESP32:")
            if material_type == 'plastic':
                print("  → OPEN_PLASTIC")
            elif material_type in ['tin', 'metal', 'aluminum']:
                print("  → OPEN_TIN")
            else:
                print("  → OPEN_REJECTED")
            
            return True
        else:
            print(f"\n❌ Error: HTTP {response.status_code}")
            print(f"Response: {response.text}")
            return False
            
    except requests.exceptions.ConnectionError:
        print("\n❌ Connection Error: Cannot reach backend server")
        print("   • Check if backend is running (npm run dev)")
        print(f"   • Verify URL: {BACKEND_URL}")
        print("   • Check firewall settings")
        return False
        
    except requests.exceptions.Timeout:
        print("\n❌ Timeout: Backend took too long to respond")
        print("   • Check if AI model is loaded")
        print("   • Verify backend server performance")
        return False
        
    except Exception as e:
        print(f"\n❌ Error: {str(e)}")
        return False

def main():
    print("\n🔍 ESP32-CAM Backend API Tester\n")
    
    # Get image path from command line or use default
    if len(sys.argv) > 1:
        image_path = sys.argv[1]
    else:
        print("Usage: python test_backend_api.py <image_path>")
        print("\nExample:")
        print("  python test_backend_api.py test_plastic.jpg")
        print("  python test_backend_api.py C:\\Users\\Photos\\bottle.jpg")
        return
    
    # Test the API
    success = test_api(image_path)
    
    print("\n" + "=" * 60)
    if success:
        print("✅ Test completed successfully!")
        print("   Your backend API is working correctly.")
        print("   ESP32-CAM should be able to communicate with it.")
    else:
        print("❌ Test failed!")
        print("   Fix the issues above before uploading to ESP32-CAM.")
    print("=" * 60 + "\n")

if __name__ == "__main__":
    main()
