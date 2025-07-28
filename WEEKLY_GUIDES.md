# 📖 주차별 상세 학습 가이드

<div align="center">

![Weekly Guide](https://img.shields.io/badge/📖-Weekly_Learning_Guides-brightgreen?style=for-the-badge)
![40 Weeks](https://img.shields.io/badge/40_Weeks-Complete_Journey-blue?style=for-the-badge)

**매주 체계적인 학습으로 임베디드 전문가 되기!**

</div>

## 🎓 Week 1: Arduino 개발환경 & 기본 I/O

### 📅 학습 목표
- Arduino IDE 설치 및 환경 설정
- 기본 문법 (setup, loop) 이해
- 디지털 I/O를 이용한 LED 제어
- 시리얼 모니터 활용법 익히기

### 📚 5일 상세 계획

#### 🌅 Day 1 (월요일): 환경 설정의 날
**학습 시간**: 2시간

**오전 (1시간)**: 이론 학습
- [ ] Arduino란? (15분)
- [ ] 마이크로컨트롤러 기초 개념 (15분)
- [ ] Arduino Uno 보드 구조 이해 (15분)
- [ ] 개발 환경 소개 (15분)

**오후 (1시간)**: 실습
- [ ] Arduino IDE 다운로드 및 설치
- [ ] 드라이버 설치 및 보드 연결 확인
- [ ] 예제 코드 "Blink" 업로드 테스트
- [ ] 시리얼 모니터 사용해보기

**📝 오늘의 첫 Arduino 코드**:
```arduino
// 여러분의 첫 Arduino 코드!
void setup() {
  pinMode(13, OUTPUT);
  Serial.begin(9600);
  Serial.println("Hello, Arduino World!");
}

void loop() {
  digitalWrite(13, HIGH);
  delay(1000);
  digitalWrite(13, LOW);
  delay(1000);
}
```

#### 🔥 Day 2 (화요일): LED 깜빡이기 마스터
**학습 시간**: 2시간

**실습 프로젝트**:
1. **프로젝트 1.1**: 500ms 간격으로 깜빡이는 LED
2. **프로젝트 1.2**: 2개의 LED가 번갈아 깜빡이기
3. **프로젝트 1.3**: SOS 신호 만들기

**파일 위치**: `01-fundamentals/arduino-introduction/03-first-program-blink/`

#### ⚡ Day 3 (수요일): 버튼 입력의 세계
**학습 시간**: 2시간

**📝 실습 프로젝트**:
```arduino
// 프로젝트 1.4: 버튼으로 LED 제어
const int buttonPin = 2;
const int ledPin = 13;

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  Serial.begin(9600);
}

void loop() {
  int buttonState = digitalRead(buttonPin);
  
  if (buttonState == LOW) {
    digitalWrite(ledPin, HIGH);
    Serial.println("Button Pressed - LED ON");
  } else {
    digitalWrite(ledPin, LOW);
    Serial.println("Button Released - LED OFF");
  }
  
  delay(50);
}
```

#### 🎯 Day 4 (목요일): 토글 기능 구현
**학습 시간**: 2시간

**고급 프로젝트**: 스마트 토글 LED (디바운싱 포함)

#### 🎉 Day 5 (금요일): 복습 및 창의 프로젝트
**학습 시간**: 2시간

**미니 프로젝트**: 신호등 시스템 만들기
- 빨강 (3초) → 노랑 (1초) → 초록 (3초) 반복
- 버튼을 누르면 보행자 신호로 전환

### 📋 Week 1 체크리스트

#### 🧠 이론 지식
- [ ] Arduino 보드 구조 이해
- [ ] 디지털 I/O 개념 숙지
- [ ] pinMode(), digitalWrite(), digitalRead() 함수 활용
- [ ] 시리얼 통신 기초

#### 🔨 실습 기술
- [ ] Arduino IDE 능숙한 사용
- [ ] 브레드보드 회로 구성
- [ ] LED 제어 프로그래밍
- [ ] 버튼 입력 처리

#### 📁 완성된 프로젝트
- [ ] 기본 LED 깜빡이기
- [ ] 교대로 깜빡이는 LED
- [ ] SOS 신호 LED
- [ ] 버튼 제어 LED
- [ ] 토글 기능 LED
- [ ] 신호등 시스템

---

## 🎨 Week 2: 아날로그 I/O & PWM 제어

### 📅 학습 목표
- 아날로그와 디지털의 차이점 이해
- PWM(Pulse Width Modulation) 원리 학습
- 센서 값 읽기 및 LED 밝기 제어
- RGB LED로 다양한 색상 구현

### 📚 5일 상세 계획

#### 🌈 Day 1 (월요일): 아날로그의 세계
**실습 코드**:
```arduino
void setup() {
  Serial.begin(9600);
}

void loop() {
  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (5.0 / 1023.0);
  
  Serial.print("센서 값: ");
  Serial.print(sensorValue);
  Serial.print(" | 전압: ");
  Serial.print(voltage);
  Serial.println("V");
  
  delay(100);
}
```

#### 💡 Day 2 (화요일): PWM으로 LED 밝기 조절
**프로젝트 2.1**: 스마트 조명 시스템
```arduino
const int potPin = A0;
const int ledPin = 9;

void setup() {
  Serial.begin(9600);
}

void loop() {
  int potValue = analogRead(potPin);
  int brightness = map(potValue, 0, 1023, 0, 255);
  
  analogWrite(ledPin, brightness);
  
  Serial.print("포텐셔미터: ");
  Serial.print(potValue);
  Serial.print(" | LED 밝기: ");
  Serial.println(brightness);
  
  delay(10);
}
```

#### 🌈 Day 3 (수요일): RGB LED 색상 제어
**프로젝트 2.2**: RGB 무드등

#### 📊 Day 4 (목요일): 센서 데이터 처리
**프로젝트 2.3**: 자동 조명 시스템 (이동 평균 필터 포함)

#### 🎪 Day 5 (금요일): 통합 프로젝트
**최종 프로젝트**: 스마트 무드등 시스템
- 3개의 가변저항으로 RGB 색상 개별 제어
- 버튼으로 자동/수동 모드 전환
- 조도 센서에 따른 자동 밝기 조절

---

## 📈 Week 3-40 가이드 (요약)

### Week 3: 센서 활용 마스터
- 온도/습도 센서 (DHT22)
- 초음파 거리 센서 (HC-SR04)
- 다중 센서 데이터 로깅

### Week 4: 시리얼 통신 & 첫 통합 프로젝트
- 시리얼 프로토콜 이해
- 데이터 파싱 및 처리
- **통합 프로젝트**: 전자 온도계

### Week 5-10: Intermediate Phase
- I2C, SPI 통신 프로토콜
- LCD, OLED 디스플레이
- 모터 제어 (서보, DC, 스테퍼)
- 무선 통신 (블루투스, WiFi)
- **통합 프로젝트**: RC카 또는 드론

### Week 11-18: Advanced Phase
- ESP32 IoT 시스템
- 실시간 운영체제 (RTOS)
- 클라우드 연동 (Firebase, AWS)
- 컴퓨터 비전 기초
- **통합 프로젝트**: 스마트 홈 시스템

### Week 19-28: Professional Phase
- 산업용 프로토콜 (Modbus, CAN)
- 자동차 전자 시스템
- 의료기기 개발 표준
- 항공우주 시스템
- **통합 프로젝트**: 상용 제품 프로토타입

### Week 29-40: Expert Phase
- 머신러닝 on Edge
- 신경망 하드웨어 구현
- 양자 컴퓨팅 시뮬레이션
- 뇌-컴퓨터 인터페이스
- **최종 프로젝트**: 혁신적인 개인 프로젝트

---

<div align="center">

**🎯 매주 체계적인 학습으로 전문가가 되어보세요!**

[![Week 1 시작](https://img.shields.io/badge/Week_1-시작하기-brightgreen?style=for-the-badge)](01-fundamentals/arduino-introduction/)
[![진도 체크](https://img.shields.io/badge/진도_체크-확인하기-blue?style=for-the-badge)](PROGRESS_TRACKER.md)

**📚 각 주차별로 상세한 가이드를 따라 학습하세요!**

</div>