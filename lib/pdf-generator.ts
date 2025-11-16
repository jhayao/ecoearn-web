import jsPDF from 'jspdf';
import html2canvas from 'html2canvas';
import { AdminService } from './admin-service';

export interface DashboardData {
  selectedYear: number;
  userStats: {
    totalUsers: number;
    activeUsers: number;
    inactiveUsers: number;
    userReports: number;
  };
  recyclingStats: {
    plastic: number;
    glass: number;
  };
  barChartData: Array<{
    name: string;
    value: number;
    color: string;
  }>;
  generatedAt: Date;
  // Additional data for comprehensive report
  userReports?: Array<{
    username: string;
    date: string;
    time: string;
    description: string;
    location: string;
    upload: string;
  }>;
  recentRecycles?: Array<{
    username: string;
    date: string;
    material: string;
  }>;
  binInformation?: Array<{
    binName: string;
    latitude: number;
    longitude: number;
    binLevel: number;
  }>;
  monthlyData?: Array<{
    month: string;
    plasticBottles: number;
    tinCans: number;
  }>;
  pricing?: {
    plasticBottle: string;
    tinCan: string;
  };
  activityLogs?: Array<{
    email: string;
    ipAddress: string;
    date: string;
    time: string;
    action: string;
    description: string;
  }>;
}

export class PDFReportGenerator {
  private doc: jsPDF;
  private pageWidth: number;
  private pageHeight: number;
  private margin: number;
  private currentY: number;

  constructor() {
    this.doc = new jsPDF('p', 'mm', 'a4');
    this.pageWidth = this.doc.internal.pageSize.getWidth();
    this.pageHeight = this.doc.internal.pageSize.getHeight();
    this.margin = 10;
    this.currentY = this.margin;
  }

  private addHeader(data: DashboardData): void {
    const leftLogoHeight = 20;
    const leftLogoWidth = 20;
    const rightLogoHeight = 35;
    const rightLogoWidth = 35;
    const headerHeight = 50;
    
    // Add logos and title in a three-column layout
    try {
      // Left: USTP Logo (smaller)
      this.doc.addImage('/ustp-logo.png', 'PNG', this.margin, this.currentY + 5, leftLogoWidth, leftLogoHeight);
      
      // Right: EcoEarn Logo (larger)
      this.doc.addImage('/ecoearn-logo.png', 'PNG', this.pageWidth - this.margin - rightLogoWidth, this.currentY, rightLogoWidth, rightLogoHeight);
      
      // Center: Report Title and Timestamp
      this.doc.setFontSize(20);
      this.doc.setFont('helvetica', 'bold');
      this.doc.text('WASTE MANAGEMENT REPORT', this.pageWidth / 2, this.currentY + 8, { align: 'center' });
      
      this.doc.setFontSize(12);
      this.doc.setFont('helvetica', 'normal');
      this.doc.text(`Generated: ${data.generatedAt.toLocaleDateString()} ${data.generatedAt.toLocaleTimeString()}`, this.pageWidth / 2, this.currentY + 22, { align: 'center' });
      
      this.doc.text(`Year: ${data.selectedYear}`, this.pageWidth / 2, this.currentY + 32, { align: 'center' });
      
    } catch (error) {
      console.warn('Could not load logos, using text-only header:', error);
      
      // Fallback to text-only header if logos fail to load
      this.doc.setFontSize(20);
      this.doc.setFont('helvetica', 'bold');
      this.doc.text('WASTE MANAGEMENT REPORT', this.pageWidth / 2, this.currentY, { align: 'center' });
      
      this.currentY += 15;
      
      this.doc.setFontSize(12);
      this.doc.setFont('helvetica', 'normal');
      this.doc.text(`Generated on: ${data.generatedAt.toLocaleDateString()} ${data.generatedAt.toLocaleTimeString()}`, this.margin, this.currentY);
      this.doc.text(`Year: ${data.selectedYear}`, this.pageWidth - this.margin - 30, this.currentY);
    }
    
    this.currentY += headerHeight;
  }

