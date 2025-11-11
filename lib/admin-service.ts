import { 
  collection, 
  doc, 
  addDoc, 
  deleteDoc, 
  getDocs, 
  getDoc, 
  updateDoc, 
  setDoc,
  query, 
  orderBy, 
  where, 
  limit, 
  onSnapshot, 
  Timestamp,
  serverTimestamp,
  QuerySnapshot,
  DocumentData
} from 'firebase/firestore';
import { db } from './firebase';

export interface Bin {
  id?: string;
  name: string;
  image: string;
  qrData: string;
  qrCodePhoto?: string; // QR code saved as photo
  status: 'active' | 'inactive';
  currentUser?: string; // User ID who scanned the QR code
  createdAt: Timestamp;
  apiKey?: string; // API key for IoT device authentication
  // IoT capacity monitoring
  comp1Capacity?: number; // Compartment 1 capacity (0-100%)
  comp2Capacity?: number; // Compartment 2 capacity (0-100%)
  lastCapacityUpdate?: Timestamp; // Last capacity update timestamp
  // IoT heartbeat & online status
  lastHeartbeat?: Timestamp; // Last heartbeat from IoT device
  onlineStatus?: 'online' | 'offline'; // Current online status
  deviceStatus?: string; // Device status: 'active', 'idle', 'error', etc.
  // Legacy fields for backward compatibility
  lat?: number;
  lng?: number;
  level?: number;
  color?: string;
}

export interface Report {
  id?: string;
  userName: string;
  description: string;
  location: string;
  image: string;
  timestamp: Timestamp;
}

export interface RecyclingRequest {
  id?: string;
  userId: string;
  userName: string;
  materialType: string;
  quantity: number;
  weight: number;
  status: string;
  timestamp: Timestamp;
  profilePicture?: string;
}

export interface ActivityLog {
  id?: string;
  email: string;
  ipAddress: string;
  date: string;
  time: string;
  action: string;
  description: string;
}

export interface Transaction {
  id?: string;
  amount: number;
  status: string;
  timestamp: Timestamp;
}

export interface AdminTransaction {
  id?: string;
  type: 'add' | 'withdraw';
  amount: number;
  timestamp: Timestamp;
}

export interface Pricing {
  plastic: number;
  glass: number;
  metal?: Record<string, number>;
  conversionRate?: number;
  updatedAt: Timestamp;
}

export class AdminService {
  // Notifications
  notifications: any[] = [];
  unreadNotificationCount = 0;

  constructor() {
    this.setupNotificationListeners();
  }

  private setupNotificationListeners() {
    // Listen for bins that are 90% full or more
    const binsQuery = query(
      collection(db, 'bins'),
      where('level', '>=', 90)
    );
    
    onSnapshot(binsQuery, (snapshot) => {
      snapshot.docChanges().forEach((change) => {
        if (change.type === 'added' || change.type === 'modified') {
          const data = change.doc.data();
          this.addNotification({
            title: 'Bin Alert',
            message: `${data.name} is ${data.level}% full!`,
            type: 'bin',
            timestamp: serverTimestamp(),
          });
        }
      });
    });

    // Listen for new reports
    const reportsQuery = query(
      collection(db, 'reports'),
      orderBy('timestamp', 'desc'),
      limit(1)
    );
    
    onSnapshot(reportsQuery, (snapshot) => {
      snapshot.docChanges().forEach((change) => {
        if (change.type === 'added') {
          this.addNotification({
            title: 'New Report',
            message: 'A new user report has been submitted',
            type: 'report',
            timestamp: serverTimestamp(),
          });
        }
      });
    });

    // Listen for new recycling requests
    const recyclingQuery = query(
      collection(db, 'recycling_requests'),
      orderBy('timestamp', 'desc'),
      limit(1)
    );
    
    onSnapshot(recyclingQuery, (snapshot) => {
      snapshot.docChanges().forEach((change) => {
        if (change.type === 'added') {
          this.addNotification({
            title: 'New Recycling Request',
            message: 'A new recycling request has been submitted',
            type: 'recycling',
            timestamp: serverTimestamp(),
          });
        }
      });
    });
  }

