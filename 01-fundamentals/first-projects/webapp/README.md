# 📱 SmartFan PWA (Progressive Web App)

**스마트 선풍기 제어를 위한 모바일 친화적 웹 애플리케이션**

## 🌟 주요 특징

### 📱 **Progressive Web App**
- **오프라인 지원**: 네트워크 없이도 기본 기능 사용 가능
- **앱 설치**: 홈 화면에 설치하여 네이티브 앱처럼 사용
- **반응형 디자인**: 모바일, 태블릿, 데스크톱 모든 화면에 최적화
- **빠른 로딩**: Service Worker를 통한 캐싱으로 빠른 앱 로딩

### 🎛️ **실시간 제어**
- **즉시 응답**: WebSocket 기반 실시간 통신
- **직관적 UI**: Material Design 기반의 사용하기 쉬운 인터페이스
- **터치 최적화**: 모바일 터치 인터페이스에 최적화
- **시각적 피드백**: 버튼 애니메이션과 상태 표시

### 🔧 **완전한 기능**
- **모터 제어**: 속도, 방향, 시작/정지
- **센서 모니터링**: 온도, 습도, 전류, 조도 실시간 표시
- **자동 모드**: 온도 기반 자동 속도 조절
- **안전 시스템**: 비상정지 및 안전 모드 표시

## 📋 기술 스택

- **Frontend**: Vanilla JavaScript (ES6+), CSS3, HTML5
- **UI Framework**: Material Design Components
- **PWA Features**: Service Worker, Web App Manifest
- **Communication**: WebSocket, Fetch API
- **Responsive**: CSS Grid, Flexbox
- **Fonts**: Noto Sans KR, Material Icons

## 🚀 설치 및 실행

### 1. ESP32에 업로드

```bash
# ESP32 웹서버 코드에 이 PWA 파일들을 포함시키거나
# SPIFFS를 사용하여 ESP32에 업로드

arduino-cli upload --fqbn esp32:esp32:esp32 ESP32_WiFi_Controller.ino
```

### 2. 개발 환경에서 테스트

```bash
# Python 간단 서버 (개발용)
cd src/webapp
python -m http.server 8080

# 또는 Node.js live-server
npx live-server --port=8080
```

### 3. ESP32 IP 주소 확인

```bash
# 시리얼 모니터에서 ESP32 IP 주소 확인
# 예: 192.168.1.100
```

### 4. 브라우저에서 접속

```
http://192.168.1.100  # ESP32 IP 주소
# 또는 개발 환경
http://localhost:8080
```

## 📱 PWA 설치 방법

### Android (Chrome)
1. 웹사이트 접속
2. 주소창 오른쪽 "설치" 버튼 클릭
3. "설치" 확인
4. 홈 화면에 아이콘 생성됨

### iOS (Safari)
1. Safari에서 웹사이트 접속
2. 공유 버튼 (⬆️) 터치
3. "홈 화면에 추가" 선택
4. "추가" 터치

### 데스크톱 (Chrome/Edge)
1. 주소창 오른쪽 설치 아이콘 클릭
2. "설치" 버튼 클릭
3. 작업표시줄/독에 아이콘 생성됨

## 🎛️ 사용법

### 기본 제어

```
🟢 시작: 팬 모터 시작
🔴 정지: 팬 모터 정지  
🛑 비상정지: 즉시 안전 정지
🔄 방향 전환: 시계방향 ↔ 반시계방향
```

### 속도 조절

```
🎚️ 속도 슬라이더: 0% ~ 100% 무단 조절
🌪️ 풍향 슬라이더: 0° ~ 180° 서보 제어
```

### 센서 모니터링

```
🌡️ 온도: 실시간 온도 표시
💧 습도: 공기 습도 백분율
⚡ 전류: 모터 소비 전류
💡 조도: 주변 밝기 수준
```

### 자동 모드

```
🤖 자동 모드 ON/OFF
🎯 목표 온도: 15°C ~ 35°C
📏 온도 임계값: 1°C ~ 5°C
⚡ 최대 속도: 50% ~ 100%
```

## ⚙️ 설정

### 연결 설정

- **ESP32 IP 주소**: 디바이스 IP 주소 설정
- **업데이트 간격**: 1초 / 2초 / 5초 선택

### 알림 설정

- **상태 알림**: 시스템 상태 변화 알림
- **소리 알림**: 경고음 활성화/비활성화

## 🔧 API 엔드포인트

### 시스템 정보

```javascript
GET /api/system/info
응답: {
  "version": "3.0.1",
  "uptime": 3600,
  "freeMemory": 180000
}
```

### 팬 상태

```javascript
GET /api/fan/status
응답: {
  "speed": 50,
  "direction": true,
  "enabled": true,
  "safety": false,
  "runtime": 7200
}
```

### 팬 제어

```javascript
POST /api/fan/control
요청: {
  "command": "START" | "STOP" | "SPEED:50" | "DIR" | "EMERGENCY"
}
```

### 센서 데이터

```javascript
GET /api/sensors/current
응답: {
  "temperature": 25.5,
  "humidity": 60.2,
  "current": 0.85,
  "light": 75
}
```

### 자동 모드 설정

