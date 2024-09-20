#include <stddef.h>
#include <stdint.h>
#include "vmm.h"
#include "utils.h"
#include "kheap.h"
#include "paging.h"
#include "pmm.h"
#include "mem_map.h"

/* Manages the remaining kernel address space and allocates pages */

//this region is located after the pmm bitmap in the kernel virtual address space. 
//*these addreses should be multiples of pages
unsigned long dynamic_alloc_area_bottom;
unsigned long dynamic_alloc_area_top;

//is sorted from greatest to least, so the highest allocated addresses will be here
dynam_alloc_area_t * allocated_areas_list;

/**
 * After a new virtual address range has been found for the request, physical memory has been alloated for it, and its paging entries created, this function will be called to create a dynamic_alloc_area_t to track this new area
 * @param size the size of the allocation in bytes
 * @param flags the arch-independent flags 
 */
void init_dynamic_alloc_area(dynam_alloc_area_t * dest, unsigned long start_address, size_t size, size_t flags){

    size_t num_pages_needed = num_pages_needed(size);

    dest->start_address = start_address;
    dest->length = num_pages_needed*PAGE_SIZE;
    dest->flags = flags;
    dest->next_area = NULL;
}

/**
 * Handles allocations for MMIO devices and general allocations by making a request to the PMM for physical pages and then calling the paging subsystem to map the newly allocated physical mem at the entries for the virtual address range passed to this func. Direct calls are made to this func, bypassing vmalloc, when the desired virtual address range is already known
 * @param virtual_address the virtual address starting address found suitable to be allocated to the requestee
 * @param size the size of the allocation in bytes
 * @param flags the arch-independent flags
 * @return the start of the allocated virtual address range, or NULL if not possible to allocate or map 
 */
void * vmalloc_core(unsigned long virtual_address, size_t size, size_t flags){

    size_t num_pages_needed = num_pages_needed(size);

    //only MMIO requests can request a specific kernel virtual address (if it is kernel space)
    if(flags & VM_AREA_MMIO){
        //map the physical address if it isn't already. unlike the other cases, its ok if the physical allocation fails because this MMIO may already be allocated and mapped, in which we just try to allocate it to the current PGD
        alloc_requested_pageframe(virtual_address, size);
        
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
                    bool mapped = map_pageframe(&kernel_PGD, pageframe_address, (void *) (virtual_address + page*PAGE_SIZE), flags);
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
            bool mapped = map_contiguous_pages(&kernel_PGD, (unsigned long) pageframe_address, virtual_address, flags, num_pages_needed);
            if(!mapped){
                //TODO: printk error here
                return NULL;
            }
        }
    }
    return (void *) virtual_address;
}

/**
 * Simply checks if the address passed is within a particular dynamic_alloc_area_t's virtual address range
 */
static inline bool virtyAddyIsInArea(unsigned long area_start,unsigned long area_end,unsigned long virtual_address){
    if(virtual_address >= area_start && virtual_address < area_end){
            return true;
        }
    return false;
}

static dynam_alloc_area_t * find_vm_area_starting_at(unsigned long virtual_address){
    if(!allocated_areas_list){
        return NULL;
    }

    dynam_alloc_area_t * curr = allocated_areas_list;
    while(curr){
        if(curr->start_address == virtual_address){
            return curr;
        }
        curr = curr->next_area;
    }
    return NULL;
}

/**
 * Find and return, if possible, the dynam_alloc_area_t that start_address is in
 * @param size the size of the allocation in bytes
 */
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

//**** THIS CODE BELOW WAS BASED ON WRONG ASSUMPTIONS OF HOW RECURSIVE PAGING WORKS. It still may prove useful if I use recursive paging with more than just a PGD and PTs, so I'll keep it here. */
// /**
//  * Checks if a provided contiguous virtual address includes a recursive entry, and decides if adjusting the range to exclude this recursive entry is viable or not. 
//  * @param potential_start should be an address LOWER than end
//  */
// static void * find_valid_address_range_without_recursive_address_within_given_range(unsigned long potential_start, unsigned long potential_end, unsigned long next_area_end, size_t requested_size){
//     //Again, this func assumes the address range must 1023 pages in size or smaller, so that it cannot overlap two seperate recursive entries

//     int end_pde_idx = get_PDE_idx(potential_end);
//     int start_pde_idx = get_PDE_idx(potential_start);

//     int end_pte_idx = get_pte_idx(potential_end);
//     int start_pte_idx = get_pte_idx(potential_start);

