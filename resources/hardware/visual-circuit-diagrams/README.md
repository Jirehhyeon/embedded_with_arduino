# 🔌 SmartFan v3.0 시각적 회로도 모음

**쉽게 따라할 수 있는 시각적 회로도 및 연결 가이드**

## 📋 개요

이 폴더에는 SmartFan v3.0 프로젝트의 모든 회로 연결을 시각적으로 보여주는 다양한 형태의 회로도가 포함되어 있습니다:

- **브레드보드 레이아웃**: 실제 조립 시 따라할 수 있는 시각적 가이드
- **회로도**: 전자공학적 연결 관계를 보여주는 표준 회로도
- **배선도**: 실제 와이어 연결을 색상별로 구분한 다이어그램
- **3D 모델**: 입체적으로 확인할 수 있는 3차원 회로 모델

## 🎯 대상 사용자

### 초보자용 (Visual Learner)
- 브레드보드 배치도
- 컬러 배선 가이드
- 단계별 조립 순서
- 실물 크기 템플릿

### 중급자용 (Technical Builder)
- 표준 회로도
- PCB 레이아웃 가이드
- 신호 흐름도
- 테스트 포인트 표시

### 고급자용 (Engineer)
- 상세 회로 분석
- 임피던스 매칭
- EMC 고려사항
- 최적화 방안

## 📂 파일 구조

```
visual-circuit-diagrams/
├── breadboard-layouts/              # 브레드보드 배치도
│   ├── bb-complete-system.png       # 전체 시스템 브레드보드
│   ├── bb-step1-arduino.png         # 1단계: Arduino 연결
│   ├── bb-step2-esp32.png           # 2단계: ESP32 추가
│   ├── bb-step3-sensors.png         # 3단계: 센서 연결
│   ├── bb-step4-motors.png          # 4단계: 모터 연결
│   └── bb-step5-complete.png        # 5단계: 전체 완성
├── schematic-diagrams/              # 표준 회로도
│   ├── schematic-main.svg           # 메인 회로도 (벡터)
│   ├── schematic-power.svg          # 전원부 회로도
│   ├── schematic-sensors.svg        # 센서부 회로도
│   ├── schematic-control.svg        # 제어부 회로도
│   └── schematic-safety.svg         # 안전 시스템 회로도
├── wiring-diagrams/                 # 배선도
│   ├── wiring-overview.png          # 전체 배선 개요
│   ├── wiring-power-red.png         # 전원선 (빨간색)
│   ├── wiring-ground-black.png      # 접지선 (검은색)
│   ├── wiring-signal-multi.png      # 신호선 (다색)
│   └── wiring-checklist.pdf         # 배선 체크리스트
├── 3d-models/                       # 3D 회로 모델
│   ├── 3d-breadboard-view.obj       # 3D 브레드보드 뷰
│   ├── 3d-pcb-layout.step          # PCB 3D 모델
│   └── 3d-assembly-guide.mp4       # 3D 조립 애니메이션
├── fritzing-files/                  # Fritzing 소스 파일
│   ├── SmartFan-v3.fzz             # 메인 Fritzing 파일
│   ├── custom-parts/                # 커스텀 부품 라이브러리
│   └── exports/                     # 내보내기 파일들
└── reference-images/                # 참조 이미지
    ├── real-breadboard-photos/      # 실제 브레드보드 사진
    ├── component-pinouts/           # 부품 핀아웃 다이어그램
    └── troubleshooting-images/      # 문제 해결 이미지
```

## 🎨 시각적 회로도 유형별 설명

### 1. 브레드보드 레이아웃 (Breadboard Layout)

**파일**: `bb-complete-system.png`  
**크기**: 1920×1080px (Full HD)  
**포맷**: PNG (고화질)

**특징**:
- 실제 브레드보드와 동일한 레이아웃
- 부품 실물 크기로 표현
- 색상별 점퍼 와이어 구분
- 행/열 번호 표시로 정확한 위치 안내

**사용법**:
1. 화면에 이미지를 띄워놓고 참조
2. 태블릿으로 확인하며 조립
3. 인쇄하여 책상 옆에 비치
4. 각 단계별로 체크하며 진행

### 2. 표준 회로도 (Schematic Diagram)

**파일**: `schematic-main.svg`  
**크기**: 벡터 (무한 확대 가능)  
**포맷**: SVG (웹 호환)

**특징**:
- IEEE 표준 기호 사용
- 신호 흐름 방향 표시
- 전압/전류 값 표기
- 테스트 포인트 명시

