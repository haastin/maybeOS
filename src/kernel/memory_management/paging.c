
#include "paging.h"
#include <stdbool.h>
#include <stddef.h>
#include "vmm.h"
#include "pmm.h"
#include "string.h"
#include "utils.h"

Page_Directory_t kernel_PGD __attribute__((aligned(0x1000)));

//need two page tables since the identity map and mapping the virtual kernel will have two diff page dir indexes
Page_Table_t kernel_identitymap_PT __attribute__((aligned(0x1000)));
Page_Table_t kernel_directmap_PT __attribute__((aligned(0x1000)));

Page_Table_Entry_t * get_pte(void * pgd, unsigned long virtual_address){

    Page_Directory_t * pgd_p = (Page_Directory_t *) pgd;

    unsigned int pde_idx = get_PDE_idx((uintptr_t)virtual_address); 
    Page_Directory_Entry_t * pde_p = (Page_Directory_Entry_t *) &pgd_p->page_tables[pde_idx];
    if(!pde_p){
        //TODO: printk error- page tab doesnt exist
        return false;
    }

    //doesnt matter if pte is already or not, we will zero it anyway

    Page_Table_t * pt_p = (Page_Table_t *) get_page_tab_virtual_pointer(pde_idx);

    unsigned int pte_idx = get_PTE_idx((uintptr_t)virtual_address);  
    Page_Table_Entry_t * pte_p = (Page_Table_Entry_t *) &pt_p->page_frames[pte_idx];

    return pte_p;
}

bool free_mapping(void* pgd, unsigned long virtual_address){

    Page_Table_Entry_t * pte_p = get_pte(pgd, virtual_address);
    pte_p->pte_val = 0;
    return true;
}

static inline void init_pde(Page_Directory_Entry_t * pde_p, unsigned long physical_address, size_t flags){
    pde_p->pde_val = physical_address;
    pde_p->pde_val |= flags;
}

void create_recursive_mapping(void *pgd, void * phys_address){

    bool mapped = map_pageframe(pgd, phys_address, NEW_PAGE_TAB_TEMP_VIRT_POINTER, VM_AREA_WRITE);
    if(!mapped){
        //TODO: printk error- temp mapping for new page tab failed
        return;
    }

    Page_Table_t * pt_p = (Page_Table_t *) NEW_PAGE_TAB_TEMP_VIRT_POINTER;
    pt_p->page_frames[PAGE_TAB_RECURSIVE_ENTRY_INDEX].pte_val = phys_address;
    pt_p->page_frames[PAGE_TAB_RECURSIVE_ENTRY_INDEX].pte_val |= KERNEL_PTE_DEFAULT_FLAGS;
    
    free_mapping(pgd, pt_p);

    //TODO: was having probs flushign TLB wihtout the full flush, but need to get that up
    asm volatile("mov %cr3, %eax\n"
                 "mov %eax, %cr3\n");
    
    //unsigned int pde_idx = get_PDE_idx(pt_p);
    //unsigned long pt_holding_const_temp_virt_pointer = get_page_tab_virtual_pointer(pde_idx);
    
    //need to invalidate the tlb cached addresses when changing a mapping
    //asm volatile ("invlpg (%0)\n" : : "r" (pt_p->page_frames[PAGE_TAB_RECURSIVE_ENTRY_INDEX]) :"memory" );
}

static void alloc_page_table(void * pgd, unsigned int pde_index){

    //call PMM to alloc phys page
    void * phys_address = alloc_pageframe(NUM_PAGING_STRUCTURE_ENTRIES_IN_32_BIT_MODE*BYTES_PER_PAGING_ENTRY);

    if(!phys_address){
        //TODO: printk error- phys alloc for new page tab failed
    }
    //init the page table to be all 0s
    memset(phys_address, 0, PAGE_SIZE);


    Page_Directory_t * pgd_p = (Page_Directory_t *) pgd;
    Page_Directory_Entry_t * pde_p = (Page_Directory_Entry_t *) &pgd_p->page_tables[pde_index];
    init_pde(pde_p, phys_address, KERNEL_PDE_DEFAULT_FLAGS);

    //make a recursive mapping in this newly created page table
    create_recursive_mapping(&kernel_PGD, phys_address);
}

static size_t convert_flags_to_arch_flags(size_t kernel_obj_flags){

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
        converted_flags |= PTE_IS_GLOBAL;
        
    #else

    #endif

    return converted_flags;
}

bool map_pageframe(void * pgd, void * pageframe_phys_addy, void * virtual_addy, size_t flags){

    Page_Directory_t * pgd_p = (Page_Directory_t *) pgd;
    
    //if this fails there is a bug in the system 
    if(pgd_p){ 
        
        //before trying to access the page table, make sure the pde is actually valid
        unsigned int pde_idx = get_PDE_idx((uintptr_t)virtual_addy); 
        Page_Directory_Entry_t * pde_p = (Page_Directory_Entry_t *) &pgd_p->page_tables[pde_idx];
        if(!paging_entry_present(pde_p->pde_val)){
            alloc_page_table(pgd, pde_idx);
        }
        
        //now we can get the pte
        Page_Table_Entry_t * pte_p = get_pte(pgd, virtual_addy);
        
        //at this point pte_p->pte_val should be 0, indicating the PTE is not present; since this is expected, we don't check that with an if statement here

        pte_p->pte_val = (uint32_t) pageframe_phys_addy;

        //to determine whether or not we want to make the new PTE global
        // bool isKernelPGD = (pgd == ((void*)&kernel_PGD));

        size_t flags_converted = convert_flags_to_arch_flags(flags);
        pte_p->pte_val |= flags_converted;

    }
    else{
        //TODO: printk here to dump info to fix bug
        return false;
    }

    return true;

}

bool map_contiguous_pages(void * pgd, unsigned long physical_address_start, unsigned long virtual_address_start, size_t flags, size_t num_pages){

    for(size_t page=0; page<num_pages; page++){
        bool mapped = map_pageframe(pgd, physical_address_start + page*PAGE_SIZE, virtual_address_start + page*PAGE_SIZE, flags);
        if(!mapped){
            //TODO: printk error 
            return false;
        }
    }
    return true;
}

bool identity_map_pageframes(void * pgd, unsigned long physical_address_start, size_t flags, size_t num_pages){

    for(size_t page=0; page<num_pages; page++){
        bool mapped = map_pageframe(pgd, physical_address_start + page*PAGE_SIZE, physical_address_start + page*PAGE_SIZE, flags);
        if(!mapped){
            //TODO: printk error 
            return false;
        }
    }
    return true;
}

bool direct_map_pageframes(void * pgd, unsigned long physical_address_start, size_t flags, size_t num_pages){
    
    for(size_t page=0; page<num_pages; page++){
        bool mapped = map_pageframe(pgd, physical_address_start + page*PAGE_SIZE, virt_addy(physical_address_start) + page*PAGE_SIZE, flags);
        if(!mapped){
            //TODO: printk error 
            return false;
        }
    }
    return true;
}


