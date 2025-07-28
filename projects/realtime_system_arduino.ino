/*
 * Project 10: Real-Time Embedded System (Arduino Version)
 * Description: Industrial-grade real-time multitasking system
 * Hardware: Arduino Uno, I/O modules, Real-time control application
 * 
 * Connection:
 * - Control Outputs: Pins 4-7 (Digital outputs for actuators)
 * - Sensor Inputs: A0-A3 (Analog sensors)
 * - Digital Inputs: Pins 8-11 (Limit switches, buttons)
 * - Emergency Stop: Pin 2 (INT0 interrupt)
 * - Status LEDs: Pin 12 (Green), Pin 13 (Red/Built-in)
 * - Heartbeat LED: Pin 3 (System alive indicator)
 * - Communication: Serial (USB) for monitoring
 * 
 * Real-time Requirements:
 * - Control loop: 1ms period, <0.9ms deadline
 * - Sensor acquisition: 10ms period, <8ms deadline  
 * - System monitoring: 100ms period, <90ms deadline
 * - Maximum response time: <2ms
 * - CPU utilization: <80%
 * - Jitter: <100μs
 */

#include <avr/wdt.h>
#include <TimerOne.h>

// 실시간 시스템 설정
#define MAX_TASKS 8
#define TICK_PERIOD_US 100
#define SCHEDULER_FREQUENCY_HZ 10000

// 핀 정의
#define EMERGENCY_STOP_PIN 2
#define HEARTBEAT_LED_PIN 3
#define CONTROL_OUTPUT_BASE 4    // Pins 4-7
#define DIGITAL_INPUT_BASE 8     // Pins 8-11  
#define STATUS_GREEN_LED 12
#define STATUS_RED_LED 13

// 태스크 상태
enum TaskState {
  TASK_READY = 0,
  TASK_RUNNING = 1,
  TASK_WAITING = 2,
  TASK_SUSPENDED = 3,
  TASK_COMPLETED = 4
};

// 태스크 우선순위
enum TaskPriority {
  PRIORITY_CRITICAL = 0,
  PRIORITY_HIGH = 1,
  PRIORITY_NORMAL = 2,
  PRIORITY_LOW = 3,
  PRIORITY_IDLE = 4
};

// 태스크 제어 블록
struct TaskControlBlock {
  uint8_t taskId;
  TaskState state;
  TaskPriority priority;
  
  unsigned long periodUs;
  unsigned long deadlineUs;
  unsigned long executionTimeUs;
  unsigned long nextRunTime;
  unsigned long lastStartTime;
  unsigned long worstCaseTime;
  
  unsigned long runCount;
  unsigned long deadlineMisses;
  unsigned long totalRuntime;
  
  void (*taskFunction)();
  
  bool enabled;
  bool periodic;
  String name;
};

// 시스템 성능 메트릭
struct SystemMetrics {
  unsigned long totalTicks;
  unsigned long idleTicks;
  unsigned long schedulerOverhead;
  uint8_t cpuUtilization;
  unsigned int maxResponseTimeUs;
  unsigned int avgJitterUs;
  uint8_t activeTasks;
};

// 실시간 I/O 상태
struct RealtimeIO {
  uint16_t analogInputs[4];
  uint8_t digitalInputs;
  uint8_t digitalOutputs;
  int16_t controlOutput;
  bool emergencyStop;
  uint16_t systemStatus;
} rtIO = {0};

// 전역 변수
volatile unsigned long systemTickCount = 0;
volatile bool schedulerEnabled = false;
TaskControlBlock taskTable[MAX_TASKS];
SystemMetrics metrics = {0};
volatile uint8_t currentTaskId = 0;

// 태스크 함수 선언
void taskControlLoop();
void taskSensorAcquisition(); 
void taskCommunication();
void taskSystemMonitor();
void taskUserInterface();
void taskWatchdogService();
void taskDiagnostic();
void taskIdle();

