// servo control example using avr round robin scheduler
// demonstrates controlling multiple servo motors
// servo 1: sweeps left to right continuously
// servo 2: moves to specific positions in a pattern
// servo 3: controlled by a simulated input
// target: arduino uno (atmega328p)
// connections:
//   - servo 1: pin 9 (pb1 - oc1a)
//   - servo 2: pin 10 (pb2 - oc1b)
//   - status led: pin 13 (pb5)
// servo signals: 1000us (0°) to 2000us (180°), 50hz refresh rate

#include "scheduler.h"
#include <avr/io.h>

// servo pulse widths in timer ticks (16mhz / 8 prescaler = 2mhz = 0.5us per tick)
#define SERVO_MIN  2000   // 1000us = 0°
#define SERVO_MID  3000   // 1500us = 90°
#define SERVO_MAX  4000   // 2000us = 180°
#define SERVO_PERIOD 40000 // 20ms period for 50hz

// current servo positions
static volatile uint16_t servo1_position = SERVO_MID;
static volatile uint16_t servo2_position = SERVO_MID;

// initialize timer1 for servo pwm generation
// using fast pwm with icr1 as top
void servo_init(void) {
    // set servo pins as outputs
    DDRB |= (1 << PB1) | (1 << PB2);  // pins 9 and 10
    
    // configure timer1 for fast pwm, icr1 as top
    // com1a1, com1b1: clear on compare match, set at bottom
    // wgm13, wgm12, wgm11: fast pwm mode 14
    TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);  // prescaler = 8
    
    // set top value for 50hz (20ms period)
    ICR1 = SERVO_PERIOD;
    
    // initialize servo positions to center
    OCR1A = servo1_position;
    OCR1B = servo2_position;
}

// set servo 1 position (0-180 degrees)
void set_servo1(uint8_t angle) {
    if (angle > 180) angle = 180;
    
    // map angle (0-180) to pulse width (servo_min to servo_max)
    servo1_position = SERVO_MIN + ((uint32_t)angle * (SERVO_MAX - SERVO_MIN)) / 180;
    OCR1A = servo1_position;
}

// set servo 2 position (0-180 degrees)
void set_servo2(uint8_t angle) {
    if (angle > 180) angle = 180;
    
    servo2_position = SERVO_MIN + ((uint32_t)angle * (SERVO_MAX - SERVO_MIN)) / 180;
    OCR1B = servo2_position;
}

// task 1: servo 1 - smooth sweeping motion
void servo1_sweep_task(void) {
    uint8_t angle = 0;
    int8_t direction = 1;
    
    while (1) {
        // move servo
        set_servo1(angle);
        
        // update angle
        angle += direction * 2;  // move by 2 degrees
        
        // reverse at endpoints
        if (angle >= 180) {
            angle = 180;
            direction = -1;
        } else if (angle <= 0) {
            angle = 0;
            direction = 1;
        }
        
        // delay for smooth motion
        task_delay(20);  // 20ms = ~11 degrees/second
    }
}

// task 2: servo 2 - specific position pattern
void servo2_pattern_task(void) {
    const uint8_t positions[] = {0, 45, 90, 135, 180, 135, 90, 45};
    const uint8_t num_positions = sizeof(positions) / sizeof(positions[0]);
    uint8_t index = 0;
    
    while (1) {
        // move to position
        set_servo2(positions[index]);
        
        // hold position
        task_delay(1000);
        
        // next position
        index = (index + 1) % num_positions;
    }
}

// task 3: simulated sensor reading and servo control
// in a real application, this might read from a potentiometer or sensor
void servo_control_task(void) {
    uint8_t simulated_input = 0;
    int8_t direction = 1;
    
    while (1) {
        // simulate reading a sensor value (0-180)
        simulated_input += direction * 5;
        
        if (simulated_input >= 180) {
            simulated_input = 180;
            direction = -1;
        } else if (simulated_input <= 0) {
            simulated_input = 0;
            direction = 1;
        }
        
        // in a real application, you might control a third servo here
        // or adjust the existing servos based on sensor input
        
        task_delay(100);  // read every 100ms
    }
}

// task 4: status led heartbeat
void status_led_task(void) {
    // set pin 13 (pb5) as output
    DDRB |= (1 << PB5);
    
    while (1) {
        // quick double blink
        PORTB |= (1 << PB5);
        task_delay(100);
        PORTB &= ~(1 << PB5);
        task_delay(100);
        
        PORTB |= (1 << PB5);
        task_delay(100);
        PORTB &= ~(1 << PB5);
        
        // long pause
        task_delay(1000);
    }
}

int main(void) {
    servo_init();
    
    scheduler_init();
    
    scheduler_add_task(servo1_sweep_task);
    scheduler_add_task(servo2_pattern_task);
    scheduler_add_task(servo_control_task);
    scheduler_add_task(status_led_task);
    
    scheduler_start();
    
    return 0;
}

