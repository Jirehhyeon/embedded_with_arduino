/*
 * Project 08: I2C Communication - Multi-Sensor Interface
 * Description: Professional I2C implementation for sensor communication
 * Hardware: Arduino Uno, DS1307 RTC, 24C256 EEPROM, DHT22 (I2C version)
 * 
 * 실무 중심 학습:
 * - I2C 프로토콜 직접 구현
 * - Master/Slave 통신
 * - 에러 처리 및 타임아웃
 * - 다중 디바이스 관리
 * - 데이터 무결성 검증
 * 
 * I2C 연결:
 * - SDA: Arduino A4 (PC4)
 * - SCL: Arduino A5 (PC5)
 * - 4.7kΩ 풀업 저항 필수
 * 
 * 지원 디바이스:
 * - DS1307 RTC (0x68)
 * - 24C256 EEPROM (0x50)
 * - 가상 온도센서 (0x48)
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include <string.h>

// I2C 핀 정의
#define SDA_PIN PC4
#define SCL_PIN PC5

// I2C 디바이스 주소
#define DS1307_ADDR     0x68    // RTC
#define EEPROM_ADDR     0x50    // 24C256 EEPROM
#define TEMP_SENSOR_ADDR 0x48   // 온도 센서 (시뮬레이션)

// I2C 상태 코드
#define I2C_SUCCESS     0x00
#define I2C_ERROR_START 0x01
#define I2C_ERROR_ADDR  0x02
#define I2C_ERROR_DATA  0x03
#define I2C_ERROR_STOP  0x04
#define I2C_ERROR_TIMEOUT 0x05

// I2C 클럭 주파수 설정 (100kHz)
#define I2C_BITRATE 100000UL

// RTC 데이터 구조체
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;
} rtc_time_t;

// 센서 데이터 구조체
typedef struct {
    int16_t temperature;    // 0.1도 단위
    uint16_t humidity;      // 0.1% 단위
    bool valid;
} sensor_data_t;

// I2C 초기화
void i2c_init(void) {
    // I2C 핀을 출력으로 설정하고 풀업 활성화
    DDRC &= ~((1 << SDA_PIN) | (1 << SCL_PIN));
    PORTC |= (1 << SDA_PIN) | (1 << SCL_PIN);
    
    // TWI 모듈 초기화
    // TWBR = (F_CPU / I2C_BITRATE - 16) / 2
    TWBR = ((F_CPU / I2C_BITRATE) - 16) / 2;
    TWCR = (1 << TWEN);  // TWI 활성화
}

// I2C 시작 조건
uint8_t i2c_start(void) {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    
    // 완료까지 대기 (타임아웃 포함)
    uint16_t timeout = 1000;
    while (!(TWCR & (1 << TWINT)) && timeout--) {
        _delay_us(10);
    }
    
    if (timeout == 0) return I2C_ERROR_TIMEOUT;
    
    // 상태 확인
    if ((TWSR & 0xF8) != 0x08 && (TWSR & 0xF8) != 0x10) {
        return I2C_ERROR_START;
    }
    
    return I2C_SUCCESS;
}

// I2C 정지 조건
uint8_t i2c_stop(void) {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    
    // 정지 조건 완료까지 대기
    uint16_t timeout = 1000;
    while ((TWCR & (1 << TWSTO)) && timeout--) {
        _delay_us(10);
    }
    
    return (timeout > 0) ? I2C_SUCCESS : I2C_ERROR_TIMEOUT;
}

// I2C 주소 전송 (쓰기)
uint8_t i2c_write_addr(uint8_t addr) {
    TWDR = (addr << 1);  // 쓰기 비트 (0)
    TWCR = (1 << TWINT) | (1 << TWEN);
    
    uint16_t timeout = 1000;
    while (!(TWCR & (1 << TWINT)) && timeout--) {
        _delay_us(10);
    }
    
    if (timeout == 0) return I2C_ERROR_TIMEOUT;
    
    if ((TWSR & 0xF8) != 0x18) {
        return I2C_ERROR_ADDR;
    }
    
    return I2C_SUCCESS;
}

// I2C 주소 전송 (읽기)
uint8_t i2c_read_addr(uint8_t addr) {
    TWDR = (addr << 1) | 0x01;  // 읽기 비트 (1)
    TWCR = (1 << TWINT) | (1 << TWEN);
    
    uint16_t timeout = 1000;
    while (!(TWCR & (1 << TWINT)) && timeout--) {
        _delay_us(10);
    }
    
    if (timeout == 0) return I2C_ERROR_TIMEOUT;
    
    if ((TWSR & 0xF8) != 0x40) {
        return I2C_ERROR_ADDR;
    }
    
    return I2C_SUCCESS;
}

// I2C 데이터 쓰기
uint8_t i2c_write_data(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    
    uint16_t timeout = 1000;
    while (!(TWCR & (1 << TWINT)) && timeout--) {
        _delay_us(10);
    }
    
    if (timeout == 0) return I2C_ERROR_TIMEOUT;
    
    if ((TWSR & 0xF8) != 0x28) {
        return I2C_ERROR_DATA;
    }
    
    return I2C_SUCCESS;
}

// I2C 데이터 읽기 (ACK 포함)
uint8_t i2c_read_data_ack(uint8_t *data) {
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    
    uint16_t timeout = 1000;
    while (!(TWCR & (1 << TWINT)) && timeout--) {
        _delay_us(10);
    }
    
    if (timeout == 0) return I2C_ERROR_TIMEOUT;
    
    if ((TWSR & 0xF8) != 0x50) {
        return I2C_ERROR_DATA;
    }
    
    *data = TWDR;
    return I2C_SUCCESS;
}

// I2C 데이터 읽기 (NACK 포함)
uint8_t i2c_read_data_nack(uint8_t *data) {
    TWCR = (1 << TWINT) | (1 << TWEN);
    
    uint16_t timeout = 1000;
    while (!(TWCR & (1 << TWINT)) && timeout--) {
        _delay_us(10);
    }
    
    if (timeout == 0) return I2C_ERROR_TIMEOUT;
    
    if ((TWSR & 0xF8) != 0x58) {
        return I2C_ERROR_DATA;
    }
    
    *data = TWDR;
    return I2C_SUCCESS;
}

// BCD → 십진수 변환
uint8_t bcd_to_decimal(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// 십진수 → BCD 변환
uint8_t decimal_to_bcd(uint8_t decimal) {
    return ((decimal / 10) << 4) | (decimal % 10);
}

// DS1307 RTC 시간 읽기
uint8_t rtc_read_time(rtc_time_t *time) {
    uint8_t status;
    uint8_t data[7];
    
    // 시작 조건
    status = i2c_start();
    if (status != I2C_SUCCESS) return status;
    
    // DS1307 주소 전송 (쓰기)
    status = i2c_write_addr(DS1307_ADDR);
    if (status != I2C_SUCCESS) {
        i2c_stop();
        return status;
    }
    
    // 레지스터 주소 (0x00 - seconds)
    status = i2c_write_data(0x00);
    if (status != I2C_SUCCESS) {
        i2c_stop();
        return status;
    }
    
    // 재시작 조건
    status = i2c_start();
    if (status != I2C_SUCCESS) return status;
    
    // DS1307 주소 전송 (읽기)
    status = i2c_read_addr(DS1307_ADDR);
    if (status != I2C_SUCCESS) {
        i2c_stop();
        return status;
    }
    
    // 7바이트 데이터 읽기
    for (int i = 0; i < 6; i++) {
        status = i2c_read_data_ack(&data[i]);
        if (status != I2C_SUCCESS) {
            i2c_stop();
            return status;
        }
    }
    
    // 마지막 바이트 (NACK)
    status = i2c_read_data_nack(&data[6]);
    if (status != I2C_SUCCESS) {
        i2c_stop();
        return status;
    }
    
    // 정지 조건
    i2c_stop();
    
    // BCD → 십진수 변환
    time->seconds = bcd_to_decimal(data[0] & 0x7F);
    time->minutes = bcd_to_decimal(data[1]);
    time->hours = bcd_to_decimal(data[2] & 0x3F);
    time->day = bcd_to_decimal(data[3]);
    time->date = bcd_to_decimal(data[4]);
    time->month = bcd_to_decimal(data[5]);
    time->year = bcd_to_decimal(data[6]);
    
    return I2C_SUCCESS;
}

// DS1307 RTC 시간 설정
uint8_t rtc_set_time(const rtc_time_t *time) {
    uint8_t status;
    uint8_t data[7];
    
    // 십진수 → BCD 변환
    data[0] = decimal_to_bcd(time->seconds);
    data[1] = decimal_to_bcd(time->minutes);
    data[2] = decimal_to_bcd(time->hours);
    data[3] = decimal_to_bcd(time->day);
    data[4] = decimal_to_bcd(time->date);
    data[5] = decimal_to_bcd(time->month);
    data[6] = decimal_to_bcd(time->year);
    
    // 시작 조건
    status = i2c_start();
    if (status != I2C_SUCCESS) return status;
    
    // DS1307 주소 전송 (쓰기)
    status = i2c_write_addr(DS1307_ADDR);
    if (status != I2C_SUCCESS) {
        i2c_stop();
        return status;
    }
    
    // 레지스터 주소 (0x00)
    status = i2c_write_data(0x00);
    if (status != I2C_SUCCESS) {
        i2c_stop();
        return status;
    }
    
    // 7바이트 데이터 쓰기
    for (int i = 0; i < 7; i++) {
        status = i2c_write_data(data[i]);
        if (status != I2C_SUCCESS) {
            i2c_stop();
            return status;
        }
    }
    
    // 정지 조건
    return i2c_stop();
}

// EEPROM 데이터 쓰기
uint8_t eeprom_write(uint16_t addr, const uint8_t *data, uint8_t length) {
    uint8_t status;
    
    // 시작 조건
    status = i2c_start();
    if (status != I2C_SUCCESS) return status;
    
    // EEPROM 주소 전송
    status = i2c_write_addr(EEPROM_ADDR);
    if (status != I2C_SUCCESS) {
        i2c_stop();
        return status;
    }
    
    // 메모리 주소 (MSB)
    status = i2c_write_data((uint8_t)(addr >> 8));
    if (status != I2C_SUCCESS) {
        i2c_stop();
        return status;
    }
    
    // 메모리 주소 (LSB)
    status = i2c_write_data((uint8_t)(addr & 0xFF));
    if (status != I2C_SUCCESS) {
        i2c_stop();
        return status;
    }
    
    // 데이터 쓰기
    for (uint8_t i = 0; i < length; i++) {
        status = i2c_write_data(data[i]);
        if (status != I2C_SUCCESS) {
            i2c_stop();
            return status;
        }
    }
    
    // 정지 조건
    status = i2c_stop();
    
    // EEPROM 쓰기 완료 대기 (5ms)
    _delay_ms(5);
    
    return status;
}

// EEPROM 데이터 읽기
uint8_t eeprom_read(uint16_t addr, uint8_t *data, uint8_t length) {
    uint8_t status;
    
    // 시작 조건
    status = i2c_start();
    if (status != I2C_SUCCESS) return status;
    
    // EEPROM 주소 전송 (쓰기)
    status = i2c_write_addr(EEPROM_ADDR);
    if (status != I2C_SUCCESS) {
        i2c_stop();
        return status;
    }
    
    // 메모리 주소 설정
    status = i2c_write_data((uint8_t)(addr >> 8));
    if (status != I2C_SUCCESS) {
        i2c_stop();
        return status;
    }
    
    status = i2c_write_data((uint8_t)(addr & 0xFF));
    if (status != I2C_SUCCESS) {
        i2c_stop();
        return status;
    }
    
    // 재시작 조건
    status = i2c_start();
    if (status != I2C_SUCCESS) return status;
    
    // EEPROM 주소 전송 (읽기)
    status = i2c_read_addr(EEPROM_ADDR);
    if (status != I2C_SUCCESS) {
        i2c_stop();
        return status;
    }
    
    // 데이터 읽기
    for (uint8_t i = 0; i < length - 1; i++) {
        status = i2c_read_data_ack(&data[i]);
        if (status != I2C_SUCCESS) {
            i2c_stop();
            return status;
        }
    }
    
    // 마지막 바이트 (NACK)
    status = i2c_read_data_nack(&data[length - 1]);
    if (status != I2C_SUCCESS) {
        i2c_stop();
        return status;
    }
    
    return i2c_stop();
}

// 온도 센서 시뮬레이션 (가상 디바이스)
uint8_t temp_sensor_read(sensor_data_t *sensor) {
    // 실제 센서 대신 시뮬레이션 데이터 생성
    static int16_t temp_base = 250;  // 25.0도
    static uint16_t hum_base = 600;  // 60.0%
    
    // 약간의 변화 시뮬레이션
    temp_base += (rand() % 21) - 10;  // ±1도
    hum_base += (rand() % 21) - 10;   // ±1%
    
    // 범위 제한
    if (temp_base < 0) temp_base = 0;
    if (temp_base > 500) temp_base = 500;
    if (hum_base < 0) hum_base = 0;
    if (hum_base > 1000) hum_base = 1000;
    
    sensor->temperature = temp_base;
    sensor->humidity = hum_base;
    sensor->valid = true;
    
    return I2C_SUCCESS;
}

// 시스템 초기화
void system_init(void) {
    i2c_init();
    
    // RTC 초기 설정 (필요 시)
    rtc_time_t init_time = {0, 0, 12, 1, 1, 1, 24};  // 2024년 1월 1일 12:00:00
    rtc_set_time(&init_time);
    
    // EEPROM 테스트 데이터 저장
    const char test_data[] = "I2C Test Data";
    eeprom_write(0x0000, (const uint8_t*)test_data, sizeof(test_data));
}

int main(void) {
    system_init();
    
    rtc_time_t current_time;
    sensor_data_t sensor_data;
    char eeprom_buffer[32];
    
    while (1) {
        // RTC 시간 읽기
        if (rtc_read_time(&current_time) == I2C_SUCCESS) {
            // 시간 정보 처리 (여기서는 LED로 표시)
            // 실제로는 디스플레이나 시리얼 출력
        }
        
        // 센서 데이터 읽기
        if (temp_sensor_read(&sensor_data) == I2C_SUCCESS) {
            // 센서 데이터를 EEPROM에 로깅
            uint8_t log_data[4];
            log_data[0] = (uint8_t)(sensor_data.temperature >> 8);
            log_data[1] = (uint8_t)(sensor_data.temperature & 0xFF);
            log_data[2] = (uint8_t)(sensor_data.humidity >> 8);
            log_data[3] = (uint8_t)(sensor_data.humidity & 0xFF);
            
            static uint16_t log_addr = 0x0100;  // 로그 시작 주소
            eeprom_write(log_addr, log_data, 4);
            log_addr += 4;
            
            // 주소 순환 (1KB 로그 영역)
            if (log_addr >= 0x0500) {
                log_addr = 0x0100;
            }
        }
        
        // EEPROM 테스트 데이터 읽기
        eeprom_read(0x0000, (uint8_t*)eeprom_buffer, 16);
        
        _delay_ms(1000);  // 1초마다 갱신
    }
    
    return 0;
}