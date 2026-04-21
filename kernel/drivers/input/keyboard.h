#ifndef DRIVERS_INPUT_KEYBOARD_H
#define DRIVERS_INPUT_KEYBOARD_H

#define KEY_UP    0x01
#define KEY_DOWN  0x02
#define KEY_LEFT  0x03
#define KEY_RIGHT 0x04

void keyboard_handler(void);
char keyboard_read(void);

#endif