//     if(start_pde_idx == end_pde_idx){
//         if(!(start_pte_idx <= PAGE_TAB_RECURSIVE_ENTRY_INDEX && end_pte_idx > PAGE_TAB_RECURSIVE_ENTRY_INDEX)){
//             return potential_start;
//         }
//         //if it does overlap a recursive entry, see if starting after the recursive entry leaves enough space
//         else{
//             int pte_idx_before_recursive_pte_idx = PAGE_TAB_RECURSIVE_ENTRY_INDEX - 1;
//             unsigned long next_avail_new_area_end = construct_virtual_address(end_pde_idx, pte_idx_before_recursive_pte_idx);

//             size_t avail_size = next_avail_new_area_end - next_area_end;
//             if(avail_size >= requested_size){
//                 return next_avail_new_area_end - requested_size;
//             }
//         }
//     }
//     else if(start_pde_idx < end_pde_idx){

//         //it is ok if end is = page tab recursive because end is the address AFTER the entire addess range
//         if(end_pte_idx <= PAGE_TAB_RECURSIVE_ENTRY_INDEX && start_pte_idx > PAGE_TAB_RECURSIVE_ENTRY_INDEX){
//             return potential_start;
//         }
//         //this else is reached if there is currently a recursive entry in the range
//         else{
//             //end will always have to move before a recursive entry (before because end will always need to move to a lower address if there is a recursive address in the attempted range); the other diff is whether it will be within its own pde or starts
//             int pte_idx_before_recursive_pte_idx = PAGE_TAB_RECURSIVE_ENTRY_INDEX - 1;

//             //less than because if end_pte is == recursive pte then we know start is above its own table's recursive
//             if(end_pte_idx < PAGE_TAB_RECURSIVE_ENTRY_INDEX){
//                 //note the pde now is the same as start, because end needs to be moved to a lower address before the recursive entry of start's table to make the range even theoeretically possible
//                 unsigned long next_avail_new_area_end = construct_virtual_address(start_pde_idx, pte_idx_before_recursive_pte_idx);

//                 size_t avail_size = next_avail_new_area_end - next_area_end;
//                 if(avail_size >= requested_size){
//                     return next_avail_new_area_end - requested_size;
//                 }
//             }
//             else{
//                 //note the pde now is the same as start, because end needs to be moved to a lower address before the recursive entry of start's table to make the range even theoeretically possible
//                 unsigned long next_avail_new_area_end = construct_virtual_address(end_pte_idx, pte_idx_before_recursive_pte_idx);

//                 size_t avail_size = next_avail_new_area_end - next_area_end;
//                 if(avail_size >= requested_size){
//                     return next_avail_new_area_end - requested_size;
//                 }
//             }
//         }
//     }
//     else{
//         //TODO: printk - start should always be a lower address than end; if it has a higher pde it has a higher address than end, which means something has gone wrong
//         return NULL;
//     }

//    return NULL;
// }

/**
 * Returns the vm area that the new area will be inserted after
 * @param requested_size the size of the allocation in bytes
 * @return the starting address of the first suitable region found, or NULL if there is no region big enough in the entire virtual address space
 */
static void * find_first_alloc(size_t requested_size){
    //TODO: the way this works is that it allocates at the first address after the last allocation (all allocations by the vmm are in pages so the address will still be a multiple of a page), but this kinda screws the vmalloc_request_more_mem. change the default behavior of this?

    //TODO: if a dynamic alloc area is large enough for the request but crosses a recursive entry it fails, so we try to allocate from after it instead. in this case we also check if it crosses another recursive entry (the one after the one it originally crossed), which would imply the request is large enough to go across 1023 pages. i think instead of doing this check i should just disable any request which would require that # of pages 

    if(!allocated_areas_list){
        //normal vmalloc allocations will start at the top and grow down, like a stack
        return (void*) (dynamic_alloc_area_top- num_pages_needed(requested_size));
    }
    
    //need a case to check if there's space before the head of the dynamic_alloc_t list because the initial allocation for an empty lsit that goes right up to the end of the vmm area may have been freed, but if there's other allocations in the list then allocated areas_list will not be null, but the area freed previously will now be available and could fit this new alloc
    //need to place that case here to detect whether the new allocation is larger than the first because the loop logic fails to detect that case
    unsigned long next_area_end = allocated_areas_list->start_address + allocated_areas_list->length;
    if(dynamic_alloc_area_top - requested_size >= next_area_end){
        return (void *) (dynamic_alloc_area_top-requested_size);
    }
    // void * found_start = find_valid_address_range_without_recursive_address_within_given_range(dynamic_alloc_area_top - requested_size, dynamic_alloc_area_top, next_area_end, requested_size);
    // if(found_start){
    //     return found_start;
    // }

    dynam_alloc_area_t * prev = allocated_areas_list;
    dynam_alloc_area_t * next = allocated_areas_list->next_area;

    while(next){

        unsigned long prev_area_start = prev->start_address;
        unsigned long next_area_end = next->next_area->start_address + next->next_area->length;

        //prev start - next end because the list is sorted greatest address range to least, so prev will be higher address
        size_t avail_size = prev_area_start - next_area_end;
        
        if(avail_size >= requested_size){
            
            unsigned long potential_new_area_start = prev_area_start - requested_size;
            if(potential_new_area_start >= next_area_end){
                return (void*) (prev_area_start-requested_size);
            }
            // void * found_start = find_valid_address_range_without_recursive_address_within_given_range(potential_new_area_start, prev_area_start, next_area_end, requested_size);
            // if(found_start){
            //     return found_start;
            // }
        }
        prev = next;
        next = next->next_area;
    }

    //at this point prev is the last range in the list, so there has been no places suitable inside the list. the last possible case is after this last range but before the bottom of this area
    unsigned long prev_area_start = prev->start_address;
    unsigned long potential_new_area_start = prev_area_start - requested_size;
    if(potential_new_area_start >= dynamic_alloc_area_bottom){
        return (void*) (prev_area_start-requested_size);
    }
    // void * found_start = find_valid_address_range_without_recursive_address_within_given_range(prev_area_start-requested_size, prev_area_start, dynamic_alloc_area_bottom, requested_size);
    // if(found_start){
    //     return found_start;
    // }
    return NULL;
}

