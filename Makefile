# Makefile for AVR Round Robin Scheduler
# Default target: Arduino Uno (ATmega328P)

# MCU Configuration
MCU = atmega328p
F_CPU = 16000000UL
BAUD = 115200

# Programmer Configuration (change as needed)
PROGRAMMER = arduino
PORT = /dev/ttyACM0

# Compiler and Tools
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
AVRDUDE = avrdude

# Compiler Flags
CFLAGS = -Wall -Wextra -Os -g -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DBAUD=$(BAUD)
CFLAGS += -std=gnu99
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -I.

# Linker Flags
LDFLAGS = -mmcu=$(MCU)
LDFLAGS += -Wl,-Map=$(TARGET).map,--cref
LDFLAGS += -Wl,--gc-sections

# Example Selection (default: led_example)
EXAMPLE ?= led_example

# Available examples
EXAMPLES = led_example pwm_motor_example servo_example stepper_example debug_example

# Source Files
SCHEDULER_SOURCES = scheduler.c
EXAMPLE_SOURCE = examples/$(EXAMPLE).c
SOURCES = $(SCHEDULER_SOURCES) $(EXAMPLE_SOURCE)
OBJECTS = $(SCHEDULER_SOURCES:.c=.o) $(EXAMPLE).o

# Target Name
TARGET = $(EXAMPLE)

# Output Files
HEX = $(TARGET).hex
ELF = $(TARGET).elf
LST = $(TARGET).lst

# Default target
.PHONY: all
all: $(HEX) $(LST) size

# Compile scheduler source
scheduler.o: scheduler.c scheduler.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile example source
$(EXAMPLE).o: $(EXAMPLE_SOURCE) scheduler.h
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files
$(ELF): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

# Create hex file
$(HEX): $(ELF)
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# Create listing file (disassembly with source)
$(LST): $(ELF)
	$(OBJDUMP) -h -S $< > $@

# Display size information
.PHONY: size
size: $(ELF)
	@echo "====== Size Information ======"
	$(SIZE) --format=avr --mcu=$(MCU) $<

# Flash the program to the microcontroller
.PHONY: flash
flash: $(HEX)
	$(AVRDUDE) -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -b $(BAUD) -U flash:w:$(HEX):i

# Flash with verbose output
.PHONY: flash-verbose
flash-verbose: $(HEX)
	$(AVRDUDE) -v -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -b $(BAUD) -U flash:w:$(HEX):i

# Read fuses
.PHONY: fuses
fuses:
	$(AVRDUDE) -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h

# Build all examples
.PHONY: all-examples
all-examples:
	@for example in $(EXAMPLES); do \
		echo ""; \
		echo "========================================"; \
		echo "Building $$example..."; \
		echo "========================================"; \
		$(MAKE) clean; \
		$(MAKE) EXAMPLE=$$example || exit 1; \
	done
	@echo ""
	@echo "========================================"
	@echo "All examples built successfully!"
	@echo "========================================"

# List available examples
.PHONY: list-examples
list-examples:
	@echo "Available examples:"
	@for example in $(EXAMPLES); do \
		echo "  - $$example"; \
	done
	@echo ""
	@echo "Build an example with: make EXAMPLE=<name>"
	@echo "Flash an example with: make EXAMPLE=<name> flash"

# Clean build files
.PHONY: clean
clean:
	rm -f $(OBJECTS) *.elf *.hex *.lst *.map scheduler.o

# Clean everything including dependencies
.PHONY: distclean
distclean: clean
	rm -f *~

# Show help
.PHONY: help
help:
	@echo "AVR Scheduler Makefile"
	@echo "======================"
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build the selected example (default: led_example)"
	@echo "  flash            - Build and flash to MCU"
	@echo "  flash-verbose    - Flash with verbose output"
	@echo "  clean            - Remove build files"
	@echo "  distclean        - Remove all generated files"
	@echo "  size             - Display memory usage"
	@echo "  fuses            - Read fuse settings"
	@echo "  all-examples     - Build all examples"
	@echo "  list-examples    - List available examples"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make EXAMPLE=led_example"
	@echo "  make EXAMPLE=pwm_motor_example flash"
	@echo "  make EXAMPLE=servo_example PORT=/dev/ttyUSB0 flash"
	@echo ""
	@echo "Available Examples:"
	@for example in $(EXAMPLES); do \
		echo "  - $$example"; \
	done
	@echo ""
	@echo "Configuration:"
	@echo "  MCU = $(MCU)"
	@echo "  F_CPU = $(F_CPU)"
	@echo "  PROGRAMMER = $(PROGRAMMER)"
	@echo "  PORT = $(PORT)"
	@echo "  EXAMPLE = $(EXAMPLE)"
	@echo ""
	@echo "To change settings, edit the Makefile or use:"
	@echo "  make MCU=atmega2560 PORT=/dev/ttyUSB0 EXAMPLE=pwm_motor_example flash"
