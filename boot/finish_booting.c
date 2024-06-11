#include "gdt.h"
#include "stdint.h"
#include "multiboot2.h"

#define NUM_GDT_ENTRIES 3
/*size+7 increases size to the next multiple of 8, & ~7 sets the lower 3 bits to 0*/
#define ALIGN_TO_8_BYTES(tag_size) ((tag_size + 7) & ~7)

#define FRAMEBUFFER_ADDRESS_OFFSET 8
#define FRAMEBUFFER_HEIGHT_OFFSET 24
#define FRAMEBUFFER_WIDTH_OFFSET 20
#define FRAMEBUFFER_BITSPERPIXEL_OFFSET 28

//we will use a Protected Flat Model, as described in Intel's docs
//for now, we just build null, kernel code, and kernel data segments
struct segment_descriptor gdt[NUM_GDT_ENTRIES];

struct multiboot_bootinfo mb_bootinfo;
uint8_t * framebuffer;
uint32_t width;
uint32_t height; 
uint8_t bits_per_pixel;
uint8_t bytes_per_pixel;


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
            if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER){
                framebuffer = *(uint32_t *)((char *)tag + FRAMEBUFFER_ADDRESS_OFFSET);
                //mb_bootinfo.framebuffer.common.framebuffer_addr = (uint32_t)*((char *)&tag + 12);
                width = *(uint32_t *)((char *)tag + FRAMEBUFFER_WIDTH_OFFSET);
                height = *(uint32_t *)((char *)tag + FRAMEBUFFER_HEIGHT_OFFSET);
                bits_per_pixel = *(uint32_t *)((char *)tag + FRAMEBUFFER_BITSPERPIXEL_OFFSET);
                bytes_per_pixel = bits_per_pixel/8;
                for(int col = 0; col < width; col++){
                    for(int row=0; row<height; row++){
                    *(framebuffer + row*col*bytes_per_pixel) = 0xFF;
                    }
                }
                return;
            }
            else{
                *framebuffer = 0xDEADBEEF;
            }
        }
}

void kernel_start(void){
    
    init_gdt();
    
    store_multiboot2_bootinfo();
   
    return;
}