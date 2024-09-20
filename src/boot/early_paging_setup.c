#include "paging.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "early_paging_setup.h"
#include "utils.h"
#include "mem_map.h"
#include "string.h"

static void * early_memset(void* buff, int c, size_t n){
    unsigned char *buff_ptr = (unsigned char*) buff;
    for(size_t i=0; i<n; i++){
        buff_ptr[i] = (unsigned char) c;
    }
    return buff;
}

static Page_Directory_Entry_t before_paging_create_pde(unsigned long aligned_physical_addy){
    
    Page_Directory_Entry_t pde;

    //because a page table only records aligned addresses, and they reside in the upper 20 bits, then we can just directly copy those bits into our pde/pte because the lower 12 bits, which is where the flags/configs are will just be set to 0, which fits our expectation
    pde.pde_val = aligned_physical_addy;
    pde.pde_val |= KERNEL_PDE_DEFAULT_FLAGS;

    return pde;
}

static Page_Table_Entry_t before_paging_create_pte(unsigned long aligned_physical_addy){
    
    Page_Table_Entry_t pte;

    //because a page table only records aligned addresses, and they reside in the upper 20 bits, then we can just directly copy those bits into our pde/pte because the lower 12 bits, which is where the flags/configs are will just be set to 0, which fits our expectation
    pte.pte_val = aligned_physical_addy;
    pte.pte_val |= KERNEL_PTE_DEFAULT_FLAGS;
   
    return pte;
}

/**
 * * This function will be linked at the physical starting address of the kernel, not virtual, so that we can call it before enabling paging
 */
void before_paging_init_page_tables(unsigned long kernel_starting_phys_addy, unsigned long kernel_starting_virt_addy){

    Page_Directory_t * phys_kern_pagedir = (Page_Directory_t *) phys_addy(&kernel_PGD);
    Page_Table_t * zero_to_four_MiB_PT_phys_addy = (Page_Table_t *) phys_addy(&zero_to_four_MiB_PT);
    Page_Table_t * four_to_eight_MiB_PT_phys_addy = (Page_Table_t *) phys_addy(&four_to_eight_MiB_PT);

    //init entries of page dir and page tables
    early_memset(phys_kern_pagedir, 0, sizeof(Page_Directory_t));
    early_memset(zero_to_four_MiB_PT_phys_addy, 0, sizeof(Page_Table_t));
    early_memset(four_to_eight_MiB_PT_phys_addy, 0, sizeof(Page_Table_t));

    //these two for loops map all page frames [0x000000-0x7ff000]

    //page tab 1 - page frames: [0x000000 - 0x3ff000]
    for(size_t page=0; page < NUM_PAGING_STRUCTURE_ENTRIES_IN_32_BIT_MODE; page++){
        
        uint32_t curr_aligned_phys_addy_being_mapped = 0 + page*PAGE_SIZE;

        zero_to_four_MiB_PT_phys_addy->page_frames[page] = before_paging_create_pte(curr_aligned_phys_addy_being_mapped);
        
    }
    
    //page tab 2 - page frames: [0x400000 - 0x7ff000]
    for(size_t page=0; page < NUM_PAGING_STRUCTURE_ENTRIES_IN_32_BIT_MODE; page++){
        
        //starts at 0x400000
        uint32_t curr_aligned_phys_addy_being_mapped = PAGE_SIZE*NUM_PAGING_STRUCTURE_ENTRIES_IN_32_BIT_MODE + page*PAGE_SIZE;

        four_to_eight_MiB_PT_phys_addy->page_frames[page] = before_paging_create_pte(curr_aligned_phys_addy_being_mapped);
        
    }

    //create the page directory entries for identity map and direct map

    //identity map
    unsigned long identity_map_start = 0x0;
    unsigned long second_identity_map_page_start = identity_map_start+  PAGE_SIZE*NUM_PAGING_STRUCTURE_ENTRIES_IN_32_BIT_MODE;

    phys_kern_pagedir->page_tables[get_PDE_idx(identity_map_start)] = before_paging_create_pde((unsigned long)zero_to_four_MiB_PT_phys_addy);

    phys_kern_pagedir->page_tables[get_PDE_idx(second_identity_map_page_start)] = before_paging_create_pde((unsigned long)four_to_eight_MiB_PT_phys_addy);

    //direct map
    unsigned long virtual_map_start = VIRTUAL_KERNEL_OFFSET;
    unsigned long second_virtual_map_page_start = virtual_map_start + PAGE_SIZE*NUM_PAGING_STRUCTURE_ENTRIES_IN_32_BIT_MODE;
    
    phys_kern_pagedir->page_tables[get_PDE_idx(virtual_map_start)] = before_paging_create_pde((unsigned long)zero_to_four_MiB_PT_phys_addy);

    phys_kern_pagedir->page_tables[get_PDE_idx(second_virtual_map_page_start)] = before_paging_create_pde((unsigned long)four_to_eight_MiB_PT_phys_addy);
    
    //* while all other dynamically created page tables must be accessed through their recursive entries, the statically allocated page tables do not need to be since they are direct mapped as part of the kernel

    //create the recursive PGD entry so that PTs can be accessed through it
    phys_kern_pagedir->page_tables[RECURSIVE_PDE_IDX] = before_paging_create_pde((unsigned long)phys_kern_pagedir);

    //TODO: check to see if the kernel image extends past the mapped mem, if so throw an exception
}