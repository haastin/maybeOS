#include <stddef.h>
#include <stdint.h>
#include "vmm.h"
#include "utils.h"
#include "kheap.h"
#include "paging.h"
#include "pmm.h"
#include "mem_map.h"

/* Manages the remaining kernel address space and allocates pages */

//this region is located after the pmm bitmap in the kernel virtual address space
unsigned long dynamic_alloc_area_start;
unsigned long dynamic_alloc_area_end;

dynam_alloc_area_t * allocated_areas_list;

void init_dynamic_alloc_area(dynam_alloc_area_t * dest, unsigned long start_address, size_t size, size_t flags){
    dest->start_address = start_address;
    dest->length = size;
    dest->flags = flags;
    dest->next_area = NULL;
}

void * vmalloc_core(unsigned long virtual_address, size_t size, size_t flags){

    //only MMIO requests can request a specific kernel virtual address (if it is kernel space)
    if(flags & VM_AREA_MMIO){
        void * pageframe_address = alloc_requested_pageframe(virtual_address, size);
        //because the caller needs a specific phys address, do not try to allocate anything else if the original allocation fails
        if(!pageframe_address){
            return NULL;
        }
        else{
            bool mapped = direct_map_pageframes(&kernel_PGD, pageframe_address, flags, num_pages_needed(size));
            if(!mapped){
                //TODO: printk error here
                return NULL;
            }
        }
    }
    else{
        void * pageframe_address = alloc_pageframe(size);
        //in this case the allocation could fail becasue the pmm tries to give contiguous pages, so noncontiguous physical pages are taken from the pmm and made contiguous in the virtual address space of the heap; this is what Linux's vmalloc does
        if(!pageframe_address){
            
            size_t num_pages_needed = num_pages_needed(size);
            
            for(size_t page=0; page<num_pages_needed; page++){
                
                void * pageframe_address = alloc_pageframe(PAGE_SIZE);
                
                if(pageframe_address){
                    bool mapped = map_pageframe(&kernel_PGD, pageframe_address, virtual_address, flags);
                    if(!mapped){
                        //TODO: printk error here- mapping failed
                        return NULL;
                    }
                }
                else{
                    //TODO: printk error here- PMM OUT OF MEM
                    return NULL;
                }
            }
        }
        else{
            bool mapped = map_contiguous_pages(&kernel_PGD, pageframe_address, virtual_address, flags, size);
            if(!mapped){
                //TODO: printk error here
                return NULL;
            }
        }
    }
    return virtual_address;
}

void * vmm_first_heap_alloc(void){

    //heap's paging flags
    size_t heap_flags = VM_AREA_WRITE;

    void * kheap_virtual_address_start = vmalloc_core(dynamic_alloc_area_start, HEAP_SIZE, heap_flags);

    init_kheap(kheap_virtual_address_start);

    // dynam_alloc_area_t * first_dynamic_alloc_area_struct = (dynam_alloc_area_t *) kmalloc(sizeof(dynam_alloc_area_t));

    
   
    //     //the heap entry starts at the first address of this region
    //     init_heap_alloc_entry(alloc_page_virtual_address, sizeof(dynam_alloc_area_t));

    //     //there's two things going on here: the first is that we are allocating a page for the kheap. the first available location for this page in the virtual vmm region is the start of the region. so, a dynamic alloc area struct needs to be created to represent this virtual address range and page that will be allocated to the kheap. secondly, the place where this dynamic alloc area struct will be placed is actually right at the start of this allocated page. so there is a rare case where the dynamic alloc area that is being created is located at/in the same place it is allocating to the kheap.
    //     init_dynamic_alloc_area(first_dynamic_alloc_area_struct, (unsigned long)alloc_page_virtual_address, PAGE_SIZE, starting_flags);

    //     allocated_areas_list = first_dynamic_alloc_area_struct;

    //     //return the virtual page
    //     return (void*) dynamic_alloc_area_start;
    // }
    // else{
    //     //TODO: printk here
    //     return NULL;
    // }
    
    
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


//TODO: NEED TO DEBUG STARTING HERE 
void init_vmm(){
    //this leaves a one page gap between PMM bitmap and dynamic alloc area start to guard against accidental out of bounds errors
    dynamic_alloc_area_start = round_up_to_nearest_page(pmm_bitmap_end_addy) + PAGE_SIZE;
    //TODO: right now I'm assigning the rest of the kernel virtual address space to the kernel dynamic allocator; as more features are added in the future this will likely change
    //TODO: instead of using specific cpu's mem, should make it general
    dynamic_alloc_area_end = boot_cpu_mem.start + boot_cpu_mem.length;
   
}