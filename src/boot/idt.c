#include "idt.h"

void build_gate_descriptor(struct gate_descriptor_32bit * gate_desc, uint32_t base_address, uint16_t segment_selector, uint8_t flags){

    gate_desc->offset_low = base_address & 0xFFFF;
    gate_desc->offset_high = (base_address >> 16) & 0xFFFF;
    gate_desc->segment_selector = segment_selector;
    gate_desc->unused = 0;
    gate_desc->flags = flags;
}