**구성 요소**:
- 전원 공급 회로
- 마이크로컨트롤러 연결
- 센서 인터페이스
- 모터 드라이브 회로
- 안전 보호 회로

### 3. 배선도 (Wiring Diagram)

**파일**: `wiring-overview.png`  
**크기**: 2048×1536px (4K)  
**포맷**: PNG (레이어별 분리 가능)

**색상 코드**:
- 🔴 **빨간색**: +5V, +12V 전원선
- ⚫ **검은색**: GND 접지선
- 🟡 **노란색**: 디지털 신호선
- 🟢 **초록색**: 아날로그 신호선
- 🔵 **파란색**: I2C 통신선 (SDA/SCL)
- 🟠 **주황색**: PWM 출력선
- 🟣 **보라색**: 시리얼 통신선

### 4. 3D 모델 (3D Circuit Model)

**파일**: `3d-breadboard-view.obj`  
**크기**: 약 15MB  
**포맷**: OBJ (범용 3D 형식)

**지원 뷰어**:
- Windows 10/11 3D Viewer
- Blender (무료)
- SketchFab (웹)
- Fusion 360 Viewer

**활용법**:
- 다양한 각도에서 회로 확인
- 부품 배치 공간 검토
- 케이블 라우팅 계획
- 교육용 시각 자료

## 🔧 Fritzing 프로젝트 파일

### SmartFan-v3.fzz 구성

**브레드보드 뷰**:
- 1:1 실물 크기 레이아웃
- 정확한 핀 매핑
- 색상별 와이어 구분
- 부품 라벨 표시

**회로도 뷰**:
- 표준 전자 기호
- 논리적 연결 관계
- 신호 흐름 표시
- 전압/전류 레벨 표기

**PCB 뷰**:
- 실제 PCB 제작용 레이아웃
- 자동 라우팅 결과
- 비아 및 트레이스 표시
- 부품 배치 최적화

### 커스텀 부품 라이브러리

**추가된 부품**:
```
custom-parts/
├── ESP32-DevKit-v1.fzpz           # ESP32 개발보드
├── HW-020-Motor-Driver.fzpz       # HW-020 모터 드라이버
├── ACS712-Current-Sensor.fzpz     # ACS712 전류 센서
├── DHT22-Temp-Humidity.fzpz       # DHT22 온습도 센서
└── 120mm-PC-Fan.fzpz              # 120mm PC 팬
```

**설치 방법**:
1. Fritzing 실행
2. File → Import → Part Library
3. 해당 .fzpz 파일 선택
4. My Parts 폴더에서 확인

## 📏 실물 크기 템플릿

### 브레드보드 템플릿

**인쇄 설정**:
- 용지: A3 (297×420mm)
- 해상도: 300 DPI
- 여백: 10mm
- 비율: 100% (실물 크기)

**사용법**:
1. `bb-template-a3.pdf` 인쇄
2. 브레드보드를 템플릿 위에 올리기
3. 부품 위치를 템플릿으로 확인
4. 배선 후 체크리스트로 검증

### 부품 배치 가이드

**Arduino Uno 위치**:
- X축: 30mm (좌측 기준)
- Y축: 50mm (상단 기준)
- 방향: USB 포트가 좌측

**ESP32 위치**:
- X축: 150mm (Arduino 우측)
- Y축: 80mm (Arduino와 수평)
- 방향: USB 포트가 하단

**센서 배치**:
- DHT22: 브레드보드 상단 중앙
- ACS712: 전원선과 직렬 연결
- LDR: 브레드보드 우측 상단

## 🎯 단계별 조립 가이드

### Step 1: 전원부 구성
![Step 1](bb-step1-arduino.png)

**소요시간**: 15분  
**난이도**: ⭐  
**준비물**: Arduino, 점퍼 와이어 (빨강/검정)

**작업 내용**:
1. Arduino를 브레드보드 좌측에 배치
2. 5V 핀을 빨간색 전원 레일에 연결
3. GND 핀을 파란색 접지 레일에 연결
4. 전원 LED 확인 (빨간색 불빛)

### Step 2: ESP32 추가
![Step 2](bb-step2-esp32.png)

**소요시간**: 10분  
**난이도**: ⭐⭐  
**준비물**: ESP32, 점퍼 와이어 (노랑/초록)

**작업 내용**:
1. ESP32를 Arduino 우측에 배치
2. 3V3과 GND를 전원 레일에 연결
3. TX2(GPIO17) → Arduino D2 (시리얼 통신)
4. RX2(GPIO16) → Arduino D3 (시리얼 통신)

