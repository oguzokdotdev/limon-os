#include "pmm.h"
#include "core/boot_log.h"

/* ── Внутреннее состояние ─────────────────────────────────────────────── */

static uint8_t  *pmm_bitmap;        /* 1 бит = 1 фрейм 4 КБ; 1 = занят  */
static uint32_t  pmm_total_frames;
static uint32_t  pmm_used_frames;
static uint32_t  pmm_bitmap_bytes;

/* ── Bitmap helpers ───────────────────────────────────────────────────── */

static inline void _frame_set(uint32_t frame) {
    pmm_bitmap[frame >> 3] |= (uint8_t)(1u << (frame & 7));
}

static inline void _frame_clear(uint32_t frame) {
    pmm_bitmap[frame >> 3] &= (uint8_t)~(1u << (frame & 7));
}

static inline int _frame_test(uint32_t frame) {
    return (pmm_bitmap[frame >> 3] >> (frame & 7)) & 1;
}

/* Безопасные counted-версии: не трогают счётчик дважды */
static inline void _frame_set_counted(uint32_t frame) {
    if (frame >= pmm_total_frames) return;
    if (!_frame_test(frame)) {
        _frame_set(frame);
        pmm_used_frames++;
    }
}

static inline void _frame_clear_counted(uint32_t frame) {
    if (frame >= pmm_total_frames) return;
    if (_frame_test(frame)) {
        _frame_clear(frame);
        pmm_used_frames--;
    }
}

/* ── Инициализация ────────────────────────────────────────────────────── */

void pmm_init(uint32_t mmap_addr, uint32_t mmap_length,
              uint32_t mem_upper_kb, uint32_t kernel_end_addr) {

    /* Полный объём памяти: 1 МБ (low) + mem_upper КБ */
    uint32_t total_kb    = 1024u + mem_upper_kb;
    pmm_total_frames     = (total_kb * 1024u) / PMM_FRAME_SIZE;
    pmm_bitmap_bytes     = (pmm_total_frames + PMM_FRAMES_PER_BYTE - 1)
                           / PMM_FRAMES_PER_BYTE;

    /* Bitmap сразу после ядра, выровнен на 4 КБ */
    uint32_t bitmap_phys = (kernel_end_addr + PMM_FRAME_SIZE - 1)
                           & ~(PMM_FRAME_SIZE - 1u);
    pmm_bitmap           = (uint8_t *)bitmap_phys;

    /* Шаг 1: все фреймы заняты */
    for (uint32_t i = 0; i < pmm_bitmap_bytes; i++)
        pmm_bitmap[i] = 0xFF;
    pmm_used_frames = pmm_total_frames;

    /* Шаг 2: освободить доступные регионы из mmap */
    if (mmap_addr && mmap_length) {
        MmapEntry *e = (MmapEntry *)mmap_addr;
        while ((uint32_t)e < mmap_addr + mmap_length) {

            /* Игнорируем память выше 4 ГБ (PAE нет) */
            if (e->type == MMAP_TYPE_AVAILABLE && e->base_high == 0) {
                uint32_t base        = e->base_low;
                uint32_t len         = e->length_low;
                uint32_t first_frame = base / PMM_FRAME_SIZE;
                uint32_t n_frames    = len  / PMM_FRAME_SIZE;

                for (uint32_t i = 0; i < n_frames; i++)
                    _frame_clear_counted(first_frame + i);
            }

            /* Advance: size не включает само поле size (4 байта) */
            e = (MmapEntry *)((uint32_t)e + e->size + sizeof(uint32_t));
        }
    } else {
        /* Fallback без mmap: только консервативный диапазон выше 1 МБ */
        boot_log(BOOT_WARN, "PMM: no mmap, using mem_upper fallback");
        uint32_t first = 0x100000u / PMM_FRAME_SIZE;
        for (uint32_t i = first; i < pmm_total_frames; i++)
            _frame_clear_counted(i);
    }

    /* Шаг 3: принудительно пометить зарезервированные зоны */

    /* 3а: первый МБ (BIOS, VGA, IVT, EBDA) */
    for (uint32_t i = 0; i < (0x100000u / PMM_FRAME_SIZE); i++)
        _frame_set_counted(i);

    /* 3б: код ядра + стек + bitmap */
    uint32_t bitmap_end   = bitmap_phys + pmm_bitmap_bytes;
    uint32_t first_kernel = 0x100000u / PMM_FRAME_SIZE;
    uint32_t last_bitmap  = (bitmap_end + PMM_FRAME_SIZE - 1) / PMM_FRAME_SIZE;

    for (uint32_t i = first_kernel; i < last_bitmap; i++)
        _frame_set_counted(i);

    boot_log(BOOT_OK, "PMM initialized");
}

/* ── Аллокация ────────────────────────────────────────────────────────── */

uint32_t pmm_alloc_frame(void) {
    if (pmm_used_frames >= pmm_total_frames)
        return (uint32_t)-1;   /* OOM — caller обязан проверить */

    for (uint32_t i = 0; i < pmm_bitmap_bytes; i++) {
        if (pmm_bitmap[i] == 0xFF) continue;   /* быстрый пропуск */

        for (int j = 0; j < PMM_FRAMES_PER_BYTE; j++) {
            uint32_t frame = i * PMM_FRAMES_PER_BYTE + j;
            if (frame >= pmm_total_frames) break;

            if (!_frame_test(frame)) {
                _frame_set_counted(frame);
                return frame * PMM_FRAME_SIZE;   /* физический адрес */
            }
        }
    }
    return (uint32_t)-1;
}

void pmm_free_frame(uint32_t frame_addr) {
    /* Защита от освобождения зарезервированной памяти */
    if (frame_addr < 0x100000u) return;   /* никогда не освобождаем < 1 МБ */

    uint32_t frame = frame_addr / PMM_FRAME_SIZE;
    _frame_clear_counted(frame);
}

/* ── Статистика ───────────────────────────────────────────────────────── */

uint32_t pmm_get_total_frames(void) { return pmm_total_frames; }
uint32_t pmm_get_used_frames(void)  { return pmm_used_frames;  }
uint32_t pmm_get_free_frames(void)  { return pmm_total_frames - pmm_used_frames; }

