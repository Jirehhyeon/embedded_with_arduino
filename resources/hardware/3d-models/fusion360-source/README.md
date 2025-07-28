# 🔧 SmartFan v3.0 Fusion 360 소스 파일

**CAD 소스 파일 및 설계 히스토리 - 커스터마이징과 수정을 위한 완전한 3D 모델**

## 📁 파일 구조

### Fusion 360 프로젝트 파일
```
fusion360-source/
├── SmartFan-v3-Complete.f3d         # 통합 프로젝트 파일 (187MB)
├── Main-Controller-Case.f3d         # 메인 컨트롤러 케이스 (45MB)
├── Sensor-Housing.f3d               # 센서 하우징 (23MB)
├── Fan-Module.f3d                   # 팬 모듈 케이스 (52MB)
├── Assembly-Hardware.f3d            # 조립 하드웨어 (18MB)
└── Parametric-Templates.f3d         # 파라메트릭 템플릿 (31MB)
```

### 보조 파일
```
fusion360-source/
├── parameters/
│   ├── design-parameters.txt        # 주요 설계 변수
│   ├── material-properties.json     # 재료 물성값
│   └── tolerance-table.xlsx         # 공차 테이블
├── sketches/
│   ├── base-sketches.dxf           # 기본 스케치 (DXF)
│   └── reference-drawings.pdf       # 참조 도면
└── simulation/
    ├── stress-analysis.f3s          # 응력 해석 결과
    ├── thermal-analysis.f3s         # 열해석 결과
    └── vibration-analysis.f3s       # 진동 해석 결과
```

## 🎯 설계 의도 및 컨셉

### 핵심 설계 원칙
1. **모듈형 구조**: 독립적인 조립/분해 가능
2. **파라메트릭 모델링**: 치수 변경으로 쉬운 커스터마이징
3. **DFM 최적화**: 3D 프린팅에 최적화된 형상
4. **확장성**: 향후 센서/기능 추가 고려

### 주요 설계 변수 (Parameters)

**전체 시스템**:
```
Case_Length = 120mm          # 케이스 전체 길이
Case_Width = 80mm            # 케이스 전체 폭  
Case_Height = 65mm           # 케이스 전체 높이
Wall_Thickness = 2.0mm       # 벽 두께
Fillet_Radius = 3.0mm        # 모서리 라운딩
```

**Arduino 관련**:
```
Arduino_Length = 68.6mm      # Arduino Uno 길이
Arduino_Width = 53.4mm       # Arduino Uno 폭
Arduino_Height = 15mm        # PCB + 부품 높이
Mounting_Hole_Dia = 3.2mm    # 마운팅 홀 지름
Standoff_Height = 8mm        # 스탠드오프 높이
```

**팬 모듈**:
```
Fan_Size = 120mm             # 팬 크기 (정사각형)
Fan_Thickness = 25mm         # 팬 두께
Mounting_Hole_Spacing = 105mm # 마운팅 홀 간격
Airflow_Direction = 1        # 1=흡입, -1=배출
```

**센서 모듈**:
```
DHT22_Width = 15.5mm         # DHT22 센서 폭
DHT22_Depth = 12mm           # DHT22 센서 깊이
DHT22_Height = 5.8mm         # DHT22 센서 높이
ACS712_Length = 31mm         # ACS712 센서 길이
ACS712_Width = 13mm          # ACS712 센서 폭
```

## 🔧 Fusion 360 작업 환경 설정

### 권장 시스템 사양
- **CPU**: Intel i5-8400 이상 / AMD Ryzen 5 2600 이상
- **RAM**: 16GB 이상 (32GB 권장)
- **GPU**: GTX 1060 이상 / RX 580 이상
- **저장공간**: SSD 500GB 이상

### Fusion 360 설정

**단위 설정**:
```
File → Preferences → Units
- Default units: Millimeter
- Angular units: Degree  
- Time: Second
- Mass: Kilogram
```

**성능 최적화**:
```
Graphics Settings:
- Effects: Medium
- Anti-aliasing: 4x MSAA
- Realtime graphics: High
- Timeline detail: Medium
```

**백업 설정**:
```
Data Management:
- Auto-save: Every 5 minutes
- Version retention: 10 versions
- Export location: Local + Cloud
```

## 📐 설계 히스토리 및 Feature Tree

### Main-Controller-Case.f3d 구조

**기본 형상 (Base Features)**:
1. **Base Sketch**: 외형 기본 스케치 (120×80mm)
2. **Main Extrude**: 기본 박스 형상 (높이 25mm)
3. **Wall Shell**: 쉘 피처로 벽 두께 생성 (2mm)
4. **Corner Fillets**: 모서리 라운딩 (R3)

