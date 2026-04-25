CC      = gcc
AS      = nasm
LD      = ld

CFLAGS  = -m32 -std=gnu99 -ffreestanding -fno-stack-protector -nostdlib \
          -fno-pic -O2 -Wall -Wextra \
          -Ikernel -Ikernel/core -Ilibc -Iinclude

LDFLAGS = -m elf_i386 -T linker.ld

VERSION          = 0.2.0
CODENAME         = sicily
CODENAME_DISPLAY = Sicily

BUILD := $(shell cat build.no)

VERSION_H = kernel/core/version.h

KERNEL_SRCS = \
    kernel/core/kernel.c            \
    kernel/core/panic.c             \
    kernel/core/boot_log.c          \
    kernel/arch/x86/gdt.c           \
    kernel/arch/x86/idt.c           \
    kernel/arch/x86/cpu.c           \
    kernel/drivers/video/vga.c      \
    kernel/drivers/input/keyboard.c \
    kernel/drivers/timer/pit.c      \
    kernel/drivers/timer/rtc.c      \
    kernel/shell/shell.c            \
    kernel/mm/pmm.c	            \
    libc/string.c                   \
    libc/memory.c                   \
    libc/convert.c

KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)
ASM_OBJS    = boot/boot.o boot/idt_asm.o

all: iso/boot/kernel.bin

$(VERSION_H): Makefile
	@mkdir -p kernel/core
	@printf '#ifndef VERSION_H\n#define VERSION_H\n'                                              > $(VERSION_H)
	@printf '#define LIMON_VERSION_STRING "%s"\n'   "$(VERSION)"                                >> $(VERSION_H)
	@printf '#define LIMON_CODENAME       "%s"\n'   "$(CODENAME_DISPLAY)"                       >> $(VERSION_H)
	@printf '#define LIMON_VERSION_FULL   "LimonOS %s (v%s)"\n' "$(CODENAME_DISPLAY)" "$(VERSION)" >> $(VERSION_H)
	@printf '#define LIMON_ARCH           "i386"\n'                                             >> $(VERSION_H)
	@printf '#define LIMON_BUILD          %s\n'     "$(BUILD)"                                  >> $(VERSION_H)
	@printf '#endif\n'                                                                          >> $(VERSION_H)

%.o: %.c $(VERSION_H)
	$(CC) $(CFLAGS) -c $< -o $@

boot/boot.o: boot/boot.asm
	$(AS) -f elf32 $< -o $@

boot/idt_asm.o: boot/idt_asm.asm
	$(AS) -f elf32 $< -o $@

kernel.bin: $(ASM_OBJS) $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

iso/boot/kernel.bin: kernel.bin
	$(eval BUILD := $(shell echo $$(( $(BUILD) + 1 ))))
	@echo $(BUILD) > build.no
	@printf '#ifndef VERSION_H\n#define VERSION_H\n'                                              > $(VERSION_H)
	@printf '#define LIMON_VERSION_STRING "%s"\n'   "$(VERSION)"                                >> $(VERSION_H)
	@printf '#define LIMON_CODENAME       "%s"\n'   "$(CODENAME_DISPLAY)"                       >> $(VERSION_H)
	@printf '#define LIMON_VERSION_FULL   "LimonOS %s (v%s)"\n' "$(CODENAME_DISPLAY)" "$(VERSION)" >> $(VERSION_H)
	@printf '#define LIMON_ARCH           "i386"\n'                                             >> $(VERSION_H)
	@printf '#define LIMON_BUILD          %s\n'     "$(BUILD)"                                  >> $(VERSION_H)
	@printf '#endif\n'                                                                          >> $(VERSION_H)
	$(CC) $(CFLAGS) -c kernel/core/kernel.c  -o kernel/core/kernel.o
	$(CC) $(CFLAGS) -c kernel/core/panic.c   -o kernel/core/panic.o
	$(CC) $(CFLAGS) -c kernel/shell/shell.c  -o kernel/shell/shell.o
	$(LD) $(LDFLAGS) -o kernel.bin $(ASM_OBJS) $(KERNEL_OBJS)
	@mkdir -p iso/boot/grub
	@cp kernel.bin iso/boot/kernel.bin
	@cp boot/grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o limon_v$(VERSION)_$(CODENAME)_b$(BUILD).iso iso
	@echo limon_v$(VERSION)_$(CODENAME)_b$(BUILD).iso > last.iso
	@echo "  --> Build $(BUILD) complete: limon_v$(VERSION)_$(CODENAME)_b$(BUILD).iso"
	
iso: iso/boot/kernel.bin

run:
	qemu-system-i386 -cdrom $$(cat last.iso) -m 256M

commit-build:
	git config --global user.name "github-actions[bot]"
	git config --global user.email "github-actions[bot]@users.noreply.github.com"
	git pull --rebase origin main
	git add build.no
	git commit -m "chore: bump build number to b$(BUILD) [skip ci]"
	git push

clean:
	rm -f $(KERNEL_OBJS) $(ASM_OBJS) kernel.bin limon_*.iso
	rm -rf iso
	rm -f $(VERSION_H) last.iso

.PHONY: all iso run clean commit-build
