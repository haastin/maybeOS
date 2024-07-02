#ifndef __IDT_H__
#define __IDT_H__

#include <stdint.h>

struct gate_descriptor_32bit{
    uint16_t offset_low;
    uint16_t segment_selector;
    uint8_t unused;
    uint8_t flags;
    uint16_t offset_high;
} __attribute__((packed));

struct idt_register_data_32bit{
    uint16_t limit;
    uint32_t base_address;
} __attribute__((packed));

#define KERNEL_SEG (0 << 6)
#define USER_SEG (3 << 6)

#define PRESENT_IN_MEMORY 1
#define NOT_PRESENT_IN_MEMORY 0

#define INTERRUPT_GATE_BASE_FLAG_VALS 6
#define TRAP_GATE_BASE_FLAG_VALS 7

#define GATE_DESC_IS_32BIT (1 << 3)
#define GATE_DESC_IS_16BIT (0 << 3)

void build_gate_descriptor(struct gate_descriptor_32bit * gate_desc, uint32_t base_address, uint16_t segment_selector, uint8_t flags);



#endif/* __IDT_H__*/