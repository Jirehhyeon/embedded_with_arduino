/*
 * Project 03: PWM LED Brightness Control (Arduino IDE Version)
 * Description: Control LED brightness using analogWrite()
 * Hardware: Arduino Uno, LED, 220Ω resistor
 */

const int ledPin = 6;  // PWM 핀 (6번)

void setup() {
  pinMode(ledPin, OUTPUT);
  
  // 시리얼 통신 초기화 (밝기 값 모니터링)
  Serial.begin(9600);
  Serial.println("PWM LED Brightness Control Started!");
  Serial.println("Brightness range: 0-255");
}

void loop() {
  // 밝아지는 효과 (0 → 255)
  for (int brightness = 0; brightness <= 255; brightness += 2) {
    analogWrite(ledPin, brightness);
    Serial.print("Brightness: ");
    Serial.println(brightness);
    delay(20);
  }
  
  // 어두워지는 효과 (255 → 0) 
  for (int brightness = 255; brightness >= 0; brightness -= 2) {
    analogWrite(ledPin, brightness);
    Serial.print("Brightness: ");
    Serial.println(brightness);
    delay(20);
  }
}