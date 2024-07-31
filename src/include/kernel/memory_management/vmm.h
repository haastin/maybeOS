#ifndef __VMM_H__
#define __VMM_H__

#include <stddef.h>
#include <stdbool.h>

/*Each dynamic allocation will be tracked and have certain privileges*/
typedef struct dynam_alloc_area{
    unsigned long start_address;
    size_t length;
    size_t flags; 
    struct dynam_alloc_area * next_area; 
} dynam_alloc_area_t;

/*The flags defined for a dynamic alloc area*/
#define VM_AREA_DEFAULT 0

#define VM_AREA_WRITE (1 << 0)

#define VM_AREA_EXEC (1 << 1)

#define VM_AREA_USER_ACCESSIBLE (1 << 2)

#define VM_AREA_MMIO (1 << 3)


#endif /*__VMM_H__*/

void *vmalloc_request_more_memory(unsigned long starting_address, size_t extension_size);

bool vmalloc_free(unsigned long virtual_page_address);

void init_vmm();

void *vmm_first_heap_alloc(void);

void *vmalloc(size_t requested_size, size_t flags);

void * vmalloc_request_virtual_address(unsigned long virtual_page_address, size_t flags, size_t num_pages);

