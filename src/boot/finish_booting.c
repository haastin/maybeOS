#include "gdt.h"
#include <stdint.h>
#include "multiboot2.h"
#include "string.h"
#include "VGA_driver.h"
#include "ps2.h"
#include "idt.h"
#include "isr.h"

#include "apic.h"
#include "acpi.h"
#include "lapic.h"



#define NUM_GDT_ENTRIES 3

#define NUM_SUPPORTED_INTERRUPTS 256
#define INTERRUPT_GATE_SIZE 8

#define VECTOR_X_INTERRUPT_VECTOR_HANDLER_MAX_BYTES 32
 
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
    flag_lowbits = 0 | PROCESS_SEGMENT | KERNEL_SEGMENT | SEGMENT_IS_IN_MEMORY | DATA_SEGMENT | EXPAND_UP | 
    WRITABLE | NOT_ACCESSED;
    flag_highbits = 0 | NATIVELY_32BIT | AVAILBLE_FLAG_NOT_USED | ON_32BIT_PLATFORM | 
    EXTENDED_SEGMENT_LIMIT;
    build_segment_descriptor(&gdt[KERNEL_DATA_SEG_GDT_INDEX], 0x0, 0xFFFFF, flag_lowbits, flag_highbits);

    struct gdt_register_data gdt_reg_info;
    gdt_reg_info.base = (uint32_t)gdt;
    gdt_reg_info.limit = sizeof(struct segment_descriptor)*NUM_GDT_ENTRIES -1;

    asm volatile ("lgdt %0" : : "m" (gdt_reg_info));

    /**need to flush our current seg regs to make sure that they point to valid segment descriptors and to clear their cached values. 0x10 is the segment selector for the kernel data segment, and the cs reg can be set automatically by the cpu by doing a far jump, which is the jmp command */
    asm volatile ("mov $0x10, %eax\n"
                "mov %eax, %ds\n"
                "mov %eax, %ss\n"
                "mov %eax, %es\n"
                "mov %eax, %fs\n"
                "mov %eax, %gs\n"
                "jmp $0x8,$.continue\n"
                ".continue:"
                );
}

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

void initialize_interrupts(void){
    initialize_lapic();
    initialize_ioapic();
    asm volatile("sti\n");
    //initialize_interrupt_source_overrides();
}

void kernel_start(void){
    
    init_gdt();
    init_idt();
    store_multiboot2_bootinfo();
    initialize_interrupts();
    initialize_ps2keyboard();
    initialize_shell_UI();
    
    asm volatile("loop2: jmp loop2\n");
    return;
}