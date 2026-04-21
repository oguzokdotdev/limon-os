#ifndef CORE_BOOT_LOG_H
#define CORE_BOOT_LOG_H

typedef enum { BOOT_OK, BOOT_WARN, BOOT_FAIL } boot_status_t;

void boot_log(boot_status_t status, const char* msg);

#endif