**기능 형상 (Functional Features)**:
5. **Arduino Mount Sketch**: Arduino 마운팅 위치 스케치
6. **Mounting Posts**: 스탠드오프 포스트 (높이 8mm)
7. **ESP32 Slot**: ESP32 장착 슬롯
8. **Cable Channels**: 케이블 배선 채널

**통풍 시스템 (Ventilation System)**:
9. **Side Vents Pattern**: 측면 통풍구 패턴
10. **Top Grille**: 상단 격자 통풍구
11. **Bottom Air Intake**: 하단 흡기구

**하드웨어 (Hardware Features)**:
12. **Screw Bosses**: 나사 결합부
13. **Snap Clips**: 스냅핏 클립
14. **Rubber Feet Recess**: 고무발 홈

### 파라메트릭 관계 (Parametric Relationships)

**부모-자식 관계**:
```
Base Sketch → Main Extrude → Wall Shell
    ↓
Arduino Mount Sketch → Mounting Posts
    ↓
Side Vents Pattern → Array Pattern
    ↓
Snap Clips → Mirror Feature
```

**제약 조건 (Constraints)**:
- Arduino 마운팅 홀: 표준 Arduino 치수에 구속
- 통풍구 크기: 최소 공기 흐름량 확보
- 벽 두께: 3D 프린팅 최소 두께 (1.6mm) 이상

## 🔍 설계 검증 및 시뮬레이션

### 구조 해석 (Structural Analysis)

**해석 조건**:
```
Material: PLA+ (Density: 1.25g/cm³)
Elastic Modulus: 3.5 GPa
Tensile Strength: 50 MPa
Operating Temperature: 20-60°C
```

**하중 조건**:
- 자중: 1G 하향
- 팬 진동: 0.5G @ 85Hz
- 사용자 하중: 10N (버튼 누름)
- 열팽창: 40°C 온도 상승

**해석 결과**:
- 최대 응력: 12.3 MPa (허용치의 25%)
- 최대 변위: 0.8mm (팬 모듈 모서리)
- 안전계수: 4.1 (충분한 안전도)

### 열해석 (Thermal Analysis)

**경계 조건**:
```
외부 온도: 25°C
내부 발열원:
- Arduino: 2W
- ESP32: 1.5W  
- 모터 드라이버: 3W
- 팬 모터: 12W (부하 시)
```

**해석 결과**:
- 최고 온도: 58°C (모터 드라이버 부근)
- 평균 온도: 35°C
- 온도 구배: 33°C (충분한 자연 대류)

### 진동 해석 (Vibration Analysis)

**고유진동수 분석**:
- 1차 모드: 85Hz (팬 회전수와 분리)
- 2차 모드: 156Hz
- 3차 모드: 234Hz

**강제진동 응답**:
- 팬 진동 주파수: 25-85Hz
- 최대 진폭: 0.12mm @ 65Hz
- 공진 회피: 안전 마진 20Hz 확보

## 🎨 커스터마이징 가이드

### 크기 변경 (Scaling)

**전체 크기 조정**:
1. Parameters 메뉴에서 주요 치수 수정
2. Update 실행하여 모델 재생성
3. Feature 에러 확인 및 수정
4. STL 재생성 및 확인

**지원 크기 범위**:
- 길이: 100-150mm (±25%)
- 폭: 60-100mm (±25%) 
- 높이: 50-80mm (±25%)

### 새로운 센서 추가

**센서 슬롯 추가 절차**:
1. **새 스케치**: 센서 형상 스케치 생성
2. **Pocket 피처**: 센서 장착 공간 생성
3. **Mounting Features**: 고정 방법 추가
4. **케이블 루팅**: 배선 경로 확보

**예시: PM2.5 센서 추가**:
```python
# 새 파라미터 추가
PM25_Width = 50mm
PM25_Height = 38mm  
PM25_Depth = 23mm

# 스케치 구속조건
- 센서 중심을 케이스 측면에 위치
- 공기 흡입구가 외부로 향하도록
- 최소 5mm 클리어런스 확보
```

### 재료 최적화

**PLA용 설계 수정**:
- 벽 두께: 2.0mm → 2.4mm
- 인필 밀도: 20% → 25%
- 서포트 각도: 45° → 50°

**PETG용 설계 수정**:
- 수축 보상: +0.2%
- 브리징 거리: 최대 40mm
- 오버행 각도: 최대 55°

**ABS용 설계 수정**:
- 수축 보상: +0.8%
- 뒤틀림 방지: 베이스 플레이트 확대
- 벽 두께: 최소 1.8mm

