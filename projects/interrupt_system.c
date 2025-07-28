/*
 * Project 06: Interrupt-Driven System - Emergency Stop with Multiple Buttons
 * Description: Professional interrupt handling with debouncing and priority
 * Hardware: Arduino Uno, 3 buttons, 3 LEDs, resistors
 * 
 * 실무 중심 학습:
 * - External Interrupt 사용법
 * - 인터럽트 우선순위 관리
 * - 하드웨어 디바운싱
 * - 안전 시스템 설계 패턴
 * 
 * 인터럽트 핀:
 * - INT0 (PD2, Arduino 2) - 긴급 정지 (최고 우선순위)
 * - INT1 (PD3, Arduino 3) - 시스템 리셋
 * - PCINT (PB0, Arduino 8) - 일반 이벤트
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>

// 핀 정의 - 실무에서는 명확한 이름 사용
#define EMERGENCY_STOP_PIN PD2    // INT0 - 긴급 정지
#define SYSTEM_RESET_PIN PD3      // INT1 - 시스템 리셋  
#define EVENT_BUTTON_PIN PB0      // PCINT0 - 일반 이벤트

#define RED_LED_PIN PD6           // 긴급 상태 LED
#define YELLOW_LED_PIN PD7        // 시스템 상태 LED
#define GREEN_LED_PIN PB1         // 정상 동작 LED

// 시스템 상태 정의 (실무 패턴)
typedef enum {
    SYSTEM_NORMAL = 0,
    SYSTEM_EMERGENCY = 1,
    SYSTEM_RESET = 2,
    SYSTEM_EVENT = 3
} system_state_t;

// 전역 변수 (인터럽트에서 사용하므로 volatile 필수)
volatile system_state_t current_state = SYSTEM_NORMAL;
volatile uint32_t emergency_count = 0;
volatile uint32_t reset_count = 0;
volatile uint32_t event_count = 0;
volatile bool system_enabled = true;

// 디바운싱을 위한 변수
volatile uint32_t last_interrupt_time = 0;
#define DEBOUNCE_DELAY_MS 50

void setup_gpio(void) {
    // 버튼 핀들을 입력으로 설정하고 내부 풀업 활성화
    DDRD &= ~((1 << EMERGENCY_STOP_PIN) | (1 << SYSTEM_RESET_PIN));
    PORTD |= (1 << EMERGENCY_STOP_PIN) | (1 << SYSTEM_RESET_PIN);
    
    DDRB &= ~(1 << EVENT_BUTTON_PIN);
    PORTB |= (1 << EVENT_BUTTON_PIN);
    
    // LED 핀들을 출력으로 설정
    DDRD |= (1 << RED_LED_PIN) | (1 << YELLOW_LED_PIN);
    DDRB |= (1 << GREEN_LED_PIN);
    
    // 초기 LED 상태 (정상: 녹색 ON)
    PORTD &= ~((1 << RED_LED_PIN) | (1 << YELLOW_LED_PIN));
    PORTB |= (1 << GREEN_LED_PIN);
}

void setup_interrupts(void) {
    // External Interrupt 0 (INT0) - 긴급 정지 (falling edge)
    EICRA |= (1 << ISC01);  // Falling edge trigger
    EIMSK |= (1 << INT0);   // Enable INT0
    
    // External Interrupt 1 (INT1) - 시스템 리셋 (falling edge)
    EICRA |= (1 << ISC11);  // Falling edge trigger
    EIMSK |= (1 << INT1);   // Enable INT1
    
    // Pin Change Interrupt (PCINT0) - 일반 이벤트
    PCICR |= (1 << PCIE0);   // Enable PCINT[7:0]
    PCMSK0 |= (1 << PCINT0); // Enable PCINT0 (PB0)
    
    // 전역 인터럽트 활성화
    sei();
}

// 시간 기반 디바운싱 (실무 패턴)
bool is_debounced(void) {
    static uint32_t timer_counter = 0;
    timer_counter++;
    
    if (timer_counter - last_interrupt_time > DEBOUNCE_DELAY_MS) {
        last_interrupt_time = timer_counter;
        return true;
    }
    return false;
}

void update_led_status(void) {
    // 모든 LED 끄기
    PORTD &= ~((1 << RED_LED_PIN) | (1 << YELLOW_LED_PIN));
    PORTB &= ~(1 << GREEN_LED_PIN);
    
    switch (current_state) {
        case SYSTEM_EMERGENCY:
            PORTD |= (1 << RED_LED_PIN);     // 빨간색: 긴급
            break;
        case SYSTEM_RESET:
            PORTD |= (1 << YELLOW_LED_PIN);  // 노란색: 리셋
            break;
        case SYSTEM_EVENT:
            // 노란색 + 녹색 = 이벤트 처리 중
            PORTD |= (1 << YELLOW_LED_PIN);
            PORTB |= (1 << GREEN_LED_PIN);
            break;
        case SYSTEM_NORMAL:
        default:
            PORTB |= (1 << GREEN_LED_PIN);   // 녹색: 정상
            break;
    }
}

// 긴급 정지 인터럽트 (최고 우선순위)
ISR(INT0_vect) {
    if (!is_debounced()) return;
    
    current_state = SYSTEM_EMERGENCY;
    emergency_count++;
    system_enabled = false;  // 시스템 즉시 정지
    
    // 긴급 정지 시 모든 출력 안전 상태로
    PORTD &= ~(1 << YELLOW_LED_PIN);
    PORTB &= ~(1 << GREEN_LED_PIN);
    PORTD |= (1 << RED_LED_PIN);
}

// 시스템 리셋 인터럽트
ISR(INT1_vect) {
    if (!is_debounced()) return;
    
    // 긴급 상태에서만 리셋 허용 (안전 설계)
    if (current_state == SYSTEM_EMERGENCY) {
        current_state = SYSTEM_RESET;
        reset_count++;
        
        // 3초 후 정상 상태로 복귀 (실제로는 타이머 사용)
        _delay_ms(3000);
        current_state = SYSTEM_NORMAL;
        system_enabled = true;
    }
}

// Pin Change 인터럽트 (일반 이벤트)
ISR(PCINT0_vect) {
    // 버튼이 눌렸는지 확인 (falling edge 감지)
    if (!(PINB & (1 << EVENT_BUTTON_PIN))) {
        if (!is_debounced()) return;
        
        // 시스템이 정상 상태일 때만 처리
        if (current_state == SYSTEM_NORMAL && system_enabled) {
            current_state = SYSTEM_EVENT;
            event_count++;
            
            // 이벤트 처리 시뮬레이션 (100ms)
            _delay_ms(100);
            current_state = SYSTEM_NORMAL;
        }
    }
}

// 시스템 상태 진단 (실무에서 중요)
void system_diagnostics(void) {
    // 여기서는 시리얼 출력 대신 LED 패턴으로 표시
    // 실무에서는 로그 시스템 또는 디스플레이 출력
    
    if (emergency_count > 0) {
        // 긴급 정지 발생 이력이 있음을 표시
        for (int i = 0; i < 3; i++) {
            PORTD |= (1 << RED_LED_PIN);
            _delay_ms(100);
            PORTD &= ~(1 << RED_LED_PIN);
            _delay_ms(100);
        }
    }
}

int main(void) {
    setup_gpio();
    setup_interrupts();
    
    // 시스템 초기화 완료 표시
    for (int i = 0; i < 3; i++) {
        PORTB |= (1 << GREEN_LED_PIN);
        _delay_ms(200);
        PORTB &= ~(1 << GREEN_LED_PIN);
        _delay_ms(200);
    }
    PORTB |= (1 << GREEN_LED_PIN);  // 정상 상태 표시
    
    while (1) {
        // 메인 루프는 LED 상태 업데이트만 담당
        update_led_status();
        
        // 시스템 진단 (10초마다)
        static uint16_t diag_counter = 0;
        if (++diag_counter >= 10000) {
            diag_counter = 0;
            system_diagnostics();
        }
        
        _delay_ms(1);  // 1ms 간격으로 상태 확인
    }
    
    return 0;
}