void setup() {
  Serial.begin(115200);
  Serial.println("=== Real-Time Embedded System ===");
  Serial.println("Industrial Control System");
  Serial.println("Real-time Requirements:");
  Serial.println("- Control: 1ms cycle, <0.9ms deadline");
  Serial.println("- Max response: <2ms, CPU: <80%");
  Serial.println();
  
  // 핀 설정
  setupGPIO();
  
  // 워치독 타이머 설정
  wdt_enable(WDTO_1S);
  
  // 태스크 테이블 초기화
  initTaskTable();
  
  // 고정밀 타이머 설정 (100μs)
  TimerOne.initialize(TICK_PERIOD_US);
  TimerOne.attachInterrupt(systemTickISR);
  
  // 긴급 정지 인터럽트
  pinMode(EMERGENCY_STOP_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(EMERGENCY_STOP_PIN), emergencyStopISR, FALLING);
  
  // 시스템 초기화 완료 표시
  systemInitialization();
  
  // 스케줄러 활성화
  schedulerEnabled = true;
  
  Serial.println("Real-time scheduler started");
  Serial.println("Commands: status, tasks, metrics, reset, emergency");
  Serial.println();
}

void loop() {
  // 메인 루프는 비실시간 백그라운드 작업만 수행
  // 실시간 태스크들은 타이머 인터럽트에서 실행됨
  
  handleSerialCommands();
  handleEmergencyRecovery();
  
  // 시스템 상태 체크 및 복구
  checkSystemHealth();
  
  delay(10);  // 백그라운드 루프 주기
}

void setupGPIO() {
  // 제어 출력 핀들
  for (int i = 0; i < 4; i++) {
    pinMode(CONTROL_OUTPUT_BASE + i, OUTPUT);
    digitalWrite(CONTROL_OUTPUT_BASE + i, LOW);
  }
  
  // 디지털 입력 핀들
  for (int i = 0; i < 4; i++) {
    pinMode(DIGITAL_INPUT_BASE + i, INPUT_PULLUP);
  }
  
  // 상태 LED들
  pinMode(HEARTBEAT_LED_PIN, OUTPUT);
  pinMode(STATUS_GREEN_LED, OUTPUT);
  pinMode(STATUS_RED_LED, OUTPUT);
  
  // 초기 상태
  digitalWrite(STATUS_GREEN_LED, HIGH);
  digitalWrite(STATUS_RED_LED, LOW);
}

void initTaskTable() {
  // Task 0: 실시간 제어 루프
  taskTable[0] = {
    .taskId = 0,
    .state = TASK_READY,
    .priority = PRIORITY_CRITICAL,
    .periodUs = 1000,      // 1ms
    .deadlineUs = 900,     // 0.9ms
    .taskFunction = taskControlLoop,
    .enabled = true,
    .periodic = true,
    .name = "Control Loop"
  };
  
  // Task 1: 센서 데이터 수집
  taskTable[1] = {
    .taskId = 1,
    .state = TASK_READY,
    .priority = PRIORITY_HIGH,
    .periodUs = 10000,     // 10ms
    .deadlineUs = 8000,    // 8ms
    .taskFunction = taskSensorAcquisition,
    .enabled = true,
    .periodic = true,
    .name = "Sensor Acquisition"
  };
  
  // Task 2: 통신 처리
  taskTable[2] = {
    .taskId = 2,
    .state = TASK_READY,
    .priority = PRIORITY_NORMAL,
    .periodUs = 50000,     // 50ms
    .deadlineUs = 45000,   // 45ms
    .taskFunction = taskCommunication,
    .enabled = true,
    .periodic = true,
    .name = "Communication"
  };
  
  // Task 3: 시스템 모니터링
  taskTable[3] = {
    .taskId = 3,
    .state = TASK_READY,
    .priority = PRIORITY_NORMAL,
    .periodUs = 100000,    // 100ms
    .deadlineUs = 90000,   // 90ms
    .taskFunction = taskSystemMonitor,
    .enabled = true,
    .periodic = true,
    .name = "System Monitor"
  };
  
  // Task 4: 사용자 인터페이스
  taskTable[4] = {
    .taskId = 4,
    .state = TASK_READY,
    .priority = PRIORITY_LOW,
    .periodUs = 200000,    // 200ms
    .deadlineUs = 180000,  // 180ms
    .taskFunction = taskUserInterface,
    .enabled = true,
    .periodic = true,
    .name = "User Interface"
  };
  
  // Task 5: 워치독 서비스
  taskTable[5] = {
    .taskId = 5,
    .state = TASK_READY,
    .priority = PRIORITY_HIGH,
    .periodUs = 500000,    // 500ms
    .deadlineUs = 450000,  // 450ms
    .taskFunction = taskWatchdogService,
    .enabled = true,
    .periodic = true,
    .name = "Watchdog Service"
  };
  
  // Task 6: 시스템 진단
  taskTable[6] = {
    .taskId = 6,
    .state = TASK_READY,
    .priority = PRIORITY_LOW,
    .periodUs = 1000000,   // 1000ms
    .deadlineUs = 900000,  // 900ms
    .taskFunction = taskDiagnostic,
    .enabled = true,
    .periodic = true,
    .name = "Diagnostic"
  };
  
  // Task 7: 유휴 태스크
  taskTable[7] = {
    .taskId = 7,
    .state = TASK_READY,
    .priority = PRIORITY_IDLE,
    .periodUs = 0,
    .deadlineUs = 0,
    .taskFunction = taskIdle,
    .enabled = true,
    .periodic = false,
    .name = "Idle"
  };
  
  // 초기 실행 시간 설정
  unsigned long currentTime = micros();
  for (int i = 0; i < MAX_TASKS; i++) {
    if (taskTable[i].periodic) {
      taskTable[i].nextRunTime = currentTime + taskTable[i].periodUs;
    }
  }
}

