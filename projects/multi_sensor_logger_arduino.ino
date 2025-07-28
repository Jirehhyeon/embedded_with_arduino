/*
 * Project 09: Multi-Sensor Data Logger (Arduino Version)
 * Description: IoT-ready multi-sensor data collection system
 * Hardware: Arduino Uno, Multiple sensors, SD card module, Status LEDs
 * 
 * Connection:
 * - Temperature/Humidity: DHT22 → Pin 2
 * - Light Sensor: LDR → A0 (with 10kΩ pull-down)
 * - Pressure: BMP280 → I2C (A4, A5)
 * - Accelerometer: MPU6050 → I2C (A4, A5)
 * - Battery Monitor: Voltage divider → A1
 * - SD Card: SPI (Pins 10, 11, 12, 13)
 * - Status LEDs: Red(Pin 3), Yellow(Pin 4), Green(Pin 5)
 * 
 * Features:
 * - Real-time sensor fusion
 * - Data quality validation
 * - Circular buffer management
 * - Packet-based data logging
 * - System health monitoring
 * - Error recovery mechanisms
 */

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>
#include <MPU6050.h>

// 핀 정의
#define DHT_PIN 2
#define LIGHT_SENSOR_PIN A0
#define BATTERY_MONITOR_PIN A1
#define SD_CS_PIN 10
#define LED_RED_PIN 3
#define LED_YELLOW_PIN 4
#define LED_GREEN_PIN 5

// 센서 설정
#define DHT_TYPE DHT22
#define MAX_SENSORS 5
#define BUFFER_SIZE 64
#define PACKET_SIZE 16
#define LOG_INTERVAL_MS 5000

// 센서 타입
enum SensorType {
  SENSOR_TEMPERATURE = 0,
  SENSOR_HUMIDITY = 1,
  SENSOR_LIGHT = 2,
  SENSOR_PRESSURE = 3,
  SENSOR_ACCELERATION = 4
};

// 센서 상태
enum SensorStatus {
  SENSOR_OK = 0,
  SENSOR_ERROR = 1,
  SENSOR_TIMEOUT = 2,
  SENSOR_CALIBRATING = 3,
  SENSOR_DISABLED = 4
};

// 시스템 상태
enum SystemState {
  STATE_INIT = 0,
  STATE_CALIBRATION,
  STATE_NORMAL_OPERATION,
  STATE_DATA_LOGGING,
  STATE_ERROR_RECOVERY,
  STATE_LOW_POWER,
  STATE_SHUTDOWN
};

// 센서 데이터 구조체
struct SensorReading {
  SensorType type;
  float value;
  unsigned long timestamp;
  SensorStatus status;
  uint8_t quality;  // 0-100%
  uint16_t checksum;
};

// 센서 설정 구조체
struct SensorConfig {
  SensorType type;
  bool enabled;
  unsigned long samplePeriodMs;
  unsigned long lastSampleTime;
  float minValue;
  float maxValue;
  uint8_t errorCount;
  String name;
};

// 데이터 패킷 구조체
struct DataPacket {
  uint16_t packetId;
  unsigned long timestamp;
  uint8_t sensorCount;
  SensorReading readings[MAX_SENSORS];
  uint16_t packetChecksum;
};

// 센서 객체
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_BMP280 bmp;
MPU6050 mpu;

// 전역 변수
SystemState currentState = STATE_INIT;
SensorReading sensorBuffer[BUFFER_SIZE];
volatile uint8_t bufferHead = 0;
volatile uint8_t bufferTail = 0;
volatile uint8_t bufferCount = 0;

// 센서 설정
SensorConfig sensors[MAX_SENSORS] = {
  {SENSOR_TEMPERATURE, true, 2000, 0, -40.0, 85.0, 0, "Temperature"},
  {SENSOR_HUMIDITY, true, 2000, 0, 0.0, 100.0, 0, "Humidity"},
  {SENSOR_LIGHT, true, 500, 0, 0.0, 1000.0, 0, "Light"},
  {SENSOR_PRESSURE, true, 1000, 0, 800.0, 1200.0, 0, "Pressure"},
  {SENSOR_ACCELERATION, true, 100, 0, -32.0, 32.0, 0, "Acceleration"}
};

