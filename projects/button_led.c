/*
 * Project 02: Button LED Control
 * Description: Basic digital input control with button debouncing
 * Hardware: Arduino Uno, LED, Button, Resistors
 * 
 * 학습 목표:
 * - 디지털 입력 읽기
 * - 풀업 저항 사용법
 * - 버튼 디바운싱 처리
 * - 상태 토글 로직
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define LED_PIN PB5     // 디지털 13번 (포트B, 핀5)
#define BUTTON_PIN PD2  // 디지털 2번 (포트D, 핀2)

int main(void) {
    // LED 핀을 출력으로 설정
    DDRB |= (1 << LED_PIN);
    
    // 버튼 핀을 입력으로 설정하고 풀업 저항 활성화
    DDRD &= ~(1 << BUTTON_PIN);  // 입력으로 설정
    PORTD |= (1 << BUTTON_PIN);  // 풀업 저항 활성화
    
    uint8_t led_state = 0;      // LED 현재 상태
    uint8_t button_prev = 1;    // 이전 버튼 상태 (풀업이므로 1이 기본)
    
    while(1) {
        // 현재 버튼 상태 읽기 (0이면 눌림, 1이면 안눌림)
        uint8_t button_current = (PIND & (1 << BUTTON_PIN)) ? 1 : 0;
        
        // 버튼이 눌렸을 때 (falling edge 감지)
        if (button_prev == 1 && button_current == 0) {
            led_state = !led_state;  // LED 상태 토글
            
            if (led_state) {
                PORTB |= (1 << LED_PIN);   // LED 켜기
            } else {
                PORTB &= ~(1 << LED_PIN);  // LED 끄기
            }
            
            _delay_ms(50);  // 디바운싱용 딜레이
        }
        
        button_prev = button_current;
        _delay_ms(10);  // 메인 루프 딜레이
    }
    
    return 0;
}