# 🍋 LimonOS Sicily

> clean, minimal, yours.

A hobby x86 operating system built from scratch.  
Inspired by [VibeOS](https://github.com/kaansenol5/VibeOS). Developed with Claude Sonnet 4.6.

**Current version: Sicily 0.2.0**

**LimonOS Menton UI prototype: <a href="https://limon.oguzok.tech/" target="_blank">click 🚀</a>**

---

## What's inside

- GRUB bootloader (Multiboot)
- VGA text driver with 16 colors and scrolling
- PS/2 keyboard driver via hardware interrupts (IDT)
- GDT — memory segmentation
- PIT timer at 100 Hz — uptime in `limonfetch`
- CPU exception handlers (#DE, #GP, #PF, #DF) with a panic screen (`panic()`)
- Interactive shell with commands: `help`, `lscmd`, `about`, `uname`, `ver`, `clear`, `echo`, `limonfetch`, `reboot`, `halt`, `date`
- `help [category]` — categorized help (System Control / Info & Utilities)
- `lscmd` — lists all commands in a column layout
- `uname` — system info with flags (`-o`, `-v`, `-c`, `-i`, `-b`, `-a`)
- `limonfetch` — system info with ASCII logo, CPU model, memory, uptime, and color palette
- `reboot` / `halt` — power control via keyboard controller and `cli; hlt`
- `date' — show current datetime
- Command history (16 entries) with ↑↓ arrow navigation
- Tab completion (single match inserts, multiple matches listed)
- CPUID for CPU model detection
- Memory read from Multiboot info structure (`mem_upper`)
- Kernel libc `kernel/libc/`: strings (`strlen`, `strcmp`, `strcpy` etc.), memory (`memset`, `memcpy`, `memmove`, `memcmp`), conversion (`atoi`, `itoa`, `itoh`)
- VGA hardware cursor
- Boot logging
- RTC driver
- Physical Memory Manager (PMM)

---

## 📖 See Also

- [Roadmap & Changelog](ROADMAP.md)

*Limon OS — clean, minimal, yours.* 🍋
