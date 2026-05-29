#include "vmm.h"
#include "pmm.h"
#include "core/boot_log.h"
#include <string.h>

static page_directory_t kernel_dir __attribute__((aligned(4096)));

/* ── Внутренние хелперы ───────────────────────────────────────────────── */

static page_table_t *get_page_table(page_directory_t *dir,
                                    uint32_t virt,
                                    int create)
{
    uint32_t idx = virt >> 22;

    if (dir->tables[idx])
        return dir->tables[idx];

    if (!create)
        return 0;

    uint32_t phys = pmm_alloc_frame();
    if (phys == (uint32_t)-1)
        return 0;

    page_table_t *table = (page_table_t *)phys;

    memset(table, 0, sizeof(page_table_t));

    dir->tables[idx] = table;

    /*
     * CPU читает именно tables_phys как page directory entries.
     */
    dir->tables_phys[idx] =
        (phys & 0xFFFFF000)
        | PAGE_PRESENT
        | PAGE_WRITE;

    return table;
}

static void identity_map_range(page_directory_t *dir,
                               uint32_t start,
                               uint32_t end,
                               uint32_t flags)
{
    for (uint32_t addr = start;
         addr < end;
         addr += PAGE_SIZE)
    {
        vmm_map_page(dir, addr, addr, flags);
    }
}

/* ── Публичный API ────────────────────────────────────────────────────── */

void vmm_map_page(page_directory_t *dir,
                  uint32_t virt,
                  uint32_t phys,
                  uint32_t flags)
{
    page_table_t *table = get_page_table(dir, virt, 1);

    if (!table)
        return;

    uint32_t page_idx = (virt >> 12) & 0x3FF;

    table->entries[page_idx] =
        (phys & 0xFFFFF000)
        | flags;
}

void vmm_unmap_page(page_directory_t *dir,
                    uint32_t virt)
{
    page_table_t *table = get_page_table(dir, virt, 0);

    if (!table)
        return;

    uint32_t page_idx = (virt >> 12) & 0x3FF;

    table->entries[page_idx] = 0;

    asm volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

void vmm_init(void)
{
    boot_log(BOOT_OK, "VMM: init start");

    memset(&kernel_dir, 0, sizeof(kernel_dir));

    /*
     * Identity map всей используемой памяти.
     * +32 страницы запас под page tables.
     */
    uint32_t map_end =
        (pmm_get_used_frames() + 32) * PAGE_SIZE;

    identity_map_range(
        &kernel_dir,
        0x00000000,
        map_end,
        PAGE_PRESENT | PAGE_WRITE
    );

    boot_log(BOOT_OK, "VMM: identity map done");
}

void vmm_switch_page_directory(page_directory_t *dir)
{
    /*
     * CR3 должен указывать на ФИЗИЧЕСКИЙ адрес
     * page directory entries.
     */
    uint32_t phys =
        (uint32_t)&dir->tables_phys;

    asm volatile(
        "mov %0, %%cr3"
        :
        : "r"(phys)
        : "memory"
    );

    uint32_t cr0;

    asm volatile("mov %%cr0, %0"
                 : "=r"(cr0));

    cr0 |= 0x80000000;

    asm volatile("mov %0, %%cr0"
                 :
                 : "r"(cr0)
                 : "memory");
}

page_directory_t *vmm_get_kernel_dir(void)
{
    return &kernel_dir;
}