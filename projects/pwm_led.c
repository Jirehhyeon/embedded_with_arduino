/*
 * Project 03: PWM LED Brightness Control
 * Description: Control LED brightness using PWM (Timer0)
 * Hardware: Arduino Uno, LED, 220Ω resistor
 * 
 * 학습 목표:
 * - PWM(Pulse Width Modulation) 원리 이해
 * - Timer0 Fast PWM 모드 설정
 * - 듀티 사이클로 밝기 조절
 * - 자동 밝기 변화 (Breathing Effect)
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define PWM_PIN PD6  // Arduino 6번 핀 (Timer0 OC0A)

void setup_pwm(void) {
    // PWM 핀을 출력으로 설정
    DDRD |= (1 << PWM_PIN);
    
    /*
     * Timer0 Fast PWM 설정
     * COM0A1:COM0A0 = 10 (Non-inverting mode)
     * WGM02:WGM01:WGM00 = 011 (Fast PWM, TOP=0xFF)
     */
    TCCR0A |= (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);
    
    /*
     * 프리스케일러 설정 (CS02:CS01:CS00 = 010)
     * 16MHz / 8 = 2MHz
     * PWM 주파수 = 2MHz / 256 = 7.8kHz
     */
    TCCR0B |= (1 << CS01);
    
    // 초기 밝기 0 (꺼진 상태)
    OCR0A = 0;
}

void set_brightness(uint8_t brightness) {
    /*
     * OCR0A 레지스터에 듀티 사이클 값 설정
     * 0 = 0% (완전히 꺼짐)
     * 255 = 100% (최대 밝기)
     */
    OCR0A = brightness;
}

int main(void) {
    setup_pwm();
    
    uint8_t brightness = 0;
    int8_t direction = 1;  // 1: 밝아짐, -1: 어두워짐
    
    while(1) {
        // 현재 밝기 설정
        set_brightness(brightness);
        
        // 밝기 변화 (Breathing Effect)
        brightness += direction * 2;
        
        // 방향 전환 (0 ↔ 255)
        if (brightness >= 255) {
            brightness = 255;
            direction = -1;
        } else if (brightness <= 0) {
            brightness = 0;
            direction = 1;
        }
        
        _delay_ms(20);  // 부드러운 변화를 위한 딜레이
    }
    
    return 0;
}