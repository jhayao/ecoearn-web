/*
 * EcoEarn Smart Bin - Component Test Sketch (NodeMCU ESP8266 Version)
 * 
 * This sketch tests all hardware components WITHOUT any WiFi, API, or server connections.
 * Use this to verify your hardware setup before deploying the full system.
 * 
 * Components tested:
 * - Sharp 2Y0A21 IR Distance Sensor (User Detection)
 * - 2x HC-SR04 Ultrasonic Sensors (Capacity Monitoring: Plastic & Tin compartments)
 * - 3x Servo Motors:
 *   * 1x MG90S - Bin lid opener (opens/closes main bin lid)
 *   * 1x MG90S - Trash dropper (releases trash into compartment)
 *   * 1x MG995 - Rotating platform (positions trash above plastic/tin/rejected)
 * - Serial Communication (ESP32-CAM ↔ NodeMCU)
 * - LED Status Indicator
 * - GPS Module (optional)
 * 
 * Hardware: NodeMCU ESP8266 Development Board
 * 
 * System Design:
 * - Single entry point with rotating platform
 * - User deposits trash through main lid
 * - Platform rotates to selected compartment
 * - Dropper releases trash into compartment
 * 
 * Usage:
 * 1. Upload this sketch to NodeMCU ESP8266
 * 2. Open Serial Monitor at 115200 baud
 * 3. Follow the on-screen menu to test each component
 */

#include <Servo.h>
#include <SoftwareSerial.h>

// ============================================
// PIN CONFIGURATION (NodeMCU ESP8266)
// ============================================

// User Presence Detection (Sharp 2Y0A21 IR Sensor)
const int USER_DETECTION_PIN = A0;  // Analog pin A0

// Servo Motors (3 different functions)
const int BIN_LID_SERVO_PIN = D3;      // GPIO0 - MG90S: Opens/closes main bin lid
const int DROPPER_SERVO_PIN = D4;      // GPIO2 - MG90S: Drops trash into compartment
const int ROTATOR_SERVO_PIN = D6;      // GPIO12 - MG995: Rotates platform to compartment

// Capacity Sensors (2 compartments)
const int PLASTIC_TRIG_PIN = D7;    // GPIO13
const int PLASTIC_ECHO_PIN = D8;    // GPIO15
const int TIN_TRIG_PIN = D0;        // GPIO16
const int TIN_ECHO_PIN = D5;        // GPIO14

// Status LED
const int LED_PIN = D4;              // GPIO2 (built-in LED on NodeMCU)


// GPS (Optional - comment out if not using)
#define USE_GPS false  // Set to true to test GPS
#if USE_GPS
  #include <TinyGPS++.h>
  const int GPS_RX_PIN = D2;  // GPIO4
  const int GPS_TX_PIN = D1;  // GPIO5
  TinyGPSPlus gps;
  SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);  // Use SoftwareSerial for GPS
#endif

// ============================================
// COMPONENT CONFIGURATION
// ============================================

// Sharp 2Y0A21 IR Sensor (ESP8266 10-bit ADC: 0-1023)
const int USER_DETECTION_THRESHOLD = 400;   // Scaled for 10-bit ADC
const int USER_DETECTION_MIN_ADC = 120;     // Scaled for 10-bit ADC
const int USER_DETECTION_MAX_ADC = 860;     // Scaled for 10-bit ADC

// Ultrasonic Sensors
const float BIN_HEIGHT = 30.0;  // cm

// Servo Positions
// Bin Lid Servo (MG90S) - Opens/closes main bin lid
const int LID_CLOSED = 0;
const int LID_OPEN = 90;

// Dropper Servo (MG90S) - Holds/drops trash
const int DROPPER_HOLD = 0;     // Hold trash
const int DROPPER_RELEASE = 90; // Release trash

