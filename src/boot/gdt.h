#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/*everything here was taken as defined in Intel's Segementation memory
management technique for the IA-32 architecture*/

struct segment_descriptor{
    uint16_t segmentlimit_lowbits;
    uint16_t baseaddress_lowbits;
    uint8_t baseaddress_middlebits;
    uint8_t flags_lowbits;
    uint8_t seglimit_and_flags_highbits;
    uint8_t baseaddress_highbits;
} __attribute__((packed));

struct gdt_register_data{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/*no global structure for a GDT because it is up to the end user how many
segment desciptors they want in it, so we'll leave that for a different file*/
#define KERNEL_CODE_SEG_GDT_INDEX 1
#define KERNEL_DATA_SEG_GDT_INDEX 2

#define KERNEL_CODE_SEG_SELECTOR 0x8

/*we define some constants and macros to implement the flags portion
of a GDT descriptor to improve readability*/

//FLAG HIGHBITS

//G (Granularity) flag field
#define REGULAR_SEGMENT_LIMIT (0 << 7)
#define EXTENDED_SEGMENT_LIMIT (1 << 7)

// D or B (Default operation size) flag field

//D field (if this is for a code segment)
#define DEFAULT_16BIT_INSTR_SIZE (0 << 6)
#define DEFAULT_32BIT_INSTR_SIZE (1 << 6)

//B field (if this is for a data segment)
#define ON_16BIT_PLATFORM (0 << 6)
#define ON_32BIT_PLATFORM (1 << 6)

//L (Long mode) flag field
#define NATIVELY_32BIT (0 << 5)
#define NATIVELY_64BIT (1 << 5)

//AVL (Available for software use) flag field
#define AVAILBLE_FLAG_NOT_USED (0 << 4)

//FLAG LOWBITS

//P (Present) flag field
#define SEGMENT_IS_NOT_IN_MEMORY (0 << 7)
#define SEGMENT_IS_IN_MEMORY (1 << 7)


//DPL (Descriptor Privilege Level) flag field
#define KERNEL_SEGMENT (0 << 5)
#define USERSPACE_SEGMENT (3 << 5)

//S (Desciptor type) flag field
#define SYSTEM_SEGMENT (0 << 4)
#define PROCESS_SEGMENT (1 << 4)

//Type flag field

//if S == 1

//bit 3 in Type field
#define DATA_SEGMENT (0 << 3)
#define CODE_SEGMENT (1 << 3)

//remaining type field bits for a data segment

//bit 2 in Type field
#define EXPAND_UP (0 << 2)
#define EXPAND_DOWN (1 << 2)

//bit 1 in Type field
#define NOT_WRITABLE (0 << 1)
#define WRITABLE (1 << 1)

//remaining type field bits for a code segment

//bit 2 in Type field
#define CALLABLE_BY_LESS_PRIVILEGED_CODE (0 << 2)
#define NOT_CALLABLE_BY_LESS_PRIVILEGED_CODE (1 << 2)

//bit 1 in Type field
#define NOT_READABLE (0 << 1)
#define READABLE (1 << 1)

//both code and data segment types use this Type field

//bit 0 in Type field
#define NOT_ACCESSED 0
#define ACCESSED 1

//if S == 0

//all Type field bits are accounted for for all of these
#define LDT 2 
#define BUSY_TSS_16BIT 3
#define CALL_GATE_16BIT 4
#define TASK_GATE 5
#define INTERRUPT_GATE_16BIT 6
#define TRAP_GATE_16BIT 7
#define AVAILABLE_TSS_32BIT 9
#define BUSY_TSS_32BIT 11
#define CALL_GATE_32BIT 12
#define INTERRUPT_GATE_32BIT 14
#define TRAP_GATE_32BIT 15

void build_segment_descriptor(struct segment_descriptor * seg_desc, uint32_t baseaddress, uint32_t segmentlimit, uint8_t flag_lowbits, uint8_t flag_highbits);

#endif //GDT_H