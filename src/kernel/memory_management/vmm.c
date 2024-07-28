#include <stddef.h>
#include <stdint.h>
#include "vmm.h"
#include "utils.h"
#include "kheap.h"
#include "paging.h"
#include "pmm.h"

/* Manages the remaining kernel address space and allocates pages */

//this region is located after the pmm bitmap in the kernel virtual address space
unsigned long dynamic_alloc_area_start;
unsigned long dynamic_alloc_area_end;

dynam_alloc_area_t * allocated_areas_list;

extern unsigned long pmm_bitmap_end_addy;

void init_vmm(){
    //this leaves a one page gap between PMM bitmap and dynamic alloc area start to guard against accidental out of bounds errors
    dynamic_alloc_area_start = round_up_to_nearest_page(pmm_bitmap_end_addy) + PAGE_SIZE;
   
}

void init_dynamic_alloc_area(dynam_alloc_area_t * dest, unsigned long start_address, size_t size, size_t flags){
    dest->start_address = start_address;
    dest->length = size;
    dest->flags = flags;
    dest->next_area = NULL;
}

void * vmm_first_heap_alloc(void){

    //get a page from PMM
    void * pageframe_address = alloc_pageframe();

    size_t starting_flags = VM_AREA_WRITE;

    //this is a unique situation where the allocated address of our dynamic alloc area structure will not be from kmalloc. This is because the heap can't get a page from the vmm, because vmalloc relies on kmalloc to create a dynamic allocate area to track the area its about to allocate. Instead, the vmm allocates a page and itself chooses the beginning of the page as the area to place its dynamic alloc struct (after building a heap entry for the kheap tp keep track of the allocation on the page itself that the vmm performed). The heap will be handed this page once the vmm completes allocation, mapping, and creating the first heap entry for its own dynamic alloc struct.

    void * alloc_page_virtual_address = (void *) dynamic_alloc_area_start;

    dynam_alloc_area_t * first_dynamic_alloc_area_struct = (dynam_alloc_area_t *) (dynamic_alloc_area_start + sizeof(heap_alloc_t));

    //need to map the next avail VMM address to the page allocated from the PMM before trying to write to this page
    //TODO: reference some global structure to get the PGD that will hold the current task, not just hardcoding kernel page directory
    bool allocated = map_pageframe(&kernel_PGD, pageframe_address, alloc_page_virtual_address , starting_flags);
    if(allocated){
        //the heap entry starts at the first address of this region
        init_heap_alloc_entry(alloc_page_virtual_address, sizeof(dynam_alloc_area_t));

        //there's two things going on here: the first is that we are allocating a page for the kheap. the first available location for this page in the virtual vmm region is the start of the region. so, a dynamic alloc area struct needs to be created to represent this virtual address range and page that will be allocated to the kheap. secondly, the place where this dynamic alloc area struct will be placed is actually right at the start of this allocated page. so there is a rare case where the dynamic alloc area that is being created is located at/in the same place it is allocating to the kheap.
        init_dynamic_alloc_area(first_dynamic_alloc_area_struct, (unsigned long)alloc_page_virtual_address, PAGE_SIZE, starting_flags);

        allocated_areas_list = first_dynamic_alloc_area_struct;

        //return the virtual page
        return (void*) dynamic_alloc_area_start;
    }
    else{
        //TODO: printk here
        return NULL;
    }
    
    
}

static void * find_alloc_area(unsigned long target_address){
    //iterate through linked list until area is found
}

static void * find_optimal_alloc_area(){
    //iterate through the linked list until space big enough is found
    //take the first avail page after the entry before this (or if this is the start, the starting address of this region) and return it 
}

//the VMM decides where in the dynamic alloc area to allocate the space being used
void * vmalloc(unsigned long requested_size){

    //request for some amount of bytes
    //rounds up to whole # of pages
    //finds suitable area for this alloc
    //requests the whole # pages from PMM
    //maps each page as it gets it from PMM to the curr PGD 
    //creates dynamic alloc area with kmalloc
    //initializes dynamic alloc area w/ suitable address found and used in the PTEs created
    //inserts it into dynamic alloc area list at the found optimal pos

}

bool vmalloc_request_virtual_pages(unsigned long virtual_page_address, size_t num_pages){
    //should prob perform some access control checks, but won't for now
    //iterate through list to see if this region has been created already, or if it starts exactly when a region ends, so that an existing region can just have its size be extended
}

bool vmalloc_free(unsigned long virtual_page_address){
    //search dynamic alloc area list for entry w/ this starting virtual address
    //check if MMIO. if so, skip the next step (ideally should have a refcount for all of this)
    //retrieve the physical address of the allocated page(s), then free them
    //delete it from the linked list
}

bool vmalloc_mmio_region(unsigned long mmio_region_phys_address, size_t length){
    //iterate through list to see if this region has been created already
    //if not, request specific page(s) from PMM
    //create dynamic alloc area with kmalloc?
    //initialize dynamic alloc area
    //create mappings in curr PGD
    //ret
}