/*
 * Project 08: I2C Communication (Arduino Version)
 * Description: Multi-sensor I2C interface with data logging
 * Hardware: Arduino Uno, DS1307 RTC, 24C256 EEPROM, DHT22 I2C module
 * 
 * Connection:
 * - SDA: Arduino A4 ← Pull-up 4.7kΩ to 5V
 * - SCL: Arduino A5 ← Pull-up 4.7kΩ to 5V
 * - DS1307 RTC: VCC→5V, GND→GND, SDA→A4, SCL→A5
 * - 24C256 EEPROM: VCC→5V, GND→GND, SDA→A4, SCL→A5
 * - Status LED: Pin 13 → LED → GND (built-in)
 * 
 * Features:
 * - Real-time clock management
 * - EEPROM data logging
 * - Sensor data collection
 * - I2C device scanning
 * - Error handling and recovery
 */

#include <Wire.h>
#include <RTClib.h>  // DS1307 library

// I2C 디바이스 주소
#define DS1307_ADDR     0x68
#define EEPROM_ADDR     0x50
#define TEMP_SENSOR_ADDR 0x48  // 가상 온도센서

// EEPROM 설정
#define EEPROM_SIZE     32768  // 24C256 = 32KB
#define LOG_START_ADDR  0x0100
#define LOG_END_ADDR    0x1000  // 4KB 로그 영역
#define LOG_ENTRY_SIZE  8       // 타임스탬프(4) + 온도(2) + 습도(2)

// 데이터 구조체
struct SensorData {
  int16_t temperature;  // 0.1도 단위
  uint16_t humidity;    // 0.1% 단위
  bool valid;
  unsigned long timestamp;
};

struct SystemStatus {
  bool rtcConnected;
  bool eepromConnected;
  bool sensorConnected;
  unsigned int errorCount;
  unsigned long lastUpdate;
};

// 전역 변수
RTC_DS1307 rtc;
SystemStatus systemStatus = {false, false, false, 0, 0};
unsigned int currentLogAddress = LOG_START_ADDR;

void setup() {
  Serial.begin(9600);
  Serial.println("=== I2C Multi-Sensor System ===");
  Serial.println("Initializing I2C devices...");
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  // I2C 초기화
  Wire.begin();
  Wire.setClock(100000);  // 100kHz
  
  delay(1000);  // 디바이스 안정화 대기
  
  // 시스템 초기화
  initializeSystem();
  
  // I2C 디바이스 스캔
  scanI2CDevices();
  
  // 시스템 상태 출력
  printSystemStatus();
  
  // 초기 설정 (필요 시)
  setupInitialSettings();
  
  Serial.println("System ready!");
  Serial.println("Commands: help, status, time, log, scan, test");
  Serial.println();
}

void loop() {
  // 메인 시스템 루프
  updateSystemStatus();
  collectSensorData();
  handleSerialCommands();
  
  // 상태 LED 업데이트
  updateStatusLED();
  
  delay(1000);  // 1초 간격
}

void initializeSystem() {
  // RTC 초기화
  if (rtc.begin()) {
    systemStatus.rtcConnected = true;
    Serial.println("✓ DS1307 RTC connected");
    
    // RTC가 실행 중인지 확인
    if (!rtc.isrunning()) {
      Serial.println("⚠ RTC is not running, setting time...");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  } else {
    Serial.println("✗ DS1307 RTC connection failed");
  }
  
  // EEPROM 연결 테스트
  if (testEEPROM()) {
    systemStatus.eepromConnected = true;
    Serial.println("✓ 24C256 EEPROM connected");
  } else {
    Serial.println("✗ 24C256 EEPROM connection failed");
  }
  
  // 가상 센서 (항상 연결된 것으로 처리)
  systemStatus.sensorConnected = true;
  Serial.println("✓ Virtual temperature sensor connected");
}

bool testEEPROM() {
  // EEPROM 읽기/쓰기 테스트
  const uint8_t testData[] = {0xAA, 0x55, 0xFF, 0x00};
  uint8_t readData[4];
  
  // 테스트 데이터 쓰기
  if (!writeEEPROM(0x0000, testData, 4)) {
    return false;
  }
  
  delay(10);  // 쓰기 완료 대기
  
  // 테스트 데이터 읽기
  if (!readEEPROM(0x0000, readData, 4)) {
    return false;
  }
  
  // 데이터 비교
  for (int i = 0; i < 4; i++) {
    if (testData[i] != readData[i]) {
      return false;
    }
  }
  
  return true;
}

void scanI2CDevices() {
  Serial.println("Scanning I2C bus...");
  int deviceCount = 0;
  
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("Device found at 0x");
      if (addr < 16) Serial.print("0");
      Serial.print(addr, HEX);
      
      // 알려진 디바이스 식별
      switch (addr) {
        case DS1307_ADDR:
          Serial.print(" (DS1307 RTC)");
          break;
        case EEPROM_ADDR:
          Serial.print(" (24C256 EEPROM)");
          break;
        case TEMP_SENSOR_ADDR:
          Serial.print(" (Temperature Sensor)");
          break;
        default:
          Serial.print(" (Unknown device)");
          break;
      }
      Serial.println();
      deviceCount++;
    }
  }
  
  Serial.print("Total devices found: ");
  Serial.println(deviceCount);
  Serial.println();
}

