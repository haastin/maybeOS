#include "gdt.h"
#include <stdint.h>
#include "multiboot2.h"
#include "string.h"
#include "VGA_driver.h"
#include "ps2.h"
#include "idt.h"
#include "isr.h"
#include "mem_map.h"
#include "apic.h"
#include "acpi.h"
#include "lapic.h"

#include "utils.h"


#define NUM_SUPPORTED_INTERRUPTS 256
#define INTERRUPT_GATE_SIZE 8

#define VECTOR_X_INTERRUPT_VECTOR_HANDLER_MAX_BYTES 32
 
//TODO: move this to multiboot header??
/*size+7 increases size to the next multiple of 8, & ~7 sets the lower 3 bits to 0*/
#define ALIGN_TO_8_BYTES(tag_size) ((tag_size + 7) & ~7)




__attribute__((aligned(8)))struct gate_descriptor_32bit idt[NUM_SUPPORTED_INTERRUPTS];

//we will use a Protected Flat Model, as described in Intel's docs
//for now, we just build null, kernel code, and kernel data segments
__attribute__((aligned(8))) struct segment_descriptor gdt[NUM_GDT_ENTRIES];

extern struct multiboot_bootinfo mb_bootinfo;


static void init_idt(void){
    extern uint32_t arch_defined_interrupts_start;
    for(size_t interrupt_vector = 0; interrupt_vector < 256; interrupt_vector++){
        uint32_t interr_handler_addy = ((char*)&arch_defined_interrupts_start + (interrupt_vector*VECTOR_X_INTERRUPT_VECTOR_HANDLER_MAX_BYTES));
        build_interrupt_gate_descriptor(&idt[interrupt_vector], interr_handler_addy, KERNEL_CODE_SEG_SELECTOR, GATE_DESC_IS_32BIT, KERNEL_SEG, PRESENT_IN_MEMORY);
    }

    struct idt_register_data_32bit idtr;
    idtr.base_address = (uint32_t) idt;
    /*this is the last valid address of the idt, since it includes the base address*/
    idtr.limit = (NUM_SUPPORTED_INTERRUPTS*INTERRUPT_GATE_SIZE) -1; 

    asm volatile("lidt %0\n" :: "m" (idtr));
}

static void process_multiboot2_bootinfo(uint32_t multiboot2_bootinfo_startaddress){

    //this loop logic is from GRUB's multiboot2 example code, slightly modified to improve readability
    for(struct multiboot_tag *tag = (struct multiboot_tag *)((char*)multiboot2_bootinfo_startaddress + 8);
        tag->type != MULTIBOOT_TAG_TYPE_END;
        tag = (struct multiboot_tag *)((char *)tag + ALIGN_TO_8_BYTES(tag->size))
        ){
            switch (tag->type){
                case (MULTIBOOT_TAG_TYPE_FRAMEBUFFER): 
                    struct multiboot_tag_framebuffer * framebuffer_info = (struct multiboot_tag_framebuffer *) tag;
                    initialize_framebuffer_attributes(framebuffer_info);
                    break;
                case (MULTIBOOT_TAG_TYPE_ACPI_NEW):
                    struct multiboot_tag_new_acpi * acpi_info_tag = (struct multiboot_tag_new_acpi *) tag;
                    process_acpi_info(acpi_info_tag);
                    break;
                case(MULTIBOOT_TAG_TYPE_MMAP):
                    struct multiboot_tag_mmap * mmap_tag = (struct multiboot_tag_mmap *) tag;
                    init_memory(mmap_tag);
                    break;      
            }
        }
}

void initialize_interrupts(void){
    init_idt();
    initialize_lapic();
    initialize_ioapic();
    asm volatile("sti\n");
    //initialize_interrupt_source_overrides();
}

void initialize_peripheral_devices(void){
    initialize_ps2keyboard();
    bool res = init_serial_port();
    if(!res){
        //TODO: throw some error?
    }
}


void kernel_start(uint32_t multiboot2_bootinfo_startaddress){
    
    //at this point the GDT is initialized and paging is enabled
    
    process_multiboot2_bootinfo(multiboot2_bootinfo_startaddress);
    
    initialize_interrupts();
    initialize_peripheral_devices();

    //this will not return
    start_shell();
}