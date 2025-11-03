/*
 * AVR Mock Header for Host Testing
 * 
 * This file mocks AVR-specific headers and registers for testing
 * scheduler logic on the host system without AVR toolchain.
 * 
 * IMPORTANT: This must be included BEFORE any scheduler headers.
 */

#ifndef AVR_MOCK_H
#define AVR_MOCK_H

#include <stdint.h>

// Create fake avr/io.h and avr/interrupt.h to prevent compilation errors
// We do this by creating the expected header guard macros

// Prevent real AVR headers from being included
#ifndef _AVR_IO_H_
#define _AVR_IO_H_ 1
#endif

#ifndef _AVR_SFR_DEFS_H_
#define _AVR_SFR_DEFS_H_ 1
#endif

#ifndef _AVR_INTERRUPT_H_
#define _AVR_INTERRUPT_H_ 1
#endif

// Mock F_CPU
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

// Mock AVR registers
static uint8_t mock_TCCR0A = 0;
static uint8_t mock_TCCR0B = 0;
static uint8_t mock_OCR0A = 0;
static uint8_t mock_TIMSK0 = 0;
static uint8_t mock_SREG = 0;

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

// Mock interrupt control
#define cli() do { } while(0)
#define sei() do { } while(0)

// Mock ISR macro
#define ISR(vector, ...) void vector(void)
#define TIMER0_COMPA_vect timer0_compare_isr

// Mock interrupt vector names
#define TIMER0_COMPA_vect_num 1

// Mock stack pointer operations (inline assembly won't work on host)
#define __SP_L__ 0x3D
#define __SP_H__ 0x3E
#define __SREG__ 0x3F

#endif // AVR_MOCK_H