/**
 * This exists as a standalone function because some calls will not call find_first_alloc if they are requesting a specific range, so these regions have not interacted with the dynam_alloc_area_t linked list yet and need to be inserted after having their dynam_alloc_area_t created and initialized. 
 */
static void insert_new_vm_area(dynam_alloc_area_t * new_vm_area){

    if(!allocated_areas_list){
        allocated_areas_list = new_vm_area;
        return;
    }
    //need special logic to detect whether the new allocation is larger than the first because the loop logic fails in that case
    else if(new_vm_area->start_address > allocated_areas_list->start_address){
        new_vm_area->next_area = allocated_areas_list;
        allocated_areas_list = new_vm_area;
        return;
    }

    dynam_alloc_area_t * prev = allocated_areas_list;
    dynam_alloc_area_t * next = allocated_areas_list->next_area;

    //compares the NEXT (not prev) area with our newly allocated range (prev should already have been determined to be bigger)
    while(next){
        
        unsigned long next_area_start = next->next_area->start_address;
        unsigned long next_area_end = next_area_start + next->length;
        
        //have found the placed in the list for this
        if(new_vm_area->start_address > next_area_start){
            prev->next_area = new_vm_area;
            new_vm_area = next;
            return;
        }
        else if(new_vm_area->start_address < next_area_end){
            prev = next;
            next = next->next_area;
        }
        else if (new_vm_area->start_address == next_area_end){
            //TODO: printk error- shouldnt reach here, because that would mean the new area has a start addy == the area being checked, which shouldnt happen because it should already be checked if this area address range exists
            return;
        }
    }
    //if this point is reached prev is at the end of the list and so the new area is inserted as the new tail
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

/**
 * The vmm initializes the kheap itself because the range of memory allocated to the kheap needs to be tracked as a dynam_alloc_area_t, but if the kheap itself calls vmalloc, vmalloc itself dpeends on kmalloc being active, and calling it before kheap init is finished would break it
 */
void vmm_init_kheap(void){

    void * kheap_virtual_address_start = vmalloc_core(dynamic_alloc_area_top-HEAP_SIZE, HEAP_SIZE, VM_AREA_WRITE);

    init_kheap(kheap_virtual_address_start);

    //need to give the heap memory and init it first in order to be able to make dynamic alloc entries, so the heap must be the first thing initialized

    dynam_alloc_area_t * first_dynamic_alloc_area_struct = (dynam_alloc_area_t *) kmalloc(sizeof(dynam_alloc_area_t));
    init_dynamic_alloc_area(first_dynamic_alloc_area_struct, (unsigned long) kheap_virtual_address_start, HEAP_SIZE, VM_AREA_WRITE);
    insert_new_vm_area(first_dynamic_alloc_area_struct);  

    //track the memory range that is taken up by the PGD recursive entry. no need to call vmalloc because i dont need to map this region due to the fact that it will never be used, so allocating to it would just waste pages. all that needs to be done is to make sure the vmm knows this area cannot be allocated.
    dynam_alloc_area_t * recursive_pgd_space = (dynam_alloc_area_t *) kmalloc(sizeof(dynam_alloc_area_t));
    unsigned long num_bytes_mapped_by_one_pde = ((RECURSIVE_PDE_IDX+1)<< PAGE_DIR_INDEX_BIT_INDEX) - (RECURSIVE_PDE_IDX << PAGE_DIR_INDEX_BIT_INDEX);
    init_dynamic_alloc_area(recursive_pgd_space, (RECURSIVE_PDE_IDX << PAGE_DIR_INDEX_BIT_INDEX), num_bytes_mapped_by_one_pde, VM_AREA_NO_ACCESS);
    insert_new_vm_area(recursive_pgd_space);
}

/**
 * Vmalloc decides where to allocate the requested memory, and then calls vmalloc_core and takes care of the dynam_alloc_area_t procedures needed to track this newly allocated area 
 */
void * vmalloc(size_t requested_size, size_t flags){
    
    void * new_vm_area_starting_address = find_first_alloc(requested_size);

    void * allocated_address_start = vmalloc_core((unsigned long)new_vm_area_starting_address,requested_size,flags);

    if(!allocated_address_start){
        //TODO: printk blah blah blah
        return NULL;
    }

    dynam_alloc_area_t * new_vm_area = kmalloc(sizeof(dynam_alloc_area_t));

    init_dynamic_alloc_area(new_vm_area,(unsigned long) allocated_address_start, requested_size, flags);

    insert_new_vm_area(new_vm_area);

    return allocated_address_start;

}

/**
 * 
 * @param requested_size requested allocation size in bytes
 */
void * vmalloc_request_virtual_address(unsigned long virtual_address, size_t requested_size, size_t flags){
    
    //iterate through list to see if this region has been created already

    if(!virt_region_exists_in_area(virtual_address, requested_size)){

        void * allocated_address = vmalloc_core(virtual_address, requested_size, flags);

        dynam_alloc_area_t * new_vm_area = kmalloc(sizeof(dynam_alloc_area_t));

        init_dynamic_alloc_area(new_vm_area, (unsigned long)allocated_address, requested_size, flags);

        insert_new_vm_area(new_vm_area);

        return allocated_address;
    }
    else{
        //if a process is looking to get a device mapped into it, it is ok that the region already exists or has already been mapped. the only reason it should ever return NULL for a MMIO mapping is if the caller doesnt have the requisite permission
        if(flags & VM_AREA_MMIO){
            return (void *) virtual_address;
        }
        else{
            return NULL;
        }
    }
}

/**
 * Right now mainly used by the heap to extend memory
 * @param starting_address the start address of region being extended, NOT the start of the newly allocated mem
 * @param extension_size the additional size requested in bytes
 * @return The start of the newly allocated area that extends the original area
 */
void * vmalloc_request_more_memory(unsigned long starting_address, size_t extension_size){

    //TODO: prob need to do access control checks to make sure the proc/kernel obj requesting more mem owns the range it is trying to extend

    dynam_alloc_area_t * curr_area = find_vm_area_starting_at(starting_address);
    if(!curr_area){
        //TODO: printk- no region exists with this starting address
        return NULL;
    }

    //need to make sure no other regions are overlapping the requested new mem first
    unsigned long curr_area_end = curr_area->start_address + curr_area->length;
    if(!virt_region_exists_in_area(curr_area_end, extension_size)){
        void * new_area = vmalloc_core(curr_area_end, extension_size, VM_AREA_WRITE);
        curr_area->length += extension_size;
        return new_area;
    }
    else{
        //TODO: printk - not enough room before next allocated virtual area to give extension_size mem. need to do a vmalloc call instead
        return NULL;
    }
}

/**
 * 
 */
bool vmalloc_free(unsigned long virtual_page_address){

     //TODO: this should truly be decrementing a refcount

    //search dynamic alloc area list for entry w/ this starting virtual address
    dynam_alloc_area_t * area_to_be_removed = find_vm_area_starting_at(virtual_page_address);
    
    if(area_to_be_removed->flags & VM_AREA_MMIO){
        //don't deallocate mem for a device MMIO
        return true;
    }

    //free the allocated pages and virtual mappings
    size_t num_pages = num_pages_needed(area_to_be_removed->length);
    for(size_t page=0; page<num_pages; page++){

        unsigned long virt_addy_to_free = area_to_be_removed->start_address + page*PAGE_SIZE;

        //free the physical page from the pmm
        unsigned long pte = (unsigned long) get_pte((void*)&kernel_PGD, virt_addy_to_free);
        unsigned long pageframe_allocated = get_pageframe_address(pte);
        bool freed = free_pageframe(pageframe_allocated);
        if(!freed){
            //TODO: printk error
            return false;
        }

        //free the page table mapping for this page
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
    dynamic_alloc_area_bottom = round_up_to_nearest_page(pmm_bitmap_end_addy) + PAGE_SIZE;
    
    //TODO: right now I'm assigning the rest of the kernel virtual address space to the kernel dynamic allocator; as more features are added in the future this will likely change
    
    //TODO: instead of using specific cpu's mem, should make it general
    dynamic_alloc_area_top = (unsigned long)boot_cpu_mem.start + boot_cpu_mem.length;

    allocated_areas_list = NULL;
}