// SmartFan PWA Application
// Version: 3.0.1

class SmartFanApp {
    constructor() {
        this.baseURL = window.location.hostname === 'localhost' 
            ? 'http://192.168.1.100' // 개발용 ESP32 IP
            : `http://${window.location.hostname}`;
        
        this.websocket = null;
        this.updateInterval = 2000;
        this.lastUpdate = null;
        this.isConnected = false;
        this.retryCount = 0;
        this.maxRetries = 5;
        
        // 상태 데이터
        this.systemData = {
            speed: 0,
            direction: true,
            enabled: false,
            safety: false,
            temperature: 0,
            humidity: 0,
            current: 0,
            light: 0,
            runtime: 0,
            auto: false
        };
        
        // 설정
        this.settings = {
            deviceIP: '192.168.1.100',
            updateInterval: 2000,
            enableNotifications: true,
            soundAlerts: true
        };
        
        this.init();
    }
    
    async init() {
        console.log('SmartFan App 초기화 중...');
        
        // 설정 로드
        this.loadSettings();
        
        // DOM 요소 초기화
        this.initializeElements();
        
        // 이벤트 리스너 설정
        this.setupEventListeners();
        
        // 연결 시도
        await this.connectToDevice();
        
        // 로딩 화면 숨기기
        this.hideLoadingScreen();
        
        console.log('SmartFan App 초기화 완료');
    }
    
    initializeElements() {
        // 상태 요소들
        this.elements = {
            // 로딩
            loadingScreen: document.getElementById('loadingScreen'),
            app: document.getElementById('app'),
            
            // 연결 상태
            connectionStatus: document.getElementById('connectionStatus'),
            
            // 상태 표시
            systemStatus: document.getElementById('systemStatus'),
            currentSpeed: document.getElementById('currentSpeed'),
            currentDirection: document.getElementById('currentDirection'),
            currentTemp: document.getElementById('currentTemp'),
            currentHumidity: document.getElementById('currentHumidity'),
            
            // 제어 버튼
            startBtn: document.getElementById('startBtn'),
            stopBtn: document.getElementById('stopBtn'),
            emergencyBtn: document.getElementById('emergencyBtn'),
            directionBtn: document.getElementById('directionBtn'),
            refreshSensors: document.getElementById('refreshSensors'),
            
            // 슬라이더
            speedSlider: document.getElementById('speedSlider'),
            windDirectionSlider: document.getElementById('windDirectionSlider'),
            
            // 센서 데이터
            sensorTemp: document.getElementById('sensorTemp'),
            sensorHumidity: document.getElementById('sensorHumidity'),
            sensorCurrent: document.getElementById('sensorCurrent'),
            sensorLight: document.getElementById('sensorLight'),
            
            // 자동 모드
            autoModeToggle: document.getElementById('autoModeToggle'),
            autoModeSettings: document.getElementById('autoModeSettings'),
            targetTemp: document.getElementById('targetTemp'),
            tempThreshold: document.getElementById('tempThreshold'),
            maxAutoSpeed: document.getElementById('maxAutoSpeed'),
            
            // 설정 모달
            settingsBtn: document.getElementById('settingsBtn'),
            settingsModal: document.getElementById('settingsModal'),
            closeSettings: document.getElementById('closeSettings'),
            saveSettings: document.getElementById('saveSettings'),
            deviceIP: document.getElementById('deviceIP'),
            updateInterval: document.getElementById('updateInterval'),
            enableNotifications: document.getElementById('enableNotifications'),
            soundAlerts: document.getElementById('soundAlerts'),
            libraryVersion: document.getElementById('libraryVersion'),
            lastUpdate: document.getElementById('lastUpdate'),
            
            // 토스트
            toast: document.getElementById('toast')
        };
    }
    
