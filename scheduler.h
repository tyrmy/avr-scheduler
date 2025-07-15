#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <avr/io.h>

// maximum number of tasks the scheduler can handle
#define MAX_TASKS 8

// default stack size for each task (in bytes)
#define TASK_STACK_SIZE 128

// enable debug tracing (comment out to disable)
#define SCHEDULER_DEBUG

// task states
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SUSPENDED
} task_state_t;

// task control block
typedef struct {
    uint8_t *stack_pointer;     // current stack pointer
    uint8_t stack[TASK_STACK_SIZE]; // task's stack
    task_state_t state;         // current task state
    uint8_t task_id;            // unique task identifier
    uint16_t delay_ticks;       // delay counter in system ticks
#ifdef SCHEDULER_DEBUG
    uint32_t runtime_ticks;     // total ticks this task has been running
    uint32_t times_scheduled;   // number of times task was scheduled
#endif
} task_t;

#ifdef SCHEDULER_DEBUG
// debug statistics structure
typedef struct {
    uint32_t total_ticks;           // total system ticks since start
    uint32_t context_switches;      // total number of context switches
    uint32_t voluntary_yields;      // number of voluntary yields
} scheduler_debug_t;
#endif

// task function pointer type
typedef void (*task_func_t)(void);

// initialize the scheduler - must be called before any other scheduler functions
void scheduler_init(void);

// add a new task to the scheduler
// returns task ID (0-255) on success, -1 on failure
int8_t scheduler_add_task(task_func_t task_function);

// start the scheduler - this function never returns
void scheduler_start(void) __attribute__((noreturn));

// suspend a task
void scheduler_suspend_task(uint8_t task_id);

// resume a suspended task
void scheduler_resume_task(uint8_t task_id);

// yield cpu to next task (voluntary context switch)
void scheduler_yield(void);

// delay current task for specified number of system ticks
// task will be blocked and other tasks will run during this time
// ticks: number of system ticks to delay (1 tick = 1ms by default)
void task_delay(uint16_t ticks);

// get the current running task id
uint8_t scheduler_get_current_task(void);

// get number of active tasks
uint8_t scheduler_get_task_count(void);

#ifdef SCHEDULER_DEBUG
// get debug statistics for the scheduler
const scheduler_debug_t* scheduler_get_debug_stats(void);

// get debug statistics for a specific task
// returns 0 on success, -1 on error
int8_t scheduler_get_task_stats(uint8_t task_id, uint32_t *runtime_ticks, uint32_t *times_scheduled);

// reset debug statistics
void scheduler_reset_debug_stats(void);

// print debug statistics to uart (if uart is initialized)
// requires uart to be initialized separately
void scheduler_print_debug_stats(void);
#endif

#endif // SCHEDULER_H

