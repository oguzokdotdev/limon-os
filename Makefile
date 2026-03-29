CC      = gcc
AS      = nasm
CFLAGS  = -m32 -ffreestanding -fno-stack-protector -nostdlib -fno-pic -O2
LDFLAGS = -m elf_i386 -T linker.ld

KERNEL_SRC = kernel/kernel.c
KERNEL_BIN = kernel.bin
ISO         = limon.iso

all: $(ISO)

boot.o: boot/boot.asm
	$(AS) -f elf32 $< -o $@

kernel.o: $(KERNEL_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): boot.o kernel.o
	ld $(LDFLAGS) -o $@ $^

$(ISO): $(KERNEL_BIN)
	mkdir -p iso/boot/grub
	cp $(KERNEL_BIN) iso/boot/kernel.bin
	cp boot/grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) iso

run: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

clean:
	rm -f *.o $(KERNEL_BIN) $(ISO)
	rm -rf iso

.PHONY: all run clean