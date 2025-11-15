'use client';

import React, { useState, useEffect } from 'react';
import { Edit2, Check, X } from 'lucide-react';
import { AdminService } from '@/lib/admin-service';

interface MaterialCardProps {
  title: string;
  points: string;
  additionalText: string;
  color: string;
  onSave: (newPrice: string) => void;
}

const MaterialCard: React.FC<MaterialCardProps> = ({
  title,
  points,
  additionalText,
  color,
  onSave
}) => {
  const [isEditing, setIsEditing] = useState(false);
  const [editValue, setEditValue] = useState(title.replace(' pts', ''));

  const handleSave = () => {
    onSave(editValue); // Don't add ' pts' - admin service handles it
    setIsEditing(false);
  };

  const handleCancel = () => {
    setEditValue(title.replace(' pts', ''));
    setIsEditing(false);
  };

  // Determine the label and value display based on material type
  const getLabel = () => {
    if (additionalText === 'Plastic Bottle') return 'Bottles per Point';
    if (additionalText === 'Tin Can') return 'Cans per Point';
    return 'Items per Point';
  };

  const getDisplayValue = () => {
    const numValue = parseFloat(title.replace(' pts', ''));
    return numValue;
  };

  return (
    <div className="bg-gradient-to-br from-white to-gray-50 border border-gray-200 rounded-lg sm:rounded-xl p-3 sm:p-4 shadow-sm hover:shadow-lg transition-all duration-200 h-auto sm:h-full flex flex-col group">
      <div className="flex items-center justify-between mb-2 sm:mb-3">
        <div className="flex items-center space-x-1.5 sm:space-x-2 min-w-0 flex-1">
          <div className={`w-2.5 h-2.5 sm:w-3 sm:h-3 rounded-full flex-shrink-0 ${additionalText === 'Plastic Bottle' ? 'bg-blue-500' : 'bg-orange-500'}`}></div>
          <h4 className="text-sm sm:text-base lg:text-lg font-semibold text-gray-900 truncate">{additionalText}</h4>
        </div>
        <div className="flex items-center space-x-1 flex-shrink-0">
          <button
            onClick={isEditing ? handleSave : () => setIsEditing(true)}
            className="p-1 sm:p-1.5 hover:bg-green-100 rounded-lg transition-colors group-hover:bg-green-50"
            title={isEditing ? "Save changes" : "Edit pricing"}
          >
            {isEditing ? (
              <Check className="w-3 h-3 sm:w-4 sm:h-4 text-green-600" />
            ) : (
              <Edit2 className="w-3 h-3 sm:w-4 sm:h-4 text-gray-500 group-hover:text-green-600" />
            )}
          </button>
          {isEditing && (
            <button
              onClick={handleCancel}
              className="p-1 sm:p-1.5 hover:bg-red-100 rounded-lg transition-colors"
              title="Cancel editing"
            >
              <X className="w-3 h-3 sm:w-4 sm:h-4 text-red-500" />
            </button>
          )}
        </div>
      </div>
      
      <div className="space-y-2 sm:space-y-3 flex-1">
        <div>
          <label className="block text-xs font-medium text-gray-500 uppercase tracking-wide mb-1">{getLabel()}</label>
          {isEditing ? (
            <input
              type="text"
              value={editValue}
              onChange={(e) => setEditValue(e.target.value)}
              className="w-full px-2 sm:px-3 py-1.5 sm:py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-green-500 focus:border-green-500 text-lg sm:text-xl font-bold"
              autoFocus
            />
          ) : (
            <div className="text-lg sm:text-xl lg:text-2xl font-bold text-gray-900">{getDisplayValue()}</div>
          )}
        </div>
        
        <div>
          <label className="block text-xs font-medium text-gray-500 uppercase tracking-wide mb-1">Conversion Rate</label>
          <div className="flex items-center space-x-1.5 sm:space-x-2">
            <div className="text-xs sm:text-sm font-medium text-gray-700">100 points = ₱1.00</div>
            <div className="w-1.5 h-1.5 sm:w-2 sm:h-2 bg-green-500 rounded-full flex-shrink-0"></div>
          </div>
        </div>
      </div>
    </div>
  );
};

const MaterialsSection: React.FC = () => {
  const [pricing, setPricing] = useState<{ plastic: number; glass: number } | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const fetchPricing = async () => {
      try {
        const adminService = new AdminService();
        const currentPricing = await adminService.getCurrentPricing();
        console.log('Fetched pricing data:', currentPricing); // Debug log
        
        if (currentPricing) {
          setPricing({
            plastic: currentPricing.plastic,
            glass: currentPricing.glass
          });
        }
      } catch (error) {
        console.error('Error fetching pricing:', error);
      } finally {
        setLoading(false);
      }
    };

    fetchPricing();
  }, []);

  const handleSave = async (material: string, newPrice: string) => {
    try {
      const adminService = new AdminService();
      const price = parseFloat(newPrice.replace(' pts', ''));
      await adminService.updateMaterialPricing(material, price);
      
      // Refresh pricing data
      const currentPricing = await adminService.getCurrentPricing();
      if (currentPricing) {
        setPricing({
          plastic: currentPricing.plastic,
          glass: currentPricing.glass
        });
      }
    } catch (error) {
      console.error('Error updating pricing:', error);
    }
  };

  if (loading) {
    return (
      <div className="bg-white p-3 sm:p-4 lg:p-6 rounded-lg sm:rounded-xl border border-gray-200 shadow-sm h-full flex flex-col">
        <h3 className="text-sm sm:text-base lg:text-lg font-semibold text-gray-900 mb-2 sm:mb-3">Materials</h3>
        <div className="flex justify-center flex-1 items-center">
          <div className="animate-spin rounded-full h-6 w-6 sm:h-8 sm:w-8 border-b-2 border-primary-600"></div>
        </div>
      </div>
    );
  }

  return (
    <div className="bg-gradient-to-br from-gray-50 to-white p-3 sm:p-4 lg:p-6 rounded-lg sm:rounded-xl border border-gray-200 shadow-sm h-auto sm:h-full flex flex-col overflow-hidden">
      <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between mb-3 sm:mb-4 gap-2 sm:gap-0">
        <div>
          <h3 className="text-base sm:text-lg lg:text-xl font-bold text-gray-900">Material Pricing</h3>
          <p className="text-xs sm:text-sm text-gray-600 mt-1">Configure point values for materials</p>
        </div>
        <div className="text-xs text-gray-500 bg-gray-100 px-2 sm:px-3 py-1 rounded-full self-start sm:self-auto">
          Live Updates
        </div>
      </div>
      <div className="grid grid-cols-1 sm:grid-cols-2 gap-2 sm:gap-3 sm:flex-1 sm:min-h-0">
        <MaterialCard
          title={`${pricing?.plastic || 0} pts`}
          points="10 points = 1 PHP"
          additionalText="Plastic Bottle"
          color="bg-green-100 text-green-800"
          onSave={(newPrice) => handleSave('plastic', newPrice)}
        />
        <MaterialCard
          title={`${pricing?.glass || 0} pts`}
          points="10 points = 1 PHP"
          additionalText="Tin Can"
          color="bg-green-100 text-green-800"
          onSave={(newPrice) => handleSave('glass', newPrice)}
        />
      </div>
    </div>
  );
};

export default MaterialsSection;
