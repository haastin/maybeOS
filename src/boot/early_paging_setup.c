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

static void create_paging_entry(unsigned long aligned_phys_addy, unsigned long virt_addy){

    Page_Directory_t * phys_kern_pagedir = (Page_Directory_t *) phys_addy(&kernel_PGD);
    Page_Table_t * phys_kern_pagtab;

    //choose the page table declared in the static image based on if we are identity mapping or mapping the virtual kernel (they will have differnt page directory indices so we need two page tables)
    if (aligned_phys_addy == virt_addy){
        phys_kern_pagtab = (Page_Table_t *) phys_addy(&kernel_identitymap_PT);
    }
    else{
        phys_kern_pagtab = (Page_Table_t *) phys_addy(&kernel_directmap_PT);
    }

    unsigned short page_dir_index = get_PDE_idx(virt_addy);

    #ifdef PAGE_SIZE_4K

    //initialize PDE for this virtual address if necessary
    bool dirEntryIsPresent = phys_kern_pagedir->page_tables[page_dir_index].pde_val & 0x1;
    if(!dirEntryIsPresent){
        phys_kern_pagedir->page_tables[page_dir_index] = before_paging_create_pde((unsigned long)phys_kern_pagtab);
    }

    //initialize PTE for this virtual address
    unsigned short page_tab_index = get_PTE_idx(virt_addy);
    phys_kern_pagtab->page_frames[page_tab_index] = before_paging_create_pte(aligned_phys_addy);

    #else
    //for 4 MiB pages, the PDE is just the page frame addy of the page and its flags, no page table
    phys_kern_pagedir->page_tables[page_dir_index] = before_paging_create_pde(aligned_phys_addy);
    #endif
}

/**
 * * This function will be linked at the physical starting address of the kernel, not virtual, so that we can call it before enabling paging
 */
void before_paging_init_page_tables(unsigned long kernel_starting_phys_addy, unsigned long kernel_starting_virt_addy){

    Page_Directory_t * phys_kern_pagedir = (Page_Directory_t *) phys_addy(&kernel_PGD);
    Page_Table_t * phys_kern_identity_pagetab = (Page_Table_t *) phys_addy(&kernel_identitymap_PT);
    Page_Table_t * phys_kern_virtual_pagetab = (Page_Table_t *) phys_addy(&kernel_directmap_PT);

    //init entries of page dir and page tables
    early_memset(phys_kern_pagedir, 0, sizeof(Page_Directory_t));
    early_memset(phys_kern_identity_pagetab, 0, sizeof(Page_Table_t));
    early_memset(phys_kern_virtual_pagetab, 0, sizeof(Page_Table_t));

    //decided to identity map everything before the kernel too since there's boot info there I need
    unsigned int num_pages_before_kernel = get_page_above_pfn(kernel_starting_phys_addy); 

    for(size_t page=0; page < num_pages_before_kernel; page++){
        
        uint32_t curr_aligned_phys_addy_being_mapped = 0 + page*PAGE_SIZE;
        
        //this is the same as above because we are identity mapping the kernel
        uint32_t curr_aligned_virt_addy_being_mapped = curr_aligned_phys_addy_being_mapped;
        
        create_paging_entry(curr_aligned_phys_addy_being_mapped, curr_aligned_virt_addy_being_mapped);
    }
    
   
    //round up to the nearest multiple of pages
    unsigned int kernel_size = (unsigned int)&_kernel_size;
    unsigned int kernel_size_in_pages = get_page_above_pfn(kernel_size);
    
    //this identity maps the kernel pages
    for(size_t page=0;page<kernel_size_in_pages;page++){

        uint32_t curr_aligned_phys_addy_being_mapped = kernel_starting_phys_addy + page*PAGE_SIZE;
        
        //this is the same as above because we are identity mapping the kernel
        uint32_t curr_aligned_virt_addy_being_mapped = curr_aligned_phys_addy_being_mapped;
        
        create_paging_entry(curr_aligned_phys_addy_being_mapped, curr_aligned_virt_addy_being_mapped);

    }

    //this maps the part of our kernel linked at the specified virtual address
    for(size_t page=0;page<kernel_size_in_pages;page++){

        uint32_t curr_aligned_phys_addy_being_mapped = kernel_starting_phys_addy + page*PAGE_SIZE;
        
        uint32_t curr_aligned_virt_addy_being_mapped = kernel_starting_virt_addy + page*PAGE_SIZE;
        
        create_paging_entry(curr_aligned_phys_addy_being_mapped, curr_aligned_virt_addy_being_mapped);

    }
    
    //because the recursive entry is at index 936 in the paging structures, nothing should be using the existing kernel or HW mappings in this table, so it's likely ok to overwrite them. need to do this here because going forward page tables only have their address only referred to with a recursive mapping, which these don't initially have.
    
    phys_kern_identity_pagetab->page_frames[PAGE_TAB_RECURSIVE_ENTRY_INDEX] = before_paging_create_pte(phys_addy(&kernel_identitymap_PT));

    phys_kern_virtual_pagetab->page_frames[PAGE_TAB_RECURSIVE_ENTRY_INDEX] = before_paging_create_pte(phys_addy(&kernel_directmap_PT));
}