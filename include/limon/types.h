#ifndef LIMON_TYPES_H
#define LIMON_TYPES_H

#include <stdint.h>

/* Базовые типы для будущего userspace ABI */
typedef uint32_t pid_t;
typedef uint32_t size_t;
typedef int32_t  ssize_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint32_t mode_t;
typedef int32_t  off_t;

#endif