  private addSummaryStats(data: DashboardData): void {
    // Section Title
    this.doc.setFontSize(16);
    this.doc.setFont('helvetica', 'bold');
    this.doc.text('Executive Summary', this.margin, this.currentY);
    this.currentY += 10;

    // Stats Grid
    const stats = [
      { label: 'Total Users', value: data.userStats.totalUsers, color: '#3B82F6' },
      { label: 'Active Users', value: data.userStats.activeUsers, color: '#10B981' },
      { label: 'User Reports', value: data.userStats.userReports, color: '#EF4444' },
      { label: 'Inactive Users', value: data.userStats.inactiveUsers, color: '#F59E0B' }
    ];

    const boxWidth = (this.pageWidth - 2 * this.margin - 30) / 4;
    const boxHeight = 25;

    stats.forEach((stat, index) => {
      const x = this.margin + index * (boxWidth + 10);
      const y = this.currentY;

      // Box background
      const [r, g, b] = this.hexToRgb(stat.color);
      this.doc.setFillColor(r, g, b);
      this.doc.roundedRect(x, y, boxWidth, boxHeight, 3, 3, 'F');

      // Text
      this.doc.setTextColor(255, 255, 255);
      this.doc.setFontSize(12);
      this.doc.setFont('helvetica', 'bold');
      this.doc.text(stat.value.toString(), x + 5, y + 10);

      this.doc.setFontSize(8);
      this.doc.setFont('helvetica', 'normal');
      this.doc.text(stat.label, x + 5, y + 18);
    });

    this.currentY += boxHeight + 15;
  }

  private addRecyclingStats(data: DashboardData): void {
    // Section Title
    this.doc.setFontSize(16);
    this.doc.setFont('helvetica', 'bold');
    this.doc.setTextColor(0, 0, 0);
    this.doc.text('Recycling Statistics', this.margin, this.currentY);
    this.currentY += 10;

    // Calculate percentages
    const totalRecycling = data.recyclingStats.plastic + data.recyclingStats.glass;
    const plasticPercentage = totalRecycling > 0 ? ((data.recyclingStats.plastic / totalRecycling) * 100).toFixed(1) : '0.0';
    const glassPercentage = totalRecycling > 0 ? ((data.recyclingStats.glass / totalRecycling) * 100).toFixed(1) : '0.0';

    // Recycling data table
    const tableData = [
      ['Material Type', 'Quantity (kg)', 'Percentage'],
      ['Plastic', data.recyclingStats.plastic.toString(), `${plasticPercentage}%`],
      ['Glass/Tin Cans', data.recyclingStats.glass.toString(), `${glassPercentage}%`],
      ['Total', totalRecycling.toString(), '100.0%']
    ];

    const tableWidth = this.pageWidth - 2 * this.margin;
    const colWidth = tableWidth / 3;
    const rowHeight = 8;

    tableData.forEach((row, rowIndex) => {
      const y = this.currentY + (rowIndex * rowHeight);
      
      row.forEach((cell, colIndex) => {
        const x = this.margin + (colIndex * colWidth);
        
        if (rowIndex === 0) {
          // Header row
          this.doc.setFillColor(59, 130, 246);
          this.doc.rect(x, y, colWidth, rowHeight, 'F');
          this.doc.setTextColor(255, 255, 255);
          this.doc.setFont('helvetica', 'bold');
        } else if (rowIndex === tableData.length - 1) {
          // Total row
          this.doc.setFillColor(240, 240, 240);
          this.doc.rect(x, y, colWidth, rowHeight, 'F');
          this.doc.setTextColor(0, 0, 0);
          this.doc.setFont('helvetica', 'bold');
        } else {
          this.doc.setTextColor(0, 0, 0);
          this.doc.setFont('helvetica', 'normal');
        }
        
        this.doc.setFontSize(10);
        this.doc.text(cell, x + 2, y + 5);
      });
    });

    this.currentY += (tableData.length * rowHeight) + 15;
  }