// Rotator Servo (MG995) - Positions platform
const int ROTATE_PLASTIC = 0;   // Position for plastic compartment
const int ROTATE_TIN = 90;      // Position for tin compartment
const int ROTATE_REJECTED = 180; // Position for rejected compartment

// Servo Objects
Servo binLidServo;    // MG90S - Main bin lid
Servo dropperServo;   // MG90S - Trash dropper
Servo rotatorServo;   // MG995 - Platform rotator

// ============================================
// GLOBAL VARIABLES
// ============================================

bool testRunning = false;
char menuChoice = '0';

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  printBanner();
  
  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize Sharp IR Sensor
  pinMode(USER_DETECTION_PIN, INPUT);
  
  // Initialize Ultrasonic Sensors
  pinMode(PLASTIC_TRIG_PIN, OUTPUT);
  pinMode(PLASTIC_ECHO_PIN, INPUT);
  pinMode(TIN_TRIG_PIN, OUTPUT);
  pinMode(TIN_ECHO_PIN, INPUT);
  
  // Initialize Servos
  binLidServo.attach(BIN_LID_SERVO_PIN);
  dropperServo.attach(DROPPER_SERVO_PIN);
  rotatorServo.attach(ROTATOR_SERVO_PIN);
  
  // Set servos to initial positions
  binLidServo.write(LID_CLOSED);      // Lid closed
  dropperServo.write(DROPPER_HOLD);   // Holding trash
  rotatorServo.write(ROTATE_PLASTIC); // Default to plastic position
  
  #if USE_GPS
    gpsSerial.begin(9600);  // ESP8266 SoftwareSerial
    Serial.println("[GPS] Module initialized");
  #endif
  
  Serial.println("\n[INIT] All components initialized!");
  Serial.println("[INIT] NodeMCU ESP8266 ready!");
  Serial.println("[INIT] Initialization complete!\n");
  
  blinkLED(3, 200);
  
  printMenu();
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {
  if (Serial.available() > 0) {
    menuChoice = Serial.read();
    handleMenuChoice(menuChoice);
  }
  
  delay(10);
}

// ============================================
// MENU SYSTEM
// ============================================

void printBanner() {
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════════╗");
  Serial.println("║   EcoEarn Smart Bin - Component Tester    ║");
  Serial.println("║   NodeMCU ESP8266 Hardware Verification   ║");
  Serial.println("╚════════════════════════════════════════════╝");
  Serial.println();
}

void printMenu() {
  Serial.println("\n┌──────────────────────────────────────────┐");
  Serial.println("│          COMPONENT TEST MENU             │");
  Serial.println("├──────────────────────────────────────────┤");
  Serial.println("│ 1 - Test Sharp IR Sensor (User Detection)│");
  Serial.println("│ 2 - Test Plastic Ultrasonic Sensor       │");
  Serial.println("│ 3 - Test Tin Ultrasonic Sensor           │");
  Serial.println("│ 4 - Test Bin Lid Servo (MG90S)           │");
  Serial.println("│ 5 - Test Dropper Servo (MG90S)           │");
  Serial.println("│ 6 - Test Rotator Servo (MG995)           │");
  Serial.println("│ 7 - Test ALL Servos (Sequential)         │");
  Serial.println("│ 8 - Test LED Indicator                   │");
  Serial.println("│ 9 - Run Full System Test                 │");
  #if USE_GPS
  Serial.println("│ G - Test GPS Module                      │");
  #endif
  Serial.println("│ M - Show this Menu                       │");
  Serial.println("│ S - Stop current test                    │");
  Serial.println("└──────────────────────────────────────────┘");
  Serial.println("\nEnter your choice:");
}