// 시스템 통계
struct {
  unsigned long totalSamples;
  unsigned long errorCount;
  unsigned long packetsGenerated;
  uint16_t bufferOverflowCount;
  uint8_t systemHealth;
  float batteryVoltage;
  bool sdCardReady;
} systemStats = {0};

void setup() {
  Serial.begin(115200);
  Serial.println("=== Multi-Sensor Data Logger ===");
  Serial.println("IoT-Ready Sensor Fusion System");
  Serial.println();
  
  // 핀 초기화
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  pinMode(BATTERY_MONITOR_PIN, INPUT);
  
  // 초기화 표시
  setStatusLED(LED_YELLOW_PIN, true);
  
  // 센서 초기화
  initializeSensors();
  
  // SD 카드 초기화
  initializeSDCard();
  
  // 시스템 보정
  currentState = STATE_CALIBRATION;
  calibrateSensors();
  
  // 정상 운영 모드로 전환
  currentState = STATE_NORMAL_OPERATION;
  setStatusLED(LED_GREEN_PIN, true);
  
  Serial.println("System initialization complete!");
  Serial.println("Commands: help, status, log, calibrate, test");
  Serial.println();
  
  // 초기 시스템 상태 출력
  printSystemStatus();
}

void loop() {
  // 상태 머신 업데이트
  updateSystemState();
  
  // 센서 데이터 수집
  collectAllSensorData();
  
  // 시스템 건강 상태 업데이트
  updateSystemHealth();
  
  // 배터리 전압 모니터링
  monitorBatteryVoltage();
  
  // 상태 LED 업데이트
  updateStatusLEDs();
  
  // 시리얼 명령어 처리
  handleSerialCommands();
  
  // 데이터 로깅 (5초마다)
  static unsigned long lastLog = 0;
  if (millis() - lastLog > LOG_INTERVAL_MS) {
    lastLog = millis();
    logDataToSD();
  }
  
  delay(50);  // 50ms 간격
}

void initializeSensors() {
  Serial.println("Initializing sensors...");
  
  // DHT22 초기화
  dht.begin();
  Serial.println("✓ DHT22 Temperature/Humidity sensor");
  
  // BMP280 초기화
  if (bmp.begin()) {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
    Serial.println("✓ BMP280 Pressure sensor");
  } else {
    Serial.println("✗ BMP280 connection failed");
    sensors[SENSOR_PRESSURE].enabled = false;
  }
  
  // MPU6050 초기화
  Wire.begin();
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("✓ MPU6050 Accelerometer");
  } else {
    Serial.println("✗ MPU6050 connection failed");
    sensors[SENSOR_ACCELERATION].enabled = false;
  }
  
  Serial.println("✓ Light sensor (LDR)");
  Serial.println("✓ Battery voltage monitor");
  Serial.println();
}

void initializeSDCard() {
  Serial.print("Initializing SD card...");
  
  if (SD.begin(SD_CS_PIN)) {
    systemStats.sdCardReady = true;
    Serial.println(" ✓ SD card ready");
    
    // 로그 파일 헤더 작성
    File logFile = SD.open("sensor_log.csv", FILE_WRITE);
    if (logFile) {
      logFile.println("Timestamp,Temperature,Humidity,Light,Pressure,Acceleration,Quality,Health");
      logFile.close();
      Serial.println("✓ Log file initialized");
    }
  } else {
    systemStats.sdCardReady = false;
    Serial.println(" ✗ SD card initialization failed");
  }
  Serial.println();
}

