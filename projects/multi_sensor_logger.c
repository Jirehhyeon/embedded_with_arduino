/*
 * Project 09: Multi-Sensor Data Logger - IoT Ready System
 * Description: Professional multi-sensor data collection and fusion system
 * Hardware: Arduino Uno, Multiple sensors, SD card, Real-time processing
 * 
 * 실무 중심 학습:
 * - 센서 융합 및 데이터 검증
 * - 상태 머신 기반 시스템 설계
 * - 데이터 패킷화 및 압축
 * - 메모리 효율적 버퍼링
 * - 에러 복구 및 시스템 복원력
 * 
 * 지원 센서:
 * - 온도/습도 (I2C)
 * - 광도 센서 (ADC)
 * - 압력 센서 (시뮬레이션)
 * - 가속도계 (I2C)
 * - 배터리 전압 모니터링
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdbool.h>
#include <string.h>

// 시스템 설정
#define MAX_SENSORS 5
#define BUFFER_SIZE 64
#define PACKET_SIZE 16
#define SAMPLE_RATE_MS 1000

// 센서 타입 정의
typedef enum {
    SENSOR_TEMPERATURE = 0,
    SENSOR_HUMIDITY = 1,
    SENSOR_LIGHT = 2,
    SENSOR_PRESSURE = 3,
    SENSOR_ACCELERATION = 4
} sensor_type_t;

// 센서 상태
typedef enum {
    SENSOR_OK = 0,
    SENSOR_ERROR = 1,
    SENSOR_TIMEOUT = 2,
    SENSOR_CALIBRATING = 3,
    SENSOR_DISABLED = 4
} sensor_status_t;

// 센서 데이터 구조체
typedef struct {
    sensor_type_t type;
    int32_t value;              // 스케일된 정수값 (x1000)
    uint32_t timestamp;         // 시스템 틱
    sensor_status_t status;
    uint8_t quality;            // 데이터 품질 (0-100)
    uint16_t checksum;          // 데이터 무결성
} sensor_reading_t;

// 센서 설정 구조체
typedef struct {
    sensor_type_t type;
    bool enabled;
    uint16_t sample_period_ms;
    uint32_t last_sample_time;
    int32_t min_value;
    int32_t max_value;
    uint8_t error_count;
    bool (*read_function)(sensor_reading_t*);
    void (*calibrate_function)(void);
} sensor_config_t;

// 데이터 패킷 구조체 (메모리 효율적)
typedef struct __attribute__((packed)) {
    uint16_t packet_id;
    uint32_t timestamp;
    uint8_t sensor_count;
    sensor_reading_t readings[MAX_SENSORS];
    uint16_t packet_checksum;
} data_packet_t;

// 시스템 상태 머신
typedef enum {
    STATE_INIT = 0,
    STATE_CALIBRATION,
    STATE_NORMAL_OPERATION,
    STATE_DATA_LOGGING,
    STATE_ERROR_RECOVERY,
    STATE_LOW_POWER,
    STATE_SHUTDOWN
} system_state_t;

// 전역 변수
volatile uint32_t system_tick_ms = 0;
volatile system_state_t current_state = STATE_INIT;

// 센서 데이터 버퍼 (링 버퍼)
sensor_reading_t sensor_buffer[BUFFER_SIZE];
volatile uint8_t buffer_head = 0;
volatile uint8_t buffer_tail = 0;
volatile uint8_t buffer_count = 0;

// 패킷 버퍼
data_packet_t packet_buffer[8];
volatile uint8_t packet_head = 0;
volatile uint8_t packet_tail = 0;

// 시스템 통계
struct {
    uint32_t total_samples;
    uint32_t error_count;
    uint32_t packets_generated;
    uint16_t buffer_overflow_count;
    uint8_t system_health;
} system_stats = {0};

// 센서 읽기 함수 선언
bool read_temperature_humidity(sensor_reading_t* reading);
bool read_light_sensor(sensor_reading_t* reading);
bool read_pressure_sensor(sensor_reading_t* reading);
bool read_acceleration(sensor_reading_t* reading);
bool read_battery_voltage(sensor_reading_t* reading);

// 센서 보정 함수 선언
void calibrate_light_sensor(void);
void calibrate_pressure_sensor(void);
void calibrate_acceleration(void);

// 센서 설정 배열
sensor_config_t sensors[MAX_SENSORS] = {
    {SENSOR_TEMPERATURE, true, 2000, 0, -40000, 85000, 0, read_temperature_humidity, NULL},
    {SENSOR_LIGHT, true, 500, 0, 0, 100000, 0, read_light_sensor, calibrate_light_sensor},
    {SENSOR_PRESSURE, true, 1000, 0, 800000, 1200000, 0, read_pressure_sensor, calibrate_pressure_sensor},
    {SENSOR_ACCELERATION, true, 100, 0, -32000, 32000, 0, read_acceleration, calibrate_acceleration},
    {SENSOR_HUMIDITY, true, 2000, 0, 0, 100000, 0, read_temperature_humidity, NULL}
};

// Timer0 설정 - 시스템 틱 (1ms)
void setup_system_timer(void) {
    TCCR0A |= (1 << WGM01);         // CTC mode
    TCCR0B |= (1 << CS01) | (1 << CS00);  // Prescaler 64
    OCR0A = 249;                    // 1ms @ 16MHz
    TIMSK0 |= (1 << OCIE0A);        // Enable interrupt
}

// 시스템 틱 인터럽트
ISR(TIMER0_COMPA_vect) {
    system_tick_ms++;
}

// I2C 초기화 (간단 버전)
void i2c_init(void) {
    DDRC &= ~((1 << PC4) | (1 << PC5));  // SDA, SCL을 입력으로
    PORTC |= (1 << PC4) | (1 << PC5);    // 풀업 활성화
    
    TWBR = ((F_CPU / 100000UL) - 16) / 2;  // 100kHz
    TWCR = (1 << TWEN);
}

// ADC 초기화
void adc_init(void) {
    ADMUX |= (1 << REFS0);                    // AVCC 기준전압
    ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // ADC 활성화, 128 분주
}

// ADC 값 읽기
uint16_t adc_read(uint8_t channel) {
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

// 체크섬 계산
uint16_t calculate_checksum(const uint8_t* data, uint8_t length) {
    uint16_t checksum = 0;
    for (uint8_t i = 0; i < length; i++) {
        checksum += data[i];
    }
    return checksum;
}

// 센서 읽기 함수들
bool read_temperature_humidity(sensor_reading_t* reading) {
    // 온도/습도 센서 시뮬레이션 (실제로는 DHT22, SHT30 등 사용)
    static int32_t base_temp = 25000;   // 25.0도
    static int32_t base_hum = 60000;    // 60.0%
    
    // 시간 기반 변화 시뮬레이션
    uint32_t time_factor = system_tick_ms / 1000;
    int32_t temp_variation = (time_factor % 100) - 50;  // ±5도
    int32_t hum_variation = (time_factor % 200) - 100;  // ±10%
    
    if (reading->type == SENSOR_TEMPERATURE) {
        reading->value = base_temp + temp_variation * 100;
        reading->quality = 95;
    } else if (reading->type == SENSOR_HUMIDITY) {
        reading->value = base_hum + hum_variation * 100;
        reading->quality = 90;
    }
    
    reading->timestamp = system_tick_ms;
    reading->status = SENSOR_OK;
    reading->checksum = calculate_checksum((uint8_t*)&reading->value, sizeof(reading->value));
    
    return true;
}

bool read_light_sensor(sensor_reading_t* reading) {
    // 광도 센서 (포토다이오드, LDR 등)
    uint16_t adc_value = adc_read(0);  // A0 핀
    
    // ADC → 럭스 변환 (보정된 값)
    reading->value = (int32_t)adc_value * 100;  // 0-102300 럭스 범위
    reading->timestamp = system_tick_ms;
    reading->status = SENSOR_OK;
    reading->quality = (adc_value > 50) ? 95 : 70;  // 낮은 조도에서 품질 저하
    reading->checksum = calculate_checksum((uint8_t*)&reading->value, sizeof(reading->value));
    
    return true;
}

bool read_pressure_sensor(sensor_reading_t* reading) {
    // 압력 센서 시뮬레이션 (BMP280, MS5611 등)
    static int32_t base_pressure = 1013250;  // 1013.25 hPa
    
    // 고도 및 날씨 변화 시뮬레이션
    uint32_t time_factor = system_tick_ms / 10000;  // 10초 단위
    int32_t pressure_variation = (time_factor % 100) - 50;  // ±5 hPa
    
    reading->value = base_pressure + pressure_variation * 100;
    reading->timestamp = system_tick_ms;
    reading->status = SENSOR_OK;
    reading->quality = 98;  // 압력 센서는 일반적으로 정확함
    reading->checksum = calculate_checksum((uint8_t*)&reading->value, sizeof(reading->value));
    
    return true;
}

bool read_acceleration(sensor_reading_t* reading) {
    // 가속도계 시뮬레이션 (MPU6050, ADXL345 등)
    // X, Y, Z 축 중 하나만 시뮬레이션 (실제로는 3축 모두)
    static int32_t base_accel = 0;  // 정지 상태
    
    // 진동/움직임 시뮬레이션
    uint32_t time_factor = system_tick_ms / 100;  // 0.1초 단위
    int32_t accel_variation = ((time_factor % 40) - 20) * 100;  // ±2g
    
    reading->value = base_accel + accel_variation;
    reading->timestamp = system_tick_ms;
    reading->status = SENSOR_OK;
    reading->quality = 92;
    reading->checksum = calculate_checksum((uint8_t*)&reading->value, sizeof(reading->value));
    
    return true;
}

bool read_battery_voltage(sensor_reading_t* reading) {
    // 배터리 전압 모니터링 (전압 분배기 사용)
    uint16_t adc_value = adc_read(1);  // A1 핀
    
    // ADC → 전압 변환 (5V 기준, 전압 분배기 2:1)
    reading->value = (int32_t)adc_value * 5000 * 2 / 1023;  // mV 단위
    reading->timestamp = system_tick_ms;
    reading->status = SENSOR_OK;
    reading->quality = 99;  // 전압 측정은 매우 정확
    reading->checksum = calculate_checksum((uint8_t*)&reading->value, sizeof(reading->value));
    
    return true;
}

// 센서 보정 함수들
void calibrate_light_sensor(void) {
    // 광도 센서 보정 (어두운 환경에서 0점 조정)
    _delay_ms(1000);  // 안정화 대기
}

void calibrate_pressure_sensor(void) {
    // 압력 센서 보정 (해수면 기압 기준)
    _delay_ms(2000);  // 안정화 대기
}

void calibrate_acceleration(void) {
    // 가속도계 보정 (중력 기준 1g 조정)
    _delay_ms(1500);  // 안정화 대기
}

// 링 버퍼에 센서 데이터 추가
bool add_sensor_data(const sensor_reading_t* reading) {
    if (buffer_count >= BUFFER_SIZE) {
        system_stats.buffer_overflow_count++;
        return false;  // 버퍼 오버플로우
    }
    
    memcpy(&sensor_buffer[buffer_head], reading, sizeof(sensor_reading_t));
    buffer_head = (buffer_head + 1) % BUFFER_SIZE;
    buffer_count++;
    
    return true;
}

// 링 버퍼에서 센서 데이터 읽기
bool get_sensor_data(sensor_reading_t* reading) {
    if (buffer_count == 0) {
        return false;  // 버퍼 비어있음
    }
    
    memcpy(reading, &sensor_buffer[buffer_tail], sizeof(sensor_reading_t));
    buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
    buffer_count--;
    
    return true;
}

// 데이터 패킷 생성
bool create_data_packet(data_packet_t* packet) {
    static uint16_t packet_id = 0;
    
    packet->packet_id = packet_id++;
    packet->timestamp = system_tick_ms;
    packet->sensor_count = 0;
    
    // 버퍼에서 센서 데이터 수집
    for (uint8_t i = 0; i < MAX_SENSORS && packet->sensor_count < MAX_SENSORS; i++) {
        sensor_reading_t reading;
        if (get_sensor_data(&reading)) {
            packet->readings[packet->sensor_count++] = reading;
        }
    }
    
    if (packet->sensor_count == 0) {
        return false;  // 데이터 없음
    }
    
    // 패킷 체크섬 계산
    packet->packet_checksum = calculate_checksum((uint8_t*)packet, 
                                                sizeof(data_packet_t) - sizeof(uint16_t));
    
    system_stats.packets_generated++;
    return true;
}

// 센서 데이터 수집
void collect_sensor_data(void) {
    for (uint8_t i = 0; i < MAX_SENSORS; i++) {
        if (!sensors[i].enabled) continue;
        
        // 샘플링 주기 확인
        if ((system_tick_ms - sensors[i].last_sample_time) >= sensors[i].sample_period_ms) {
            sensors[i].last_sample_time = system_tick_ms;
            
            sensor_reading_t reading;
            reading.type = sensors[i].type;
            
            // 센서 읽기
            if (sensors[i].read_function(&reading)) {
                // 범위 검증
                if (reading.value >= sensors[i].min_value && 
                    reading.value <= sensors[i].max_value) {
                    
                    add_sensor_data(&reading);
                    system_stats.total_samples++;
                    sensors[i].error_count = 0;  // 에러 카운트 리셋
                } else {
                    sensors[i].error_count++;
                    system_stats.error_count++;
                }
            } else {
                sensors[i].error_count++;
                system_stats.error_count++;
                
                // 연속 에러 시 센서 비활성화
                if (sensors[i].error_count > 5) {
                    sensors[i].enabled = false;
                }
            }
        }
    }
}

// 시스템 상태 머신
void update_system_state(void) {
    static uint32_t state_enter_time = 0;
    static bool state_changed = true;
    
    if (state_changed) {
        state_enter_time = system_tick_ms;
        state_changed = false;
    }
    
    uint32_t time_in_state = system_tick_ms - state_enter_time;
    
    switch (current_state) {
        case STATE_INIT:
            if (time_in_state > 1000) {  // 1초 초기화
                current_state = STATE_CALIBRATION;
                state_changed = true;
            }
            break;
            
        case STATE_CALIBRATION:
            // 센서 보정 수행
            for (uint8_t i = 0; i < MAX_SENSORS; i++) {
                if (sensors[i].enabled && sensors[i].calibrate_function) {
                    sensors[i].calibrate_function();
                }
            }
            current_state = STATE_NORMAL_OPERATION;
            state_changed = true;
            break;
            
        case STATE_NORMAL_OPERATION:
            collect_sensor_data();
            
            // 버퍼가 충분히 찼으면 데이터 로깅 모드로
            if (buffer_count > BUFFER_SIZE / 2) {
                current_state = STATE_DATA_LOGGING;
                state_changed = true;
            }
            
            // 에러가 많으면 복구 모드로
            if (system_stats.error_count > 100) {
                current_state = STATE_ERROR_RECOVERY;
                state_changed = true;
            }
            break;
            
        case STATE_DATA_LOGGING:
            collect_sensor_data();
            
            // 패킷 생성 및 저장
            data_packet_t packet;
            if (create_data_packet(&packet)) {
                // 여기서 패킷을 EEPROM, SD카드 등에 저장
                // 또는 무선 전송
            }
            
            // 버퍼가 비워지면 정상 모드로 복귀
            if (buffer_count < BUFFER_SIZE / 4) {
                current_state = STATE_NORMAL_OPERATION;
                state_changed = true;
            }
            break;
            
        case STATE_ERROR_RECOVERY:
            // 센서 재초기화
            for (uint8_t i = 0; i < MAX_SENSORS; i++) {
                sensors[i].error_count = 0;
                sensors[i].enabled = true;
            }
            
            system_stats.error_count = 0;
            current_state = STATE_CALIBRATION;
            state_changed = true;
            break;
            
        case STATE_LOW_POWER:
            // 저전력 모드 (배터리 전압 낮을 때)
            _delay_ms(5000);  // 5초 대기
            current_state = STATE_NORMAL_OPERATION;
            state_changed = true;
            break;
            
        case STATE_SHUTDOWN:
            // 시스템 종료 준비
            break;
    }
}

// 시스템 건강 상태 계산
void update_system_health(void) {
    uint8_t active_sensors = 0;
    uint8_t healthy_sensors = 0;
    
    for (uint8_t i = 0; i < MAX_SENSORS; i++) {
        if (sensors[i].enabled) {
            active_sensors++;
            if (sensors[i].error_count < 3) {
                healthy_sensors++;
            }
        }
    }
    
    if (active_sensors == 0) {
        system_stats.system_health = 0;
    } else {
        system_stats.system_health = (healthy_sensors * 100) / active_sensors;
    }
    
    // 버퍼 오버플로우도 건강 상태에 영향
    if (system_stats.buffer_overflow_count > 10) {
        system_stats.system_health = (system_stats.system_health * 80) / 100;
    }
}

int main(void) {
    // 시스템 초기화
    setup_system_timer();
    i2c_init();
    adc_init();
    
    // 전역 인터럽트 활성화
    sei();
    
    // 메인 루프
    while (1) {
        update_system_state();
        update_system_health();
        
        // 100ms마다 실행
        static uint32_t last_update = 0;
        if ((system_tick_ms - last_update) >= 100) {
            last_update = system_tick_ms;
            
            // 시스템 상태 LED 표시 (여기서는 생략)
            // 실제로는 RGB LED로 상태 표시
        }
        
        _delay_ms(10);  // CPU 부하 줄이기
    }
    
    return 0;
}