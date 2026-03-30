# 🍋 LimonOS Sicily

> чисто, минималистично, своё.

Хобби-операционная система написанная с нуля для x86.  
Вдохновлена [VibeOS](https://github.com/kaansenol5/VibeOS). Разработано с Claude Sonnet 4.6.

**Актуальная версия: Sicily 0.1.2**

---

## Скриншоты

![Bootscreen](screenshots/v0.1.2/bootscreen.png)
![Shell](screenshots/v0.1.2/shell.png)

---

## Что внутри

- Загрузчик через GRUB (Multiboot)
- VGA драйвер с 16 цветами и скроллингом
- Драйвер клавиатуры PS/2 через аппаратные прерывания (IDT)
- GDT — сегментация памяти
- PIT-таймер (100 Гц) - uptime в `limonfetch`
- Интерактивный шелл с командами: `help`, `about`, `uname`, `clear`, `echo`, `limonfetch`
- `limonfetch` — системная информация с CPU vendor, памятью, uptime и цветовой палитрой

---

## Быстрый старт

### 1. Зависимости
```bash
sudo apt install build-essential gcc nasm qemu-system-x86 grub-pc-bin xorriso
```

### 2. Клонировать и запустить
```bash
git clone https://github.com/oguzokdotdev/limon-os.git
cd limon-os
make        # собрать
make run    # запустить в QEMU
```

---

## Roadmap

- ✅ v0.1.0 — VGA, клавиатура, GDT/IDT, шелл
- ✅ v0.1.1 — версионирование, limonfetch, CI/CD
- ✅ v0.1.2 — reboot, halt, uptime
- 🔲 v0.2.0 — paging, kmalloc
- 🔲 v0.3.0 — процессы, планировщик
- 🔲 v0.4.0 — FAT32
- 🔲 v0.5.0 — userspace, syscalls
- 🔲 v1.0.0 — GUI "Menton"

---

*Limon OS — чисто, минималистично, своё.* 🍋