    setupEventListeners() {
        // 제어 버튼
        this.elements.startBtn.addEventListener('click', () => this.sendCommand('START'));
        this.elements.stopBtn.addEventListener('click', () => this.sendCommand('STOP'));
        this.elements.emergencyBtn.addEventListener('click', () => this.emergencyStop());
        this.elements.directionBtn.addEventListener('click', () => this.sendCommand('DIR'));
        this.elements.refreshSensors.addEventListener('click', () => this.refreshSensorData());
        
        // 슬라이더
        this.elements.speedSlider.addEventListener('input', (e) => {
            this.sendCommand(`SPEED:${e.target.value}`);
        });
        
        this.elements.windDirectionSlider.addEventListener('input', (e) => {
            this.sendCommand(`SERVO:${e.target.value}`);
        });
        
        // 자동 모드
        this.elements.autoModeToggle.addEventListener('change', (e) => {
            this.toggleAutoMode(e.target.checked);
        });
        
        // 자동 모드 설정
        ['targetTemp', 'tempThreshold', 'maxAutoSpeed'].forEach(id => {
            this.elements[id].addEventListener('change', () => {
                this.updateAutoModeSettings();
            });
        });
        
        // 설정 모달
        this.elements.settingsBtn.addEventListener('click', () => this.openSettings());
        this.elements.closeSettings.addEventListener('click', () => this.closeSettings());
        this.elements.saveSettings.addEventListener('click', () => this.saveSettingsAndClose());
        
        // 모달 외부 클릭 시 닫기
        this.elements.settingsModal.addEventListener('click', (e) => {
            if (e.target === this.elements.settingsModal) {
                this.closeSettings();
            }
        });
        
        // 키보드 단축키
        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape') {
                this.closeSettings();
            }
        });
        
        // 페이지 언로드 시 WebSocket 정리
        window.addEventListener('beforeunload', () => {
            if (this.websocket) {
                this.websocket.close();
            }
        });
        
        // 온라인/오프라인 상태
        window.addEventListener('online', () => this.handleOnlineStatusChange(true));
        window.addEventListener('offline', () => this.handleOnlineStatusChange(false));
    }
    
    async connectToDevice() {
        try {
            console.log('디바이스 연결 시도 중...');
            
            // HTTP API 연결 테스트
            const response = await fetch(`${this.baseURL}/api/system/info`, {
                method: 'GET',
                timeout: 5000
            });
            
            if (response.ok) {
                const data = await response.json();
                console.log('HTTP 연결 성공:', data);
                
                // WebSocket 연결
                this.connectWebSocket();
                
                // 초기 데이터 로드
                await this.loadInitialData();
                
                this.updateConnectionStatus(true);
                this.retryCount = 0;
            } else {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
        } catch (error) {
            console.error('연결 실패:', error);
            this.updateConnectionStatus(false);
            this.scheduleReconnect();
        }
    }
    
    connectWebSocket() {
        try {
            const wsURL = `ws://${this.baseURL.replace('http://', '')}/ws`;
            console.log('WebSocket 연결 시도:', wsURL);
            
            this.websocket = new WebSocket(wsURL);
            
            this.websocket.onopen = () => {
                console.log('WebSocket 연결됨');
                this.isConnected = true;
                this.updateConnectionStatus(true);
                this.retryCount = 0;
            };
            
            this.websocket.onmessage = (event) => {
                try {
                    const data = JSON.parse(event.data);
                    this.handleWebSocketData(data);
                } catch (error) {
                    console.error('WebSocket 데이터 파싱 오류:', error);
                }
            };
            
            this.websocket.onclose = () => {
                console.log('WebSocket 연결 종료');
                this.isConnected = false;
                this.updateConnectionStatus(false);
                this.scheduleReconnect();
            };
            
            this.websocket.onerror = (error) => {
                console.error('WebSocket 오류:', error);
            };
        } catch (error) {
            console.error('WebSocket 연결 실패:', error);
            this.scheduleReconnect();
        }
    }
    
    handleWebSocketData(data) {
        // 시스템 데이터 업데이트
        Object.assign(this.systemData, data);
        
        // UI 업데이트
        this.updateUI();
        
        // 마지막 업데이트 시간 기록
        this.lastUpdate = new Date();
        this.elements.lastUpdate.textContent = this.lastUpdate.toLocaleTimeString();
        
        // 안전 모드 알림
        if (data.safety && !this.systemData.safety) {
            this.showNotification('⚠️ 안전 모드 활성화됨', 'warning');
            this.playAlertSound();
        }
    }
    
    async loadInitialData() {
        try {
            // 시스템 정보
            const systemInfo = await this.apiCall('/api/system/info');
            if (systemInfo.version) {
                this.elements.libraryVersion.textContent = systemInfo.version;
            }
            
            // 현재 상태
            const status = await this.apiCall('/api/fan/status');
            if (status) {
                Object.assign(this.systemData, status);
                this.updateUI();
            }
            
            // 센서 데이터
            const sensors = await this.apiCall('/api/sensors/current');
            if (sensors) {
                Object.assign(this.systemData, sensors);
                this.updateSensorDisplay();
            }
            
        } catch (error) {
            console.error('초기 데이터 로드 실패:', error);
        }
    }
    
    async apiCall(endpoint, method = 'GET', data = null) {
        try {
            const options = {
                method,
                headers: {
                    'Content-Type': 'application/json',
                },
                timeout: 5000
            };
            
            if (data) {
                options.body = JSON.stringify(data);
            }
            
            const response = await fetch(`${this.baseURL}${endpoint}`, options);
            
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
            
            return await response.json();
        } catch (error) {
            console.error(`API 호출 실패 (${endpoint}):`, error);
            throw error;
        }
    }
    
    async sendCommand(command) {
        try {
            console.log('명령 전송:', command);
            
            // WebSocket으로 즉시 전송 시도
            if (this.websocket && this.websocket.readyState === WebSocket.OPEN) {
                this.websocket.send(JSON.stringify({ command }));
            } else {
                // HTTP API 백업 사용
                await this.apiCall('/api/fan/control', 'POST', { command });
            }
            
            // UI 피드백
            this.showNotification(`명령 전송: ${command}`, 'info');
            
        } catch (error) {
            console.error('명령 전송 실패:', error);
            this.showNotification('명령 전송 실패', 'error');
        }
    }
    
    async emergencyStop() {
        try {
            // 중요한 명령이므로 HTTP와 WebSocket 모두 사용
            const promises = [];
            
            if (this.websocket && this.websocket.readyState === WebSocket.OPEN) {
                promises.push(new Promise((resolve) => {
                    this.websocket.send(JSON.stringify({ command: 'EMERGENCY' }));
                    resolve();
                }));
            }
            
            promises.push(this.apiCall('/api/fan/control', 'POST', { command: 'EMERGENCY' }));
            
            await Promise.all(promises);
            
            this.showNotification('🛑 비상정지 실행됨', 'warning');
            this.playAlertSound();
            
        } catch (error) {
            console.error('비상정지 실패:', error);
            this.showNotification('비상정지 실패', 'error');
        }
    }
    
    async refreshSensorData() {
        try {
            this.elements.refreshSensors.innerHTML = '<i class="material-icons">refresh</i>';
            this.elements.refreshSensors.style.animation = 'spin 1s linear infinite';
            
            const sensors = await this.apiCall('/api/sensors/current');
            if (sensors) {
                Object.assign(this.systemData, sensors);
                this.updateSensorDisplay();
                this.showNotification('센서 데이터 업데이트됨', 'success');
            }
            
        } catch (error) {
            console.error('센서 데이터 갱신 실패:', error);
            this.showNotification('센서 데이터 갱신 실패', 'error');
        } finally {
            this.elements.refreshSensors.style.animation = '';
        }
    }
    
    async toggleAutoMode(enabled) {
        try {
            await this.sendCommand(`AUTO:${enabled ? 'ON' : 'OFF'}`);
            
            this.systemData.auto = enabled;
            this.elements.autoModeSettings.style.display = enabled ? 'block' : 'none';
            
            if (enabled) {
                this.updateAutoModeSettings();
            }
            
            this.showNotification(`자동 모드 ${enabled ? '활성화' : '비활성화'}`, 'info');
            
        } catch (error) {
            console.error('자동 모드 토글 실패:', error);
            this.elements.autoModeToggle.checked = !enabled; // 원상복구
        }
    }
    
    async updateAutoModeSettings() {
        try {
            const settings = {
                targetTemp: parseFloat(this.elements.targetTemp.value),
                tempThreshold: parseFloat(this.elements.tempThreshold.value),
                maxSpeed: parseInt(this.elements.maxAutoSpeed.value)
            };
            
            await this.apiCall('/api/settings/auto', 'POST', settings);
            this.showNotification('자동 모드 설정 업데이트됨', 'success');
            
        } catch (error) {
            console.error('자동 모드 설정 실패:', error);
            this.showNotification('설정 업데이트 실패', 'error');
        }
    }
    
    updateUI() {
        // 시스템 상태
        const statusElement = this.elements.systemStatus;
        if (this.systemData.safety) {
            statusElement.textContent = '안전 모드';
            statusElement.className = 'status-indicator safety';
        } else if (this.systemData.enabled) {
            statusElement.textContent = '동작 중';
            statusElement.className = 'status-indicator running';
        } else {
            statusElement.textContent = '정지';
            statusElement.className = 'status-indicator stopped';
        }
        
        // 상태 값들
        this.elements.currentSpeed.textContent = `${this.systemData.speed}%`;
        this.elements.currentDirection.textContent = this.systemData.direction ? '시계방향' : '반시계방향';
        this.elements.currentTemp.textContent = `${this.systemData.temperature?.toFixed(1) || '--'}°C`;
        this.elements.currentHumidity.textContent = `${this.systemData.humidity?.toFixed(1) || '--'}%`;
        
        // 슬라이더 동기화
        if (!this.elements.speedSlider.matches(':focus')) {
            this.elements.speedSlider.value = this.systemData.speed;
        }
        
        // 자동 모드
        this.elements.autoModeToggle.checked = this.systemData.auto;
        this.elements.autoModeSettings.style.display = this.systemData.auto ? 'block' : 'none';
        
        // 센서 데이터 업데이트
        this.updateSensorDisplay();
    }
    
    updateSensorDisplay() {
        this.elements.sensorTemp.textContent = `${this.systemData.temperature?.toFixed(1) || '--'}°C`;
        this.elements.sensorHumidity.textContent = `${this.systemData.humidity?.toFixed(1) || '--'}%`;
        this.elements.sensorCurrent.textContent = `${this.systemData.current?.toFixed(2) || '--'}A`;
        this.elements.sensorLight.textContent = `${this.systemData.light || '--'}%`;
    }
    
    updateConnectionStatus(connected) {
        this.isConnected = connected;
        const status = this.elements.connectionStatus;
        
        if (connected) {
            status.className = 'connection-status online';
            status.innerHTML = '<i class="material-icons">wifi</i><span>연결됨</span>';
        } else {
            status.className = 'connection-status offline';
            status.innerHTML = '<i class="material-icons">wifi_off</i><span>연결 끊김</span>';
        }
    }
    
    scheduleReconnect() {
        if (this.retryCount >= this.maxRetries) {
            console.log('최대 재연결 시도 횟수 초과');
            this.showNotification('연결 실패: 수동으로 새로고침해주세요', 'error');
            return;
        }
        
        const delay = Math.min(1000 * Math.pow(2, this.retryCount), 30000); // 최대 30초
        console.log(`${delay}ms 후 재연결 시도 (${this.retryCount + 1}/${this.maxRetries})`);
        
        setTimeout(() => {
            this.retryCount++;
            this.connectToDevice();
        }, delay);
    }
    
    handleOnlineStatusChange(online) {
        if (online) {
            console.log('네트워크 연결 복구됨');
            this.connectToDevice();
        } else {
            console.log('네트워크 연결 끊김');
            this.updateConnectionStatus(false);
        }
    }
    
    openSettings() {
        // 현재 설정값 표시
        this.elements.deviceIP.value = this.settings.deviceIP;
        this.elements.updateInterval.value = this.settings.updateInterval;
        this.elements.enableNotifications.checked = this.settings.enableNotifications;
        this.elements.soundAlerts.checked = this.settings.soundAlerts;
        
        this.elements.settingsModal.style.display = 'block';
    }
    
    closeSettings() {
        this.elements.settingsModal.style.display = 'none';
    }
    
    saveSettingsAndClose() {
        // 설정 업데이트
        this.settings.deviceIP = this.elements.deviceIP.value;
        this.settings.updateInterval = parseInt(this.elements.updateInterval.value);
        this.settings.enableNotifications = this.elements.enableNotifications.checked;
        this.settings.soundAlerts = this.elements.soundAlerts.checked;
        
        // 로컬 저장소에 저장
        localStorage.setItem('smartfan-settings', JSON.stringify(this.settings));
        
        // 연결 URL 업데이트
        if (this.settings.deviceIP !== this.baseURL.replace('http://', '')) {
            this.baseURL = `http://${this.settings.deviceIP}`;
            this.connectToDevice(); // 새 주소로 재연결
        }
        
        this.closeSettings();
        this.showNotification('설정이 저장되었습니다', 'success');
    }
    
    loadSettings() {
        try {
            const saved = localStorage.getItem('smartfan-settings');
            if (saved) {
                Object.assign(this.settings, JSON.parse(saved));
                this.baseURL = `http://${this.settings.deviceIP}`;
            }
        } catch (error) {
            console.error('설정 로드 실패:', error);
        }
    }
    
    showNotification(message, type = 'info') {
        if (!this.settings.enableNotifications) return;
        
        const toast = this.elements.toast;
        toast.textContent = message;
        toast.className = `toast show ${type}`;
        
        setTimeout(() => {
            toast.className = 'toast';
        }, 3000);
        
        console.log(`알림 [${type}]: ${message}`);
    }
    
    playAlertSound() {
        if (!this.settings.soundAlerts) return;
        
        try {
            // Web Audio API를 사용한 간단한 경고음
            const audioContext = new (window.AudioContext || window.webkitAudioContext)();
            const oscillator = audioContext.createOscillator();
            const gainNode = audioContext.createGain();
            
            oscillator.connect(gainNode);
            gainNode.connect(audioContext.destination);
            
            oscillator.frequency.setValueAtTime(800, audioContext.currentTime);
            gainNode.gain.setValueAtTime(0.1, audioContext.currentTime);
            gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.5);
            
            oscillator.start(audioContext.currentTime);
            oscillator.stop(audioContext.currentTime + 0.5);
        } catch (error) {
            console.log('알림음 재생 실패:', error);
        }
    }
    
    hideLoadingScreen() {
        setTimeout(() => {
            this.elements.loadingScreen.style.opacity = '0';
            setTimeout(() => {
                this.elements.loadingScreen.style.display = 'none';
                this.elements.app.style.display = 'block';
                setTimeout(() => {
                    this.elements.app.style.opacity = '1';
                }, 50);
            }, 300);
        }, 1000);
    }
}

