/*
 * Project 03 Advanced: PWM LED with Button Control
 * Description: Control LED brightness manually using buttons
 * Hardware: Arduino Uno, LED, 2 buttons, resistors
 * 
 * 기능:
 * - 버튼1: 밝기 증가 (0~255)
 * - 버튼2: 밝기 감소 (255~0)
 * - PWM으로 부드러운 밝기 조절
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define PWM_PIN PD6     // Arduino 6번 핀 (Timer0 OC0A)
#define BUTTON_UP PD2   // 밝기 증가 버튼 (2번 핀)
#define BUTTON_DOWN PD3 // 밝기 감소 버튼 (3번 핀)

void setup_pwm(void) {
    // PWM 핀을 출력으로 설정
    DDRD |= (1 << PWM_PIN);
    
    // 버튼 핀들을 입력으로 설정하고 풀업 활성화
    DDRD &= ~((1 << BUTTON_UP) | (1 << BUTTON_DOWN));
    PORTD |= (1 << BUTTON_UP) | (1 << BUTTON_DOWN);
    
    // Timer0 Fast PWM 설정
    TCCR0A |= (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);
    TCCR0B |= (1 << CS01);  // 프리스케일러 8
    
    // 초기 밝기 128 (중간)
    OCR0A = 128;
}

uint8_t read_button(uint8_t pin) {
    return !(PIND & (1 << pin));  // 버튼이 눌리면 1 반환
}

int main(void) {
    setup_pwm();
    
    uint8_t brightness = 128;  // 중간 밝기로 시작
    uint8_t prev_up = 0, prev_down = 0;
    
    while(1) {
        uint8_t curr_up = read_button(BUTTON_UP);
        uint8_t curr_down = read_button(BUTTON_DOWN);
        
        // 밝기 증가 버튼 (falling edge)
        if (prev_up == 0 && curr_up == 1) {
            if (brightness < 255) {
                brightness += 10;  // 10씩 증가
                if (brightness > 255) brightness = 255;
                OCR0A = brightness;
            }
            _delay_ms(50);  // 디바운싱
        }
        
        // 밝기 감소 버튼 (falling edge)
        if (prev_down == 0 && curr_down == 1) {
            if (brightness > 0) {
                brightness -= 10;  // 10씩 감소
                OCR0A = brightness;
            }
            _delay_ms(50);  // 디바운싱
        }
        
        prev_up = curr_up;
        prev_down = curr_down;
        
        _delay_ms(10);
    }
    
    return 0;
}