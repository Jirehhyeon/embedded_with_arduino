/*
 * Real-time Performance Monitor for Embedded Systems
 * Description: Advanced performance monitoring and optimization toolkit
 * Author: Embedded Systems Framework
 * Version: 1.0
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

// ================================
// CONFIGURATION
// ================================

#define MONITOR_UART_BAUD 115200
#define SAMPLE_BUFFER_SIZE 64
#define PERFORMANCE_WINDOW 1000  // ms
#define MAX_TASKS 8
#define STACK_CANARY 0xDEADBEEF

// Performance monitoring levels
typedef enum {
    PERF_LEVEL_MINIMAL = 0,
    PERF_LEVEL_BASIC = 1,
    PERF_LEVEL_DETAILED = 2,
    PERF_LEVEL_COMPREHENSIVE = 3
} perf_level_t;

// Task states for monitoring
typedef enum {
    TASK_IDLE = 0,
    TASK_RUNNING = 1,
    TASK_WAITING = 2,
    TASK_SUSPENDED = 3
} task_state_t;

// ================================
// DATA STRUCTURES
// ================================

// Performance sample
typedef struct {
    uint32_t timestamp;
    uint16_t cpu_usage;      // Percentage * 100
    uint16_t memory_usage;   // Bytes
    uint16_t free_stack;     // Bytes
    uint8_t task_count;
    uint8_t interrupt_load;  // Percentage
} performance_sample_t;

// Task performance metrics
typedef struct {
    char name[16];
    task_state_t state;
    uint32_t run_time;       // Total run time in microseconds
    uint32_t last_start;     // Last execution start time
    uint16_t stack_usage;    // Maximum stack usage
    uint16_t priority;
    uint8_t cpu_percent;     // CPU usage percentage
    bool active;
} task_metrics_t;

// Memory pool information
typedef struct {
    uint16_t total_size;
    uint16_t used_size;
    uint16_t free_size;
    uint16_t largest_free;
    uint16_t fragment_count;
} memory_stats_t;

// Interrupt statistics
typedef struct {
    uint32_t count;
    uint32_t total_time;
    uint32_t max_time;
    uint32_t min_time;
    bool enabled;
} interrupt_stats_t;

// System health metrics
typedef struct {
    uint16_t voltage_mv;     // System voltage in mV
    int8_t temperature_c;    // Temperature in Celsius
    uint32_t uptime_seconds;
    uint32_t total_cycles;
    uint16_t watchdog_resets;
    uint8_t brown_out_count;
} health_metrics_t;

// ================================
// GLOBAL VARIABLES
// ================================

static perf_level_t monitoring_level = PERF_LEVEL_BASIC;
static bool monitoring_enabled = true;

// Performance data buffers
static performance_sample_t sample_buffer[SAMPLE_BUFFER_SIZE];
static volatile uint8_t buffer_head = 0;
static volatile uint8_t buffer_tail = 0;

// Task monitoring
static task_metrics_t tasks[MAX_TASKS];
static uint8_t task_count = 0;
static volatile uint8_t current_task = 0;

// System metrics
static memory_stats_t memory_stats = {0};
static interrupt_stats_t interrupt_stats[8] = {0}; // 8 interrupt vectors
static health_metrics_t health_metrics = {0};

// Timing
static volatile uint32_t system_ticks = 0;
static uint32_t last_sample_time = 0;

// ================================
// TIMER SETUP FOR MONITORING
// ================================

void performance_timer_init(void) {
    // Configure Timer1 for 1ms interrupts
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // CTC mode, prescaler 64
    OCR1A = (F_CPU / 64 / 1000) - 1; // 1ms interval
    TIMSK1 |= (1 << OCIE1A);
}

ISR(TIMER1_COMPA_vect) {
    system_ticks++;
    
    // Update performance monitoring every 100ms
    if (system_ticks % 100 == 0) {
        if (monitoring_enabled) {
            sample_performance();
        }
    }
    
    // Update task timing
    if (current_task < task_count && tasks[current_task].active) {
        tasks[current_task].run_time++;
    }
}

// ================================
// UART FOR MONITORING OUTPUT
// ================================

void monitor_uart_init(void) {
    uint16_t ubrr = F_CPU / 16 / MONITOR_UART_BAUD - 1;
    
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void monitor_uart_putchar(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

void monitor_uart_puts(const char* str) {
    while (*str) {
        if (*str == '\n') {
            monitor_uart_putchar('\r');
        }
        monitor_uart_putchar(*str++);
    }
}

void monitor_printf(const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    monitor_uart_puts(buffer);
}

// ================================
// PERFORMANCE MONITORING CORE
// ================================

void performance_monitor_init(void) {
    monitor_uart_init();
    performance_timer_init();
    
    // Initialize buffers
    memset(sample_buffer, 0, sizeof(sample_buffer));
    memset(tasks, 0, sizeof(tasks));
    
    // Initialize health metrics
    health_metrics.uptime_seconds = 0;
    health_metrics.voltage_mv = 5000; // Assume 5V
    health_metrics.temperature_c = 25; // Room temperature
    
    sei(); // Enable interrupts
    
    monitor_printf("Performance Monitor v1.0 initialized\n");
    monitor_printf("Monitoring level: %d\n", monitoring_level);
}

void set_monitoring_level(perf_level_t level) {
    monitoring_level = level;
    monitor_printf("Monitoring level changed to: %d\n", level);
}

void sample_performance(void) {
    performance_sample_t sample;
    
    // Get current timestamp
    sample.timestamp = system_ticks;
    
    // Calculate CPU usage
    sample.cpu_usage = calculate_cpu_usage();
    
    // Get memory usage
    update_memory_stats();
    sample.memory_usage = memory_stats.used_size;
    
    // Calculate free stack
    sample.free_stack = calculate_free_stack();
    
    // Get task count
    sample.task_count = count_active_tasks();
    
    // Calculate interrupt load
    sample.interrupt_load = calculate_interrupt_load();
    
    // Store sample in circular buffer
    uint8_t next_head = (buffer_head + 1) % SAMPLE_BUFFER_SIZE;
    if (next_head != buffer_tail) {
        sample_buffer[buffer_head] = sample;
        buffer_head = next_head;
    } else {
        // Buffer full, advance tail
        buffer_tail = (buffer_tail + 1) % SAMPLE_BUFFER_SIZE;
        sample_buffer[buffer_head] = sample;
        buffer_head = next_head;
    }
    
    last_sample_time = sample.timestamp;
}

uint16_t calculate_cpu_usage(void) {
    static uint32_t last_idle_time = 0;
    static uint32_t last_total_time = 0;
    
    uint32_t current_time = system_ticks;
    uint32_t idle_time = get_idle_time();
    
    uint32_t time_delta = current_time - last_total_time;
    uint32_t idle_delta = idle_time - last_idle_time;
    
    uint16_t cpu_usage = 0;
    if (time_delta > 0) {
        cpu_usage = ((time_delta - idle_delta) * 10000) / time_delta;
    }
    
    last_total_time = current_time;
    last_idle_time = idle_time;
    
    return cpu_usage; // Percentage * 100
}

uint32_t get_idle_time(void) {
    // Calculate time spent in idle task
    for (uint8_t i = 0; i < task_count; i++) {
        if (strcmp(tasks[i].name, "IDLE") == 0) {
            return tasks[i].run_time;
        }
    }
    return 0;
}

uint16_t calculate_free_stack(void) {
    uint8_t* stack_ptr = (uint8_t*)SP;
    uint8_t* stack_bottom = (uint8_t*)RAMEND;
    return stack_bottom - stack_ptr;
}

uint8_t count_active_tasks(void) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < task_count; i++) {
        if (tasks[i].active) {
            count++;
        }
    }
    return count;
}

uint8_t calculate_interrupt_load(void) {
    uint32_t total_interrupt_time = 0;
    
    for (uint8_t i = 0; i < 8; i++) {
        if (interrupt_stats[i].enabled) {
            total_interrupt_time += interrupt_stats[i].total_time;
        }
    }
    
    uint32_t total_time = system_ticks * 1000; // Convert to microseconds
    
    if (total_time > 0) {
        return (total_interrupt_time * 100) / total_time;
    }
    return 0;
}

// ================================
// TASK MONITORING
// ================================

uint8_t register_task(const char* name, uint16_t priority) {
    if (task_count >= MAX_TASKS) {
        return 0xFF; // Error: too many tasks
    }
    
    uint8_t task_id = task_count++;
    
    strncpy(tasks[task_id].name, name, sizeof(tasks[task_id].name) - 1);
    tasks[task_id].priority = priority;
    tasks[task_id].state = TASK_IDLE;
    tasks[task_id].run_time = 0;
    tasks[task_id].stack_usage = 0;
    tasks[task_id].cpu_percent = 0;
    tasks[task_id].active = true;
    
    monitor_printf("Task registered: %s (ID: %d, Priority: %d)\n", name, task_id, priority);
    
    return task_id;
}

void task_switch_in(uint8_t task_id) {
    if (task_id < task_count && tasks[task_id].active) {
        tasks[task_id].state = TASK_RUNNING;
        tasks[task_id].last_start = system_ticks * 1000; // Convert to microseconds
        current_task = task_id;
        
        // Measure stack usage
        uint16_t current_stack = calculate_free_stack();
        if (current_stack < tasks[task_id].stack_usage || tasks[task_id].stack_usage == 0) {
            tasks[task_id].stack_usage = current_stack;
        }
    }
}

void task_switch_out(uint8_t task_id) {
    if (task_id < task_count && tasks[task_id].active) {
        tasks[task_id].state = TASK_WAITING;
        
        uint32_t execution_time = (system_ticks * 1000) - tasks[task_id].last_start;
        tasks[task_id].run_time += execution_time;
    }
}

void task_suspend(uint8_t task_id) {
    if (task_id < task_count) {
        tasks[task_id].state = TASK_SUSPENDED;
        tasks[task_id].active = false;
    }
}

void task_resume(uint8_t task_id) {
    if (task_id < task_count) {
        tasks[task_id].state = TASK_WAITING;
        tasks[task_id].active = true;
    }
}

// ================================
// MEMORY MONITORING
// ================================

void update_memory_stats(void) {
    // Calculate stack usage
    uint16_t stack_usage = (uint8_t*)RAMEND - (uint8_t*)SP;
    
    // Estimate heap usage (simplified)
    extern char __heap_start;
    extern char *__brkval;
    
    uint16_t heap_usage = 0;
    if (__brkval) {
        heap_usage = (char*)__brkval - &__heap_start;
    }
    
    memory_stats.total_size = RAMSIZE;
    memory_stats.used_size = stack_usage + heap_usage;
    memory_stats.free_size = memory_stats.total_size - memory_stats.used_size;
    
    // Calculate largest free block (simplified)
    memory_stats.largest_free = memory_stats.free_size;
    memory_stats.fragment_count = 1; // Simplified
}

void check_memory_corruption(void) {
    // Check stack canary
    uint32_t* canary_ptr = (uint32_t*)(RAMEND - sizeof(uint32_t));
    if (*canary_ptr != STACK_CANARY) {
        monitor_printf("CRITICAL: Stack corruption detected!\n");
        health_metrics.brown_out_count++; // Reuse this field for corruption count
    }
    
    // Check for stack overflow
    if (calculate_free_stack() < 64) {
        monitor_printf("WARNING: Stack space low (%d bytes)\n", calculate_free_stack());
    }
}

// ================================
// INTERRUPT MONITORING
// ================================

void interrupt_enter(uint8_t vector) {
    if (vector < 8 && interrupt_stats[vector].enabled) {
        interrupt_stats[vector].count++;
        // Start timing (would need hardware timer support)
    }
}

void interrupt_exit(uint8_t vector, uint32_t execution_time) {
    if (vector < 8 && interrupt_stats[vector].enabled) {
        interrupt_stats[vector].total_time += execution_time;
        
        if (execution_time > interrupt_stats[vector].max_time) {
            interrupt_stats[vector].max_time = execution_time;
        }
        
        if (execution_time < interrupt_stats[vector].min_time || interrupt_stats[vector].min_time == 0) {
            interrupt_stats[vector].min_time = execution_time;
        }
    }
}

void enable_interrupt_monitoring(uint8_t vector) {
    if (vector < 8) {
        interrupt_stats[vector].enabled = true;
        monitor_printf("Interrupt monitoring enabled for vector %d\n", vector);
    }
}

// ================================
// HEALTH MONITORING
// ================================

void update_health_metrics(void) {
    // Update uptime
    health_metrics.uptime_seconds = system_ticks / 1000;
    
    // Read system voltage (ADC channel for voltage divider)
    uint16_t adc_value = read_system_voltage_adc();
    health_metrics.voltage_mv = (adc_value * 5000UL) / 1023;
    
    // Read temperature (internal temperature sensor if available)
    health_metrics.temperature_c = read_internal_temperature();
    
    // Update cycle counter
    health_metrics.total_cycles += 1000; // Approximation
}

uint16_t read_system_voltage_adc(void) {
    // Configure ADC for internal 1.1V reference
    ADMUX = (1 << REFS1) | (1 << REFS0) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    
    // Start conversion
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    
    uint16_t result = ADC;
    
    // Convert to actual voltage (VCC = 1.1V * 1024 / ADC_result)
    return (1100UL * 1024) / result;
}

int8_t read_internal_temperature(void) {
    // ATmega328P doesn't have internal temperature sensor
    // This would interface with external sensor
    return 25; // Default room temperature
}

void check_system_health(void) {
    update_health_metrics();
    
    // Check voltage levels
    if (health_metrics.voltage_mv < 4500) {
        monitor_printf("WARNING: Low voltage detected (%dmV)\n", health_metrics.voltage_mv);
    }
    
    // Check temperature
    if (health_metrics.temperature_c > 60) {
        monitor_printf("WARNING: High temperature (%d°C)\n", health_metrics.temperature_c);
    }
    
    // Check for brownout conditions
    if (MCUSR & (1 << BORF)) {
        health_metrics.brown_out_count++;
        MCUSR &= ~(1 << BORF);
        monitor_printf("Brownout reset detected (count: %d)\n", health_metrics.brown_out_count);
    }
    
    // Check for watchdog resets
    if (MCUSR & (1 << WDRF)) {
        health_metrics.watchdog_resets++;
        MCUSR &= ~(1 << WDRF);
        monitor_printf("Watchdog reset detected (count: %d)\n", health_metrics.watchdog_resets);
    }
}

// ================================
// REPORTING AND OUTPUT
// ================================

void print_performance_summary(void) {
    monitor_printf("\n=== Performance Summary ===\n");
    monitor_printf("Uptime: %lu seconds\n", health_metrics.uptime_seconds);
    monitor_printf("CPU Usage: %d.%02d%%\n", 
                  sample_buffer[buffer_head-1].cpu_usage / 100,
                  sample_buffer[buffer_head-1].cpu_usage % 100);
    monitor_printf("Memory Used: %d/%d bytes (%d%% free)\n",
                  memory_stats.used_size,
                  memory_stats.total_size,
                  (memory_stats.free_size * 100) / memory_stats.total_size);
    monitor_printf("Free Stack: %d bytes\n", calculate_free_stack());
    monitor_printf("Active Tasks: %d\n", count_active_tasks());
    monitor_printf("System Voltage: %dmV\n", health_metrics.voltage_mv);
    monitor_printf("Temperature: %d°C\n", health_metrics.temperature_c);
}

void print_task_summary(void) {
    monitor_printf("\n=== Task Summary ===\n");
    monitor_printf("ID  Name            State   CPU%%    Runtime(ms)  Stack\n");
    monitor_printf("--- --------------- ------- ------- ----------- -----\n");
    
    for (uint8_t i = 0; i < task_count; i++) {
        if (tasks[i].active) {
            const char* state_str = "Unknown";
            switch (tasks[i].state) {
                case TASK_IDLE: state_str = "Idle"; break;
                case TASK_RUNNING: state_str = "Running"; break;
                case TASK_WAITING: state_str = "Waiting"; break;
                case TASK_SUSPENDED: state_str = "Suspend"; break;
            }
            
            uint32_t runtime_ms = tasks[i].run_time / 1000;
            
            monitor_printf("%2d  %-15s %-7s %6d%% %10lu %5d\n",
                          i,
                          tasks[i].name,
                          state_str,
                          tasks[i].cpu_percent,
                          runtime_ms,
                          tasks[i].stack_usage);
        }
    }
}

void print_memory_summary(void) {
    monitor_printf("\n=== Memory Summary ===\n");
    monitor_printf("Total RAM: %d bytes\n", memory_stats.total_size);
    monitor_printf("Used: %d bytes (%d%%)\n", 
                  memory_stats.used_size,
                  (memory_stats.used_size * 100) / memory_stats.total_size);
    monitor_printf("Free: %d bytes (%d%%)\n",
                  memory_stats.free_size,
                  (memory_stats.free_size * 100) / memory_stats.total_size);
    monitor_printf("Largest free block: %d bytes\n", memory_stats.largest_free);
    monitor_printf("Fragmentation: %d blocks\n", memory_stats.fragment_count);
}

void print_interrupt_summary(void) {
    monitor_printf("\n=== Interrupt Summary ===\n");
    monitor_printf("Vec  Count      Total(us)  Avg(us)  Max(us)  Min(us)\n");
    monitor_printf("---  ---------  ---------  -------  -------  -------\n");
    
    for (uint8_t i = 0; i < 8; i++) {
        if (interrupt_stats[i].enabled && interrupt_stats[i].count > 0) {
            uint32_t avg_time = interrupt_stats[i].total_time / interrupt_stats[i].count;
            
            monitor_printf("%2d   %9lu  %9lu  %7lu  %7lu  %7lu\n",
                          i,
                          interrupt_stats[i].count,
                          interrupt_stats[i].total_time,
                          avg_time,
                          interrupt_stats[i].max_time,
                          interrupt_stats[i].min_time);
        }
    }
}

void print_full_report(void) {
    check_system_health();
    check_memory_corruption();
    
    print_performance_summary();
    
    if (monitoring_level >= PERF_LEVEL_BASIC) {
        print_task_summary();
        print_memory_summary();
    }
    
    if (monitoring_level >= PERF_LEVEL_DETAILED) {
        print_interrupt_summary();
    }
    
    if (monitoring_level >= PERF_LEVEL_COMPREHENSIVE) {
        print_historical_data();
    }
}

void print_historical_data(void) {
    monitor_printf("\n=== Historical Performance Data ===\n");
    monitor_printf("Time(s)  CPU%%    Memory  Stack  Tasks  IntLoad\n");
    monitor_printf("-------  ------  ------  -----  -----  -------\n");
    
    uint8_t index = buffer_tail;
    uint8_t count = 0;
    
    while (index != buffer_head && count < 10) {
        performance_sample_t* sample = &sample_buffer[index];
        
        monitor_printf("%7lu  %3d.%02d  %6d  %5d  %5d  %6d%%\n",
                      sample->timestamp / 1000,
                      sample->cpu_usage / 100,
                      sample->cpu_usage % 100,
                      sample->memory_usage,
                      sample->free_stack,
                      sample->task_count,
                      sample->interrupt_load);
        
        index = (index + 1) % SAMPLE_BUFFER_SIZE;
        count++;
    }
}

// ================================
// COMMAND INTERFACE
// ================================

void process_monitor_command(const char* command) {
    if (strcmp(command, "summary") == 0) {
        print_performance_summary();
    } else if (strcmp(command, "tasks") == 0) {
        print_task_summary();
    } else if (strcmp(command, "memory") == 0) {
        print_memory_summary();
    } else if (strcmp(command, "interrupts") == 0) {
        print_interrupt_summary();
    } else if (strcmp(command, "report") == 0) {
        print_full_report();
    } else if (strcmp(command, "history") == 0) {
        print_historical_data();
    } else if (strncmp(command, "level ", 6) == 0) {
        int level = atoi(command + 6);
        if (level >= 0 && level <= 3) {
            set_monitoring_level((perf_level_t)level);
        }
    } else if (strcmp(command, "enable") == 0) {
        monitoring_enabled = true;
        monitor_printf("Performance monitoring enabled\n");
    } else if (strcmp(command, "disable") == 0) {
        monitoring_enabled = false;
        monitor_printf("Performance monitoring disabled\n");
    } else {
        monitor_printf("Unknown command. Available: summary, tasks, memory, interrupts, report, history, level N, enable, disable\n");
    }
}

// ================================
// EXAMPLE USAGE
// ================================

/*
int main(void) {
    // Initialize performance monitoring
    performance_monitor_init();
    
    // Register tasks
    uint8_t main_task = register_task("MAIN", 1);
    uint8_t sensor_task = register_task("SENSOR", 2);
    uint8_t comm_task = register_task("COMM", 3);
    
    // Enable interrupt monitoring
    enable_interrupt_monitoring(0); // Timer0
    enable_interrupt_monitoring(1); // Timer1
    
    // Set monitoring level
    set_monitoring_level(PERF_LEVEL_DETAILED);
    
    // Main application loop
    while (1) {
        task_switch_in(main_task);
        
        // Your main application code here
        _delay_ms(100);
        
        // Simulate task switching
        task_switch_out(main_task);
        
        // Process monitoring commands
        char command[32];
        if (read_monitor_command(command, sizeof(command))) {
            process_monitor_command(command);
        }
        
        // Print periodic report
        static uint32_t last_report = 0;
        if (system_ticks - last_report > 10000) { // Every 10 seconds
            print_performance_summary();
            last_report = system_ticks;
        }
    }
    
    return 0;
}
*/