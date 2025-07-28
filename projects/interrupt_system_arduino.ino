/*
 * Project 06: Interrupt-Driven System (Arduino Version)
 * Description: Professional interrupt handling with multiple priorities
 * Hardware: Arduino Uno, 3 buttons, 3 LEDs, resistors
 * 
 * Connection:
 * - Emergency Stop: Pin 2 (INT0) → Button → GND, 10kΩ pullup to 5V
 * - System Reset: Pin 3 (INT1) → Button → GND, 10kΩ pullup to 5V  
 * - Event Button: Pin 8 → Button → GND, 10kΩ pullup to 5V
 * - Red LED: Pin 6 → 220Ω → LED → GND
 * - Yellow LED: Pin 7 → 220Ω → LED → GND
 * - Green LED: Pin 9 → 220Ω → LED → GND
 */

// 핀 정의
const int EMERGENCY_STOP_PIN = 2;   // INT0
const int SYSTEM_RESET_PIN = 3;     // INT1
const int EVENT_BUTTON_PIN = 8;     // Digital pin

const int RED_LED_PIN = 6;          // Emergency status
const int YELLOW_LED_PIN = 7;       // System status  
const int GREEN_LED_PIN = 9;        // Normal operation

// 시스템 상태
enum SystemState {
  SYSTEM_NORMAL = 0,
  SYSTEM_EMERGENCY = 1, 
  SYSTEM_RESET = 2,
  SYSTEM_EVENT = 3
};

// 전역 변수 (인터럽트에서 사용하므로 volatile 필수)
volatile SystemState currentState = SYSTEM_NORMAL;
volatile unsigned long emergencyCount = 0;
volatile unsigned long resetCount = 0;
volatile unsigned long eventCount = 0;
volatile bool systemEnabled = true;

// 디바운싱
volatile unsigned long lastInterruptTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

void setup() {
  // 직렬 통신 초기화
  Serial.begin(9600);
  Serial.println("=== Interrupt-Driven Safety System ===");
  Serial.println("Controls:");
  Serial.println("- Pin 2: Emergency Stop (highest priority)");
  Serial.println("- Pin 3: System Reset (emergency state only)");
  Serial.println("- Pin 8: General Event");
  Serial.println();
  
  // 버튼 핀 설정 (내부 풀업 사용)
  pinMode(EMERGENCY_STOP_PIN, INPUT_PULLUP);
  pinMode(SYSTEM_RESET_PIN, INPUT_PULLUP);
  pinMode(EVENT_BUTTON_PIN, INPUT_PULLUP);
  
  // LED 핀 설정
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  
  // 인터럽트 설정
  attachInterrupt(digitalPinToInterrupt(EMERGENCY_STOP_PIN), emergencyStopISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(SYSTEM_RESET_PIN), systemResetISR, FALLING);
  
  // 시스템 초기화 완료 표시
  systemInitialization();
  
  Serial.println("System initialized - Normal operation");
  updateLEDStatus();
}

void loop() {
  // 메인 루프: LED 상태 업데이트 및 이벤트 버튼 폴링
  updateLEDStatus();
  
  // 이벤트 버튼 체크 (폴링 방식)
  static bool lastEventButtonState = HIGH;
  bool currentEventButtonState = digitalRead(EVENT_BUTTON_PIN);
  
  if (lastEventButtonState == HIGH && currentEventButtonState == LOW) {
    if (millis() - lastInterruptTime > DEBOUNCE_DELAY) {
      handleEventButton();
      lastInterruptTime = millis();
    }
  }
  lastEventButtonState = currentEventButtonState;
  
  // 시스템 상태 리포팅 (5초마다)
  static unsigned long lastReport = 0;
  if (millis() - lastReport > 5000) {
    lastReport = millis();
    reportSystemStatus();
  }
  
  delay(10);  // 10ms 간격
}

