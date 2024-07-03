#include "apic.h"
#include "include/asm_helpers.h"
#include "include/string.h"
#include <stddef.h>

//LAPIC 
#define IA32_LAPIC_MSR_NUM 0x1B

#define LAPIC_SPURIOUS_VEC_REG_OFFSET 0xF0
#define SPUR_VEC_REG_APIC_ENABLE_BIT_OFFSET 8

#define LAPIC_APICID_REG_OFFSET 0x20

#define LAPIC_LINT0_REG_OFFSET 0x350
#define LAPIC_LINT1_REG_OFFSET 0x360


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


struct IA32_APIC_MSR{
    uint8_t reserved_low;
    unsigned int is_bsp : 1;
    unsigned int reserved_mid : 2;
    unsigned int APIC_global_enable : 1;
    unsigned int LAPIC_base_inunitsofpages : 24;
    unsigned int reserved_high : 28;
} __attribute__((packed));

//this should prob be malloc, not bss
struct IOAPIC ioapic;

/**
 * TODO: this should be a proper data structure, not just a pointer where I write mem values; is better for readability and encapsulation
 */
uint32_t* lapic_base_address;

static uint32_t get_lapic_msr(unsigned int msr_num);

static void enable_lapic(void);

struct IOAPIC_Descriptor * get_IOAPIC_info(struct multiboot_tag_new_acpi * acpi_info_tag);
static inline void assign_spurious_interrupt_vector_num(uint8_t interrupt_vector_num);
void initialize_irt(void);
union Interrupt_Redirection_Table_Entry build_irt_entry(size_t irq_idx, uint8_t dest_lapic);
void set_irt_entry(unsigned char irq_idx, union Interrupt_Redirection_Table_Entry new_irq_irt_entry);
static inline void select_ioapic_reg(uint8_t reg_num);
static inline void write_ioapic_reg(uint32_t data);

void initialize_ioapic(struct IOAPIC_Descriptor * ioapic_desc){
    ioapic.IO_Register_Selector = ioapic_desc->physical_base_address;
    ioapic.IO_Window_Register = ioapic_desc->physical_base_address + 0x10;
    initialize_irt();
}

void initialize_irt(void){
    for(size_t irq_idx=0; irq_idx<NUM_IOAPIC_IRQs; irq_idx++){
        uint32_t lapic_id = *(lapic_base_address + LAPIC_APICID_REG_OFFSET);
        union Interrupt_Redirection_Table_Entry irq_irt_entry = build_irt_entry(irq_idx, lapic_id);
        set_irt_entry(irq_idx, irq_irt_entry);
    }
}

/**
 * TODO: not sure if these hardcoded values are ok for all connected devices. make this func have these fields be params and set them diff per IRQ
 */
union Interrupt_Redirection_Table_Entry build_irt_entry(size_t irq_idx, uint8_t dest_lapic){
    union Interrupt_Redirection_Table_Entry irq_irt_entry;
    irq_irt_entry.fields.interrupt_vector_num = irq_idx;
    irq_irt_entry.fields.delivery_mode = IOAPIC_FIXED_DELIV_MODE;
    irq_irt_entry.fields.dest_mode = IOAPIC_DEST_SINGLE_LAPIC;
    irq_irt_entry.fields.pin_polarity = IOAPIC_PIN_ACTIVE_HIGH;
    irq_irt_entry.fields.trigger_mode = IOAPIC_INTERRUPT_TRIGGER_ON_EDGE;
    irq_irt_entry.fields.interrupt_mask = IOAPIC_INTERRUPT_UNMASKED;
    irq_irt_entry.fields.destination = dest_lapic;

    return irq_irt_entry;
}

void set_irt_entry(unsigned char irq_idx, union Interrupt_Redirection_Table_Entry new_irq_irt_entry){

    //2 regs per irq, so the irt index for an irq in units of regs will be in multiples of 2
    unsigned char irq_irt_entry_lowbits_reg = IOAPIC_IRT_BASE_REG + irq_idx*2; 
    unsigned char irq_irt_entry_highbits_reg = irq_irt_entry_lowbits_reg+1;

    select_ioapic_reg(irq_irt_entry_lowbits_reg);
    write_ioapic_reg((uint32_t) new_irq_irt_entry.bits);
    
    select_ioapic_reg(irq_irt_entry_highbits_reg);
    write_ioapic_reg((uint32_t) (new_irq_irt_entry.bits >> 32));
}

static inline void select_ioapic_reg(uint8_t reg_num){
    *ioapic.IO_Register_Selector = reg_num;
}

static inline void write_ioapic_reg(uint32_t data){
    *ioapic.IO_Window_Register = data;
}

void initialize_lapic(){
    struct IA32_APIC_MSR * lapic_msr = (struct IA32_APIC_MSR*) get_lapic_msr(IA32_LAPIC_MSR_NUM);
    lapic_base_address = lapic_msr->LAPIC_base_inunitsofpages << 12;
    enable_lapic();
    unsigned char spur_vec = 0xFE; //some implementations require a high bit of 1, so this is the recommend vector for spurious interrupt
    assign_spurious_interrupt_vector_num(spur_vec);
}

