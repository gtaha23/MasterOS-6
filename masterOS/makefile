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
	gcc $(CFLAGS) -c src/shell.c -o shell.o
	gcc $(CFLAGS) -c src/fat.c -o fat.o
	gcc $(CFLAGS) -c src/ata.c -o ata.o
	gcc $(CFLAGS) -c src/speaker.c -o speaker.o

bootloader:
	nasm -f elf32 src/boot.s -o boot.o
	nasm -f elf32 src/gdt.s -o gdts.o
	nasm -f elf32 src/interrupts/idt.s -o idts.o

iso:
	ld -m elf_i386 -T linker.ld -o kernel boot.o kernel.o vga.o gdt.o gdts.o util.o idt.o idts.o timer.o stdio.o keyboard.o memory.o kmalloc.o shell.o fat.o ata.o speaker.o
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
