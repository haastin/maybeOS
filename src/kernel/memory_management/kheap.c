
#include <stddef.h>
#include <stdbool.h>
#include "kheap.h"
#include "paging.h"

//* Kheap CANNOT handle allocations larger than NUM_PAGES_REQUESTED_WHEN_FULL - sizeof(heap_alloc_t) - sizeof(heap_range_t) because it only requests new mem in increments of this size. Larger requests should go to the vmm directly.

#define NUM_PAGES_REQUESTED_WHEN_FULL 2

typedef struct heap_range heap_range_t;

struct heap_range {
    void * start;
    void * end;
    void * first_free_tag;
    heap_range_t * next;
}; 

heap_range_t * heap_ranges;

static inline heap_alloc_t * get_next_heap_tag(heap_alloc_t * curr_entry){
    return (heap_alloc_t *)((char*)curr_entry + curr_entry->size + sizeof(heap_alloc_t));
} 

/**
 * Initializes the desired heap tag with its attributes and, if possible, creates a new heap tag entry with the leftover space that this allocation doesn't use of this heap entry
 * @param new_heap_entry the heap tag found that is free and has enough space for the request
 * @param avail_size the amount of available space for data; does NOT include the passed heap tag's size - aka this is the current value of this heap tag's size which we are now replacing
 */
static void init_heap_alloc_entry(heap_alloc_t * new_heap_entry, size_t requested_size, size_t avail_size){
    
    //init the heap entry
    new_heap_entry->used = true;

    //in the case where there is only enough space enough space for a heap_alloc tag or less, then we cant actually store any more data in this heap location, so just give it all to the requestee. in this condition < implies more avail space than all the needed space plus a tag for the potential new heap entry after this current one would be split
    if((requested_size + sizeof(heap_alloc_t)) < avail_size){
        new_heap_entry->size = requested_size;

        //mark the next avail heap entry after this as available 
        heap_alloc_t * next_tag = get_next_heap_tag(new_heap_entry);
        next_tag->used = false;
        next_tag->size = avail_size - (requested_size + sizeof(heap_alloc_t) );
    }
    else{
        new_heap_entry->size = avail_size;
    }
}

static void * init_new_heap_range(void * start, void * end){
    
    //alloc mem for the range at the beginning of the provided new range
    init_heap_alloc_entry(start, sizeof(heap_range_t), HEAP_SIZE - sizeof(heap_alloc_t));

    //init the range
    heap_range_t * new_heap_range = start + sizeof(heap_alloc_t);
    new_heap_range->start = start;
    new_heap_range->end = end;
    new_heap_range->first_free_tag = start + sizeof(heap_alloc_t) + sizeof(heap_range_t);
    new_heap_range->next = NULL;

    //insert the range into the list
    heap_range_t * range = heap_ranges;
    while(range != NULL){
        range = range->next;
    }
    range = new_heap_range;

    return new_heap_range;
}

static void * goto_first_free_tag(heap_range_t * range, heap_alloc_t * starting_tag){

    //iterate through the heap, checking heap entries along the way to see if they are used or not

    heap_alloc_t * heap_entry = (heap_alloc_t*) starting_tag;

    while(heap_entry < range->end && heap_entry->used == true){

        heap_entry = get_next_heap_tag(heap_entry);
    }

    if(heap_entry->used == false){
        return heap_entry;
    }
    else{
        return NULL;
    } 
}

/**
 * @param requested_alloc_size this is the size of the requested kmalloc, NOT how much more mem will be requested from the vmm. the amount of mem requested from the vmm is static and determined by a macro
 */
