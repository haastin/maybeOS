#include <stdint.h>
#define NUM_PAGING_STRUCTURE_ENTRIES_IN_32_BIT_MODE 1024

/*Bit Indices*/

/*PTE/PDE Common Fields For 4k Pages */
#define PRESENT_BIT_INDEX 0
#define READ_WRITE_BIT_INDEX 1
#define USER_ACCESS_BIT_INDEX 2
#define WRITE_THROUGH_BIT_INDEX 3
#define CACHEABLE_BIT_INDEX 4
#define SOFTWARE_ACCESS_BIT_INDEX 5

//this field is in the next byte of the entry; the one above are all in the same byte
#define PAGE_FRAME_ADDY_LOWER_BITS_BIT_INDEX 12
#define PAGE_FRAME_ADDY_HIGH_BITS_BIT_INDEX 16

/*Page Table Entry (PTE) Unique Flags/Fields */
#define PTE_SOFTWARE_WRITTEN_BIT_INDEX 6
#define PTE_PAT_INDEX_BIT_INDEX 7

//next byte
#define IS_GLOBAL_PAGE_BIT_INDEX 0
#define PTE_IGNORED_FIELD_BIT_INDEX 1

/*Page Directory Entry (PDE) Unique Flags/Fields */

#define PDE_IGNORED_BIT_INDEX 6
#define PDE_IS_4MB_PAGE_ENTRY_BIT_INDEX 7

//next byte
#define PDE_IGNORED_FIELD_BIT_INDEX 0

/* Bit Values (4k Pages) */

/* Common */

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

/*Page Table Entry (PTE) Unique Flags/Fields */

#define PTE_SOFTWARE_WRITTEN (1 << (PTE_SOFTWARE_WRITTEN_BIT_INDEX))
#define PTE_SOFTWARE_HASNT_WRITTEN (0 << (PTE_SOFTWARE_WRITTEN_BIT_INDEX))

#define PTE_PAT_INDEX (1 << (PTE_PAT_INDEX_BIT_INDEX))
#define PTE_NO_PAT (0 << (PTE_PAT_INDEX_BIT_INDEX))

//next byte
#define PTE_IS_GLOBAL (1 << (IS_GLOBAL_PAGE_BIT_INDEX))
#define PTE_NOT_GLOBAL (0 << (IS_GLOBAL_PAGE_BIT_INDEX))

/*Page Directory Entry (PDE) Unique Flags/Fields */

#define PDE_IGNORED (0 << (PTE_IGNORED_FIELD_BIT_INDEX))

#define PDE_ENTRY_4MB_PAGE (1 << (PDE_IS_4MB_PAGE_ENTRY_BIT_INDEX))
#define PDE_ENTRY_PAGE_TAB (0 << (PDE_IS_4MB_PAGE_ENTRY_BIT_INDEX))

/* Misc Macros*/

#define PAGE_DIR_INDEX_BIT_INDEX 22
#define PAGE_TAB_INDEX_BIT_INDEX 12

//this is defined in the Makefile
#ifdef PAGE_SIZE_4K
#define PAGE_SIZE 4096
#else
#define PAGE_SIZE (4 * 1024 * 1024)
#endif

#define ISOLATE_PAGE_TAB_ADDRESS_HIGH_BITS_BITMASK 0xffff
#define ISOLATE_PAGE_TAB_ADDRESS_LOW_BITS_BITMASK  0xf


typedef struct {
    uint8_t flags;
    uint8_t page_frame_addy_low_and_ignored_field_and_global_flag;
    uint16_t page_frame_addy_high;
} Page_Table_Entry_t __attribute__((packed));

typedef struct {
    Page_Table_Entry_t page_frames[NUM_PAGING_STRUCTURE_ENTRIES_IN_32_BIT_MODE];
} Page_Table_t __attribute__((packed));

typedef struct {
    uint8_t flags;
    uint8_t page_tab_addy_low_and_ignored_field;
    uint16_t page_tab_addy_high;
} __attribute__((packed)) Page_Directory_Entry_t;

typedef struct {
    Page_Directory_Entry_t page_tables[NUM_PAGING_STRUCTURE_ENTRIES_IN_32_BIT_MODE];
} Page_Directory_t __attribute__((packed));