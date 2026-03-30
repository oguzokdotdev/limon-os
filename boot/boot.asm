section .multiboot
align 4
    dd 0x1BADB002
    dd 0x00000001       ; flags: бит 0 = запросить mem_lower/mem_upper
    dd -(0x1BADB002 + 0x00000001)

section .text
global _start
extern kernel_main

_start:
    cli
    mov esp, stack_top
    push ebx            ; передаём multiboot_info* в kernel_main
    push eax            ; передаём magic
    call kernel_main
    hlt

section .bss
align 16
stack_bottom:
    resb 16384
stack_top: