
#include <stdint.h>
#include "paging.h"
#include "mem_map.h"
#include <stdbool.h>
#include "pmm.h"
#include "utils.h"
#include "string.h"
#include "vmm.h"

#define NUM_BITS_IN_ONE_BITMAP (sizeof(unsigned int)*8)

//defines the bounds of the bitmap we will use for dynamic memory alloc- this is technically the first "allocation" of the system since it dynamically sets the bounds of the bitmap depending on how much memory is detected by the bootloader. this memory will be marked as used in the bitmap after it is initialized
unsigned long pmm_bitmap_start_addy;
unsigned long pmm_bitmap_end_addy;
unsigned long bitmap_num_bits;

void * alloc_bitmap;

static inline unsigned int * get_pfn_parent_bitmap(void * bitmap_start, unsigned int pfn){
    
    //this is the num of unsigned ints from the start of the bitmap to the unsinged long the requested pfn resides in 
    unsigned int num_bitmaps_to_advance = pfn/NUM_BITS_IN_ONE_BITMAP;
    
    unsigned int * parent_bitmap = ((unsigned int *) bitmap_start) + num_bitmaps_to_advance;

    return parent_bitmap;
}

static bool test_bit(void * bitmap, size_t bit_index){

    unsigned int * parent_bitmap = get_pfn_parent_bitmap(bitmap, bit_index);
    unsigned char local_bit_idx = bit_index % NUM_BITS_IN_ONE_BITMAP;
    return *parent_bitmap & (1 << local_bit_idx);
}

static unsigned int find_first_avail_pfn(void * bitmap, unsigned long num_sequential_bits_needed){

    bitmap = (unsigned int *) bitmap;

    for(size_t bit_idx=0; bit_idx< bitmap_num_bits; bit_idx++){
        bool isAllocated = test_bit(bitmap, bit_idx);
        if(!isAllocated){
            unsigned int last_bit_idx = bit_idx + num_sequential_bits_needed - 1;
            bool enoughSpace = (last_bit_idx < bitmap_num_bits);
            if(!enoughSpace){
                return -1;
            }
            else{
                bool failed = false;
                for(size_t seq_bit_idx=bit_idx+1; seq_bit_idx <= (bit_idx+num_sequential_bits_needed); seq_bit_idx++)
                    {
                        bool isAllocated = test_bit(bitmap, seq_bit_idx);
                        if(isAllocated){
                            failed = true;
                            break;
                        }
                    }
                if(!failed){
                    return bit_idx;
                }
            }
        }
    }
    return -1;
}

static bool alloc_pfn(void * bitmap, unsigned int pfn){

    unsigned int * pfn_parent_bitmap = get_pfn_parent_bitmap(bitmap, pfn);
    unsigned int pfn_bit_index = pfn % NUM_BITS_IN_ONE_BITMAP;
    
    if(*pfn_parent_bitmap & (1 << pfn_bit_index)){
        //the bit was already set, meaning this was already allocated; should NOT reach this point
        //TODO: printk error here
        return false;
    }

    *pfn_parent_bitmap |= (1 << pfn_bit_index); 
    
    return true;
}

static bool free_pfn(void * bitmap, unsigned int pfn){
   
    unsigned int * pfn_parent_bitmap = get_pfn_parent_bitmap(bitmap, pfn);
    unsigned int pfn_bit_index = pfn % NUM_BITS_IN_ONE_BITMAP;
    
    if(!(*pfn_parent_bitmap & (1 << pfn_bit_index))){
        //the bit was already free, meaning this was already allocated; should NOT reach this point
        //TODO: printk error here
        return false;
    }

    *pfn_parent_bitmap &= ~(1 << pfn_bit_index);

    return true;
}

