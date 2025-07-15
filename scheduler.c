#include "scheduler.h"
#include <avr/interrupt.h>
#include <string.h>

// task control blocks
static task_t tasks[MAX_TASKS];
static uint8_t task_count = 0;
static uint8_t current_task = 0;
static volatile uint8_t scheduler_running = 0;

#ifdef SCHEDULER_DEBUG
// debug statistics
static volatile scheduler_debug_t debug_stats = {0, 0, 0};
#endif

// forward declarations
static uint8_t* init_stack(uint8_t *stack_top, task_func_t task_function);
static void task_exit(void);

// initialize the scheduler
void scheduler_init(void) {
    task_count = 0;
    current_task = 0;
    scheduler_running = 0;
    
    // clear all task control blocks
    memset(tasks, 0, sizeof(tasks));
    
#ifdef SCHEDULER_DEBUG
    // reset debug statistics
    debug_stats.total_ticks = 0;
    debug_stats.context_switches = 0;
    debug_stats.voluntary_yields = 0;
#endif
    
    // configure timer0 for context switching (1ms tick)
    // assuming 16MHz clock
    TCCR0A = (1 << WGM01);  // ctc mode
    TCCR0B = (1 << CS01) | (1 << CS00);  // prescaler 64
    OCR0A = 249;  // 16MHz / 64 / 250 = 1000Hz (1ms)
    TIMSK0 = (1 << OCIE0A);  // enable compare match interrupt
}

// initialize a task's stack
static uint8_t* init_stack(uint8_t *stack_top, task_func_t task_function) {
    uint16_t func_addr = (uint16_t)task_function;
    
    // simulate what the stack looks like after a context save
    // push return address (task exit handler)
    *stack_top-- = (uint16_t)task_exit & 0xFF;
    *stack_top-- = ((uint16_t)task_exit >> 8) & 0xFF;
    
    // push task function address (where task will start)
    *stack_top-- = func_addr & 0xFF;
    *stack_top-- = (func_addr >> 8) & 0xFF;
    
    // push r0 (zero register)
    *stack_top-- = 0x00;
    
    // push sreg (status register) with interrupts enabled
    *stack_top-- = 0x80;
    
    // push r1-r31 (general purpose registers)
    for (uint8_t i = 0; i < 31; i++) {
        *stack_top-- = 0x00;
    }
    
    return stack_top;
}

// task exit handler (called if task function returns)
static void task_exit(void) {
    // mark task as blocked if it returns
    tasks[current_task].state = TASK_BLOCKED;
    
    // yield to next task
    while(1) {
        scheduler_yield();
    }
}

// add a new task to the scheduler
int8_t scheduler_add_task(task_func_t task_function) {
    if (task_count >= MAX_TASKS || task_function == NULL) {
        return -1;
    }
    
    uint8_t task_id = task_count;
    
    // initialize task control block
    tasks[task_id].task_id = task_id;
    tasks[task_id].state = TASK_READY;
    tasks[task_id].delay_ticks = 0;
    
    // initialize stack (point to top of stack)
    uint8_t *stack_top = &tasks[task_id].stack[TASK_STACK_SIZE - 1];
    tasks[task_id].stack_pointer = init_stack(stack_top, task_function);
    
    task_count++;
    
    return task_id;
}

// start the scheduler
void scheduler_start(void) {
    if (task_count == 0) {
        // no tasks to run
        while(1);
    }
    
    // set first task as running
    current_task = 0;
    tasks[current_task].state = TASK_RUNNING;
    scheduler_running = 1;
    
    // enable global interrupts
    sei();
    
    // load the first task's context and start running
    // this is done by manually restoring the first task's stack
    uint8_t *sp = tasks[current_task].stack_pointer;
    
    // set stack pointer and jump to task
    asm volatile (
        "out __SP_L__, %A0 \n\t"
        "out __SP_H__, %B0 \n\t"
        :
        : "r" (sp)
    );
    
    // restore registers and start task
    asm volatile (
        "pop r31 \n\t"
        "pop r30 \n\t"
        "pop r29 \n\t"
        "pop r28 \n\t"
        "pop r27 \n\t"
        "pop r26 \n\t"
        "pop r25 \n\t"
        "pop r24 \n\t"
        "pop r23 \n\t"
        "pop r22 \n\t"
        "pop r21 \n\t"
        "pop r20 \n\t"
        "pop r19 \n\t"
        "pop r18 \n\t"
        "pop r17 \n\t"
        "pop r16 \n\t"
        "pop r15 \n\t"
        "pop r14 \n\t"
        "pop r13 \n\t"
        "pop r12 \n\t"
        "pop r11 \n\t"
        "pop r10 \n\t"
        "pop r9  \n\t"
        "pop r8  \n\t"
        "pop r7  \n\t"
        "pop r6  \n\t"
        "pop r5  \n\t"
        "pop r4  \n\t"
        "pop r3  \n\t"
        "pop r2  \n\t"
        "pop r1  \n\t"
        "out __SREG__, r1 \n\t"
        "pop r1  \n\t"
        "pop r0  \n\t"
        "ret     \n\t"
    );
    
    // should never reach here
    while(1);
}

