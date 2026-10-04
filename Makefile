NASM = nasm
CC = gcc
LD = ld
OBJCOPY = objcopy

CFLAGS = -m32 \
         -ffreestanding \
         -fno-pie \
         -fno-stack-protector \
         -fno-builtin

LDFLAGS = -m elf_i386 -T linker.ld

all: banana.img


boot.bin: boot/boot.asm
	$(NASM) -f bin boot/boot.asm -o boot.bin


kernel_entry.o: kernel/kernel_entry.asm
	$(NASM) -f elf32 kernel/kernel_entry.asm -o kernel_entry.o


kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) -c kernel/kernel.c -o kernel.o

vga.o: kernel/vga.c kernel/vga.h
	$(CC) $(CFLAGS) -c kernel/vga.c -o vga.o

interrupt.o: kernel/interrupt.asm
	$(NASM) -f elf32 kernel/interrupt.asm -o interrupt.o

idt.o: kernel/idt.c kernel/idt.h
	$(CC) $(CFLAGS) -c kernel/idt.c -o idt.o


pic.o: kernel/pic.c kernel/pic.h kernel/io.h
	$(CC) $(CFLAGS) -c kernel/pic.c -o pic.o

timer.o: kernel/timer.c kernel/timer.h kernel/io.h kernel/pic.h kernel/vga.h
	$(CC) $(CFLAGS) -c kernel/timer.c -o timer.o


keyboard.o: kernel/keyboard.c kernel/keyboard.h kernel/io.h kernel/pic.h kernel/vga.h
	$(CC) $(CFLAGS) -c kernel/keyboard.c -o keyboard.o


shell.o: kernel/shell.c kernel/shell.h kernel/vga.h
	$(CC) $(CFLAGS) -c kernel/shell.c -o shell.o

memory.o: kernel/memory.c kernel/memory.h
	$(CC) $(CFLAGS) -c kernel/memory.c -o memory.o


e820.o: kernel/e820.c kernel/e820.h kernel/vga.h
	$(CC) $(CFLAGS) -c kernel/e820.c -o e820.o

pmm.o: kernel/pmm.c kernel/pmm.h kernel/e820.h kernel/memory.h
	$(CC) $(CFLAGS) -c kernel/pmm.c -o pmm.o

kernel.elf: kernel_entry.o kernel.o vga.o idt.o interrupt.o  pic.o timer.o keyboard.o  shell.o memory.o e820.o pmm.o linker.ld
	$(LD) $(LDFLAGS) \
		-o kernel.elf \
		kernel_entry.o \
		kernel.o \
		vga.o \
		idt.o \
		interrupt.o \
		pic.o \
		timer.o \
		keyboard.o \
		shell.o \
		memory.o \
		e820.o \
		pmm.o

kernel.raw: kernel.elf
	$(OBJCOPY) -O binary kernel.elf kernel.raw


boot/kernel_sectors.inc: kernel.raw
	@size=$$(stat -c %s kernel.raw); \
	sectors=$$(( (size + 511) / 512 )); \
	echo "KERNEL_SECTORS equ $$sectors" > boot/kernel_sectors.inc


kernel.bin: kernel.raw boot/kernel_sectors.inc
	cp kernel.raw kernel.bin
	@size=$$(stat -c %s kernel.raw); \
	sectors=$$(( (size + 511) / 512 )); \
	truncate -s $$(( sectors * 512 )) kernel.bin


stage2.bin: boot/stage2.asm boot/kernel_sectors.inc
	$(NASM) -f bin boot/stage2.asm -o stage2.bin



banana.img: boot.bin stage2.bin kernel.bin
	cat boot.bin stage2.bin kernel.bin > banana.img
	truncate -s 1M banana.img


run: banana.img
	qemu-system-i386 -drive format=raw,file=banana.img


clean:
	rm -f \
		boot.bin \
		stage2.bin \
		kernel_entry.o \
		kernel.o \
		vga.o \
		kernel.elf \
		kernel.bin \
		banana.img \
		idt.o \
		interrupt.o \
		pic.o \
		timer.o \
		keyboard.o \
		shell.o \
		memory.o \
		e820.o \
		pmm.o \
		kernel.raw \
		boot/kernel_sectors.inc