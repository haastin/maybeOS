#include "gdt.h"
#include <stdint.h>
#include "multiboot2.h"
#include "lib/string.h"
#include "VGA_driver.h"

#define NUM_GDT_ENTRIES 3
/*size+7 increases size to the next multiple of 8, & ~7 sets the lower 3 bits to 0*/
#define ALIGN_TO_8_BYTES(tag_size) ((tag_size + 7) & ~7)

//we will use a Protected Flat Model, as described in Intel's docs
//for now, we just build null, kernel code, and kernel data segments
struct segment_descriptor gdt[NUM_GDT_ENTRIES];

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
    build_segment_descriptor(&gdt[1], 0x0, 0xFFFFF, flag_lowbits, flag_highbits); 

    //our kernel data segment
    flag_lowbits = 0 | PROCESS_SEGMENT | KERNEL_SEGMENT | SEGMENT_IS_IN_MEMORY | DATA_SEGMENT | EXPAND_DOWN | 
    WRITABLE | NOT_ACCESSED;
    flag_highbits = 0 | NATIVELY_32BIT | AVAILBLE_FLAG_NOT_USED | ON_32BIT_PLATFORM | 
    EXTENDED_SEGMENT_LIMIT;
    build_segment_descriptor(&gdt[2], 0x0, 0xFFFFF, flag_lowbits, flag_highbits);

    struct gdt_register_data gdt_reg_info;
    gdt_reg_info.base = (uint32_t)gdt;
    gdt_reg_info.limit = sizeof(struct segment_descriptor)*NUM_GDT_ENTRIES -1;

    asm volatile ("lgdt %0" : : "m" (gdt_reg_info));
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
            }
        }
}

static void initialize_framebuffer_attributes(struct multiboot_tag_framebuffer * framebuffer_info){
    framebuffer.starting_address = framebuffer_info->common.framebuffer_addr;
    framebuffer.width = framebuffer_info->common.framebuffer_width;
    framebuffer.height = framebuffer_info->common.framebuffer_height;

    framebuffer.pixel.total_bits = framebuffer_info->common.framebuffer_bpp;
    framebuffer.pixel.num_red_bits = framebuffer_info->framebuffer_red_mask_size;
    framebuffer.pixel.red_bits_index = framebuffer_info->framebuffer_red_field_position;
    framebuffer.pixel.num_green_bits = framebuffer_info->framebuffer_green_mask_size;
    framebuffer.pixel.green_bits_index = framebuffer_info->framebuffer_green_field_position;
    framebuffer.pixel.num_blue_bits = framebuffer_info->framebuffer_blue_mask_size;
    framebuffer.pixel.blue_bits_index = framebuffer_info->framebuffer_blue_field_position;

}

void kernel_start(void){
    
    init_gdt();
    
    store_multiboot2_bootinfo();
   
    return;
}