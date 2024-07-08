#include <stdint.h>
#include <stddef.h>
#include "acpi.h"
#include "multiboot2.h"
#include "string.h"
#include "data_structures.h"

#define ACPI_MADT_IOAPIC 1
#define ACPI_MADT_ISO 2

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
    uint8_t revision;
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

struct RootSystemDescriptorPointer_2 * rsdp;
struct RootSystemDescriptorTable * rsdt;
struct MultipleAPIC_DescriptionTable * madt;

/**
 * TODO: make my own acpi structures to eventually free the acpi mem?
 */
void process_acpi_info(struct multiboot_tag_new_acpi * acpi_info_tag){
    rsdp = (struct RootSystemDescriptorPointer_2 *) &acpi_info_tag->rsdp;
    rsdt = (struct RootSystemDescriptorTable *) rsdp->rsd1_header.RSDtable_address;
    madt = (struct MultipleAPIC_DescriptionTable *) get_system_descriptor_table(MADT);
}

void * get_system_descriptor_table(SystemDescriptorTable table){

    //TODO: need some logic to distinguish between xsdt and rsdt, assumign rsdt for now

    char * target_table_sig;
    switch(table){
        case(MADT):
            target_table_sig= ACPI_MADT_SIG + '\0';
            break;
    }

    unsigned int num_tables = (rsdt->header.SDT_length - sizeof(struct SystemDescriptionTable_Header))/4; 
    
    for(size_t table_counter = 0; table_counter < num_tables; table_counter ++){
        
        //get the signature of the table at the current table index
        char currtable_sig[5];
        strncpy(currtable_sig,((struct SystemDescriptionTable_Header *) rsdt->table_addresses[table_counter])->SDT_sig,4);
        currtable_sig[4] = '\0';

        if(strcmp(currtable_sig,target_table_sig)==0){
            return rsdt->table_addresses[table_counter];
        }
    }
    return NULL;
}
/**
 * TODO: get C++ up. having too much trouble bootstrapping basic types in a less clean way than it would be in a class.
 * TODO: THIS IS BUGGED
 * *because there can be multiple interrupt controllers present, we return the address for each of them*
 */
Array_t get_interruptcontroller(InterruptController intr_ctlr){

    Array_t inter_ctlrs;
    init_array(&inter_ctlrs);
    
    unsigned int entry_bytes = madt->header.SDT_length - sizeof(struct MultipleAPIC_DescriptionTable);

    uint8_t target_intr_ctlr_type;
    switch(intr_ctlr){
        case(IOAPIC):
            target_intr_ctlr_type = ACPI_MADT_IOAPIC;
            break;
        case(ISO):
            target_intr_ctlr_type = ACPI_MADT_ISO;
            break;
        default:
            target_intr_ctlr_type = 0xFF;
    }

    //dont know exactly what tables will be present
    for(size_t idx=0; idx < entry_bytes; idx += ((struct IOAPIC_Desc *) &madt->interrupt_controllers[idx])->length){
        
        uint8_t curr_entry_intr_ctl_type = ((struct IOAPIC_Desc *) &madt->interrupt_controllers[idx])->interrupt_controller_type;

        if(curr_entry_intr_ctl_type == target_intr_ctlr_type){
            inter_ctlrs.data[inter_ctlrs.curr_size] = &madt->interrupt_controllers[idx];
            inter_ctlrs.curr_size++;
        }
    }
    return inter_ctlrs;

}