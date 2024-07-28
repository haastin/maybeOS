
#include <stddef.h>
#include <stdbool.h>
#include "kheap.h"
#include "paging.h"

#define NUM_PAGES_REQUESTED_WHEN_FULL 2
#define HEAP_SIZE 20*PAGE_SIZE

void * start;
void * end;

void * first_open_spot;

void init_heap_alloc_entry(heap_alloc_t * new_heap_entry, size_t size){
    new_heap_entry->size = size;
    new_heap_entry->used = true;
}

static inline unsigned long calc_avail_space_from_curr_pos(void * curr_pos){
    return (unsigned long) (end - curr_pos);
}

static void * get_more_mem(void){
    //call vmalloc
    //ideally the heap should be able to use any virtual page to give to a process
     //by default request 2 pages at a time
    vmalloc(PAGE_SIZE*NUM_PAGES_REQUESTED_WHEN_FULL);
    end += NUM_PAGES_REQUESTED_WHEN_FULL*PAGE_SIZE;
}

static void * goto_first_open_spot(void){
    char * heap = start;
    heap_alloc_t * heap_entry = (heap_alloc_t*) heap; 
    while(heap_entry <= end && heap_entry->used == true){
        size_t mem_used = heap_entry->size + sizeof(heap_alloc_t);
        heap_entry = (heap_alloc_t *) ((char*) heap_entry + heap_entry->size);
    }
    if(heap_entry > end){
       void * new_adddress = get_more_mem();
       return new_adddress;
    }
    else if(heap_entry->used != true){
        return heap_entry;
    }
    
}


void * kmalloc(size_t requested_size){
    //traverse to find big enough spot in the avail mem
    //if no avail spot, call get more mem
    //intitialize heap entry
    //return address immediately after heap entry
}

bool free(void * start_address_for_allocation){
    //
}

void init_kheap(void){

    //at the start, the heap is only given one page, this will change once the vmm and kheap are initialized
    start = vmm_first_heap_alloc();
    end = start + PAGE_SIZE;

    //request the rest of the pages allocated for the heap after getting the first (kmalloc can now be used)
    bool allocated = vmalloc_request_virtual_pages(end, HEAP_SIZE-PAGE_SIZE);
    if(allocated){
        end +=  (HEAP_SIZE - PAGE_SIZE);
        first_open_spot = goto_first_open_spot(); 
    }
    else{
        //TODO: printk error here
    }
}