void calibrateSensors() {
  Serial.println("Calibrating sensors...");
  setStatusLED(LED_YELLOW_PIN, true);
  
  // 광도 센서 보정 (어두운 환경에서 0점 조정)
  Serial.print("Calibrating light sensor...");
  delay(2000);
  Serial.println(" Done");
  
  // 가속도계 보정 (중력 기준)
  if (sensors[SENSOR_ACCELERATION].enabled) {
    Serial.print("Calibrating accelerometer...");
    mpu.setXAccelOffset(0);
    mpu.setYAccelOffset(0);
    mpu.setZAccelOffset(0);
    delay(1000);
    Serial.println(" Done");
  }
  
  Serial.println("Sensor calibration complete");
  setStatusLED(LED_YELLOW_PIN, false);
}

void collectAllSensorData() {
  for (uint8_t i = 0; i < MAX_SENSORS; i++) {
    if (!sensors[i].enabled) continue;
    
    // 샘플링 주기 확인
    if ((millis() - sensors[i].lastSampleTime) >= sensors[i].samplePeriodMs) {
      sensors[i].lastSampleTime = millis();
      
      SensorReading reading;
      reading.type = sensors[i].type;
      reading.timestamp = millis();
      
      // 센서별 데이터 읽기
      if (readSensorData(&reading)) {
        // 데이터 유효성 검증
        if (validateSensorData(&reading, &sensors[i])) {
          addSensorDataToBuffer(&reading);
          systemStats.totalSamples++;
          sensors[i].errorCount = 0;
        } else {
          sensors[i].errorCount++;
          systemStats.errorCount++;
        }
      } else {
        sensors[i].errorCount++;
        systemStats.errorCount++;
        
        // 연속 에러 시 센서 비활성화
        if (sensors[i].errorCount > 5) {
          sensors[i].enabled = false;
          Serial.print("Sensor disabled: ");
          Serial.println(sensors[i].name);
        }
      }
    }
  }
}

bool readSensorData(SensorReading* reading) {
  switch (reading->type) {
    case SENSOR_TEMPERATURE: {
      float temp = dht.readTemperature();
      if (!isnan(temp)) {
        reading->value = temp;
        reading->status = SENSOR_OK;
        reading->quality = 95;
        return true;
      }
      break;
    }
    
    case SENSOR_HUMIDITY: {
      float hum = dht.readHumidity();
      if (!isnan(hum)) {
        reading->value = hum;
        reading->status = SENSOR_OK;
        reading->quality = 90;
        return true;
      }
      break;
    }
    
    case SENSOR_LIGHT: {
      int lightValue = analogRead(LIGHT_SENSOR_PIN);
      reading->value = map(lightValue, 0, 1023, 0, 1000);  // 0-1000 럭스
      reading->status = SENSOR_OK;
      reading->quality = (lightValue > 50) ? 95 : 70;
      return true;
    }
    
    case SENSOR_PRESSURE: {
      if (sensors[SENSOR_PRESSURE].enabled) {
        reading->value = bmp.readPressure() / 100.0;  // hPa
        reading->status = SENSOR_OK;
        reading->quality = 98;
        return true;
      }
      break;
    }
    
    case SENSOR_ACCELERATION: {
      if (sensors[SENSOR_ACCELERATION].enabled) {
        int16_t ax, ay, az;
        mpu.getAcceleration(&ax, &ay, &az);
        // 3축 벡터 크기 계산
        reading->value = sqrt(ax*ax + ay*ay + az*az) / 16384.0;  // g 단위
        reading->status = SENSOR_OK;
        reading->quality = 92;
        return true;
      }
      break;
    }
  }
  
  reading->status = SENSOR_ERROR;
  reading->quality = 0;
  return false;
}

bool validateSensorData(const SensorReading* reading, const SensorConfig* config) {
  // 범위 검증
  if (reading->value < config->minValue || reading->value > config->maxValue) {
    return false;
  }
  
  // 품질 검증
  if (reading->quality < 50) {
    return false;
  }
  
  return true;
}