// 시스템 틱 인터럽트 (100μs)
void systemTickISR() {
  systemTickCount++;
  
  if (schedulerEnabled) {
    unsigned long schedulerStart = systemTickCount;
    scheduleTasks();
    metrics.schedulerOverhead += (systemTickCount - schedulerStart);
  }
  
  metrics.totalTicks++;
}

// 긴급 정지 인터럽트
void emergencyStopISR() {
  rtIO.emergencyStop = true;
  rtIO.digitalOutputs = 0;
  
  // 모든 제어 출력 즉시 정지
  for (int i = 0; i < 4; i++) {
    digitalWrite(CONTROL_OUTPUT_BASE + i, LOW);
  }
  
  digitalWrite(STATUS_RED_LED, HIGH);
  digitalWrite(STATUS_GREEN_LED, LOW);
}

// EDF (Earliest Deadline First) 스케줄러
void scheduleTasks() {
  unsigned long currentTime = systemTickCount * TICK_PERIOD_US;
  uint8_t nextTask = MAX_TASKS;
  unsigned long earliestDeadline = 0xFFFFFFFF;
  
  // 실행 가능한 태스크 중 데드라인이 가장 빠른 것 선택
  for (uint8_t i = 0; i < MAX_TASKS; i++) {
    if (!taskTable[i].enabled || !taskTable[i].periodic) continue;
    
    if (currentTime >= taskTable[i].nextRunTime) {
      taskTable[i].state = TASK_READY;
      
      unsigned long absoluteDeadline = taskTable[i].nextRunTime + taskTable[i].deadlineUs;
      
      if (absoluteDeadline < earliestDeadline) {
        earliestDeadline = absoluteDeadline;
        nextTask = i;
      }
    }
  }
  
  // 실행할 태스크가 없으면 유휴 태스크
  if (nextTask == MAX_TASKS) {
    nextTask = 7;
    metrics.idleTicks++;
  }
  
  // 태스크 실행
  if (nextTask < MAX_TASKS) {
    executeTask(nextTask);
  }
}

void executeTask(uint8_t taskId) {
  if (taskId >= MAX_TASKS || !taskTable[taskId].enabled) return;
  
  TaskControlBlock* task = &taskTable[taskId];
  unsigned long startTime = micros();
  
  task->state = TASK_RUNNING;
  task->lastStartTime = startTime;
  currentTaskId = taskId;
  
  // 태스크 함수 실행
  task->taskFunction();
  
  // 실행 시간 측정 및 통계 업데이트
  unsigned long endTime = micros();
  unsigned long executionTime = endTime - startTime;
  
  task->executionTimeUs = executionTime;
  task->totalRuntime += executionTime;
  task->runCount++;
  
  if (executionTime > task->worstCaseTime) {
    task->worstCaseTime = executionTime;
  }
  
  if (executionTime > metrics.maxResponseTimeUs) {
    metrics.maxResponseTimeUs = executionTime;
  }
  
  // 데드라인 체크
  if (executionTime > task->deadlineUs) {
    task->deadlineMisses++;
  }
  
  // 다음 실행 시간 설정
  if (task->periodic) {
    task->nextRunTime += task->periodUs;
    task->state = TASK_WAITING;
  } else {
    task->state = TASK_COMPLETED;
  }
}

