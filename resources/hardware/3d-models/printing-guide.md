# 🖨️ SmartFan v3.0 3D 프린팅 완벽 가이드

**초보자부터 전문가까지 - 성공적인 3D 프린팅을 위한 완전한 가이드**

## 🎯 프린팅 준비 단계

### 1. 프린터 사전 점검

**필수 점검 항목**:
- [ ] 베드 레벨링 상태 확인
- [ ] 노즐 막힘 없음 확인  
- [ ] 필라멘트 잔량 충분 (최소 200g)
- [ ] SD카드/USB 연결 상태
- [ ] 실내 온도 18-25°C 유지

**베드 레벨링 가이드**:
```
1. 베드 온도를 60°C로 예열
2. 노즐을 베드에서 0.1mm 떨어뜨리기
3. 4모서리와 중앙 총 5지점 확인
4. A4 용지가 약간의 저항과 함께 들어가는 정도
5. 자동 베드 레벨링 센서 재조정
```

### 2. 필라멘트 선택 가이드

| 재료 | 난이도 | 온도 | 장점 | 단점 | 권장용도 |
|------|---------|------|------|------|----------|
| **PLA** | ⭐ | 190-220°C | 쉬운 출력, 무취 | 낮은 내열성 | 프로토타입, 교육용 |
| **PLA+** | ⭐⭐ | 210-230°C | 향상된 강도 | 약간 높은 온도 | **권장 - 메인 케이스** |
| **PETG** | ⭐⭐⭐ | 220-250°C | 투명성, 강도 | 스트링 현상 | 투명 윈도우, 커버 |
| **ABS** | ⭐⭐⭐⭐ | 230-260°C | 고강도, 내열 | 냄새, 뒤틀림 | 고온 환경 부품 |

**권장 브랜드**:
- **입문용**: 코팩필라 PLA+ (국산, 안정적)
- **중급용**: eSUN PLA+ (가성비 우수)
- **고급용**: Prusament PETG (최고품질)

### 3. 슬라이서 설정

#### Cura 4.8+ 권장 설정

**기본 설정 (PLA+)**:
```
Layer Height: 0.2mm
Line Width: 0.4mm
Wall Thickness: 1.2mm (3 perimeters)
Top/Bottom Thickness: 0.8mm (4 layers)
Infill Density: 20%
Infill Pattern: Grid
```

**온도 설정**:
```
Printing Temperature: 215°C
Build Plate Temperature: 60°C
Initial Layer Temperature: 220°C
Initial Layer Bed Temperature: 65°C
```

**속도 설정**:
```
Print Speed: 50mm/s
Infill Speed: 60mm/s
Wall Speed: 30mm/s
Top/Bottom Speed: 25mm/s
Initial Layer Speed: 20mm/s
```

**서포트 설정**:
```
Support Structure: Tree
Support Overhang Angle: 45°
Support Density: 15%
Support Z Distance: 0.3mm
Support X/Y Distance: 0.8mm
```

#### PrusaSlicer 2.4+ 설정

**Quality 탭**:
```
Layer height: 0.2mm
First layer height: 0.25mm
Perimeters: 3
Top solid layers: 4
Bottom solid layers: 4
```

**Infill 탭**:
```
Fill density: 20%
Fill pattern: Grid
Top/bottom fill pattern: Rectilinear
```

**Support 탭**:
```
Generate support material: ✓
Support on build plate only: ✓
Pattern: Organic
Threshold: 45°
```

## 📋 파트별 출력 가이드

### 1단계: main-controller-bottom.stl

**출력 설정**:
- 오리엔테이션: 바닥면이 베드에 평평하게
- 서포트: 불필요
- 브림: 5mm (뒤틀림 방지)
- 예상 시간: 3시간 30분

**주의사항**:
- Arduino 마운팅 홀 정확도가 중요
- 첫 레이어 접착력 확인 필수
- 베드 중앙에 배치 권장

**실시간 모니터링**:
- 10분: 첫 레이어 접착 상태 확인
- 1시간: 벽면 품질 확인
- 2시간: 인필 상태 점검

### 2단계: main-controller-top.stl

**출력 설정**:
- 오리엔테이션: LCD 창이 위쪽
- 서포트: LCD 창 부분만 필요
- 브림: 3mm
- 예상 시간: 2시간 45분

**서포트 최적화**:
```
LCD 창 부분:
- Support Type: Tree
- Density: 15%
- Z Distance: 0.2mm
```

