#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include "paging.h"

void vmm_init(void);
void vmm_switch_page_directory(page_directory_t* dir);

void vmm_map_page(page_directory_t* dir,
                  uint32_t virt,
                  uint32_t phys,
                  uint32_t flags);

#endif