```javascript
POST /api/settings/auto
요청: {
  "targetTemp": 25.0,
  "tempThreshold": 2.0,
  "maxSpeed": 90
}
```

## 🌐 WebSocket 통신

### 연결

```javascript
const ws = new WebSocket('ws://192.168.1.100/ws');
```

### 메시지 형식

```javascript
// 명령 전송
ws.send(JSON.stringify({
  "command": "SPEED:75"
}));

// 상태 수신
{
  "speed": 75,
  "direction": true,
  "enabled": true,
  "temperature": 26.1,
  "humidity": 58.3,
  "current": 0.92,
  "light": 80,
  "timestamp": 1672531200
}
```

## 📱 오프라인 기능

### 캐시된 기능

- ✅ 기본 UI 표시
- ✅ 마지막 센서 데이터 표시
- ✅ 설정 저장/불러오기
- ✅ 오프라인 상태 표시

### 제한된 기능

- ❌ 실시간 제어 (네트워크 필요)
- ❌ 센서 데이터 업데이트
- ❌ 상태 변경

### 재연결 동작

```javascript
// 자동 재연결 시도
- 첫 시도: 1초 후
- 두 번째: 2초 후  
- 세 번째: 4초 후
- 최대: 30초 간격
- 최대 재시도: 5회
```

## 🎨 UI 컴포넌트

### 상태 카드

```html
<div class="card">
  <div class="card-header">
    <h2><i class="material-icons">dashboard</i> 시스템 상태</h2>
    <div class="status-indicator running">동작 중</div>
  </div>
  <div class="card-content">
    <!-- 상태 그리드 -->
  </div>
</div>
```

### 제어 버튼

```html
<button class="btn btn-primary">
  <i class="material-icons">play_arrow</i>
  시작
</button>
```

### 슬라이더

```html
<div class="slider-container">
  <input type="range" class="slider" min="0" max="100" value="50">
  <div class="slider-labels">
    <span>0%</span>
    <span>50%</span>
    <span>100%</span>
  </div>
</div>
```

### 센서 아이템

```html
<div class="sensor-item">
  <div class="sensor-icon">
    <i class="material-icons">thermostat</i>
  </div>
  <div class="sensor-info">
    <div class="sensor-label">온도</div>
    <div class="sensor-value">25.5°C</div>
  </div>
</div>
```

## 📊 성능 최적화

### 번들 크기 최적화

- **CSS**: 약 25KB (압축)
- **JavaScript**: 약 35KB (압축)
- **HTML**: 약 8KB
- **총 크기**: 약 70KB (초기 로드)

### 캐싱 전략

```javascript
// 정적 파일: 캐시 우선
Cache-First Strategy
- HTML, CSS, JS, 이미지
- 오프라인에서도 즉시 로드

// API 데이터: 네트워크 우선  
Network-First Strategy
- 실시간 센서 데이터
- 제어 명령
```

### 성능 지표

- **First Contentful Paint**: < 1.5초
- **Largest Contentful Paint**: < 2.5초
- **Time to Interactive**: < 3.5초
- **Cumulative Layout Shift**: < 0.1

## 🐛 문제 해결

### 연결 문제

**문제**: "연결 끊김" 표시
**해결**:
1. ESP32 전원 및 WiFi 연결 확인
2. IP 주소 정확성 확인
3. 방화벽 설정 확인
4. 브라우저 캐시 삭제

### PWA 설치 문제

**문제**: 설치 버튼이 나타나지 않음
**해결**:
1. HTTPS 또는 localhost에서 실행 확인
2. manifest.json 파일 존재 확인
3. Service Worker 등록 확인
4. 브라우저 PWA 지원 확인

### 성능 문제

**문제**: 앱이 느림
**해결**:
1. 네트워크 연결 상태 확인
2. 브라우저 개발자 도구에서 성능 분석
3. 업데이트 간격 조정 (설정에서)
4. 브라우저 캐시 활용

### 오프라인 문제

**문제**: 오프라인에서 앱이 로드되지 않음
**해결**:
1. Service Worker 등록 상태 확인
2. 브라우저 캐시 저장 공간 확인
3. 개발자 도구에서 Application > Storage 확인

## 🔄 업데이트

### 자동 업데이트

```javascript
// Service Worker 업데이트 감지
if ('serviceWorker' in navigator) {
  navigator.serviceWorker.addEventListener('controllerchange', () => {
    window.location.reload();
  });
}
```

### 수동 업데이트

1. 브라우저 새로고침 (Ctrl+F5)
2. 앱 재설치
3. 브라우저 캐시 삭제

## 📞 지원

- **GitHub Issues**: [버그 신고](https://github.com/Jirehhyeon/Arduino-Daily-project/issues)
- **Documentation**: [프로젝트 문서](https://github.com/Jirehhyeon/Arduino-Daily-project)
- **Community**: [토론 포럼](https://github.com/Jirehhyeon/Arduino-Daily-project/discussions)

---

**📱 어디서든 스마트 선풍기를 제어하세요! 📱**

PWA 기술로 네이티브 앱 같은 경험을 웹에서 제공합니다. 오프라인에서도 동작하고, 홈 화면에 설치하여 언제든지 빠르게 접근할 수 있습니다! 🚀