  private addBarChart(data: DashboardData): void {
    // Section Title
    this.doc.setFontSize(16);
    this.doc.setFont('helvetica', 'bold');
    this.doc.text('Recycling by Material Type', this.margin, this.currentY);
    this.currentY += 10;

    // Simple bar chart representation
    const maxValue = Math.max(...data.barChartData.map(item => item.value));
    const chartWidth = this.pageWidth - 2 * this.margin;
    const chartHeight = 60;
    const barWidth = 30;
    const barSpacing = 20;

    data.barChartData.forEach((item, index) => {
      const barHeight = (item.value / maxValue) * chartHeight;
      const x = this.margin + index * (barWidth + barSpacing);
      const y = this.currentY + chartHeight - barHeight;

      // Bar
      const [r, g, b] = this.hexToRgb(item.color);
      this.doc.setFillColor(r, g, b);
      this.doc.rect(x, y, barWidth, barHeight, 'F');

      // Label
      this.doc.setTextColor(0, 0, 0);
      this.doc.setFontSize(10);
      this.doc.setFont('helvetica', 'normal');
      this.doc.text(item.name, x, this.currentY + chartHeight + 5);
      this.doc.text(item.value.toString(), x, this.currentY + chartHeight + 15);
    });

    this.currentY += chartHeight + 30;
  }

  private addFooter(): void {
    const footerY = this.pageHeight - 20;
    
    this.doc.setFontSize(8);
    this.doc.setFont('helvetica', 'italic');
    this.doc.setTextColor(128, 128, 128);
    this.doc.text('This report was generated by EcoEarn Admin Dashboard', this.margin, footerY);
    this.doc.text(`Page ${this.doc.getCurrentPageInfo().pageNumber}`, this.pageWidth - this.margin - 20, footerY);
  }

  private hexToRgb(hex: string): [number, number, number] {
    const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? [
      parseInt(result[1], 16),
      parseInt(result[2], 16),
      parseInt(result[3], 16)
    ] : [0, 0, 0];
  }

  private checkPageBreak(requiredSpace: number): void {
    if (this.currentY + requiredSpace > this.pageHeight - 30) {
      this.doc.addPage();
      this.currentY = this.margin;
    }
  }

  private addExecutiveSummary(data: DashboardData): void {
    // Section Title
    this.doc.setFontSize(16);
    this.doc.setFont('helvetica', 'bold');
    this.doc.setTextColor(0, 0, 0);
    this.doc.text('1. Executive Summary', this.margin, this.currentY);
    this.currentY += 10;

    // Summary text
    this.doc.setFontSize(11);
    this.doc.setFont('helvetica', 'normal');
    this.doc.setTextColor(50, 50, 50);
    
    const summaryText = "This report provides an overview of the waste management system, including user activity, waste collection statistics, and recent recycling activities. The data is compiled to ensure effective monitoring and management of waste collection processes.";
    
    this.doc.text(summaryText, this.margin, this.currentY, { maxWidth: this.pageWidth - 2 * this.margin });
    this.currentY += 20;
  }

  private addCurrentPricing(data: DashboardData): void {
    // Section Title
    this.doc.setFontSize(16);
    this.doc.setFont('helvetica', 'bold');
    this.doc.setTextColor(0, 0, 0);
    this.doc.text('2. Current Pricing', this.margin, this.currentY);
    this.currentY += 10;

    // Pricing information
    this.doc.setFontSize(11);
    this.doc.setFont('helvetica', 'normal');
    this.doc.setTextColor(50, 50, 50);
    
    const pricing = data.pricing || {
      plasticBottle: "1 points = 1 PHP",
      tinCan: "1 points = 1 PHP"
    };
    
    this.doc.text(`Plastic Bottle: ${pricing.plasticBottle}`, this.margin, this.currentY);
    this.currentY += 6;
    this.doc.text(`Tin Can: ${pricing.tinCan}`, this.margin, this.currentY);
    this.currentY += 20;
  }

