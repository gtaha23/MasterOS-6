CFLAGS = -m32 -ffreestanding -O2 -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -mno-mmx -mno-80387 -w
LDFLAGS = -m elf_i386 -T linker.ld
CC = gcc
AS = nasm
LD = ld

all: clean kernel bootloader iso

clean:
	rm -rf *.o

kernel:
	$(CC) $(CFLAGS) -c src/kernel.c -o kernel.o
	$(CC) $(CFLAGS) -c src/drivers/vga.c -o vga.o
	$(CC) $(CFLAGS) -c src/drivers/gdt.c -o gdt.o
	$(CC) $(CFLAGS) -c src/memory/util.c -o util.o
	$(CC) $(CFLAGS) -c src/interrupts/idt.c -o idt.o
	$(CC) $(CFLAGS) -c src/timer.c -o timer.o
	$(CC) $(CFLAGS) -c src/stdlib/stdio.c -o stdio.o
	$(CC) $(CFLAGS) -c src/drivers/device/keyboard.c -o keyboard.o
	$(CC) $(CFLAGS) -c src/memory/memory.c -o memory.o
	$(CC) $(CFLAGS) -c src/memory/kmalloc.c -o kmalloc.o
	$(CC) $(CFLAGS) -c src/shell.c -o shell.o
	$(CC) $(CFLAGS) -c src/drivers/fs/fat.c -o fat.o
	$(CC) $(CFLAGS) -c src/drivers/fs/ata.c -o ata.o
	$(CC) $(CFLAGS) -c src/drivers/device/speaker.c -o speaker.o
	$(CC) $(CFLAGS) -c src/rtc.c -o rtc.o
	$(CC) $(CFLAGS) -c src/drivers/device/exec.c -o exec.o
	$(CC) $(CFLAGS) -c src/drivers/device/cpu.c -o cpu.o
	$(CC) $(CFLAGS) -c src/power.c -o power.o

bootloader:
	$(AS) -f elf32 src/boot.s -o boot.o
	$(AS) -f elf32 src/gdt.s -o gdts.o
	$(AS) -f elf32 src/interrupts/idt.s -o idts.o

iso:
	$(LD) -m elf_i386 -T linker.ld -o kernel boot.o kernel.o vga.o gdt.o gdts.o util.o idt.o idts.o timer.o stdio.o keyboard.o memory.o kmalloc.o shell.o fat.o ata.o speaker.o rtc.o exec.o cpu.o power.o
	mv kernel mOS/boot/kernel
	grub-mkrescue -o kernel.iso mOS/
	rm *.o

# Creates a 32MB FAT16 disk image with a test file — !! only run once !!
disk:
	dd if=/dev/zero of=disk.img bs=1M count=32
	mkfs.fat -F 16 disk.img
	mkdir -p /tmp/mnt_disk
	sudo mount -o loop disk.img /tmp/mnt_disk
	echo "Hello from MasterOS!" | sudo tee /tmp/mnt_disk/HELLO.TXT
	sudo umount /tmp/mnt_disk
	rmdir /tmp/mnt_disk

run:
	qemu-system-i386 -cdrom kernel.iso \
	    -drive file=disk.img,format=raw,index=1,media=disk,if=ide \
	    -boot d -m 32 \
	    -display gtk \
	    -audiodev alsa,id=speaker \
	    -machine pcspk-audiodev=speaker
