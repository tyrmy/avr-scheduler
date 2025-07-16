// example application demonstrating the avr round robin scheduler
// creates three tasks that blink different leds
// uses task_delay() for scheduler-aware delays
// target: arduino uno (atmega328p) or similar

#include "scheduler.h"
#include <avr/io.h>

// led pins on portb (arduino uno digital pins 8-13)
#define LED1 PB0  // arduino d8
#define LED2 PB1  // arduino d9
#define LED3 PB2  // arduino d10

// task 1: blink led1 slowly (500ms)
// uses task_delay() which allows other tasks to run during the delay
void task1(void) {
    while (1) {
        PORTB ^= (1 << LED1);  // toggle led1
        task_delay(500);       // delay 500ms (500 ticks)
    }
}

// task 2: blink led2 at medium speed (300ms)
void task2(void) {
    while (1) {
        PORTB ^= (1 << LED2);  // toggle led2
        task_delay(300);       // delay 300ms (300 ticks)
    }
}

// task 3: blink led3 quickly (200ms)
void task3(void) {
    while (1) {
        PORTB ^= (1 << LED3);  // toggle led3
        task_delay(200);       // delay 200ms (200 ticks)
    }
}

// task 4: idle task - just yields
// this task runs when other tasks are delayed
void task_idle(void) {
    while (1) {
        scheduler_yield();     // give up cpu voluntarily
        task_delay(100);       // delay 100ms
    }
}

int main(void) {
    // configure led pins as outputs
    DDRB |= (1 << LED1) | (1 << LED2) | (1 << LED3);
    
    // initialize leds to off
    PORTB &= ~((1 << LED1) | (1 << LED2) | (1 << LED3));
    
    scheduler_init();
    
    scheduler_add_task(task1);
    scheduler_add_task(task2);
    scheduler_add_task(task3);
    scheduler_add_task(task_idle);
    
    scheduler_start();
    
    return 0;
}

