global isr0
global isr_keyboard
global isr_timer

extern isr0_handler
extern keyboard_handler
extern timer_handler

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

isr_timer:
    cli
    pushad
    call timer_handler
    popad
    sti
    iret
