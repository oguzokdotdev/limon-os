#ifndef LIMON_SYSCALL_H
#define LIMON_SYSCALL_H

/*
 * LimonOS Syscall ABI — заполняется в v0.5.0
 *
 * Calling convention (x86):
 *   eax = syscall number
 *   ebx = arg1, ecx = arg2, edx = arg3
 *   return value in eax
 */

/* Syscall numbers (placeholder) */
#define SYS_EXIT  1
#define SYS_WRITE 4
#define SYS_READ  3

#endif