**품질 체크포인트**:
- LCD 창 모서리 정확도
- 통풍구 막힘 없음
- 클립 홈 치수 정확성

### 3단계: sensor-housing.stl

**출력 설정**:
- 오리엔테이션: 센서 슬롯이 위쪽
- 서포트: 불필요
- 래프트: 사용 안함
- 예상 시간: 1시간 30분

**정밀도 확인**:
- DHT22 슬롯: 15.5×12.0×5.8mm
- ACS712 슬롯: 31×13×15mm
- LDR 창: Ø5.2mm

### 4단계: fan-case-base.stl

**출력 설정**:
- 오리엔테이션: 팬 마운팅면이 위쪽
- 서포트: 최소 (트리 서포트)
- 브림: 8mm (대형 파트)
- 예상 시간: 4시간 15분

**대형 파트 주의사항**:
- 베드 사이즈 220×220mm 이상 필요
- 인클로저 사용 권장 (뒤틀림 방지)
- 팬 마운팅 홀 정확도 중요

**출력 중 점검**:
- 30분: 첫 레이어 전체 접착 확인
- 2시간: 벽면 직각도 확인
- 3시간: 팬 홀 형태 확인

### 5단계: fan-protective-grille.stl

**출력 설정**:
- 오리엔테이션: 평평한 면이 베드
- 서포트: 불필요 (브리징 최적화)
- 속도: 40mm/s (정밀도 우선)
- 예상 시간: 2시간 30분

**브리징 최적화**:
- 냉각팬 100% 가동
- 브리징 속도 25mm/s
- 레이어 높이 0.15mm (선택사항)

### 6단계: assembly-clips.stl

**출력 설정**:
- 오리엔테이션: 클립 길이 방향으로 배치
- 8개 동시 출력
- 서포트: 불필요
- 예상 시간: 45분

**소형 파트 팁**:
- 베드 접착제 사용 권장
- 출력 속도 35mm/s
- 스커트 3라인으로 설정

## 🔧 출력 중 문제 해결

### 일반적인 문제와 해결법

**문제 1: 첫 레이어 접착 불량**

증상: 필라멘트가 베드에서 떨어짐
```
해결책:
1. 베드 레벨링 재조정
2. 베드 온도 +5°C 증가
3. 첫 레이어 속도 15mm/s로 감소
4. 브림 폭 5mm로 증가
5. 베드 접착제 사용 (글루스틱)
```

**문제 2: 스트링/오징**

증상: 파트 사이에 실 같은 선
```
해결책:
1. 리트랙션 거리 6mm로 증가
2. 리트랙션 속도 40mm/s로 증가
3. 이동 속도 150mm/s로 증가
4. 온도 -5°C 감소
5. 코스팅 활성화
```

**문제 3: 레이어 분리**

증상: 레이어 사이가 갈라짐
```
해결책:
1. 노즐 온도 +10°C 증가
2. 베드 온도 +5°C 증가
3. 냉각팬 50%로 감소
4. 출력 속도 감소
5. 실내 온도 확인 (20°C 이상)
```

**문제 4: 서포트 제거 어려움**

증상: 서포트가 파트에 달라붙음
```
해결책:
1. Z Distance 0.3mm로 증가
2. 서포트 밀도 10%로 감소
3. Interface layer 활성화
4. 서포트 패턴을 Zigzag로 변경
5. 제거 도구 사용 (플라이어, 니퍼)
```

### 고급 문제 해결

**베드 접착력 향상법**:
1. **유리 베드**: 글루스틱 얇게 도포
2. **PEI 시트**: 이소프로필알콜로 청소
3. **BuildTak**: 수명 확인 후 교체
4. **가열베드**: 온도 균일성 점검

**정밀도 향상법**:
1. **벨트 장력**: 적절한 장력 유지
2. **베어링 윤활**: 주기적 그리스 보충
3. **프레임 강성**: 볼트 조임 확인
4. **진동 차단**: 방진 패드 사용

## 📊 품질 관리 체크리스트

### 출력 전 점검

**프린터 상태**:
- [ ] 베드 레벨링 OK
- [ ] 노즐 청소 완료
- [ ] 필라멘트 로딩 정상
- [ ] 벨트 장력 적절
- [ ] 온도 센서 정상

**파일 준비**:
- [ ] STL 파일 무결성 확인
- [ ] 슬라이서 설정 검토
- [ ] G-code 미리보기 확인
- [ ] 예상 시간/재료량 확인