void handleMenuChoice(char choice) {
  Serial.print("\n[CMD] Selected: ");
  Serial.println(choice);
  
  switch(choice) {
    case '1':
      testSharpIRSensor();
      break;
    case '2':
      testPlasticUltrasonic();
      break;
    case '3':
      testTinUltrasonic();
      break;
    case '4':
      testBinLidServo();
      break;
    case '5':
      testDropperServo();
      break;
    case '6':
      testRotatorServo();
      break;
    case '7':
      testAllServos();
      break;
    case '8':
      testLED();
      break;
    case '9':
      runFullSystemTest();
      break;
    #if USE_GPS
    case 'G':
    case 'g':
      testGPS();
      break;
    #endif
    case 'M':
    case 'm':
      printMenu();
      break;
    case 'S':
    case 's':
      testRunning = false;
      Serial.println("[STOP] Test stopped by user");
      printMenu();
      break;
    case '\n':
    case '\r':
      // Ignore newline characters
      break;
    default:
      Serial.println("[ERROR] Invalid choice! Press 'M' for menu.");
      break;
  }
}

// ============================================
// TEST FUNCTIONS
// ============================================

void testSharpIRSensor() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   Testing Sharp 2Y0A21 IR Sensor      ║");
  Serial.println("║   (ESP8266 10-bit ADC: 0-1023)        ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("\nMove your hand in front of the sensor...");
  Serial.println("Reading values for 10 seconds...");
  Serial.println("Press 'S' to stop\n");
  
  testRunning = true;
  unsigned long startTime = millis();
  
  Serial.println("ADC Value | Approx Distance | Status");
  Serial.println("----------|-----------------|--------");
  
  while (testRunning && (millis() - startTime < 10000)) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == 'S' || c == 's') {
        testRunning = false;
        break;
      }
    }
    
    int adcValue = analogRead(USER_DETECTION_PIN);
    String distance = estimateDistance(adcValue);
    String status = (adcValue >= USER_DETECTION_THRESHOLD) ? "USER DETECTED" : "No user";
    
    Serial.print("   ");
    Serial.print(adcValue);
    Serial.print("    | ");
    Serial.print(distance);
    Serial.print(" | ");
    Serial.println(status);
    
    // Visual feedback
    if (adcValue >= USER_DETECTION_THRESHOLD) {
      digitalWrite(LED_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
    }
    
    delay(500);
  }
  
  digitalWrite(LED_PIN, LOW);
  testRunning = false;
  
  Serial.println("\n[DONE] Sharp IR Sensor test complete!");
  printMenu();
}

String estimateDistance(int adcValue) {
  // ESP8266 10-bit ADC (0-1023) values
  if (adcValue < USER_DETECTION_MIN_ADC) {
    return "> 80cm (out of range)";
  } else if (adcValue > 800) {
    return "10-15cm (very close)";
  } else if (adcValue > 600) {
    return "15-20cm";
  } else if (adcValue > 400) {
    return "20-35cm";
  } else if (adcValue > 250) {
    return "35-50cm";
  } else {
    return "50-80cm";
  }
}

void testPlasticUltrasonic() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   Testing Plastic Ultrasonic Sensor   ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("\nReading for 10 seconds...");
  Serial.println("Press 'S' to stop\n");
  
  testRunning = true;
  unsigned long startTime = millis();
  
  Serial.println("Distance | Capacity | Fill Level");
  Serial.println("---------|----------|------------");
  
  while (testRunning && (millis() - startTime < 10000)) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == 'S' || c == 's') break;
    }
    
    float distance = getUltrasonicDistance(PLASTIC_TRIG_PIN, PLASTIC_ECHO_PIN);
    
    if (distance > 0) {
      float capacity = ((BIN_HEIGHT - distance) / BIN_HEIGHT) * 100.0;
      capacity = constrain(capacity, 0, 100);
      
      Serial.print(" ");
      Serial.print(distance, 1);
      Serial.print("cm  |  ");
      Serial.print(capacity, 1);
      Serial.print("%   | ");
      printBar(capacity);
      Serial.println();
    } else {
      Serial.println(" ERROR   | -------- | No reading");
    }
    
    delay(500);
  }
  
  testRunning = false;
  Serial.println("\n[DONE] Plastic ultrasonic test complete!");
  printMenu();
}

