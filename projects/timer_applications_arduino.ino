/*
 * Project 07: Timer & Counter Applications (Arduino Version)
 * Description: Precision timing control and frequency measurement
 * Hardware: Arduino Uno, LED, Buzzer, Function generator (or PWM source)
 * 
 * Connection:
 * - Status LED: Pin 6 → 220Ω → LED → GND
 * - Buzzer: Pin 5 → Buzzer → GND (or Pin 5 → 100Ω → Speaker → GND)
 * - Frequency Input: Pin 8 (ICP1) ← Signal source
 * - Test PWM Output: Pin 9 (for testing frequency measurement)
 * 
 * Features:
 * - 1ms system tick using Timer0
 * - Frequency measurement using Timer1 Input Capture
 * - Tone generation using Timer2
 * - Simple real-time task scheduler
 */

#include <TimerOne.h>  // For advanced timer control (optional library)

// 핀 정의
const int STATUS_LED_PIN = 6;
const int BUZZER_PIN = 5;
const int FREQUENCY_INPUT_PIN = 8;  // ICP1 for input capture
const int TEST_PWM_PIN = 9;         // For generating test signals

// 시스템 변수
volatile unsigned long systemTickMs = 0;
volatile unsigned int ledBlinkCounter = 0;

// 주파수 측정 변수
volatile unsigned int captureCount = 0;
volatile unsigned int captureValues[2];
volatile bool frequencyReady = false;
volatile unsigned long measuredFrequency = 0;

// 톤 관련 변수
volatile bool toneActive = false;
volatile unsigned int toneDurationMs = 0;

// 태스크 스케줄러 구조체
struct Task {
  void (*function)();
  unsigned long periodMs;
  unsigned long lastRunMs;
  bool enabled;
};

void setup() {
  Serial.begin(9600);
  Serial.println("=== Timer & Counter Applications ===");
  Serial.println("Features:");
  Serial.println("- System tick (1ms precision)");
  Serial.println("- Frequency measurement via Input Capture");
  Serial.println("- Tone generation");
  Serial.println("- Real-time task scheduling");
  Serial.println();
  
  // 핀 설정
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(FREQUENCY_INPUT_PIN, INPUT);
  pinMode(TEST_PWM_PIN, OUTPUT);
  
  // 테스트용 PWM 신호 생성 (1kHz)
  analogWrite(TEST_PWM_PIN, 128);  // 50% duty cycle
  
  setupTimers();
  
  // 시스템 시작 신호
  playTone(1000, 500);
  delay(600);
  playTone(1500, 300);
  delay(400);
  
  Serial.println("System ready!");
  Serial.println("Connect Pin 9 to Pin 8 to test frequency measurement");
  Serial.println();
}

void loop() {
  // 실시간 스케줄러 실행
  runScheduler();
  
  // 시리얼 명령어 처리
  handleSerialCommands();
  
  delay(1);  // 1ms 간격
}

void setupTimers() {
  // Timer0는 Arduino에서 millis()에 사용되므로 그대로 활용
  
  // Timer1 설정 - Input Capture
  // Input Capture 인터럽트 설정
  TIMSK1 |= (1 << ICIE1);     // Enable Input Capture interrupt
  TCCR1B |= (1 << ICES1);     // Capture on rising edge
  TCCR1B |= (1 << CS10);      // No prescaling (16MHz)
  
  // Timer2는 tone() 함수에서 사용
  
  // 전역 인터럽트 활성화
  sei();
}

// Input Capture 인터럽트 (Timer1)
ISR(TIMER1_CAPT_vect) {
  captureValues[captureCount] = ICR1;
  captureCount++;
  
  if (captureCount >= 2) {
    unsigned int periodTicks = captureValues[1] - captureValues[0];
    
    if (periodTicks > 0) {
      measuredFrequency = 16000000UL / periodTicks;  // 16MHz clock
      frequencyReady = true;
    }
    
    captureCount = 0;
    TCNT1 = 0;  // Reset timer
  }
}

// 톤 생성 함수 (Arduino tone() 라이브러리 사용)
void playTone(unsigned int frequencyHz, unsigned int durationMs) {
  tone(BUZZER_PIN, frequencyHz, durationMs);
  toneActive = true;
  toneDurationMs = durationMs;
}

void stopTone() {
  noTone(BUZZER_PIN);
  toneActive = false;
}

// 정밀 논블로킹 딜레이
void preciseDelayMs(unsigned int delayMs) {
  unsigned long startTime = millis();
  while ((millis() - startTime) < delayMs) {
    // 다른 작업 수행 가능
    yield();  // Arduino 스케줄러에게 제어권 양보
  }
}