  private addWasteCollectionSummary(data: DashboardData): void {
    // Section Title
    this.doc.setFontSize(16);
    this.doc.setFont('helvetica', 'bold');
    this.doc.setTextColor(0, 0, 0);
    this.doc.text('3. Waste Collection Summary', this.margin, this.currentY);
    this.currentY += 5;
    
    // Subtitle
    this.doc.setFontSize(12);
    this.doc.setFont('helvetica', 'italic');
    this.doc.text(`Monthly Waste Collection Data (${data.selectedYear})`, this.margin, this.currentY);
    this.currentY += 10;

    // Monthly data table
    const monthlyData = data.monthlyData || this.generateDefaultMonthlyData(data);
    
    const tableData = [
      ['Month', 'Plastic Bottle (pcs)', 'Tin Can (pcs)'],
      ...monthlyData.map(month => [
        month.month,
        month.plasticBottles.toString(),
        month.tinCans.toString()
      ])
    ];

    this.addTable(tableData);
    
    // Summary totals
    const totalPlastic = monthlyData.reduce((sum, month) => sum + month.plasticBottles, 0);
    const totalTinCans = monthlyData.reduce((sum, month) => sum + month.tinCans, 0);
    
    this.currentY += 5;
    this.doc.setFontSize(10);
    this.doc.setFont('helvetica', 'normal');
    this.doc.text(`Total Plastic Bottles: ${totalPlastic}`, this.margin, this.currentY);
    this.doc.text(`Total Tin Cans: ${totalTinCans}`, this.margin, this.currentY + 6);
    
    // Peak months
    const peakPlasticMonth = monthlyData.reduce((max, month) => 
      month.plasticBottles > max.plasticBottles ? month : max, monthlyData[0]);
    const peakTinCanMonth = monthlyData.reduce((max, month) => 
      month.tinCans > max.tinCans ? month : max, monthlyData[0]);
    
    this.doc.text(`Peak Month (Plastic): ${peakPlasticMonth.month}`, this.pageWidth - this.margin - 60, this.currentY);
    this.doc.text(`Peak Month (Tin Can): ${peakTinCanMonth.month}`, this.pageWidth - this.margin - 60, this.currentY + 6);
    
    this.currentY += 20;
  }

  private addRecentRecycles(data: DashboardData): void {
    // Section Title
    this.doc.setFontSize(16);
    this.doc.setFont('helvetica', 'bold');
    this.doc.setTextColor(0, 0, 0);
    this.doc.text('4. Recent Recycles', this.margin, this.currentY);
    this.currentY += 10;

    // Recent recycles table
    const recentRecycles = data.recentRecycles || this.generateDefaultRecentRecycles();
    
    const tableData = [
      ['Username', 'Date', 'Material'],
      ...recentRecycles.map(recycle => [
        recycle.username,
        recycle.date,
        recycle.material
      ])
    ];

    this.addTable(tableData);
    this.currentY += 15;
  }

  private addUserReports(data: DashboardData): void {
    // Section Title
    this.doc.setFontSize(16);
    this.doc.setFont('helvetica', 'bold');
    this.doc.setTextColor(0, 0, 0);
    this.doc.text('5. User Reports', this.margin, this.currentY);
    this.currentY += 10;

    // User reports table
    const userReports = data.userReports || this.generateDefaultUserReports();
    
    const tableData = [
      ['Username', 'Date', 'Time', 'Description', 'Location', 'Upload'],
      ...userReports.map(report => [
        report.username,
        report.date,
        report.time,
        report.description,
        report.location,
        report.upload
      ])
    ];

    this.addTable(tableData);
    this.currentY += 15;
  }

  private addActivityLogs(data: DashboardData): void {
    // Section Title
    this.doc.setFontSize(16);
    this.doc.setFont('helvetica', 'bold');
    this.doc.setTextColor(0, 0, 0);
    this.doc.text('6. Activity Logs', this.margin, this.currentY);
    this.currentY += 10;

    // Activity logs table
    const activityLogs = data.activityLogs || this.generateDefaultActivityLogs();
    
    const tableData = [
      ['Email', 'IP Address', 'Date & Time', 'Action', 'Description'],
      ...activityLogs.map(activity => [
        activity.email,
        this.formatIPForPDF(activity.ipAddress),
        `${activity.date} ${activity.time}`,
        activity.action,
        activity.description
      ])
    ];

    this.addTable(tableData);
    this.currentY += 15;
  }

  private addBinInformation(data: DashboardData): void {
    // Section Title
    this.doc.setFontSize(16);
    this.doc.setFont('helvetica', 'bold');
    this.doc.setTextColor(0, 0, 0);
    this.doc.text('7. Bin Information', this.margin, this.currentY);
    this.currentY += 10;

    // Bin information table
    const binInformation = data.binInformation || this.generateDefaultBinInformation();
    
    const tableData = [
      ['Bin Name', 'Latitude', 'Longitude', 'Bin Level (%)'],
      ...binInformation.map(bin => [
        bin.binName,
        bin.latitude.toString(),
        bin.longitude.toString(),
        `${bin.binLevel}%`
      ])
    ];

    this.addTable(tableData);
    this.currentY += 15;
  }