void testTinUltrasonic() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   Testing Tin Ultrasonic Sensor       ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("\nReading for 10 seconds...");
  Serial.println("Press 'S' to stop\n");
  
  testRunning = true;
  unsigned long startTime = millis();
  
  Serial.println("Distance | Capacity | Fill Level");
  Serial.println("---------|----------|------------");
  
  while (testRunning && (millis() - startTime < 10000)) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == 'S' || c == 's') break;
    }
    
    float distance = getUltrasonicDistance(TIN_TRIG_PIN, TIN_ECHO_PIN);
    
    if (distance > 0) {
      float capacity = ((BIN_HEIGHT - distance) / BIN_HEIGHT) * 100.0;
      capacity = constrain(capacity, 0, 100);
      
      Serial.print(" ");
      Serial.print(distance, 1);
      Serial.print("cm  |  ");
      Serial.print(capacity, 1);
      Serial.print("%   | ");
      printBar(capacity);
      Serial.println();
    } else {
      Serial.println(" ERROR   | -------- | No reading");
    }
    
    delay(500);
  }
  
  testRunning = false;
  Serial.println("\n[DONE] Tin ultrasonic test complete!");
  printMenu();
}

void testBinLidServo() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   Testing Bin Lid Servo (MG90S)        ║");
  Serial.println("╚════════════════════════════════════════╝");
  
  Serial.println("\n[SERVO] Opening bin lid (90°)...");
  binLidServo.write(LID_OPEN);
  digitalWrite(LED_PIN, HIGH);
  delay(2000);
  
  Serial.println("[SERVO] Closing bin lid (0°)...");
  binLidServo.write(LID_CLOSED);
  digitalWrite(LED_PIN, LOW);
  delay(2000);
  
  Serial.println("[DONE] Bin lid servo test complete!");
  printMenu();
}

void testDropperServo() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   Testing Dropper Servo (MG90S)        ║");
  Serial.println("╚════════════════════════════════════════╝");
  
  Serial.println("\n[SERVO] Holding position (0°)...");
  dropperServo.write(DROPPER_HOLD);
  delay(2000);
  
  Serial.println("[SERVO] Releasing trash (90°)...");
  dropperServo.write(DROPPER_RELEASE);
  digitalWrite(LED_PIN, HIGH);
  delay(2000);
  
  Serial.println("[SERVO] Back to holding position (0°)...");
  dropperServo.write(DROPPER_HOLD);
  digitalWrite(LED_PIN, LOW);
  delay(2000);
  
  Serial.println("[DONE] Dropper servo test complete!");
  printMenu();
}

void testRotatorServo() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   Testing Rotator Servo (MG995)        ║");
  Serial.println("╚════════════════════════════════════════╝");
  
  Serial.println("\n[SERVO] Rotating to PLASTIC position (0°)...");
  rotatorServo.write(ROTATE_PLASTIC);
  blinkLED(2, 100);
  delay(2000);
  
  Serial.println("[SERVO] Rotating to TIN position (90°)...");
  rotatorServo.write(ROTATE_TIN);
  blinkLED(2, 100);
  delay(2000);
  
  Serial.println("[SERVO] Rotating to REJECTED position (180°)...");
  rotatorServo.write(ROTATE_REJECTED);
  blinkLED(2, 100);
  delay(2000);
  
  Serial.println("[SERVO] Back to PLASTIC position (0°)...");
  rotatorServo.write(ROTATE_PLASTIC);
  delay(2000);
  
  Serial.println("[DONE] Rotator servo test complete!");
  printMenu();
}

