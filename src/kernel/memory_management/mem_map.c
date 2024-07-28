
#include "mem_map.h"
#include "string.h"
#include <stdbool.h>
#include "paging.h"

mem_bank_t boot_cpu_mem;

//TODO: right now the mm subsystem needs mem info to start, so can't use malloc. add some sort of boot mem allocator so i can dynamically track how many reserved regions, etc. there are
usable_region_t regions[MAX_SUPPORTED_RESERVED_BOOT_REGIONS];



static void init_usable_region(struct multiboot_mmap_entry * mem_entry, unsigned char next_free_reserved_region_idx){
    regions[next_free_reserved_region_idx].start = mem_entry->addr;
    regions[next_free_reserved_region_idx].length = mem_entry->len;
}

void init_memory(struct multiboot_tag_mmap *mmap_tag){

    size_t num_mmap_entries = (mmap_tag->size - sizeof(struct multiboot_tag_mmap))/mmap_tag->entry_size;
    
    struct multiboot_mmap_entry * mem_entry = (struct multiboot_mmap_entry *) &mmap_tag->entries[0];

    unsigned char next_free_reserved_region_idx = 0;

    for(size_t mem_entry_idx=0;mem_entry_idx<num_mmap_entries;mem_entry_idx++){

        switch(mem_entry->type){
            case(MULTIBOOT_MEMORY_AVAILABLE):
                init_usable_region(mem_entry, next_free_reserved_region_idx);
                next_free_reserved_region_idx++;
                break;

            //all pages in the system are marked allocated by default, so these will never be freed and don't need to worry about them
            case(MULTIBOOT_MEMORY_ACPI_RECLAIMABLE):
                break;
            case(MULTIBOOT_MEMORY_NVS):
                break;
            case(MULTIBOOT_MEMORY_RESERVED):
                break;
            case(MULTIBOOT_MEMORY_BADRAM):
                break;
        };
        boot_cpu_mem.length += mem_entry->len;
        mem_entry++;
    }

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