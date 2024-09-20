
#include "mem_map.h"
#include "string.h"
#include <stdbool.h>
#include "paging.h"
#include "utils.h"
#include "driver.h"

mem_bank_t boot_cpu_mem;

//TODO: right now the mm subsystem needs mem info to start, so can't use malloc. add some sort of boot mem allocator so i can dynamically track how many reserved regions, etc. there are
usable_region_t regions[MAX_SUPPORTED_RESERVED_BOOT_REGIONS];



static void init_usable_region(struct multiboot_mmap_entry * mem_entry, unsigned char next_free_reserved_region_idx){
    regions[next_free_reserved_region_idx].start = mem_entry->addr;
    regions[next_free_reserved_region_idx].length = mem_entry->len;
}

void init_memory(struct multiboot_tag_mmap *mmap_tag){

    //num of memory regions provided by the bootloader
    size_t num_mmap_entries = (mmap_tag->size - sizeof(struct multiboot_tag_mmap))/mmap_tag->entry_size;
    
    //the first memory region
    struct multiboot_mmap_entry * mem_entry = (struct multiboot_mmap_entry *) &mmap_tag->entries[0];

    //keeping track of the next free static array index, which tracks usable ram regions
    unsigned char next_free_reserved_region_idx = 0;

    //keep track of this because we can't process it at the time we discover it beacuse the mm system isn't set up yet
    struct multiboot_mmap_entry * acpi_entry = NULL;

    for(size_t mem_entry_idx=0;mem_entry_idx<num_mmap_entries;mem_entry_idx++){

        switch(mem_entry->type){
            case(MULTIBOOT_MEMORY_AVAILABLE):
                init_usable_region(mem_entry, next_free_reserved_region_idx);
                next_free_reserved_region_idx++;
                break;

            case(MULTIBOOT_MEMORY_ACPI_RECLAIMABLE):
                acpi_entry = mem_entry;
                break;

            //all pages in the system are marked allocated by default, so these will never be freed and don't need to worry about them
            case(MULTIBOOT_MEMORY_NVS):
                break;
            case(MULTIBOOT_MEMORY_RESERVED):
                break;
            case(MULTIBOOT_MEMORY_BADRAM):
                break;
        };

        //add all mem, whether its usable or not, to the cpu mem length because RAM will most likely NOT be contiguous, so for simplcitys sake, keeping track of all pfns is much easier than select pfns which may have a pfn val different from its index in the bitmap, which makes things complicated. this avoids dealing with that
        boot_cpu_mem.length += mem_entry->len;
        mem_entry++;
    }

    //need to initialize the pmm before continuing because we need to start allocating mem and perform mappings for device that also get registered by the multiboot tags (their tags are processed after this memory map tag)
    init_pmm(boot_cpu_mem.length);

    init_vmm();

    //the vmm initializes the kheap so it can remember the area it allocated to the kheap, init the kheap, and then create its dynamic alloc area which will mark that area of the kernel virt addy space as used. that relies on kmalloc, so otherwise the vmm would have to remember the area it gave to the kheap across function calls if not packaged within the same function
    vmm_init_kheap();

    //can only init after the mm system is setup. preserves ACPI info so it doesnt get overwritten, is not a real device
    init_MMIO_device(acpi_entry->addr, acpi_entry->len);
}

bool is_page_usable(unsigned long pageframe_address){

    for(usable_region_t * reg = &regions[0]; reg->length != 0; reg++){
        unsigned long reg_end = reg->start + reg->length;
        if(pageframe_address >= reg->start && (pageframe_address+PAGE_SIZE-1) < reg_end){
            return true;
        }
    }
    return false;
}