'use client';

import React, { useState, useEffect } from 'react';
import { Search, Filter, Calendar, Users, UserCheck, UserX } from 'lucide-react';
import { AdminService, ActivityLog } from '@/lib/admin-service';
import StatsCard from '@/components/dashboard/StatsCard';

const ActivityLogsPage: React.FC = () => {
  const [logs, setLogs] = useState<ActivityLog[]>([]);
  const [filteredLogs, setFilteredLogs] = useState<ActivityLog[]>([]);
  const [searchQuery, setSearchQuery] = useState('');
  const [sortFilter, setSortFilter] = useState('Newest');
  const [currentPage, setCurrentPage] = useState(1);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [userStats, setUserStats] = useState({
    totalUsers: 0,
    activeUsers: 0,
    inactiveUsers: 0,
  });
  
  const itemsPerPage = 10;

  // Utility function to format IP addresses
  const formatIPAddress = (ip: string): { display: string; isIPv6: boolean } => {
    if (!ip || ip === 'N/A') return { display: 'N/A', isIPv6: false };
    
    // Check if it's IPv6 (contains colons)
    const isIPv6 = ip.includes(':');
    
    if (isIPv6) {
      // For IPv6, show first 3 groups + ellipsis if longer
      const parts = ip.split(':');
      if (parts.length > 3) {
        return { 
          display: `${parts.slice(0, 3).join(':')}...`, 
          isIPv6: true 
        };
      }
    } else {
      // For IPv4, show normally (they're usually short enough)
      return { display: ip, isIPv6: false };
    }
    
    return { display: ip, isIPv6 };
  };

  useEffect(() => {
    const fetchData = async () => {
      try {
        const adminService = new AdminService();
        
        // Fetch activity logs
        const logsList = await adminService.fetchActivityData();
        setLogs(logsList);
        setFilteredLogs(logsList);
        
        // Fetch user stats
        const stats = await adminService.getUserStats();
        setUserStats(stats);
      } catch (err) {
        setError('Failed to fetch data');
        console.error('Error fetching data:', err);
      } finally {
        setLoading(false);
      }
    };

    fetchData();
  }, []);

  useEffect(() => {
    let filtered = logs.filter(log =>
      (log.email?.toLowerCase() || '').includes(searchQuery.toLowerCase()) ||
      (log.action?.toLowerCase() || '').includes(searchQuery.toLowerCase()) ||
      (log.description?.toLowerCase() || '').includes(searchQuery.toLowerCase()) ||
      (log.binName?.toLowerCase() || '').includes(searchQuery.toLowerCase())
    );

    // Sort logs
    if (sortFilter === 'Newest') {
      filtered.sort((a, b) => new Date(b.date + ' ' + b.time).getTime() - new Date(a.date + ' ' + a.time).getTime());
    } else {
      filtered.sort((a, b) => new Date(a.date + ' ' + a.time).getTime() - new Date(b.date + ' ' + b.time).getTime());
    }

    setFilteredLogs(filtered);
    setCurrentPage(1);
  }, [logs, searchQuery, sortFilter]);

  const getCurrentPageLogs = () => {
    const startIndex = (currentPage - 1) * itemsPerPage;
    const endIndex = startIndex + itemsPerPage;
    return filteredLogs.slice(startIndex, endIndex);
  };

  const totalPages = Math.ceil(filteredLogs.length / itemsPerPage);

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
    <div className="space-y-6">
      {/* Header */}
      <div className="flex justify-between items-center">
        <h1 className="text-3xl font-bold text-gray-900">Activity Logs</h1>
        <div className="flex items-center space-x-2 text-sm text-gray-600">
          <Calendar className="w-4 h-4" />
          <span>Total: {logs.length} activities</span>
        </div>
      </div>

      {/* User Stats Row */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
        <StatsCard
          title="Inactive Users"
          value={userStats.inactiveUsers}
          icon={UserX}
          color="text-red-600"
          bgColor="bg-red-100"
        />
        <StatsCard
          title="Active Users"
          value={userStats.activeUsers}
          icon={UserCheck}
          color="text-green-600"
          bgColor="bg-green-100"
        />
        <StatsCard
          title="Total Users"
          value={userStats.totalUsers}
          icon={Users}
          color="text-blue-600"
          bgColor="bg-blue-100"
        />
      </div>

      {/* Search and Filter */}
      <div className="flex space-x-4">
        <div className="flex-1 relative">
          <Search className="absolute left-3 top-1/2 transform -translate-y-1/2 text-gray-400 w-4 h-4" />
          <input
            type="text"
            placeholder="Search by email, bin name, action, or description..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="w-full pl-10 pr-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-primary-500 focus:border-primary-500"
          />
        </div>
        <div className="relative">
          <Filter className="absolute left-3 top-1/2 transform -translate-y-1/2 text-gray-400 w-4 h-4" />
          <select
            value={sortFilter}
            onChange={(e) => setSortFilter(e.target.value)}
            className="pl-10 pr-8 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-primary-500 focus:border-primary-500 appearance-none bg-white"
          >
            <option value="Newest">Newest</option>
            <option value="Oldest">Oldest</option>
          </select>
        </div>
      </div>

      {/* Activity Logs Table */}
      <div className="bg-white rounded-xl border border-gray-200 shadow-sm overflow-hidden">
        {/* Mobile scrollable container */}
        <div className="overflow-x-auto">
          <div className="min-w-[800px]">
            {/* Table Header */}
            <div className="px-6 py-4 bg-gray-50 border-b border-gray-200">
              <div className="grid grid-cols-6 gap-4 text-sm font-medium text-gray-700">
                <div className="min-w-[120px]">User/Bin</div>
                <div className="flex items-center gap-1 min-w-[120px]">
                  IP Address
                  <span className="text-xs text-gray-500 font-normal">(IPv4/IPv6)</span>
                </div>
                <div className="min-w-[120px]">Date & Time</div>
                <div className="min-w-[100px]">Action</div>
                <div className="min-w-[150px]">Details</div>
                <div className="min-w-[100px]">Type</div>
              </div>
            </div>

            {/* Table Body */}
            <div className="divide-y divide-gray-200">
              {getCurrentPageLogs().map((log) => (
                <div key={log.id} className="px-6 py-4 hover:bg-gray-50 transition-colors">
                  <div className="grid grid-cols-6 gap-4 text-sm text-gray-900">
                    <div className="font-medium truncate min-w-[120px]" title={log.binName || log.email || 'N/A'}>
                      {log.binName || log.email || 'N/A'}
                    </div>
                    <div 
                      className="font-mono text-xs truncate cursor-help min-w-[120px]" 
                      title={`${log.ipAddress || 'N/A'}${formatIPAddress(log.ipAddress || '').isIPv6 ? ' (IPv6)' : ''}`}
                    >
                      {formatIPAddress(log.ipAddress || '').display}
                    </div>
                    <div className="min-w-[120px]">
                      <div className="truncate">{log.date || 'N/A'}</div>
                      <div className="text-xs text-gray-500 truncate">{log.time || 'N/A'}</div>
                    </div>
                    <div className="min-w-[100px]">
                      <span className={`px-2 py-1 rounded-full text-xs font-medium ${
                        log.action === 'Online' ? 'bg-green-100 text-green-800' :
                        log.action === 'Login' ? 'bg-blue-100 text-blue-800' :
                        log.action === 'Logout' ? 'bg-red-100 text-red-800' :
                        log.action === 'Bin Activated' ? 'bg-emerald-100 text-emerald-800' :
                        log.action === 'Bin Deactivated' ? 'bg-orange-100 text-orange-800' :
                        'bg-gray-100 text-gray-800'
                      }`}>
                        {log.action || 'N/A'}
                      </span>
                    </div>
                    <div className="truncate min-w-[150px]" title={log.description || 'N/A'}>
                      {log.description || 'N/A'}
                    </div>
                    <div className="min-w-[100px]">
                      <span className={`px-2 py-1 rounded-full text-xs font-medium ${
                        log.binId ? 'bg-purple-100 text-purple-800' : 'bg-blue-100 text-blue-800'
                      }`}>
                        {log.binId ? 'Bin' : 'User'}
                      </span>
                    </div>
                  </div>
                </div>
              ))}
            </div>
          </div>
        </div>
      </div>

      {/* Pagination */}
      <div className="flex justify-between items-center">
        <p className="text-sm text-gray-600">
          Showing {((currentPage - 1) * itemsPerPage) + 1} to {Math.min(currentPage * itemsPerPage, filteredLogs.length)} of {filteredLogs.length} entries
        </p>
        <div className="flex space-x-2">
          <button
            onClick={() => setCurrentPage(prev => Math.max(prev - 1, 1))}
            disabled={currentPage === 1}
            className="px-3 py-1 border border-gray-300 rounded-lg disabled:opacity-50 disabled:cursor-not-allowed hover:bg-gray-50"
          >
            Previous
          </button>
          {Array.from({ length: totalPages }, (_, i) => i + 1).map((page) => (
            <button
              key={page}
              onClick={() => setCurrentPage(page)}
              className={`px-3 py-1 border rounded-lg ${
                currentPage === page
                  ? 'bg-primary-600 text-white border-primary-600'
                  : 'border-gray-300 hover:bg-gray-50'
              }`}
            >
              {page}
            </button>
          ))}
          <button
            onClick={() => setCurrentPage(prev => Math.min(prev + 1, totalPages))}
            disabled={currentPage === totalPages}
            className="px-3 py-1 border border-gray-300 rounded-lg disabled:opacity-50 disabled:cursor-not-allowed hover:bg-gray-50"
          >
            Next
          </button>
        </div>
      </div>
    </div>
  );
};

export default ActivityLogsPage;
