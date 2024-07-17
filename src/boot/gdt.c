#include "gdt.h"
#include "utils.h"

extern struct segment_descriptor gdt[];

/*for the sake of convenience, we pass entire fields to build a segment descriptor and this will
parse them and properly divvy them up into their assigned places*/
void build_segment_descriptor(struct segment_descriptor * seg_desc, uint32_t baseaddress, uint32_t segmentlimit, uint8_t flag_lowbits, uint8_t flag_highbits){
   
   seg_desc->segmentlimit_lowbits = segmentlimit & 0x0000FFFF;
   seg_desc->seglimit_and_flags_highbits = ((segmentlimit >>16) & 0x000F) | flag_highbits;
   
   seg_desc->baseaddress_lowbits = baseaddress & 0x0000FFFF;
   seg_desc->baseaddress_middlebits = (baseaddress >> 16) & 0x00FF;
   seg_desc->baseaddress_highbits = (baseaddress >> 24) & 0xFF;

   seg_desc->flags_lowbits = flag_lowbits;
}

void init_gdt(void) {

    //our null descriptor
    unsigned int gdt_phys_addy = (uint32_t)&gdt[0] - (uint32_t)&_kernel_vm_offset;
    build_segment_descriptor(gdt_phys_addy, 0, 0, 0, 0); 
    
    //our kernel code segment 
    uint8_t flag_lowbits = PROCESS_SEGMENT | KERNEL_SEGMENT | SEGMENT_IS_IN_MEMORY | CODE_SEGMENT | 
    CALLABLE_BY_LESS_PRIVILEGED_CODE | READABLE | NOT_ACCESSED;
    uint8_t flag_highbits = NATIVELY_32BIT | AVAILBLE_FLAG_NOT_USED | DEFAULT_32BIT_INSTR_SIZE | 
    EXTENDED_SEGMENT_LIMIT;
    /*limit is 0xFFFFF because the granularity flag  extends the maximum possible segment limit by multiplying
    the segment limit value by 2^12, a standard page length. this changes the units of the limit we pass from bytes
    to pages, so 0xFFFFF with G flag set will cover all of memory*/
    build_segment_descriptor(((uint32_t)&gdt[KERNEL_CODE_SEG_GDT_INDEX] - (uint32_t)&_kernel_vm_offset), 0x0, 0xFFFFF, flag_lowbits, flag_highbits); 

    //our kernel data segment
    flag_lowbits = 0 | PROCESS_SEGMENT | KERNEL_SEGMENT | SEGMENT_IS_IN_MEMORY | DATA_SEGMENT | EXPAND_UP | 
    WRITABLE | NOT_ACCESSED;
    flag_highbits = 0 | NATIVELY_32BIT | AVAILBLE_FLAG_NOT_USED | ON_32BIT_PLATFORM | 
    EXTENDED_SEGMENT_LIMIT;
    build_segment_descriptor(((uint32_t)&gdt[KERNEL_DATA_SEG_GDT_INDEX]- (uint32_t)&_kernel_vm_offset), 0x0, 0xFFFFF, flag_lowbits, flag_highbits);

    struct gdt_register_data gdt_reg_info;
    gdt_reg_info.base = phys_addy((uint32_t)gdt);
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