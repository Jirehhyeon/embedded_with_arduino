 @"
  /*
   * LED Blink Program
   * Arduino/AVR 호환 코드
   */
  #include <avr/io.h>
  #include <util/delay.h>

  int main(void) {
      DDRB |= (1 << PB5);  // PB5(디지털 13번 핀)을 출력으로 설정

      while(1) {
          PORTB |= (1 << PB5);   // LED 켜기
          _delay_ms(1000);       // 1초 대기
          PORTB &= ~(1 << PB5);  // LED 끄기
          _delay_ms(1000);       // 1초 대기
      }

      return 0;
  }
  "@ | Out-File -FilePath "led_blink.c" -Encoding UTF8

  // Arduino IDE용 간단한 버전
  void setup() {
    pinMode(13, OUTPUT);  // 13번 핀을 출력으로 설정
  }

  void loop() {
    digitalWrite(13, HIGH);  // LED 켜기
    delay(1000);             // 1초 대기
    digitalWrite(13, LOW);   // LED 끄기
    delay(1000);             // 1초 대기
  }