### Step 3: 센서 연결
![Step 3](bb-step3-sensors.png)

**소요시간**: 20분  
**난이도**: ⭐⭐⭐  
**준비물**: DHT22, ACS712, LDR, 저항

**DHT22 연결**:
- VCC → 5V (빨간색)
- GND → GND (검은색)
- DATA → Arduino D7 (노란색)

**ACS712 연결**:
- VCC → 5V (빨간색)
- GND → GND (검은색)
- OUT → Arduino A1 (초록색)

**LDR 연결**:
- 한쪽 → 5V (빨간색)
- 다른쪽 → 10kΩ 저항 → GND
- 중간점 → Arduino A2 (초록색)

### Step 4: 모터 드라이버
![Step 4](bb-step4-motors.png)

**소요시간**: 25분  
**난이도**: ⭐⭐⭐⭐  
**준비물**: HW-020, 서보모터, 12V 팬

**HW-020 연결**:
- VMS → 12V 외부 전원 (빨간색)
- VSS → 5V Arduino (주황색)
- GND → 공통 GND (검은색)
- IN1 → Arduino D12 (파란색)
- ENA → Arduino D3 (보라색)

**서보 연결**:
- 빨간선 → 5V (빨간색)
- 갈색선 → GND (검은색)
- 주황선 → Arduino D11 (주황색)

### Step 5: 최종 완성
![Step 5](bb-step5-complete.png)

**소요시간**: 15분  
**난이도**: ⭐⭐  
**준비물**: LCD, 최종 점검 도구

**I2C LCD 연결**:
- VCC → 5V (빨간색)
- GND → GND (검은색)
- SDA → Arduino A4 (파란색)
- SCL → Arduino A5 (초록색)

**최종 점검**:
- [ ] 모든 전원 연결 확인
- [ ] 단선/합선 없음 확인
- [ ] 부품 방향 올바름 확인
- [ ] 케이블 정리 완료

## 🔍 문제 해결 시각 가이드

### 자주 발생하는 연결 오류

**문제 1: Arduino 인식 안됨**
![Troubleshoot 1](troubleshooting-arduino-connection.png)

**체크 포인트**:
- USB 케이블 연결 확인
- 드라이버 설치 상태
- 포트 선택 확인 (Tools → Port)
- 보드 선택 확인 (Arduino Uno)

**문제 2: 센서 데이터 이상**
![Troubleshoot 2](troubleshooting-sensor-data.png)

**체크 포인트**:
- 센서 전원 연결 (VCC, GND)
- 데이터 핀 연결 확인
- 풀업 저항 필요 여부
- 라이브러리 설치 확인

**문제 3: 모터 동작 안됨**
![Troubleshoot 3](troubleshooting-motor-control.png)

**체크 포인트**:
- 외부 전원 연결 (12V)
- 모터 드라이버 연결
- PWM 신호 확인
- 모터 부하 점검

## 📱 모바일 친화적 가이드

### QR 코드 링크
각 단계별 이미지에는 QR 코드가 포함되어 있어 스마트폰으로 스캔하면 해당 단계의 상세 가이드를 확인할 수 있습니다.

**QR 코드 활용법**:
1. 스마트폰 카메라로 QR 코드 스캔
2. 브라우저에서 상세 가이드 페이지 열기
3. 확대/축소하며 세부 연결 확인
4. 오프라인 저장으로 인터넷 없이도 사용

### 증강현실 (AR) 지원
**준비 중인 기능**:
- 스마트폰 카메라로 브레드보드 인식
- 실시간 부품 위치 안내
- 연결 순서 단계별 표시
- 오류 부분 자동 감지 및 수정 안내

## 📊 시각적 통계 및 분석

### 부품별 연결 복잡도
```
Arduino Uno: ████████████████████ 20개 연결
ESP32:      ████████████ 12개 연결  
DHT22:      ███ 3개 연결
ACS712:     ███ 3개 연결
HW-020:     █████████ 9개 연결
Servo:      ███ 3개 연결
LCD:        ████ 4개 연결
```

### 난이도별 소요 시간
```
초보자:    ████████████████████ 120분
중급자:    ████████████ 60분
고급자:    ████████ 40분
전문가:    ████ 20분
```

---

**🔌 시각적 회로도로 완벽한 이해를! 🔌**

다양한 형태의 시각 자료를 통해 누구나 쉽게 SmartFan v3.0을 조립할 수 있습니다! 🚀