
#include <stddef.h>
#include <stdbool.h>
#include "kheap.h"
#include "paging.h"

#define NUM_PAGES_REQUESTED_WHEN_FULL 2

void * start;
void * end;

void * first_open_spot;

static inline heap_alloc_t * get_next_heap_entry(heap_alloc_t * curr_entry){
    return (heap_alloc_t *)((char*)curr_entry + curr_entry->size + sizeof(heap_alloc_t));
} 

static void init_heap_alloc_entry(heap_alloc_t * new_heap_entry, size_t size){
    new_heap_entry->size = size;
    new_heap_entry->used = true;

}

static void * get_more_mem(void){
    //call vmalloc
    //ideally the heap should be able to use any virtual page to give to a process
    //by default request 2 pages at a time
    vmalloc_request_more_memory(start, PAGE_SIZE*NUM_PAGES_REQUESTED_WHEN_FULL);
    end += NUM_PAGES_REQUESTED_WHEN_FULL*PAGE_SIZE;
}

static void * goto_first_open_spot(void){

    //iterate through the heap, checking heap entries along the way to see if they are used or not

    char * heap = start;
    heap_alloc_t * heap_entry = (heap_alloc_t*) heap;

    while(heap_entry < end && heap_entry->used == true){

        heap_entry = get_next_heap_entry(heap_entry);
    }
    if(heap_entry >= end){
       void * new_adddress = get_more_mem();
       return new_adddress;
    }
    else if(heap_entry->used != true){
        return heap_entry;
    }
    
}

static void * find_first_fit(size_t requested_size){
    
    char * heap = first_open_spot;
    heap_alloc_t * heap_entry = (heap_alloc_t*) heap;

    while(heap_entry < end){

        //if used
        if(heap_entry->used == true){
            
            heap_entry = get_next_heap_entry(heap_entry);
        }
        //if open and big enough
        else if(heap_entry->size >= requested_size){
            return heap_entry;
        }
        //if open but not big enough, keep going
        else{
            heap_entry = get_next_heap_entry(heap_entry);
        }
    }
    //this should always be the case if this point is reached
    if(heap_entry >= end){
        //keep asking for pages until enough space has been reached
        void * new_address = get_more_mem();
        while(end - new_address < requested_size){
            get_more_mem();
        }
       return new_address;
    }
}


void * kmalloc(size_t requested_size){

    //traverse to find big enough spot in the avail mem
    heap_alloc_t * new_heap_entry = find_first_fit(requested_size);
    //intitialize heap entry
    init_heap_alloc_entry(new_heap_entry, requested_size);

    //if this is a fresh page, it will be zeroed out, so we need to make sure to create a heap entry is needed
    heap_alloc_t * next_entry = get_next_heap_entry(new_heap_entry);
    next_entry->used = false;
    next_entry->size = ((unsigned long)end - (unsigned long)next_entry);

    first_open_spot = goto_first_open_spot();

    //return address immediately after heap entry
    return (char*)new_heap_entry + sizeof(heap_alloc_t);
}

void kfree(void * start_address_for_allocation){

    heap_alloc_t * free_entry = start_address_for_allocation;
    free_entry->used = false;
    
     //TODO: for now, can only merge with free blocks after it. this would be much cleaner with true OO code. get C++ up at some point
    heap_alloc_t * next_entry = get_next_heap_entry(free_entry);

    if(next_entry->used == false){
        free_entry->size += next_entry->size;
    }
    
    if(first_open_spot > start_address_for_allocation){
        first_open_spot = start_address_for_allocation - sizeof(heap_alloc_t);
    }
}

void init_kheap(void * heap_start){

    //at the start, the heap is only given one page, this will change once the vmm and kheap are initialized
    start = heap_start;
    end = start + HEAP_SIZE;
    first_open_spot = start;
    heap_alloc_t * first_heap_entry = (heap_alloc_t *) start;
    first_heap_entry->used = false;
    first_heap_entry->size = end-start;
}
