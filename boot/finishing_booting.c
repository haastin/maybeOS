#include "gdt.h"
#include "stdint.h"

void build_gdt(void){

    //we will use a Protected Flat Model, as described in Intel's docs
    //for now, we just build null, kernel code, and kernel data segments
    struct segment_descriptor gdt[3];

    //our null descriptor
    build_segment_descriptor(&gdt[0], 0, 0, 0, 0); 
    
    //our kernel code segment 
    uint8_t flag_lowbits = PROCESS_SEGMENT | KERNEL_SEGMENT | SEGMENT_IS_IN_MEMORY | CODE_SEGMENT | 
    CALLABLE_BY_LESS_PRIVILEGED_CODE | READABLE | NOT_ACCESSED;
    uint8_t flag_highbits = NATIVELY_32BIT | AVAILBLE_FLAG_NOT_USED | DEFAULT_32BIT_INSTR_SIZE | 
    EXTENDED_SEGMENT_LIMIT;
    build_segment_descriptor(&gdt[1], 0x0, 0xFFFFF, flag_lowbits, flag_highbits); 

    //our kernel data segment
    flag_lowbits = 0 | PROCESS_SEGMENT | KERNEL_SEGMENT | SEGMENT_IS_IN_MEMORY | DATA_SEGMENT | EXPAND_DOWN | 
    WRITABLE | NOT_ACCESSED;
    flag_highbits = 0 | NATIVELY_32BIT | AVAILBLE_FLAG_NOT_USED | ON_32BIT_PLATFORM | 
    EXTENDED_SEGMENT_LIMIT;
    build_segment_descriptor(&gdt[2], 0x0, 0xFFFFF, flag_lowbits, flag_highbits);
}

void kernel_start(void){
    build_gdt();
}