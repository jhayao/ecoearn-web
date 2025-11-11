'use client';

import React, { useState } from 'react';
import { Upload, QrCode, Trash2, Check, Copy } from 'lucide-react';
import { AdminService } from '@/lib/admin-service';

// Image compression function
const compressImage = (file: File, maxWidth: number = 800, quality: number = 0.7): Promise<File> => {
  return new Promise((resolve) => {
    const canvas = document.createElement('canvas');
    const ctx = canvas.getContext('2d');
    const img = new Image();
    
    img.onload = () => {
      // Calculate new dimensions
      let { width, height } = img;
      if (width > maxWidth) {
        height = (height * maxWidth) / width;
        width = maxWidth;
      }
      
      // Set canvas dimensions
      canvas.width = width;
      canvas.height = height;
      
      // Draw and compress
      ctx?.drawImage(img, 0, 0, width, height);
      
      // Clear canvas with transparent background
      ctx?.clearRect(0, 0, width, height);
      
      canvas.toBlob((blob) => {
        if (blob) {
          const compressedFile = new File([blob], file.name, {
            type: 'image/png', // Use PNG to preserve transparency
            lastModified: Date.now(),
          });
          resolve(compressedFile);
        } else {
          resolve(file); // Fallback to original file
        }
      }, 'image/png', quality);
    };
    
    img.src = URL.createObjectURL(file);
  });
};

interface BinFormProps {
  onBinAdded: () => void;
}

