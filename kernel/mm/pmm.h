#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define PMM_FRAME_SIZE      4096
#define PMM_FRAME_SHIFT       12
#define PMM_FRAMES_PER_BYTE    8

#define MMAP_TYPE_AVAILABLE    1

/* Структура mmap-записи multiboot (packed — GRUB не гарантирует выравнивание) */
typedef struct {
    uint32_t size;          /* размер оставшейся части записи (обычно 20) */
    uint32_t base_low;
    uint32_t base_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;          /* 1 = доступна, иное = зарезервирована */
} __attribute__((packed)) MmapEntry;

void     pmm_init(uint32_t mmap_addr, uint32_t mmap_length,
                  uint32_t mem_upper_kb, uint32_t kernel_end_addr);

uint32_t pmm_alloc_frame(void);
void     pmm_free_frame(uint32_t frame_addr);

uint32_t pmm_get_total_frames(void);
uint32_t pmm_get_used_frames(void);
uint32_t pmm_get_free_frames(void);

#endif /* PMM_H */