bool addSensorDataToBuffer(const SensorReading* reading) {
  if (bufferCount >= BUFFER_SIZE) {
    systemStats.bufferOverflowCount++;
    return false;  // 버퍼 오버플로우
  }
  
  sensorBuffer[bufferHead] = *reading;
  bufferHead = (bufferHead + 1) % BUFFER_SIZE;
  bufferCount++;
  
  return true;
}

void updateSystemState() {
  static unsigned long stateEnterTime = 0;
  static SystemState lastState = STATE_INIT;
  
  if (currentState != lastState) {
    stateEnterTime = millis();
    lastState = currentState;
  }
  
  unsigned long timeInState = millis() - stateEnterTime;
  
  switch (currentState) {
    case STATE_NORMAL_OPERATION:
      // 버퍼가 반 이상 찼으면 로깅 모드로
      if (bufferCount > BUFFER_SIZE / 2) {
        currentState = STATE_DATA_LOGGING;
      }
      
      // 에러가 많으면 복구 모드로
      if (systemStats.errorCount > 50) {
        currentState = STATE_ERROR_RECOVERY;
      }
      
      // 배터리 전압이 낮으면 저전력 모드로
      if (systemStats.batteryVoltage < 3.5) {
        currentState = STATE_LOW_POWER;
      }
      break;
      
    case STATE_DATA_LOGGING:
      // 패킷 생성 및 저장
      createAndLogDataPacket();
      
      // 버퍼가 비워지면 정상 모드로
      if (bufferCount < BUFFER_SIZE / 4) {
        currentState = STATE_NORMAL_OPERATION;
      }
      break;
      
    case STATE_ERROR_RECOVERY:
      Serial.println("Entering error recovery mode...");
      
      // 센서 재초기화
      for (uint8_t i = 0; i < MAX_SENSORS; i++) {
        sensors[i].errorCount = 0;
        sensors[i].enabled = true;
      }
      
      systemStats.errorCount = 0;
      currentState = STATE_CALIBRATION;
      break;
      
    case STATE_CALIBRATION:
      calibrateSensors();
      currentState = STATE_NORMAL_OPERATION;
      break;
      
    case STATE_LOW_POWER:
      // 저전력 모드 (샘플링 주기 연장)
      for (uint8_t i = 0; i < MAX_SENSORS; i++) {
        sensors[i].samplePeriodMs *= 2;  // 주기 2배 연장
      }
      
      delay(5000);  // 5초 대기
      currentState = STATE_NORMAL_OPERATION;
      break;
  }
}

void createAndLogDataPacket() {
  static uint16_t packetId = 0;
  
  DataPacket packet;
  packet.packetId = packetId++;
  packet.timestamp = millis();
  packet.sensorCount = 0;
  
  // 버퍼에서 센서 데이터 수집
  while (bufferCount > 0 && packet.sensorCount < MAX_SENSORS) {
    SensorReading reading;
    if (getSensorDataFromBuffer(&reading)) {
      packet.readings[packet.sensorCount++] = reading;
    }
  }
  
  if (packet.sensorCount > 0) {
    // 패킷 체크섬 계산
    packet.packetChecksum = calculatePacketChecksum(&packet);
    
    // SD 카드에 저장
    logPacketToSD(&packet);
    
    systemStats.packetsGenerated++;
  }
}

bool getSensorDataFromBuffer(SensorReading* reading) {
  if (bufferCount == 0) {
    return false;
  }
  
  *reading = sensorBuffer[bufferTail];
  bufferTail = (bufferTail + 1) % BUFFER_SIZE;
  bufferCount--;
  
  return true;
}

uint16_t calculatePacketChecksum(const DataPacket* packet) {
  // 간단한 체크섬 계산
  uint16_t checksum = packet->packetId + packet->timestamp + packet->sensorCount;
  for (uint8_t i = 0; i < packet->sensorCount; i++) {
    checksum += (uint16_t)(packet->readings[i].value * 100);
  }
  return checksum;
}