static void disable_pic(void){
    /**
     * can explicitly mask interrupts in the PIC, OR just mask the LINT pins in the LAPIC's LVT; you lose the option of using the LINT pins for other uses by just masking them and not masking the PIC interrupts, but it is simpler to implement
     */
    *(lapic_base_address + LAPIC_LINT0_REG_OFFSET) = (1 << 16); //16 is the interrupt mask bit in the LVT for LINT 0 and 1; 1 masks it
    *(lapic_base_address + LAPIC_LINT1_REG_OFFSET) = (1 << 16);
    
    //to disable PIC, need to send specified commands to io ports correpsonding to it 
}

static inline uint32_t get_lapic_msr(unsigned int msr_num){
    uint32_t lapic_msr_contents;
    asm volatile("mov %1, %%ecx\n" 
                "rdmsr\n" : "=a" (lapic_msr_contents) : "r" (msr_num));
    return lapic_msr_contents;
}

static inline void enable_lapic(void){

/**
 * * enable the APICs in the spurious vector reg as opposed to the APIC MSR because disabling APIC in the future if need be has less consequences than doing so in the MSR, so best to just use the same place to enable in that case. Also Intel stated that the global APIC enable in the APIC MSR is not guaranteed to be at the same location in future processors
 * TODO: change this from shifting by certain set of bits to setting a field in a LAPIC data struct
 */
    *(lapic_base_address + LAPIC_SPURIOUS_VEC_REG_OFFSET) |= (1<<SPUR_VEC_REG_APIC_ENABLE_BIT_OFFSET);
}

static inline void assign_spurious_interrupt_vector_num(uint8_t interrupt_vector_num){
    //spurious vec does what?
    *(lapic_base_address + LAPIC_SPURIOUS_VEC_REG_OFFSET) |= interrupt_vector_num;
}

/**
 * TODO: figure out where to properly initialize LAPIC, because in the future I may want to use other parts of ACPI data structures, so will need this to be clearer in functionality
 */
void process_acpi_info(struct multiboot_tag_new_acpi * acpi_info_tag){
    struct IOAPIC_Descriptor * ioapic_desc = get_IOAPIC_info(acpi_info_tag);
    initialize_ioapic(ioapic_desc);
    disable_pic();
    initialize_lapic();
}

struct IOAPIC_Descriptor * get_IOAPIC_info(struct multiboot_tag_new_acpi * acpi_info_tag){
    
    struct RootSystemDescriptorPointer_2 * rsdp = (struct RootSystemDescriptorPointer_2 *) &acpi_info_tag->rsdp;
    
    struct RootSystemDescriptorTable * rsdt = (struct RootSystemDescriptorTable *) rsdp->rsd1_header.RSDtable_address;
    struct ExtendedSystemDescriptorTable *xsdt = (struct ExtendedSystemDescriptorTable *) rsdp->ExtendedSystemDescriptorTable_address;
    
    unsigned int num_tables = (rsdt->header.SDT_length - sizeof(struct SystemDescriptionTable_Header))/4; 
    
    //outer loop: find the MADT in the list of tables of the RSDT or XSDT
    for(size_t table_counter = 0; table_counter < num_tables; table_counter ++){
        
        //get the signature of the table at the current index to see if its the MADT
        char currtable_sig[5];
        strncpy(currtable_sig,((struct SystemDescriptionTable_Header *) rsdt->table_addresses[table_counter])->SDT_sig,4);
        currtable_sig[4] = '\0';

        //the target signature of the APIC
        char * madt_sig = ACPI_MADT_SIG + '\0';
        
        if(strcmp(currtable_sig,madt_sig)==0){
            
            //since this is 32 bit, the XSDT wont be used, so we access rsdt
            struct MultipleAPIC_DescriptionTable * madt = (struct MultipleAPIC_DescriptionTable *) rsdt->table_addresses[table_counter];
            
            unsigned int num_interrupt_controllers = (madt->header.SDT_length - sizeof(struct SystemDescriptionTable_Header))/4;
            
            //inner loop: find the IOAPIC descriptor in the list of interrupt controllers in the MADT
            /**
             * TODO: may need to also parse the Interrupt Source Overrides and keep track of any devices not being at the IRQ expected
             */
            for(size_t interrupt_controller_counter=0; interrupt_controller_counter < num_interrupt_controllers; interrupt_controller_counter += ((struct IOAPIC_Descriptor *) &madt->interrupt_controllers[interrupt_controller_counter])->length){
                
                //check to see if the interrupt controller at the current index has the IOAPIC type
                if(((struct IOAPIC_Descriptor *) &madt->interrupt_controllers[interrupt_controller_counter])->interrupt_controller_type == ACPI_MADT_IOAPIC){
                    
                    //initialize the global ioapic
                    return (struct IOAPIC_Descriptor *) &madt->interrupt_controllers[interrupt_controller_counter];
                }
            }

        }
    }
}

void write_EOI(){
    //cpu must write here before interrupt handler finishes to indicate CPU has started handling the interrupt; doesnt matter what the val is
    *(lapic_base_address + LAPIC_EOI_REG_OFFSET) = 0;
}

/**
 * TODO: skip for now until i use this set of interrupts
 */
void config_LVT(void){
}