// 실시간 태스크 구현들
void taskControlLoop() {
  // 1ms 실시간 제어 루프
  if (rtIO.emergencyStop) return;
  
  // 센서 값 읽기
  rtIO.analogInputs[0] = analogRead(A0);
  
  // PID 제어 (간단한 비례 제어)
  static int setpoint = 512;
  int error = setpoint - rtIO.analogInputs[0];
  rtIO.controlOutput = error / 4;
  
  // 제어 출력 적용
  if (rtIO.controlOutput > 0) {
    rtIO.digitalOutputs |= 0x01;
    digitalWrite(CONTROL_OUTPUT_BASE, HIGH);
  } else {
    rtIO.digitalOutputs &= ~0x01;
    digitalWrite(CONTROL_OUTPUT_BASE, LOW);
  }
}

void taskSensorAcquisition() {
  // 10ms 센서 데이터 수집
  
  // 모든 아날로그 센서 읽기
  for (int i = 0; i < 4; i++) {
    rtIO.analogInputs[i] = analogRead(A0 + i);
  }
  
  // 디지털 입력 읽기
  rtIO.digitalInputs = 0;
  for (int i = 0; i < 4; i++) {
    if (digitalRead(DIGITAL_INPUT_BASE + i) == LOW) {
      rtIO.digitalInputs |= (1 << i);
    }
  }
  
  // 센서 데이터 필터링 및 검증
  static int filterBuffer[4][8];
  static int filterIndex = 0;
  static long filterSum[4] = {0};
  
  for (int i = 0; i < 4; i++) {
    filterSum[i] -= filterBuffer[i][filterIndex];
    filterBuffer[i][filterIndex] = rtIO.analogInputs[i];
    filterSum[i] += filterBuffer[i][filterIndex];
    rtIO.analogInputs[i] = filterSum[i] / 8;
    
    // 센서 범위 체크
    if (rtIO.analogInputs[i] < 50 || rtIO.analogInputs[i] > 1000) {
      rtIO.systemStatus |= (1 << i);
    } else {
      rtIO.systemStatus &= ~(1 << i);
    }
  }
  
  filterIndex = (filterIndex + 1) % 8;
}

void taskCommunication() {
  // 50ms 통신 처리
  
  // 시스템 상태 데이터 전송 (간단한 텍스트 프로토콜)
  if (Serial.availableForWrite() > 50) {  // 버퍼 공간 확인
    Serial.print("STATUS:");
    Serial.print(rtIO.digitalInputs, HEX);
    Serial.print(",");
    Serial.print(rtIO.digitalOutputs, HEX);
    Serial.print(",");
    Serial.print(rtIO.analogInputs[0]);
    Serial.print(",");
    Serial.print(rtIO.controlOutput);
    Serial.print(",");
    Serial.print(metrics.cpuUtilization);
    Serial.print(",");
    Serial.println(rtIO.emergencyStop ? 1 : 0);
  }
}

void taskSystemMonitor() {
  // 100ms 시스템 모니터링
  
  // CPU 사용률 계산
  static unsigned long lastTotalTicks = 0;
  static unsigned long lastIdleTicks = 0;
  
  unsigned long tickDelta = metrics.totalTicks - lastTotalTicks;
  unsigned long idleDelta = metrics.idleTicks - lastIdleTicks;
  
  if (tickDelta > 0) {
    metrics.cpuUtilization = 100 - ((idleDelta * 100) / tickDelta);
  }
  
  lastTotalTicks = metrics.totalTicks;
  lastIdleTicks = metrics.idleTicks;
  
  // 활성 태스크 개수
  metrics.activeTasks = 0;
  for (int i = 0; i < MAX_TASKS; i++) {
    if (taskTable[i].enabled) {
      metrics.activeTasks++;
    }
  }
  
  // 시스템 건강 상태 LED
  bool systemHealthy = true;
  
  // 데드라인 위반 체크
  for (int i = 0; i < MAX_TASKS - 1; i++) {
    if (taskTable[i].deadlineMisses > 0) {
      systemHealthy = false;
      break;
    }
  }
  
  // CPU 과부하 체크
  if (metrics.cpuUtilization > 80) {
    systemHealthy = false;
  }
  
  // 상태 LED 업데이트
  if (systemHealthy && !rtIO.emergencyStop) {
    digitalWrite(STATUS_GREEN_LED, HIGH);
    digitalWrite(STATUS_RED_LED, LOW);
  } else {
    digitalWrite(STATUS_GREEN_LED, LOW);
    digitalWrite(STATUS_RED_LED, HIGH);
  }
}

