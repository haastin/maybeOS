#ifdef __EARLY_PAGING_SETUP_H__
#define __EARLY_PAGING_SETUP_H__

#include "paging.h"
extern Page_Directory_t kernel_init_page_dir;

Page_Directory_Entry_t before_paging_init_pde_entry(uint32_t aligned_physical_addy);
Page_Table_Entry_t before_paging_init_pte_entry(uint32_t aligned_physical_addy);
void create_paging_entry_for_phys_address(uint32_t aligned_phys_addy, uint32_t virt_addy);
void before_paging_page_dir_and_table_setup(uint32_t kernel_starting_phys_addy, uint32_t kernel_starting_virt_addy);


#endif /*__EARLY_PAGING_SETUP_H__*/