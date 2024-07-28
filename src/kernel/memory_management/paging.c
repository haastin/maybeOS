
#include "paging.h"
#include <stdbool.h>
#include <stddef.h>
#include "vmm.h"
#include "pmm.h"
#include "string.h"

Page_Directory_t kernel_PGD __attribute__((aligned(0x1000)));

//need two page tables since the identity map and mapping the virtual kernel will have two diff page dir indexes
Page_Table_t kernel_identitymap_PT __attribute__((aligned(0x1000)));
Page_Table_t kernel_directmap_PT __attribute__((aligned(0x1000)));


static void alloc_page_table(void * pgd, unsigned int pde_index){

    //call PMM to alloc phys page
    uint32_t phys_address = alloc_pageframe();

    //init the page table to be all 0s
    memset((void *)phys_address, 0, PAGE_SIZE);

    //create and insert PDE entry with this pageframe addy
    Page_Directory_t * pgd_p = (Page_Directory_t *) pgd;

    Page_Directory_Entry_t * pde_p = (Page_Directory_Entry_t *) &pgd_p[pde_index];
    pde_p->pde_val = phys_address;

    #ifdef ARCH_x86
        pde_p->pde_val |= KERNEL_PDE_DEFAULT_FLAGS;
    #else
    #endif
}

static size_t convert_flags_to_arch_flags(size_t kernel_obj_flags, bool isKernelPGD){

    size_t converted_flags;

    #ifdef ARCH_x86
        converted_flags = 0;
        if(kernel_obj_flags & VM_AREA_WRITE){
            converted_flags |= WRITABLE;
        }
        if(kernel_obj_flags & VM_AREA_USER_ACCESSIBLE){
            converted_flags |= USER_ACCESSIBLE;
        }
        converted_flags |= PRESENT;
 
        //kernel pages are global so they don't get flushed from TLB across context switches
        if(isKernelPGD){
            converted_flags |= PTE_IS_GLOBAL;
        }
    #else

    #endif

    return converted_flags;
}

bool map_pageframe(void * pgd, void * pageframe_phys_addy, void * virtual_addy, size_t flags){

    Page_Directory_t * pgd_p = (Page_Directory_t *) pgd;
    
    //if this fails there is a bug in the system 
    if(pgd_p){ 
        
        unsigned int pde_idx = get_PDE_idx((uintptr_t)virtual_addy); 
        Page_Directory_Entry_t * pde_p = (Page_Directory_Entry_t *) &pgd_p[pde_idx];
        
        if(!paging_entry_present(pde_p->pde_val)){
            alloc_page_table(pgd, pde_idx);
        }

        //now that we've checked the PDE is valid, we can treat the page frame address as a page table
        Page_Table_t * pt_p = (Page_Table_t *) get_pageframe_address(pde_p->pde_val);
        
        unsigned int pte_idx = get_PTE_idx((uintptr_t)virtual_addy);  
        Page_Table_Entry_t * pte_p = (Page_Table_Entry_t *) &pt_p[pte_idx];
        
        //at this point pte_p->pte_val should be 0, indicating the PTE is not present; since this is expected, we don't check that with an if statement here

        pte_p->pte_val = (uint32_t) pageframe_phys_addy;

        //to determine whether or not we want to make the new PTE global
        bool isKernelPGD = (pgd == ((void*)&kernel_PGD));

        size_t flags_converted = convert_flags_to_arch_flags(flags, isKernelPGD);
        pte_p->pte_val |= flags_converted;

    }
    else{
        //TODO: printk here to dump info to fix bug
        return false;
    }

    return true;

}





