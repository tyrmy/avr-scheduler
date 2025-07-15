# AVR Round Robin Scheduler

A lightweight, cooperative round-robin scheduler for AVR microcontrollers written in C. This scheduler enables multitasking on resource-constrained AVR platforms like Arduino Uno (ATmega328P).

## Hardware Requirements

- AVR microcontroller (ATmega328P, ATmega2560, etc.)
- Minimum 2KB RAM recommended
- Timer0 available for scheduler

## Project Structure

```
avr-scheduler/
├── scheduler.h          # Scheduler API header
├── scheduler.c          # Scheduler implementation
├── Makefile            # Build system
├── README.md           # This file
├── DEBUG.md            # Debug tracing documentation
├── FLASHING.md         # Guide for blank chips and ISP programming
└── examples/           # Example applications
    ├── led_example.c           # Basic LED blinking
    ├── pwm_motor_example.c     # DC motor control
    ├── servo_example.c         # Servo motor control
    ├── stepper_example.c       # Stepper motor control
    ├── debug_example.c         # Debug statistics demo
    └── README.md              # Examples documentation
```

## Quick Start

```bash
# Build LED example (default)
make

# Flash to Arduino Uno
make flash

# Build and flash a different example
make EXAMPLE=pwm_motor_example flash

# List all available examples
make list-examples
```

See `led_example.c` for a how to get started on creating custom applications.

Use `make list-examples` to see all available demos.

## Non-arduino flashing

```
# Set fuses (16MHz crystal)
avrdude -p atmega328p -c usbasp -U lfuse:w:0xFF:m -U hfuse:w:0xD9:m -U efuse:w:0xFD:m

# Flash your code
make PROGRAMMER=usbasp EXAMPLE=led_example flash
```

## Debug Tracing

The scheduler includes built-in debug tracing to monitor performance:

```c
#ifdef SCHEDULER_DEBUG
const scheduler_debug_t *stats = scheduler_get_debug_stats();
printf("System ticks: %lu\n", stats->total_ticks);
printf("Context switches: %lu\n", stats->context_switches);

// Get per-task statistics
uint32_t runtime, scheduled;
scheduler_get_task_stats(0, &runtime, &scheduled);
printf("Task 0: %lu ticks runtime, %lu times scheduled\n", runtime, scheduled);
#endif
```

**Features:**
- Track total system ticks (uptime)
- Count context switches
- Monitor voluntary yields
- Measure per-task CPU usage
- Calculate task responsiveness

See `DEBUG.md` for complete documentation and `examples/debug_example.c` for a working demonstration.

**To disable debug** (saves ~44 bytes RAM): Comment out `#define SCHEDULER_DEBUG` in `scheduler.h`

## License

This project is provided as-is for educational and commercial use.

## Contributing

Contributions welcome! Areas for improvement:
- Priority-based scheduling
- Semaphores/mutexes
- Message queues
- Dynamic task management