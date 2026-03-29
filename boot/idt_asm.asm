global isr0
global isr_keyboard

extern isr0_handler
extern keyboard_handler

isr0:
    cli
    call isr0_handler
    iret

isr_keyboard:
    cli
    pushad
    call keyboard_handler
    popad
    sti
    iret