  private formatIPForPDF(ip: string): string {
    if (!ip || ip === 'N/A') return 'N/A';
    
    // Check if it's IPv6 (contains colons)
    if (ip.includes(':')) {
      const parts = ip.split(':');
      if (parts.length > 3) {
        return `${parts.slice(0, 3).join(':')}...`;
      }
    }
    
    return ip;
  }

  private addTable(tableData: string[][]): void {
    const tableWidth = this.pageWidth - 2 * this.margin;
    const colCount = tableData[0].length;
    const colWidth = tableWidth / colCount;
    const rowHeight = 8;

    tableData.forEach((row, rowIndex) => {
      this.checkPageBreak(rowHeight + 5);
      const y = this.currentY + (rowIndex * rowHeight);
      
      row.forEach((cell, colIndex) => {
        const x = this.margin + (colIndex * colWidth);
        
        if (rowIndex === 0) {
          // Header row
          this.doc.setFillColor(59, 130, 246);
          this.doc.rect(x, y, colWidth, rowHeight, 'F');
          this.doc.setTextColor(255, 255, 255);
          this.doc.setFont('helvetica', 'bold');
        } else {
          this.doc.setTextColor(0, 0, 0);
          this.doc.setFont('helvetica', 'normal');
        }
        
        this.doc.setFontSize(9);
        // Ensure cell value is a valid string, handle null/undefined values
        const cellText = cell == null ? 'N/A' : String(cell);
        this.doc.text(cellText, x + 2, y + 5);
      });
    });

    this.currentY += (tableData.length * rowHeight) + 10;
  }

  private generateDefaultMonthlyData(data: DashboardData) {
    const months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
    return months.map(month => ({
      month,
      plasticBottles: month === 'Dec' ? 1068 : month === 'Nov' ? 0 : Math.floor(Math.random() * 100),
      tinCans: month === 'Dec' ? 87 : month === 'Nov' ? 75 : Math.floor(Math.random() * 50)
    }));
  }

  private generateDefaultRecentRecycles() {
    return [
      { username: 'demrek', date: '2024-12-18 | 13:35', material: 'plastic' },
      { username: 'demrek', date: '2024-12-10 | 16:58', material: 'glass' },
      { username: 'Kian', date: '2024-12-10 | 15:50', material: 'plastic' },
      { username: 'Harold', date: '2024-12-10 | 14:38', material: 'plastic' },
      { username: 'Harold', date: '2024-12-10 | 14:37', material: 'metal' }
    ];
  }

  private generateDefaultUserReports() {
    return [
      { username: 'user1', date: '2024-12-18', time: '13:35', description: 'Bin overflow', location: 'Location A', upload: 'Yes' },
      { username: 'user2', date: '2024-12-17', time: '16:58', description: 'Damaged bin', location: 'Location B', upload: 'No' },
      { username: 'user3', date: '2024-12-16', time: '15:50', description: 'Full bin', location: 'Location C', upload: 'Yes' }
    ];
  }

  private generateDefaultActivityLogs() {
    return [
      { email: 'user1@gmail.com', ipAddress: '192.168.1.1', date: '2024-12-18', time: '13:35', action: 'Online', description: 'User signed in and is now online.' },
      { email: 'user2@gmail.com', ipAddress: '2405:8d40:4919:1eb6:e578:2ebe:6618:29e2', date: '2024-12-17', time: '16:58', action: 'Offline', description: 'User signed out and is now offline.' },
      { email: 'user3@ustp.edu.ph', ipAddress: '175.176.85.165', date: '2024-12-16', time: '15:50', action: 'Online', description: 'User signed up and is now online.' }
    ];
  }

  private generateDefaultBinInformation() {
    return [
      { binName: 'Bin-001', latitude: 8.4855, longitude: 123.8064, binLevel: 75 },
      { binName: 'Bin-002', latitude: 8.4865, longitude: 123.8074, binLevel: 45 },
      { binName: 'Bin-003', latitude: 8.4875, longitude: 123.8084, binLevel: 90 }
    ];
  }

  public async generateReport(data: DashboardData): Promise<void> {
    try {
      // Add header
      this.addHeader(data);
      
      // Add executive summary
      this.checkPageBreak(50);
      this.addExecutiveSummary(data);
      
      // Add current pricing
      this.checkPageBreak(30);
      this.addCurrentPricing(data);
      
      // Add waste collection summary (fits on first page)
      this.checkPageBreak(120);
      this.addWasteCollectionSummary(data);
      
      // Add recent recycles
      this.checkPageBreak(100);
      this.addRecentRecycles(data);
      
      // Add user reports
      this.checkPageBreak(100);
      this.addUserReports(data);
      
      // Add activity logs
      this.checkPageBreak(100);
      this.addActivityLogs(data);
      
      // Add bin information
      this.checkPageBreak(100);
      this.addBinInformation(data);
      
      // Add charts section
      this.checkPageBreak(100);
      this.addChartsSection(data);
      
      // Add footer to all pages
      const totalPages = this.doc.getNumberOfPages();
      for (let i = 1; i <= totalPages; i++) {
        this.doc.setPage(i);
        this.addFooter();
      }
      
    } catch (error) {
      console.error('Error generating PDF report:', error);
      throw error;
    }
  }

  private addChartsSection(data: DashboardData): void {
    // Section Title
    this.doc.setFontSize(16);
    this.doc.setFont('helvetica', 'bold');
    this.doc.setTextColor(0, 0, 0);
    this.doc.text('7. Charts and Analytics', this.margin, this.currentY);
    this.currentY += 10;

    // Add summary stats
    this.addSummaryStats(data);
    
    // Add recycling stats
    this.checkPageBreak(40);
    this.addRecyclingStats(data);
    
    // Add bar chart
    this.checkPageBreak(100);
    this.addBarChart(data);
  }

  public saveReport(filename?: string): void {
    const defaultFilename = `EcoEarn_Report_${new Date().getFullYear()}_${String(new Date().getMonth() + 1).padStart(2, '0')}_${String(new Date().getDate()).padStart(2, '0')}.pdf`;
    this.doc.save(filename || defaultFilename);
  }

  public getBlob(): Blob {
    return this.doc.output('blob');
  }
}

export async function generateDashboardPDF(
  dashboardElement: HTMLElement,
  data: DashboardData
): Promise<Blob> {
  try {
    // Generate PDF using the custom generator
    const pdfGenerator = new PDFReportGenerator();
    await pdfGenerator.generateReport(data);
    return pdfGenerator.getBlob();
  } catch (error) {
    console.error('Error generating PDF from dashboard:', error);
    throw error;
  }
}

