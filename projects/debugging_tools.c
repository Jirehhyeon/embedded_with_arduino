/*
 * Advanced Debugging Tools for Embedded Systems
 * Description: Comprehensive debugging utilities for embedded development
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

#define DEBUG_UART_BAUD 115200
#define DEBUG_BUFFER_SIZE 256
#define TRACE_BUFFER_SIZE 512
#define BREAKPOINT_MAX 8
#define WATCHPOINT_MAX 4
#define PROFILER_FUNCTIONS 16

// Debug levels
typedef enum {
    DEBUG_NONE = 0,
    DEBUG_ERROR = 1,
    DEBUG_WARN = 2,
    DEBUG_INFO = 3,
    DEBUG_VERBOSE = 4,
    DEBUG_TRACE = 5
} debug_level_t;

// ================================
// DATA STRUCTURES
// ================================

// Breakpoint structure
typedef struct {
    uint16_t address;
    bool enabled;
    bool hit;
    uint16_t hit_count;
    char condition[32];
} breakpoint_t;

// Watchpoint structure
typedef struct {
    volatile uint8_t* address;
    uint8_t old_value;
    bool write_watch;
    bool read_watch;
    bool enabled;
    uint16_t trigger_count;
} watchpoint_t;

// Function profiler entry
typedef struct {
    const char* name;
    uint32_t call_count;
    uint32_t total_cycles;
    uint32_t min_cycles;
    uint32_t max_cycles;
    uint32_t last_entry;
} profiler_entry_t;

// Stack analyzer
typedef struct {
    uint16_t max_usage;
    uint16_t current_usage;
    uint16_t stack_size;
    uint8_t* stack_bottom;
    uint8_t* stack_top;
    uint8_t canary_value;
} stack_info_t;

// Memory dump region
typedef struct {
    uint16_t start_address;
    uint16_t length;
    const char* label;
} memory_region_t;

// ================================
// GLOBAL VARIABLES
// ================================

static debug_level_t current_debug_level = DEBUG_INFO;
static char debug_buffer[DEBUG_BUFFER_SIZE];
static volatile bool debug_enabled = true;

// Debugging structures
static breakpoint_t breakpoints[BREAKPOINT_MAX];
static watchpoint_t watchpoints[WATCHPOINT_MAX];
static profiler_entry_t profiler[PROFILER_FUNCTIONS];
static stack_info_t stack_info;

// Trace buffer
static struct {
    char buffer[TRACE_BUFFER_SIZE];
    uint16_t write_index;
    uint16_t read_index;
    bool overflow;
} trace_buffer = {0};

// ================================
// UART DEBUG OUTPUT
// ================================

void debug_uart_init(void) {
    uint16_t ubrr = F_CPU / 16 / DEBUG_UART_BAUD - 1;
    
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void debug_uart_putchar(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

void debug_uart_puts(const char* str) {
    while (*str) {
        if (*str == '\n') {
            debug_uart_putchar('\r');
        }
        debug_uart_putchar(*str++);
    }
}

void debug_uart_puts_P(const char* str) {
    char c;
    while ((c = pgm_read_byte(str++))) {
        if (c == '\n') {
            debug_uart_putchar('\r');
        }
        debug_uart_putchar(c);
    }
}

// ================================
// DEBUG LOGGING
// ================================

void debug_init(void) {
    debug_uart_init();
    
    // Initialize debugging structures
    memset(breakpoints, 0, sizeof(breakpoints));
    memset(watchpoints, 0, sizeof(watchpoints));
    memset(profiler, 0, sizeof(profiler));
    
    // Initialize stack monitoring
    stack_info.stack_bottom = (uint8_t*)RAMEND;
    stack_info.stack_top = (uint8_t*)RAMEND - 512;  // Assume 512 byte stack
    stack_info.stack_size = 512;
    stack_info.canary_value = 0xAA;
    
    // Fill stack with canary pattern
    for (uint8_t* p = stack_info.stack_top; p < stack_info.stack_bottom; p++) {
        *p = stack_info.canary_value;
    }
    
    debug_uart_puts_P(PSTR("\n=== Debug System Initialized ===\n"));
}

void debug_set_level(debug_level_t level) {
    current_debug_level = level;
}

void debug_log(debug_level_t level, const char* fmt, ...) {
    if (!debug_enabled || level > current_debug_level) {
        return;
    }
    
    // Add timestamp
    uint32_t millis = 0;  // Would use system timer
    snprintf(debug_buffer, sizeof(debug_buffer), "[%lu.%03lu] ", 
             millis / 1000, millis % 1000);
    
    // Add level indicator
    switch (level) {
        case DEBUG_ERROR:   strcat(debug_buffer, "ERROR: "); break;
        case DEBUG_WARN:    strcat(debug_buffer, "WARN:  "); break;
        case DEBUG_INFO:    strcat(debug_buffer, "INFO:  "); break;
        case DEBUG_VERBOSE: strcat(debug_buffer, "DEBUG: "); break;
        case DEBUG_TRACE:   strcat(debug_buffer, "TRACE: "); break;
        default: break;
    }
    
    // Format message
    va_list args;
    va_start(args, fmt);
    vsnprintf(debug_buffer + strlen(debug_buffer), 
              sizeof(debug_buffer) - strlen(debug_buffer), fmt, args);
    va_end(args);
    
    // Output
    debug_uart_puts(debug_buffer);
    debug_uart_putchar('\n');
    
    // Add to trace buffer
    trace_add(debug_buffer);
}

// ================================
// BREAKPOINT SYSTEM
// ================================

int8_t breakpoint_set(uint16_t address, const char* condition) {
    for (uint8_t i = 0; i < BREAKPOINT_MAX; i++) {
        if (!breakpoints[i].enabled) {
            breakpoints[i].address = address;
            breakpoints[i].enabled = true;
            breakpoints[i].hit = false;
            breakpoints[i].hit_count = 0;
            
            if (condition) {
                strncpy(breakpoints[i].condition, condition, 
                        sizeof(breakpoints[i].condition) - 1);
            }
            
            debug_log(DEBUG_INFO, "Breakpoint %d set at 0x%04X", i, address);
            return i;
        }
    }
    return -1;  // No free breakpoint
}

void breakpoint_clear(uint8_t id) {
    if (id < BREAKPOINT_MAX) {
        breakpoints[id].enabled = false;
        debug_log(DEBUG_INFO, "Breakpoint %d cleared", id);
    }
}

void breakpoint_check(uint16_t pc) {
    for (uint8_t i = 0; i < BREAKPOINT_MAX; i++) {
        if (breakpoints[i].enabled && breakpoints[i].address == pc) {
            breakpoints[i].hit = true;
            breakpoints[i].hit_count++;
            
            debug_log(DEBUG_WARN, "BREAKPOINT %d hit at 0x%04X (count: %d)",
                     i, pc, breakpoints[i].hit_count);
            
            // Could trigger debugger or halt here
            debug_dump_registers();
            debug_dump_stack(8);
        }
    }
}

// ================================
// WATCHPOINT SYSTEM
// ================================

int8_t watchpoint_set(volatile uint8_t* address, bool write, bool read) {
    for (uint8_t i = 0; i < WATCHPOINT_MAX; i++) {
        if (!watchpoints[i].enabled) {
            watchpoints[i].address = address;
            watchpoints[i].write_watch = write;
            watchpoints[i].read_watch = read;
            watchpoints[i].enabled = true;
            watchpoints[i].trigger_count = 0;
            watchpoints[i].old_value = *address;
            
            debug_log(DEBUG_INFO, "Watchpoint %d set at 0x%04X (%s%s)",
                     i, (uint16_t)address,
                     write ? "W" : "", read ? "R" : "");
            return i;
        }
    }
    return -1;
}

void watchpoint_clear(uint8_t id) {
    if (id < WATCHPOINT_MAX) {
        watchpoints[id].enabled = false;
        debug_log(DEBUG_INFO, "Watchpoint %d cleared", id);
    }
}

void watchpoint_check(void) {
    for (uint8_t i = 0; i < WATCHPOINT_MAX; i++) {
        if (watchpoints[i].enabled) {
            uint8_t current_value = *watchpoints[i].address;
            
            if (watchpoints[i].write_watch && 
                current_value != watchpoints[i].old_value) {
                watchpoints[i].trigger_count++;
                
                debug_log(DEBUG_WARN, 
                         "WATCHPOINT %d: Write to 0x%04X (0x%02X -> 0x%02X)",
                         i, (uint16_t)watchpoints[i].address,
                         watchpoints[i].old_value, current_value);
                
                watchpoints[i].old_value = current_value;
            }
        }
    }
}

// ================================
// PROFILER SYSTEM
// ================================

int8_t profiler_register(const char* function_name) {
    for (uint8_t i = 0; i < PROFILER_FUNCTIONS; i++) {
        if (profiler[i].name == NULL) {
            profiler[i].name = function_name;
            profiler[i].call_count = 0;
            profiler[i].total_cycles = 0;
            profiler[i].min_cycles = 0xFFFFFFFF;
            profiler[i].max_cycles = 0;
            return i;
        }
    }
    return -1;
}

void profiler_enter(uint8_t function_id) {
    if (function_id < PROFILER_FUNCTIONS && profiler[function_id].name) {
        profiler[function_id].last_entry = TCNT1;  // Use Timer1 for cycle counting
    }
}

void profiler_exit(uint8_t function_id) {
    if (function_id < PROFILER_FUNCTIONS && profiler[function_id].name) {
        uint32_t cycles = TCNT1 - profiler[function_id].last_entry;
        
        profiler[function_id].call_count++;
        profiler[function_id].total_cycles += cycles;
        
        if (cycles < profiler[function_id].min_cycles) {
            profiler[function_id].min_cycles = cycles;
        }
        if (cycles > profiler[function_id].max_cycles) {
            profiler[function_id].max_cycles = cycles;
        }
    }
}

void profiler_report(void) {
    debug_uart_puts_P(PSTR("\n=== Profiler Report ===\n"));
    debug_uart_puts_P(PSTR("Function            Calls    Avg     Min     Max\n"));
    debug_uart_puts_P(PSTR("------------------------------------------------\n"));
    
    for (uint8_t i = 0; i < PROFILER_FUNCTIONS; i++) {
        if (profiler[i].name && profiler[i].call_count > 0) {
            uint32_t avg = profiler[i].total_cycles / profiler[i].call_count;
            
            snprintf(debug_buffer, sizeof(debug_buffer),
                    "%-16s %6lu %6lu %6lu %6lu\n",
                    profiler[i].name,
                    profiler[i].call_count,
                    avg,
                    profiler[i].min_cycles,
                    profiler[i].max_cycles);
            
            debug_uart_puts(debug_buffer);
        }
    }
}

// ================================
// STACK ANALYSIS
// ================================

void stack_check(void) {
    // Find current stack usage
    uint8_t* sp = (uint8_t*)SP;
    uint16_t current_usage = stack_info.stack_bottom - sp;
    
    if (current_usage > stack_info.max_usage) {
        stack_info.max_usage = current_usage;
    }
    stack_info.current_usage = current_usage;
    
    // Check for stack overflow
    if (sp < stack_info.stack_top) {
        debug_log(DEBUG_ERROR, "STACK OVERFLOW! SP=0x%04X", (uint16_t)sp);
    }
    
    // Check canary
    if (*stack_info.stack_top != stack_info.canary_value) {
        debug_log(DEBUG_ERROR, "STACK CORRUPTION DETECTED!");
    }
}

void stack_report(void) {
    stack_check();
    
    debug_uart_puts_P(PSTR("\n=== Stack Analysis ===\n"));
    snprintf(debug_buffer, sizeof(debug_buffer),
            "Current: %d bytes\nMaximum: %d bytes\nSize:    %d bytes\nUsage:   %d%%\n",
            stack_info.current_usage,
            stack_info.max_usage,
            stack_info.stack_size,
            (stack_info.max_usage * 100) / stack_info.stack_size);
    
    debug_uart_puts(debug_buffer);
}

// ================================
// MEMORY DUMP
// ================================

void debug_dump_memory(uint16_t address, uint16_t length) {
    debug_uart_puts_P(PSTR("\nMemory dump:\n"));
    
    for (uint16_t i = 0; i < length; i += 16) {
        // Address
        snprintf(debug_buffer, sizeof(debug_buffer), "%04X: ", address + i);
        debug_uart_puts(debug_buffer);
        
        // Hex values
        for (uint8_t j = 0; j < 16 && (i + j) < length; j++) {
            uint8_t value = *(uint8_t*)(address + i + j);
            snprintf(debug_buffer, sizeof(debug_buffer), "%02X ", value);
            debug_uart_puts(debug_buffer);
        }
        
        // ASCII representation
        debug_uart_puts("  ");
        for (uint8_t j = 0; j < 16 && (i + j) < length; j++) {
            uint8_t value = *(uint8_t*)(address + i + j);
            if (value >= 32 && value < 127) {
                debug_uart_putchar(value);
            } else {
                debug_uart_putchar('.');
            }
        }
        
        debug_uart_putchar('\n');
    }
}

void debug_dump_registers(void) {
    debug_uart_puts_P(PSTR("\n=== Register Dump ===\n"));
    
    // Note: In real implementation, would need assembly to access registers
    debug_uart_puts_P(PSTR("[Register dump requires assembly implementation]\n"));
}

void debug_dump_stack(uint8_t depth) {
    uint8_t* sp = (uint8_t*)SP;
    
    debug_uart_puts_P(PSTR("\n=== Stack Dump ===\n"));
    snprintf(debug_buffer, sizeof(debug_buffer), "SP: 0x%04X\n", (uint16_t)sp);
    debug_uart_puts(debug_buffer);
    
    for (uint8_t i = 0; i < depth && (sp + i) <= stack_info.stack_bottom; i++) {
        snprintf(debug_buffer, sizeof(debug_buffer), 
                "SP+%d: 0x%04X = 0x%02X\n", 
                i, (uint16_t)(sp + i), *(sp + i));
        debug_uart_puts(debug_buffer);
    }
}

// ================================
// TRACE BUFFER
// ================================

void trace_add(const char* message) {
    uint16_t len = strlen(message);
    
    if (len > TRACE_BUFFER_SIZE - 1) {
        len = TRACE_BUFFER_SIZE - 1;
    }
    
    // Check for overflow
    uint16_t space_needed = len + 1;  // +1 for newline
    uint16_t space_available = TRACE_BUFFER_SIZE - trace_buffer.write_index;
    
    if (space_needed > space_available) {
        trace_buffer.overflow = true;
        trace_buffer.write_index = 0;  // Wrap around
    }
    
    // Copy message
    memcpy(&trace_buffer.buffer[trace_buffer.write_index], message, len);
    trace_buffer.write_index += len;
    trace_buffer.buffer[trace_buffer.write_index++] = '\n';
}

void trace_dump(void) {
    debug_uart_puts_P(PSTR("\n=== Trace Buffer ===\n"));
    
    if (trace_buffer.overflow) {
        debug_uart_puts_P(PSTR("[Buffer overflow - some messages lost]\n"));
    }
    
    // Output entire buffer
    for (uint16_t i = 0; i < trace_buffer.write_index; i++) {
        debug_uart_putchar(trace_buffer.buffer[i]);
    }
}

void trace_clear(void) {
    trace_buffer.write_index = 0;
    trace_buffer.overflow = false;
    memset(trace_buffer.buffer, 0, sizeof(trace_buffer.buffer));
}

// ================================
// ASSERTION SYSTEM
// ================================

void debug_assert_fail(const char* expr, const char* file, int line) {
    cli();  // Disable interrupts
    
    debug_log(DEBUG_ERROR, "ASSERTION FAILED: %s", expr);
    debug_log(DEBUG_ERROR, "File: %s, Line: %d", file, line);
    
    // Dump debugging information
    debug_dump_registers();
    debug_dump_stack(16);
    stack_report();
    
    // Halt system
    while (1) {
        // Flash LED to indicate assertion failure
        PORTB ^= (1 << PB5);
        _delay_ms(100);
    }
}

#define DEBUG_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            debug_assert_fail(#expr, __FILE__, __LINE__); \
        } \
    } while(0)

// ================================
// INTERACTIVE DEBUGGER
// ================================

void debugger_prompt(void) {
    debug_uart_puts_P(PSTR("\ndebug> "));
}

void debugger_help(void) {
    debug_uart_puts_P(PSTR(
        "\nDebugger Commands:\n"
        "  h        - Help\n"
        "  c        - Continue\n"
        "  s        - Step\n"
        "  b <addr> - Set breakpoint\n"
        "  d <id>   - Delete breakpoint\n"
        "  w <addr> - Set watchpoint\n"
        "  m <addr> - Memory dump\n"
        "  r        - Register dump\n"
        "  k        - Stack dump\n"
        "  p        - Profiler report\n"
        "  t        - Trace dump\n"
        "  q        - Quit debugger\n"
    ));
}

void debugger_interactive(void) {
    char cmd[32];
    uint8_t cmd_index = 0;
    
    debug_uart_puts_P(PSTR("\n=== Interactive Debugger ===\n"));
    debugger_help();
    
    while (1) {
        debugger_prompt();
        
        // Read command
        cmd_index = 0;
        while (1) {
            if (UCSR0A & (1 << RXC0)) {
                char c = UDR0;
                
                if (c == '\r' || c == '\n') {
                    cmd[cmd_index] = '\0';
                    debug_uart_putchar('\n');
                    break;
                } else if (c == '\b' && cmd_index > 0) {
                    cmd_index--;
                    debug_uart_puts("\b \b");
                } else if (cmd_index < sizeof(cmd) - 1) {
                    cmd[cmd_index++] = c;
                    debug_uart_putchar(c);
                }
            }
        }
        
        // Process command
        if (cmd[0] == 'h') {
            debugger_help();
        } else if (cmd[0] == 'c') {
            debug_uart_puts_P(PSTR("Continuing...\n"));
            break;
        } else if (cmd[0] == 's') {
            debug_uart_puts_P(PSTR("Step not implemented\n"));
        } else if (cmd[0] == 'b') {
            uint16_t addr;
            if (sscanf(cmd + 2, "%x", &addr) == 1) {
                breakpoint_set(addr, NULL);
            }
        } else if (cmd[0] == 'd') {
            uint8_t id;
            if (sscanf(cmd + 2, "%hhu", &id) == 1) {
                breakpoint_clear(id);
            }
        } else if (cmd[0] == 'm') {
            uint16_t addr;
            if (sscanf(cmd + 2, "%x", &addr) == 1) {
                debug_dump_memory(addr, 64);
            }
        } else if (cmd[0] == 'r') {
            debug_dump_registers();
        } else if (cmd[0] == 'k') {
            debug_dump_stack(16);
        } else if (cmd[0] == 'p') {
            profiler_report();
        } else if (cmd[0] == 't') {
            trace_dump();
        } else if (cmd[0] == 'q') {
            debug_uart_puts_P(PSTR("Exiting debugger\n"));
            break;
        } else if (cmd[0] != '\0') {
            debug_uart_puts_P(PSTR("Unknown command\n"));
        }
    }
}

// ================================
// EXAMPLE USAGE
// ================================

/*
// In your main code:

int main(void) {
    // Initialize debugging
    debug_init();
    debug_set_level(DEBUG_VERBOSE);
    
    // Set up profiling
    uint8_t main_profile = profiler_register("main");
    profiler_enter(main_profile);
    
    // Set a breakpoint
    breakpoint_set(0x1234, NULL);
    
    // Set a watchpoint on PORTB
    watchpoint_set(&PORTB, true, false);
    
    // Your application code
    debug_log(DEBUG_INFO, "System started");
    
    // Use assertions
    DEBUG_ASSERT(PORTB == 0);
    
    // Check debugging
    breakpoint_check(PC);  // Would need real PC value
    watchpoint_check();
    stack_check();
    
    // End profiling
    profiler_exit(main_profile);
    profiler_report();
    
    return 0;
}
*/
