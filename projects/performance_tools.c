/*
 * Performance Benchmarking Tools for Embedded Systems
 * Description: Comprehensive performance analysis and optimization tools
 * 
 * Features:
 * - Execution time measurement (microsecond precision)
 * - Memory usage analysis (stack, heap, global)
 * - CPU utilization monitoring
 * - I/O performance testing
 * - Real-time system metrics
 * - Performance regression detection
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <string.h>

// 성능 측정 구조체
typedef struct {
    uint32_t start_time;
    uint32_t end_time;
    uint32_t min_time;
    uint32_t max_time;
    uint32_t total_time;
    uint32_t call_count;
    char name[16];
} performance_counter_t;

// 시스템 메트릭 구조체
typedef struct {
    uint32_t cpu_cycles;
    uint32_t idle_cycles;
    uint16_t free_ram;
    uint16_t stack_usage;
    uint8_t cpu_utilization;
    uint16_t interrupt_count;
    uint32_t context_switches;
} system_metrics_t;

// 전역 변수
static performance_counter_t counters[16];
static uint8_t counter_count = 0;
static system_metrics_t metrics = {0};
static volatile uint32_t system_ticks = 0;

// 외부 심볼 (링커 스크립트에서 정의)
extern char __data_start, __data_end;
extern char __bss_start, __bss_end;
extern char __heap_start, __heap_end;
extern char __stack;

// 고정밀 타이머 초기화 (Timer1, 1μs 해상도)
void performance_timer_init(void) {
    // Timer1 설정: CTC 모드, 1MHz (16MHz/16)
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);  // CTC, prescaler 64
    OCR1A = 249;  // 1μs @ 16MHz/64
    TIMSK1 |= (1 << OCIE1A);
    
    // 시스템 틱 카운터 활성화
    sei();
}

// 타이머 인터럽트 (1μs마다)
ISR(TIMER1_COMPA_vect) {
    system_ticks++;
    metrics.cpu_cycles++;
}

// 마이크로초 단위 시간 반환
uint32_t micros(void) {
    uint32_t ticks;
    cli();
    ticks = system_ticks;
    sei();
    return ticks;
}

// 성능 카운터 초기화
performance_counter_t* perf_counter_create(const char* name) {
    if (counter_count >= 16) return NULL;
    
    performance_counter_t* counter = &counters[counter_count++];
    strncpy(counter->name, name, sizeof(counter->name) - 1);
    counter->name[sizeof(counter->name) - 1] = '\0';
    counter->min_time = 0xFFFFFFFF;
    counter->max_time = 0;
    counter->total_time = 0;
    counter->call_count = 0;
    
    return counter;
}

// 성능 측정 시작
void perf_counter_start(performance_counter_t* counter) {
    if (!counter) return;
    counter->start_time = micros();
}

// 성능 측정 종료
void perf_counter_end(performance_counter_t* counter) {
    if (!counter) return;
    
    counter->end_time = micros();
    uint32_t elapsed = counter->end_time - counter->start_time;
    
    counter->total_time += elapsed;
    counter->call_count++;
    
    if (elapsed < counter->min_time) {
        counter->min_time = elapsed;
    }
    if (elapsed > counter->max_time) {
        counter->max_time = elapsed;
    }
}

// 성능 카운터 리셋
void perf_counter_reset(performance_counter_t* counter) {
    if (!counter) return;
    
    counter->min_time = 0xFFFFFFFF;
    counter->max_time = 0;
    counter->total_time = 0;
    counter->call_count = 0;
}

// 모든 성능 카운터 출력 (시리얼 필요)
void perf_counters_print(void) {
    // 시리얼 출력이 가능한 환경에서 사용
    // printf("=== Performance Counters ===\n");
    // for (uint8_t i = 0; i < counter_count; i++) {
    //     performance_counter_t* c = &counters[i];
    //     if (c->call_count > 0) {
    //         uint32_t avg = c->total_time / c->call_count;
    //         printf("%s: calls=%lu, avg=%luμs, min=%luμs, max=%luμs\n",
    //                c->name, c->call_count, avg, c->min_time, c->max_time);
    //     }
    // }
    // printf("========================\n");
}

// 메모리 사용량 분석
void memory_usage_analyze(void) {
    // 데이터 세그먼트 크기
    uint16_t data_size = &__data_end - &__data_start;
    
    // BSS 세그먼트 크기
    uint16_t bss_size = &__bss_end - &__bss_start;
    
    // 스택 사용량 추정 (SP 레지스터 기반)
    uint16_t stack_ptr = SP;
    uint16_t stack_usage = (uint16_t)&__stack - stack_ptr;
    
    // 힙 사용량 (malloc 사용 시)
    // extern char* __brkval;
    // uint16_t heap_usage = __brkval ? ((char*)__brkval - &__heap_start) : 0;
    
    // 총 사용 RAM
    uint16_t total_used = data_size + bss_size + stack_usage;
    uint16_t total_ram = 2048;  // Arduino Uno: 2KB RAM
    
    metrics.free_ram = total_ram - total_used;
    metrics.stack_usage = stack_usage;
}

// CPU 사용률 계산
void cpu_utilization_update(void) {
    static uint32_t last_total_cycles = 0;
    static uint32_t last_idle_cycles = 0;
    static uint32_t last_update = 0;
    
    uint32_t current_time = micros();
    
    // 1초마다 업데이트
    if (current_time - last_update >= 1000000) {
        uint32_t cycle_delta = metrics.cpu_cycles - last_total_cycles;
        uint32_t idle_delta = metrics.idle_cycles - last_idle_cycles;
        
        if (cycle_delta > 0) {
            metrics.cpu_utilization = 100 - ((idle_delta * 100) / cycle_delta);
        }
        
        last_total_cycles = metrics.cpu_cycles;
        last_idle_cycles = metrics.idle_cycles;
        last_update = current_time;
    }
}

// 유휴 시간 카운터 (메인 루프에서 호출)
void cpu_idle_tick(void) {
    metrics.idle_cycles++;
}

// 인터럽트 카운터 증가 (각 ISR에서 호출)
void interrupt_count_increment(void) {
    metrics.interrupt_count++;
}

// I/O 성능 테스트
typedef struct {
    uint32_t read_time;
    uint32_t write_time;
    uint16_t throughput_kbps;
} io_performance_t;

// GPIO 성능 테스트
io_performance_t test_gpio_performance(uint8_t pin) {
    io_performance_t result = {0};
    uint32_t start_time, end_time;
    const uint16_t iterations = 1000;
    
    // GPIO 쓰기 성능 테스트
    start_time = micros();
    for (uint16_t i = 0; i < iterations; i++) {
        PORTB |= (1 << pin);
        PORTB &= ~(1 << pin);
    }
    end_time = micros();
    result.write_time = (end_time - start_time) / iterations;
    
    // GPIO 읽기 성능 테스트
    volatile uint8_t dummy;
    start_time = micros();
    for (uint16_t i = 0; i < iterations; i++) {
        dummy = PINB & (1 << pin);
    }
    end_time = micros();
    result.read_time = (end_time - start_time) / iterations;
    
    // 처리량 계산 (단위: operations/sec)
    if (result.write_time > 0) {
        result.throughput_kbps = (1000000 / result.write_time) / 1000;
    }
    
    return result;
}

// ADC 성능 테스트
io_performance_t test_adc_performance(uint8_t channel) {
    io_performance_t result = {0};
    uint32_t start_time, end_time;
    const uint16_t samples = 100;
    
    // ADC 설정
    ADMUX = (1 << REFS0) | (channel & 0x0F);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    
    // ADC 변환 성능 테스트
    start_time = micros();
    for (uint16_t i = 0; i < samples; i++) {
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC));
        volatile uint16_t adc_value = ADC;
    }
    end_time = micros();
    
    result.read_time = (end_time - start_time) / samples;
    
    // 샘플링 레이트 계산
    if (result.read_time > 0) {
        result.throughput_kbps = (1000000 / result.read_time) / 1000;
    }
    
    return result;
}

// 메모리 대역폭 테스트
uint32_t test_memory_bandwidth(void) {
    const uint16_t buffer_size = 512;
    static uint8_t buffer1[buffer_size];
    static uint8_t buffer2[buffer_size];
    
    uint32_t start_time = micros();
    
    // 메모리 복사 테스트
    for (uint8_t i = 0; i < 10; i++) {
        memcpy(buffer2, buffer1, buffer_size);
    }
    
    uint32_t end_time = micros();
    uint32_t total_bytes = buffer_size * 10;
    uint32_t bandwidth_bps = (total_bytes * 1000000) / (end_time - start_time);
    
    return bandwidth_bps;
}

// 산술 연산 성능 테스트
typedef struct {
    uint32_t add_time;
    uint32_t mul_time;
    uint32_t div_time;
    uint32_t float_time;
} math_performance_t;

math_performance_t test_math_performance(void) {
    math_performance_t result = {0};
    const uint16_t iterations = 1000;
    uint32_t start_time, end_time;
    
    // 정수 덧셈 테스트
    volatile uint32_t a = 12345, b = 67890, c;
    start_time = micros();
    for (uint16_t i = 0; i < iterations; i++) {
        c = a + b;
    }
    end_time = micros();
    result.add_time = (end_time - start_time) / iterations;
    
    // 정수 곱셈 테스트
    start_time = micros();
    for (uint16_t i = 0; i < iterations; i++) {
        c = a * b;
    }
    end_time = micros();
    result.mul_time = (end_time - start_time) / iterations;
    
    // 정수 나눗셈 테스트
    start_time = micros();
    for (uint16_t i = 0; i < iterations; i++) {
        c = a / (b >> 8);  // 0으로 나누기 방지
    }
    end_time = micros();
    result.div_time = (end_time - start_time) / iterations;
    
    // 부동소수점 연산 테스트
    volatile float fa = 123.45f, fb = 678.90f, fc;
    start_time = micros();
    for (uint16_t i = 0; i < iterations; i++) {
        fc = fa * fb;
    }
    end_time = micros();
    result.float_time = (end_time - start_time) / iterations;
    
    return result;
}

// 실시간 성능 모니터링
typedef struct {
    uint32_t worst_case_latency;
    uint32_t average_latency;
    uint16_t jitter;
    uint8_t deadline_misses;
} realtime_metrics_t;

static realtime_metrics_t rt_metrics = {0};

// 실시간 태스크 성능 추적
void realtime_task_monitor(uint32_t start_time, uint32_t deadline_us) {
    uint32_t end_time = micros();
    uint32_t latency = end_time - start_time;
    
    // 최악 지연시간 업데이트
    if (latency > rt_metrics.worst_case_latency) {
        rt_metrics.worst_case_latency = latency;
    }
    
    // 평균 지연시간 계산 (이동평균)
    static uint32_t latency_sum = 0;
    static uint16_t sample_count = 0;
    
    latency_sum += latency;
    sample_count++;
    
    if (sample_count >= 100) {
        rt_metrics.average_latency = latency_sum / sample_count;
        latency_sum = 0;
        sample_count = 0;
    }
    
    // 지터 계산 (평균에서의 편차)
    if (rt_metrics.average_latency > 0) {
        uint32_t deviation = (latency > rt_metrics.average_latency) ?
            (latency - rt_metrics.average_latency) :
            (rt_metrics.average_latency - latency);
        
        if (deviation > rt_metrics.jitter) {
            rt_metrics.jitter = deviation;
        }
    }
    
    // 데드라인 위반 검사
    if (latency > deadline_us) {
        rt_metrics.deadline_misses++;
    }
}

// 성능 회귀 감지
typedef struct {
    uint32_t baseline_time;
    uint32_t current_time;
    float regression_threshold;  // 회귀 임계값 (%)
    bool regression_detected;
} regression_detector_t;

void regression_detector_init(regression_detector_t* detector, 
                             uint32_t baseline, float threshold) {
    detector->baseline_time = baseline;
    detector->regression_threshold = threshold;
    detector->regression_detected = false;
}

bool regression_detector_check(regression_detector_t* detector, uint32_t current) {
    detector->current_time = current;
    
    if (detector->baseline_time > 0) {
        float change_percent = ((float)(current - detector->baseline_time) / 
                               detector->baseline_time) * 100.0f;
        
        if (change_percent > detector->regression_threshold) {
            detector->regression_detected = true;
            return true;
        }
    }
    
    detector->regression_detected = false;
    return false;
}

// 종합 성능 리포트 생성
void generate_performance_report(void) {
    memory_usage_analyze();
    cpu_utilization_update();
    
    // LED 패턴으로 성능 상태 표시
    if (metrics.cpu_utilization > 90) {
        // 빨간색: CPU 과부하
        PORTB |= (1 << PB0);
        PORTB &= ~(1 << PB1);
    } else if (metrics.cpu_utilization > 70) {
        // 노란색: 높은 사용률
        PORTB |= (1 << PB0) | (1 << PB1);
    } else {
        // 초록색: 정상
        PORTB &= ~(1 << PB0);
        PORTB |= (1 << PB1);
    }
    
    // 메모리 부족 경고
    if (metrics.free_ram < 256) {  // 256바이트 미만
        // 빠른 깜빡임으로 경고
        static uint8_t blink_counter = 0;
        if (++blink_counter % 10 == 0) {
            PORTB ^= (1 << PB2);
        }
    }
}

// 성능 최적화 제안
typedef enum {
    OPTIMIZE_NONE = 0,
    OPTIMIZE_CPU = 1,
    OPTIMIZE_MEMORY = 2,
    OPTIMIZE_IO = 4,
    OPTIMIZE_REALTIME = 8
} optimization_flags_t;

uint8_t analyze_optimization_opportunities(void) {
    uint8_t flags = OPTIMIZE_NONE;
    
    // CPU 최적화 필요
    if (metrics.cpu_utilization > 80) {
        flags |= OPTIMIZE_CPU;
    }
    
    // 메모리 최적화 필요
    if (metrics.free_ram < 512) {  // 25% 미만
        flags |= OPTIMIZE_MEMORY;
    }
    
    // 실시간 성능 최적화 필요
    if (rt_metrics.deadline_misses > 0 || rt_metrics.jitter > 100) {
        flags |= OPTIMIZE_REALTIME;
    }
    
    return flags;
}

// 메인 성능 모니터링 루프
void performance_monitor_task(void) {
    static uint32_t last_report = 0;
    uint32_t current_time = micros();
    
    // 1초마다 리포트 생성
    if (current_time - last_report >= 1000000) {
        generate_performance_report();
        
        // 최적화 기회 분석
        uint8_t opt_flags = analyze_optimization_opportunities();
        
        // 성능 카운터 통계 업데이트
        // (실제 환경에서는 로그 파일이나 네트워크로 전송)
        
        last_report = current_time;
    }
    
    cpu_idle_tick();  // 유휴 시간 카운트
}