void * alloc_pageframe(size_t size){
    unsigned long num_sequential_bits_needed = num_pages_needed(size);
    unsigned int first_avail_pfn = find_first_avail_pfn(alloc_bitmap, num_sequential_bits_needed);
    if(first_avail_pfn == -1){
        return NULL;
    }
    for(size_t pfn=0; pfn<num_sequential_bits_needed; pfn++){
        bool alloc_success = alloc_pfn(alloc_bitmap, first_avail_pfn + pfn);
        if(!alloc_success){
            return NULL;
        }
    }
    return pfn_to_phys_addy(first_avail_pfn);
}

/**
 * @param physical_address 
 * @param size
 */
void * alloc_requested_pageframe(unsigned long physical_address, size_t size){
    unsigned int requested_pfn = (unsigned int) phys_addy_to_pfn(physical_address);
    unsigned long num_sequential_bits_needed = num_pages_needed(size);
    bool enoughSpace = ((requested_pfn + num_sequential_bits_needed) < bitmap_num_bits);
    if(!enoughSpace){
        return NULL;
    }
    for(size_t pfn=requested_pfn; pfn<(requested_pfn+num_sequential_bits_needed); pfn++){
        bool isAllocated = test_bit(alloc_bitmap, pfn);
        if(isAllocated){
            return NULL;
        }
    }
    //if we made it here, there is enough space
    for(size_t pfn=requested_pfn; pfn<(requested_pfn+num_sequential_bits_needed); pfn++){
        alloc_pfn(alloc_bitmap, pfn);
    }

    return physical_address;
}

bool free_pageframe(unsigned long pageframe_addy){
    unsigned int pfn_to_free = phys_addy_to_pfn(pageframe_addy);
    bool wasFreed = free_pfn(alloc_bitmap, pfn_to_free);
    return wasFreed;
}

/**
 * @param memory_size Size of RAM 
 */
void init_pmm(unsigned long memory_size){

    pmm_bitmap_start_addy = round_up_to_nearest_page(VIRT_KERNEL_END);
    bitmap_num_bits = memory_size/PAGE_SIZE;
    unsigned int pmm_bitmap_byte_size = bitmap_num_bits/8;
    pmm_bitmap_end_addy = round_up_to_nearest_page(pmm_bitmap_start_addy + pmm_bitmap_byte_size);
    alloc_bitmap = (void *) pmm_bitmap_start_addy;

    //direct map the allocation bitmap so we can start to access it
    unsigned int pages_req_for_bitmap = (pmm_bitmap_end_addy - pmm_bitmap_start_addy)/PAGE_SIZE;
    for(size_t idx=0; idx<pages_req_for_bitmap; idx++){
        //we aren't allocating this as a dynamic alloc area, because the vmm relies on the pmm existing, so there's no point in keeping track of it in the vmm; the vmm's starting address is simply made to be after the end of the bitmap. However, since the mapping API uses the flags I define for a general, arch-independent paging (truly an address space) entry, we use them here
        unsigned long curr_page = pmm_bitmap_start_addy + (idx*PAGE_SIZE);
        map_pageframe(&kernel_PGD,phys_addy(curr_page),curr_page,VM_AREA_WRITE);
    }

    //mark all pages all allocated
    memset(alloc_bitmap, 0xff, pmm_bitmap_byte_size);

    //free pages found to be in usable RAM; only free pages above 1 MiB; the lower 1 MiB will remain not used so that MMIO devs can accesses it if needed, and the heap pages will be allocated before MMIO devices are registered, so we want to make sure those pages aren't allocated to the heap
    size_t pfn_for_1MiB = 0x100000/PAGE_SIZE;
    for(size_t pfn=pfn_for_1MiB; pfn<bitmap_num_bits; pfn++){
        unsigned long check_address = pfn_to_phys_addy(pfn);
        bool usable = is_page_usable(check_address);
        if(usable){
            free_pfn(alloc_bitmap, pfn);
        }
    }

    //alloc the necessary pages for this bitmap
    void * bitmap_phys_address = alloc_requested_pageframe(phys_addy(pmm_bitmap_start_addy),pmm_bitmap_byte_size);
    if(!bitmap_phys_address){
        //TODO: printk error here
    }
    return;
}