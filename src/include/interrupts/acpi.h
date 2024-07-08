#ifndef __ACPI_H__
#define __ACPI_H__
#include "multiboot2.h"
#include <stdint.h>
#include "data_structures.h"

struct IOAPIC_Desc {
    uint8_t interrupt_controller_type;
    uint8_t length;
    uint8_t id;
    uint8_t reserved;
    uint32_t physical_base_address;
    uint32_t global_system_interrupt_base_interrupt_num;
}__attribute__ ((packed));

struct InterruptSourceOverride_Desc{
    uint8_t type;
    uint8_t length;
    uint8_t bus;
    uint8_t source;
    uint32_t global_system_interrupt;
    uint16_t flags;
} __attribute__ ((packed));

typedef union {
    struct IOAPIC_Desc ioapic_desc;
    struct InterruptSourceOverride_Desc intr_src_override_desc;
} InterruptController_t;


//other interrupt controllers that ACPI supports can be implemented here

typedef enum{
    MADT,
} SystemDescriptorTable;

typedef enum{
    LAPIC,
    IOAPIC,
    ISO,
} InterruptController;

void process_acpi_info(struct multiboot_tag_new_acpi * acpi_info_tag);
void * get_system_descriptor_table(SystemDescriptorTable table);
Array_t get_interruptcontroller(InterruptController intr_ctlr);

#endif /*__ACPI_H__*/