void updateSystemStatus() {
  systemStatus.lastUpdate = millis();
  
  // RTC 연결 상태 확인
  DateTime now = rtc.now();
  systemStatus.rtcConnected = (now.year() > 2020);  // 유효한 시간인지 확인
  
  // 주기적으로 EEPROM 테스트
  static unsigned long lastEEPROMTest = 0;
  if (millis() - lastEEPROMTest > 30000) {  // 30초마다
    lastEEPROMTest = millis();
    systemStatus.eepromConnected = testEEPROM();
  }
}

void collectSensorData() {
  SensorData data;
  
  // 가상 센서 데이터 생성
  generateVirtualSensorData(&data);
  
  if (data.valid) {
    // EEPROM에 로그 저장
    logSensorData(&data);
    
    // 시리얼 출력 (10초마다)
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 10000) {
      lastPrint = millis();
      printSensorData(&data);
    }
  }
}

void generateVirtualSensorData(SensorData* data) {
  // 실제 센서 대신 시뮬레이션 데이터
  static int16_t baseTemp = 250;   // 25.0도
  static uint16_t baseHum = 600;   // 60.0%
  
  // 시간에 따른 변화 시뮬레이션
  unsigned long time = millis() / 1000;
  int16_t tempVariation = (sin(time * 0.01) * 50);  // ±5도 변화
  uint16_t humVariation = (cos(time * 0.02) * 100); // ±10% 변화
  
  data->temperature = baseTemp + tempVariation;
  data->humidity = baseHum + humVariation;
  data->timestamp = time;
  data->valid = true;
  
  // 범위 제한
  if (data->temperature < 0) data->temperature = 0;
  if (data->temperature > 500) data->temperature = 500;
  if (data->humidity < 0) data->humidity = 0;
  if (data->humidity > 1000) data->humidity = 1000;
}

void logSensorData(const SensorData* data) {
  if (!systemStatus.eepromConnected) return;
  
  uint8_t logEntry[LOG_ENTRY_SIZE];
  
  // 로그 엔트리 구성: 타임스탬프(4) + 온도(2) + 습도(2)
  logEntry[0] = (data->timestamp >> 24) & 0xFF;
  logEntry[1] = (data->timestamp >> 16) & 0xFF;
  logEntry[2] = (data->timestamp >> 8) & 0xFF;
  logEntry[3] = data->timestamp & 0xFF;
  logEntry[4] = (data->temperature >> 8) & 0xFF;
  logEntry[5] = data->temperature & 0xFF;
  logEntry[6] = (data->humidity >> 8) & 0xFF;
  logEntry[7] = data->humidity & 0xFF;
  
  // EEPROM에 쓰기
  if (writeEEPROM(currentLogAddress, logEntry, LOG_ENTRY_SIZE)) {
    currentLogAddress += LOG_ENTRY_SIZE;
    
    // 로그 영역 순환
    if (currentLogAddress >= LOG_END_ADDR) {
      currentLogAddress = LOG_START_ADDR;
      Serial.println("Log area wrapped around");
    }
  } else {
    systemStatus.errorCount++;
    Serial.println("EEPROM write error");
  }
}

bool writeEEPROM(unsigned int addr, const uint8_t* data, uint8_t length) {
  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write((addr >> 8) & 0xFF);  // MSB
  Wire.write(addr & 0xFF);         // LSB
  
  for (uint8_t i = 0; i < length; i++) {
    Wire.write(data[i]);
  }
  
  uint8_t result = Wire.endTransmission();
  return (result == 0);
}

bool readEEPROM(unsigned int addr, uint8_t* data, uint8_t length) {
  // 주소 설정
  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write((addr >> 8) & 0xFF);
  Wire.write(addr & 0xFF);
  if (Wire.endTransmission() != 0) return false;
  
  // 데이터 읽기
  Wire.requestFrom(EEPROM_ADDR, length);
  
  for (uint8_t i = 0; i < length; i++) {
    if (Wire.available()) {
      data[i] = Wire.read();
    } else {
      return false;
    }
  }
  
  return true;
}

