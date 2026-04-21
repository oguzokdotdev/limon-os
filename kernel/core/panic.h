#ifndef CORE_PANIC_H
#define CORE_PANIC_H

#include "arch/x86/idt.h"

void panic(const char* msg, Registers* regs);

#endif