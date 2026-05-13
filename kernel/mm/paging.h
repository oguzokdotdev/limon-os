#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGE_SIZE 4096

#define PAGE_PRESENT 0x1
#define PAGE_WRITE   0x2
#define PAGE_USER    0x4

typedef uint32_t page_entry_t;

typedef struct {
    page_entry_t entries[1024];
} page_table_t;

typedef struct {
    page_table_t* tables[1024];
    uint32_t tables_phys[1024];
    uint32_t phys_addr;
} page_directory_t;

#endif
