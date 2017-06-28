
MCU=attiny13

CC=avr-gcc
CFLAGS=-mmcu=$(MCU) -Wpedantic -Wall -Os -DF_CPU=1200000 -std=c99
OBJCOPY=avr-objcopy
SIZE=avr-size
NM=avr-nm

SOURCE=CandleFlickerLED.c
TARGET=$(SOURCE:.c=.elf)
FIRMWARE=$(TARGET:.elf=.hex)

all: $(FIRMWARE) size

debug-asm: $(SOURCE)
	$(CC) $(CFLAGS) -S -o $(^:.c=.s) $<
	
size: $(TARGET)
	$(SIZE) -C --mcu=$(MCU) $^
	$(NM) --size-sort -C -t d $^

%.hex: %.elf
	$(OBJCOPY) $< $@

%.elf: %.c
	$(CC) $(CFLAGS) -o $@ $^

upload: all
ifeq ($(MCU),attiny13)
	avrdude -P /dev/ttyACM0 -b 19200 -c avrisp -p $(MCU) -v -e -U hfuse:w:0xFF:m -U lfuse:w:0x79:m
endif
	avrdude -P /dev/ttyACM0 -b 19200 -c avrisp -p $(MCU) -v -e -U flash:w:$(FIRMWARE)

.PHONY: all debug-asm size upload
