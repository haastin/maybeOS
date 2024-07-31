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

//is sorted from greatest to least, so the highest allocated addresses will be here
dynam_alloc_area_t * allocated_areas_list;

void init_dynamic_alloc_area(dynam_alloc_area_t * dest, unsigned long start_address, size_t size, size_t flags){

    size_t num_pages_needed = num_pages_needed(size);

    dest->start_address = start_address;
    dest->length = num_pages_needed*PAGE_SIZE;
    dest->flags = flags;
    dest->next_area = NULL;
}

void * vmalloc_core(unsigned long virtual_address, size_t size, size_t flags){

    size_t num_pages_needed = num_pages_needed(size);

    //only MMIO requests can request a specific kernel virtual address (if it is kernel space)
    if(flags & VM_AREA_MMIO){
        void * pageframe_address = alloc_requested_pageframe(virtual_address, size);
        
        //unlike the other cases, its ok if the physical allocation fails because this MMIO may already be allocated and mapped, in which we just try to allocate it to the current PGD
        
        //because the physical alloc may fail (this is ok), we pass the virtual address as the virtual address since we identity map MMIO devices
        bool mapped = identity_map_pageframes(&kernel_PGD, virtual_address, flags, num_pages_needed);
        if(!mapped){
            //TODO: printk error here
            return NULL;
        }
    }
    else{
        void * pageframe_address = alloc_pageframe(size);
        //in this case the allocation could fail becasue the pmm tries to give contiguous pages, so noncontiguous physical pages are taken from the pmm and made contiguous in the virtual address space of the heap; this is what Linux's vmalloc does
        if(!pageframe_address){
            
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
            bool mapped = map_contiguous_pages(&kernel_PGD, pageframe_address, virtual_address, flags, num_pages_needed);
            if(!mapped){
                //TODO: printk error here
                return NULL;
            }
        }
    }
    return virtual_address;
}

static inline bool virtyAddyIsInArea(unsigned long area_start,unsigned long area_end,unsigned long virtual_address){
    if(virtual_address >= area_start && virtual_address < area_end){
            return true;
        }
    return false;
}

static dynam_alloc_area_t * find_vm_area_at(unsigned long virtual_address){
    if(!allocated_areas_list){
        return NULL;
    }

    dynam_alloc_area_t * curr = allocated_areas_list;
    while(curr){
        if(curr->start_address = virtual_address){
            return curr;
        }
        curr = curr->next_area;
    }
    return NULL;
}

static dynam_alloc_area_t * virt_region_exists_in_area(unsigned long start_address, size_t size){

    if(!allocated_areas_list){
        return NULL;
    }

    /**
     * three cases:
     * the attempted region size is > the region being checked, so the start and end can be outside of the region, and both the start and end of the region being checked will inhabit the attempted region
     * the attempted region size is < the region being checked, in which case either the start or end can inhabit it
     * the attempted region size is = the region being checked, in which case either the start or end can inhabit it
     */

    unsigned long end = start_address + size;

    dynam_alloc_area_t * curr = allocated_areas_list;

    while(curr){

        unsigned long area_start = curr->start_address;
        unsigned long area_end = area_start + curr->length;

        if(virtyAddyIsInArea(area_start,area_end,start_address)){
            return curr;
        }
        else if(virtyAddyIsInArea(area_start,area_end,end)){
            return curr;
        }
        else if(virtyAddyIsInArea(start_address,end,area_start)){
            return curr;
        }

        curr = curr->next_area;
    }
    return NULL;
}

/**
 * Returns the vm area that the new area will be inserted after
 */
static void * find_first_alloc(size_t requested_size){
    
    if(!allocated_areas_list){
        //normal vmalloc allocations will start at the top and grow down, like a stack
        return dynamic_alloc_area_end- num_pages_needed(requested_size);
    }

    dynam_alloc_area_t * prev = allocated_areas_list;
    dynam_alloc_area_t * next = allocated_areas_list->next_area;

    unsigned long prev_area_end = prev->start_address + prev->length;
    unsigned long next_area_start = next->next_area->start_address;

    while(next){

        //end - start because the list is sorted greatest to least
        size_t avail_size = prev_area_end - next_area_start ;
        
        if(avail_size >= requested_size){

            return prev_area_end;
        }
        else{
            prev = next;
            next = next->next_area;
        }

        prev_area_end = prev->start_address + prev->length;
        next_area_start = next->next_area->start_address;
    }
    return prev_area_end;
}

static void insert_new_vm_area(dynam_alloc_area_t * new_vm_area){

    if(!allocated_areas_list){
        allocated_areas_list = new_vm_area;
        return;
    }
    dynam_alloc_area_t * prev = allocated_areas_list;
    dynam_alloc_area_t * next = allocated_areas_list;
    dynam_alloc_area_t * temp;

    while(next){
        
        unsigned long next_area_start = next->next_area->start_address;
        unsigned long next_area_end = next_area_start = next->length;
        
        
        if(new_vm_area->start_address > next_area_start){
            //have found the placed in the list for this
            prev->next_area = new_vm_area;
            new_vm_area = next;
            return;
        }
        else if(new_vm_area->start_address < next_area_end){
            prev = next;
            next = next->next_area;
        }
        else{
            //TODO: printk error- shouldnt reach here, because it should already be checked if this area exists
            return;
        }
    }
    prev->next_area = new_vm_area;
}