  private addNotification(notification: any) {
    this.notifications.unshift({
      ...notification,
      read: false,
    });
    this.unreadNotificationCount++;
  }

  markNotificationsAsRead() {
    this.notifications.forEach(notification => {
      notification.read = true;
    });
    this.unreadNotificationCount = 0;
  }

  // Generate a unique API key for IoT devices
  private generateApiKey(): string {
    const timestamp = Date.now().toString(36);
    const randomStr = Math.random().toString(36).substring(2, 15);
    const randomStr2 = Math.random().toString(36).substring(2, 15);
    return `BIN_${timestamp}_${randomStr}${randomStr2}`.toUpperCase();
  }

  // Bin Management
  async addBin(binData: Omit<Bin, 'id' | 'createdAt' | 'status' | 'currentUser' | 'apiKey'>): Promise<string> {
    try {
      const apiKey = this.generateApiKey();
      const docRef = await addDoc(collection(db, 'bins'), {
        ...binData,
        status: 'inactive',
        apiKey: apiKey,
        createdAt: serverTimestamp(),
      });
      
      console.log(`Bin created with API Key: ${apiKey}`);
      return docRef.id;
    } catch (error) {
      throw new Error(`Failed to add bin: ${error}`);
    }
  }

  // Activate bin when QR code is scanned
  async activateBin(binId: string, userId: string): Promise<void> {
    try {
      const binRef = doc(db, 'bins', binId);
      await updateDoc(binRef, {
        status: 'active',
        currentUser: userId,
      });
    } catch (error) {
      throw new Error(`Failed to activate bin: ${error}`);
    }
  }

  // Deactivate bin when user finishes recycling
  async deactivateBin(binId: string): Promise<void> {
    try {
      const binRef = doc(db, 'bins', binId);
      await updateDoc(binRef, {
        status: 'inactive',
        currentUser: null,
      });
    } catch (error) {
      throw new Error(`Failed to deactivate bin: ${error}`);
    }
  }

  // Deactivate bin for a specific user (when they finish recycling)
  async deactivateBinForUser(binId: string, userId: string): Promise<void> {
    try {
      const binDoc = await getDoc(doc(db, 'bins', binId));
      if (!binDoc.exists()) {
        throw new Error('Bin not found');
      }
      
      const binData = binDoc.data() as Bin;
      if (binData.currentUser !== userId) {
        throw new Error('User is not authorized to deactivate this bin');
      }
      
      const binRef = doc(db, 'bins', binId);
      await updateDoc(binRef, {
        status: 'inactive',
        currentUser: null,
        // CRITICAL: Send DEACTIVATE_BIN command to ESP32
        pendingCommand: 'DEACTIVATE_BIN',
        pendingCommandTimestamp: serverTimestamp()
      });
      
      console.log(`[Deactivate] Set DEACTIVATE_BIN command for bin ${binId}`);
    } catch (error) {
      throw new Error(`Failed to deactivate bin for user: ${error}`);
    }
  }

  // Get a specific bin by ID
  async getBinById(binId: string): Promise<Bin | null> {
    try {
      const binDoc = await getDoc(doc(db, 'bins', binId));
      if (!binDoc.exists()) return null;
      
      return {
        id: binDoc.id,
        ...binDoc.data()
      } as Bin;
    } catch (error) {
      console.error('Error getting bin by ID:', error);
      return null;
    }
  }

  // Update bin information
  async updateBin(binId: string, updateData: Partial<Bin>): Promise<void> {
    try {
      const binRef = doc(db, 'bins', binId);
      await updateDoc(binRef, updateData);
    } catch (error) {
      throw new Error(`Failed to update bin: ${error}`);
    }
  }

  // Verify API key and get bin ID
  async verifyApiKey(apiKey: string): Promise<string | null> {
    try {
      const snapshot = await getDocs(
        query(collection(db, 'bins'), where('apiKey', '==', apiKey))
      );
      
      if (snapshot.empty) {
        return null;
      }
      
      return snapshot.docs[0].id;
    } catch (error) {
      console.error('Error verifying API key:', error);
      return null;
    }
  }

  // Update bin location from IoT device
  async updateBinLocation(apiKey: string, latitude: number, longitude: number): Promise<boolean> {
    try {
      const binId = await this.verifyApiKey(apiKey);
      
      if (!binId) {
        console.error('Invalid API key');
        return false;
      }
      
      const binRef = doc(db, 'bins', binId);
      await updateDoc(binRef, {
        lat: latitude,
        lng: longitude,
      });
      
      console.log(`Updated bin ${binId} location: ${latitude}, ${longitude}`);
      return true;
    } catch (error) {
      console.error('Error updating bin location:', error);
      return false;
    }
  }

  // Update bin capacity from IoT device
  async updateBinCapacity(apiKey: string, comp1Capacity: number, comp2Capacity: number): Promise<{ success: boolean; binId?: string; error?: string }> {
    try {
      const binId = await this.verifyApiKey(apiKey);
      
      if (!binId) {
        return { success: false, error: 'Invalid API key' };
      }

      const binRef = doc(db, 'bins', binId);
      await updateDoc(binRef, {
        comp1Capacity: comp1Capacity,
        comp2Capacity: comp2Capacity,
        lastCapacityUpdate: serverTimestamp(),
      });
      
      console.log(`Updated bin ${binId} capacity: Comp1=${comp1Capacity}%, Comp2=${comp2Capacity}%`);
      return { success: true, binId };
    } catch (error) {
      console.error('Error updating bin capacity:', error);
      return { success: false, error: 'Failed to update capacity' };
    }
  }

  // Check if user can recycle in a specific bin
  async canUserRecycleInBin(binId: string, userId: string): Promise<boolean> {
    try {
      const binDoc = await getDoc(doc(db, 'bins', binId));
      if (!binDoc.exists()) return false;
      
      const binData = binDoc.data() as Bin;
      return binData.status === 'active' && binData.currentUser === userId;
    } catch (error) {
      console.error('Error checking bin access:', error);
      return false;
    }
  }

  async deleteBin(binId: string): Promise<void> {
    try {
      await deleteDoc(doc(db, 'bins', binId));
    } catch (error) {
      throw new Error(`Failed to delete bin: ${error}`);
    }
  }

  async getBins(): Promise<Bin[]> {
    try {
      const snapshot = await getDocs(collection(db, 'bins'));
      return snapshot.docs.map(doc => ({
        id: doc.id,
        ...doc.data()
      } as Bin));
    } catch (error) {
      console.error('Error fetching bins:', error);
      return [];
    }
  }

  async getBinsByYear(year: number): Promise<Bin[]> {
    try {
      const snapshot = await getDocs(collection(db, 'bins'));
      const yearBins: Bin[] = [];
      
      snapshot.docs.forEach((doc) => {
        const data = doc.data();
        const createdAt = data.createdAt as Timestamp;
        
        if (createdAt) {
          const dateTime = createdAt.toDate();
          if (dateTime.getFullYear() === year) {
            yearBins.push({
              id: doc.id,
              ...data
            } as Bin);
          }
        }
      });
      
      return yearBins;
    } catch (error) {
      console.error('Error fetching bins for year:', error);
      return [];
    }
  }

  // Reports
  async getReports(): Promise<Report[]> {
    try {
      const snapshot = await getDocs(collection(db, 'reports'));
      return snapshot.docs.map(doc => ({
        id: doc.id,
        ...doc.data()
      } as Report));
    } catch (error) {
      console.error('Error fetching reports:', error);
      return [];
    }
  }

  async getReportsCount(): Promise<number> {
    try {
      const snapshot = await getDocs(collection(db, 'reports'));
      return snapshot.docs.length;
    } catch (error) {
      console.error('Error fetching reports count:', error);
      return 0;
    }
  }

  async getReportsByYear(year: number): Promise<Report[]> {
    try {
      const snapshot = await getDocs(collection(db, 'reports'));
      const yearReports: Report[] = [];
      
      snapshot.docs.forEach((doc) => {
        const data = doc.data();
        const timestamp = data.timestamp as Timestamp;
        
        if (timestamp) {
          const dateTime = timestamp.toDate();
          if (dateTime.getFullYear() === year) {
            yearReports.push({
              id: doc.id,
              ...data
            } as Report);
          }
        }
      });
      
      return yearReports;
    } catch (error) {
      console.error('Error fetching reports for year:', error);
      return [];
    }
  }

  // Recycling Requests
  async getRecentRecycles(): Promise<RecyclingRequest[]> {
    try {
      console.log('Fetching recent recycles...');
      const q = query(
        collection(db, 'recycling_requests'),
        orderBy('timestamp', 'desc'),
        limit(6)
      );
      const snapshot = await getDocs(q);
      console.log('Found', snapshot.docs.length, 'recycling requests');
      
      // Get all unique user IDs from the recycling requests
      const userIds = Array.from(new Set(snapshot.docs.map(doc => doc.data().userId).filter(Boolean)));
      console.log('Unique user IDs:', userIds);
      
      // Fetch all user profiles at once
      const userProfiles = new Map();
      if (userIds.length > 0) {
        const userPromises = userIds.map(async (userId) => {
          try {
            const userDocRef = doc(db, 'users', userId);
            const userDoc = await getDoc(userDocRef);
            if (userDoc.exists()) {
              const userData = userDoc.data();
              userProfiles.set(userId, userData?.profilePicture || '');
              console.log(`Fetched profile for user ${userId}: ${userData?.profilePicture ? 'Yes' : 'No'}`);
            }
          } catch (error) {
            console.log(`Error fetching user ${userId}:`, error);
          }
        });
        await Promise.all(userPromises);
      }
      
      const recycles = snapshot.docs.map(doc => {
        const data = doc.data();
        const profilePicture = userProfiles.get(data.userId) || '';
        
        console.log(`User ${data.userName} (${data.userId}): Profile picture ${profilePicture ? 'found' : 'not found'}`);
        
        return {
          id: doc.id,
          ...data,
          profilePicture
        } as RecyclingRequest;
      });
      
      return recycles;
    } catch (error) {
      console.error('Error fetching recent recycles:', error);
      return [];
    }
  }

  async getTotalRecyclingStats(): Promise<{ plastic: number; glass: number }> {
    try {
      const snapshot = await getDocs(
        query(collection(db, 'recycling_requests'), where('status', '==', 'approved'))
      );
      
      let plastic = 0;
      let glass = 0;
      
      console.log('Total recycling requests found:', snapshot.docs.length);
      
      snapshot.docs.forEach((doc) => {
        const data = doc.data();
        const quantity = data.quantity || 0;
        const weight = data.weight || 0;
        const materialType = data.materialType?.toLowerCase();
        
        console.log('Processing recycling request:', {
          materialType,
          quantity,
          weight,
          status: data.status
        });
        
        if (materialType === 'plastic' || materialType === 'plastic bottle') {
          plastic += weight;
        } else if (materialType === 'glass' || materialType === 'tin can' || materialType === 'metal') {
          glass += quantity;
        }
      });
      
      console.log('Final recycling stats:', { plastic, glass });
      return { plastic, glass };
    } catch (error) {
      console.error('Error in getTotalRecyclingStats:', error);
      throw new Error(`Failed to get recycling stats: ${error}`);
    }
  }

  async getAvailableYears(): Promise<number[]> {
    try {
      const years = new Set<number>();
      
      // Get years from recycling requests
      const recyclingSnapshot = await getDocs(
        query(collection(db, 'recycling_requests'), where('status', '==', 'approved'))
      );
      
      recyclingSnapshot.docs.forEach((doc) => {
        const data = doc.data();
        const timestamp = data.timestamp as Timestamp;
        if (timestamp) {
          years.add(timestamp.toDate().getFullYear());
        }
      });
      
      // Get years from reports
      const reportsSnapshot = await getDocs(collection(db, 'reports'));
      reportsSnapshot.docs.forEach((doc) => {
        const data = doc.data();
        const timestamp = data.timestamp as Timestamp;
        if (timestamp) {
          years.add(timestamp.toDate().getFullYear());
        }
      });
      
      // Get years from activity logs
      const activitySnapshot = await getDocs(collection(db, 'userActivities'));
      activitySnapshot.docs.forEach((doc) => {
        const data = doc.data();
        const dateStr = data.date as string;
        if (dateStr) {
          try {
            const date = new Date(dateStr);
            if (!isNaN(date.getTime())) {
              years.add(date.getFullYear());
            }
          } catch (e) {
            console.error('Error parsing date:', dateStr);
          }
        }
      });
      
      // Get years from bins
      const binsSnapshot = await getDocs(collection(db, 'bins'));
      binsSnapshot.docs.forEach((doc) => {
        const data = doc.data();
        const createdAt = data.createdAt as Timestamp;
        if (createdAt) {
          years.add(createdAt.toDate().getFullYear());
        }
      });
      
      return Array.from(years).sort();
    } catch (error) {
      console.error('Error getting available years:', error);
      return [];
    }
  }

  async getMonthlyRecyclingStats(year?: number): Promise<{
    plastic: { data: number[]; users: number[] };
    glass: { data: number[]; users: number[] };
  }> {
    const targetYear = year || new Date().getFullYear();
    
    const monthlyStats = {
      plastic: { data: new Array(12).fill(0), users: new Array(12).fill(0) },
      glass: { data: new Array(12).fill(0), users: new Array(12).fill(0) },
    };
    
    try {
      const snapshot = await getDocs(
        query(collection(db, 'recycling_requests'), where('status', '==', 'approved'))
      );
      
      const userSets = {
        plastic: new Array(12).fill(null).map(() => new Set<string>()),
        glass: new Array(12).fill(null).map(() => new Set<string>()),
      };
      
      snapshot.docs.forEach((doc) => {
        const data = doc.data();
        const timestamp = data.timestamp as Timestamp;
        const quantity = data.quantity || 0;
        const weight = data.weight || 0;
        const userId = data.userId || '';
        const materialType = data.materialType?.toLowerCase() || '';
        
        if (timestamp) {
          const dateTime = timestamp.toDate();
          if (dateTime.getFullYear() === targetYear) {
            const monthIndex = dateTime.getMonth();
            
            if (materialType === 'plastic' || materialType === 'plastic bottle') {
              monthlyStats.plastic.data[monthIndex] += weight;
              userSets.plastic[monthIndex].add(userId);
            } else if (materialType === 'glass' || materialType === 'tin can' || materialType === 'metal') {
              monthlyStats.glass.data[monthIndex] += quantity;
              userSets.glass[monthIndex].add(userId);
            }
          }
        }
      });
      
      // Convert sets to counts
      monthlyStats.plastic.users = userSets.plastic.map(set => set.size);
      monthlyStats.glass.users = userSets.glass.map(set => set.size);
      
      return monthlyStats;
    } catch (error) {
      throw new Error(`Failed to get monthly recycling stats: ${error}`);
    }
  }

  // Pricing
  async updateMaterialPricing(material: string, price: number): Promise<void> {
    try {
      await updateDoc(doc(db, 'pricing', 'current'), {
        [material]: price,
        updatedAt: serverTimestamp(),
      });
    } catch (error) {
      throw new Error(`Failed to update pricing for ${material}: ${error}`);
    }
  }

  async updateConversionRate(rate: number): Promise<void> {
    try {
      await updateDoc(doc(db, 'pricing', 'current'), {
        conversionRate: rate,
        updatedAt: serverTimestamp(),
      });
    } catch (error) {
      throw new Error(`Failed to update conversion rate: ${error}`);
    }
  }

  async getCurrentPricing(): Promise<Pricing | null> {
    try {
      const docSnap = await getDoc(doc(db, 'pricing', 'current'));
      if (docSnap.exists()) {
        const data = docSnap.data() as Pricing;
        // If conversionRate doesn't exist, set a default value
        if (data.conversionRate === undefined) {
          data.conversionRate = 10; // Default conversion rate
          // Update the document with the default conversion rate
          await updateDoc(doc(db, 'pricing', 'current'), {
            conversionRate: 10,
            updatedAt: serverTimestamp(),
          });
        }
        return data;
      } else {
        // Create initial pricing document if it doesn't exist
        const initialPricing = {
          plastic: 1,
          glass: 10,
          conversionRate: 10,
          updatedAt: serverTimestamp(),
        };
        await setDoc(doc(db, 'pricing', 'current'), initialPricing);
        return initialPricing as Pricing;
      }
    } catch (error) {
      console.error('Error fetching current pricing:', error);
      return null;
    }
  }

  // Activity Logs
  async fetchActivityData(): Promise<ActivityLog[]> {
    try {
      const snapshot = await getDocs(collection(db, 'userActivities'));
      return snapshot.docs.map((doc) => ({
        id: doc.id,
        ...doc.data()
      } as ActivityLog));
    } catch (error) {
      console.error('Error fetching activity logs:', error);
      return [];
    }
  }

  async getActivityLogsByYear(year: number): Promise<ActivityLog[]> {
    try {
      const snapshot = await getDocs(collection(db, 'userActivities'));
      const yearLogs: ActivityLog[] = [];
      
      snapshot.docs.forEach((doc) => {
        const data = doc.data();
        const dateStr = data.date as string;
        
        if (dateStr) {
          try {
            const date = new Date(dateStr);
            if (date.getFullYear() === year) {
              yearLogs.push({
                id: doc.id,
                ...data
              } as ActivityLog);
            }
          } catch (e) {
            console.error('Error parsing date:', dateStr);
          }
        }
      });
      
      return yearLogs;
    } catch (error) {
      console.error('Error fetching activity logs for year:', error);
      return [];
    }
  }

  // Admin Cash Balance
  async getAdminCashBalance(): Promise<number> {
    try {
      const snapshot = await getDocs(
        query(collection(db, 'admin_transactions'), orderBy('timestamp', 'desc'))
      );
      
      let balance = 100000.0; // Start with 100,000
      snapshot.docs.forEach((doc) => {
        const data = doc.data();
        const type = data.type;
        const amount = data.amount || 0;
        
        if (type === 'add') {
          balance += amount;
        } else if (type === 'withdraw') {
          balance -= amount;
        }
      });
      
      return balance;
    } catch (error) {
      console.error('Error getting admin cash balance:', error);
      return 100000.0;
    }
  }

  // Add funds to admin balance
  async addFunds(amount: number, description?: string): Promise<void> {
    try {
      await addDoc(collection(db, 'admin_transactions'), {
        type: 'add',
        amount: amount,
        description: description || `Admin added ₱${amount} to balance`,
        timestamp: serverTimestamp(),
      });
    } catch (error) {
      throw new Error(`Failed to add funds: ${error}`);
    }
  }

  // Process user withdrawal and decrease admin balance
  async processUserWithdrawal(userId: string, userEmail: string, pointsRedeemed: number, amount: number): Promise<void> {
    try {
      // Add withdrawal transaction to admin_transactions
      await addDoc(collection(db, 'admin_transactions'), {
        type: 'withdraw',
        amount: amount,
        description: `User redemption: ${userEmail} redeemed ${pointsRedeemed} points for ₱${amount}`,
        timestamp: serverTimestamp(),
        userId: userId,
        userEmail: userEmail,
        pointsRedeemed: pointsRedeemed,
      });
    } catch (error) {
      throw new Error(`Failed to process user withdrawal: ${error}`);
    }
  }

