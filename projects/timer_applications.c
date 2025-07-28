/*
 * Project 07: Timer & Counter Applications - Precision Timing Control
 * Description: Professional timer usage for real-time applications
 * Hardware: Arduino Uno, LED, Buzzer, Input signal source
 * 
 * 실무 중심 학습:
 * - Timer/Counter 모드 설정 (Normal, CTC, Fast PWM)
 * - Input Capture로 주파수 측정
 * - 정밀한 타이밍 제어
 * - 실시간 스케줄링 기초
 * 
 * Timer 사용:
 * - Timer0: 시스템 틱 (1ms)
 * - Timer1: 주파수 측정 및 정밀 딜레이
 * - Timer2: PWM 톤 생성
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>

// 핀 정의
#define STATUS_LED_PIN PD6        // 상태 표시 LED
#define BUZZER_PIN PD5            // 톤 출력 (Timer0 OC0B)
#define INPUT_CAPTURE_PIN PB0     // ICP1 - 주파수 측정 입력

// 시스템 틱 관련 (Timer0)
volatile uint32_t system_tick_ms = 0;
volatile uint16_t led_blink_counter = 0;

// 주파수 측정 관련 (Timer1)
volatile uint16_t capture_count = 0;
volatile uint16_t capture_values[2];
volatile bool frequency_ready = false;
volatile uint32_t measured_frequency = 0;

// 톤 생성 관련 (Timer2)
volatile bool tone_active = false;
volatile uint16_t tone_duration_ms = 0;

// 실시간 스케줄링을 위한 태스크 구조체
typedef struct {
    void (*function)(void);
    uint32_t period_ms;
    uint32_t last_run_ms;
    bool enabled;
} task_t;

// Timer0 설정 - 시스템 틱 (1ms 간격)
void setup_timer0_systick(void) {
    // CTC 모드, OCR0A = 249 (1ms @ 16MHz, prescaler 64)
    // 16MHz / 64 / 250 = 1000Hz (1ms)
    TCCR0A |= (1 << WGM01);         // CTC mode
    TCCR0B |= (1 << CS01) | (1 << CS00);  // Prescaler 64
    OCR0A = 249;                    // Compare value for 1ms
    TIMSK0 |= (1 << OCIE0A);        // Enable compare match interrupt
}

// Timer1 설정 - Input Capture & 정밀 딜레이
void setup_timer1_capture(void) {
    // Normal mode, Input Capture on rising edge
    TCCR1B |= (1 << ICES1) | (1 << CS10);  // Rising edge, no prescaler
    TIMSK1 |= (1 << ICIE1);                 // Enable input capture interrupt
    
    // Input Capture 핀을 입력으로 설정
    DDRB &= ~(1 << INPUT_CAPTURE_PIN);
}

// Timer2 설정 - 톤 생성 (Fast PWM)
void setup_timer2_tone(void) {
    // Fast PWM mode, OC2B 출력
    TCCR2A |= (1 << WGM21) | (1 << WGM20);  // Fast PWM
    TCCR2A |= (1 << COM2B1);                // Clear OC2B on match
    DDRD |= (1 << BUZZER_PIN);              // OC2B를 출력으로 설정
}

void setup_gpio(void) {
    // LED 핀을 출력으로 설정
    DDRD |= (1 << STATUS_LED_PIN);
    PORTD &= ~(1 << STATUS_LED_PIN);
}

// 시스템 틱 ISR (1ms 간격)
ISR(TIMER0_COMPA_vect) {
    system_tick_ms++;
    
    // LED 깜빡임 (500ms 간격)
    if (++led_blink_counter >= 500) {
        led_blink_counter = 0;
        PORTD ^= (1 << STATUS_LED_PIN);  // LED 토글
    }
    
    // 톤 지속시간 관리
    if (tone_active && tone_duration_ms > 0) {
        tone_duration_ms--;
        if (tone_duration_ms == 0) {
            stop_tone();
        }
    }
}

// Input Capture ISR - 주파수 측정
ISR(TIMER1_CAPT_vect) {
    capture_values[capture_count] = ICR1;
    capture_count++;
    
    if (capture_count >= 2) {
        // 두 번의 캡처 값으로 주파수 계산
        uint16_t period_ticks = capture_values[1] - capture_values[0];
        
        // 주파수 = F_CPU / period_ticks
        if (period_ticks > 0) {
            measured_frequency = F_CPU / period_ticks;
            frequency_ready = true;
        }
        
        capture_count = 0;
        TCNT1 = 0;  // 타이머 리셋
    }
}

// 톤 생성 함수
void play_tone(uint16_t frequency_hz, uint16_t duration_ms) {
    if (frequency_hz == 0) {
        stop_tone();
        return;
    }
    
    // Timer2 주파수 설정
    // PWM 주파수 = F_CPU / (prescaler * 256)
    // 원하는 톤 주파수 = PWM 주파수 / 2
    uint8_t prescaler_bits = 0;
    uint16_t prescaler_value = 1;
    
    // 적절한 프리스케일러 선택
    uint32_t target_ocr = F_CPU / (frequency_hz * 2 * 256);
    
    if (target_ocr <= 1) {
        prescaler_bits = (1 << CS20);
        prescaler_value = 1;
    } else if (target_ocr <= 8) {
        prescaler_bits = (1 << CS21);
        prescaler_value = 8;
    } else if (target_ocr <= 32) {
        prescaler_bits = (1 << CS21) | (1 << CS20);
        prescaler_value = 32;
    } else {
        prescaler_bits = (1 << CS22);
        prescaler_value = 64;
    }
    
    TCCR2B = prescaler_bits;
    OCR2B = 128;  // 50% duty cycle
    
    tone_active = true;
    tone_duration_ms = duration_ms;
}

void stop_tone(void) {
    TCCR2B = 0;  // 타이머 정지
    PORTD &= ~(1 << BUZZER_PIN);  // 버저 끄기
    tone_active = false;
}

// 정밀 딜레이 함수 (ms 단위)
void precise_delay_ms(uint16_t delay_ms) {
    uint32_t start_time = system_tick_ms;
    while ((system_tick_ms - start_time) < delay_ms) {
        // 다른 작업 수행 가능 (논블로킹)
    }
}

// 실시간 태스크 함수들
void task_frequency_monitor(void) {
    if (frequency_ready) {
        frequency_ready = false;
        
        // 주파수별 톤 생성 (시각적 피드백)
        if (measured_frequency < 1000) {
            play_tone(500, 100);  // 저주파 - 낮은 톤
        } else if (measured_frequency < 5000) {
            play_tone(1000, 100); // 중주파 - 중간 톤
        } else {
            play_tone(2000, 100); // 고주파 - 높은 톤
        }
    }
}

void task_system_monitor(void) {
    // 시스템 상태 모니터링
    // 실제로는 여기서 센서 읽기, 상태 확인 등 수행
    static uint8_t monitor_counter = 0;
    
    if (++monitor_counter >= 10) {  // 1초마다 (100ms * 10)
        monitor_counter = 0;
        
        // 시스템 건강 상태 표시 (LED 패턴)
        PORTD |= (1 << STATUS_LED_PIN);
        precise_delay_ms(50);
        PORTD &= ~(1 << STATUS_LED_PIN);
    }
}

void task_beep_sequence(void) {
    // 주기적 비프음 (5초마다)
    static uint8_t beep_counter = 0;
    
    if (++beep_counter >= 50) {  // 5초마다 (100ms * 50)
        beep_counter = 0;
        play_tone(1500, 200);  // 1.5kHz, 200ms
    }
}

// 실시간 스케줄러 (간단한 버전)
void run_scheduler(void) {
    static task_t tasks[] = {
        {task_frequency_monitor, 10, 0, true},   // 10ms마다
        {task_system_monitor, 100, 0, true},     // 100ms마다
        {task_beep_sequence, 100, 0, true}       // 100ms마다 (내부에서 5초 주기)
    };
    
    uint8_t num_tasks = sizeof(tasks) / sizeof(task_t);
    
    for (uint8_t i = 0; i < num_tasks; i++) {
        if (tasks[i].enabled && 
            (system_tick_ms - tasks[i].last_run_ms) >= tasks[i].period_ms) {
            
            tasks[i].function();
            tasks[i].last_run_ms = system_tick_ms;
        }
    }
}

int main(void) {
    setup_gpio();
    setup_timer0_systick();
    setup_timer1_capture();
    setup_timer2_tone();
    
    // 전역 인터럽트 활성화
    sei();
    
    // 시스템 시작 신호
    play_tone(1000, 500);
    precise_delay_ms(600);
    play_tone(1500, 300);
    precise_delay_ms(400);
    
    while (1) {
        // 실시간 스케줄러 실행
        run_scheduler();
        
        // CPU 사용률 절약을 위한 짧은 딜레이
        _delay_ms(1);
    }
    
    return 0;
}