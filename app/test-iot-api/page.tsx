'use client';

import React, { useState } from 'react';

export default function TestIoTAPI() {
  const [apiKey, setApiKey] = useState('');
  const [latitude, setLatitude] = useState('8.476876');
  const [longitude, setLongitude] = useState('123.799913');
  const [response, setResponse] = useState<any>(null);
  const [loading, setLoading] = useState(false);

  const testAPI = async () => {
    setLoading(true);
    setResponse(null);

    try {
      const res = await fetch('/api/iot/update-location', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          apiKey: apiKey,
          latitude: parseFloat(latitude),
          longitude: parseFloat(longitude),
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

  const testHealthCheck = async () => {
    setLoading(true);
    setResponse(null);

    try {
      const res = await fetch('/api/iot/update-location', {
        method: 'GET',
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
      <div className="max-w-2xl mx-auto">
        <h1 className="text-3xl font-bold text-gray-900 mb-2">
          IoT API Endpoint Test
        </h1>
        <p className="text-gray-600 mb-8">
          Test the /api/iot/update-location endpoint
        </p>

        <div className="bg-white rounded-lg shadow-md p-6 mb-6">
          <h2 className="text-xl font-semibold mb-4">Health Check (GET)</h2>
          <button
            onClick={testHealthCheck}
            disabled={loading}
            className="px-4 py-2 bg-green-600 text-white rounded hover:bg-green-700 disabled:opacity-50"
          >
            {loading ? 'Testing...' : 'Test Health Check'}
          </button>
        </div>

        <div className="bg-white rounded-lg shadow-md p-6 mb-6">
          <h2 className="text-xl font-semibold mb-4">Update Location (POST)</h2>
          
          <div className="space-y-4">
            <div>
              <label className="block text-sm font-medium text-gray-700 mb-2">
                API Key
              </label>
              <input
                type="text"
                value={apiKey}
                onChange={(e) => setApiKey(e.target.value)}
                placeholder="BIN_XXXXXXXXX_XXXXXX"
                className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500"
              />
            </div>

            <div>
              <label className="block text-sm font-medium text-gray-700 mb-2">
                Latitude
              </label>
              <input
                type="number"
                step="any"
                value={latitude}
                onChange={(e) => setLatitude(e.target.value)}
                className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500"
              />
            </div>

            <div>
              <label className="block text-sm font-medium text-gray-700 mb-2">
                Longitude
              </label>
              <input
                type="number"
                step="any"
                value={longitude}
                onChange={(e) => setLongitude(e.target.value)}
                className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500"
              />
            </div>

            <button
              onClick={testAPI}
              disabled={loading || !apiKey}
              className="w-full px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 disabled:opacity-50"
            >
              {loading ? 'Sending...' : 'Test Update Location'}
            </button>
          </div>
        </div>

        {response && (
          <div className="bg-white rounded-lg shadow-md p-6">
            <h2 className="text-xl font-semibold mb-4">Response</h2>
            
            <div className="mb-4">
              <span className="text-sm font-medium text-gray-700">Status: </span>
              <span
                className={`font-bold ${
                  response.status === 200
                    ? 'text-green-600'
                    : response.status === 'ERROR'
                    ? 'text-red-600'
                    : 'text-orange-600'
                }`}
              >
                {response.status}
              </span>
            </div>

            <div className="bg-gray-50 rounded p-4 overflow-auto">
              <pre className="text-sm text-gray-800">
                {JSON.stringify(response.data, null, 2)}
              </pre>
            </div>

            {response.status === 200 && (
              <div className="mt-4 p-4 bg-green-50 border border-green-200 rounded">
                <p className="text-green-800 font-semibold">✅ Success!</p>
                <p className="text-sm text-green-600 mt-1">
                  The API endpoint is working correctly.
                </p>
              </div>
            )}

            {response.status === 401 && (
              <div className="mt-4 p-4 bg-red-50 border border-red-200 rounded">
                <p className="text-red-800 font-semibold">❌ Invalid API Key</p>
                <p className="text-sm text-red-600 mt-1">
                  The API key you provided is invalid. Create a new bin to get a valid API key.
                </p>
              </div>
            )}

            {response.status === 400 && (
              <div className="mt-4 p-4 bg-orange-50 border border-orange-200 rounded">
                <p className="text-orange-800 font-semibold">⚠️ Bad Request</p>
                <p className="text-sm text-orange-600 mt-1">
                  Check that all fields are filled correctly.
                </p>
              </div>
            )}
          </div>
        )}

        <div className="mt-8 p-4 bg-blue-50 rounded-lg border border-blue-200">
          <h3 className="font-semibold text-blue-900 mb-2">📝 Instructions</h3>
          <ol className="text-sm text-blue-800 space-y-1 list-decimal list-inside">
            <li>First, test the health check to verify the endpoint exists</li>
            <li>Create a new bin in the admin panel to get an API key</li>
            <li>Copy the API key and paste it above</li>
            <li>Adjust latitude/longitude if needed</li>
            <li>Click "Test Update Location"</li>
          </ol>
        </div>
      </div>
    </div>
  );
}