## 📊 품질 관리 및 검증

### 설계 검증 체크리스트

**형상 검증**:
- [ ] 최소 벽 두께 1.6mm 이상
- [ ] 오버행 각도 45° 이하
- [ ] 브리징 거리 30mm 이하
- [ ] 서포트 최소화 설계

**기능 검증**:
- [ ] 모든 부품 간섭 없음
- [ ] 조립/분해 가능성 확인
- [ ] 케이블 배선 공간 충분
- [ ] 통풍 효율성 확보

**제조 검증**:
- [ ] 3D 프린팅 오리엔테이션 최적
- [ ] 서포트 제거 용이성
- [ ] 후처리 접근성 확보
- [ ] 공차 범위 내 설계

### 버전 관리 (Version Control)

**명명 규칙**:
```
SmartFan-v3.1.2-YYYYMMDD
         │ │ │     │
         │ │ │     └── 날짜
         │ │ └────── 패치 (버그 수정)
         │ └──────── 마이너 (기능 추가)  
         └────────── 메이저 (대규모 변경)
```

**변경 로그 예시**:
```
v3.1.2 (2024-01-15):
- 팬 마운팅 홀 공차 조정 (+0.1mm)
- LCD 창 모서리 라운딩 추가
- 케이블 채널 폭 확대 (8mm → 10mm)

v3.1.1 (2024-01-10):
- DHT22 슬롯 깊이 수정
- 스냅핏 클립 강도 개선
- 통풍구 패턴 최적화

v3.1.0 (2024-01-05):
- 센서 하우징 모듈 분리
- PM2.5 센서 슬롯 추가
- RGB LED 확산판 지원
```

## 🔄 협업 및 공유

### 팀 협업 설정

**Fusion 360 Team 설정**:
1. **프로젝트 생성**: SmartFan-v3-Development
2. **권한 설정**: Editor, Viewer, Commenter
3. **브랜치 전략**: main, feature, hotfix
4. **병합 정책**: Pull Request 기반

**파일 공유 프로토콜**:
- **설계 변경**: 별도 브랜치에서 작업
- **검토 과정**: 팀 리뷰 후 병합
- **릴리즈**: 태그를 통한 버전 관리
- **백업**: 주간 로컬 백업 수행

### 오픈소스 기여

**기여 가이드라인**:
1. **이슈 등록**: 개선사항 또는 버그 신고
2. **포크 & 브랜치**: 개인 저장소에서 작업
3. **Pull Request**: 상세한 변경 내용 설명
4. **코드 리뷰**: 커뮤니티 검토 과정
5. **병합**: 승인 후 메인 브랜치 병합

**표준 포맷**:
```markdown
## 변경 사항
- 기능 추가/수정/삭제 내용

## 테스트 결과  
- 3D 프린팅 테스트 완료
- 조립 테스트 완료
- 기능 테스트 완료

## 스크린샷
- 변경 전/후 비교 이미지
```

## 🛠️ 고급 활용법

### 생성형 설계 (Generative Design)

**목표 설정**:
- 중량 최소화 (현재 대비 30% 감량)
- 강성 유지 (현재 수준)
- 제조 제약 조건 (3D 프린팅)

**설계 공간**:
- 유지 영역: 마운팅 홀, 인터페이스
- 장애물 영역: 전자 부품 공간
- 목표 영역: 구조 최적화 대상

**결과 활용**:
- 유기적 형상 적용
- 격자 구조 도입
- 토폴로지 최적화 반영

### 시뮬레이션 최적화

**CFD 해석 (Computational Fluid Dynamics)**:
```
유체: 공기 (20°C, 1기압)
경계조건:
- 팬 출구: 120CFM
- 케이스 입구: 대기압
- 벽면: No-slip 조건
```

**최적화 목표**:
- 압력 손실 최소화
- 온도 분포 균일화
- 소음 수준 감소

### 제조 비용 최적화

**재료 사용량 분석**:
- 현재 설계: 193g 필라멘트
- 최적화 목표: 150g (-22%)
- 비용 절감: 필라멘트당 약 800원

**최적화 방법**:
1. **벽 두께 최적화**: 응력 집중 부위만 보강
2. **인필 최적화**: 자이로이드 패턴 적용
3. **서포트 제거**: 설계 변경으로 서포트 불필요
4. **재료 효율**: 중첩 출력으로 시간 단축

---

**🔧 전문적인 CAD 설계로 무한한 가능성을! 🔧**

Fusion 360 소스 파일을 활용하여 자유롭게 커스터마이징하고 개선해보세요! 🚀