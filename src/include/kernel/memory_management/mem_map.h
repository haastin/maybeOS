#ifndef __MEM_MAP_H__
#define __MEM_MAP_H__

#include <stddef.h>
#include "multiboot2.h"
#include <stdbool.h>

/**
 * Userspace
 * Kernel image
 * PMM bitmap (variable-sized)
 * 1 page gap
 * Kheap (20 pages; is a part of the overall dynamic mem area) 
 * Dynamic memory area
 * Fixed Mappings (MMIO devices; is a part of the overall dynamic mem area)
 */

#define VIRTUAL_KERNEL_OFFSET 0xC0000000

extern unsigned long _kernel_size, _kernel_phys_size, _kernel_virt_size, _virt_kernel_start, _virt_kernel_end;

#define KERNEL_SIZE &_kernel_size

#define PHYS_KERNEL_SIZE &_kernel_phys_size

#define VIRT_KERNEL_SIZE &_kernel_virt_size

#define VIRT_KERNEL_START &_virt_kernel_start

#define VIRT_KERNEL_END &_virt_kernel_end

#define MAX_SUPPORTED_RESERVED_BOOT_REGIONS 20

#define MAX_SUPPORTED_MEM_BANKS 10

typedef struct {
    void * start;
    size_t length;
} mem_bank_t;

typedef struct {
    void * start;
    size_t length;
} usable_region_t;

extern mem_bank_t boot_cpu_mem;
extern usable_region_t regions[MAX_SUPPORTED_RESERVED_BOOT_REGIONS];

void init_memory(struct multiboot_tag_mmap *);

bool is_page_usable(unsigned long pageframe_address);

#endif /*__MEM_MAP_H__*/