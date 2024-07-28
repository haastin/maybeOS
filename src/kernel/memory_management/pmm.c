
#include <stdint.h>
#include "paging.h"
#include "mem_map.h"
#include <stdbool.h>
#include "pmm.h"
#include "utils.h"
#include "string.h"

#define NUM_BITS_IN_ONE_BITMAP sizeof(unsigned int)*8

//defines the bounds of the bitmap we will use for dynamic memory alloc- this is technically the first "allocation" of the system since it dynamically sets the bounds of the bitmap depending on how much memory is detected by the bootloader. this memory will be marked as used in the bitmap after it is initialized
unsigned long pmm_bitmap_start_addy;
unsigned long pmm_bitmap_end_addy;

void * alloc_bitmap;

/**
 * @param memory_size Size of RAM 
 */
void init_pmm(unsigned long memory_size){

    pmm_bitmap_start_addy = round_up_to_nearest_page(VIRT_KERNEL_END);
    unsigned int num_bits_needed = memory_size/PAGE_SIZE;
    unsigned int pmm_bitmap_byte_size = num_bits_needed/8;
    pmm_bitmap_end_addy = pmm_bitmap_start_addy + pmm_bitmap_byte_size;
    alloc_bitmap = (void *) pmm_bitmap_start_addy;
    //mark all pages all allocated
    memset(alloc_bitmap, 0xff, pmm_bitmap_byte_size);
    //free pages found to be in usable RAM

    //alloc the necessary pages for this bitmap
}

static unsigned int find_first_avail_pageframe(void){

    unsigned int * bitmap = (unsigned int *)pmm_bitmap_start_addy;

    //keeps track of how many bits we've checked so far 
    unsigned int bit_counter = 0;
    
    while(bitmap <= (unsigned int *)pmm_bitmap_end_addy){
        //gcc builtin that finds set bit w/ least index in an int
        unsigned int res = (unsigned int) __builtin_ffs((int) *bitmap);
        if(res){
            //return a pfn 
            return (bit_counter + res);
        }
        else{
            //advance to the next bitmap
            bitmap++;
            //increase the bit counter by num of bits in an unsigned int
            bit_counter += NUM_BITS_IN_ONE_BITMAP;
        }
    }

}

static unsigned int * get_pfn_bitmap(unsigned int pfn){
    //this is the num of unsigned ints from the start of the bitmap to the unsinged long the requested pfn resides in 
    unsigned int requested_pfn_bitmap_offset = pfn / NUM_BITS_IN_ONE_BITMAP;
    unsigned int * bitmap = (unsigned int *)pmm_bitmap_start_addy;
    bitmap += requested_pfn_bitmap_offset;
    return bitmap;
}

static unsigned char get_pfn_bit(unsigned int pfn){
    unsigned int * pfn_bitmap = get_pfn_bitmap(pfn);
    unsigned int pfn_bit_index = pfn % NUM_BITS_IN_ONE_BITMAP;
    unsigned char pfn_bit = ((*pfn_bitmap) >> pfn_bit_index) & 0x1;
    return pfn_bit;
}


static bool set_pfn_bit(unsigned int pfn){
    unsigned char pfn_bit = get_pfn_bit(pfn);
    if(pfn_bit == 1){
        //the page is already allocated
        return false;
    }
    else{
        unsigned int * pfn_bitmap = get_pfn_bitmap(pfn);
        unsigned int pfn_bit_index = pfn % NUM_BITS_IN_ONE_BITMAP;
        *pfn_bitmap |= (1 << pfn_bit_index); 
    }
    return true;
}

static bool free_pfn_bit(unsigned int pfn){
    unsigned char pfn_bit = get_pfn_bit(pfn);
    if(pfn_bit == 0){
        //the page is already free
        return false;
    }
    else{
        unsigned int * pfn_bitmap = get_pfn_bitmap(pfn);
        unsigned int pfn_bit_index = pfn % NUM_BITS_IN_ONE_BITMAP;
        *pfn_bitmap &= ~(1 << pfn_bit_index);
    }
    return true;
}

void * alloc_pageframe(void){
    unsigned int first_avail_pfn = find_first_avail_pageframe();
    return pfn_to_phys_addy(first_avail_pfn);
}

bool alloc_requested_pageframe(unsigned long physical_address){
    unsigned int requested_pfn = (unsigned int) phys_addy_to_pfn(physical_address);
    bool res = set_pfn_bit(requested_pfn);
    return res;
}

bool free_pageframe(unsigned long pageframe_addy){
    unsigned int pfn_to_free = phys_addy_to_pfn(pageframe_addy);
    bool res = free_pfn_bit(pfn_to_free);
    return res;
}