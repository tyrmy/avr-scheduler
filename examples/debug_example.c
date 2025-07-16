// debug statistics example using avr round robin scheduler
// demonstrates debug tracing: system ticks, context switches, runtime, yields
// prints stats via uart at 9600 baud
// target: arduino uno (atmega328p)
// connections:
//   - uart tx: arduino tx (connect to usb-serial)
//   - led: pin 13 (pb5) - status indicator

#include "scheduler.h"
#include <avr/io.h>
#include <stdio.h>

#ifdef SCHEDULER_DEBUG

// uart buffer for printf support
static int uart_putchar(char c, FILE *stream);
static FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

// initialize uart for debug output (9600 baud @ 16MHz)
void uart_init(void) {
    // set baud rate to 9600 @ 16MHz
    uint16_t ubrr = 103;  // 16MHz / (16 * 9600) - 1
    
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;
    
    // enable transmitter
    UCSR0B = (1 << TXEN0);
    
    // set frame format: 8 data bits, 1 stop bit, no parity
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    
    // set stdout to uart
    stdout = &uart_output;
}

// send character via uart (for printf support)
static int uart_putchar(char c, FILE *stream) {
    if (c == '\n') {
        uart_putchar('\r', stream);
    }
    
    // wait for empty transmit buffer
    while (!(UCSR0A & (1 << UDRE0)));
    
    // put data into buffer, sends the data
    UDR0 = c;
    
    return 0;
}

// task 1: busy work simulation (short delays)
void task_busy(void) {
    volatile uint16_t counter = 0;
    
    while (1) {
        // do some busy work
        for (uint16_t i = 0; i < 100; i++) {
            counter++;
        }
        
        // short delay
        task_delay(50);  // 50ms
    }
}

// task 2: medium work with longer delays
void task_medium(void) {
    while (1) {
        // simulate some work
        volatile uint8_t dummy = 0;
        for (uint8_t i = 0; i < 50; i++) {
            dummy += i;
        }
        
        // medium delay
        task_delay(150);  // 150ms
    }
}

// task 3: idle task (mostly sleeping)
void task_idle(void) {
    while (1) {
        scheduler_yield();
        task_delay(500);  // 500ms
    }
}

// task 4: debug statistics reporter - prints stats every 5 seconds
void task_debug_reporter(void) {
    uint16_t report_interval = 5000;  // 5 seconds
    
    // wait a bit before first report
    task_delay(report_interval);
    
    while (1) {
        // get debug statistics
        const scheduler_debug_t *stats = scheduler_get_debug_stats();
        
        // print header
        printf("\n========================================\n");
        printf("Scheduler Debug Statistics\n");
        printf("========================================\n");
        printf("Total System Ticks:  %lu\n", stats->total_ticks);
        printf("Context Switches:    %lu\n", stats->context_switches);
        printf("Voluntary Yields:    %lu\n", stats->voluntary_yields);
        
        // calculate and print utilization
        if (stats->total_ticks > 0) {
            printf("Avg Switch Rate:     %lu switches/sec\n", 
                   (stats->context_switches * 1000UL) / stats->total_ticks);
        }
        
        printf("\nPer-Task Statistics:\n");
        printf("----------------------------------------\n");
        
        // print statistics for each task
        uint8_t task_count = scheduler_get_task_count();
        for (uint8_t i = 0; i < task_count; i++) {
            uint32_t runtime, scheduled;
            
            if (scheduler_get_task_stats(i, &runtime, &scheduled) == 0) {
                printf("Task %u: ", i);
                printf("Runtime=%lu ticks ", runtime);
                
                // calculate cpu percentage
                if (stats->total_ticks > 0) {
                    uint16_t cpu_percent = (uint16_t)((runtime * 100UL) / stats->total_ticks);
                    printf("(%u%% CPU), ", cpu_percent);
                }
                
                printf("Scheduled=%lu times\n", scheduled);
                
                // calculate average runtime per schedule
                if (scheduled > 0) {
                    uint32_t avg_runtime = runtime / scheduled;
                    printf("        Avg Runtime: %lu ticks/schedule\n", avg_runtime);
                }
            }
        }
        
        printf("========================================\n\n");
        
        // wait for next report
        task_delay(report_interval);
    }
}

// task 5: led blinker (visual indicator)
void task_led_blink(void) {
    // set pin 13 (pb5) as output
    DDRB |= (1 << PB5);
    
    while (1) {
        PORTB ^= (1 << PB5);  // toggle led
        task_delay(1000);      // 1 second
    }
}

int main(void) {
    uart_init();
    
    printf("\n\n");
    printf("========================================\n");
    printf("AVR Scheduler Debug Example\n");
    printf("========================================\n");
    printf("Starting scheduler with debug tracing...\n");
    printf("Statistics will be reported every 5 seconds.\n\n");
    
    scheduler_init();
    
    printf("Adding tasks...\n");
    scheduler_add_task(task_busy);           // task 0 - busy
    scheduler_add_task(task_medium);         // task 1 - medium
    scheduler_add_task(task_idle);           // task 2 - idle
    scheduler_add_task(task_debug_reporter); // task 3 - debug
    scheduler_add_task(task_led_blink);      // task 4 - led
    
    printf("Tasks added: %u\n", scheduler_get_task_count());
    printf("Starting scheduler...\n\n");
    
    // small delay to let uart finish
    for (volatile uint32_t i = 0; i < 100000; i++);
    
    scheduler_start();
    
    return 0;
}

#else
// if debug is not enabled, provide a simple error message
#warning "SCHEDULER_DEBUG is not enabled! This example requires debug support."

int main(void) {
    // infinite loop with led blinking to indicate error
    DDRB |= (1 << PB5);
    
    while(1) {
        PORTB ^= (1 << PB5);
        for (volatile uint32_t i = 0; i < 100000; i++);
    }
}

#endif // SCHEDULER_DEBUG

