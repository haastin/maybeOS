CC = i686-elf-gcc
AS = i686-elf-as
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
LDFLAGS = -T linker.ld -ffreestanding -O2 -nostdlib -lgcc

SRCS = boot.s finish_booting.c gdt.c
OBJS = $(SRCS:.c=.o) $(SRCS:.s=.o)

TARGET = maybeOS.bin

all: $(TARGET)

$(TARGET): finish_booting.o gdt.o boot.o
	$(CC) $(LDFLAGS) $^ -o $@

finish_booting.o: finish_booting.c
	$(CC) $(CFLAGS) -c $< -o $@

gdt.o: gdt.c gdt.h
	$(CC) $(CFLAGS) -c $< -o $@

boot.o: boot.s
	$(AS) $< -o $@ 

clean:
	rm *.o *.bin