void testAllServos() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   Testing ALL Servo Motors             ║");
  Serial.println("╚════════════════════════════════════════╝");
  
  Serial.println("\n[TEST] Simulating full trash sorting cycle...\n");
  
  // Step 1: Open bin lid for user
  Serial.println("[1/7] Opening bin lid (user detected)...");
  binLidServo.write(LID_OPEN);
  digitalWrite(LED_PIN, HIGH);
  delay(2000);
  
  // Step 2: User drops trash, lid closes
  Serial.println("[2/7] Closing bin lid (trash received)...");
  binLidServo.write(LID_CLOSED);
  delay(1500);
  
  // Step 3: Rotate to PLASTIC compartment
  Serial.println("[3/7] Rotating platform to PLASTIC (0°)...");
  rotatorServo.write(ROTATE_PLASTIC);
  blinkLED(2, 100);
  delay(2000);
  
  // Step 4: Drop trash into plastic compartment
  Serial.println("[4/7] Dropping trash into PLASTIC compartment...");
  dropperServo.write(DROPPER_RELEASE);
  delay(1500);
  dropperServo.write(DROPPER_HOLD);
  delay(1000);
  
  // Step 5: Rotate to TIN compartment
  Serial.println("[5/7] Rotating platform to TIN (90°)...");
  rotatorServo.write(ROTATE_TIN);
  blinkLED(2, 100);
  delay(2000);
  
  // Step 6: Rotate to REJECTED compartment
  Serial.println("[6/7] Rotating platform to REJECTED (180°)...");
  rotatorServo.write(ROTATE_REJECTED);
  blinkLED(2, 100);
  delay(2000);
  
  // Step 7: Return to default position
  Serial.println("[7/7] Returning to default position...");
  rotatorServo.write(ROTATE_PLASTIC);
  binLidServo.write(LID_CLOSED);
  dropperServo.write(DROPPER_HOLD);
  digitalWrite(LED_PIN, LOW);
  delay(2000);
  
  Serial.println("\n[DONE] All servos test complete!");
  printMenu();
}

void testLED() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   Testing LED Indicator                ║");
  Serial.println("╚════════════════════════════════════════╝");
  
  Serial.println("\n[LED] Blinking 5 times...");
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, HIGH);
    Serial.print(".");
    delay(300);
    digitalWrite(LED_PIN, LOW);
    delay(300);
  }
  Serial.println();
  
  Serial.println("[LED] Solid ON for 2 seconds...");
  digitalWrite(LED_PIN, HIGH);
  delay(2000);
  
  Serial.println("[LED] OFF");
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("[DONE] LED test complete!");
  printMenu();
}

#if USE_GPS
void testGPS() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   Testing GPS Module                   ║");
  Serial.println("║   (Using SoftwareSerial)               ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("\nWaiting for GPS signal (30 seconds)...");
  Serial.println("Press 'S' to stop\n");
  
  testRunning = true;
  unsigned long startTime = millis();
  
  while (testRunning && (millis() - startTime < 30000)) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == 'S' || c == 's') break;
    }
    
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }
    
    if (gps.location.isValid()) {
      Serial.println("\n[GPS] ✓ Fix acquired!");
      Serial.print("  Latitude:  ");
      Serial.println(gps.location.lat(), 6);
      Serial.print("  Longitude: ");
      Serial.println(gps.location.lng(), 6);
      Serial.print("  Satellites: ");
      Serial.println(gps.satellites.value());
      Serial.print("  Altitude:  ");
      Serial.print(gps.altitude.meters());
      Serial.println(" meters");
      
      digitalWrite(LED_PIN, HIGH);
      delay(1000);
      digitalWrite(LED_PIN, LOW);
      break;
    } else {
      Serial.print(".");
      delay(1000);
    }
  }
  
  if (!gps.location.isValid()) {
    Serial.println("\n[GPS] ✗ No fix acquired");
    Serial.println("  - Check antenna placement");
    Serial.println("  - Ensure clear sky view");
    Serial.println("  - Verify wiring (D2=RX, D1=TX)");
  }
  
  testRunning = false;
  Serial.println("\n[DONE] GPS test complete!");
  printMenu();
}
#endif

