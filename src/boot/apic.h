#ifndef __APIC_H__
#define __APIC_H__

#include <stdint.h>
#include "multiboot2.h"

#define LAPIC_EOI_REG_OFFSET 0xB0

#define ACPI_MADT_IOAPIC 1
#define ACPI_MADT_SIG "APIC"

struct RootSystemDescriptorPointer_1 {
    char RSDP_signature[8];
    uint8_t checksum;
    char OEM_ID[6];
    uint8_t RSD_version;
    uint32_t RSDtable_address;
} __attribute__ ((packed));

//only use Extended table if its present, not the RSD table
struct RootSystemDescriptorPointer_2 {
    struct RootSystemDescriptorPointer_1 rsd1_header;
    uint32_t RSDtable_size;
    uint64_t ExtendedSystemDescriptorTable_address;
    uint8_t extended_checkdum;
    uint8_t reserved[3];
} __attribute__ ((packed));

struct SystemDescriptionTable_Header {
    char SDT_sig[4];
    uint32_t SDT_length;
    const uint8_t revision;
    uint8_t checksum;
    char OEM_ID[6];
    uint64_t OEMtable_ID;
    uint32_t OEM_revision;
    uint32_t creator_ID;
    uint32_t creator_revision;
} __attribute__ ((packed));

struct RootSystemDescriptorTable {
    struct SystemDescriptionTable_Header header;
    uint32_t table_addresses[];
} __attribute__ ((packed));

struct ExtendedSystemDescriptorTable {
    struct SystemDescriptionTable_Header header;
    uint64_t table_addresses[];
} __attribute__ ((packed));

struct MultipleAPIC_DescriptionTable {
    struct SystemDescriptionTable_Header header;
    uint32_t local_APIC_address;
    uint32_t flags; //if this is set, need to disable 8259A vectors
    uint8_t interrupt_controllers[];
} __attribute__ ((packed));

struct IOAPIC_Descriptor {
    uint8_t interrupt_controller_type;
    uint8_t length;
    uint8_t id;
    uint8_t reserved;
    uint32_t physical_base_address;
    uint32_t global_system_interrupt_base_interrupt_num;
} __attribute__ ((packed));

//other interrupt controllers that ACPI supports can be implemented here

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

struct InterruptSourceOverride_Entry{
    uint8_t type;
    uint8_t length;
    uint8_t bus;
    uint8_t source;
    uint32_t global_system_interrupt;
    uint16_t flags;
} __attribute__((packed));;

extern uint32_t* lapic_base_address;

void process_acpi_info( struct multiboot_tag_new_acpi * acpi_info_tag);

//used in my interrupt handlers
void write_EOI(uint32_t val);

#endif /*__APIC_H__*/