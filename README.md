# 🍋 Limon OS

Минималистичная операционная система с нуля.  
Вдохновлена macOS-эстетикой, гибкостью Linux, и проектом [VibeOS](https://github.com/kaansenol5/VibeOS).  
Разработано с Claude Sonnet 4.6.

---

## Требования

- Linux (Ubuntu / Debian)
- GCC, NASM, GRUB, QEMU, xorriso

## Установка зависимостей
```bash
sudo apt install build-essential gcc nasm qemu-system-x86 grub-pc-bin xorriso
```

## Сборка и запуск
```bash
make        # собрать ядро
make run    # запустить в QEMU
```

---

*Limon OS — чисто, минималистично, своё.*