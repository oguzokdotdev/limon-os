section .multiboot
align 4

MAGIC      equ 0x1BADB002
FLAGS      equ 0x00000003    ; align modules + mem info
CHECKSUM   equ -(MAGIC + FLAGS)

    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .text
global _start
extern kernel_main

_start:
    cli

    mov esp, stack_top

    ; multiboot:
    ; eax = magic
    ; ebx = multiboot_info*

    push ebx
    push eax

    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

section .bss
align 16

stack_bottom:
    resb 16384

stack_top: