#ifndef __APIC_H__
#define __APIC_H__

#include <stdint.h>

union Interrupt_Redirection_Table_Entry{
    struct{
    uint8_t interrupt_vector_num;
    unsigned int delivery_mode : 3;
    unsigned int dest_mode : 1;
    unsigned int delivery_status : 1;
    unsigned int pin_polarity : 1;
    unsigned int remote_irr : 1;
    unsigned int trigger_mode : 1;
    unsigned int interrupt_mask : 1;
    unsigned int reserved_low : 7;
    uint32_t reserved_high;
    uint8_t destination;
    } fields;
    uint64_t bits;

} __attribute__((packed));

struct IOAPIC {
    uint32_t *IO_Register_Selector;
    uint32_t *IO_Window_Register;

    /*the irt struct should never be accessed outside the get/set functions, because
    changing its values has no effect on the actual IOAPIC; to do so the IO Reg Select
    and IO Window registers must be used. The result of this is that the irt below is simply
    to cache the current IRT in the IOAPIC so we don't have to access the IO registers to get
    any IRT entry values */
    union Interrupt_Redirection_Table_Entry irt[24];

} __attribute__ ((packed));

uint32_t read_ioapic_reg(uint8_t reg_num);

void initialize_ioapic();

#endif /*__APIC_H__*/