/*
 * Project 10: Real-Time Embedded System - Industrial Control System
 * Description: Professional real-time multitasking system with deterministic behavior
 * Hardware: Arduino Uno, Multiple I/O, Real-time constraints
 * 
 * 실무 중심 학습:
 * - 협력적 멀티태스킹 스케줄러
 * - 실시간 제약 조건 관리
 * - 태스크 우선순위 및 데드라인
 * - 리소스 공유 및 동기화
 * - 시스템 타이밍 분석
 * - 워치독 타이머 구현
 * 
 * 실시간 태스크:
 * - 고속 제어 루프 (1ms)
 * - 센서 데이터 수집 (10ms)
 * - 통신 처리 (50ms)
 * - 시스템 모니터링 (100ms)
 * - 사용자 인터페이스 (200ms)
 * 
 * 실시간 보장:
 * - 최대 응답 시간 < 2ms
 * - 지터(Jitter) < 100μs
 * - CPU 사용률 < 80%
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <string.h>

// 실시간 시스템 설정
#define MAX_TASKS 8
#define TICK_PERIOD_US 100      // 100μs 시스템 틱
#define SCHEDULER_FREQUENCY_HZ 10000  // 10kHz 스케줄러

// 태스크 상태 정의
typedef enum {
    TASK_READY = 0,
    TASK_RUNNING = 1,
    TASK_WAITING = 2,
    TASK_SUSPENDED = 3,
    TASK_COMPLETED = 4
} task_state_t;

// 태스크 우선순위 (0이 최고 우선순위)
typedef enum {
    PRIORITY_CRITICAL = 0,      // 실시간 제어
    PRIORITY_HIGH = 1,          // 센서 처리
    PRIORITY_NORMAL = 2,        // 통신
    PRIORITY_LOW = 3,           // 백그라운드
    PRIORITY_IDLE = 4           // 유휴 태스크
} task_priority_t;

// 태스크 제어 블록 (TCB)
typedef struct {
    uint8_t task_id;
    task_state_t state;
    task_priority_t priority;
    
    // 타이밍 정보
    uint32_t period_us;         // 주기 (마이크로초)
    uint32_t deadline_us;       // 데드라인
    uint32_t execution_time_us; // 실행 시간
    uint32_t next_run_time;     // 다음 실행 시간
    uint32_t last_start_time;   // 마지막 시작 시간
    uint32_t worst_case_time;   // 최악 실행 시간
    
    // 통계 정보
    uint32_t run_count;         // 실행 횟수
    uint32_t deadline_misses;   // 데드라인 위반 횟수
    uint32_t total_runtime;     // 총 실행 시간
    
    // 함수 포인터
    void (*task_function)(void);
    
    // 활성화 플래그
    bool enabled;
    bool periodic;
} task_control_block_t;

// 시스템 성능 메트릭
typedef struct {
    uint32_t total_ticks;
    uint32_t idle_ticks;
    uint32_t scheduler_overhead;
    uint8_t cpu_utilization;
    uint16_t max_response_time_us;
    uint16_t avg_jitter_us;
    uint8_t active_tasks;
} system_metrics_t;

// 전역 변수
volatile uint32_t system_tick_count = 0;
volatile bool scheduler_enabled = false;
task_control_block_t task_table[MAX_TASKS];
system_metrics_t metrics = {0};
volatile uint8_t current_task_id = 0;

// 실시간 I/O 상태
volatile struct {
    uint16_t analog_inputs[4];      // ADC 값들
    uint8_t digital_inputs;         // 디지털 입력 상태
    uint8_t digital_outputs;        // 디지털 출력 상태
    int16_t control_output;         // 제어 출력값
    bool emergency_stop;            // 긴급 정지
    uint16_t system_status;         // 시스템 상태 워드
} realtime_io = {0};

// 태스크 함수 선언
void task_control_loop(void);           // 1ms - 실시간 제어
void task_sensor_acquisition(void);    // 10ms - 센서 데이터 수집
void task_communication(void);         // 50ms - 통신 처리
void task_system_monitor(void);        // 100ms - 시스템 모니터링
void task_user_interface(void);        // 200ms - 사용자 인터페이스
void task_watchdog_service(void);      // 500ms - 워치독 서비스
void task_diagnostic(void);            // 1000ms - 시스템 진단
void task_idle(void);                  // 백그라운드 - 유휴 처리

// Timer1 설정 - 고정밀 시스템 틱 (100μs)
void setup_system_timer(void) {
    // Timer1 CTC 모드, 100μs 주기
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11);  // CTC, prescaler 8
    OCR1A = (F_CPU / 8 / SCHEDULER_FREQUENCY_HZ) - 1;  // 100μs
    TIMSK1 |= (1 << OCIE1A);              // Compare match interrupt
}

// 워치독 타이머 초기화
void setup_watchdog(void) {
    wdt_enable(WDTO_1S);  // 1초 워치독
}

// GPIO 초기화
void setup_gpio(void) {
    // 디지털 출력 (제어 신호)
    DDRD |= 0xF0;  // PD4-PD7 출력
    DDRB |= 0x3F;  // PB0-PB5 출력
    
    // 디지털 입력 (센서 신호)
    DDRC &= 0xF0;  // PC0-PC3 입력 (풀업 활성화)
    PORTC |= 0x0F;
    
    // 인터럽트 핀 (긴급 정지)
    DDRD &= ~(1 << PD2);  // PD2 입력
    PORTD |= (1 << PD2);   // 풀업 활성화
    
    // External interrupt 설정
    EICRA |= (1 << ISC01);  // Falling edge
    EIMSK |= (1 << INT0);   // Enable INT0
}

// ADC 초기화 (고속 변환)
void setup_adc(void) {
    ADMUX |= (1 << REFS0);  // AVCC 기준
    ADCSRA |= (1 << ADEN) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1);  // ADC 활성화, 인터럽트, 64분주
    ADCSRA |= (1 << ADSC);  // 첫 변환 시작
}

// 시스템 틱 인터럽트 (100μs)
ISR(TIMER1_COMPA_vect) {
    system_tick_count++;
    
    if (scheduler_enabled) {
        // 스케줄러 실행 시간 측정 시작
        uint32_t scheduler_start = system_tick_count;
        
        // 태스크 스케줄링
        schedule_tasks();
        
        // 스케줄러 오버헤드 계산
        metrics.scheduler_overhead += (system_tick_count - scheduler_start);
    }
    
    metrics.total_ticks++;
}

// 긴급 정지 인터럽트
ISR(INT0_vect) {
    realtime_io.emergency_stop = true;
    realtime_io.digital_outputs = 0;  // 모든 출력 정지
    PORTD &= 0x0F;  // 출력 핀들 클리어
    PORTB &= 0xC0;
}

// ADC 변환 완료 인터럽트
ISR(ADC_vect) {
    static uint8_t adc_channel = 0;
    
    // 현재 채널 값 저장
    realtime_io.analog_inputs[adc_channel] = ADC;
    
    // 다음 채널로 이동
    adc_channel = (adc_channel + 1) % 4;
    ADMUX = (ADMUX & 0xF0) | adc_channel;
    
    // 다음 변환 시작
    ADCSRA |= (1 << ADSC);
}

// 태스크 테이블 초기화
void init_task_table(void) {
    // Task 0: 실시간 제어 루프 (1ms, 최고 우선순위)
    task_table[0] = (task_control_block_t){
        .task_id = 0,
        .state = TASK_READY,
        .priority = PRIORITY_CRITICAL,
        .period_us = 1000,      // 1ms
        .deadline_us = 900,     // 0.9ms 데드라인
        .task_function = task_control_loop,
        .enabled = true,
        .periodic = true
    };
    
    // Task 1: 센서 데이터 수집 (10ms)
    task_table[1] = (task_control_block_t){
        .task_id = 1,
        .state = TASK_READY,
        .priority = PRIORITY_HIGH,
        .period_us = 10000,     // 10ms
        .deadline_us = 8000,    // 8ms 데드라인
        .task_function = task_sensor_acquisition,
        .enabled = true,
        .periodic = true
    };
    
    // Task 2: 통신 처리 (50ms)
    task_table[2] = (task_control_block_t){
        .task_id = 2,
        .state = TASK_READY,
        .priority = PRIORITY_NORMAL,
        .period_us = 50000,     // 50ms
        .deadline_us = 45000,   // 45ms 데드라인
        .task_function = task_communication,
        .enabled = true,
        .periodic = true
    };
    
    // Task 3: 시스템 모니터링 (100ms)
    task_table[3] = (task_control_block_t){
        .task_id = 3,
        .state = TASK_READY,
        .priority = PRIORITY_NORMAL,
        .period_us = 100000,    // 100ms
        .deadline_us = 90000,   // 90ms 데드라인
        .task_function = task_system_monitor,
        .enabled = true,
        .periodic = true
    };
    
    // Task 4: 사용자 인터페이스 (200ms)
    task_table[4] = (task_control_block_t){
        .task_id = 4,
        .state = TASK_READY,
        .priority = PRIORITY_LOW,
        .period_us = 200000,    // 200ms
        .deadline_us = 180000,  // 180ms 데드라인
        .task_function = task_user_interface,
        .enabled = true,
        .periodic = true
    };
    
    // Task 5: 워치독 서비스 (500ms)
    task_table[5] = (task_control_block_t){
        .task_id = 5,
        .state = TASK_READY,
        .priority = PRIORITY_HIGH,
        .period_us = 500000,    // 500ms
        .deadline_us = 450000,  // 450ms 데드라인
        .task_function = task_watchdog_service,
        .enabled = true,
        .periodic = true
    };
    
    // Task 6: 시스템 진단 (1000ms)
    task_table[6] = (task_control_block_t){
        .task_id = 6,
        .state = TASK_READY,
        .priority = PRIORITY_LOW,
        .period_us = 1000000,   // 1000ms
        .deadline_us = 900000,  // 900ms 데드라인
        .task_function = task_diagnostic,
        .enabled = true,
        .periodic = true
    };
    
    // Task 7: 유휴 태스크
    task_table[7] = (task_control_block_t){
        .task_id = 7,
        .state = TASK_READY,
        .priority = PRIORITY_IDLE,
        .period_us = 0,         // 비주기적
        .deadline_us = 0,
        .task_function = task_idle,
        .enabled = true,
        .periodic = false
    };
}

// 실시간 스케줄러 (Rate Monotonic + EDF)
void schedule_tasks(void) {
    uint32_t current_time = system_tick_count * TICK_PERIOD_US;
    uint8_t next_task = MAX_TASKS;  // 유효하지 않은 태스크 ID
    uint32_t earliest_deadline = 0xFFFFFFFF;
    
    // EDF (Earliest Deadline First) 스케줄링
    for (uint8_t i = 0; i < MAX_TASKS; i++) {
        if (!task_table[i].enabled || !task_table[i].periodic) continue;
        
        // 실행 시간 확인
        if (current_time >= task_table[i].next_run_time) {
            task_table[i].state = TASK_READY;
            
            // 가장 빠른 데드라인을 가진 태스크 선택
            uint32_t absolute_deadline = task_table[i].next_run_time + task_table[i].deadline_us;
            
            if (absolute_deadline < earliest_deadline) {
                earliest_deadline = absolute_deadline;
                next_task = i;
            }
        }
    }
    
    // 유휴 태스크 (실행할 태스크가 없을 때)
    if (next_task == MAX_TASKS) {
        next_task = 7;  // 유휴 태스크
        metrics.idle_ticks++;
    }
    
    // 선택된 태스크 실행
    if (next_task < MAX_TASKS) {
        execute_task(next_task);
    }
}

// 태스크 실행
void execute_task(uint8_t task_id) {
    if (task_id >= MAX_TASKS || !task_table[task_id].enabled) return;
    
    task_control_block_t* task = &task_table[task_id];
    uint32_t start_time = system_tick_count * TICK_PERIOD_US;
    
    // 태스크 상태 업데이트
    task->state = TASK_RUNNING;
    task->last_start_time = start_time;
    current_task_id = task_id;
    
    // 태스크 함수 실행
    task->task_function();
    
    // 실행 완료 후 통계 업데이트
    uint32_t end_time = system_tick_count * TICK_PERIOD_US;
    uint32_t execution_time = end_time - start_time;
    
    task->execution_time_us = execution_time;
    task->total_runtime += execution_time;
    task->run_count++;
    
    // 최악 실행 시간 업데이트
    if (execution_time > task->worst_case_time) {
        task->worst_case_time = execution_time;
    }
    
    // 응답 시간 메트릭 업데이트
    if (execution_time > metrics.max_response_time_us) {
        metrics.max_response_time_us = execution_time;
    }
    
    // 데드라인 체크
    if (execution_time > task->deadline_us) {
        task->deadline_misses++;
    }
    
    // 다음 실행 시간 설정 (주기적 태스크)
    if (task->periodic) {
        task->next_run_time += task->period_us;
        task->state = TASK_WAITING;
    } else {
        task->state = TASK_COMPLETED;
    }
}

// 태스크 구현들
void task_control_loop(void) {
    // 실시간 제어 루프 (1ms 주기, 최고 우선순위)
    if (realtime_io.emergency_stop) return;
    
    // 센서 값 읽기 (이미 ADC 인터럽트에서 업데이트됨)
    uint16_t sensor_value = realtime_io.analog_inputs[0];
    
    // PID 제어 알고리즘 (간단한 비례 제어)
    static uint16_t setpoint = 512;  // 목표값
    int16_t error = setpoint - sensor_value;
    realtime_io.control_output = error / 4;  // 비례 게인
    
    // 제어 출력 적용 (PWM 시뮬레이션)
    if (realtime_io.control_output > 0) {
        realtime_io.digital_outputs |= (1 << 0);
    } else {
        realtime_io.digital_outputs &= ~(1 << 0);
    }
    
    // 출력 포트 업데이트
    PORTD = (PORTD & 0x0F) | ((realtime_io.digital_outputs & 0x0F) << 4);
}

void task_sensor_acquisition(void) {
    // 센서 데이터 수집 및 필터링 (10ms 주기)
    
    // 디지털 입력 읽기
    realtime_io.digital_inputs = PINC & 0x0F;
    
    // 아날로그 센서 값 필터링 (이동 평균)
    static uint32_t filter_sum[4] = {0};
    static uint8_t filter_index = 0;
    static uint16_t filter_buffer[4][8];  // 8점 이동 평균
    
    for (uint8_t i = 0; i < 4; i++) {
        filter_sum[i] -= filter_buffer[i][filter_index];
        filter_buffer[i][filter_index] = realtime_io.analog_inputs[i];
        filter_sum[i] += filter_buffer[i][filter_index];
        realtime_io.analog_inputs[i] = filter_sum[i] / 8;
    }
    
    filter_index = (filter_index + 1) % 8;
    
    // 센서 범위 및 결함 검출
    for (uint8_t i = 0; i < 4; i++) {
        if (realtime_io.analog_inputs[i] < 50 || realtime_io.analog_inputs[i] > 1000) {
            realtime_io.system_status |= (1 << i);  // 센서 오류 플래그
        } else {
            realtime_io.system_status &= ~(1 << i);
        }
    }
}

void task_communication(void) {
    // 통신 처리 (50ms 주기)
    
    // 시스템 상태 패킷 생성 (간단한 바이너리 프로토콜)
    static uint8_t comm_buffer[16];
    
    comm_buffer[0] = 0xAA;  // 헤더
    comm_buffer[1] = 0x55;
    comm_buffer[2] = realtime_io.digital_inputs;
    comm_buffer[3] = realtime_io.digital_outputs;
    comm_buffer[4] = (realtime_io.analog_inputs[0] >> 8) & 0xFF;
    comm_buffer[5] = realtime_io.analog_inputs[0] & 0xFF;
    comm_buffer[6] = (realtime_io.analog_inputs[1] >> 8) & 0xFF;
    comm_buffer[7] = realtime_io.analog_inputs[1] & 0xFF;
    comm_buffer[8] = (realtime_io.control_output >> 8) & 0xFF;
    comm_buffer[9] = realtime_io.control_output & 0xFF;
    comm_buffer[10] = realtime_io.system_status & 0xFF;
    comm_buffer[11] = metrics.cpu_utilization;
    comm_buffer[12] = realtime_io.emergency_stop ? 0xFF : 0x00;
    
    // 체크섬 계산
    uint8_t checksum = 0;
    for (uint8_t i = 2; i < 13; i++) {
        checksum ^= comm_buffer[i];
    }
    comm_buffer[13] = checksum;
    comm_buffer[14] = 0xBB;  // 푸터
    comm_buffer[15] = 0xCC;
    
    // 실제로는 UART, SPI, I2C 등을 통해 전송
    // 여기서는 시뮬레이션만 수행
}

void task_system_monitor(void) {
    // 시스템 모니터링 (100ms 주기)
    
    // CPU 사용률 계산
    if (metrics.total_ticks > 0) {
        metrics.cpu_utilization = 100 - ((metrics.idle_ticks * 100) / metrics.total_ticks);
    }
    
    // 활성 태스크 개수 계산
    metrics.active_tasks = 0;
    for (uint8_t i = 0; i < MAX_TASKS; i++) {
        if (task_table[i].enabled) {
            metrics.active_tasks++;
        }
    }
    
    // 시스템 건강 상태 체크
    bool system_healthy = true;
    
    // 데드라인 위반 체크
    for (uint8_t i = 0; i < MAX_TASKS - 1; i++) {  // 유휴 태스크 제외
        if (task_table[i].deadline_misses > 0) {
            system_healthy = false;
            break;
        }
    }
    
    // CPU 과부하 체크
    if (metrics.cpu_utilization > 80) {
        system_healthy = false;
    }
    
    // 시스템 상태 LED 업데이트
    if (system_healthy) {
        PORTB |= (1 << PB5);   // 녹색 LED 켜기
        PORTB &= ~(1 << PB4);  // 빨간색 LED 끄기
    } else {
        PORTB &= ~(1 << PB5);  // 녹색 LED 끄기
        PORTB |= (1 << PB4);   // 빨간색 LED 켜기
    }
}

void task_user_interface(void) {
    // 사용자 인터페이스 (200ms 주기)
    
    // 상태 LED 점멸 패턴 (시스템 하트비트)
    static bool heartbeat_state = false;
    heartbeat_state = !heartbeat_state;
    
    if (heartbeat_state) {
        PORTB |= (1 << PB0);   // 하트비트 LED
    } else {
        PORTB &= ~(1 << PB0);
    }
    
    // 버튼 입력 처리 (실제로는 버튼 디바운싱 등 포함)
    static uint8_t last_input = 0;
    uint8_t current_input = realtime_io.digital_inputs;
    
    // 버튼 눌림 감지 (에지 트리거)
    uint8_t button_pressed = (~last_input) & current_input;
    
    if (button_pressed & 0x01) {
        // 버튼 1: 긴급 정지 리셋
        if (realtime_io.emergency_stop) {
            realtime_io.emergency_stop = false;
            realtime_io.system_status &= 0x00FF;  // 상위 바이트 클리어
        }
    }
    
    if (button_pressed & 0x02) {
        // 버튼 2: 시스템 리셋
        // 실제로는 소프트웨어 리셋 수행
    }
    
    last_input = current_input;
}

void task_watchdog_service(void) {
    // 워치독 서비스 (500ms 주기)
    
    // 모든 중요 태스크가 정상 실행 중인지 확인
    bool all_tasks_healthy = true;
    
    for (uint8_t i = 0; i < 4; i++) {  // 중요 태스크들만 체크
        if (task_table[i].enabled) {
            // 마지막 실행 시간이 주기의 2배를 넘으면 비정상
            uint32_t current_time = system_tick_count * TICK_PERIOD_US;
            if ((current_time - task_table[i].last_start_time) > (task_table[i].period_us * 2)) {
                all_tasks_healthy = false;
                break;
            }
        }
    }
    
    // 시스템이 정상이면 워치독 리셋
    if (all_tasks_healthy && !realtime_io.emergency_stop) {
        wdt_reset();
    }
    // 비정상이면 워치독이 시스템을 리셋함
}

void task_diagnostic(void) {
    // 시스템 진단 (1000ms 주기)
    
    // 성능 메트릭 리셋 (1초마다)
    static uint32_t last_total_ticks = 0;
    static uint32_t last_idle_ticks = 0;
    
    uint32_t tick_delta = metrics.total_ticks - last_total_ticks;
    uint32_t idle_delta = metrics.idle_ticks - last_idle_ticks;
    
    if (tick_delta > 0) {
        metrics.cpu_utilization = 100 - ((idle_delta * 100) / tick_delta);
    }
    
    last_total_ticks = metrics.total_ticks;
    last_idle_ticks = metrics.idle_ticks;
    
    // 태스크별 통계 분석
    for (uint8_t i = 0; i < MAX_TASKS - 1; i++) {
        if (task_table[i].run_count > 0) {
            // 평균 실행 시간 계산
            uint32_t avg_runtime = task_table[i].total_runtime / task_table[i].run_count;
            
            // 실행 시간이 데드라인의 80%를 넘으면 경고
            if (avg_runtime > (task_table[i].deadline_us * 80 / 100)) {
                realtime_io.system_status |= (1 << (8 + i));  // 경고 플래그
            }
        }
    }
    
    // 메모리 사용량 체크 (스택 오버플로우 감지)
    // 실제 구현에서는 스택 포인터 확인 등 수행
}

void task_idle(void) {
    // 유휴 태스크 - CPU 사용률 절약
    
    // 저전력 모드 진입 (실제로는 sleep 모드)
    // 여기서는 간단한 NOP 실행
    asm volatile("nop");
    
    // 시스템 상태 확인
    if (metrics.cpu_utilization > 95) {
        // CPU 과부하 시 비중요 태스크 일시 정지
        task_table[4].enabled = false;  // 사용자 인터페이스 태스크 정지
        task_table[6].enabled = false;  // 진단 태스크 정지
    } else if (metrics.cpu_utilization < 70) {
        // CPU 여유 시 태스크 재활성화
        task_table[4].enabled = true;
        task_table[6].enabled = true;
    }
}

int main(void) {
    // 시스템 초기화
    setup_gpio();
    setup_adc();
    setup_system_timer();
    setup_watchdog();
    
    // 태스크 테이블 초기화
    init_task_table();
    
    // 초기 다음 실행 시간 설정
    for (uint8_t i = 0; i < MAX_TASKS; i++) {
        if (task_table[i].periodic) {
            task_table[i].next_run_time = task_table[i].period_us;
        }
    }
    
    // 전역 인터럽트 활성화
    sei();
    
    // 스케줄러 활성화
    scheduler_enabled = true;
    
    // 메인 루프 (백그라운드 처리)
    while (1) {
        // 실시간 태스크들은 인터럽트에서 실행됨
        // 메인 루프는 중요하지 않은 백그라운드 작업만 수행
        
        // 시스템 재구성 또는 비상 처리
        if (realtime_io.emergency_stop) {
            // 모든 출력 정지
            realtime_io.digital_outputs = 0;
            PORTD &= 0x0F;
            PORTB &= 0xC0;
            
            // 중요하지 않은 태스크 정지
            for (uint8_t i = 2; i < MAX_TASKS - 1; i++) {
                task_table[i].enabled = false;
            }
        }
        
        // 메인 루프는 가능한 한 빠르게 실행
        _delay_us(10);
    }
    
    return 0;
}