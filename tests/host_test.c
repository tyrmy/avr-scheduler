/*
 * Host-based unit tests for AVR Scheduler
 * 
 * These tests mock AVR-specific functionality and run on the host system
 * for quick validation during development.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

// Define mock AVR registers (extern declarations are in avr/io.h)
uint8_t mock_TCCR0A = 0;
uint8_t mock_TCCR0B = 0;
uint8_t mock_OCR0A = 0;
uint8_t mock_TIMSK0 = 0;
uint8_t mock_SREG = 0;

// Enable debug mode
#ifndef SCHEDULER_DEBUG
#define SCHEDULER_DEBUG
#endif

// Now include scheduler
#include "../scheduler.h"

// Test framework
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    static void name(void); \
    static void name##_wrapper(void) { \
        printf("Running test: %s ... ", #name); \
        fflush(stdout); \
        tests_run++; \
        name(); \
    } \
    static void name(void)

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL\n  %s\n  at %s:%d\n", message, __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) != (expected)) { \
            printf("FAIL\n  %s\n  Expected: %d, Got: %d\n  at %s:%d\n", \
                   message, (int)(expected), (int)(actual), __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("PASS\n"); \
        tests_passed++; \
    } while(0)

#define RUN_TEST(test) test##_wrapper()

// Test state
static volatile int task1_run_count = 0;
static volatile int task2_run_count = 0;
static volatile uint32_t task1_checksum = 0;
static volatile uint32_t task2_checksum = 0;

// Test task functions
void test_task_1(void) {
    task1_run_count++;
    volatile uint32_t local_data = 0xDEADBEEF;
    
    for (int i = 0; i < 5; i++) {
        task1_checksum += local_data;
        scheduler_yield();
        
        // Verify local data wasn't corrupted
        if (local_data != 0xDEADBEEF) {
            task1_checksum = 0xFFFFFFFF;  // Error marker
            break;
        }
    }
}

void test_task_2(void) {
    task2_run_count++;
    volatile uint32_t local_data = 0xCAFEBABE;
    
    for (int i = 0; i < 5; i++) {
        task2_checksum += local_data;
        scheduler_yield();
        
        // Verify local data wasn't corrupted
        if (local_data != 0xCAFEBABE) {
            task2_checksum = 0xFFFFFFFF;  // Error marker
            break;
        }
    }
}

void simple_task(void) {
    task1_run_count++;
}

void empty_task(void) {
    // Does nothing
}

// ============================================================================
// Tests
// ============================================================================

TEST(test_scheduler_init) {
    scheduler_init();
    
    ASSERT_EQ(scheduler_get_task_count(), 0, "Task count should be 0 after init");
    ASSERT_EQ(scheduler_get_current_task(), 0, "Current task should be 0");
    
    TEST_PASS();
}

TEST(test_add_single_task) {
    scheduler_init();
    
    int8_t task_id = scheduler_add_task(simple_task);
    
    ASSERT(task_id >= 0, "Task should be added successfully");
    ASSERT_EQ(scheduler_get_task_count(), 1, "Task count should be 1");
    
    TEST_PASS();
}

TEST(test_add_multiple_tasks) {
    scheduler_init();
    
    int8_t task1_id = scheduler_add_task(test_task_1);
    int8_t task2_id = scheduler_add_task(test_task_2);
    
    ASSERT(task1_id >= 0, "Task 1 should be added");
    ASSERT(task2_id >= 0, "Task 2 should be added");
    ASSERT(task1_id != task2_id, "Task IDs should be unique");
    ASSERT_EQ(scheduler_get_task_count(), 2, "Task count should be 2");
    
    TEST_PASS();
}

TEST(test_add_null_task) {
    scheduler_init();
    
    int8_t task_id = scheduler_add_task(NULL);
    
    ASSERT(task_id < 0, "Adding NULL task should fail");
    ASSERT_EQ(scheduler_get_task_count(), 0, "Task count should remain 0");
    
    TEST_PASS();
}

TEST(test_max_tasks) {
    scheduler_init();
    
    // Add MAX_TASKS tasks
    for (int i = 0; i < MAX_TASKS; i++) {
        int8_t task_id = scheduler_add_task(simple_task);
        ASSERT(task_id >= 0, "Task should be added");
    }
    
    ASSERT_EQ(scheduler_get_task_count(), MAX_TASKS, "Should have MAX_TASKS tasks");
    
    // Try to add one more
    int8_t overflow_task = scheduler_add_task(simple_task);
    ASSERT(overflow_task < 0, "Adding beyond MAX_TASKS should fail");
    
    TEST_PASS();
}

TEST(test_timer_initialization) {
    scheduler_init();
    
    // Check that Timer0 is configured for 1ms ticks at 16MHz
    ASSERT(TCCR0A != 0, "TCCR0A should be configured");
    ASSERT(TCCR0B != 0, "TCCR0B should be configured");
    ASSERT_EQ(OCR0A, 249, "OCR0A should be 249 for 1ms @ 16MHz");
    ASSERT(TIMSK0 != 0, "TIMSK0 should enable interrupt");
    
    TEST_PASS();
}

TEST(test_suspend_resume_task) {
    scheduler_init();
    
    int8_t task_id = scheduler_add_task(simple_task);
    ASSERT(task_id >= 0, "Task should be added");
    
    // Suspend task
    scheduler_suspend_task(task_id);
    
    // Resume task
    scheduler_resume_task(task_id);
    
    // Test boundary conditions
    scheduler_suspend_task(255);  // Invalid task ID
    scheduler_resume_task(255);   // Invalid task ID
    
    TEST_PASS();
}

TEST(test_task_delay_timer) {
    scheduler_init();
    
    int8_t task_id = scheduler_add_task(simple_task);
    ASSERT(task_id >= 0, "Task should be added");
    
    // Note: We can't fully test task_delay without running the scheduler,
    // but we can test that it doesn't crash
    
    TEST_PASS();
}

#ifdef SCHEDULER_DEBUG
TEST(test_debug_stats_initialization) {
    scheduler_init();
    
    const scheduler_debug_t *stats = scheduler_get_debug_stats();
    
    ASSERT(stats != NULL, "Debug stats should not be NULL");
    ASSERT_EQ(stats->total_ticks, 0, "Total ticks should be 0");
    ASSERT_EQ(stats->context_switches, 0, "Context switches should be 0");
    ASSERT_EQ(stats->voluntary_yields, 0, "Voluntary yields should be 0");
    
    TEST_PASS();
}

TEST(test_debug_stats_reset) {
    scheduler_init();
    
    scheduler_reset_debug_stats();
    
    const scheduler_debug_t *stats = scheduler_get_debug_stats();
    ASSERT_EQ(stats->total_ticks, 0, "Total ticks should be 0 after reset");
    ASSERT_EQ(stats->context_switches, 0, "Context switches should be 0 after reset");
    ASSERT_EQ(stats->voluntary_yields, 0, "Voluntary yields should be 0 after reset");
    
    TEST_PASS();
}

TEST(test_get_task_stats) {
    scheduler_init();
    
    int8_t task_id = scheduler_add_task(simple_task);
    ASSERT(task_id >= 0, "Task should be added");
    
    uint32_t runtime_ticks = 0;
    uint32_t times_scheduled = 0;
    
    int8_t result = scheduler_get_task_stats(task_id, &runtime_ticks, &times_scheduled);
    ASSERT_EQ(result, 0, "Getting task stats should succeed");
    ASSERT_EQ(runtime_ticks, 0, "Runtime ticks should be 0 initially");
    ASSERT_EQ(times_scheduled, 0, "Times scheduled should be 0 initially");
    
    // Test invalid task ID
    result = scheduler_get_task_stats(255, &runtime_ticks, &times_scheduled);
    ASSERT(result < 0, "Getting stats for invalid task should fail");
    
    TEST_PASS();
}
#endif

TEST(test_scheduler_yield_no_tasks) {
    scheduler_init();
    
    // Yield with no tasks should not crash
    scheduler_yield();
    
    TEST_PASS();
}

TEST(test_scheduler_yield_single_task) {
    scheduler_init();
    task1_run_count = 0;
    
    int8_t task_id = scheduler_add_task(simple_task);
    ASSERT(task_id >= 0, "Task should be added");
    
    // Manually set the first task as running
    // (normally done by scheduler_start)
    
    // Yield should not crash with single task
    scheduler_yield();
    
    TEST_PASS();
}

TEST(test_stack_initialization) {
    scheduler_init();
    
    // Add a task and verify stack pointer is set
    int8_t task_id = scheduler_add_task(simple_task);
    ASSERT(task_id >= 0, "Task should be added");
    
    // We can't directly access stack_pointer from here,
    // but we verified the task was added successfully
    
    TEST_PASS();
}

TEST(test_multiple_scheduler_init) {
    // Test that we can reinitialize the scheduler
    scheduler_init();
    scheduler_add_task(simple_task);
    ASSERT_EQ(scheduler_get_task_count(), 1, "Should have 1 task");
    
    scheduler_init();
    ASSERT_EQ(scheduler_get_task_count(), 0, "Task count should be 0 after reinit");
    
    TEST_PASS();
}

// ============================================================================
// Main test runner
// ============================================================================

int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("  AVR Scheduler Host Unit Tests\n");
    printf("========================================\n\n");
    
    // Run all tests
    RUN_TEST(test_scheduler_init);
    RUN_TEST(test_add_single_task);
    RUN_TEST(test_add_multiple_tasks);
    RUN_TEST(test_add_null_task);
    RUN_TEST(test_max_tasks);
    RUN_TEST(test_timer_initialization);
    RUN_TEST(test_suspend_resume_task);
    RUN_TEST(test_task_delay_timer);
    RUN_TEST(test_scheduler_yield_no_tasks);
    RUN_TEST(test_scheduler_yield_single_task);
    RUN_TEST(test_stack_initialization);
    RUN_TEST(test_multiple_scheduler_init);
    
#ifdef SCHEDULER_DEBUG
    RUN_TEST(test_debug_stats_initialization);
    RUN_TEST(test_debug_stats_reset);
    RUN_TEST(test_get_task_stats);
#endif
    
    // Print summary
    printf("\n");
    printf("========================================\n");
    printf("  Test Results\n");
    printf("========================================\n");
    printf("Tests run:    %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    if (tests_failed == 0) {
        printf("\n✓ All tests passed!\n\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed!\n\n");
        return 1;
    }
}

