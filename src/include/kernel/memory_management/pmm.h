#ifndef __PMM_H__
#define __PMM_H__

void init_pmm(unsigned long memory_size);

void * alloc_pageframe(size_t size);

void * alloc_requested_pageframe(unsigned long physical_address, size_t size);

bool free_pageframe(unsigned long pageframe_addy);

extern unsigned long pmm_bitmap_start_addy;
extern unsigned long pmm_bitmap_end_addy;

#endif /*__PMM_H__*/
