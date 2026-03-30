CC      = gcc
AS      = nasm
CFLAGS  = -m32 -ffreestanding -fno-stack-protector -nostdlib -fno-pic -O2 -Ikernel
LDFLAGS = -m elf_i386 -T linker.ld

VERSION          = 0.1.1
CODENAME         = sicily
CODENAME_DISPLAY = Sicily
BUILD           := $(shell cat build.no)

KERNEL_BIN = kernel.bin
ISO         = limon_v$(VERSION)_$(CODENAME)_b$(BUILD).iso

all: $(ISO)

kernel/version.h:
	@echo "#ifndef VERSION_H"                                                      > kernel/version.h
	@echo "#define VERSION_H"                                                     >> kernel/version.h
	@echo "#define LIMON_VERSION_STRING \"$(VERSION)\""                           >> kernel/version.h
	@echo "#define LIMON_CODENAME       \"$(CODENAME_DISPLAY)\""                  >> kernel/version.h
	@echo "#define LIMON_VERSION_FULL   \"LimonOS $(CODENAME_DISPLAY) (v$(VERSION))\"" >> kernel/version.h
	@echo "#define LIMON_ARCH           \"i386\""                                 >> kernel/version.h
	@echo "#define LIMON_BUILD          $(BUILD)"                                 >> kernel/version.h
	@echo "#endif"                                                                >> kernel/version.h

boot.o: boot/boot.asm
	$(AS) -f elf32 $< -o $@

idt_asm.o: boot/idt_asm.asm
	$(AS) -f elf32 $< -o $@

kernel.o: kernel/version.h kernel/kernel.c
	$(CC) $(CFLAGS) -c kernel/kernel.c -o $@

gdt.o: kernel/gdt.c
	$(CC) $(CFLAGS) -c $< -o $@

idt.o: kernel/idt.c
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): boot.o idt_asm.o kernel.o gdt.o idt.o
	ld $(LDFLAGS) -o $@ $^

$(ISO): $(KERNEL_BIN)
	mkdir -p iso/boot/grub
	cp $(KERNEL_BIN) iso/boot/kernel.bin
	cp boot/grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) iso
	@echo $$(( $(BUILD) + 1 )) > build.no
	@echo "  --> Build $(BUILD) complete: $(ISO)"

run: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

clean:
	rm -f *.o $(KERNEL_BIN) limon_*.iso
	rm -rf iso
	rm -f kernel/version.h

.PHONY: all run clean
