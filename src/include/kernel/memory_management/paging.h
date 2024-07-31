#ifndef __PAGING_H__
#define __PAGING_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef ARCH_x86

/* x86 PTE/PDE Fields Bit Indices */
#define PRESENT_BIT_INDEX 0
#define READ_WRITE_BIT_INDEX 1
#define USER_ACCESS_BIT_INDEX 2
#define WRITE_THROUGH_BIT_INDEX 3
#define CACHEABLE_BIT_INDEX 4
#define SOFTWARE_ACCESS_BIT_INDEX 5

/* PTE-Specific Fields Bit Indices */
#define PTE_SOFTWARE_WRITTEN_BIT_INDEX 6
#define PTE_PAT_INDEX_BIT_INDEX 7
#define IS_GLOBAL_PAGE_BIT_INDEX 8

/* PDE-Specific Fields Bit Indices */
#define PAGESIZE_BIT_INDEX 7


/* PTE/PDE Fields Bit Values (NOT indices) */

#define PRESENT (1 << (PRESENT_BIT_INDEX))
#define NOT_PRESENT (0 << (PRESENT_BIT_INDEX))

#define WRITABLE (1 << (READ_WRITE_BIT_INDEX))
#define NOT_WRITABLE (0 << (READ_WRITE_BIT_INDEX))

#define USER_ACCESSIBLE (1 << (USER_ACCESS_BIT_INDEX))
#define NOT_USER_ACCESSIBLE (0 << (USER_ACCESS_BIT_INDEX))

#define WRITE_THROUGH (1 << (WRITE_THROUGH_BIT_INDEX))
#define DONT_WRITE_THROUGH (0 << (WRITE_THROUGH_BIT_INDEX))

#define CACHEABLE (1 << (CACHEABLE_BIT_INDEX))
#define DONT_CACHE (0 << (CACHEABLE_BIT_INDEX))

#define SOFTWARE_ACCESSED (1 << (SOFTWARE_ACCESS_BIT_INDEX))
#define SOFTWARE_HASNT_ACCESSED (0 << (SOFTWARE_ACCESS_BIT_INDEX))

/* PTE-Specific Fields Bit Values (NOT indices) */

#define PTE_SOFTWARE_WRITTEN (1 << (PTE_SOFTWARE_WRITTEN_BIT_INDEX))
#define PTE_SOFTWARE_HASNT_WRITTEN (0 << (PTE_SOFTWARE_WRITTEN_BIT_INDEX))

#define PTE_PAT_INDEX (1 << (PTE_PAT_INDEX_BIT_INDEX))
#define PTE_NO_PAT (0 << (PTE_PAT_INDEX_BIT_INDEX))

#define PTE_IS_GLOBAL (1 << (IS_GLOBAL_PAGE_BIT_INDEX))
#define PTE_NOT_GLOBAL (0 << (IS_GLOBAL_PAGE_BIT_INDEX))

/* PDE-Specific Fields Bit Values (NOT indices) */

// the only PDE-Specific Field is the page size, which is defined below as SET_PDE_PAGESIZE_FLAG


//Pagesize-specific Macros

#ifdef PAGE_SIZE_4K 
//4 KiB page size

#define PAGE_SIZE 4096

#define SET_PDE_PAGESIZE_FLAG (1 << (PAGESIZE_BIT_INDEX))

//page tables on exist for 4k Pages in i386 no PAE
#define PAGE_TAB_INDEX_BIT_INDEX 12

#else 
// 4 MiB page size

#define PAGE_SIZE (4 * 1024 * 1024)

//this field is in the next byte of the entry; the one above are all in the same byte
#define PAGE_FRAME_ADDY_LOWER_BITS_BIT_INDEX 12
#define PAGE_FRAME_ADDY_HIGH_BITS_BIT_INDEX 16

#define SET_PDE_PAGESIZE_FLAG (0 << (PAGESIZE_BIT_INDEX))

#endif

#define PAGE_FRAME_BITMASK ~(PAGE_SIZE-1)

//x86 Uninitialized PDE/PTE Values
#define PTE_UNINITALIZED_VAL 0
#define PDE_UNINITALIZED_VAL 0

//x86 PDE/PTE Default Flags
#define KERNEL_PDE_DEFAULT_FLAGS (PRESENT | WRITABLE | NOT_USER_ACCESSIBLE | SET_PDE_PAGESIZE_FLAG)
#define KERNEL_PTE_DEFAULT_FLAGS (PRESENT | WRITABLE | NOT_USER_ACCESSIBLE | PTE_IS_GLOBAL)

/* Misc x86 Macros*/

#define PAGE_DIR_INDEX_BIT_INDEX 22

#define NUM_PAGING_STRUCTURE_ENTRIES_IN_32_BIT_MODE 1024
#define BYTES_PER_PAGING_ENTRY 4

