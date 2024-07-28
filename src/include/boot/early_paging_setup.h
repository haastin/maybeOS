#ifdef __EARLY_PAGING_SETUP_H__
#define __EARLY_PAGING_SETUP_H__

#include "paging.h"

void before_paging_init_page_tables(unsigned long kernel_starting_phys_addy, unsigned long kernel_starting_virt_addy);


#endif /*__EARLY_PAGING_SETUP_H__*/