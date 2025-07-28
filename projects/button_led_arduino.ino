/*
 * Project 02: Button LED Control (Arduino IDE Version)
 * Description: Simplified version using Arduino functions
 * Hardware: Arduino Uno, LED, Button, Resistors
 */

const int ledPin = 13;
const int buttonPin = 2;

bool ledState = false;
bool buttonPrev = HIGH;

void setup() {
  // 핀 모드 설정
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // 내장 풀업 저항 사용
  
  // 시리얼 통신 초기화 (디버깅용)
  Serial.begin(9600);
  Serial.println("Button LED Control Started!");
}

void loop() {
  // 현재 버튼 상태 읽기
  bool buttonCurrent = digitalRead(buttonPin);
  
  // 버튼이 눌렸을 때 (HIGH에서 LOW로 변경)
  if (buttonPrev == HIGH && buttonCurrent == LOW) {
    ledState = !ledState;  // LED 상태 토글
    digitalWrite(ledPin, ledState);
    
    // 시리얼 모니터에 상태 출력
    Serial.print("Button pressed! LED is now: ");
    Serial.println(ledState ? "ON" : "OFF");
    
    delay(50);  // 디바운싱
  }
  
  buttonPrev = buttonCurrent;
  delay(10);
}