static dynam_alloc_area_t * get_vm_area_prior(dynam_alloc_area_t * entry){

    if(!allocated_areas_list){
        return NULL;
    }
    
    dynam_alloc_area_t * curr = allocated_areas_list;

    while(curr){
        if(curr->next_area == entry){
            return curr;
        }
        curr = curr->next_area;
    }
    return NULL;
}

void vmm_init_kheap(void){

    //heap's paging flags
    size_t heap_flags = VM_AREA_WRITE;

    void * kheap_virtual_address_start = vmalloc_core(dynamic_alloc_area_start, HEAP_SIZE, heap_flags);

    init_kheap(kheap_virtual_address_start);

    dynam_alloc_area_t * first_dynamic_alloc_area_struct = (dynam_alloc_area_t *) kmalloc(sizeof(dynam_alloc_area_t));

    init_dynamic_alloc_area(first_dynamic_alloc_area_struct, kheap_virtual_address_start, HEAP_SIZE, heap_flags);

    insert_new_vm_area(first_dynamic_alloc_area_struct);  
    
}

//the VMM decides where in the dynamic alloc area to allocate the space being used
void * vmalloc(size_t requested_size, size_t flags){
    
    void * new_vm_area_starting_address = find_first_alloc(requested_size);

    void * allocated_address_start = vmalloc_core(new_vm_area_starting_address,requested_size,flags);

    if(!allocated_address_start){
        //TODO: printk blah blah blah
        return NULL;
    }

    dynam_alloc_area_t * new_vm_area = kmalloc(sizeof(dynam_alloc_area_t));

    init_dynamic_alloc_area(new_vm_area, new_vm_area, requested_size, flags);

    insert_new_vm_area(new_vm_area);

    return new_vm_area_starting_address;

}

void * vmalloc_request_virtual_address(size_t virtual_address, size_t requested_size, size_t flags){
    
    //iterate through list to see if this region has been created already

    if(!virt_region_exists_in_area(virtual_address, requested_size)){

        void * allocated_address = vmalloc_core(virtual_address, requested_size, flags);

        dynam_alloc_area_t * new_vm_area = kmalloc(sizeof(dynam_alloc_area_t));

        init_dynamic_alloc_area(new_vm_area, allocated_address, requested_size, flags);

        insert_new_vm_area(new_vm_area);

        return allocated_address;
    }
    else{
        //if a process is looking to get a device mapped into it, it is ok that the region already exists or has already been mapped. the only reason it should ever return NULL for a MMIO mapping is if the caller doesnt have the requisite permission
        if(flags & VM_AREA_MMIO){
            return virtual_address;
        }
        else{
            return NULL;
        }
    }
}

void * vmalloc_request_more_memory(unsigned long starting_address, size_t extension_size){
    
    dynam_alloc_area_t * curr_area = find_vm_area_at(starting_address);
    unsigned long curr_area_end = curr_area->start_address + curr_area->length;

    if(curr_area){

        //need to make sure no other regions are overlapping the requested new mem first
        dynam_alloc_area_t * existing_area = find_vm_area_at(curr_area_end);

        void * new_area = vmalloc_core(curr_area_end, extension_size, VM_AREA_WRITE);
        if(!new_area){
            //TODO: printk error
            return NULL;
        }
        return new_area;
    }
    else{
        //TODO: printk error
        return NULL;
    }
}

bool vmalloc_free(unsigned long virtual_page_address){

     //TODO: this should truly be decrementing a refcount

    //search dynamic alloc area list for entry w/ this starting virtual address
    dynam_alloc_area_t * area_to_be_removed = find_vm_area_at(virtual_page_address);
    
    if(area_to_be_removed->flags & VM_AREA_MMIO){
        //don't deallocate mem for a device MMIO
        return true;
    }

    //free the allocated pages and virtual mappings
    size_t num_pages = num_pages_needed(area_to_be_removed->length);
    for(size_t page=0; page<num_pages; page++){

        unsigned long virt_addy_to_free = area_to_be_removed->start_address + page*PAGE_SIZE;

        //free the physical page
        unsigned long pte = get_pte(&kernel_PGD, virt_addy_to_free);
        unsigned long pageframe_allocated = get_pageframe_address(pte);
        bool freed = free_pageframe(pageframe_allocated);
        if(!freed){
            //TODO: printk error
            return false;
        }

        //free the mapping
        bool freed_mapping = free_mapping(&kernel_PGD, virt_addy_to_free);
        if(!freed_mapping){
            //TODO: printk error
            return false;
        }
    }
    
    //delete it from the linked list and free from heap
    dynam_alloc_area_t * prev_entry = get_vm_area_prior(area_to_be_removed);
    if(!prev_entry){
        //TODO: printk error
        return false;
    }
    prev_entry->next_area = area_to_be_removed->next_area;

    kfree(area_to_be_removed);

    return true;
}

void init_vmm(){
    //this leaves a one page gap between PMM bitmap and dynamic alloc area start to guard against accidental out of bounds errors
    dynamic_alloc_area_start = round_up_to_nearest_page(pmm_bitmap_end_addy) + PAGE_SIZE;
    //TODO: right now I'm assigning the rest of the kernel virtual address space to the kernel dynamic allocator; as more features are added in the future this will likely change
    //TODO: instead of using specific cpu's mem, should make it general
    dynamic_alloc_area_end = boot_cpu_mem.start + boot_cpu_mem.length;

    allocated_areas_list = NULL;
   
}