void runFullSystemTest() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   FULL SYSTEM TEST                     ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("\nThis will test all components sequentially...\n");
  
  delay(1000);
  
  // Test 1: LED
  Serial.println("[1/7] Testing LED...");
  blinkLED(3, 200);
  delay(500);
  
  // Test 2: Sharp IR Sensor
  Serial.println("[2/7] Testing Sharp IR Sensor (5 sec)...");
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {
    int adcValue = analogRead(USER_DETECTION_PIN);
    if (adcValue >= USER_DETECTION_THRESHOLD) {
      Serial.println("  ✓ User detected!");
      digitalWrite(LED_PIN, HIGH);
      delay(500);
      digitalWrite(LED_PIN, LOW);
      break;
    }
    delay(100);
  }
  
  // Test 3: Plastic Ultrasonic
  Serial.println("[3/7] Testing Plastic Ultrasonic...");
  float plasticDist = getUltrasonicDistance(PLASTIC_TRIG_PIN, PLASTIC_ECHO_PIN);
  if (plasticDist > 0) {
    Serial.print("  ✓ Distance: ");
    Serial.print(plasticDist);
    Serial.println(" cm");
  } else {
    Serial.println("  ✗ No reading");
  }
  delay(1000);
  
  // Test 4: Tin Ultrasonic
  Serial.println("[4/7] Testing Tin Ultrasonic...");
  float tinDist = getUltrasonicDistance(TIN_TRIG_PIN, TIN_ECHO_PIN);
  if (tinDist > 0) {
    Serial.print("  ✓ Distance: ");
    Serial.print(tinDist);
    Serial.println(" cm");
  } else {
    Serial.println("  ✗ No reading");
  }
  delay(1000);
  
  // Test 5: Bin Lid Servo (MG90S)
  Serial.println("[5/7] Testing Bin Lid Servo...");
  binLidServo.write(LID_OPEN);
  delay(1000);
  binLidServo.write(LID_CLOSED);
  Serial.println("  ✓ Servo tested");
  delay(500);
  
  // Test 6: Dropper Servo (MG90S)
  Serial.println("[6/7] Testing Dropper Servo...");
  dropperServo.write(DROPPER_RELEASE);
  delay(1000);
  dropperServo.write(DROPPER_HOLD);
  Serial.println("  ✓ Servo tested");
  delay(500);
  
  // Test 7: Rotator Servo (MG995)
  Serial.println("[7/7] Testing Rotator Servo...");
  rotatorServo.write(ROTATE_PLASTIC);
  delay(500);
  rotatorServo.write(ROTATE_TIN);
  delay(500);
  rotatorServo.write(ROTATE_REJECTED);
  delay(500);
  rotatorServo.write(ROTATE_PLASTIC);
  Serial.println("  ✓ Servo tested");
  delay(500);
  
  // Summary
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   FULL SYSTEM TEST COMPLETE!          ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("\nAll components have been tested.");
  Serial.println("Review the output above for any errors.\n");
  
  blinkLED(5, 100);
  printMenu();
}

// ============================================
// HELPER FUNCTIONS
// ============================================

float getUltrasonicDistance(int trigPin, int echoPin) {
  // Send trigger pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Read echo pulse
  long duration = pulseIn(echoPin, HIGH, 30000);  // 30ms timeout
  
  // Calculate distance in cm
  float distance = duration * 0.034 / 2;
  
  // Filter out invalid readings
  if (distance <= 0 || distance > 400) {
    return -1;  // Invalid reading
  }
  
  return distance;
}

void blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    delay(delayMs);
  }
}

void printBar(float percentage) {
  int bars = map(percentage, 0, 100, 0, 20);
  Serial.print("[");
  for (int i = 0; i < bars; i++) {
    Serial.print("█");
  }
  for (int i = bars; i < 20; i++) {
    Serial.print(" ");
  }
  Serial.print("]");
}
