#include "paging.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "early_paging_setup.h"
#include "utils.h"

extern unsigned int _kernel_size, _kernel_phys_size, _kernel_virt_size, _virt_kernel_start, _virt_kernel_end;

Page_Directory_t kernel_init_page_dir __attribute__((aligned(0x1000)));

//need two page tables since the identity map and mapping the virtual kernel will have two diff page dir indexes
Page_Table_t kernel_init_page_tab_identity __attribute__((aligned(0x1000)));
Page_Table_t kernel_init_page_tab_virtual __attribute__((aligned(0x1000)));

Page_Directory_Entry_t before_paging_init_pde_entry(uint32_t aligned_physical_addy){
    
    Page_Directory_Entry_t pde;

    //because a page table only records aligned addresses, and they reside in the upper 20 bits, then we can just directly copy those bits into our pde/pte because the lower 12 bits, which is where the flags/configs are will just be set to 0, which fits our expectation
    uint32_t * pde_pointer = (uint32_t *) &pde;
    *pde_pointer = aligned_physical_addy;

    if(aligned_physical_addy == NULL){
        //used for when when we init a page directory
        pde.flags = NOT_PRESENT;
    }
    else{
        pde.flags = PRESENT | WRITABLE | NOT_USER_ACCESSIBLE | CACHEABLE | WRITE_THROUGH | SOFTWARE_HASNT_ACCESSED | PDE_IGNORED;
    }

    #ifdef PAGE_SIZE_4K
        pde.flags |= PDE_ENTRY_PAGE_TAB;
    #else
        pde.flags |= PDE_ENTRY_4MB_PAGE;
    #endif

    return pde;
}

Page_Table_Entry_t before_paging_init_pte_entry(uint32_t aligned_physical_addy){
    
    Page_Table_Entry_t pte;

    //because a page table only records aligned addresses, and they reside in the upper 20 bits, then we can just directly copy those bits into our pde/pte because the lower 12 bits, which is where the flags/configs are will just be set to 0, which fits our expectation
    uint32_t * pte_pointer = (uint32_t *) &pte;
    *pte_pointer = aligned_physical_addy;

    if(aligned_physical_addy == NULL){
        //used for when when we init a page table
        pte.flags = NOT_PRESENT;
    }
    else{
        pte.flags = PRESENT | WRITABLE | NOT_USER_ACCESSIBLE | CACHEABLE | WRITE_THROUGH | SOFTWARE_HASNT_ACCESSED | PTE_SOFTWARE_HASNT_WRITTEN | PTE_NO_PAT;
    }

    pte.page_frame_addy_low_and_ignored_field_and_global_flag |= PTE_NOT_GLOBAL;

    return pte;
}

void create_paging_entry_for_phys_address(uint32_t aligned_phys_addy, uint32_t virt_addy){

    Page_Directory_t * phys_kern_pagedir = (Page_Directory_t *) phys_addy(&kernel_init_page_dir);
    Page_Table_t * phys_kern_pagtab;

    //choose the page table declared in the static image based on if we are identity mapping or mapping the virtual kernel (they will have differnt page directory indices so we need two page tables)
    if (aligned_phys_addy == virt_addy){
        phys_kern_pagtab = (Page_Table_t *) phys_addy(&kernel_init_page_tab_identity);
    }
    else{
        phys_kern_pagtab = (Page_Table_t *) phys_addy(&kernel_init_page_tab_virtual);
    }

    //virt addy determines page dir and page tab index
        unsigned short page_dir_index = virt_addy >> (PAGE_DIR_INDEX_BIT_INDEX) & 0x3ff;


        #ifdef PAGE_SIZE_4K
        unsigned short page_tab_index = virt_addy >> (PAGE_TAB_INDEX_BIT_INDEX) & 0x3ff;

        phys_kern_pagtab->page_frames[page_tab_index] = before_paging_init_pte_entry(aligned_phys_addy);

        bool dirEntryIsPresent = (phys_kern_pagedir->page_tables[page_dir_index].flags) & 0x1;
        if(!dirEntryIsPresent){
            phys_kern_pagedir->page_tables[page_dir_index] = before_paging_init_pde_entry(phys_kern_pagtab);
        }
        
        #else
        
        phys_kern_pagedir->page_tables[page_dir_index] = before_paging_init_pde_entry(aligned_phys_addy);
        #endif
}