static void * get_more_mem(size_t requested_alloc_size){
     
    //we see if we can extend any of the current heap ranges first

    heap_range_t * range = heap_ranges;

    while (range != NULL){
        int res = vmalloc_request_more_memory(range->start, NUM_PAGES_REQUESTED_WHEN_FULL*PAGE_SIZE);
        if(!res){
            range = range->next;
            continue;
        }
        void * new_alloc_start = range->end;
        range->end += NUM_PAGES_REQUESTED_WHEN_FULL*PAGE_SIZE;
        init_heap_alloc_entry(new_alloc_start, requested_alloc_size, NUM_PAGES_REQUESTED_WHEN_FULL*PAGE_SIZE - sizeof(heap_alloc_t));
        //may need to update the range if it is null before this new mem is allocated
        range->first_free_tag = goto_first_free_tag(range, range->start);
        return new_alloc_start;
    }

    //at this point we cannot extend any current heap ranges, need to make a new range
    void * new_range_start = vmalloc(NUM_PAGES_REQUESTED_WHEN_FULL*PAGE_SIZE);
    heap_range_t * new_range = (heap_range_t *) init_new_heap_range(new_range_start, new_range_start + (NUM_PAGES_REQUESTED_WHEN_FULL*PAGE_SIZE));
    //this is guaranteed to work since it is a fresh page (assuming the alloc request is < mem received from the vmm plus the heap overhead)
    heap_alloc_t * new_alloc_area = new_range->first_free_tag;
    init_heap_alloc_entry(new_alloc_area, requested_alloc_size, new_alloc_area->size);
    //may need to update the range if it is null before this new mem is allocated
    range->first_free_tag = goto_first_free_tag(range, range->start);
    return new_alloc_area;

}

static void * find_first_fit(size_t requested_size){
    
    heap_range_t * range = heap_ranges;
    
    while(range != NULL){

        //may be null if there are no free tags
        heap_alloc_t * heap_entry = (heap_alloc_t*) range->first_free_tag;

        while(heap_entry && heap_entry < range->end){

            if(heap_entry->used == false && heap_entry->size >= requested_size){
                //may need to move the first open spot if we've taken it
                init_heap_alloc_entry(heap_entry, requested_size, heap_entry->size);
                if(heap_entry == range->first_free_tag){
                    heap_alloc_t * next_tag = (char*)heap_entry + heap_entry->size + sizeof(heap_alloc_t);
                    range->first_free_tag = goto_first_free_tag(range, next_tag);
                }
                return heap_entry;
            }
            //if not open or not big enough, keep going
            else{
                heap_entry = get_next_heap_tag(heap_entry);
            }
        }

        //at this point there is no space in this heap range big enough

        if(!range->next){
            break;
        }
        range = range->next;
    }

    //at this point no heap range is big enough, need to request more mem 
    return NULL;
}


void * kmalloc(size_t requested_size){

    //traverse to find big enough spot in the avail mem
    heap_alloc_t * new_heap_entry = find_first_fit(requested_size);
    if(!new_heap_entry){
        new_heap_entry = (heap_alloc_t *) get_more_mem(requested_size);
    }

    //return address immediately after the new heap entry
    return (char*)new_heap_entry + sizeof(heap_alloc_t);
}

void kfree(void * start_address_for_allocation){

    //find the range it belongs to
    heap_range_t * range = heap_ranges;
    while(range != NULL){
        if(start_address_for_allocation >= range->start && start_address_for_allocation < range->end){
            break;
        }
        range = range->next;
    }

    if(!range){
        //attempt to free mem that has not been allocated to the heap
        return;
    }

    heap_alloc_t * tag_to_free = start_address_for_allocation - sizeof(heap_alloc_t);
    tag_to_free->used = false;
    
    //TODO: for now, can only merge with free blocks after it. extend this
    heap_alloc_t * next_entry = get_next_heap_tag(tag_to_free);

    if(next_entry->used == false){
        tag_to_free->size += next_entry->size + sizeof(heap_alloc_t);
    }

    range->first_free_tag = goto_first_free_tag(range, range->start);
}

void init_kheap(void * heap_start){

    //create the first kheap alloc range
    heap_ranges = init_new_heap_range(heap_start, heap_start + HEAP_SIZE);
}
