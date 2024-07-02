#ifndef __IDT_H__
#define __IDT_H__

#include <stdint.h>

struct gate_descriptor_32bit{
    uint16_t offset_low;
    uint16_t segment_selector;
    uint8_t unused;
    unsigned int gate_type : 3;
    unsigned int gate_bitmode : 1;
    unsigned int zero : 1;
    unsigned int desc_priv_level : 2;
    unsigned int seg_present : 1;
    uint16_t offset_high;
} __attribute__((packed));

struct idt_register_data_32bit{
    uint16_t limit;
    uint32_t base_address;
} __attribute__((packed));

#define KERNEL_SEG 0 
#define USER_SEG 3

#define PRESENT_IN_MEMORY 1 
#define NOT_PRESENT_IN_MEMORY 0 

#define GATE_DESC_IS_32BIT 1
#define GATE_DESC_IS_16BIT 0

void build_interrupt_gate_descriptor(struct gate_descriptor_32bit *gate_desc, uint32_t base_address, uint16_t segment_selector, unsigned char bit_mode, unsigned char desc_priv_level, unsigned char seg_present);


#endif/* __IDT_H__*/