void taskUserInterface() {
  // 200ms 사용자 인터페이스
  
  // 하트비트 LED
  static bool heartbeatState = false;
  heartbeatState = !heartbeatState;
  digitalWrite(HEARTBEAT_LED_PIN, heartbeatState);
  
  // 버튼 입력 처리
  static uint8_t lastInput = 0;
  uint8_t currentInput = rtIO.digitalInputs;
  uint8_t buttonPressed = (~lastInput) & currentInput;
  
  if (buttonPressed & 0x01) {
    // 버튼 1: 긴급 정지 리셋
    if (rtIO.emergencyStop) {
      rtIO.emergencyStop = false;
      rtIO.systemStatus = 0;
      digitalWrite(STATUS_RED_LED, LOW);
      Serial.println("Emergency stop reset");
    }
  }
  
  lastInput = currentInput;
}

void taskWatchdogService() {
  // 500ms 워치독 서비스
  
  bool allTasksHealthy = true;
  unsigned long currentTime = micros();
  
  // 중요 태스크들의 실행 상태 확인
  for (int i = 0; i < 4; i++) {
    if (taskTable[i].enabled) {
      if ((currentTime - taskTable[i].lastStartTime) > (taskTable[i].periodUs * 2)) {
        allTasksHealthy = false;
        break;
      }
    }
  }
  
  // 시스템이 정상이면 워치독 리셋
  if (allTasksHealthy && !rtIO.emergencyStop) {
    wdt_reset();
  }
}

void taskDiagnostic() {
  // 1000ms 시스템 진단
  
  // 태스크별 성능 분석
  for (int i = 0; i < MAX_TASKS - 1; i++) {
    if (taskTable[i].runCount > 0) {
      unsigned long avgRuntime = taskTable[i].totalRuntime / taskTable[i].runCount;
      
      // 실행 시간이 데드라인의 80%를 넘으면 경고
      if (avgRuntime > (taskTable[i].deadlineUs * 80 / 100)) {
        rtIO.systemStatus |= (1 << (8 + i));
      }
    }
  }
  
  // 메모리 사용량 체크 (Arduino에서는 제한적)
  extern int __heap_start, *__brkval;
  int v;
  int freeMemory = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
  
  if (freeMemory < 200) {  // 200바이트 미만이면 경고
    rtIO.systemStatus |= 0x8000;  // 메모리 부족 플래그
  }
}

void taskIdle() {
  // 유휴 태스크 - CPU 절약
  
  // CPU 과부하 시 비중요 태스크 정지
  if (metrics.cpuUtilization > 90) {
    taskTable[4].enabled = false;  // UI 태스크 정지
    taskTable[6].enabled = false;  // 진단 태스크 정지
  } else if (metrics.cpuUtilization < 70) {
    taskTable[4].enabled = true;   // 태스크 재활성화
    taskTable[6].enabled = true;
  }
}

void systemInitialization() {
  Serial.println("System initialization...");
  
  // 초기화 시퀀스 표시
  for (int i = 0; i < 3; i++) {
    digitalWrite(STATUS_GREEN_LED, HIGH);
    digitalWrite(STATUS_RED_LED, HIGH);
    delay(200);
    digitalWrite(STATUS_GREEN_LED, LOW);
    digitalWrite(STATUS_RED_LED, LOW);
    delay(200);
  }
  
  digitalWrite(STATUS_GREEN_LED, HIGH);
  Serial.println("System ready!");
}

void handleSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readString();
    command.trim();
    
    if (command == "status") {
      printSystemStatus();
    }
    else if (command == "tasks") {
      printTaskStatus();
    }
    else if (command == "metrics") {
      printPerformanceMetrics();
    }
    else if (command == "reset") {
      resetStatistics();
      Serial.println("Statistics reset");
    }
    else if (command == "emergency") {
      rtIO.emergencyStop = !rtIO.emergencyStop;
      Serial.print("Emergency stop: ");
      Serial.println(rtIO.emergencyStop ? "ON" : "OFF");
    }
    else if (command == "help") {
      Serial.println("Commands: status, tasks, metrics, reset, emergency, help");
    }
    else {
      Serial.println("Unknown command. Type 'help' for available commands.");
    }
  }
}

