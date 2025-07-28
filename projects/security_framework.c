/*
 * Embedded Security Framework
 * Description: Comprehensive security toolkit for embedded systems
 * Author: Embedded Systems Framework
 * Version: 1.0
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

// ================================
// SECURITY CONFIGURATION
// ================================

#define SECURITY_UART_BAUD 115200
#define MAX_AUTH_ATTEMPTS 3
#define SESSION_TIMEOUT_MS 300000  // 5 minutes
#define CRYPTO_KEY_SIZE 16
#define HASH_SIZE 32
#define NONCE_SIZE 16
#define MAX_SECURITY_LOGS 32
#define SECURE_BOOT_SIGNATURE 0xABCDEF01

// Security levels
typedef enum {
    SECURITY_LEVEL_NONE = 0,
    SECURITY_LEVEL_BASIC = 1,
    SECURITY_LEVEL_ENHANCED = 2,
    SECURITY_LEVEL_PARANOID = 3
} security_level_t;

// Authentication states
typedef enum {
    AUTH_STATE_LOCKED = 0,
    AUTH_STATE_CHALLENGE = 1,
    AUTH_STATE_AUTHENTICATED = 2,
    AUTH_STATE_EXPIRED = 3
} auth_state_t;

// Security event types
typedef enum {
    SEC_EVENT_BOOT = 0,
    SEC_EVENT_AUTH_SUCCESS = 1,
    SEC_EVENT_AUTH_FAIL = 2,
    SEC_EVENT_INTRUSION = 3,
    SEC_EVENT_TAMPER = 4,
    SEC_EVENT_CRYPTO_ERROR = 5,
    SEC_EVENT_MEMORY_VIOLATION = 6,
    SEC_EVENT_WATCHDOG = 7
} security_event_t;

// ================================
// DATA STRUCTURES
// ================================

// Security configuration stored in EEPROM
typedef struct {
    uint32_t signature;
    security_level_t level;
    uint8_t master_key[CRYPTO_KEY_SIZE];
    uint8_t device_id[8];
    uint16_t auth_timeout;
    uint8_t max_auth_attempts;
    bool tamper_detection_enabled;
    bool secure_boot_enabled;
    uint16_t checksum;
} security_config_t;

// Authentication session
typedef struct {
    auth_state_t state;
    uint32_t challenge;
    uint32_t session_start;
    uint8_t failed_attempts;
    uint8_t current_nonce[NONCE_SIZE];
    bool active;
} auth_session_t;

// Security log entry
typedef struct {
    uint32_t timestamp;
    security_event_t event_type;
    uint16_t event_data;
    uint8_t source_module;
    uint8_t severity;
} security_log_entry_t;

// Cryptographic context
typedef struct {
    uint8_t key[CRYPTO_KEY_SIZE];
    uint8_t iv[CRYPTO_KEY_SIZE];
    uint32_t counter;
    bool initialized;
} crypto_context_t;

// Intrusion detection state
typedef struct {
    bool motion_detected;
    bool case_opened;
    bool voltage_anomaly;
    bool frequency_anomaly;
    uint16_t anomaly_count;
    uint32_t last_check;
} intrusion_state_t;

// ================================
// GLOBAL VARIABLES
// ================================

static security_config_t EEMEM eeprom_config;
static security_config_t ram_config;
static auth_session_t current_session;
static crypto_context_t crypto_ctx;
static intrusion_state_t intrusion_state;
static security_log_entry_t security_log[MAX_SECURITY_LOGS];
static uint8_t log_head = 0;
static volatile uint32_t security_ticks = 0;

// ================================
// BASIC CRYPTOGRAPHY
// ================================

// Simple XOR-based encryption (replace with AES in production)
void simple_encrypt(uint8_t* data, uint8_t length, const uint8_t* key) {
    for (uint8_t i = 0; i < length; i++) {
        data[i] ^= key[i % CRYPTO_KEY_SIZE];
    }
}

void simple_decrypt(uint8_t* data, uint8_t length, const uint8_t* key) {
    // XOR is symmetric
    simple_encrypt(data, length, key);
}

// Simple hash function (replace with SHA-256 in production)
uint32_t simple_hash(const uint8_t* data, uint16_t length) {
    uint32_t hash = 0x811c9dc5; // FNV offset basis
    
    for (uint16_t i = 0; i < length; i++) {
        hash ^= data[i];
        hash *= 0x01000193; // FNV prime
    }
    
    return hash;
}

// Generate pseudo-random number
uint32_t secure_random(void) {
    static uint32_t lfsr = 0xACE1u;
    
    // Use timer values for entropy
    lfsr ^= TCNT1;
    lfsr ^= ADC; // If ADC is available
    
    // Linear feedback shift register
    uint32_t bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
    lfsr = (lfsr >> 1) | (bit << 31);
    
    return lfsr;
}

void generate_nonce(uint8_t* nonce, uint8_t length) {
    for (uint8_t i = 0; i < length; i++) {
        nonce[i] = secure_random() & 0xFF;
    }
}

// ================================
// SECURE CONFIGURATION
// ================================

void security_init(void) {
    // Read configuration from EEPROM
    eeprom_read_block(&ram_config, &eeprom_config, sizeof(security_config_t));
    
    // Verify configuration integrity
    uint16_t calculated_checksum = simple_hash((uint8_t*)&ram_config, 
                                             sizeof(security_config_t) - sizeof(uint16_t));
    
    if (ram_config.signature != SECURE_BOOT_SIGNATURE || 
        ram_config.checksum != calculated_checksum) {
        // Configuration corrupted or first boot - initialize defaults
        security_factory_reset();
    }
    
    // Initialize cryptographic context
    memcpy(crypto_ctx.key, ram_config.master_key, CRYPTO_KEY_SIZE);
    generate_nonce(crypto_ctx.iv, CRYPTO_KEY_SIZE);
    crypto_ctx.counter = 0;
    crypto_ctx.initialized = true;
    
    // Initialize authentication session
    current_session.state = AUTH_STATE_LOCKED;
    current_session.failed_attempts = 0;
    current_session.active = false;
    
    // Initialize intrusion detection
    memset(&intrusion_state, 0, sizeof(intrusion_state));
    
    // Log boot event
    log_security_event(SEC_EVENT_BOOT, 0, 0, 1);
    
    printf("Security framework initialized (Level: %d)\n", ram_config.level);
}

void security_factory_reset(void) {
    // Initialize default configuration
    ram_config.signature = SECURE_BOOT_SIGNATURE;
    ram_config.level = SECURITY_LEVEL_BASIC;
    ram_config.auth_timeout = 300; // 5 minutes
    ram_config.max_auth_attempts = MAX_AUTH_ATTEMPTS;
    ram_config.tamper_detection_enabled = true;
    ram_config.secure_boot_enabled = true;
    
    // Generate default device ID
    for (uint8_t i = 0; i < 8; i++) {
        ram_config.device_id[i] = secure_random() & 0xFF;
    }
    
    // Generate default master key
    for (uint8_t i = 0; i < CRYPTO_KEY_SIZE; i++) {
        ram_config.master_key[i] = secure_random() & 0xFF;
    }
    
    // Calculate checksum
    ram_config.checksum = simple_hash((uint8_t*)&ram_config, 
                                    sizeof(security_config_t) - sizeof(uint16_t));
    
    // Save to EEPROM
    eeprom_write_block(&ram_config, &eeprom_config, sizeof(security_config_t));
    
    printf("Security configuration reset to factory defaults\n");
}

bool security_change_master_key(const uint8_t* old_key, const uint8_t* new_key) {
    // Verify old key
    if (memcmp(old_key, ram_config.master_key, CRYPTO_KEY_SIZE) != 0) {
        log_security_event(SEC_EVENT_AUTH_FAIL, 0, 0, 2);
        return false;
    }
    
    // Update master key
    memcpy(ram_config.master_key, new_key, CRYPTO_KEY_SIZE);
    memcpy(crypto_ctx.key, new_key, CRYPTO_KEY_SIZE);
    
    // Update checksum and save
    ram_config.checksum = simple_hash((uint8_t*)&ram_config, 
                                    sizeof(security_config_t) - sizeof(uint16_t));
    eeprom_write_block(&ram_config, &eeprom_config, sizeof(security_config_t));
    
    log_security_event(SEC_EVENT_AUTH_SUCCESS, 1, 0, 1);
    return true;
}

// ================================
// AUTHENTICATION SYSTEM
// ================================

void auth_start_session(void) {
    if (current_session.active && 
        (security_ticks - current_session.session_start) < (ram_config.auth_timeout * 1000)) {
        printf("Session already active\n");
        return;
    }
    
    // Generate new challenge
    current_session.challenge = secure_random();
    current_session.state = AUTH_STATE_CHALLENGE;
    current_session.session_start = security_ticks;
    current_session.failed_attempts = 0;
    current_session.active = true;
    
    // Generate nonce
    generate_nonce(current_session.current_nonce, NONCE_SIZE);
    
    printf("Auth challenge: 0x%08lx\n", current_session.challenge);
    printf("Nonce: ");
    for (uint8_t i = 0; i < NONCE_SIZE; i++) {
        printf("%02X", current_session.current_nonce[i]);
    }\n    printf("\n");
}

bool auth_verify_response(uint32_t response) {
    if (current_session.state != AUTH_STATE_CHALLENGE) {
        printf("No active authentication challenge\n");
        return false;
    }
    
    // Calculate expected response (simplified HMAC)\n    uint8_t challenge_bytes[4];\n    challenge_bytes[0] = (current_session.challenge >> 24) & 0xFF;\n    challenge_bytes[1] = (current_session.challenge >> 16) & 0xFF;\n    challenge_bytes[2] = (current_session.challenge >> 8) & 0xFF;\n    challenge_bytes[3] = current_session.challenge & 0xFF;\n    \n    uint32_t expected = simple_hash(challenge_bytes, 4);\n    expected ^= simple_hash(ram_config.master_key, CRYPTO_KEY_SIZE);\n    expected ^= simple_hash(current_session.current_nonce, NONCE_SIZE);\n    \n    if (response == expected) {\n        current_session.state = AUTH_STATE_AUTHENTICATED;\n        current_session.failed_attempts = 0;\n        log_security_event(SEC_EVENT_AUTH_SUCCESS, 0, 0, 1);\n        printf("Authentication successful\n");\n        return true;\n    } else {\n        current_session.failed_attempts++;\n        log_security_event(SEC_EVENT_AUTH_FAIL, current_session.failed_attempts, 0, 2);\n        \n        if (current_session.failed_attempts >= ram_config.max_auth_attempts) {\n            auth_lockout();\n        }\n        \n        printf("Authentication failed (%d/%d attempts)\n", \n               current_session.failed_attempts, ram_config.max_auth_attempts);\n        return false;\n    }\n}\n\nvoid auth_lockout(void) {\n    current_session.state = AUTH_STATE_LOCKED;\n    current_session.active = false;\n    \n    // Trigger security measures\n    log_security_event(SEC_EVENT_INTRUSION, MAX_AUTH_ATTEMPTS, 0, 3);\n    \n    if (ram_config.level >= SECURITY_LEVEL_ENHANCED) {\n        // Clear sensitive data\n        memset(&crypto_ctx, 0, sizeof(crypto_ctx));\n        \n        // Could trigger:\n        // - Factory reset\n        // - Data wipe\n        // - Physical lockout mechanisms\n    }\n    \n    printf("SECURITY LOCKOUT: Too many failed attempts\n");\n}\n\nbool auth_is_authenticated(void) {\n    if (!current_session.active || current_session.state != AUTH_STATE_AUTHENTICATED) {\n        return false;\n    }\n    \n    // Check session timeout\n    if ((security_ticks - current_session.session_start) >= (ram_config.auth_timeout * 1000)) {\n        current_session.state = AUTH_STATE_EXPIRED;\n        current_session.active = false;\n        printf("Authentication session expired\n");\n        return false;\n    }\n    \n    return true;\n}\n\nvoid auth_end_session(void) {\n    current_session.active = false;\n    current_session.state = AUTH_STATE_LOCKED;\n    \n    // Clear sensitive session data\n    memset(current_session.current_nonce, 0, NONCE_SIZE);\n    current_session.challenge = 0;\n    \n    printf("Authentication session ended\n");\n}\n\n// ================================\n// INTRUSION DETECTION\n// ================================\n\nvoid intrusion_detection_init(void) {\n    if (!ram_config.tamper_detection_enabled) {\n        return;\n    }\n    \n    // Configure ADC for voltage monitoring\n    ADMUX = (1 << REFS0); // AVcc reference\n    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1); // Enable ADC, prescaler 64\n    \n    // Initialize baseline readings\n    intrusion_state.voltage_anomaly = false;\n    intrusion_state.frequency_anomaly = false;\n    intrusion_state.case_opened = false;\n    intrusion_state.motion_detected = false;\n    intrusion_state.anomaly_count = 0;\n    \n    printf("Intrusion detection initialized\n");\n}\n\nvoid check_voltage_anomaly(void) {\n    // Read supply voltage\n    ADCSRA |= (1 << ADSC);\n    while (ADCSRA & (1 << ADSC));\n    \n    uint16_t voltage = ADC;\n    \n    // Check for anomalies (simplified)\n    static uint16_t baseline_voltage = 0;\n    if (baseline_voltage == 0) {\n        baseline_voltage = voltage;\n        return;\n    }\n    \n    uint16_t deviation = (voltage > baseline_voltage) ? \n                        (voltage - baseline_voltage) : (baseline_voltage - voltage);\n    \n    if (deviation > 50) { // Threshold for anomaly\n        intrusion_state.voltage_anomaly = true;\n        intrusion_state.anomaly_count++;\n        log_security_event(SEC_EVENT_TAMPER, voltage, 1, 2);\n    }\n}\n\nvoid check_frequency_anomaly(void) {\n    // Monitor system clock frequency using Timer1\n    static uint16_t expected_count = 0;\n    \n    if (expected_count == 0) {\n        expected_count = TCNT1;\n        return;\n    }\n    \n    uint16_t current_count = TCNT1;\n    uint16_t deviation = (current_count > expected_count) ?\n                        (current_count - expected_count) : (expected_count - current_count);\n    \n    if (deviation > 100) { // Threshold for frequency anomaly\n        intrusion_state.frequency_anomaly = true;\n        intrusion_state.anomaly_count++;\n        log_security_event(SEC_EVENT_TAMPER, current_count, 2, 2);\n    }\n}\n\nvoid check_case_tamper(void) {\n    // Read case tamper switch (assume connected to pin)\n    if (!(PINB & (1 << PB0))) { // Case opened (switch released)\n        if (!intrusion_state.case_opened) {\n            intrusion_state.case_opened = true;\n            intrusion_state.anomaly_count++;\n            log_security_event(SEC_EVENT_TAMPER, 0, 3, 3);\n            printf("TAMPER: Case opened detected\n");\n        }\n    } else {\n        intrusion_state.case_opened = false;\n    }\n}\n\nvoid intrusion_detection_update(void) {\n    if (!ram_config.tamper_detection_enabled) {\n        return;\n    }\n    \n    uint32_t current_time = security_ticks;\n    \n    // Check every 100ms\n    if (current_time - intrusion_state.last_check < 100) {\n        return;\n    }\n    intrusion_state.last_check = current_time;\n    \n    check_voltage_anomaly();\n    check_frequency_anomaly();\n    check_case_tamper();\n    \n    // Check for sustained anomalies\n    if (intrusion_state.anomaly_count > 10) {\n        trigger_intrusion_response();\n    }\n}\n\nvoid trigger_intrusion_response(void) {\n    log_security_event(SEC_EVENT_INTRUSION, intrusion_state.anomaly_count, 0, 3);\n    \n    printf("SECURITY ALERT: Intrusion detected!\n");\n    \n    switch (ram_config.level) {\n        case SECURITY_LEVEL_BASIC:\n            // Log only\n            break;\n            \n        case SECURITY_LEVEL_ENHANCED:\n            // Lock system\n            auth_lockout();\n            break;\n            \n        case SECURITY_LEVEL_PARANOID:\n            // Wipe sensitive data\n            auth_lockout();\n            memset(&crypto_ctx, 0, sizeof(crypto_ctx));\n            memset(&ram_config.master_key, 0, CRYPTO_KEY_SIZE);\n            printf("CRITICAL: Sensitive data wiped\n");\n            break;\n    }\n}\n\n// ================================\n// SECURE COMMUNICATION\n// ================================\n\nbool secure_send_data(const uint8_t* data, uint8_t length) {\n    if (!auth_is_authenticated()) {\n        printf("Error: Not authenticated\n");\n        return false;\n    }\n    \n    if (!crypto_ctx.initialized) {\n        printf("Error: Crypto not initialized\n");\n        return false;\n    }\n    \n    // Create encrypted packet\n    uint8_t packet[length + 4]; // Data + counter\n    \n    // Add counter for replay protection\n    packet[0] = (crypto_ctx.counter >> 24) & 0xFF;\n    packet[1] = (crypto_ctx.counter >> 16) & 0xFF;\n    packet[2] = (crypto_ctx.counter >> 8) & 0xFF;\n    packet[3] = crypto_ctx.counter & 0xFF;\n    \n    // Copy data\n    memcpy(packet + 4, data, length);\n    \n    // Encrypt\n    simple_encrypt(packet, length + 4, crypto_ctx.key);\n    \n    // Send packet (implementation depends on communication interface)\n    printf("Encrypted packet (%d bytes): ", length + 4);\n    for (uint8_t i = 0; i < length + 4; i++) {\n        printf("%02X ", packet[i]);\n    }\n    printf("\n");\n    \n    crypto_ctx.counter++;\n    return true;\n}\n\nbool secure_receive_data(uint8_t* data, uint8_t* length, const uint8_t* packet, uint8_t packet_length) {\n    if (!auth_is_authenticated()) {\n        return false;\n    }\n    \n    if (!crypto_ctx.initialized || packet_length < 4) {\n        return false;\n    }\n    \n    // Create working copy\n    uint8_t decrypted[packet_length];\n    memcpy(decrypted, packet, packet_length);\n    \n    // Decrypt\n    simple_decrypt(decrypted, packet_length, crypto_ctx.key);\n    \n    // Extract counter\n    uint32_t received_counter = (decrypted[0] << 24) | (decrypted[1] << 16) |\n                               (decrypted[2] << 8) | decrypted[3];\n    \n    // Check for replay attacks\n    if (received_counter <= crypto_ctx.counter) {\n        log_security_event(SEC_EVENT_CRYPTO_ERROR, received_counter, 0, 2);\n        return false;\n    }\n    \n    // Extract data\n    *length = packet_length - 4;\n    memcpy(data, decrypted + 4, *length);\n    \n    return true;\n}\n\n// ================================\n// SECURITY LOGGING\n// ================================\n\nvoid log_security_event(security_event_t event, uint16_t data, uint8_t module, uint8_t severity) {\n    security_log_entry_t* entry = &security_log[log_head];\n    \n    entry->timestamp = security_ticks;\n    entry->event_type = event;\n    entry->event_data = data;\n    entry->source_module = module;\n    entry->severity = severity;\n    \n    log_head = (log_head + 1) % MAX_SECURITY_LOGS;\n    \n    // Print critical events immediately\n    if (severity >= 3) {\n        printf("CRITICAL SECURITY EVENT: Type %d, Data 0x%04X, Module %d\n",\n               event, data, module);\n    }\n}\n\nvoid print_security_log(void) {\n    printf("\n=== Security Log ===\n");\n    printf("Time(ms)  Event  Data    Mod Sev\n");\n    printf("--------  -----  ------  --- ---\n");\n    \n    for (uint8_t i = 0; i < MAX_SECURITY_LOGS; i++) {\n        uint8_t index = (log_head + i) % MAX_SECURITY_LOGS;\n        security_log_entry_t* entry = &security_log[index];\n        \n        if (entry->timestamp > 0) {\n            const char* event_names[] = {\n                "Boot", "AuthOK", "AuthFail", "Intrusion",\n                "Tamper", "CryptoErr", "MemViol", "Watchdog"\n            };\n            \n            printf("%8lu  %-9s  0x%04X  %3d %3d\n",\n                   entry->timestamp,\n                   event_names[entry->event_type],\n                   entry->event_data,\n                   entry->source_module,\n                   entry->severity);\n        }\n    }\n}\n\nvoid clear_security_log(void) {\n    if (!auth_is_authenticated()) {\n        printf("Authentication required to clear log\n");\n        return;\n    }\n    \n    memset(security_log, 0, sizeof(security_log));\n    log_head = 0;\n    \n    log_security_event(SEC_EVENT_AUTH_SUCCESS, 2, 0, 1); // Log cleared\n    printf("Security log cleared\n");\n}\n\n// ================================\n// SECURE BOOT\n// ================================\n\nbool verify_firmware_integrity(void) {\n    if (!ram_config.secure_boot_enabled) {\n        return true; // Skip verification\n    }\n    \n    // Calculate firmware hash (simplified)\n    // In production, this would use a proper signature verification\n    uint32_t firmware_hash = 0;\n    \n    // Hash the flash memory (excluding this function)\n    for (uint16_t addr = 0; addr < 0x7000; addr += 2) {\n        uint16_t word = pgm_read_word(addr);\n        firmware_hash ^= word;\n        firmware_hash = (firmware_hash << 1) | (firmware_hash >> 31);\n    }\n    \n    // Expected hash (would be stored securely)\n    uint32_t expected_hash = 0x12345678; // Example\n    \n    if (firmware_hash == expected_hash) {\n        return true;\n    } else {\n        log_security_event(SEC_EVENT_BOOT, firmware_hash, 0, 3);\n        return false;\n    }\n}\n\nvoid secure_boot_sequence(void) {\n    printf("Starting secure boot...\n");\n    \n    // Verify firmware integrity\n    if (!verify_firmware_integrity()) {\n        printf("CRITICAL: Firmware integrity check failed!\n");\n        \n        if (ram_config.level >= SECURITY_LEVEL_ENHANCED) {\n            // Halt system\n            while (1) {\n                // Flash LED to indicate failure\n                PORTB ^= (1 << PB5);\n                _delay_ms(100);\n            }\n        }\n    }\n    \n    printf("Firmware integrity verified\n");\n}\n\n// ================================\n// WATCHDOG SECURITY\n// ================================\n\nvoid security_watchdog_init(void) {\n    if (ram_config.level >= SECURITY_LEVEL_ENHANCED) {\n        // Enable watchdog with 2 second timeout\n        wdt_enable(WDTO_2S);\n        printf("Security watchdog enabled\n");\n    }\n}\n\nvoid security_watchdog_reset(void) {\n    if (ram_config.level >= SECURITY_LEVEL_ENHANCED) {\n        wdt_reset();\n    }\n}\n\n// Watchdog interrupt (if configured)\nISR(WDT_vect) {\n    log_security_event(SEC_EVENT_WATCHDOG, 0, 0, 2);\n    // Could trigger security measures here\n}\n\n// ================================\n// COMMAND INTERFACE\n// ================================\n\nvoid process_security_command(const char* command) {\n    if (strcmp(command, "status") == 0) {\n        printf("Security Level: %d\n", ram_config.level);\n        printf("Auth State: %d\n", current_session.state);\n        printf("Session Active: %s\n", current_session.active ? "Yes" : "No");\n        printf("Tamper Detection: %s\n", ram_config.tamper_detection_enabled ? "Enabled" : "Disabled");\n        printf("Secure Boot: %s\n", ram_config.secure_boot_enabled ? "Enabled" : "Disabled");\n        \n    } else if (strcmp(command, "auth") == 0) {\n        auth_start_session();\n        \n    } else if (strncmp(command, "response ", 9) == 0) {\n        uint32_t response = strtoul(command + 9, NULL, 16);\n        auth_verify_response(response);\n        \n    } else if (strcmp(command, "logout") == 0) {\n        auth_end_session();\n        \n    } else if (strcmp(command, "log") == 0) {\n        print_security_log();\n        \n    } else if (strcmp(command, "clearlog") == 0) {\n        clear_security_log();\n        \n    } else if (strcmp(command, "intrusion") == 0) {\n        printf("Intrusion State:\n");\n        printf("  Voltage Anomaly: %s\n", intrusion_state.voltage_anomaly ? "Yes" : "No");\n        printf("  Frequency Anomaly: %s\n", intrusion_state.frequency_anomaly ? "Yes" : "No");\n        printf("  Case Opened: %s\n", intrusion_state.case_opened ? "Yes" : "No");\n        printf("  Anomaly Count: %d\n", intrusion_state.anomaly_count);\n        \n    } else if (strncmp(command, "level ", 6) == 0) {\n        if (!auth_is_authenticated()) {\n            printf("Authentication required\n");\n            return;\n        }\n        \n        int level = atoi(command + 6);\n        if (level >= 0 && level <= 3) {\n            ram_config.level = (security_level_t)level;\n            // Update checksum and save\n            ram_config.checksum = simple_hash((uint8_t*)&ram_config, \n                                            sizeof(security_config_t) - sizeof(uint16_t));\n            eeprom_write_block(&ram_config, &eeprom_config, sizeof(security_config_t));\n            printf("Security level changed to: %d\n", level);\n        }\n        \n    } else if (strcmp(command, "reset") == 0) {\n        if (!auth_is_authenticated()) {\n            printf("Authentication required\n");\n            return;\n        }\n        security_factory_reset();\n        \n    } else {\n        printf("Security commands: status, auth, response <hex>, logout, log, clearlog, intrusion, level N, reset\n");\n    }\n}\n\n// ================================\n// TIMER INTERRUPT FOR SECURITY TICKS\n// ================================\n\nISR(TIMER0_COMPA_vect) {\n    security_ticks++;\n}\n\nvoid security_timer_init(void) {\n    // Configure Timer0 for 1ms interrupts\n    TCCR0A = (1 << WGM01); // CTC mode\n    TCCR0B = (1 << CS01) | (1 << CS00); // Prescaler 64\n    OCR0A = (F_CPU / 64 / 1000) - 1; // 1ms interval\n    TIMSK0 |= (1 << OCIE0A);\n}\n\n// ================================\n// MAIN SECURITY FRAMEWORK\n// ================================\n\nvoid security_framework_init(void) {\n    security_timer_init();\n    security_init();\n    intrusion_detection_init();\n    security_watchdog_init();\n    \n    if (ram_config.secure_boot_enabled) {\n        secure_boot_sequence();\n    }\n    \n    sei(); // Enable interrupts\n    \n    printf("Security framework ready\n");\n}\n\nvoid security_framework_update(void) {\n    intrusion_detection_update();\n    security_watchdog_reset();\n    \n    // Check for authentication timeout\n    if (current_session.active && \n        (security_ticks - current_session.session_start) >= (ram_config.auth_timeout * 1000)) {\n        printf("Authentication timeout\n");\n        auth_end_session();\n    }\n}\n\n/*\n// Example usage:\nint main(void) {\n    // Initialize security framework\n    security_framework_init();\n    \n    while (1) {\n        // Update security systems\n        security_framework_update();\n        \n        // Process security commands\n        char command[64];\n        if (read_security_command(command, sizeof(command))) {\n            process_security_command(command);\n        }\n        \n        // Your application code here\n        if (auth_is_authenticated()) {\n            // Perform authenticated operations\n        }\n        \n        _delay_ms(10);\n    }\n    \n    return 0;\n}\n*/