  // Transaction Stats
  async getTransactionStats(): Promise<{
    totalTransactions: number;
    totalAmount: number;
    pendingCount: number;
    failedCount: number;
  }> {
    try {
      const snapshot = await getDocs(collection(db, 'transactions'));
      
      let totalTransactions = snapshot.docs.length;
      let totalAmount = 0;
      let pendingCount = 0;
      let failedCount = 0;
      
      snapshot.docs.forEach((doc) => {
        const data = doc.data();
        const amount = data.amount || 0;
        const status = data.status?.toLowerCase() || '';
        
        totalAmount += amount;
        
        if (status === 'pending') {
          pendingCount++;
        } else if (status === 'failed') {
          failedCount++;
        }
      });
      
      return {
        totalTransactions,
        totalAmount,
        pendingCount,
        failedCount,
      };
    } catch (error) {
      console.error('Error getting transaction stats:', error);
      return {
        totalTransactions: 0,
        totalAmount: 0,
        pendingCount: 0,
        failedCount: 0,
      };
    }
  }

  // Get all transactions from admin_transactions
  async getTransactions(): Promise<any[]> {
    try {
      const snapshot = await getDocs(
        query(collection(db, 'admin_transactions'), orderBy('timestamp', 'desc'))
      );
      
      return snapshot.docs.map(doc => ({
        id: doc.id,
        ...doc.data()
      }));
    } catch (error) {
      console.error('Error getting transactions:', error);
      return [];
    }
  }

  // Get admin transactions
  async getAdminTransactions(): Promise<any[]> {
    try {
      const snapshot = await getDocs(
        query(collection(db, 'admin_transactions'), orderBy('timestamp', 'desc'))
      );
      
      return snapshot.docs.map(doc => ({
        id: doc.id,
        ...doc.data()
      }));
    } catch (error) {
      console.error('Error getting admin transactions:', error);
      return [];
    }
  }

  // Get user cards (if they exist in a cards collection)
  async getUserCards(): Promise<any[]> {
    try {
      const snapshot = await getDocs(collection(db, 'cards'));
      
      return snapshot.docs.map(doc => ({
        id: doc.id,
        ...doc.data()
      }));
    } catch (error) {
      console.error('Error getting user cards:', error);
      return [];
    }
  }

  // User Stats
  async getUserStats(): Promise<{
    totalUsers: number;
    activeUsers: number;
    inactiveUsers: number;
  }> {
    try {
      const snapshot = await getDocs(collection(db, 'userActivities'));
      
      const uniqueUsers = new Set<string>();
      let activeUsers = 0;
      
      snapshot.docs.forEach((doc) => {
        const data = doc.data();
        const userId = data.userId || '';
        
        if (userId) {
          uniqueUsers.add(userId);
          if (data.action === 'Online') {
            activeUsers++;
          }
        }
      });
      
      const totalUsers = uniqueUsers.size;
      const inactiveUsers = totalUsers - activeUsers;
      
      return {
        totalUsers,
        activeUsers,
        inactiveUsers,
      };
    } catch (error) {
      console.error('Error fetching user stats:', error);
      return {
        totalUsers: 0,
        activeUsers: 0,
        inactiveUsers: 0,
      };
    }
  }

  // Utility functions
  encodeImage(imageBytes: Uint8Array): string {
    return Buffer.from(imageBytes).toString('base64');
  }

  async calculatePoints(materialType: string, metalType: string | null, weight: number, quantity: number | null): Promise<number> {
    try {
      const pricingDoc = await getDoc(doc(db, 'pricing', 'current'));
      
      if (!pricingDoc.exists()) {
        throw new Error('Pricing information not found');
      }
      
      const pricing = pricingDoc.data() as Pricing;
      
      if (materialType.toLowerCase() === 'glass') {
        const pricePerItem = pricing.glass;
        return Math.round(pricePerItem * (quantity || 0));
      } else if (materialType.toLowerCase() === 'metal') {
        const metalPrices = pricing.metal || {};
        const pricePerKg = metalPrices[metalType || 'Other'] || 0;
        return Math.round(pricePerKg * weight);
      } else {
        const pricePerKg = (pricing as any)[materialType.toLowerCase()] || 0;
        return Math.round(pricePerKg * weight);
      }
    } catch (error) {
      console.error('Error calculating points:', error);
      return 0;
    }
  }
}
