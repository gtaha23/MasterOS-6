CFLAGS = -m32 -ffreestanding -fno-stack-protector -fno-builtin -g

all: clean kernel bootloader iso

clean:
	rm -rf *.o

kernel:
	gcc $(CFLAGS) -c src/kernel.c -o kernel.o
	gcc $(CFLAGS) -c src/vga.c -o vga.o
	gcc $(CFLAGS) -c src/gdt.c -o gdt.o
	gcc $(CFLAGS) -c src/util.c -o util.o
	gcc $(CFLAGS) -c src/interrupts/idt.c -o idt.o
	gcc $(CFLAGS) -c src/timer.c -o timer.o
	gcc $(CFLAGS) -c src/stdlib/stdio.c -o stdio.o
	gcc $(CFLAGS) -c src/keyboard.c -o keyboard.o
	gcc $(CFLAGS) -c src/memory.c -o memory.o
	gcc $(CFLAGS) -c src/kmalloc.c -o kmalloc.o

bootloader:
	nasm -f elf32 src/boot.s -o boot.o
	nasm -f elf32 src/gdt.s -o gdts.o
	nasm -f elf32 src/interrupts/idt.s -o idts.o

iso:
	ld -m elf_i386 -T linker.ld -o kernel boot.o kernel.o vga.o gdt.o gdts.o util.o idt.o idts.o timer.o stdio.o keyboard.o memory.o kmalloc.o
	mv kernel mOS/boot/kernel
	grub-mkrescue -o kernel.iso mOS/
	rm *.o

run:
	qemu-system-i386 kernel.iso
