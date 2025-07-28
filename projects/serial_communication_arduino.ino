/*
 * Project 05: Serial Communication - Temperature Sensor Monitor (Arduino Version)
 * Description: Read analog sensor and send data via Serial
 * Hardware: Arduino Uno, LM35 temperature sensor (or potentiometer), LED
 * 
 * Connection:
 * - LM35: VCC → 5V, GND → GND, OUT → A0
 * - LED: Pin 6 → 220Ω resistor → LED → GND
 * - Alternative: Use potentiometer on A0 to simulate temperature
 */

const int SENSOR_PIN = A0;      // 온도 센서 (또는 가변저항)
const int LED_PIN = 6;          // 경고 LED
const float TEMP_THRESHOLD = 30.0;  // 경고 온도 (°C)

unsigned long previousMillis = 0;
const long interval = 1000;     // 1초마다 측정
unsigned long timeCounter = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);   // 초기에는 LED 끄기
  
  Serial.begin(9600);
  
  // 시작 메시지
  Serial.println("=== Temperature Monitor System ===");
  Serial.println("Format: [Time] ADC: xxx, Temp: xx.x°C, Status: xxx");
  Serial.print("Warning threshold: ");
  Serial.print(TEMP_THRESHOLD);
  Serial.println("°C");
  Serial.println();
  
  delay(2000);  // 초기화 대기
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    // 센서 값 읽기
    int adcValue = analogRead(SENSOR_PIN);
    
    // 온도로 변환 (LM35: 10mV/°C)
    // 5V 기준, 1023 = 5V이므로
    // voltage = adcValue * (5.0 / 1023.0)
    // temperature = voltage * 100 (LM35 특성)
    float voltage = adcValue * (5.0 / 1023.0);
    float temperature = voltage * 100.0;
    
    // 경고 LED 제어
    if (temperature > TEMP_THRESHOLD) {
      digitalWrite(LED_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
    }
    
    // 시리얼로 데이터 출력
    Serial.print("[");
    Serial.print(timeCounter);
    Serial.print("s] ADC: ");
    Serial.print(adcValue);
    Serial.print(", Temp: ");
    Serial.print(temperature, 1);  // 소수점 1자리
    Serial.print("°C, Status: ");
    
    if (temperature > TEMP_THRESHOLD) {
      Serial.println("WARNING!");
    } else {
      Serial.println("Normal");
    }
    
    timeCounter++;
  }
  
  // 시리얼 명령어 처리 (선택적)
  if (Serial.available() > 0) {
    String command = Serial.readString();
    command.trim();
    
    if (command == "help") {
      Serial.println("Available commands:");
      Serial.println("- help: Show this help");
      Serial.println("- status: Show current status");
      Serial.println("- reset: Reset timer");
    }
    else if (command == "status") {
      int currentADC = analogRead(SENSOR_PIN);
      float currentTemp = currentADC * (5.0 / 1023.0) * 100.0;
      Serial.print("Current: ADC=");
      Serial.print(currentADC);
      Serial.print(", Temp=");
      Serial.print(currentTemp, 1);
      Serial.println("°C");
    }
    else if (command == "reset") {
      timeCounter = 0;
      Serial.println("Timer reset.");
    }
    else {
      Serial.println("Unknown command. Type 'help' for available commands.");
    }
  }
}