section .multiboot
align 4
MAGIC       equ 0x1BADB002
FLAGS       equ 0x00000003    ; 0x1 (выравнивание) + 0x2 (инфо о памяти)
CHECKSUM    equ -(MAGIC + FLAGS)

    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .text
global _start
extern kernel_main

_start:
    cli
    mov esp, stack_top
    push ebx            ; передаём multiboot_info* в kernel_main (второй аргумент)
    push eax            ; передаём magic в kernel_main (первый аргумент)
    call kernel_main
    hlt

section .bss
align 16
stack_bottom:
    resb 16384
stack_top: