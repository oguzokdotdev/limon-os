CC      = gcc
AS      = nasm
CFLAGS  = -m32 -ffreestanding -fno-stack-protector -nostdlib -fno-pic -O2 -Ikernel
LDFLAGS = -m elf_i386 -T linker.ld
LIBC_OBJS = kernel/libc/string.o kernel/libc/memory.o kernel/libc/convert.o

VERSION          = 0.1.7
CODENAME         = sicily
CODENAME_DISPLAY = Sicily

BUILD := $(shell cat build.no)

all: iso/boot/kernel.bin

kernel/version.h: Makefile
	@printf '#ifndef VERSION_H\n#define VERSION_H\n' > kernel/version.h.tmp
	@printf '#define LIMON_VERSION_STRING "%s"\n' "$(VERSION)"          >> kernel/version.h.tmp
	@printf '#define LIMON_CODENAME       "%s"\n' "$(CODENAME_DISPLAY)" >> kernel/version.h.tmp
	@printf '#define LIMON_VERSION_FULL   "LimonOS %s (v%s)"\n' "$(CODENAME_DISPLAY)" "$(VERSION)" >> kernel/version.h.tmp
	@printf '#define LIMON_ARCH           "i386"\n'                     >> kernel/version.h.tmp
	@printf '#define LIMON_BUILD          %s\n' "$(BUILD)"              >> kernel/version.h.tmp
	@printf '#endif\n'                                                   >> kernel/version.h.tmp
	@if ! cmp -s kernel/version.h.tmp kernel/version.h 2>/dev/null; then \
		mv kernel/version.h.tmp kernel/version.h; \
		echo "  --> version.h updated (b$(BUILD))"; \
	else \
		rm kernel/version.h.tmp; \
	fi

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

kernel/libc/string.o: kernel/libc/string.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/libc/memory.o: kernel/libc/memory.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/libc/convert.o: kernel/libc/convert.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.bin: boot.o idt_asm.o kernel.o gdt.o idt.o $(LIBC_OBJS)
	ld $(LDFLAGS) -o $@ $^

iso/boot/kernel.bin: kernel.bin
	$(eval BUILD := $(shell echo $$(( $(BUILD) + 1 ))))
	@echo $(BUILD) > build.no
	@printf '#ifndef VERSION_H\n#define VERSION_H\n' > kernel/version.h
	@printf '#define LIMON_VERSION_STRING "%s"\n' "$(VERSION)"          >> kernel/version.h
	@printf '#define LIMON_CODENAME       "%s"\n' "$(CODENAME_DISPLAY)" >> kernel/version.h
	@printf '#define LIMON_VERSION_FULL   "LimonOS %s (v%s)"\n' "$(CODENAME_DISPLAY)" "$(VERSION)" >> kernel/version.h
	@printf '#define LIMON_ARCH           "i386"\n'                     >> kernel/version.h
	@printf '#define LIMON_BUILD          %s\n' "$(BUILD)"              >> kernel/version.h
	@printf '#endif\n'                                                   >> kernel/version.h
	$(CC) $(CFLAGS) -c kernel/kernel.c -o kernel.o
	ld $(LDFLAGS) -o kernel.bin boot.o idt_asm.o kernel.o gdt.o idt.o $(LIBC_OBJS)
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/kernel.bin
	cp boot/grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o limon_v$(VERSION)_$(CODENAME)_b$(BUILD).iso iso
	@echo limon_v$(VERSION)_$(CODENAME)_b$(BUILD).iso > last.iso
	@echo "  --> Build $(BUILD) complete: limon_v$(VERSION)_$(CODENAME)_b$(BUILD).iso"

iso: iso/boot/kernel.bin

run: iso/boot/kernel.bin
	qemu-system-i386 -cdrom $$(cat last.iso)

commit-build:
	git config --global user.name "github-actions[bot]"
	git config --global user.email "github-actions[bot]@users.noreply.github.com"
	git pull --rebase origin main
	git add build.no
	git commit -m "chore: bump build number to b$(BUILD) [skip ci]"
	git push

clean:
	rm -f *.o kernel.bin limon_*.iso
	rm -rf iso
	rm -f kernel/version.h last.iso
	rm -f kernel/libc/*.o

.PHONY: all iso run clean commit-build