// timer interrupt - triggers context switch
// simplified version using normal isr (not naked)
// the avr isr macro already saves sreg and other necessary registers
ISR(TIMER0_COMPA_vect) {
    if (!scheduler_running || task_count == 0) {
        return;
    }
    
#ifdef SCHEDULER_DEBUG
    // increment total system ticks
    debug_stats.total_ticks++;
    
    // track runtime for current task
    if (tasks[current_task].state == TASK_RUNNING) {
        tasks[current_task].runtime_ticks++;
    }
#endif
    
    // process delay timers for all tasks
    for (uint8_t i = 0; i < task_count; i++) {
        if (tasks[i].delay_ticks > 0) {
            tasks[i].delay_ticks--;
            // wake up task if delay expired
            if (tasks[i].delay_ticks == 0 && tasks[i].state == TASK_BLOCKED) {
                tasks[i].state = TASK_READY;
            }
        }
    }
}

// suspend a task
void scheduler_suspend_task(uint8_t task_id) {
    if (task_id < task_count) {
        tasks[task_id].state = TASK_SUSPENDED;
    }
}

// resume a suspended task
void scheduler_resume_task(uint8_t task_id) {
    if (task_id < task_count && tasks[task_id].state == TASK_SUSPENDED) {
        tasks[task_id].state = TASK_READY;
    }
}

// voluntary yield
void scheduler_yield(void) {
#ifdef SCHEDULER_DEBUG
    // track voluntary yields
    debug_stats.voluntary_yields++;
#endif
    
    // find next ready task (round-robin)
    uint8_t next_task = current_task;
    uint8_t tasks_checked = 0;
    
    do {
        next_task = (next_task + 1) % task_count;
        tasks_checked++;
        
        if (tasks[next_task].state == TASK_READY || 
            tasks[next_task].state == TASK_RUNNING) {
            break;
        }
    } while (tasks_checked < task_count);
    
    // if we found a different task, perform context switch
    if (next_task != current_task && tasks[next_task].state != TASK_BLOCKED) {
#ifdef SCHEDULER_DEBUG
        debug_stats.context_switches++;
        tasks[next_task].times_scheduled++;
#endif
        
        // update task states
        if (tasks[current_task].state == TASK_RUNNING) {
            tasks[current_task].state = TASK_READY;
        }
        
        current_task = next_task;
        tasks[current_task].state = TASK_RUNNING;
        
        // context switch happens here - but since we're cooperative,
        // we just return and the calling function resumes
    }
}

// delay current task for specified ticks
void task_delay(uint16_t ticks) {
    if (ticks == 0) {
        return;
    }
    
    // disable interrupts temporarily
    uint8_t sreg = SREG;
    cli();
    
    // set delay counter and block task
    tasks[current_task].delay_ticks = ticks;
    tasks[current_task].state = TASK_BLOCKED;
    
    // restore interrupts
    SREG = sreg;
    
    // force context switch to another task
    scheduler_yield();
}

// get current task id
uint8_t scheduler_get_current_task(void) {
    return current_task;
}

// get task count
uint8_t scheduler_get_task_count(void) {
    return task_count;
}

#ifdef SCHEDULER_DEBUG
// get debug statistics
const scheduler_debug_t* scheduler_get_debug_stats(void) {
    return (const scheduler_debug_t*)&debug_stats;
}

// get task-specific debug statistics
int8_t scheduler_get_task_stats(uint8_t task_id, uint32_t *runtime_ticks, uint32_t *times_scheduled) {
    if (task_id >= task_count) {
        return -1;
    }
    
    if (runtime_ticks != NULL) {
        *runtime_ticks = tasks[task_id].runtime_ticks;
    }
    
    if (times_scheduled != NULL) {
        *times_scheduled = tasks[task_id].times_scheduled;
    }
    
    return 0;
}

// reset debug statistics
void scheduler_reset_debug_stats(void) {
    uint8_t sreg = SREG;
    cli();
    
    debug_stats.total_ticks = 0;
    debug_stats.context_switches = 0;
    debug_stats.voluntary_yields = 0;
    
    for (uint8_t i = 0; i < task_count; i++) {
        tasks[i].runtime_ticks = 0;
        tasks[i].times_scheduled = 0;
    }
    
    SREG = sreg;
}

// print debug statistics
// note: placeholder - actual implementation would need uart
// users should implement their own print function based on their uart setup
void scheduler_print_debug_stats(void) {
    // users can call scheduler_get_debug_stats() and scheduler_get_task_stats()
    // to retrieve the data and format it themselves
}
#endif

