/*
 * Project 04: Analog Input - Potentiometer LED Control
 * Description: Control LED brightness using a potentiometer (variable resistor)
 * Hardware: Arduino Uno, LED, Potentiometer, resistor
 * 
 * 기능:
 * - 가변저항으로 LED 밝기 실시간 조절
 * - ADC 10비트 → PWM 8비트 변환
 * - 아날로그 입력을 디지털 신호로 변환
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define PWM_PIN PD6     // Arduino 6번 핀 (Timer0 OC0A)
#define POT_CHANNEL 0   // A0 핀 (ADC0)

void setup_adc(void) {
    // ADC 기준 전압: AVCC (5V)
    ADMUX |= (1 << REFS0);
    
    // ADC 채널 0 선택 (A0)
    ADMUX &= ~((1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0));
    
    // ADC 활성화, 프리스케일러 128 (16MHz/128 = 125kHz)
    ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

void setup_pwm(void) {
    // PWM 핀을 출력으로 설정
    DDRD |= (1 << PWM_PIN);
    
    // Timer0 Fast PWM 설정 (8비트)
    TCCR0A |= (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);
    TCCR0B |= (1 << CS01);  // 프리스케일러 8
    
    // 초기 밝기 0
    OCR0A = 0;
}

uint16_t read_adc(void) {
    // ADC 변환 시작
    ADCSRA |= (1 << ADSC);
    
    // 변환 완료까지 대기
    while (ADCSRA & (1 << ADSC));
    
    // 10비트 결과 반환 (0~1023)
    return ADC;
}

uint8_t map_adc_to_pwm(uint16_t adc_value) {
    // 10비트 ADC (0~1023)를 8비트 PWM (0~255)로 변환
    // adc_value * 255 / 1023 ≈ adc_value / 4
    return (uint8_t)(adc_value >> 2);
}

int main(void) {
    setup_adc();
    setup_pwm();
    
    uint16_t adc_value;
    uint8_t pwm_value;
    
    while(1) {
        // 가변저항 값 읽기
        adc_value = read_adc();
        
        // PWM 값으로 변환
        pwm_value = map_adc_to_pwm(adc_value);
        
        // LED 밝기 설정
        OCR0A = pwm_value;
        
        // 안정적인 읽기를 위한 짧은 딜레이
        _delay_ms(10);
    }
    
    return 0;
}