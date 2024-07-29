#ifndef __KHEAP_H__
#define __KHEAP_H__

#include <stddef.h>

typedef struct{
    size_t size;
    bool used;

} heap_alloc_t;

#define HEAP_SIZE 20*PAGE_SIZE

void init_heap_alloc_entry(heap_alloc_t *new_heap_entry, size_t size);

void init_kheap(void * heap_start);


#endif /*__KHEAP_H__*/