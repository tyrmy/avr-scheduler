// pwm motor control example
// demonstrates controlling two dc motors with pwm
// motor a: gradually increases/decreases speed (ramp pattern)
// motor b: pulses on and off
// motor c: manual control task that could read inputs
// target: arduino uno (atmega328p)
// connections:
//   - motor a pwm: pin 9 (pb1 - oc1a)
//   - motor a dir: pin 8 (pb0)
//   - motor b pwm: pin 10 (pb2 - oc1b)
//   - motor b dir: pin 7 (pd7)

#include "scheduler.h"
#include <avr/io.h>

// motor a pins
#define MOTOR_A_PWM_PIN PB1  // arduino d9 - oc1a
#define MOTOR_A_DIR_PIN PB0  // arduino d8

// motor b pins
#define MOTOR_B_PWM_PIN PB2  // arduino d10 - oc1b
#define MOTOR_B_DIR_PIN PD7  // arduino d7

// motor directions
#define MOTOR_FORWARD  1
#define MOTOR_BACKWARD 0

// pwm range: 0-255 (using 8-bit pwm)
#define PWM_MAX 255

// initialize pwm for motor control
// using timer1 in fast pwm mode (8-bit)
void pwm_init(void) {
    // set pwm pins as outputs
    DDRB |= (1 << MOTOR_A_PWM_PIN) | (1 << MOTOR_B_PWM_PIN);
    
    // set direction pins as outputs
    DDRB |= (1 << MOTOR_A_DIR_PIN);
    DDRD |= (1 << MOTOR_B_DIR_PIN);
    
    // configure timer1 for fast pwm, 8-bit
    // com1a1, com1b1: clear on compare match, set at bottom (non-inverting mode)
    // wgm12, wgm10: fast pwm, 8-bit (top = 0x00ff)
    TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM10);
    
    // cs11: prescaler = 8 (pwm frequency ~7.8khz at 16mhz)
    TCCR1B = (1 << WGM12) | (1 << CS11);
    
    // initialize pwm duty cycles to 0
    OCR1A = 0;
    OCR1B = 0;
}

// set motor a speed and direction
void set_motor_a(uint8_t speed, uint8_t direction) {
    if (direction == MOTOR_FORWARD) {
        PORTB |= (1 << MOTOR_A_DIR_PIN);
    } else {
        PORTB &= ~(1 << MOTOR_A_DIR_PIN);
    }
    OCR1A = speed;
}

// set motor b speed and direction
void set_motor_b(uint8_t speed, uint8_t direction) {
    if (direction == MOTOR_FORWARD) {
        PORTD |= (1 << MOTOR_B_DIR_PIN);
    } else {
        PORTD &= ~(1 << MOTOR_B_DIR_PIN);
    }
    OCR1B = speed;
}

// task 1: motor a - ramp speed up and down
void motor_a_ramp_task(void) {
    uint8_t speed = 0;
    uint8_t direction = MOTOR_FORWARD;
    
    while (1) {
        // ramp up
        for (speed = 0; speed < PWM_MAX; speed += 5) {
            set_motor_a(speed, direction);
            task_delay(50);  // 50ms delay between steps
        }
        
        // hold at max speed
        task_delay(1000);
        
        // ramp down
        for (speed = PWM_MAX; speed > 0; speed -= 5) {
            set_motor_a(speed, direction);
            task_delay(50);
        }
        
        // stop briefly
        set_motor_a(0, direction);
        task_delay(500);
        
        // change direction
        direction = !direction;
    }
}

// task 2: motor b - pulse pattern
void motor_b_pulse_task(void) {
    while (1) {
        // fast pulse
        set_motor_b(200, MOTOR_FORWARD);
        task_delay(200);
        
        set_motor_b(0, MOTOR_FORWARD);
        task_delay(200);
        
        // medium pulse
        set_motor_b(150, MOTOR_BACKWARD);
        task_delay(400);
        
        set_motor_b(0, MOTOR_BACKWARD);
        task_delay(400);
        
        // slow pulse
        set_motor_b(100, MOTOR_FORWARD);
        task_delay(800);
        
        set_motor_b(0, MOTOR_FORWARD);
        task_delay(800);
    }
}

// task 3: safety monitor
// in a real app, this would monitor temperature, current, etc.
void safety_monitor_task(void) {
    uint16_t runtime = 0;
    
    while (1) {
        task_delay(1000);  // check every second
        runtime++;
        
        // example: emergency stop after 30 seconds (for demo)
        if (runtime >= 30) {
            set_motor_a(0, MOTOR_FORWARD);
            set_motor_b(0, MOTOR_FORWARD);
            runtime = 0;  // reset counter
            task_delay(5000);  // pause for 5 seconds
        }
    }
}

// task 4: status indicator (blink led on pin 13)
void status_led_task(void) {
    // set pin 13 (pb5) as output
    DDRB |= (1 << PB5);
    
    while (1) {
        PORTB ^= (1 << PB5);  // toggle led
        task_delay(1000);      // 1 second blink
    }
}

int main(void) {
    // initialize pwm for motors
    pwm_init();
    
    scheduler_init();
    
    scheduler_add_task(motor_a_ramp_task);
    scheduler_add_task(motor_b_pulse_task);
    scheduler_add_task(safety_monitor_task);
    scheduler_add_task(status_led_task);
    
    scheduler_start();
    
    return 0;
}

