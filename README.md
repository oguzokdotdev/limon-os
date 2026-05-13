# 🍋 LimonOS

> clean, minimal, yours.

![Version](https://img.shields.io/badge/version-Sicily%200.2.0-yellow?style=flat-square)
![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)
![Arch](https://img.shields.io/badge/arch-i386-blue?style=flat-square)
![Build](https://img.shields.io/github/actions/workflow/status/oguzokdotdev/limon-os/build.yml?style=flat-square)

A hobby x86 operating system built from scratch in C and NASM.  
Inspired by [VibeOS](https://github.com/kaansenol5/VibeOS). Developed with Claude Sonnet 4.6.

---

## 📖 Documentation

| 🇷🇺 Русский | 🇬🇧 English | 🗺️ Roadmap |
|:-----------:|:-----------:|:-----------:|
| [docs/README_RU.md](docs/README_RU.md) | [docs/README_EN.md](docs/README_EN.md) | [docs/ROADMAP.md](docs/ROADMAP.md) |

---

## 📸 Screenshots

<table>
  <tr>
    <td><img src="docs/screenshots/v0.2.0/boot_log.png" width="360"/></td>
    <td><img src="docs/screenshots/v0.2.0/shell.png" width="360"/></td>
  </tr>
  <tr>
    <td><img src="docs/screenshots/v0.2.0/more_shell.png" width="360"/></td>
    <td><img src="docs/screenshots/v0.1.4/kernel_panic.png" width="360"/></td>
  </tr>
</table>

---

## 🚀 Quick Start

First, download the system `.iso` image from the latest [release](https://github.com/oguzokdotdev/limon-os/releases).

```bash
sudo apt update && sudo apt install -y qemu-system-x86      # Debian/Ubuntu
sudo pacman -S --noconfirm qemu-system-x86                  # Arch Linux
sudo dnf install -y qemu-system-x86                         # Fedora

qemu-system-i386 -cdrom limon_*.iso
```

## 🛠️ Build from source

The build environment is currently tested on **Linux Mint (Debian/Ubuntu-based)**.
Note that you may need to modify the Makefile to suit your specific system.

```bash
sudo apt install build-essential gcc nasm qemu-system-i386 grub-pc-bin xorriso

git clone https://github.com/oguzokdotdev/limon-os.git
cd limon-os
make && make run
```

---

**UI prototype (Menton):** [limon.oguzok.tech](https://limon.oguzok.tech/) 🚀

*Limon OS — clean, minimal, yours.* 🍋
