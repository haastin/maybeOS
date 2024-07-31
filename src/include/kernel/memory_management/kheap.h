#ifndef __KHEAP_H__
#define __KHEAP_H__

#include <stddef.h>

typedef struct{
    size_t size;
    bool used;

} heap_alloc_t;

#define HEAP_SIZE 20*PAGE_SIZE

void *kmalloc(size_t requested_size);

void kfree(void *start_address_for_allocation);

void init_kheap(void *heap_start);

#endif /*__KHEAP_H__*/