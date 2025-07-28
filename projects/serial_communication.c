/*
 * Project 05: Serial Communication - Temperature Sensor Monitor
 * Description: Read analog sensor and send data via UART
 * Hardware: Arduino Uno, Analog sensor (LM35 or potentiometer), LED
 * 
 * 기능:
 * - 아날로그 센서 값을 읽어서 시리얼로 전송
 * - 온도 센서 시뮬레이션 (LM35 대신 가변저항 사용 가능)
 * - 임계값 초과 시 LED 경고
 * - 실시간 데이터 모니터링
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#define SENSOR_CHANNEL 0    // A0 핀 (ADC0) - 센서 입력
#define LED_PIN PD6         // 6번 핀 - 경고 LED
#define BAUD_RATE 9600
#define TEMP_THRESHOLD 30   // 경고 온도 (°C)

// UART 초기화
void uart_init(void) {
    uint16_t ubrr = F_CPU / 16 / BAUD_RATE - 1;
    
    // 보드율 설정
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;
    
    // 송신, 수신 활성화
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0);
    
    // 8비트 데이터, 1 스톱비트, 패리티 없음
    UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);
}

// 한 문자 전송
void uart_transmit(unsigned char data) {
    // 송신 버퍼가 비워질 때까지 대기
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

// 문자열 전송
void uart_print(const char* str) {
    while (*str) {
        uart_transmit(*str++);
    }
}

// 숫자를 문자열로 변환 후 전송
void uart_print_number(uint16_t num) {
    char buffer[6];
    sprintf(buffer, "%d", num);
    uart_print(buffer);
}

void setup_adc(void) {
    // ADC 기준 전압: AVCC (5V)
    ADMUX |= (1 << REFS0);
    
    // ADC 채널 0 선택 (A0)
    ADMUX &= ~((1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0));
    
    // ADC 활성화, 프리스케일러 128
    ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

void setup_led(void) {
    // LED 핀을 출력으로 설정
    DDRD |= (1 << LED_PIN);
    PORTD &= ~(1 << LED_PIN);  // 초기에는 끄기
}

uint16_t read_adc(void) {
    // ADC 변환 시작
    ADCSRA |= (1 << ADSC);
    
    // 변환 완료까지 대기
    while (ADCSRA & (1 << ADSC));
    
    return ADC;
}

// ADC 값을 온도로 변환 (LM35 센서 시뮬레이션)
float convert_to_temperature(uint16_t adc_value) {
    // LM35: 10mV/°C, 5V 기준
    // adc_value / 1023 * 5V * 100 = 온도
    return (float)adc_value * 5.0 * 100.0 / 1023.0;
}

void control_warning_led(float temperature) {
    if (temperature > TEMP_THRESHOLD) {
        PORTD |= (1 << LED_PIN);   // LED 켜기
    } else {
        PORTD &= ~(1 << LED_PIN);  // LED 끄기
    }
}

int main(void) {
    uart_init();
    setup_adc();
    setup_led();
    
    // 시작 메시지
    uart_print("=== Temperature Monitor System ===\r\n");
    uart_print("Format: [Time] ADC: xxx, Temp: xx.x C, Status: xxx\r\n");
    uart_print("Warning threshold: ");
    uart_print_number(TEMP_THRESHOLD);
    uart_print(" C\r\n\r\n");
    
    uint32_t time_counter = 0;
    
    while(1) {
        // 센서 값 읽기
        uint16_t adc_value = read_adc();
        float temperature = convert_to_temperature(adc_value);
        
        // 경고 LED 제어
        control_warning_led(temperature);
        
        // 시리얼로 데이터 전송
        uart_print("[");
        uart_print_number(time_counter);
        uart_print("s] ADC: ");
        uart_print_number(adc_value);
        uart_print(", Temp: ");
        uart_print_number((uint16_t)(temperature * 10));  // 소수점 1자리
        uart_print(" C, Status: ");
        
        if (temperature > TEMP_THRESHOLD) {
            uart_print("WARNING!");
        } else {
            uart_print("Normal");
        }
        uart_print("\r\n");
        
        time_counter++;
        _delay_ms(1000);  // 1초마다 측정
    }
    
    return 0;
}