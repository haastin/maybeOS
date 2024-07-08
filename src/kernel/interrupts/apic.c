#include "apic.h"
#include "acpi.h"
#include "lapic.h"
#include "asm_helpers.h"
#include "string.h"
#include <stddef.h>
#include <stdbool.h>
#include "data_structures.h"

//IOAPIC
#define NUM_IOAPIC_IRQs 24
#define IOAPIC_IRT_BASE_REG 0x10

//IOAPIC IRT entry field vals
#define IOAPIC_FIXED_DELIV_MODE 0x0

#define IOAPIC_DEST_SINGLE_LAPIC 0
#define IOAPIC_DEST_MULTIPLE_LAPICS 1

#define IOAPIC_PIN_ACTIVE_HIGH 0
#define IOAPIC_PIN_ACTIVE_LOW 1

#define IOAPIC_INTERRUPT_TRIGGER_ON_EDGE 0
#define IOAPIC_INTERRUPT_TRIGGER_ON_LEVEL 1

#define IOAPIC_INTERRUPT_UNMASKED 0
#define IOAPIC_INTERRUPT_MASKED 1

#define USER_INTERRUPT_VECTOR_BEGIN 0x20


struct IOAPIC ioapic;


void initialize_irt(void);
union Interrupt_Redirection_Table_Entry build_irt_entry(size_t irq_idx, uint8_t dest_lapic);
void set_irt_entry(unsigned char irq_idx, union Interrupt_Redirection_Table_Entry new_irq_irt_entry);
static inline void select_ioapic_reg(uint8_t reg_num);
static inline void write_ioapic_reg(uint32_t data);


void initialize_ioapic(){

    Array_t ioapic_desciptors = get_interruptcontroller(IOAPIC);
    struct IOAPIC_Desc * ioapic_desc = ioapic_desciptors.data[0];
    ioapic.IO_Register_Selector = ioapic_desc->physical_base_address;
    ioapic.IO_Window_Register = ioapic_desc->physical_base_address + 0x10;

    initialize_irt();
}

void initialize_interrupt_source_overrides(void){
    Array_t iso_array; 
    iso_array = get_interruptcontroller(ISO);
    struct InterruptSourceOverride_Desc iso[iso_array.curr_size];
    for(size_t iso_idx=0;iso_idx<iso_array.curr_size;iso_idx++){
        iso[iso_idx] = *(struct InterruptSourceOverride_Desc *)iso_array.data[iso_idx];
    }

}

void initialize_irt(void){
    
    volatile uint32_t lapic_id = lapic->ID;
    unsigned char keyboard_irq = 1;
    union Interrupt_Redirection_Table_Entry irq_irt_entry = build_irt_entry(keyboard_irq, lapic_id);
    set_irt_entry(keyboard_irq, irq_irt_entry);

    // for(size_t irq_idx=0; irq_idx<NUM_IOAPIC_IRQs; irq_idx++){

    //     volatile uint32_t lapic_id = lapic->ID;
    //     union Interrupt_Redirection_Table_Entry irq_irt_entry = build_irt_entry(irq_idx, lapic_id);
    //     set_irt_entry(irq_idx, irq_irt_entry);
    // }

}

/**
 * TODO: not sure if these hardcoded values are ok for all connected devices. make this func have these fields be params and set them diff per IRQ
 */
union Interrupt_Redirection_Table_Entry build_irt_entry(size_t irq_idx, uint8_t dest_lapic){

    union Interrupt_Redirection_Table_Entry irq_irt_entry;
    irq_irt_entry.fields.interrupt_vector_num = USER_INTERRUPT_VECTOR_BEGIN + irq_idx;
    irq_irt_entry.fields.delivery_mode = IOAPIC_FIXED_DELIV_MODE;
    irq_irt_entry.fields.dest_mode = IOAPIC_DEST_SINGLE_LAPIC;
    irq_irt_entry.fields.pin_polarity = IOAPIC_PIN_ACTIVE_HIGH;
    irq_irt_entry.fields.trigger_mode = IOAPIC_INTERRUPT_TRIGGER_ON_EDGE;
    irq_irt_entry.fields.interrupt_mask = IOAPIC_INTERRUPT_UNMASKED;
    irq_irt_entry.fields.destination = dest_lapic;
    irq_irt_entry.fields.reserved_low = 0;
    irq_irt_entry.fields.reserved_high = 0;

    return irq_irt_entry;
}

void set_irt_entry(unsigned char irq_idx, union Interrupt_Redirection_Table_Entry new_irq_irt_entry){

    //2 regs per irq, so the irt index for an irq in units of regs will be in multiples of 2
    unsigned char irq_irt_entry_lowbits_reg = IOAPIC_IRT_BASE_REG + irq_idx*2; 
    unsigned char irq_irt_entry_highbits_reg = irq_irt_entry_lowbits_reg+1;

    uint32_t res_low = read_ioapic_reg(irq_irt_entry_lowbits_reg);
    uint32_t res_high =read_ioapic_reg(irq_irt_entry_highbits_reg);

    select_ioapic_reg(irq_irt_entry_lowbits_reg);
    write_ioapic_reg((uint32_t) new_irq_irt_entry.bits);

    res_low = read_ioapic_reg(irq_irt_entry_lowbits_reg);
    
    select_ioapic_reg(irq_irt_entry_highbits_reg);
    uint32_t irt_upper_bits = (uint32_t)(new_irq_irt_entry.bits >> 32);
    write_ioapic_reg(irt_upper_bits);

    res_high =read_ioapic_reg(irq_irt_entry_highbits_reg);
    
}

static inline void select_ioapic_reg(uint8_t reg_num){
    *ioapic.IO_Register_Selector = reg_num;
}

static inline void write_ioapic_reg(uint32_t data){
    *ioapic.IO_Window_Register = data;
}

uint32_t read_ioapic_reg(uint8_t reg_num){
    select_ioapic_reg(reg_num);
    return *ioapic.IO_Window_Register;
}
