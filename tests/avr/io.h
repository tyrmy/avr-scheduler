/*
 * Mock avr/io.h for host testing
 */

#ifndef _AVR_IO_H_
#define _AVR_IO_H_

#include <stdint.h>

// Mock AVR registers
extern uint8_t mock_TCCR0A;
extern uint8_t mock_TCCR0B;
extern uint8_t mock_OCR0A;
extern uint8_t mock_TIMSK0;
extern uint8_t mock_SREG;

#define TCCR0A mock_TCCR0A
#define TCCR0B mock_TCCR0B
#define OCR0A mock_OCR0A
#define TIMSK0 mock_TIMSK0
#define SREG mock_SREG

// Mock register bit positions
#define WGM01  1
#define CS01   1
#define CS00   0
#define OCIE0A 1

#endif // _AVR_IO_H_