// 긴급 정지 인터럽트 (최고 우선순위)
void emergencyStopISR() {
  if (millis() - lastInterruptTime < DEBOUNCE_DELAY) return;
  lastInterruptTime = millis();
  
  currentState = SYSTEM_EMERGENCY;
  emergencyCount++;
  systemEnabled = false;
  
  Serial.println("🚨 EMERGENCY STOP ACTIVATED!");
  Serial.print("Emergency count: ");
  Serial.println(emergencyCount);
}

// 시스템 리셋 인터럽트
void systemResetISR() {
  if (millis() - lastInterruptTime < DEBOUNCE_DELAY) return;
  lastInterruptTime = millis();
  
  // 긴급 상태에서만 리셋 허용
  if (currentState == SYSTEM_EMERGENCY) {
    currentState = SYSTEM_RESET;
    resetCount++;
    
    Serial.println("🔄 System Reset Initiated...");
    Serial.print("Reset count: ");
    Serial.println(resetCount);
    
    // 3초 후 정상 상태로 복귀
    delay(3000);
    currentState = SYSTEM_NORMAL;
    systemEnabled = true;
    
    Serial.println("✅ System Reset Complete - Normal Operation Resumed");
  } else {
    Serial.println("⚠️ Reset ignored - Emergency stop required first");
  }
}

// 일반 이벤트 버튼 처리
void handleEventButton() {
  if (currentState == SYSTEM_NORMAL && systemEnabled) {
    currentState = SYSTEM_EVENT;
    eventCount++;
    
    Serial.println("📝 Event triggered");
    Serial.print("Event count: ");
    Serial.println(eventCount);
    
    // 이벤트 처리 시뮬레이션
    delay(100);
    currentState = SYSTEM_NORMAL;
  } else {
    Serial.println("⚠️ Event ignored - System not in normal state");
  }
}

// LED 상태 업데이트
void updateLEDStatus() {
  // 모든 LED 끄기
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  
  switch (currentState) {
    case SYSTEM_EMERGENCY:
      digitalWrite(RED_LED_PIN, HIGH);
      break;
    case SYSTEM_RESET:
      digitalWrite(YELLOW_LED_PIN, HIGH);
      break;
    case SYSTEM_EVENT:
      digitalWrite(YELLOW_LED_PIN, HIGH);
      digitalWrite(GREEN_LED_PIN, HIGH);
      break;
    case SYSTEM_NORMAL:
    default:
      digitalWrite(GREEN_LED_PIN, HIGH);
      break;
  }
}

// 시스템 초기화 표시
void systemInitialization() {
  Serial.println("Initializing system...");
  
  for (int i = 0; i < 3; i++) {
    digitalWrite(GREEN_LED_PIN, HIGH);
    delay(200);
    digitalWrite(GREEN_LED_PIN, LOW);
    delay(200);
  }
  
  Serial.println("System ready!");
}

// 시스템 상태 리포팅
void reportSystemStatus() {
  Serial.println("--- System Status Report ---");
  Serial.print("Current State: ");
  
  switch (currentState) {
    case SYSTEM_NORMAL:
      Serial.println("NORMAL");
      break;
    case SYSTEM_EMERGENCY:
      Serial.println("EMERGENCY");
      break;
    case SYSTEM_RESET:
      Serial.println("RESETTING");
      break;
    case SYSTEM_EVENT:
      Serial.println("EVENT PROCESSING");
      break;
  }
  
  Serial.print("System Enabled: ");
  Serial.println(systemEnabled ? "YES" : "NO");
  Serial.print("Emergency Count: ");
  Serial.println(emergencyCount);
  Serial.print("Reset Count: ");
  Serial.println(resetCount);
  Serial.print("Event Count: ");
  Serial.println(eventCount);
  Serial.print("Uptime: ");
  Serial.print(millis() / 1000);
  Serial.println(" seconds");
  Serial.println("----------------------------");
}