void printSystemStatus() {
  Serial.println("=== System Status ===");
  Serial.print("Emergency Stop: ");
  Serial.println(rtIO.emergencyStop ? "ACTIVE" : "Normal");
  Serial.print("CPU Utilization: ");
  Serial.print(metrics.cpuUtilization);
  Serial.println("%");
  Serial.print("Active Tasks: ");
  Serial.println(metrics.activeTasks);
  Serial.print("Max Response Time: ");
  Serial.print(metrics.maxResponseTimeUs);
  Serial.println(" μs");
  Serial.print("System Status: 0x");
  Serial.println(rtIO.systemStatus, HEX);
  Serial.println("====================");
}

void printTaskStatus() {
  Serial.println("=== Task Status ===");
  for (int i = 0; i < MAX_TASKS; i++) {
    Serial.print(taskTable[i].name);
    Serial.print(": ");
    Serial.print(taskTable[i].enabled ? "Enabled" : "Disabled");
    Serial.print(", Runs: ");
    Serial.print(taskTable[i].runCount);
    Serial.print(", Misses: ");
    Serial.print(taskTable[i].deadlineMisses);
    Serial.print(", Worst: ");
    Serial.print(taskTable[i].worstCaseTime);
    Serial.println(" μs");
  }
  Serial.println("==================");
}

void printPerformanceMetrics() {
  Serial.println("=== Performance Metrics ===");
  Serial.print("Total Ticks: ");
  Serial.println(metrics.totalTicks);
  Serial.print("Idle Ticks: ");
  Serial.println(metrics.idleTicks);
  Serial.print("Scheduler Overhead: ");
  Serial.println(metrics.schedulerOverhead);
  Serial.print("Max Response Time: ");
  Serial.print(metrics.maxResponseTimeUs);
  Serial.println(" μs");
  
  // 메모리 정보
  extern int __heap_start, *__brkval;
  int v;
  int freeMemory = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
  Serial.print("Free Memory: ");
  Serial.print(freeMemory);
  Serial.println(" bytes");
  Serial.println("===========================");
}

void resetStatistics() {
  for (int i = 0; i < MAX_TASKS; i++) {
    taskTable[i].runCount = 0;
    taskTable[i].deadlineMisses = 0;
    taskTable[i].totalRuntime = 0;
    taskTable[i].worstCaseTime = 0;
  }
  
  metrics.totalTicks = 0;
  metrics.idleTicks = 0;
  metrics.schedulerOverhead = 0;
  metrics.maxResponseTimeUs = 0;
  
  rtIO.systemStatus = 0;
}

void handleEmergencyRecovery() {
  static unsigned long lastEmergencyTime = 0;
  
  if (rtIO.emergencyStop) {
    // 긴급 정지 상태에서는 모든 제어 출력 정지
    rtIO.digitalOutputs = 0;
    for (int i = 0; i < 4; i++) {
      digitalWrite(CONTROL_OUTPUT_BASE + i, LOW);
    }
    lastEmergencyTime = millis();
  }
}

void checkSystemHealth() {
  // 시스템 건강 상태 종합 체크
  static unsigned long lastHealthCheck = 0;
  
  if (millis() - lastHealthCheck > 1000) {  // 1초마다
    lastHealthCheck = millis();
    
    bool criticalError = false;
    
    // 중요 태스크 데드라인 위반 체크
    if (taskTable[0].deadlineMisses > 0 || taskTable[1].deadlineMisses > 0) {
      criticalError = true;
    }
    
    // CPU 과부하 지속 체크
    if (metrics.cpuUtilization > 95) {
      criticalError = true;
    }
    
    // 메모리 부족 체크
    if (rtIO.systemStatus & 0x8000) {
      criticalError = true;
    }
    
    if (criticalError) {
      // 비상 모드: 비중요 태스크 정지
      taskTable[2].enabled = false;  // 통신
      taskTable[4].enabled = false;  // UI
      taskTable[6].enabled = false;  // 진단
      
      Serial.println("CRITICAL: System in emergency mode");
    }
  }
}