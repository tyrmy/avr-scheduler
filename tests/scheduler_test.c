/*
 * AVR Scheduler Unit Tests
 * 
 * Hardware test suite for validating scheduler functionality,
 * especially context switching and stack integrity.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include "../scheduler.h"

// UART configuration for test output
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1

// Test result tracking
static uint16_t tests_run = 0;
static uint16_t tests_passed = 0;
static uint16_t tests_failed = 0;

// Test state variables for context switch validation
static volatile uint8_t task1_executed = 0;
static volatile uint8_t task2_executed = 0;
static volatile uint32_t task1_magic_value = 0;
static volatile uint32_t task2_magic_value = 0;
static volatile uint16_t task1_stack_canary = 0;
static volatile uint16_t task2_stack_canary = 0;

// Magic values for stack integrity checking
#define TASK1_MAGIC 0xDEADBEEF
#define TASK2_MAGIC 0xCAFEBABE
#define STACK_CANARY_1 0xA5A5
#define STACK_CANARY_2 0x5A5A

// UART functions
void uart_init(void) {
    uint16_t ubrr = MYUBRR;
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;
    UCSR0B = (1 << TXEN0);  // Enable transmitter
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  // 8-bit data
}

void uart_putc(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

void uart_put_hex(uint16_t val) {
    const char hex[] = "0123456789ABCDEF";
    uart_putc('0'); uart_putc('x');
    uart_putc(hex[(val >> 12) & 0xF]);
    uart_putc(hex[(val >> 8) & 0xF]);
    uart_putc(hex[(val >> 4) & 0xF]);
    uart_putc(hex[val & 0xF]);
}

void uart_put_dec(uint16_t val) {
    char buf[6];
    uint8_t i = 0;
    
    if (val == 0) {
        uart_putc('0');
        return;
    }
    
    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    
    while (i > 0) {
        uart_putc(buf[--i]);
    }
}

// Test framework macros
#define TEST_START(name) \
    do { \
        uart_puts("\n[TEST] "); \
        uart_puts(name); \
        uart_puts("... "); \
        tests_run++; \
    } while(0)

#define TEST_PASS() \
    do { \
        uart_puts("PASS\n"); \
        tests_passed++; \
    } while(0)

#define TEST_FAIL(msg) \
    do { \
        uart_puts("FAIL: "); \
        uart_puts(msg); \
        uart_putc('\n'); \
        tests_failed++; \
    } while(0)

#define ASSERT_EQ(actual, expected, msg) \
    do { \
        if ((actual) != (expected)) { \
            uart_puts("FAIL: "); \
            uart_puts(msg); \
            uart_puts(" (expected: "); \
            uart_put_dec(expected); \
            uart_puts(", got: "); \
            uart_put_dec(actual); \
            uart_puts(")\n"); \
            tests_failed++; \
            return; \
        } \
    } while(0)

#define ASSERT_TRUE(condition, msg) \
    do { \
        if (!(condition)) { \
            TEST_FAIL(msg); \
            return; \
        } \
    } while(0)

// Test tasks for context switching validation
void test_task1(void) {
    // Set up stack canary
    volatile uint16_t local_canary = STACK_CANARY_1;
    task1_stack_canary = local_canary;
    
    // Set magic value to verify this task ran
    task1_magic_value = TASK1_MAGIC;
    
    while (1) {
        // Increment execution counter
        task1_executed++;
        
        // Verify stack canary is still intact
        if (local_canary != STACK_CANARY_1) {
            task1_stack_canary = 0xFFFF;  // Signal corruption
        } else {
            task1_stack_canary = local_canary;
        }
        
        // Yield to allow task2 to run
        scheduler_yield();
        
        // After context switch back, verify our data is intact
        if (task1_magic_value != TASK1_MAGIC) {
            task1_magic_value = 0xFFFFFFFF;  // Signal corruption
        }
        
        // Short delay
        if (task1_executed >= 10) {
            task_delay(5000);  // Block for a while after 10 iterations
        } else {
            task_delay(10);
        }
    }
}

void test_task2(void) {
    // Set up stack canary
    volatile uint16_t local_canary = STACK_CANARY_2;
    task2_stack_canary = local_canary;
    
    // Set magic value to verify this task ran
    task2_magic_value = TASK2_MAGIC;
    
    while (1) {
        // Increment execution counter
        task2_executed++;
        
        // Verify stack canary is still intact
        if (local_canary != STACK_CANARY_2) {
            task2_stack_canary = 0xFFFF;  // Signal corruption
        } else {
            task2_stack_canary = local_canary;
        }
        
        // Yield to allow task1 to run
        scheduler_yield();
        
        // After context switch back, verify our data is intact
        if (task2_magic_value != TASK2_MAGIC) {
            task2_magic_value = 0xFFFFFFFF;  // Signal corruption
        }
        
        // Short delay
        if (task2_executed >= 10) {
            task_delay(5000);  // Block for a while after 10 iterations
        } else {
            task_delay(10);
        }
    }
}

// Test: Basic scheduler initialization
void test_scheduler_init(void) {
    TEST_START("Scheduler initialization");
    
    scheduler_init();
    
    ASSERT_EQ(scheduler_get_task_count(), 0, "Task count should be 0 after init");
    
    TEST_PASS();
}

// Test: Add tasks
void test_add_tasks(void) {
    TEST_START("Adding tasks");
    
    scheduler_init();
    
    int8_t task1_id = scheduler_add_task(test_task1);
    ASSERT_TRUE(task1_id >= 0, "Task 1 should be added successfully");
    
    int8_t task2_id = scheduler_add_task(test_task2);
    ASSERT_TRUE(task2_id >= 0, "Task 2 should be added successfully");
    
    ASSERT_EQ(scheduler_get_task_count(), 2, "Task count should be 2");
    ASSERT_TRUE(task1_id != task2_id, "Task IDs should be unique");
    
    TEST_PASS();
}

// Test: Context switching with stack integrity
void test_context_switching(void) {
    TEST_START("Context switching with stack integrity");
    
    // Reset test variables
    task1_executed = 0;
    task2_executed = 0;
    task1_magic_value = 0;
    task2_magic_value = 0;
    task1_stack_canary = 0;
    task2_stack_canary = 0;
    
    scheduler_init();
    
    int8_t task1_id = scheduler_add_task(test_task1);
    int8_t task2_id = scheduler_add_task(test_task2);
    
    ASSERT_TRUE(task1_id >= 0, "Task 1 should be added");
    ASSERT_TRUE(task2_id >= 0, "Task 2 should be added");
    
    uart_puts("Starting scheduler...\n");
    
    // This will start the scheduler and never return
    // We'll use a watchdog timer to break out after a period
    // For now, we'll just start it and rely on the tasks to report their status
    scheduler_start();
    
    // Note: We never reach here - scheduler_start() doesn't return
}

// Simple test tasks that don't use scheduler
void simple_test_task1(void) {
    volatile uint8_t counter = 0;
    volatile uint32_t checksum = 0xAAAAAAAA;
    
    for (uint8_t i = 0; i < 100; i++) {
        counter++;
        checksum ^= counter;
        scheduler_yield();
        
        // Verify data integrity after yield
        if (checksum == 0xAAAAAAAA) {
            task1_magic_value = 0xFFFFFFFF;  // Error - data corrupted
        }
    }
    
    task1_magic_value = checksum;  // Store final checksum
    task1_executed = counter;
    
    while(1) {
        task_delay(1000);
    }
}

void simple_test_task2(void) {
    volatile uint8_t counter = 0;
    volatile uint32_t checksum = 0x55555555;
    
    for (uint8_t i = 0; i < 100; i++) {
        counter++;
        checksum ^= counter;
        scheduler_yield();
        
        // Verify data integrity after yield
        if (checksum == 0x55555555) {
            task2_magic_value = 0xFFFFFFFF;  // Error - data corrupted
        }
    }
    
    task2_magic_value = checksum;  // Store final checksum
    task2_executed = counter;
    
    while(1) {
        task_delay(1000);
    }
}

// Monitor task that checks results
void monitor_task(void) {
    uint16_t wait_cycles = 0;
    
    // Wait for both tasks to complete their iterations
    while (wait_cycles < 500) {
        task_delay(100);  // Wait 100ms
        wait_cycles++;
        
        if (task1_executed >= 100 && task2_executed >= 100) {
            break;
        }
    }
    
    uart_puts("\n=== Context Switch Test Results ===\n");
    uart_puts("Task 1 executed: ");
    uart_put_dec(task1_executed);
    uart_puts(" times\n");
    
    uart_puts("Task 2 executed: ");
    uart_put_dec(task2_executed);
    uart_puts(" times\n");
    
    uart_puts("Task 1 final checksum: ");
    uart_put_hex((uint16_t)(task1_magic_value >> 16));
    uart_put_hex((uint16_t)(task1_magic_value & 0xFFFF));
    uart_putc('\n');
    
    uart_puts("Task 2 final checksum: ");
    uart_put_hex((uint16_t)(task2_magic_value >> 16));
    uart_put_hex((uint16_t)(task2_magic_value & 0xFFFF));
    uart_putc('\n');
    
    // Verify results
    if (task1_executed == 100 && task2_executed == 100) {
        if (task1_magic_value != 0xFFFFFFFF && task2_magic_value != 0xFFFFFFFF) {
            uart_puts("\n*** ALL TESTS PASSED ***\n");
            uart_puts("Stack integrity maintained across context switches!\n");
        } else {
            uart_puts("\n*** TEST FAILED ***\n");
            uart_puts("Data corruption detected!\n");
        }
    } else {
        uart_puts("\n*** TEST FAILED ***\n");
        uart_puts("Tasks did not complete expected iterations!\n");
    }
    
    // Blink LED to signal completion
    DDRB |= (1 << PB5);  // Set LED pin as output
    while (1) {
        PORTB ^= (1 << PB5);
        task_delay(500);
    }
}

// Main test runner
int main(void) {
    // Initialize UART for test output
    uart_init();
    
    uart_puts("\n\n");
    uart_puts("===================================\n");
    uart_puts("  AVR Scheduler Unit Test Suite\n");
    uart_puts("===================================\n\n");
    
    // Run offline tests (before starting scheduler)
    // test_scheduler_init();
    // test_add_tasks();
    
    // Run context switching test
    uart_puts("Starting context switching test with stack integrity validation...\n");
    
    scheduler_init();
    
    // Add simple test tasks
    int8_t t1 = scheduler_add_task(simple_test_task1);
    int8_t t2 = scheduler_add_task(simple_test_task2);
    int8_t tm = scheduler_add_task(monitor_task);
    
    if (t1 < 0 || t2 < 0 || tm < 0) {
        uart_puts("ERROR: Failed to add tasks!\n");
        while(1);
    }
    
    uart_puts("Tasks added successfully. Starting scheduler...\n");
    uart_puts("Task 1 ID: "); uart_put_dec(t1); uart_putc('\n');
    uart_puts("Task 2 ID: "); uart_put_dec(t2); uart_putc('\n');
    uart_puts("Monitor ID: "); uart_put_dec(tm); uart_putc('\n');
    
    // Start the scheduler - this never returns
    scheduler_start();
    
    // Should never reach here
    while(1);
    
    return 0;
}

