'use client';

import React, { useState, useEffect } from 'react';
import dynamic from 'next/dynamic';
import { Download } from 'lucide-react';
import { AdminService, Bin } from '@/lib/admin-service';

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
      
      // Clear canvas with transparent background
      ctx?.clearRect(0, 0, width, height);
      
      // Draw and compress
      ctx?.drawImage(img, 0, 0, width, height);
      
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

// Download QR code function
const downloadQRCode = (qrCodeUrl: string, binName: string) => {
  const link = document.createElement('a');
  link.download = `bin-${binName.replace(/\s+/g, '-').toLowerCase()}-qr-code.png`;
  link.href = qrCodeUrl;
  link.click();
};

// Dynamically import the map component to avoid SSR issues
const BinMap = dynamic(() => import('@/components/bins/BinMap'), {
  ssr: false,
  loading: () => (
    <div className="h-full w-full bg-gray-100 rounded-xl flex items-center justify-center">
      <div className="animate-spin rounded-full h-8 w-8 border-b-2 border-primary-600"></div>
    </div>
  ),
});

const BinsPage: React.FC = () => {
  const [bins, setBins] = useState<Bin[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [isDialogOpen, setIsDialogOpen] = useState(false);
  const [selectedBin, setSelectedBin] = useState<Bin | null>(null);
  const [isEditing, setIsEditing] = useState(false);
  const [editFormData, setEditFormData] = useState({
    name: '',
    lat: '',
    lng: '',
    level: '',
    image: null as File | null,
    imagePreview: null as string | null
  });
  const [dialogFormData, setDialogFormData] = useState({
    name: '',
    image: null as File | null,
    imagePreview: null as string | null
  });
  const [isDialogSubmitting, setIsDialogSubmitting] = useState(false);

  useEffect(() => {
    const fetchBins = async () => {
      try {
        const adminService = new AdminService();
        const binsList = await adminService.getBins();
        setBins(binsList);
      } catch (err) {
        setError('Failed to fetch bins');
        console.error('Error fetching bins:', err);
      } finally {
        setLoading(false);
      }
    };

    fetchBins();
  }, []);


  const handleBinAdded = async () => {
    // Refresh the bins list
    try {
      const adminService = new AdminService();
      const binsList = await adminService.getBins();
      setBins(binsList);
      setIsDialogOpen(false); // Close dialog after successful bin addition
      // Reset dialog form
      setDialogFormData({
        name: '',
        image: null,
        imagePreview: null
      });
    } catch (err) {
      console.error('Error refreshing bins:', err);
    }
  };

  const handleDialogImageUpload = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (file) {
      setDialogFormData({
        ...dialogFormData,
        image: file
      });
      const reader = new FileReader();
      reader.onload = (e) => {
        setDialogFormData(prev => ({
          ...prev,
          imagePreview: e.target?.result as string
        }));
      };
      reader.readAsDataURL(file);
    }
  };

  const handleDialogSubmit = async () => {
    if (!dialogFormData.name.trim()) {
      alert('Please enter a bin name');
      return;
    }

    if (!dialogFormData.image) {
      alert('Please select an image');
      return;
    }

    setIsDialogSubmitting(true);
    try {
      const adminService = new AdminService();
      
      // Compress and convert image to base64
      const compressedImage = await compressImage(dialogFormData.image);
      const reader = new FileReader();
      reader.onload = async (e) => {
        try {
          const base64Image = e.target?.result as string;
          
          // Create bin data
          const binData = {
            name: dialogFormData.name.trim(),
            image: base64Image,
            qrData: '', // Will be set after QR generation
          };

          // Add bin to Firestore
          const binId = await adminService.addBin(binData);

          // Get the created bin to retrieve the API key
          const createdBin = await adminService.getBinById(binId);

          // Generate QR code
          const QRCode = await import('qrcode');
          const qrData = JSON.stringify({
            binId: binId,
            type: 'bin_activation',
            timestamp: new Date().toISOString(),
          });

          const qrCodeDataURL = await QRCode.toDataURL(qrData, {
            width: 200,
            margin: 2,
            color: {
              dark: '#000000',
              light: '#FFFFFF'
            }
          });

          // Update the bin with QR data and QR code photo
          const { updateDoc, doc } = await import('firebase/firestore');
          const { db } = await import('@/lib/firebase');
          await updateDoc(doc(db, 'bins', binId), {
            qrData: qrData,
            qrCodePhoto: qrCodeDataURL
          });

          // Show API key in alert
          const apiKeyMessage = createdBin?.apiKey 
            ? `\n\n🔑 IoT API KEY:\n${createdBin.apiKey}\n\n⚠️ IMPORTANT: Save this API key now!\nYou'll need it for your IoT device configuration.\nThis key won't be shown again.`
            : '';

          alert(`✅ Bin created successfully!${apiKeyMessage}\n\nQR code has been generated and can be downloaded from the bin details.`);
          handleBinAdded();
        } catch (error) {
          console.error('Error creating bin:', error);
          alert('Failed to create bin');
        } finally {
          setIsDialogSubmitting(false);
        }
      };
      reader.readAsDataURL(compressedImage);
    } catch (error) {
      console.error('Error creating bin:', error);
      alert('Failed to create bin');
      setIsDialogSubmitting(false);
    }
  };

  const handleDialogClear = () => {
    setDialogFormData({
      name: '',
      image: null,
      imagePreview: null
    });
  };

  const handleBinSelect = (bin: Bin) => {
    setSelectedBin(bin);
    setEditFormData({
      name: bin.name,
      lat: bin.lat?.toString() || '',
      lng: bin.lng?.toString() || '',
      level: bin.level?.toString() || '',
      image: null,
      imagePreview: null
    });
    setIsEditing(false);
  };

  const handleEdit = () => {
    setIsEditing(true);
  };

  const handleEditImageUpload = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (file) {
      setEditFormData({
        ...editFormData,
        image: file,
        imagePreview: URL.createObjectURL(file)
      });
    }
  };

  const handleEditImageClear = () => {
    setEditFormData({
      ...editFormData,
      image: null,
      imagePreview: null
    });
  };

  const handleCancelEdit = () => {
    setIsEditing(false);
    if (selectedBin) {
      setEditFormData({
        name: selectedBin.name,
        lat: selectedBin.lat?.toString() || '',
        lng: selectedBin.lng?.toString() || '',
        level: selectedBin.level?.toString() || '',
        image: null,
        imagePreview: null
      });
    }
  };

  const handleSaveEdit = async () => {
    if (!selectedBin) return;

    try {
      const adminService = new AdminService();
      
      // Prepare update data
      const updateData: any = {
        name: editFormData.name,
        lat: parseFloat(editFormData.lat) || 0,
        lng: parseFloat(editFormData.lng) || 0,
        level: parseInt(editFormData.level) || 0
      };

      // Handle image update if a new image was selected
      if (editFormData.image) {
        const compressedImage = await compressImage(editFormData.image);
        const reader = new FileReader();
        
        updateData.image = await new Promise((resolve) => {
          reader.onload = (e) => {
            resolve(e.target?.result as string);
          };
          reader.readAsDataURL(compressedImage);
        });
      }

      await adminService.updateBin(selectedBin.id!, updateData);

      // Refresh bins list
      const binsList = await adminService.getBins();
      setBins(binsList);
      
      // Update selected bin
      const updatedBin = binsList.find(bin => bin.id === selectedBin.id);
      if (updatedBin) {
        setSelectedBin(updatedBin);
      }
      
      setIsEditing(false);
      alert('Bin updated successfully!');
    } catch (error) {
      console.error('Error updating bin:', error);
      alert('Failed to update bin');
    }
  };

  const handleDeleteBin = async (binId: string) => {
    if (!confirm('Are you sure you want to delete this bin?')) {
      return;
    }

    try {
      const adminService = new AdminService();
      await adminService.deleteBin(binId);
      
      setBins(bins.filter(bin => bin.id !== binId));
      setSelectedBin(null);
      alert('Bin deleted successfully');
    } catch (error) {
      console.error('Error deleting bin:', error);
      alert('Failed to delete bin');
    }
  };


  if (loading) {
    return (
      <div className="flex items-center justify-center h-64">
        <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-primary-600"></div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="flex items-center justify-center h-64">
        <div className="text-center">
          <p className="text-red-600 text-lg font-medium">{error}</p>
          <button
            onClick={() => window.location.reload()}
            className="mt-4 px-4 py-2 bg-primary-600 text-white rounded-lg hover:bg-primary-700"
          >
            Retry
          </button>
        </div>
      </div>
    );
  }

  return (
    <div className="flex flex-col space-y-6">
      {/* Header */}
      <div className="flex justify-between items-center">
        <h1 className="text-3xl font-bold text-gray-900">Bin Management</h1>
        <button
          onClick={() => setIsDialogOpen(true)}
          className="px-3 py-2 sm:px-6 sm:py-3 bg-blue-600 text-white rounded-lg hover:bg-blue-700 transition-colors duration-200 flex items-center gap-1 sm:gap-2 shadow-md text-sm sm:text-base"
        >
          <svg className="w-4 h-4 sm:w-5 sm:h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 6v6m0 0v6m0-6h6m-6 0H6" />
          </svg>
          <span className="hidden sm:inline">Register Bin</span>
          <span className="sm:hidden">Add</span>
        </button>
      </div>


      {/* Main Content - Map with Sidebar */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-4 lg:gap-6 h-[calc(100vh-200px)]">
        {/* Map Section */}
        <div className="lg:col-span-2 order-2 lg:order-1">
          <div className="bg-white rounded-xl border border-gray-200 shadow-sm h-64 sm:h-80 lg:h-full">
            <BinMap
              bins={bins}
              onBinSelect={handleBinSelect}
              selectedBin={selectedBin}
            />
          </div>
        </div>

        {/* Bin Information Sidebar */}
        <div className="lg:col-span-1 order-1 lg:order-2">
          <div className="bg-white rounded-xl border border-gray-200 shadow-sm h-64 sm:h-80 lg:h-full overflow-y-auto">
            {selectedBin ? (
              <div className="p-6">
                {/* Header */}
                <div className="flex flex-col sm:flex-row sm:justify-between sm:items-center mb-6 pb-4 border-b border-gray-200 space-y-3 sm:space-y-0">
                  <div>
                    <h3 className="text-lg sm:text-xl font-bold text-gray-900">{selectedBin.name}</h3>
                    <p className="text-sm text-gray-500 mt-1">Bin Details</p>
                  </div>
                  <div className="flex flex-wrap gap-2">
                    {!isEditing ? (
                      <button
                        onClick={handleEdit}
                        className="px-4 py-2 bg-blue-600 text-white rounded-lg text-sm font-medium hover:bg-blue-700 transition-colors shadow-sm"
                      >
                        Edit
                      </button>
                    ) : (
                      <>
                        <button
                          onClick={handleSaveEdit}
                          className="px-4 py-2 bg-green-600 text-white rounded-lg text-sm font-medium hover:bg-green-700 transition-colors shadow-sm"
                        >
                          Save
                        </button>
                        <button
                          onClick={handleCancelEdit}
                          className="px-4 py-2 bg-gray-600 text-white rounded-lg text-sm font-medium hover:bg-gray-700 transition-colors shadow-sm"
                        >
                          Cancel
                        </button>
                      </>
                    )}
                    <button
                      onClick={() => handleDeleteBin(selectedBin.id!)}
                      className="px-4 py-2 bg-red-600 text-white rounded-lg text-sm font-medium hover:bg-red-700 transition-colors shadow-sm"
                    >
                      Delete
                    </button>
                  </div>
                </div>

                {/* Content Grid */}
                <div className="space-y-6">
                  {/* Status Card */}
                  <div className="bg-gray-50 rounded-lg p-4">
                    <div className="flex flex-col space-y-4">
                      {/* Online Status */}
                      <div>
                        <p className="text-sm font-medium text-gray-600 mb-2">Device Status</p>
                        {(() => {
                          const now = Date.now();
                          const lastHeartbeat = selectedBin.lastHeartbeat?.seconds 
                            ? selectedBin.lastHeartbeat.seconds * 1000 
                            : 0;
                          const timeSinceHeartbeat = now - lastHeartbeat;
                          
                          // Check both onlineStatus field AND heartbeat timestamp
                          // Consider online if either:
                          // 1. onlineStatus explicitly set to 'online', OR
                          // 2. heartbeat received within last 60 seconds
                          const isOnlineByStatus = selectedBin.onlineStatus === 'online';
                          const isOnlineByHeartbeat = lastHeartbeat > 0 && timeSinceHeartbeat < 60000;
                          const isOnline = isOnlineByStatus || isOnlineByHeartbeat;
                          
                          return (
                            <div className="flex items-center space-x-2">
                              <span className={`flex items-center px-3 py-1 rounded-full text-sm font-medium ${
                                isOnline 
                                  ? 'bg-green-100 text-green-800' 
                                  : 'bg-red-100 text-red-800'
                              }`}>
                                <span className={`w-2 h-2 rounded-full mr-2 ${
                                  isOnline ? 'bg-green-500 animate-pulse' : 'bg-red-500'
                                }`}></span>
                                {isOnline ? 'Online' : 'Offline'}
                              </span>
                              {selectedBin.lastHeartbeat && (
                                <span className="text-xs text-gray-500">
                                  {isOnlineByHeartbeat 
                                    ? `Active ${Math.floor(timeSinceHeartbeat / 1000)}s ago`
                                    : `Last seen ${Math.floor(timeSinceHeartbeat / 60000)}m ago`
                                  }
                                </span>
                              )}
                            </div>
                          );
                        })()}
                      </div>
                      
                      {/* User Connection Status */}
                      <div>
                        <p className="text-sm font-medium text-gray-600">User Status</p>
                        <span className={`inline-flex px-3 py-1 rounded-full text-sm font-medium ${
                          selectedBin.status === 'active' 
                            ? 'bg-blue-100 text-blue-800' 
                            : 'bg-gray-100 text-gray-800'
                        }`}>
                          {selectedBin.status === 'active' ? 'User Connected' : 'No User'}
                        </span>
                      </div>

                      {/* IoT Capacity Monitoring */}
                      {(selectedBin.comp1Capacity !== undefined || selectedBin.comp2Capacity !== undefined) ? (
                        <div className="space-y-3">
                          <p className="text-sm font-medium text-gray-600">IoT Capacity Monitoring</p>
                          
                          {/* Compartment 1 */}
                          {selectedBin.comp1Capacity !== undefined && (
                            <div className="bg-white rounded-lg p-3 border border-gray-200">
                              <div className="flex justify-between items-center mb-2">
                                <span className="text-sm font-medium text-gray-700">Compartment 1</span>
                                <span className={`text-sm font-bold ${
                                  selectedBin.comp1Capacity >= 80 ? 'text-red-600' :
                                  selectedBin.comp1Capacity >= 60 ? 'text-yellow-600' : 'text-green-600'
                                }`}>
                                  {selectedBin.comp1Capacity}%
                                </span>
                              </div>
                              <div className="w-full bg-gray-200 rounded-full h-2">
                                <div 
                                  className={`h-2 rounded-full transition-all duration-300 ${
                                    selectedBin.comp1Capacity >= 80 ? 'bg-red-500' :
                                    selectedBin.comp1Capacity >= 60 ? 'bg-yellow-500' : 'bg-green-500'
                                  }`}
                                  style={{ width: `${selectedBin.comp1Capacity}%` }}
                                ></div>
                              </div>
                              <p className="text-xs text-gray-500 mt-1">
                                {selectedBin.comp1Capacity >= 80 ? '🚨 Full - Needs emptying' :
                                 selectedBin.comp1Capacity >= 60 ? '⚠️ Moderate - Monitor closely' : '✅ Available'}
                              </p>
                            </div>
                          )}

                          {/* Compartment 2 */}
                          {selectedBin.comp2Capacity !== undefined && (
                            <div className="bg-white rounded-lg p-3 border border-gray-200">
                              <div className="flex justify-between items-center mb-2">
                                <span className="text-sm font-medium text-gray-700">Compartment 2</span>
                                <span className={`text-sm font-bold ${
                                  selectedBin.comp2Capacity >= 80 ? 'text-red-600' :
                                  selectedBin.comp2Capacity >= 60 ? 'text-yellow-600' : 'text-green-600'
                                }`}>
                                  {selectedBin.comp2Capacity}%
                                </span>
                              </div>
                              <div className="w-full bg-gray-200 rounded-full h-2">
                                <div 
                                  className={`h-2 rounded-full transition-all duration-300 ${
                                    selectedBin.comp2Capacity >= 80 ? 'bg-red-500' :
                                    selectedBin.comp2Capacity >= 60 ? 'bg-yellow-500' : 'bg-green-500'
                                  }`}
                                  style={{ width: `${selectedBin.comp2Capacity}%` }}
                                ></div>
                              </div>
                              <p className="text-xs text-gray-500 mt-1">
                                {selectedBin.comp2Capacity >= 80 ? '🚨 Full - Needs emptying' :
                                 selectedBin.comp2Capacity >= 60 ? '⚠️ Moderate - Monitor closely' : '✅ Available'}
                              </p>
                            </div>
                          )}

                          {/* Last Update */}
                          {selectedBin.lastCapacityUpdate && (
                            <p className="text-xs text-gray-500 text-center">
                              Last updated: {new Date(selectedBin.lastCapacityUpdate.seconds * 1000).toLocaleString()}
                            </p>
                          )}
                        </div>
                      ) : (
                        /* Legacy Fill Level */
                        <div>
                          <p className="text-sm font-medium text-gray-600">Fill Level</p>
                          {isEditing ? (
                            <div className="space-y-2">
                              <div className="flex items-center space-x-2">
                                <input
                                  type="number"
                                  min="0"
                                  max="100"
                                  value={editFormData.level}
                                  onChange={(e) => setEditFormData({...editFormData, level: e.target.value})}
                                  className="w-16 px-2 py-1 border border-gray-300 rounded text-sm focus:ring-2 focus:ring-blue-500"
                                />
                                <span className="text-sm text-gray-600">%</span>
                              </div>
                              <div className="flex items-center space-x-2">
                                <div className="w-16 bg-gray-200 rounded-full h-2">
                                  <div 
                                    className={`h-2 rounded-full transition-all duration-300 ${
                                      parseInt(editFormData.level) >= 80 ? 'bg-red-500' :
                                      parseInt(editFormData.level) >= 60 ? 'bg-yellow-500' : 'bg-green-500'
                                    }`}
                                    style={{ width: `${Math.min(100, Math.max(0, parseInt(editFormData.level) || 0))}%` }}
                                  ></div>
                                </div>
                                <span className={`text-xs font-medium ${
                                  parseInt(editFormData.level) >= 80 ? 'text-red-600' :
                                  parseInt(editFormData.level) >= 60 ? 'text-yellow-600' : 'text-green-600'
                                }`}>
                                  {parseInt(editFormData.level) >= 80 ? 'Full' :
                                   parseInt(editFormData.level) >= 60 ? 'Moderate' : 'Empty'}
                                </span>
                              </div>
                            </div>
                          ) : (
                            <div className="flex items-center space-x-2 mt-1">
                              <div className="w-16 bg-gray-200 rounded-full h-2">
                                <div 
                                  className={`h-2 rounded-full ${
                                    (selectedBin.level || 0) >= 80 ? 'bg-red-500' :
                                    (selectedBin.level || 0) >= 60 ? 'bg-yellow-500' : 'bg-green-500'
                                  }`}
                                  style={{ width: `${selectedBin.level || 0}%` }}
                                ></div>
                              </div>
                              <span className={`text-sm font-medium ${
                                (selectedBin.level || 0) >= 80 ? 'text-red-600' :
                                (selectedBin.level || 0) >= 60 ? 'text-yellow-600' : 'text-green-600'
                              }`}>
                                {selectedBin.level || 0}%
                              </span>
                            </div>
                          )}
                        </div>
                      )}
                    </div>
                  </div>

                  {/* Location Card */}
                  <div className="bg-blue-50 rounded-lg p-4">
                    <h4 className="text-sm font-semibold text-blue-900 mb-3">Location</h4>
                    <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
                      <div>
                        <p className="text-xs text-blue-700 mb-1">Latitude</p>
                        {isEditing ? (
                          <input
                            type="number"
                            step="any"
                            value={editFormData.lat}
                            onChange={(e) => setEditFormData({...editFormData, lat: e.target.value})}
                            className="w-full px-3 py-2 border border-blue-200 rounded-lg focus:ring-2 focus:ring-blue-500 text-sm"
                          />
                        ) : (
                          <p className="text-sm font-medium text-blue-900">{selectedBin.lat?.toFixed(6) || 'N/A'}</p>
                        )}
                      </div>
                      <div>
                        <p className="text-xs text-blue-700 mb-1">Longitude</p>
                        {isEditing ? (
                          <input
                            type="number"
                            step="any"
                            value={editFormData.lng}
                            onChange={(e) => setEditFormData({...editFormData, lng: e.target.value})}
                            className="w-full px-3 py-2 border border-blue-200 rounded-lg focus:ring-2 focus:ring-blue-500 text-sm"
                          />
                        ) : (
                          <p className="text-sm font-medium text-blue-900">{selectedBin.lng?.toFixed(6) || 'N/A'}</p>
                        )}
                      </div>
                    </div>
                  </div>

                  {/* Bin Image */}
                  <div className="bg-gray-50 rounded-lg p-4">
                    <h4 className="text-sm font-semibold text-gray-900 mb-3">Bin Photo</h4>
                    <div className="relative">
                      {isEditing ? (
                        <div className="space-y-3">
                          {/* Image Preview */}
                          <div className="relative">
                            <img
                              src={editFormData.imagePreview || selectedBin.image}
                              alt="Bin"
                              className="w-full h-32 sm:h-48 object-contain rounded-lg border border-gray-200 bg-white"
                            />
                          </div>
                          
                          {/* Image Upload Controls */}
                          <div className="flex flex-col space-y-2">
                            <input
                              type="file"
                              accept="image/*"
                              onChange={handleEditImageUpload}
                              className="block w-full text-sm text-gray-500 file:mr-4 file:py-2 file:px-4 file:rounded-lg file:border-0 file:text-sm file:font-semibold file:bg-blue-50 file:text-blue-700 hover:file:bg-blue-100"
                            />
                            {editFormData.image && (
                              <button
                                onClick={handleEditImageClear}
                                className="px-3 py-1 text-sm text-red-600 hover:text-red-800 border border-red-200 rounded-lg hover:bg-red-50"
                              >
                                Remove New Image
                              </button>
                            )}
                          </div>
                        </div>
                      ) : (
                        selectedBin.image && (
                          <img
                            src={selectedBin.image}
                            alt="Bin"
                            className="w-full h-32 sm:h-48 object-contain rounded-lg border border-gray-200 bg-white"
                          />
                        )
                      )}
                    </div>
                  </div>

                  {/* QR Code */}
                  {selectedBin.qrCodePhoto && (
                    <div className="bg-gray-50 rounded-lg p-4">
                      <h4 className="text-sm font-semibold text-gray-900 mb-3">QR Code</h4>
                      <div className="text-center">
                        <div className="inline-block p-3 bg-white rounded-lg border border-gray-200">
                          <img
                            src={selectedBin.qrCodePhoto}
                            alt="QR Code"
                            className="w-24 h-24 sm:w-32 sm:h-32"
                          />
                        </div>
                        <div className="mt-3">
                          <button
                            onClick={() => downloadQRCode(selectedBin.qrCodePhoto!, selectedBin.name)}
                            className="inline-flex items-center space-x-2 px-3 py-2 bg-green-600 text-white text-sm font-medium rounded-lg hover:bg-green-700 transition-colors"
                          >
                            <Download className="w-4 h-4" />
                            <span className="hidden sm:inline">Download QR Code</span>
                            <span className="sm:hidden">Download</span>
                          </button>
                        </div>
                      </div>
                    </div>
                  )}

                  {/* Additional Info */}
                  <div className="bg-gray-50 rounded-lg p-4">
                    <h4 className="text-sm font-semibold text-gray-900 mb-3">Additional Information</h4>
                    <div className="space-y-2">
                      {selectedBin.apiKey && (
                        <div className="mb-3 p-3 bg-blue-50 border border-blue-200 rounded-lg">
                          <div className="flex items-start justify-between mb-2">
                            <span className="text-xs font-semibold text-blue-900">🔑 IoT API Key</span>
                            <button
                              onClick={() => {
                                navigator.clipboard.writeText(selectedBin.apiKey!);
                                alert('API key copied to clipboard!');
                              }}
                              className="text-xs bg-blue-600 text-white px-2 py-1 rounded hover:bg-blue-700"
                            >
                              Copy
                            </button>
                          </div>
                          <code className="text-xs text-blue-800 break-all font-mono block bg-white p-2 rounded border border-blue-200">
                            {selectedBin.apiKey}
                          </code>
                          <p className="text-xs text-blue-600 mt-2">
                            ⚠️ Use this key for IoT device configuration
                          </p>
                        </div>
                      )}
                      <div className="flex justify-between">
                        <span className="text-sm text-gray-600">Created Date</span>
                        <span className="text-sm font-medium text-gray-900">
                          {selectedBin.createdAt 
                            ? new Date(selectedBin.createdAt.seconds * 1000).toLocaleDateString()
                            : 'N/A'
                          }
                        </span>
                      </div>
                      {selectedBin.currentUser && (
                        <div className="flex justify-between">
                          <span className="text-sm text-gray-600">Current User</span>
                          <span className="text-sm font-medium text-gray-900 font-mono">
                            {selectedBin.currentUser.substring(0, 8)}...
                          </span>
                        </div>
                      )}
                    </div>
                  </div>
                </div>
              </div>
            ) : (
              <div className="p-6 flex items-center justify-center h-full">
                <div className="text-center text-gray-500">
                  <svg className="w-12 h-12 mx-auto mb-4 text-gray-300" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M19 11H5m14 0a2 2 0 012 2v6a2 2 0 01-2 2H5a2 2 0 01-2-2v-6a2 2 0 012-2m14 0V9a2 2 0 00-2-2M5 11V9a2 2 0 012-2m0 0V5a2 2 0 012-2h6a2 2 0 012 2v2M7 7h10" />
                  </svg>
                  <p className="text-sm">Click on a bin marker to view details</p>
                </div>
              </div>
            )}
          </div>
        </div>
      </div>

      {/* Register Bin Dialog */}
      {isDialogOpen && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-[9999] p-4">
          <div className="bg-white rounded-xl shadow-xl max-w-md w-full max-h-[90vh] overflow-y-auto relative z-[10000]">
            <div className="flex justify-between items-center p-6 border-b border-gray-200">
              <h2 className="text-xl font-semibold text-gray-900">Register New Bin</h2>
              <button
                onClick={() => setIsDialogOpen(false)}
                className="text-gray-400 hover:text-gray-600 transition-colors"
              >
                <svg className="w-6 h-6" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
                </svg>
              </button>
            </div>
            <div className="p-6">
              <div className="space-y-4">
                {/* Bin Name */}
                <div>
                  <label className="block text-sm font-medium text-gray-700 mb-2">
                    Bin Name *
                  </label>
                  <input
                    type="text"
                    value={dialogFormData.name}
                    onChange={(e) => setDialogFormData({...dialogFormData, name: e.target.value})}
                    placeholder="e.g., Mobod MRF"
                    className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-blue-500"
                  />
                </div>

                {/* Image Upload */}
                <div>
                  <label className="block text-sm font-medium text-gray-700 mb-2">
                    Bin Picture *
                  </label>
                  <div className="border-2 border-dashed border-gray-300 rounded-lg p-6 text-center hover:border-gray-400 transition-colors">
                    <input
                      type="file"
                      accept="image/*"
                      onChange={handleDialogImageUpload}
                      className="hidden"
                      id="image-upload"
                    />
                    <label htmlFor="image-upload" className="cursor-pointer">
                      {dialogFormData.imagePreview ? (
                        <div className="space-y-2">
                          <img
                            src={dialogFormData.imagePreview}
                            alt="Bin preview"
                            className="w-full h-32 object-cover rounded-lg mx-auto"
                          />
                          <p className="text-sm text-gray-600">Click to change image</p>
                        </div>
                      ) : (
                        <div className="space-y-2">
                          <svg className="w-8 h-8 text-gray-400 mx-auto" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M7 16a4 4 0 01-.88-7.903A5 5 0 1115.9 6L16 6a5 5 0 011 9.9M15 13l-3-3m0 0l-3 3m3-3v12" />
                          </svg>
                          <p className="text-sm text-gray-600">Browse Files to upload</p>
                        </div>
                      )}
                    </label>
                  </div>
                </div>

                {/* Action Buttons */}
                <div className="flex space-x-3 pt-4">
                  <button
                    onClick={handleDialogClear}
                    className="flex-1 flex items-center justify-center space-x-2 px-4 py-2 bg-red-600 text-white rounded-lg hover:bg-red-700 transition-colors"
                  >
                    <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M19 7l-.867 12.142A2 2 0 0116.138 21H7.862a2 2 0 01-1.995-1.858L5 7m5 4v6m4-6v6m1-10V4a1 1 0 00-1-1h-4a1 1 0 00-1 1v3M4 7h16" />
                    </svg>
                    <span>Clear</span>
                  </button>
                  <button
                    onClick={handleDialogSubmit}
                    disabled={isDialogSubmitting || !dialogFormData.name.trim() || !dialogFormData.image}
                    className="flex-1 flex items-center justify-center space-x-2 px-4 py-2 bg-green-600 text-white rounded-lg hover:bg-green-700 transition-colors disabled:bg-gray-400 disabled:cursor-not-allowed"
                  >
                    {isDialogSubmitting ? (
                      <>
                        <div className="animate-spin rounded-full h-4 w-4 border-b-2 border-white"></div>
                        <span>Creating...</span>
                      </>
                    ) : (
                      <>
                        <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 4v1m6 11h2m-6 0h-2v4m0-11v3m0 0h.01M12 12h4.01M16 20h4M4 12h4m12 0h.01M5 8h2a1 1 0 001-1V5a1 1 0 00-1-1H5a1 1 0 00-1 1v2a1 1 0 001 1zm12 0h2a1 1 0 001-1V5a1 1 0 00-1-1h-2a1 1 0 00-1 1v2a1 1 0 001 1zM5 20h2a1 1 0 001-1v-2a1 1 0 00-1-1H5a1 1 0 00-1 1v2a1 1 0 001 1z" />
                        </svg>
                        <span>Submit</span>
                      </>
                    )}
                  </button>
                </div>

                {/* Instructions */}
                <div className="mt-4 p-3 bg-blue-50 rounded-lg">
                  <h4 className="text-sm font-medium text-blue-900 mb-1">Instructions:</h4>
                  <ul className="text-xs text-blue-800 space-y-1">
                    <li>• Enter a unique bin name</li>
                    <li>• Upload a clear picture of the bin</li>
                    <li>• Click Submit to generate QR code</li>
                    <li>• Print and attach QR code to the bin</li>
                    <li>• Users scan QR to activate bin for recycling</li>
                  </ul>
                </div>
              </div>
            </div>
          </div>
        </div>
      )}

    </div>
  );
};

export default BinsPage;