//recursion is used to get the address of any paging structure for this 32 bit OS to avoid the expense of direct mapping all of phys mem given the tight virtual address space. while technically the kernel's is statically allocated, processes' page dirs won't be, and while their pgd virt addy will liekly be recorded in a data structure, it is safe to map it anyways

//these virtual addresses used are chosen to avoid interfering with hardware or kernel mappings that require particular virtual addresses 
#define PAGE_DIR_RECURSIVE_VIRT_ADDRESS 0xf0fc3000 //this comes out to entry index 963 of the page tab

//getting a page tab phys address requires accessing the page table firstly, but we don't know what page tab will be referenced, so cannot have hardcoded virtual address for this. the page dir index for the page tab must be found and then ORed with the chosen index to hold a page table's recursive entry, which will also be located at index 963
#define PAGE_TAB_RECURSIVE_ENTRY_INDEX 0x3c3

//when creating page tables, an initial insertion needs to be made for the recursive mapping. after this, a virtual pointer is no longer needed to reference a page table, so when creating a page table we temporarily point the allocated pageframe for it with this virtual address to insert the recursive entry. This virtual address was chosen because it resides in the higher half of kernel space, but before the loaded image of my kernel, where there is a gap. This ensures it will not be in the virtual address range vmalloc keeps track of, so no dynamic alloc struct is needed to be allocated to keep track of it.  
#define NEW_PAGE_TAB_TEMP_VIRT_POINTER 0xC0000000

//10 bits to determine PT index; use this after shifting the virtual addy to get to the PT index
#define PAGE_TAB_INDEX_BITMASK 0x3ff

#define get_PTE_idx(virtual_addy) (((uintptr_t)(virtual_addy) >> PAGE_TAB_INDEX_BIT_INDEX) & PAGE_TAB_INDEX_BITMASK)
#define get_PDE_idx(virtual_addy) ((uintptr_t)(virtual_addy) >> PAGE_DIR_INDEX_BIT_INDEX)

#define paging_entry_present(paging_entry_val) ((paging_entry_val) & 0x1)
#define get_pageframe_address(paging_entry_val) ((paging_entry_val) & PAGE_FRAME_BITMASK)

#define get_page_tab_virtual_pointer(pde_idx) (((pde_idx) << PAGE_DIR_INDEX_BIT_INDEX) | (PAGE_TAB_RECURSIVE_ENTRY_INDEX << PAGE_TAB_INDEX_BIT_INDEX))


//* END OF x86-Specific DEFINITIONS
#else

// * Other archiecture paging formatting info can be included with an #elseif after the x86 section

#endif


#define pfn_to_phys_addy(pfn) ((pfn)*PAGE_SIZE)
#define phys_addy_to_pfn(phys_addy) ((phys_addy)/PAGE_SIZE)

#define num_pages_needed(size) (((size) + (PAGE_SIZE-1))/PAGE_SIZE)


typedef struct {
    uint32_t pte_val;
} Page_Table_Entry_t;

typedef struct {
    Page_Table_Entry_t page_frames[NUM_PAGING_STRUCTURE_ENTRIES_IN_32_BIT_MODE];
} Page_Table_t;

typedef struct {
    uint32_t pde_val;
} Page_Directory_Entry_t;

typedef struct {
    Page_Directory_Entry_t page_tables[NUM_PAGING_STRUCTURE_ENTRIES_IN_32_BIT_MODE];
} Page_Directory_t;

typedef struct {
    /*I'm not very familiar with what other OS besides Linux use pages for, but Linux seems to use them mainly for caching various info for file-backed memory, swapping pages out, etc. I'll create a page struct and organize it like how Linux 2.4 does for now, and then later on will add to it as I expand on features*/
} Page_t;


//the PGD for maybeOS kernel
extern Page_Directory_t kernel_PGD __attribute__((aligned(0x1000)));

//need two boot page tables since the identity map and mapping the virtual kernel will have two diff page dir indexes
extern Page_Table_t kernel_identitymap_PT __attribute__((aligned(0x1000)));
extern Page_Table_t kernel_directmap_PT __attribute__((aligned(0x1000)));

Page_Table_Entry_t *get_pte(void *pgd, unsigned long virtual_address);

bool free_mapping(void *pgd, unsigned long virtual_address);

void create_recursive_mapping(void *pgd, void *phys_address);

bool map_pageframe(void * pgd, void * pageframe_phys_addy, void * virtual_addy, size_t flags);

bool map_contiguous_pages(void *pgd, unsigned long physical_address_start, unsigned long virtual_address_start, size_t flags, size_t num_pages);

bool identity_map_pageframes(void * pgd, unsigned long physical_address_start, size_t flags, size_t num_pages);

bool direct_map_pageframes(void *pgd, unsigned long physical_address_start, size_t flags, size_t num_pages);

#endif /*__PAGING_H__*/
