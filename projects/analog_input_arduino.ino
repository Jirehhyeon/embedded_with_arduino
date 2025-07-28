/*
 * Project 04: Analog Input - Potentiometer LED Control (Arduino Version)
 * Description: Control LED brightness using a potentiometer
 * Hardware: Arduino Uno, LED, Potentiometer (10kΩ), 220Ω resistor
 * 
 * Connection:
 * - LED: Pin 6 (PWM) → 220Ω resistor → LED → GND
 * - Potentiometer: VCC → Pot → A0, Pot other end → GND
 */

const int LED_PIN = 6;      // PWM 핀
const int POT_PIN = A0;     // 아날로그 입력 핀

void setup() {
  pinMode(LED_PIN, OUTPUT);
  
  // 시리얼 통신 초기화 (디버깅용)
  Serial.begin(9600);
  Serial.println("=== Analog Input LED Control ===");
  Serial.println("Turn the potentiometer to control LED brightness");
}

void loop() {
  // 가변저항 값 읽기 (0~1023)
  int potValue = analogRead(POT_PIN);
  
  // PWM 값으로 변환 (0~255)
  int ledBrightness = map(potValue, 0, 1023, 0, 255);
  
  // LED 밝기 설정
  analogWrite(LED_PIN, ledBrightness);
  
  // 시리얼 모니터에 값 출력
  Serial.print("Pot: ");
  Serial.print(potValue);
  Serial.print(" → LED: ");
  Serial.print(ledBrightness);
  Serial.print(" (");
  Serial.print((ledBrightness * 100) / 255);
  Serial.println("%)");
  
  delay(100);  // 0.1초 딜레이
}