#include "keyboard.h"
#include "arch/x86/cpu.h"

static volatile char key_buf[256];
static volatile int  key_head      = 0;
static volatile int  key_tail      = 0;
static volatile int  shift_pressed = 0;
static volatile int  e0_prefix     = 0;

static const char keymap[] = {
    0, 0, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
};

static const char keymap_shift[] = {
    0, 0, '!','@','#','$','%','^','&','*','(',')','-','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '
};

void keyboard_handler(void) {
    uint8_t scan = inb(0x60);
    if (scan == 0xE0) { e0_prefix = 1; outb(0x20, 0x20); return; }
    if (e0_prefix) {
        e0_prefix = 0;
        if (scan == 0x48) { key_buf[key_head] = KEY_UP;    key_head = (key_head+1)%256; }
        if (scan == 0x50) { key_buf[key_head] = KEY_DOWN;  key_head = (key_head+1)%256; }
        if (scan == 0x4B) { key_buf[key_head] = KEY_LEFT;  key_head = (key_head+1)%256; }
        if (scan == 0x4D) { key_buf[key_head] = KEY_RIGHT; key_head = (key_head+1)%256; }
        outb(0x20, 0x20); return;
    }
    if (scan == 0x2A || scan == 0x36) { shift_pressed = 1; outb(0x20, 0x20); return; }
    if (scan == 0xAA || scan == 0xB6) { shift_pressed = 0; outb(0x20, 0x20); return; }
    if (!(scan & 0x80)) {
        const char* map = shift_pressed ? keymap_shift : keymap;
        if (scan < sizeof(keymap) && map[scan]) {
            key_buf[key_head] = map[scan];
            key_head = (key_head + 1) % 256;
        }
    }
    outb(0x20, 0x20);
}

char keyboard_read(void) {
    if (key_head == key_tail) return 0;
    char c = key_buf[key_tail];
    key_tail = (key_tail + 1) % 256;
    return c;
}