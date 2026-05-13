# 🍋 LimonOS Roadmap & Changelog

| Version | Name | Status                                                                     | Released   | Highlights |
|---------|------|----------------------------------------------------------------------------|------------|------------|
| **v0.1.0** | Core | [✅ Released](https://github.com/oguzokdotdev/limon-os/releases/tag/v0.1.0) | 29.03.2026 | VGA driver, PS/2 keyboard, GDT/IDT, basic shell |
| **v0.1.1** | Versioning & CI/CD | [✅ Released](https://github.com/oguzokdotdev/limon-os/releases/tag/v0.1.1) | 30.03.2026 | `version.h`, `limonfetch`, GitHub Actions |
| **v0.1.2** | System Control | [✅ Released](https://github.com/oguzokdotdev/limon-os/releases/tag/v0.1.2) | 30.03.2026 | `reboot`, `halt`, PIT timer, uptime |
| **v0.1.3** | Shell UX | [✅ Released](https://github.com/oguzokdotdev/limon-os/releases/tag/v0.1.3) | 06.04.2026 | History, tab completion, shift, `lscmd`, `ver` |
| **v0.1.4** | CPU Exceptions | [✅ Released](https://github.com/oguzokdotdev/limon-os/releases/tag/v0.1.4) | 10.04.2026 | #DE #GP #PF #DF handlers, `panic()` |
| **v0.1.5** | libc Foundation | [✅ Released](https://github.com/oguzokdotdev/limon-os/releases/tag/v0.1.5) | 11.04.2026 | `string.c`, `memory.c`, `convert.c` |
| **v0.1.6** | VGA Extended | [✅ Released](https://github.com/oguzokdotdev/limon-os/releases/tag/v0.1.6) | 14.04.2026 | Hardware cursor |
| **v0.1.7** | Verbose Boot | [✅ Released](https://github.com/oguzokdotdev/limon-os/releases/tag/v0.1.7) | 16.04.2026 | `[OK]`/`[WARN]`/`[FAIL]` boot log |
| **v0.1.8** | Timers & RTC | [✅ Released](https://github.com/oguzokdotdev/limon-os/releases/tag/v0.1.8) | 18.04.2026 | RTC, `date` command | 
| **v0.1.9** | Refactoring | [✅ Released](https://github.com/oguzokdotdev/limon-os/releases/tag/v0.1.9) | 21.04.2026 | `kernel.h`, folder structure |
| **v0.2.0** | Memory: PMM | [✅ Released](https://github.com/oguzokdotdev/limon-os/releases/tag/v0.2.0) | 13.05.2026 | Bitmap allocator, GRUB mmap, frame management |
| **v0.2.1** | Memory: VMM | 🔲 Planned                                                                 | —          | Paging, Page Tables, Higher-Half Kernel |
| **v0.2.2** | Memory: Heap | 🔲 Planned                                                                 | —          | kmalloc, kfree, slab/heap allocation |
| **v0.3.0** | Processes | 🔲 Planned        	                                                         | —          | Processes, scheduler |
| **v0.4.0** | FAT32 | 🔲 Planned                                                                 | —          | FAT32 filesystem |
| **v0.5.0** | Userspace & Syscalls | 🔲 Planned                                                                 | —          | Userspace, syscalls |
| **v1.0.0** | GUI *(Menton)* | 🔲 Planned                                                                 | —          | Graphical interface |
