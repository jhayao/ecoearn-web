'use client';

import React from 'react';
import { Bin } from '@/lib/admin-service';
import { Trash2, Eye, QrCode } from 'lucide-react';

interface BinListProps {
  bins: Bin[];
  onBinSelect: (bin: Bin) => void;
  onDeleteBin: (binId: string) => void;
}

const BinList: React.FC<BinListProps> = ({ bins, onBinSelect, onDeleteBin }) => {
  const getStatusColor = (status: string) => {
    switch (status) {
      case 'active':
        return 'bg-green-100 text-green-800';
      case 'inactive':
        return 'bg-gray-100 text-gray-800';
      default:
        return 'bg-gray-100 text-gray-800';
    }
  };

  const getStatusText = (status: string) => {
    switch (status) {
      case 'active':
        return 'Active';
      case 'inactive':
        return 'Inactive';
      default:
        return 'Unknown';
    }
  };

  return (
    <div className="bg-white rounded-xl border border-gray-200 shadow-sm">
      <div className="px-6 py-4 border-b border-gray-200">
        <h3 className="text-lg font-semibold text-gray-900">Bin Level</h3>
        <p className="text-sm text-gray-600">Manage and monitor bin status</p>
      </div>
      
      <div className="p-6">
        {bins.length === 0 ? (
          <div className="text-center py-8">
            <QrCode className="w-12 h-12 text-gray-400 mx-auto mb-3" />
            <p className="text-gray-500">No bins created yet</p>
            <p className="text-sm text-gray-400">Create your first bin using the form</p>
          </div>
        ) : (
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
            {bins.map((bin) => (
              <div
                key={bin.id}
                className="border border-gray-200 rounded-lg p-4 hover:shadow-md transition-shadow cursor-pointer group"
                onClick={() => onBinSelect(bin)}
              >
                <div className="flex justify-between items-start mb-3">
                  <h4 className="font-medium text-gray-900 truncate">{bin.name}</h4>
                  <div className="flex items-center space-x-2">
                    {/* Online/Offline Status */}
                    {(() => {
                      const now = Date.now();
                      const lastHeartbeat = bin.lastHeartbeat?.seconds 
                        ? bin.lastHeartbeat.seconds * 1000 
                        : 0;
                      const isOnline = (now - lastHeartbeat) < 60000;
                      
                      return (
                        <span className={`px-2 py-1 rounded-full text-xs font-medium flex items-center ${
                          isOnline ? 'bg-green-100 text-green-800' : 'bg-red-100 text-red-800'
                        }`}>
                          <span className={`w-1.5 h-1.5 rounded-full mr-1 ${
                            isOnline ? 'bg-green-500 animate-pulse' : 'bg-red-500'
                          }`}></span>
                          {isOnline ? 'Online' : 'Offline'}
                        </span>
                      );
                    })()}
                    
                    {/* User Status */}
                    <span className={`px-2 py-1 rounded-full text-xs font-medium ${getStatusColor(bin.status)}`}>
                      {getStatusText(bin.status)}
                    </span>
                    <button
                      onClick={(e) => {
                        e.stopPropagation();
                        onDeleteBin(bin.id!);
                      }}
                      className="opacity-0 group-hover:opacity-100 p-1 text-red-600 hover:bg-red-100 rounded transition-all"
                    >
                      <Trash2 className="w-4 h-4" />
                    </button>
                  </div>
                </div>

                {bin.image && (
                  <div className="mb-3">
                    <img
                      src={bin.image}
                      alt={bin.name}
                      className="w-full h-20 object-cover rounded"
                    />
                  </div>
                )}

                <div className="space-y-2">
                  <div className="flex items-center justify-between text-sm">
                    <span className="text-gray-600">Bin ID:</span>
                    <span className="font-mono text-xs text-gray-500">{bin.id}</span>
                  </div>
                  
                  {bin.currentUser && (
                    <div className="flex items-center justify-between text-sm">
                      <span className="text-gray-600">Current User:</span>
                      <span className="text-xs text-blue-600 font-medium">
                        {bin.currentUser.substring(0, 8)}...
                      </span>
                    </div>
                  )}

                  {bin.lat && bin.lng && (
                    <div className="flex items-center justify-between text-sm">
                      <span className="text-gray-600">Location:</span>
                      <span className="text-xs text-gray-500">
                        {bin.lat.toFixed(4)}, {bin.lng.toFixed(4)}
                      </span>
                    </div>
                  )}

                  {bin.level !== undefined && (
                    <div className="flex items-center justify-between text-sm">
                      <span className="text-gray-600">Fill Level:</span>
                      <span className={`text-xs font-medium ${
                        bin.level >= 80 ? 'text-red-600' :
                        bin.level >= 50 ? 'text-orange-600' :
                        'text-green-600'
                      }`}>
                        {bin.level}%
                      </span>
                    </div>
                  )}

                   <div className="flex items-center justify-between text-sm">
                     <span className="text-gray-600">Created:</span>
                     <span className="text-xs text-gray-500">
                       {bin.createdAt ? new Date(bin.createdAt.seconds * 1000).toLocaleDateString() : 'N/A'}
                     </span>
                   </div>
                </div>

                <div className="mt-3 pt-3 border-t border-gray-100">
                  <div className="flex items-center justify-between">
                    <span className="text-xs text-gray-500">QR Code Status</span>
                    <div className="flex items-center space-x-1">
                      <QrCode className="w-3 h-3 text-gray-400" />
                      <span className="text-xs text-gray-600">
                        {bin.qrData ? 'Generated' : 'Not Generated'}
                      </span>
                    </div>
                  </div>
                </div>
              </div>
            ))}
          </div>
        )}
      </div>
    </div>
  );
};

export default BinList;