### 출력 중 모니터링

**1시간차 점검**:
- [ ] 첫 레이어 완전 접착
- [ ] 벽면 품질 양호
- [ ] 온도 안정성 확인
- [ ] 소음 수준 정상

**중간 점검 (50% 진행)**:
- [ ] 레이어 접착 상태
- [ ] 치수 정확도 확인
- [ ] 서포트 품질 점검
- [ ] 필라멘트 잔량 확인

### 출력 후 검증

**육안 검사**:
- [ ] 표면 조도 양호
- [ ] 레이어 라인 균일
- [ ] 색상 일관성 유지
- [ ] 결함 부위 없음

**치수 검증**:
- [ ] 주요 치수 ±0.1mm 내
- [ ] 홀 지름 정확도
- [ ] 나사산 품질 확인
- [ ] 결합부 간섭 없음

## 🎨 후처리 및 마감

### 필수 후처리

**1. 서포트 제거**:
```
도구: 니퍼, 사이드 커터, 파일
순서:
1. 큰 서포트부터 제거
2. 잔여물 니퍼로 정리
3. 표면 파일로 정리
4. 세밀한 부분 샌드페이퍼
```

**2. 홀 정리**:
```
도구: 3mm, 2.5mm 드릴비트, 핸드 드릴
주의사항:
- 천천히 회전하며 가공
- 칩 제거하며 진행
- 정확한 수직도 유지
- 버 제거 필수
```

**3. 표면 정리**:
```
사포 순서: 220방 → 400방 → 600방
부위별 처리:
- 평면: 사포 블록 사용
- 곡면: 손으로 부드럽게
- 모서리: 라운딩 처리
- 홈: 줄 사용
```

### 선택적 후처리

**1. 도색 (아크릴 페인트)**:
```
준비: 탈지 → 프라이머 → 베이스 → 클리어
기법:
- 스프레이: 균일한 도포
- 붓: 세밀한 부분
- 에어브러시: 고품질 마감
- 스텐실: 패턴/로고
```

**2. 강도 보강**:
```
방법:
- 에폭시 침투: 취약 부분
- 유리섬유: 고강도 필요 부분
- 금속 인서트: 나사 결합부
- 접착제: 분할선 보강
```

**3. 기능성 추가**:
```
옵션:
- 고무발: 진동 방지
- 자석: 분리형 결합
- 힌지핀: 가동 부분
- LED 홈: 상태 표시
```

## 📈 출력 최적화 팁

### 재료별 특화 설정

**PLA+ 최적화**:
```
온도: 215°C (±5°C 조정)
속도: 50mm/s (복잡 구간 30mm/s)
냉각: 80% (첫 레이어 0%)
리트랙션: 4.5mm, 45mm/s
```

**PETG 최적화**:
```
온도: 235°C (±10°C 조정)
속도: 40mm/s (리니어 어드밴스 고려)
냉각: 30% (오버행 부분 50%)
리트랙션: 2.0mm, 25mm/s
```

### 프린터별 특화 팁

**Ender 3 시리즈**:
- 베드 스프링 교체 (실리콘 스페이서)
- BL-Touch 설치 권장
- 직진 압출기 업그레이드
- 메인보드 교체 (32비트)

**Prusa i3 시리즈**:
- 기본 설정으로도 우수한 결과
- PETG 시트 사용 권장
- MMU2S 멀티컬러 출력 고려
- OctoPrint 연동 활용

**Creality CR-10 시리즈**:
- 대형 베드 레벨링 주의
- 보우덴 시스템 한계 고려
- 인클로저 제작 권장
- 이중 Z축 업그레이드

## 🔍 품질 평가 기준

### A급 품질 (상업적 수준)
- 표면 조도: Ra 3.2 이하
- 치수 정확도: ±0.05mm
- 레이어 접착: 완벽
- 표면 결함: 0개

### B급 품질 (사용 가능)
- 표면 조도: Ra 6.3 이하
- 치수 정확도: ±0.1mm
- 레이어 접착: 양호
- 표면 결함: 2개 이하

### C급 품질 (재출력 권장)
- 표면 조도: Ra 12.5 초과
- 치수 정확도: ±0.2mm 초과
- 레이어 접착: 불량
- 표면 결함: 3개 이상

---

**🖨️ 완벽한 3D 프린팅으로 전문적인 케이스를! 🖨️**

체계적인 가이드를 따라 누구나 고품질 3D 프린팅 결과물을 얻을 수 있습니다! 🚀