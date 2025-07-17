// stepper motor control example using avr round robin scheduler
// demonstrates controlling a bipolar stepper motor
// using the scheduler for smooth, non-blocking motion control
// target: arduino uno (atmega328p)
// connections (for uln2003 or l298n driver):
//   - stepper coil a+: pin 8 (pb0)
//   - stepper coil a-: pin 9 (pb1)
//   - stepper coil b+: pin 10 (pb2)
//   - stepper coil b-: pin 11 (pb3)
//   - status led: pin 13 (pb5)

#include "scheduler.h"
#include <avr/io.h>

// stepper motor pins
#define STEP_A_PLUS  PB0  // pin 8
#define STEP_A_MINUS PB1  // pin 9
#define STEP_B_PLUS  PB2  // pin 10
#define STEP_B_MINUS PB3  // pin 11

// stepper motor configuration
#define STEPS_PER_REV 200  // 200 steps = 1.8° per step (standard stepper)

// step sequences
typedef enum {
    STEP_MODE_FULL,
    STEP_MODE_HALF
} step_mode_t;

// full-step sequence (4 steps per cycle)
static const uint8_t full_step_sequence[4] = {
    (1 << STEP_A_PLUS) | (0 << STEP_A_MINUS) | (0 << STEP_B_PLUS) | (0 << STEP_B_MINUS),
    (0 << STEP_A_PLUS) | (0 << STEP_A_MINUS) | (1 << STEP_B_PLUS) | (0 << STEP_B_MINUS),
    (0 << STEP_A_PLUS) | (1 << STEP_A_MINUS) | (0 << STEP_B_PLUS) | (0 << STEP_B_MINUS),
    (0 << STEP_A_PLUS) | (0 << STEP_A_MINUS) | (0 << STEP_B_PLUS) | (1 << STEP_B_MINUS),
};

// half-step sequence (8 steps per cycle)
static const uint8_t half_step_sequence[8] = {
    (1 << STEP_A_PLUS) | (0 << STEP_A_MINUS) | (0 << STEP_B_PLUS) | (0 << STEP_B_MINUS),
    (1 << STEP_A_PLUS) | (0 << STEP_A_MINUS) | (1 << STEP_B_PLUS) | (0 << STEP_B_MINUS),
    (0 << STEP_A_PLUS) | (0 << STEP_A_MINUS) | (1 << STEP_B_PLUS) | (0 << STEP_B_MINUS),
    (0 << STEP_A_PLUS) | (1 << STEP_A_MINUS) | (1 << STEP_B_PLUS) | (0 << STEP_B_MINUS),
    (0 << STEP_A_PLUS) | (1 << STEP_A_MINUS) | (0 << STEP_B_PLUS) | (0 << STEP_B_MINUS),
    (0 << STEP_A_PLUS) | (1 << STEP_A_MINUS) | (0 << STEP_B_PLUS) | (1 << STEP_B_MINUS),
    (0 << STEP_A_PLUS) | (0 << STEP_A_MINUS) | (0 << STEP_B_PLUS) | (1 << STEP_B_MINUS),
    (1 << STEP_A_PLUS) | (0 << STEP_A_MINUS) | (0 << STEP_B_PLUS) | (1 << STEP_B_MINUS),
};

// stepper state
static volatile uint8_t step_position = 0;
static volatile step_mode_t current_mode = STEP_MODE_FULL;

// initialize stepper motor pins
void stepper_init(void) {
    // set all stepper pins as outputs
    DDRB |= (1 << STEP_A_PLUS) | (1 << STEP_A_MINUS) | 
            (1 << STEP_B_PLUS) | (1 << STEP_B_MINUS);
    
    // start with all coils off
    PORTB &= ~((1 << STEP_A_PLUS) | (1 << STEP_A_MINUS) | 
               (1 << STEP_B_PLUS) | (1 << STEP_B_MINUS));
}

// set stepper coils based on current step position
void stepper_set_coils(uint8_t pattern) {
    // clear all stepper pins
    PORTB &= ~((1 << STEP_A_PLUS) | (1 << STEP_A_MINUS) | 
               (1 << STEP_B_PLUS) | (1 << STEP_B_MINUS));
    
    // set new pattern
    PORTB |= pattern;
}

