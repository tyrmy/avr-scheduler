# Makefile for AVR Scheduler Unit Tests

# AVR settings
MCU = atmega328p
F_CPU = 16000000UL
PROGRAMMER = arduino
PORT = /dev/ttyUSB0

# Compiler settings
AVR_CC = avr-gcc
HOST_CC = gcc
OBJCOPY = avr-objcopy
SIZE = avr-size
AVRDUDE = avrdude

# AVR compiler flags
AVR_CFLAGS = -Wall -Wextra -Os -mmcu=$(MCU) -DF_CPU=$(F_CPU)
AVR_CFLAGS += -std=gnu99
AVR_CFLAGS += -I..

# Host compiler flags
HOST_CFLAGS = -Wall -Wextra -O2 -DF_CPU=$(F_CPU)
HOST_CFLAGS += -std=gnu99
HOST_CFLAGS += -I. -I..
HOST_CFLAGS += -DSCHEDULER_DEBUG
HOST_CFLAGS += -DHOST_TEST_BUILD

# Linker flags
AVR_LDFLAGS = -mmcu=$(MCU)
HOST_LDFLAGS = 

# Source files
AVR_TEST_SRC = scheduler_test.c
HOST_TEST_SRC = host_test.c
SCHEDULER_SRC = ../scheduler.c

# Output files
AVR_TEST_TARGET = scheduler_test
AVR_TEST_ELF = $(AVR_TEST_TARGET).elf
AVR_TEST_HEX = $(AVR_TEST_TARGET).hex
HOST_TEST_TARGET = host_test

# Default target - run host tests
all: host-test

# Run host tests (quick validation without hardware)
host-test: $(HOST_TEST_TARGET)
	@echo ""
	@echo "Running host-based tests..."
	@echo "========================================"
	./$(HOST_TEST_TARGET)

# Build host test executable
$(HOST_TEST_TARGET): $(HOST_TEST_SRC) $(SCHEDULER_SRC)
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_LDFLAGS) -o $@ $^

# Build AVR test executable
avr: $(AVR_TEST_HEX)
	@echo ""
	@echo "AVR test build complete!"
	@echo "Flash to AVR with: make flash"
	@echo "Connect serial terminal at 9600 baud to see test results"

# Build AVR ELF file
$(AVR_TEST_ELF): $(AVR_TEST_SRC) $(SCHEDULER_SRC)
	$(AVR_CC) $(AVR_CFLAGS) $(AVR_LDFLAGS) -o $@ $^
	$(SIZE) $@

# Create hex file for flashing
$(AVR_TEST_HEX): $(AVR_TEST_ELF)
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# Flash to AVR
flash: $(AVR_TEST_HEX)
	$(AVRDUDE) -c $(PROGRAMMER) -p $(MCU) -P $(PORT) -U flash:w:$(AVR_TEST_HEX):i

# Clean build files
clean:
	rm -f $(AVR_TEST_ELF) $(AVR_TEST_HEX) $(HOST_TEST_TARGET) *.o

# Monitor serial output
monitor:
	@echo "Monitoring serial output at 9600 baud (Ctrl+C to exit)..."
	@echo "If you don't have screen, use: minicom -b 9600 -D $(PORT)"
	screen $(PORT) 9600

# Build and flash AVR test in one step
test-avr: avr flash
	@echo ""
	@echo "Test flashed! Connect to serial port to see results:"
	@echo "  make monitor"

# Run both host and AVR tests
test-all: host-test test-avr

# Help
help:
	@echo "AVR Scheduler Test Suite Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Run host-based tests (default)"
	@echo "  host-test  - Build and run host-based tests"
	@echo "  avr        - Build AVR test executable"
	@echo "  flash      - Flash AVR test to board"
	@echo "  test-avr   - Build and flash AVR test"
	@echo "  monitor    - Open serial monitor to view test results"
	@echo "  clean      - Remove build files"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Configuration:"
	@echo "  MCU        = $(MCU)"
	@echo "  F_CPU      = $(F_CPU)"
	@echo "  PROGRAMMER = $(PROGRAMMER)"
	@echo "  PORT       = $(PORT)"
	@echo ""
	@echo "To change settings, edit the Makefile or override on command line:"
	@echo "  make MCU=atmega2560 PORT=/dev/ttyACM0 test-avr"

.PHONY: all host-test avr flash clean monitor test-avr test-all help
