#include "gdt.h"
#include <stdint.h>
#include "multiboot2.h"
#include "include/string.h"
#include "VGA_driver.h"
#include "idt.h"
#include "isr.h"
#include "apic.h"

#define NUM_GDT_ENTRIES 3
#define KERNEL_CODE_SEG_GDT_INDEX 1
#define KERNEL_DATA_SEG_GDT_INDEX 2

#define NUM_SUPPORTED_INTERRUPTS 256
#define INTERRUPT_GATE_SIZE 8


//move this to multiboot header??
/*size+7 increases size to the next multiple of 8, & ~7 sets the lower 3 bits to 0*/
#define ALIGN_TO_8_BYTES(tag_size) ((tag_size + 7) & ~7)


__attribute__((aligned(8)))struct gate_descriptor_32bit idt[NUM_SUPPORTED_INTERRUPTS];

//we will use a Protected Flat Model, as described in Intel's docs
//for now, we just build null, kernel code, and kernel data segments
__attribute__((aligned(8))) struct segment_descriptor gdt[NUM_GDT_ENTRIES];

extern struct multiboot_bootinfo mb_bootinfo;

static void init_gdt(void){

    //our null descriptor
    build_segment_descriptor(&gdt[0], 0, 0, 0, 0); 
    
    //our kernel code segment 
    uint8_t flag_lowbits = PROCESS_SEGMENT | KERNEL_SEGMENT | SEGMENT_IS_IN_MEMORY | CODE_SEGMENT | 
    CALLABLE_BY_LESS_PRIVILEGED_CODE | READABLE | NOT_ACCESSED;
    uint8_t flag_highbits = NATIVELY_32BIT | AVAILBLE_FLAG_NOT_USED | DEFAULT_32BIT_INSTR_SIZE | 
    EXTENDED_SEGMENT_LIMIT;
    /*limit is 0xFFFFF because the granularity flag  extends the maximum possible segment limit by multiplying
    the segment limit value by 2^12, a standard page length. this changes the units of the limit we pass from bytes
    to pages, so 0xFFFFF with G flag set will cover all of memory*/
    build_segment_descriptor(&gdt[KERNEL_CODE_SEG_GDT_INDEX], 0x0, 0xFFFFF, flag_lowbits, flag_highbits); 

    //our kernel data segment
    flag_lowbits = 0 | PROCESS_SEGMENT | KERNEL_SEGMENT | SEGMENT_IS_IN_MEMORY | DATA_SEGMENT | EXPAND_DOWN | 
    WRITABLE | NOT_ACCESSED;
    flag_highbits = 0 | NATIVELY_32BIT | AVAILBLE_FLAG_NOT_USED | ON_32BIT_PLATFORM | 
    EXTENDED_SEGMENT_LIMIT;
    build_segment_descriptor(&gdt[KERNEL_DATA_SEG_GDT_INDEX], 0x0, 0xFFFFF, flag_lowbits, flag_highbits);

    struct gdt_register_data gdt_reg_info;
    gdt_reg_info.base = (uint32_t)gdt;
    gdt_reg_info.limit = sizeof(struct segment_descriptor)*NUM_GDT_ENTRIES -1;

    asm volatile ("lgdt %0" : : "m" (gdt_reg_info));
}

static void init_idt(void){
    uint8_t flags = INTERRUPT_GATE_BASE_FLAG_VALS | GATE_DESC_IS_32BIT | KERNEL_SEG | PRESENT_IN_MEMORY;
    extern uint32_t arch_defined_interrupts_start;
    for(size_t interrupt_vector = 0; interrupt_vector < 32; interrupt_vector++){
        build_gate_descriptor(&idt[interrupt_vector], arch_defined_interrupts_start + (interrupt_vector*16), KERNEL_CODE_SEG_GDT_INDEX, flags);
    }

    struct idt_register_data_32bit idtr;
    idtr.base_address = (uint32_t) idt;
    /*this is the last valid address of the idt, since it includes the base address*/
    idtr.limit = (NUM_SUPPORTED_INTERRUPTS*INTERRUPT_GATE_SIZE) -1; 

    asm volatile("lidt %0\n" :: "m" (idtr));
}

static void store_multiboot2_bootinfo(void){
    uint32_t *multiboot2_bootinfo_startaddress;
    asm volatile (
        "movl %%ebx, %0;" // Move ebx into my_var
        : "=r" (multiboot2_bootinfo_startaddress)   // Output operand
        :                 // Input operand (none)
        : "%ebx"          // Clobbered register (ebx)
    );

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

                    
            }
        }
}  

void kernel_start(void){
    
    //need to refresh the seg registers w/ their new gdt segments
    init_gdt();

    init_idt();
    store_multiboot2_bootinfo();

    print();

    asm volatile("mov $1, %%eax\n"
                "xor %%edx, %%edx\n"
                "mov $0, %%ebx\n"
                "div %%ebx\n" ::);
   
    return;
}