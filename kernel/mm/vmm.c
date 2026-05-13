#include "vmm.h"
#include "mm/pmm.h"
#include "core/boot_log.h"
#include <string.h>


static page_directory_t* kernel_dir;
static page_table_t* get_page_table(page_directory_t* dir, uint32_t virt, int create) {
    uint32_t index = virt >> 22;

    if (dir->tables[index]) {
        return dir->tables[index];
    }

    if (!create) return 0;

    uint32_t phys = pmm_alloc_frame();
    if (phys == (uint32_t)-1) return 0;

    page_table_t* table = (page_table_t*)phys;
    memset(table, 0, sizeof(page_table_t));

    dir->tables[index] = table;
    dir->tables_phys[index] = phys | PAGE_PRESENT | PAGE_WRITE;

    return table;
}

void vmm_map_page(page_directory_t* dir,
                  uint32_t virt,
                  uint32_t phys,
                  uint32_t flags)
{
    page_table_t* table = get_page_table(dir, virt, 1);
    if (!table) return;

    uint32_t page_index = (virt >> 12) & 0x3FF;

    table->entries[page_index] = (phys & 0xFFFFF000) | flags;
}

static void identity_map_range(page_directory_t* dir,
                               uint32_t start,
                               uint32_t end,
                               uint32_t flags)
{
    for (uint32_t addr = start; addr < end; addr += PAGE_SIZE) {
        vmm_map_page(dir, addr, addr, flags);
    }
}

void vmm_init(void)
{
    boot_log(BOOT_OK, "VMM: init start");

    kernel_dir = (page_directory_t*)pmm_alloc_frame();
    memset(kernel_dir, 0, sizeof(page_directory_t));

    kernel_dir->phys_addr = (uint32_t)kernel_dir;

    // 1. identity map low memory
    identity_map_range(kernel_dir,
                       0x00000000,
                       0x00400000,   // 4MB safe zone
                       PAGE_PRESENT | PAGE_WRITE);

    // 2. VGA memory
    identity_map_range(kernel_dir,
                       0xB8000,
                       0xB9000,
                       PAGE_PRESENT | PAGE_WRITE);

    boot_log(BOOT_OK, "VMM: identity map done");
}

void vmm_switch_page_directory(page_directory_t* dir)
{
    asm volatile("mov %0, %%cr3" :: "r"(dir->phys_addr));

    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // paging enable
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
}