const BinForm: React.FC<BinFormProps> = ({ onBinAdded }) => {
  const [formData, setFormData] = useState({
    name: '',
    latitude: '',
    longitude: '',
    level: '',
  });
  const [selectedImage, setSelectedImage] = useState<File | null>(null);
  const [imagePreview, setImagePreview] = useState<string | null>(null);
  const [qrData, setQrData] = useState<string | null>(null);
  const [apiKey, setApiKey] = useState<string | null>(null);
  const [isUploading, setIsUploading] = useState(false);
  const [isGeneratingQR, setIsGeneratingQR] = useState(false);
  const [apiKeyCopied, setApiKeyCopied] = useState(false);

  const handleInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const { name, value } = e.target;
    setFormData(prev => ({
      ...prev,
      [name]: value
    }));
  };

  const handleImageUpload = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (file) {
      setSelectedImage(file);
      const reader = new FileReader();
      reader.onload = (e) => {
        setImagePreview(e.target?.result as string);
      };
      reader.readAsDataURL(file);
    }
  };

  const generateQRCode = async () => {
    if (!formData.name || !formData.latitude || !formData.longitude || !formData.level) {
      alert('Please fill all fields before generating QR code');
      return;
    }

    setIsGeneratingQR(true);
    try {
      const binData = {
        binId: Date.now().toString(),
        name: formData.name,
        latitude: parseFloat(formData.latitude),
        longitude: parseFloat(formData.longitude),
        fillLevel: parseInt(formData.level),
        timestamp: new Date().toISOString(),
      };

      setQrData(JSON.stringify(binData));
    } catch (error) {
      console.error('Error generating QR code:', error);
    } finally {
      setIsGeneratingQR(false);
    }
  };

  const handleSubmit = async () => {
    if (!formData.name || !formData.latitude || !formData.longitude || !formData.level || !selectedImage || !qrData) {
      alert('Please fill all fields, select an image, and generate a QR code');
      return;
    }

    setIsUploading(true);
    try {
      const adminService = new AdminService();
      
      // Compress and convert image to base64
      const compressedImage = await compressImage(selectedImage);
      const reader = new FileReader();
      reader.onload = async (e) => {
        const base64Image = e.target?.result as string;
        const base64Data = base64Image.split(',')[1]; // Remove data:image/jpeg;base64, prefix

        const binId = await adminService.addBin({
          name: formData.name,
          lat: parseFloat(formData.latitude),
          lng: parseFloat(formData.longitude),
          level: parseInt(formData.level),
          image: base64Data,
          qrData: qrData,
        });

        // Get the created bin to retrieve the API key
        const createdBin = await adminService.getBinById(binId);
        if (createdBin?.apiKey) {
          setApiKey(createdBin.apiKey);
          // Don't reset form immediately - let user see and copy API key
          alert(`Bin added successfully!\n\nAPI Key: ${createdBin.apiKey}\n\nPlease save this API key for your IoT device.\n\nThe API key will remain visible below until you clear the form.`);
        } else {
          alert('Bin added successfully!');
        }

        // Only reset these fields, keep apiKey visible
        setFormData({ name: '', latitude: '', longitude: '', level: '' });
        setSelectedImage(null);
        setImagePreview(null);
        setQrData(null);
        
        onBinAdded();
      };
      reader.readAsDataURL(compressedImage);
    } catch (error) {
      console.error('Error adding bin:', error);
      alert('Failed to add bin');
    } finally {
      setIsUploading(false);
    }
  };

  const handleClear = () => {
    setFormData({ name: '', latitude: '', longitude: '', level: '' });
    setSelectedImage(null);
    setImagePreview(null);
    setQrData(null);
    setApiKey(null);
    setApiKeyCopied(false);
  };

  const handleCopyApiKey = () => {
    if (apiKey) {
      navigator.clipboard.writeText(apiKey);
      setApiKeyCopied(true);
      setTimeout(() => setApiKeyCopied(false), 2000);
    }
  };

  return (
    <div className="bg-white p-6 rounded-xl shadow-sm border border-gray-200">
      <h3 className="text-lg font-semibold text-gray-900 mb-4">Bin Information</h3>
      
      <div className="space-y-4">
        {/* Bin Name */}
        <div>
          <label className="block text-sm font-medium text-gray-700 mb-2">
            Bin Name
          </label>
          <input
            type="text"
            name="name"
            value={formData.name}
            onChange={handleInputChange}
            placeholder="e.g., Mobod MBF"
            className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-primary-500 focus:border-primary-500"
          />
        </div>

        {/* Latitude */}
        <div>
          <label className="block text-sm font-medium text-gray-700 mb-2">
            Latitude
          </label>
          <input
            type="number"
            step="any"
            name="latitude"
            value={formData.latitude}
            onChange={handleInputChange}
            placeholder="e.g., 8.476876"
            className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-primary-500 focus:border-primary-500"
          />
        </div>

        {/* Longitude */}
        <div>
          <label className="block text-sm font-medium text-gray-700 mb-2">
            Longitude
          </label>
          <input
            type="number"
            step="any"
            name="longitude"
            value={formData.longitude}
            onChange={handleInputChange}
            placeholder="e.g., 123.799913"
            className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-primary-500 focus:border-primary-500"
          />
        </div>

        {/* Bin Level */}
        <div>
          <label className="block text-sm font-medium text-gray-700 mb-2">
            Bin Level (%)
          </label>
          <input
            type="number"
            min="0"
            max="100"
            name="level"
            value={formData.level}
            onChange={handleInputChange}
            placeholder="e.g., 86"
            className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-primary-500 focus:border-primary-500"
          />
        </div>

        {/* Image Upload */}
        <div>
          <label className="block text-sm font-medium text-gray-700 mb-2">
            Bin Photo
          </label>
          <div
            onClick={() => document.getElementById('image-upload')?.click()}
            className="w-full h-24 border-2 border-dashed border-gray-300 rounded-lg flex items-center justify-center cursor-pointer hover:border-primary-500 transition-colors"
          >
            {imagePreview ? (
              <img
                src={imagePreview}
                alt="Preview"
                className="w-full h-full object-cover rounded-lg"
              />
            ) : (
              <div className="text-center">
                <Upload className="w-8 h-8 text-gray-400 mx-auto mb-2" />
                <p className="text-sm text-gray-500">Upload Bin Photo</p>
              </div>
            )}
          </div>
          <input
            id="image-upload"
            type="file"
            accept="image/*"
            onChange={handleImageUpload}
            className="hidden"
          />
        </div>

        {/* QR Code Generation */}
        <div>
          <button
            onClick={generateQRCode}
            disabled={isGeneratingQR}
            className="w-full flex items-center justify-center space-x-2 px-4 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 disabled:opacity-50 disabled:cursor-not-allowed"
          >
            <QrCode className="w-4 h-4" />
            <span>{isGeneratingQR ? 'Generating...' : 'Generate QR Code'}</span>
          </button>
          {qrData && (
            <div className="mt-2 p-2 bg-green-50 border border-green-200 rounded-lg">
              <p className="text-sm text-green-800">QR Code generated successfully!</p>
            </div>
          )}
        </div>

        {/* API Key Display */}
        {apiKey && (
          <div className="p-4 bg-blue-50 border border-blue-200 rounded-lg">
            <h4 className="text-sm font-semibold text-blue-900 mb-2">🔑 IoT Device API Key</h4>
            <div className="flex items-center gap-2">
              <div className="flex-1 bg-white p-3 rounded border border-blue-300">
                <code className="text-xs text-blue-800 break-all font-mono">{apiKey}</code>
              </div>
              <button
                onClick={handleCopyApiKey}
                className="p-3 bg-blue-600 text-white rounded hover:bg-blue-700 transition-colors flex items-center gap-1"
                title="Copy API Key"
              >
                {apiKeyCopied ? (
                  <>
                    <Check className="w-4 h-4" />
                    <span className="text-xs">Copied!</span>
                  </>
                ) : (
                  <>
                    <Copy className="w-4 h-4" />
                    <span className="text-xs">Copy</span>
                  </>
                )}
              </button>
            </div>
            <p className="text-xs text-blue-600 mt-2">
              ⚠️ <strong>Important:</strong> Save this API key now! You won't be able to retrieve it later. Use it in your IoT device configuration.
            </p>
          </div>
        )}

        {/* Action Buttons */}
        <div className="flex space-x-3">
          <button
            onClick={handleClear}
            className="flex-1 flex items-center justify-center space-x-2 px-4 py-2 bg-red-500 text-white rounded-lg hover:bg-red-600"
          >
            <Trash2 className="w-4 h-4" />
            <span>Cancel</span>
          </button>
          <button
            onClick={handleSubmit}
            disabled={isUploading || !qrData}
            className="flex-1 flex items-center justify-center space-x-2 px-4 py-2 bg-green-600 text-white rounded-lg hover:bg-green-700 disabled:opacity-50 disabled:cursor-not-allowed"
          >
            <Check className="w-4 h-4" />
            <span>{isUploading ? 'Submitting...' : 'Submit'}</span>
          </button>
        </div>
      </div>
    </div>
  );
};

export default BinForm;