void printSensorData(const SensorData* data) {
  DateTime now = rtc.now();
  
  Serial.print("[");
  Serial.print(now.year());
  Serial.print("/");
  Serial.print(now.month());
  Serial.print("/");
  Serial.print(now.day());
  Serial.print(" ");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.print(now.second());
  Serial.print("] ");
  
  Serial.print("Temp: ");
  Serial.print(data->temperature / 10.0, 1);
  Serial.print("°C, Humidity: ");
  Serial.print(data->humidity / 10.0, 1);
  Serial.println("%");
}

void printSystemStatus() {
  Serial.println("--- System Status ---");
  Serial.print("RTC: ");
  Serial.println(systemStatus.rtcConnected ? "Connected" : "Disconnected");
  Serial.print("EEPROM: ");
  Serial.println(systemStatus.eepromConnected ? "Connected" : "Disconnected");
  Serial.print("Sensor: ");
  Serial.println(systemStatus.sensorConnected ? "Connected" : "Disconnected");
  Serial.print("Error count: ");
  Serial.println(systemStatus.errorCount);
  Serial.print("Log address: 0x");
  Serial.println(currentLogAddress, HEX);
  Serial.println("--------------------");
}

void setupInitialSettings() {
  // 첫 실행 시 초기 설정
  if (systemStatus.eepromConnected) {
    // 설정 영역에 초기 데이터 저장
    const char signature[] = "I2C_SYSTEM_V1.0";
    writeEEPROM(0x0000, (const uint8_t*)signature, strlen(signature));
  }
}

void updateStatusLED() {
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  
  unsigned long interval = 500;  // 기본 500ms
  
  // 오류 상태에 따른 깜빡임 패턴
  if (systemStatus.errorCount > 0) {
    interval = 100;  // 빠른 깜빡임
  } else if (!systemStatus.rtcConnected || !systemStatus.eepromConnected) {
    interval = 200;  // 중간 속도
  }
  
  if (millis() - lastBlink > interval) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
  }
}

void handleSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readString();
    command.trim();
    
    if (command == "help") {
      Serial.println("Available commands:");
      Serial.println("- help: Show this help");
      Serial.println("- status: Show system status");
      Serial.println("- time: Show current time");
      Serial.println("- log: Show recent log entries");
      Serial.println("- scan: Scan I2C devices");
      Serial.println("- test: Test EEPROM");
    }
    else if (command == "status") {
      printSystemStatus();
    }
    else if (command == "time") {
      DateTime now = rtc.now();
      Serial.print("Current time: ");
      Serial.print(now.year());
      Serial.print("/");
      Serial.print(now.month());
      Serial.print("/");
      Serial.print(now.day());
      Serial.print(" ");
      Serial.print(now.hour());
      Serial.print(":");
      Serial.print(now.minute());
      Serial.print(":");
      Serial.println(now.second());
    }
    else if (command == "log") {
      showRecentLogs();
    }
    else if (command == "scan") {
      scanI2CDevices();
    }
    else if (command == "test") {
      Serial.print("EEPROM test: ");
      Serial.println(testEEPROM() ? "PASS" : "FAIL");
    }
    else {
      Serial.println("Unknown command. Type 'help' for available commands.");
    }
  }
}

void showRecentLogs() {
  if (!systemStatus.eepromConnected) {
    Serial.println("EEPROM not connected");
    return;
  }
  
  Serial.println("Recent log entries:");
  
  // 최근 5개 엔트리 표시
  for (int i = 0; i < 5; i++) {
    unsigned int addr = currentLogAddress - (i + 1) * LOG_ENTRY_SIZE;
    if (addr < LOG_START_ADDR) {
      addr = LOG_END_ADDR - LOG_ENTRY_SIZE + (addr - LOG_START_ADDR);
    }
    
    uint8_t logEntry[LOG_ENTRY_SIZE];
    if (readEEPROM(addr, logEntry, LOG_ENTRY_SIZE)) {
      // 로그 엔트리 파싱
      unsigned long timestamp = ((unsigned long)logEntry[0] << 24) |
                               ((unsigned long)logEntry[1] << 16) |
                               ((unsigned long)logEntry[2] << 8) |
                               logEntry[3];
      int16_t temperature = (logEntry[4] << 8) | logEntry[5];
      uint16_t humidity = (logEntry[6] << 8) | logEntry[7];
      
      Serial.print("Time: ");
      Serial.print(timestamp);
      Serial.print(", Temp: ");
      Serial.print(temperature / 10.0, 1);
      Serial.print("°C, Humidity: ");
      Serial.print(humidity / 10.0, 1);
      Serial.println("%");
    }
  }
}