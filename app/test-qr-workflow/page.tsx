'use client';

import React, { useState, useEffect } from 'react';
import { QrCode, Lock, Unlock, CheckCircle, XCircle } from 'lucide-react';
import { AdminService } from '@/lib/admin-service';

export default function TestQRWorkflow() {
  const [bins, setBins] = useState<any[]>([]);
  const [selectedBin, setSelectedBin] = useState<any>(null);
  const [userId, setUserId] = useState('test_user_123');
  const [userName, setUserName] = useState('Test User');
  const [userEmail, setUserEmail] = useState('test@example.com');
  const [response, setResponse] = useState<any>(null);
  const [loading, setLoading] = useState(false);
  const [activeBins, setActiveBins] = useState<any[]>([]);

  useEffect(() => {
    fetchBins();
  }, []);

  const fetchBins = async () => {
    try {
      const adminService = new AdminService();
      const binsList = await adminService.getBins();
      setBins(binsList);
      
      // Filter active bins for current user
      const userActiveBins = binsList.filter(
        (bin: any) => bin.status === 'active' && bin.currentUser === userId
      );
      setActiveBins(userActiveBins);
    } catch (error) {
      console.error('Error fetching bins:', error);
    }
  };

  const simulateScan = async (bin: any) => {
    if (!bin.qrData) {
      alert('This bin does not have a QR code generated yet.');
      return;
    }

    setLoading(true);
    setResponse(null);

    try {
      const res = await fetch('/api/bins/scan-qr', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          qrData: bin.qrData,
          userId: userId,
          userName: userName,
          userEmail: userEmail,
        }),
      });

      const data = await res.json();
      setResponse({
        status: res.status,
        data: data,
      });

      // Refresh bins to show updated status
      await fetchBins();
    } catch (error: any) {
      setResponse({
        status: 'ERROR',
        data: { error: error.message },
      });
    } finally {
      setLoading(false);
    }
  };

  const deactivateBin = async (binId: string) => {
    setLoading(true);
    setResponse(null);

    try {
      const res = await fetch('/api/bins/deactivate', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          binId: binId,
          userId: userId,
          userEmail: userEmail,
        }),
      });

      const data = await res.json();
      setResponse({
        status: res.status,
        data: data,
      });

      // Refresh bins
      await fetchBins();
    } catch (error: any) {
      setResponse({
        status: 'ERROR',
        data: { error: error.message },
      });
    } finally {
      setLoading(false);
    }
  };

  const checkStatus = async (binId: string) => {
    setLoading(true);
    setResponse(null);

    try {
      const res = await fetch('/api/bins/check-status', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          binId: binId,
          userId: userId,
        }),
      });

      const data = await res.json();
      setResponse({
        status: res.status,
        data: data,
      });
    } catch (error: any) {
      setResponse({
        status: 'ERROR',
        data: { error: error.message },
      });
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="min-h-screen bg-gray-50 p-8">
      <div className="max-w-6xl mx-auto">
        <h1 className="text-3xl font-bold text-gray-900 mb-2">
          QR Code Workflow Test
        </h1>
        <p className="text-gray-600 mb-8">
          Simulate mobile app QR scanning and bin activation
        </p>

        {/* User Info */}
        <div className="bg-white rounded-lg shadow-md p-6 mb-6">
          <h2 className="text-xl font-semibold mb-4">Current User</h2>
          <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
            <div>
              <label className="block text-sm font-medium text-gray-700 mb-2">
                User ID
              </label>
              <input
                type="text"
                value={userId}
                onChange={(e) => setUserId(e.target.value)}
                className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500"
              />
            </div>
            <div>
              <label className="block text-sm font-medium text-gray-700 mb-2">
                User Name
              </label>
              <input
                type="text"
                value={userName}
                onChange={(e) => setUserName(e.target.value)}
                className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500"
              />
            </div>
            <div>
              <label className="block text-sm font-medium text-gray-700 mb-2">
                User Email
              </label>
              <input
                type="email"
                value={userEmail}
                onChange={(e) => setUserEmail(e.target.value)}
                className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500"
              />
            </div>
          </div>
        </div>

        {/* Active Bins */}
        {activeBins.length > 0 && (
          <div className="bg-green-50 border border-green-200 rounded-lg p-6 mb-6">
            <h2 className="text-xl font-semibold text-green-900 mb-4">
              Your Active Bins ({activeBins.length})
            </h2>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
              {activeBins.map((bin) => (
                <div key={bin.id} className="bg-white rounded-lg p-4 border border-green-300">
                  <div className="flex items-center justify-between mb-2">
                    <h3 className="font-semibold text-gray-900">{bin.name}</h3>
                    <Lock className="w-5 h-5 text-green-600" />
                  </div>
                  <p className="text-sm text-gray-600 mb-3">
                    Status: <span className="font-medium text-green-600">Active (Locked to you)</span>
                  </p>
                  <button
                    onClick={() => deactivateBin(bin.id!)}
                    disabled={loading}
                    className="w-full px-4 py-2 bg-red-600 text-white rounded hover:bg-red-700 disabled:opacity-50 text-sm"
                  >
                    Finish Recycling
                  </button>
                </div>
              ))}
            </div>
          </div>
        )}

        {/* Available Bins */}
        <div className="bg-white rounded-lg shadow-md p-6 mb-6">
          <div className="flex items-center justify-between mb-4">
            <h2 className="text-xl font-semibold">Available Bins</h2>
            <button
              onClick={fetchBins}
              className="px-4 py-2 bg-gray-600 text-white rounded hover:bg-gray-700 text-sm"
            >
              Refresh
            </button>
          </div>

          {bins.length === 0 ? (
            <div className="text-center py-8">
              <QrCode className="w-12 h-12 text-gray-400 mx-auto mb-3" />
              <p className="text-gray-500">No bins available</p>
              <p className="text-sm text-gray-400">Create bins in the admin panel first</p>
            </div>
          ) : (
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
              {bins.map((bin) => (
                <div
                  key={bin.id}
                  className={`border rounded-lg p-4 ${
                    bin.status === 'active' && bin.currentUser !== userId
                      ? 'bg-red-50 border-red-200'
                      : bin.status === 'active' && bin.currentUser === userId
                      ? 'bg-green-50 border-green-200'
                      : 'bg-white border-gray-200'
                  }`}
                >
                  <div className="flex items-center justify-between mb-2">
                    <h3 className="font-semibold text-gray-900 text-sm">{bin.name}</h3>
                    {bin.status === 'active' ? (
                      <Lock className="w-4 h-4 text-red-600" />
                    ) : (
                      <Unlock className="w-4 h-4 text-green-600" />
                    )}
                  </div>

                  <div className="text-xs text-gray-600 mb-3 space-y-1">
                    <p>
                      Status:{' '}
                      <span
                        className={`font-medium ${
                          bin.status === 'active' ? 'text-red-600' : 'text-green-600'
                        }`}
                      >
                        {bin.status === 'active' ? 'Occupied' : 'Available'}
                      </span>
                    </p>
                    <p>Fill Level: {bin.level || 0}%</p>
                    {bin.currentUser && (
                      <p className="text-xs text-gray-500 truncate">
                        User: {bin.currentUser.substring(0, 12)}...
                      </p>
                    )}
                  </div>

                  <div className="flex flex-col gap-2">
                    <button
                      onClick={() => simulateScan(bin)}
                      disabled={loading || !bin.qrData}
                      className={`w-full px-3 py-2 text-white rounded text-sm ${
                        bin.status === 'active' && bin.currentUser !== userId
                          ? 'bg-gray-400 cursor-not-allowed'
                          : 'bg-blue-600 hover:bg-blue-700'
                      } disabled:opacity-50`}
                    >
                      {loading ? 'Processing...' : 'Scan QR Code'}
                    </button>
                    <button
                      onClick={() => checkStatus(bin.id!)}
                      disabled={loading}
                      className="w-full px-3 py-2 bg-gray-600 text-white rounded hover:bg-gray-700 text-sm disabled:opacity-50"
                    >
                      Check Status
                    </button>
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>

        {/* Response */}
        {response && (
          <div className="bg-white rounded-lg shadow-md p-6">
            <h2 className="text-xl font-semibold mb-4">API Response</h2>

            <div className="mb-4">
              <span className="text-sm font-medium text-gray-700">Status: </span>
              <span
                className={`font-bold ${
                  response.status === 200
                    ? 'text-green-600'
                    : response.status === 'ERROR'
                    ? 'text-red-600'
                    : response.status === 409
                    ? 'text-orange-600'
                    : 'text-red-600'
                }`}
              >
                {response.status}
              </span>
            </div>

            <div className="bg-gray-50 rounded p-4 overflow-auto mb-4">
              <pre className="text-sm text-gray-800">
                {JSON.stringify(response.data, null, 2)}
              </pre>
            </div>

            {response.status === 200 && response.data.success && (
              <div className="p-4 bg-green-50 border border-green-200 rounded flex items-start gap-3">
                <CheckCircle className="w-5 h-5 text-green-600 flex-shrink-0 mt-0.5" />
                <div>
                  <p className="text-green-800 font-semibold">
                    {response.data.message}
                  </p>
                  {response.data.binStatus === 'activated' && (
                    <p className="text-sm text-green-600 mt-1">
                      The bin is now locked to you. You can start recycling!
                    </p>
                  )}
                  {response.data.binStatus === 'already_active' && (
                    <p className="text-sm text-green-600 mt-1">
                      You already have this bin activated.
                    </p>
                  )}
                </div>
              </div>
            )}

            {response.status === 409 && (
              <div className="p-4 bg-orange-50 border border-orange-200 rounded flex items-start gap-3">
                <XCircle className="w-5 h-5 text-orange-600 flex-shrink-0 mt-0.5" />
                <div>
                  <p className="text-orange-800 font-semibold">Bin Occupied</p>
                  <p className="text-sm text-orange-600 mt-1">
                    {response.data.error}
                  </p>
                </div>
              </div>
            )}

            {response.status !== 200 && response.status !== 409 && (
              <div className="p-4 bg-red-50 border border-red-200 rounded flex items-start gap-3">
                <XCircle className="w-5 h-5 text-red-600 flex-shrink-0 mt-0.5" />
                <div>
                  <p className="text-red-800 font-semibold">Error</p>
                  <p className="text-sm text-red-600 mt-1">{response.data.error}</p>
                </div>
              </div>
            )}
          </div>
        )}

        {/* Instructions */}
        <div className="mt-8 p-6 bg-blue-50 rounded-lg border border-blue-200">
          <h3 className="font-semibold text-blue-900 mb-3">📱 How to Test</h3>
          <ol className="text-sm text-blue-800 space-y-2 list-decimal list-inside">
            <li>Create bins in the admin panel if you haven't already</li>
            <li>Change User ID to simulate different users</li>
            <li>Click "Scan QR Code" on any available bin</li>
            <li>Bin will be locked to your user (status = active)</li>
            <li>Try scanning the same bin with a different User ID - it will be blocked</li>
            <li>Click "Finish Recycling" to deactivate the bin</li>
            <li>Bin becomes available for other users again</li>
          </ol>

          <div className="mt-4 p-3 bg-blue-100 rounded">
            <p className="text-sm font-semibold text-blue-900 mb-1">
              Testing Multiple Users:
            </p>
            <p className="text-xs text-blue-700">
              Change the User ID field to "test_user_456" and try scanning a bin that's
              already active with "test_user_123". You'll see a 409 Conflict error.
            </p>
          </div>
        </div>
      </div>
    </div>
  );
}
