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

//this supports a maximum of 4 GiB of mem. these pages are guaranteed to be included with the direct map of the kernel as long as the kernel + bitmap size is <= 4Mib; if this is not the case than another page table will need to be statically allocated to be able to map the bitmap pages, because it is most convenient for the bitmap needs to be mapped when initializing the system. this means if the kernel starts at 0xC0200000, the kernel + bitmap must fit in [0xC0200000 - 0xC02800000), or, more generally, VIRT_KERNEL_START + 0x400000
#define NUM_PAGES_FOR_ALLOC_BITMAP 32

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