/**
 * * This function will be linked at the physical starting address of the kernel, not virtual, so that we can call it before enabling paging
 * TODO: make other entries of page dir/tab writable and not present
 */
void before_paging_page_dir_and_table_setup(uint32_t kernel_starting_phys_addy, uint32_t kernel_starting_virt_addy){

    Page_Directory_t * phys_kern_pagedir = (Page_Directory_t *) phys_addy(&kernel_init_page_dir);
    Page_Table_t * phys_kern_identity_pagetab = (Page_Table_t *) phys_addy(&kernel_init_page_tab_identity);
    Page_Table_t * phys_kern_virtual_pagetab = (Page_Table_t *) phys_addy(&kernel_init_page_tab_identity);

    //init entries of page dir and page tables
    for(size_t paging_entry=0; paging_entry<NUM_PAGING_STRUCTURE_ENTRIES_IN_32_BIT_MODE;paging_entry++){
        Page_Directory_Entry_t default_pagedir_entry = before_paging_init_pde_entry(NULL);
        Page_Table_Entry_t default_pagetab_entry = before_paging_init_pte_entry(NULL);

        phys_kern_pagedir->page_tables[paging_entry] = default_pagedir_entry;
        phys_kern_identity_pagetab->page_frames[paging_entry] = default_pagetab_entry;
        phys_kern_virtual_pagetab->page_frames[paging_entry] = default_pagetab_entry;
    }

    //decided to identity map everything before the kernel too since there's boot info there I need
    unsigned int num_pages_before_kernel = (kernel_starting_phys_addy + (PAGE_SIZE-1))/PAGE_SIZE;
    for(size_t page=0; page < num_pages_before_kernel; page++){
        
        uint32_t curr_aligned_phys_addy_being_mapped = 0 + page*PAGE_SIZE;
        
        //this is the same as above because we are identity mapping the kernel
        uint32_t curr_aligned_virt_addy_being_mapped = curr_aligned_phys_addy_being_mapped;
        
        create_paging_entry_for_phys_address(curr_aligned_phys_addy_being_mapped, curr_aligned_virt_addy_being_mapped);
    }
    
   
    //round up to the nearest multiple of pages
    unsigned int kernel_size = (unsigned int)&_kernel_size;
    unsigned int kernel_size_in_pages = (kernel_size + (PAGE_SIZE-1))/PAGE_SIZE;
    
    //this identity maps the kernel pages
    for(size_t page=0;page<kernel_size_in_pages;page++){

        uint32_t curr_aligned_phys_addy_being_mapped = kernel_starting_phys_addy + page*PAGE_SIZE;
        
        //this is the same as above because we are identity mapping the kernel
        uint32_t curr_aligned_virt_addy_being_mapped = curr_aligned_phys_addy_being_mapped;
        
        create_paging_entry_for_phys_address(curr_aligned_phys_addy_being_mapped, curr_aligned_virt_addy_being_mapped);

    }

    //this maps the part of our kernel linked at the specified virtual address
    for(size_t page=0;page<kernel_size_in_pages;page++){

        uint32_t curr_aligned_phys_addy_being_mapped = kernel_starting_phys_addy + page*PAGE_SIZE;
        
        uint32_t curr_aligned_virt_addy_being_mapped = kernel_starting_virt_addy + page*PAGE_SIZE;
        
        create_paging_entry_for_phys_address(curr_aligned_phys_addy_being_mapped, curr_aligned_virt_addy_being_mapped);

    }

}