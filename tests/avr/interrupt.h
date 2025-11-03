/*
 * Mock avr/interrupt.h for host testing
 */

#ifndef _AVR_INTERRUPT_H_
#define _AVR_INTERRUPT_H_

// Mock interrupt control
#define cli() do { } while(0)
#define sei() do { } while(0)

// Mock ISR macro
#define ISR(vector, ...) void vector(void)

// Mock interrupt vectors
#define TIMER0_COMPA_vect timer0_compare_isr

#endif // _AVR_INTERRUPT_H_