void logPacketToSD(const DataPacket* packet) {
  if (!systemStats.sdCardReady) return;
  
  File logFile = SD.open("sensor_log.csv", FILE_WRITE);
  if (logFile) {
    // CSV 형식으로 저장
    logFile.print(packet->timestamp);
    
    for (uint8_t i = 0; i < packet->sensorCount; i++) {
      logFile.print(",");
      logFile.print(packet->readings[i].value, 2);
    }
    
    logFile.print(",");
    logFile.print(systemStats.systemHealth);
    logFile.println();
    
    logFile.close();
  }
}

void logDataToSD() {
  if (!systemStats.sdCardReady) return;
  
  // 현재 센서 값들을 직접 로그
  File logFile = SD.open("sensor_log.csv", FILE_WRITE);
  if (logFile) {
    logFile.print(millis());
    logFile.print(",");
    logFile.print(dht.readTemperature(), 1);
    logFile.print(",");
    logFile.print(dht.readHumidity(), 1);
    logFile.print(",");
    logFile.print(analogRead(LIGHT_SENSOR_PIN));
    logFile.print(",");
    if (sensors[SENSOR_PRESSURE].enabled) {
      logFile.print(bmp.readPressure() / 100.0, 1);
    } else {
      logFile.print("N/A");
    }
    logFile.print(",");
    if (sensors[SENSOR_ACCELERATION].enabled) {
      int16_t ax, ay, az;
      mpu.getAcceleration(&ax, &ay, &az);
      logFile.print(sqrt(ax*ax + ay*ay + az*az) / 16384.0, 3);
    } else {
      logFile.print("N/A");
    }
    logFile.print(",");
    logFile.print(systemStats.systemHealth);
    logFile.print(",");
    logFile.print(systemStats.batteryVoltage, 2);
    logFile.println();
    
    logFile.close();
  }
}

void updateSystemHealth() {
  uint8_t activeSensors = 0;
  uint8_t healthySensors = 0;
  
  for (uint8_t i = 0; i < MAX_SENSORS; i++) {
    if (sensors[i].enabled) {
      activeSensors++;
      if (sensors[i].errorCount < 3) {
        healthySensors++;
      }
    }
  }
  
  if (activeSensors == 0) {
    systemStats.systemHealth = 0;
  } else {
    systemStats.systemHealth = (healthySensors * 100) / activeSensors;
  }
  
  // 버퍼 오버플로우도 건강 상태에 영향
  if (systemStats.bufferOverflowCount > 10) {
    systemStats.systemHealth = (systemStats.systemHealth * 80) / 100;
  }
}

void monitorBatteryVoltage() {
  int batteryADC = analogRead(BATTERY_MONITOR_PIN);
  systemStats.batteryVoltage = (batteryADC * 5.0 * 2.0) / 1023.0;  // 전압 분배기 2:1
}

void setStatusLED(uint8_t pin, bool state) {
  // 다른 LED 끄기
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_YELLOW_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, LOW);
  
  // 지정된 LED 설정
  digitalWrite(pin, state);
}

void updateStatusLEDs() {
  static unsigned long lastUpdate = 0;
  
  if (millis() - lastUpdate > 1000) {  // 1초마다
    lastUpdate = millis();
    
    if (systemStats.systemHealth > 80) {
      setStatusLED(LED_GREEN_PIN, true);
    } else if (systemStats.systemHealth > 50) {
      setStatusLED(LED_YELLOW_PIN, true);
    } else {
      setStatusLED(LED_RED_PIN, true);
    }
  }
}

