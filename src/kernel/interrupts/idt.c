#include "idt.h"

void build_interrupt_gate_descriptor(struct gate_descriptor_32bit * gate_desc, uint32_t base_address, uint16_t segment_selector, unsigned char bit_mode, unsigned char desc_priv_level, unsigned char seg_present){

    gate_desc->offset_low = base_address & 0xFFFF;
    gate_desc->offset_high = (base_address >> 16) & 0xFFFF;
    gate_desc->segment_selector = segment_selector;
    gate_desc->unused = 0;
    gate_desc->gate_type = 0b110;
    gate_desc->gate_bitmode = bit_mode;
    gate_desc->zero = 0;
    gate_desc->desc_priv_level = desc_priv_level;
    gate_desc->seg_present = seg_present;
}