// 실시간 태스크들
void taskFrequencyMonitor() {
  if (frequencyReady) {
    frequencyReady = false;
    
    Serial.print("Measured Frequency: ");
    Serial.print(measuredFrequency);
    Serial.println(" Hz");
    
    // 주파수별 시각적 피드백
    if (measuredFrequency < 1000) {
      playTone(500, 100);   // 저주파 - 낮은 톤
      digitalWrite(STATUS_LED_PIN, HIGH);
      delay(50);
      digitalWrite(STATUS_LED_PIN, LOW);
    } else if (measuredFrequency < 5000) {
      playTone(1000, 100);  // 중주파 - 중간 톤
      for (int i = 0; i < 2; i++) {
        digitalWrite(STATUS_LED_PIN, HIGH);
        delay(25);
        digitalWrite(STATUS_LED_PIN, LOW);
        delay(25);
      }
    } else {
      playTone(2000, 100);  // 고주파 - 높은 톤
      for (int i = 0; i < 3; i++) {
        digitalWrite(STATUS_LED_PIN, HIGH);
        delay(15);
        digitalWrite(STATUS_LED_PIN, LOW);
        delay(15);
      }
    }
  }
}

void taskSystemMonitor() {
  static unsigned long lastReport = 0;
  static unsigned int systemHealth = 0;
  
  if (millis() - lastReport > 10000) {  // 10초마다
    lastReport = millis();
    systemHealth++;
    
    Serial.println("--- System Health Report ---");
    Serial.print("Uptime: ");
    Serial.print(millis() / 1000);
    Serial.println(" seconds");
    Serial.print("Health checks: ");
    Serial.println(systemHealth);
    Serial.print("Free RAM: ");
    Serial.print(getFreeRAM());
    Serial.println(" bytes");
    Serial.println("---------------------------");
  }
}

void taskStatusLED() {
  // 하트비트 LED (500ms 간격)
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  
  if (millis() - lastBlink > 500) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(STATUS_LED_PIN, ledState);
  }
}

void taskPeriodicBeep() {
  static unsigned long lastBeep = 0;
  
  if (millis() - lastBeep > 30000) {  // 30초마다
    lastBeep = millis();
    playTone(1500, 200);  // 확인 비프
  }
}

// 간단한 실시간 스케줄러
void runScheduler() {
  static Task tasks[] = {
    {taskFrequencyMonitor, 10, 0, true},    // 10ms마다
    {taskSystemMonitor, 1000, 0, true},     // 1초마다
    {taskStatusLED, 100, 0, true},          // 100ms마다
    {taskPeriodicBeep, 1000, 0, true}       // 1초마다 체크
  };
  
  int numTasks = sizeof(tasks) / sizeof(Task);
  unsigned long currentTime = millis();
  
  for (int i = 0; i < numTasks; i++) {
    if (tasks[i].enabled && 
        (currentTime - tasks[i].lastRunMs) >= tasks[i].periodMs) {
      
      tasks[i].function();
      tasks[i].lastRunMs = currentTime;
    }
  }
}

// 시리얼 명령어 처리
void handleSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readString();
    command.trim();
    
    if (command == "help") {
      Serial.println("Available commands:");
      Serial.println("- help: Show this help");
      Serial.println("- status: System status");
      Serial.println("- tone <freq> <duration>: Play tone");
      Serial.println("- test <freq>: Generate test signal");
      Serial.println("- freq: Show current frequency");
    }
    else if (command == "status") {
      Serial.print("System uptime: ");
      Serial.print(millis() / 1000);
      Serial.println(" seconds");
      Serial.print("Last measured frequency: ");
      Serial.print(measuredFrequency);
      Serial.println(" Hz");
    }
    else if (command.startsWith("tone")) {
      // 예: "tone 1000 500" - 1000Hz, 500ms
      int firstSpace = command.indexOf(' ');
      int secondSpace = command.indexOf(' ', firstSpace + 1);
      
      if (firstSpace > 0 && secondSpace > 0) {
        int freq = command.substring(firstSpace + 1, secondSpace).toInt();
        int duration = command.substring(secondSpace + 1).toInt();
        
        Serial.print("Playing tone: ");
        Serial.print(freq);
        Serial.print("Hz for ");
        Serial.print(duration);
        Serial.println("ms");
        
        playTone(freq, duration);
      }
    }
    else if (command.startsWith("test")) {
      int space = command.indexOf(' ');
      if (space > 0) {
        int freq = command.substring(space + 1).toInt();
        if (freq > 0 && freq < 32000) {
          // Timer1으로 정확한 주파수 생성
          int period = 16000000 / (2 * freq);
          Timer1.initialize(period);
          Timer1.pwm(TEST_PWM_PIN, 512);  // 50% duty cycle
          
          Serial.print("Generating ");
          Serial.print(freq);
          Serial.println("Hz test signal on Pin 9");
        }
      }
    }
    else if (command == "freq") {
      Serial.print("Current frequency: ");
      Serial.print(measuredFrequency);
      Serial.println(" Hz");
    }
    else {
      Serial.println("Unknown command. Type 'help' for available commands.");
    }
  }
}

// 메모리 사용량 확인 (디버깅용)
int getFreeRAM() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}