// step the motor one position forward
void stepper_step_forward(void) {
    if (current_mode == STEP_MODE_FULL) {
        step_position = (step_position + 1) % 4;
        stepper_set_coils(full_step_sequence[step_position]);
    } else {
        step_position = (step_position + 1) % 8;
        stepper_set_coils(half_step_sequence[step_position]);
    }
}

// step the motor one position backward
void stepper_step_backward(void) {
    if (current_mode == STEP_MODE_FULL) {
        step_position = (step_position == 0) ? 3 : step_position - 1;
        stepper_set_coils(full_step_sequence[step_position]);
    } else {
        step_position = (step_position == 0) ? 7 : step_position - 1;
        stepper_set_coils(half_step_sequence[step_position]);
    }
}

// move stepper motor a specified number of steps
void stepper_move(int16_t steps, uint16_t speed_ms) {
    if (steps > 0) {
        // move forward
        for (int16_t i = 0; i < steps; i++) {
            stepper_step_forward();
            task_delay(speed_ms);
        }
    } else if (steps < 0) {
        // move backward
        for (int16_t i = 0; i > steps; i--) {
            stepper_step_backward();
            task_delay(speed_ms);
        }
    }
}

// task 1: continuous rotation in full-step mode
void stepper_continuous_rotation_task(void) {
    current_mode = STEP_MODE_FULL;
    
    while (1) {
        // rotate one full revolution clockwise (fast)
        stepper_move(STEPS_PER_REV, 5);  // 5ms per step = 1 rev/second
        
        // pause
        task_delay(1000);
        
        // rotate one full revolution counter-clockwise (slow)
        stepper_move(-STEPS_PER_REV, 10);  // 10ms per step = 0.5 rev/second
        
        // pause
        task_delay(1000);
    }
}

// task 2: precise positioning pattern
void stepper_positioning_task(void) {
    const int16_t positions[] = {50, 100, -75, 0, 150, -150, 0};
    const uint8_t num_positions = sizeof(positions) / sizeof(positions[0]);
    uint8_t index = 0;
    int16_t current_pos = 0;
    
    // start in half-step mode for finer control
    current_mode = STEP_MODE_HALF;
    
    while (1) {
        // calculate steps needed to reach target position
        int16_t target = positions[index];
        int16_t steps_needed = target - current_pos;
        
        // move to position
        stepper_move(steps_needed, 8);
        current_pos = target;
        
        // hold position
        task_delay(2000);
        
        // next position
        index = (index + 1) % num_positions;
    }
}

// task 3: simulated acceleration/deceleration
// demonstrates smooth speed ramping
void stepper_accel_task(void) {
    current_mode = STEP_MODE_FULL;
    
    while (1) {
        // accelerate from slow to fast
        for (uint16_t speed = 20; speed > 2; speed -= 2) {
            stepper_step_forward();
            task_delay(speed);
        }
        
        // run at max speed for some steps
        for (uint8_t i = 0; i < 100; i++) {
            stepper_step_forward();
            task_delay(2);
        }
        
        // decelerate from fast to slow
        for (uint16_t speed = 2; speed < 20; speed += 2) {
            stepper_step_forward();
            task_delay(speed);
        }
        
        // stop and reverse direction
        task_delay(1000);
        
        // repeat in reverse
        for (uint16_t speed = 20; speed > 2; speed -= 2) {
            stepper_step_backward();
            task_delay(speed);
        }
        
        for (uint8_t i = 0; i < 100; i++) {
            stepper_step_backward();
            task_delay(2);
        }
        
        for (uint16_t speed = 2; speed < 20; speed += 2) {
            stepper_step_backward();
            task_delay(speed);
        }
        
        task_delay(1000);
    }
}

// task 4: status led and heartbeat
void status_led_task(void) {
    // set pin 13 (pb5) as output
    DDRB |= (1 << PB5);
    
    while (1) {
        PORTB ^= (1 << PB5);  // toggle led
        task_delay(500);       // 0.5 second blink
    }
}

int main(void) {
    stepper_init();
    
    scheduler_init();
    
    // add tasks - choose one of the stepper tasks to avoid conflicts
    // comment out the ones you don't want to run
    
    scheduler_add_task(stepper_continuous_rotation_task);
    // scheduler_add_task(stepper_positioning_task);
    // scheduler_add_task(stepper_accel_task);
    
    scheduler_add_task(status_led_task);
    
    scheduler_start();
    
    return 0;
}