// 앱 초기화
let smartFanApp;

document.addEventListener('DOMContentLoaded', () => {
    console.log('SmartFan PWA 시작');
    smartFanApp = new SmartFanApp();
});

// PWA 설치 프롬프트
let deferredPrompt;

window.addEventListener('beforeinstallprompt', (e) => {
    console.log('PWA 설치 프롬프트 준비됨');
    e.preventDefault();
    deferredPrompt = e;
    
    // 설치 버튼 표시 (필요시)
    showInstallButton();
});

function showInstallButton() {
    // PWA 설치 버튼 로직 (선택사항)
    const installButton = document.createElement('button');
    installButton.textContent = '앱 설치';
    installButton.className = 'btn btn-outline install-btn';
    installButton.style.position = 'fixed';
    installButton.style.bottom = '20px';
    installButton.style.right = '20px';
    installButton.style.zIndex = '1000';
    
    installButton.addEventListener('click', async () => {
        if (deferredPrompt) {
            deferredPrompt.prompt();
            const { outcome } = await deferredPrompt.userChoice;
            console.log('PWA 설치 결과:', outcome);
            deferredPrompt = null;
            installButton.remove();
        }
    });
    
    document.body.appendChild(installButton);
    
    // 5초 후 자동 숨김
    setTimeout(() => {
        if (installButton.parentNode) {
            installButton.remove();
        }
    }, 5000);
}

// 앱 설치 후 이벤트
window.addEventListener('appinstalled', () => {
    console.log('PWA 설치 완료');
    if (smartFanApp) {
        smartFanApp.showNotification('앱이 설치되었습니다!', 'success');
    }
});

// 디버그용 전역 접근
if (window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1') {
    window.smartFanApp = smartFanApp;
    window.debugMode = true;
    console.log('Debug mode enabled - smartFanApp is available globally');
}