// New function to fetch comprehensive data from Firestore
export async function fetchComprehensiveData(selectedYear: number): Promise<DashboardData> {
  try {
    const adminService = new AdminService();
    
    // Fetch all required data with error handling
    const [
      userStats,
      recyclingStats,
      userReports,
      recentRecycles,
      binInformation,
      monthlyData,
      pricing,
      activityLogs
    ] = await Promise.all([
      adminService.getUserStats().catch(err => {
        console.error('Error fetching user stats:', err);
        return { totalUsers: 0, activeUsers: 0, inactiveUsers: 0 };
      }),
      adminService.getTotalRecyclingStats().catch(err => {
        console.error('Error fetching recycling stats:', err);
        return { plastic: 0, glass: 0 };
      }),
      adminService.getReportsByYear(selectedYear).catch(err => {
        console.error('Error fetching user reports:', err);
        return [];
      }),
      adminService.getRecentRecycles().catch(err => {
        console.error('Error fetching recent recycles:', err);
        return [];
      }),
      adminService.getBinsByYear(selectedYear).catch(err => {
        console.error('Error fetching bin information:', err);
        return [];
      }),
      adminService.getMonthlyRecyclingStats(selectedYear).catch(err => {
        console.error('Error fetching monthly data:', err);
        return { plastic: { data: new Array(12).fill(0), users: new Array(12).fill(0) }, glass: { data: new Array(12).fill(0), users: new Array(12).fill(0) } };
      }),
      adminService.getCurrentPricing().catch(err => {
        console.error('Error fetching pricing:', err);
        return null;
      }),
      adminService.getActivityLogsByYear(selectedYear).catch(err => {
        console.error('Error fetching activity logs:', err);
        return [];
      })
    ]);

    // Process data with error handling
    let processedUserReports: Array<{
      username: string;
      date: string;
      time: string;
      description: string;
      location: string;
      upload: string;
    }>;
    let processedRecentRecycles: Array<{
      username: string;
      date: string;
      material: string;
    }>;
    let processedBinInformation: Array<{
      binName: string;
      latitude: number;
      longitude: number;
      binLevel: number;
    }>;
    let processedMonthlyData: Array<{
      month: string;
      plasticBottles: number;
      tinCans: number;
    }>;
    let processedPricing: {
      plasticBottle: string;
      tinCan: string;
    };
    let processedActivityLogs: Array<{
      email: string;
      ipAddress: string;
      date: string;
      time: string;
      action: string;
      description: string;
    }>;
    
    try {
      // Process user reports
      processedUserReports = userReports.map(report => ({
        username: report.userName || 'Unknown User',
        date: report.timestamp?.toDate().toLocaleDateString() || 'N/A',
        time: report.timestamp?.toDate().toLocaleTimeString() || 'N/A',
        description: report.description || 'No description',
        location: report.location || 'No location',
        upload: report.image ? 'Yes' : 'No'
      }));

      // Process recent recycles
      processedRecentRecycles = recentRecycles.map(recycle => ({
        username: recycle.userName || 'Unknown User',
        date: recycle.timestamp ? `${recycle.timestamp.toDate().toLocaleDateString()} | ${recycle.timestamp.toDate().toLocaleTimeString()}` : 'N/A',
        material: recycle.materialType || 'Unknown'
      }));

      // Process bin information
      processedBinInformation = binInformation.map(bin => ({
        binName: bin.name || 'Unknown Bin',
        latitude: bin.lat || 0,
        longitude: bin.lng || 0,
        binLevel: bin.level || 0
      }));

      // Process monthly data
      const months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
      processedMonthlyData = months.map((month, index) => ({
        month,
        plasticBottles: monthlyData?.plastic?.data?.[index] || 0,
        tinCans: monthlyData?.glass?.data?.[index] || 0
      }));

      // Process pricing
      processedPricing = pricing ? {
        plasticBottle: `${pricing.plastic || 1} points = 1 PHP`,
        tinCan: `${pricing.glass || 1} points = 1 PHP`
      } : {
        plasticBottle: "1 points = 1 PHP",
        tinCan: "1 points = 1 PHP"
      };

      // Process activity logs
      processedActivityLogs = activityLogs.map(log => ({
        email: log.email || 'Unknown',
        ipAddress: log.ipAddress || 'N/A',
        date: log.date || 'N/A',
        time: log.time || 'N/A',
        action: log.action || 'Unknown',
        description: log.description || 'No description'
      }));
    } catch (processingError) {
      console.error('Error processing data:', processingError);
      // Use default data if processing fails
      processedUserReports = [];
      processedRecentRecycles = [];
      processedBinInformation = [];
      processedMonthlyData = [];
      processedPricing = { plasticBottle: "1 points = 1 PHP", tinCan: "1 points = 1 PHP" };
      processedActivityLogs = [];
    }

    // Create bar chart data
    const barChartData = [
      { 
        name: 'Plastic', 
        value: Number(recyclingStats.plastic) || 0, 
        color: '#7B61FF' 
      },
      { 
        name: 'Tin Cans', 
        value: Number(recyclingStats.glass) || 0, 
        color: '#FFA500' 
      },
    ];

    return {
      selectedYear,
      userStats: {
        ...userStats,
        userReports: userReports.length
      },
      recyclingStats,
      barChartData,
      generatedAt: new Date(),
      userReports: processedUserReports,
      recentRecycles: processedRecentRecycles,
      binInformation: processedBinInformation,
      monthlyData: processedMonthlyData,
      pricing: processedPricing,
      activityLogs: processedActivityLogs
    };
  } catch (error) {
    console.error('Error fetching comprehensive data:', error);
    throw error;
  }
}