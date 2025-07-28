# 🔴 LED Control - GPIO 제어 기초

## 📋 프로젝트 개요

임베디드 시스템의 가장 기본이 되는 GPIO(General Purpose Input/Output) 제어를 학습하는 프로젝트입니다.

### 학습 목표
- AVR 레지스터 직접 제어 이해
- 비트 연산을 통한 하드웨어 제어
- 타이밍 제어 기초

### 핵심 개념
- Data Direction Register (DDR)
- Port Register (PORT)
- 비트 마스킹 기법

## 🛠️ 하드웨어 구성

### 필요 부품
- ATmega328P (Arduino Uno/Nano)
- LED x 1
- 저항 220Ω x 1
- 점퍼 와이어

### 회로도
```
    ATmega328P
    ┌─────────┐
    │    PB5  ├──[220Ω]──┤▶├── GND
    │ (Pin 13)│          LED
    └─────────┘
```

## 💻 소스 코드

### Version 1: Basic LED Blink
```c
#include <avr/io.h>
#include <util/delay.h>

int main(void) {
    // PB5를 출력으로 설정 (Arduino Pin 13)
    DDRB |= (1 << PB5);
    
    while(1) {
        // LED ON
        PORTB |= (1 << PB5);
        _delay_ms(1000);
        
        // LED OFF
        PORTB &= ~(1 << PB5);
        _delay_ms(1000);
    }
    
    return 0;
}
```

### Version 2: Pattern Blink
```c
#include <avr/io.h>
#include <util/delay.h>

void blink_pattern(uint8_t times, uint16_t on_time, uint16_t off_time) {
    for(uint8_t i = 0; i < times; i++) {
        PORTB |= (1 << PB5);   // LED ON
        _delay_ms(on_time);
        
        PORTB &= ~(1 << PB5);  // LED OFF
        _delay_ms(off_time);
    }
}

int main(void) {
    DDRB |= (1 << PB5);
    
    while(1) {
        // 빠르게 3번 깜빡임
        blink_pattern(3, 100, 100);
        _delay_ms(500);
        
        // 천천히 2번 깜빡임
        blink_pattern(2, 500, 500);
        _delay_ms(1000);
    }
    
    return 0;
}
```

## 📊 기술 분석

### 레지스터 설명
| 레지스터 | 용도 | 설정값 |
|---------|------|--------|
| DDRB | 포트 B 방향 설정 | 1 = 출력, 0 = 입력 |
| PORTB | 포트 B 출력값 | 1 = HIGH, 0 = LOW |
| PINB | 포트 B 입력값 읽기 | 읽기 전용 |

### 비트 연산 설명
```c
DDRB |= (1 << PB5);    // PB5 비트를 1로 설정 (OR 연산)
PORTB |= (1 << PB5);   // PB5를 HIGH로 설정
PORTB &= ~(1 << PB5);  // PB5를 LOW로 설정 (AND + NOT 연산)
PORTB ^= (1 << PB5);   // PB5 토글 (XOR 연산)
```

## 🔍 동작 원리

1. **초기화**: DDRB 레지스터의 5번 비트를 1로 설정하여 PB5를 출력 모드로 변경
2. **LED ON**: PORTB 레지스터의 5번 비트를 1로 설정하여 HIGH 출력
3. **LED OFF**: PORTB 레지스터의 5번 비트를 0으로 설정하여 LOW 출력
4. **반복**: while(1) 무한 루프로 계속 반복

## 📈 성능 분석

- **프로그램 크기**: ~176 bytes
- **RAM 사용량**: ~0 bytes (스택 제외)
- **실행 시간**: 정확한 1초 딜레이 (16MHz 클럭 기준)

## 🚀 심화 학습

### 도전 과제
1. LED 밝기를 점진적으로 변화시키기 (Fade In/Out)
2. 여러 개의 LED를 순차적으로 제어하기
3. 모스 부호 패턴 구현하기

### 다음 단계
- [Button Input](../02-button-input) - 디지털 입력 처리
- [PWM Control](../03-pwm-brightness) - PWM을 이용한 밝기 제어

## 📝 학습 노트

### 배운 점
- AVR의 레지스터 직접 제어 방법
- 비트 연산의 중요성과 활용법
- 하드웨어와 소프트웨어의 연결 방식

### 어려웠던 점
- 처음에는 레지스터 이름과 비트 연산이 생소했음
- 데이터시트를 읽는 방법을 익히는데 시간이 걸림

### 개선 사항
- 타이머를 사용한 더 정확한 딜레이 구현 필요
- 전력 소비 최적화 고려

---
*Last Updated: 2024-01-20*