# AVR Scheduler Test Suite

This directory contains comprehensive unit tests for the AVR scheduler, including both host-based tests and hardware tests for AVR microcontrollers.

## Test Types

### 1. Host-Based Tests (`host_test.c`)

Quick validation tests that run on your development machine without requiring AVR hardware. These tests mock AVR-specific functionality and validate the scheduler logic.

**Run with:**
```bash
make host-test
# or simply
make
```

### 2. Hardware Tests (`scheduler_test.c`)

Comprehensive integration tests that run on actual AVR hardware (or simulator). These tests validate the complete system including context switching, stack integrity, and real-time behavior.

**Build and flash:**
```bash
make avr          # Build only
make flash        # Flash to board
make test-avr     # Build and flash in one step
make monitor      # View serial output
```

## Quick Start

### Running Host Tests

```bash
cd tests
make
```

Expected output:
```
Running test: test_scheduler_init ... PASS
Running test: test_add_single_task ... PASS
Running test: test_add_multiple_tasks ... PASS
...
✓ All tests passed!
```

### Running Hardware Tests

1. **Connect your AVR board** (e.g., Arduino Uno with ATmega328P)

2. **Build and flash the test:**
   ```bash
   cd tests
   make test-avr
   ```

3. **Monitor the output:**
   ```bash
   make monitor
   # or use your preferred serial terminal at 9600 baud
   ```

4. **Expected output:**
   ```
   ===================================
     AVR Scheduler Unit Test Suite
   ===================================
   
   Starting context switching test with stack integrity validation...
   Tasks added successfully. Starting scheduler...
   
   === Context Switch Test Results ===
   Task 1 executed: 100 times
   Task 2 executed: 100 times
   Task 1 final checksum: 0xABCD1234
   Task 2 final checksum: 0x56789ABC
   
   *** ALL TESTS PASSED ***
   Stack integrity maintained across context switches!
   ```

## Configuration

Edit the `Makefile` to change AVR settings:

```makefile
MCU = atmega328p          # Target microcontroller
F_CPU = 16000000UL        # Clock frequency
PROGRAMMER = arduino      # Programmer type
PORT = /dev/ttyUSB0       # Serial port
```

Or override on command line:
```bash
make MCU=atmega2560 PORT=/dev/ttyACM0 test-avr
```