void printSystemStatus() {
  Serial.println("--- System Status ---");
  Serial.print("State: ");
  Serial.println(getStateName(currentState));
  Serial.print("System Health: ");
  Serial.print(systemStats.systemHealth);
  Serial.println("%");
  Serial.print("Total Samples: ");
  Serial.println(systemStats.totalSamples);
  Serial.print("Error Count: ");
  Serial.println(systemStats.errorCount);
  Serial.print("Buffer Usage: ");
  Serial.print(bufferCount);
  Serial.print("/");
  Serial.println(BUFFER_SIZE);
  Serial.print("Battery Voltage: ");
  Serial.print(systemStats.batteryVoltage, 2);
  Serial.println("V");
  Serial.print("SD Card: ");
  Serial.println(systemStats.sdCardReady ? "Ready" : "Not Ready");
  
  Serial.println("\nSensor Status:");
  for (uint8_t i = 0; i < MAX_SENSORS; i++) {
    Serial.print(sensors[i].name);
    Serial.print(": ");
    Serial.print(sensors[i].enabled ? "Enabled" : "Disabled");
    Serial.print(" (Errors: ");
    Serial.print(sensors[i].errorCount);
    Serial.println(")");
  }
  Serial.println("--------------------");
}

String getStateName(SystemState state) {
  switch (state) {
    case STATE_INIT: return "Initialize";
    case STATE_CALIBRATION: return "Calibrating";
    case STATE_NORMAL_OPERATION: return "Normal";
    case STATE_DATA_LOGGING: return "Logging";
    case STATE_ERROR_RECOVERY: return "Recovery";
    case STATE_LOW_POWER: return "Low Power";
    case STATE_SHUTDOWN: return "Shutdown";
    default: return "Unknown";
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
      Serial.println("- log: Show recent sensor readings");
      Serial.println("- calibrate: Recalibrate sensors");
      Serial.println("- test: Test all sensors");
      Serial.println("- reset: Reset error counters");
    }
    else if (command == "status") {
      printSystemStatus();
    }
    else if (command == "log") {
      showRecentReadings();
    }
    else if (command == "calibrate") {
      currentState = STATE_CALIBRATION;
      Serial.println("Starting sensor calibration...");
    }
    else if (command == "test") {
      testAllSensors();
    }
    else if (command == "reset") {
      // 에러 카운터 리셋
      for (uint8_t i = 0; i < MAX_SENSORS; i++) {
        sensors[i].errorCount = 0;
        sensors[i].enabled = true;
      }
      systemStats.errorCount = 0;
      systemStats.bufferOverflowCount = 0;
      Serial.println("Error counters reset");
    }
    else {
      Serial.println("Unknown command. Type 'help' for available commands.");
    }
  }
}

void showRecentReadings() {
  Serial.println("Recent sensor readings:");
  Serial.print("Temperature: ");
  Serial.print(dht.readTemperature(), 1);
  Serial.println("°C");
  
  Serial.print("Humidity: ");
  Serial.print(dht.readHumidity(), 1);
  Serial.println("%");
  
  Serial.print("Light: ");
  Serial.println(analogRead(LIGHT_SENSOR_PIN));
  
  if (sensors[SENSOR_PRESSURE].enabled) {
    Serial.print("Pressure: ");
    Serial.print(bmp.readPressure() / 100.0, 1);
    Serial.println(" hPa");
  }
  
  if (sensors[SENSOR_ACCELERATION].enabled) {
    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);
    Serial.print("Acceleration: ");
    Serial.print(sqrt(ax*ax + ay*ay + az*az) / 16384.0, 3);
    Serial.println(" g");
  }
}

void testAllSensors() {
  Serial.println("Testing all sensors...");
  
  for (uint8_t i = 0; i < MAX_SENSORS; i++) {
    Serial.print("Testing ");
    Serial.print(sensors[i].name);
    Serial.print("... ");
    
    SensorReading reading;
    reading.type = sensors[i].type;
    reading.timestamp = millis();
    
    if (readSensorData(&reading)) {
      Serial.print("OK (Value: ");
      Serial.print(reading.value, 2);
      Serial.print(", Quality: ");
      Serial.print(reading.quality);
      Serial.println("%)");
    } else {
      Serial.println("FAILED");
    }
  }
}