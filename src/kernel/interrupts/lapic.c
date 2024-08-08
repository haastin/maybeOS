#include <stdint.h>
#include <stdbool.h>
#include "string.h"
#include "lapic.h"
#include "driver.h"

#define IA32_LAPIC_MSR_NUM 0x1B

#define LAPIC_EOI_REG_OFFSET 0xB0

#define LAPIC_SPURIOUS_VEC_REG_OFFSET 0xF0
#define SPUR_VEC_REG_APIC_ENABLE_BIT_OFFSET 8

#define LAPIC_APICID_REG_OFFSET 0x20

#define LAPIC_LINT0_REG_OFFSET 0x350
#define LAPIC_LINT1_REG_OFFSET 0x360

#define LVT_INTERRUPT_MASK_BIT_OFFSET 16
#define APIC_MSR_GLOBALENABLE_BIT_OFFSET 11

#define LAPIC_MMIO_LENGTH 0x1000

/**
 * LVT entry config options
 */

#define LVT_ENTRY_DELIV_MODE_FIXED (0 << 8)

#define LVT_ENTRY_DELIV_STATUS_IDLE (0 << 12)
#define LVT_ENTRY_DELIV_STATUS_PENDING (1 << 12)

#define LVT_ENTRY_EDGE_SENSITIVE (0 << 13)
#define LVT_ENTRY_LEVEL_SENSITIVE (1 << 13)

#define LVT_ENTRY_ACTIVE_HIGH (0 << 15)
#define LVT_ENTRY_ACTIVE_LOW (1 << 15)

#define LVT_ENTRY_UNMASKED (0 << 16)
#define LVT_ENTRY_MASKED (1 << 16)

#define LVT_TIMER_MODE_PERIODIC 1


struct IA32_APIC_MSR{
    uint8_t reserved_low;
    unsigned int is_bsp : 1;
    unsigned int reserved_mid : 2;
    unsigned int APIC_global_enable : 1;
    unsigned int LAPIC_base_inunitsofpages : 24;
    unsigned int reserved_high : 28;
} __attribute__((packed));

typedef union{
    struct {
        uint8_t interrupt_vector;
        uint8_t flags;
        uint8_t mask;
        uint8_t reserved;
    } standard;

    struct {
        uint8_t interrupt_vector;
        uint8_t deliv_status_flag;
        uint8_t mask_and_timermode;
        uint8_t reserved;
    } timer;

} LVT_Entry_t;


/**
 * TODO: this should be a proper data structure, not just a pointer where I write mem values; is better for readability and encapsulation
 */
volatile uint32_t* lapic_base_address;

volatile LAPIC_Registers * lapic;

static inline void assign_spurious_interrupt_vector_num(uint8_t interrupt_vector_num);
static struct IA32_APIC_MSR get_lapic_msr(unsigned int msr_num);
static write_lapic_msr(unsigned int msr_num, unsigned int lowerbits_data);
static void enable_lapic(void);
bool is_lapic_present(void);
static void disable_pic(void);
LVT_Entry_t build_lvt_entry(uint8_t vector, bool masked);

void initialize_lapic(){
    
    bool have_lapic = is_lapic_present();
    
    if(have_lapic){
        struct IA32_APIC_MSR lapic_msr = get_lapic_msr(IA32_LAPIC_MSR_NUM);
        lapic_base_address = (volatile uint32_t *)(lapic_msr.LAPIC_base_inunitsofpages << 12);
        lapic = (LAPIC_Registers *) lapic_base_address;
        init_MMIO_device(lapic_base_address, LAPIC_MMIO_LENGTH);
        disable_pic();
        enable_lapic();
        unsigned char spur_vec = 0xFF; //some implementations require a high bit of 1, so this is the recommend vector for spurious interrupt
        assign_spurious_interrupt_vector_num(spur_vec);
        //config_LVT();
    }
}

static void disable_pic(void){
    /**
     * can explicitly mask interrupts in the PIC, OR just mask the LINT pins in the LAPIC's LVT; you lose the option of using the LINT pins for other uses by just masking them and not masking the PIC interrupts, but it is simpler to implement
     */
    LVT_Entry_t lint0 = build_lvt_entry(0, LVT_ENTRY_MASKED);
    LVT_Entry_t lint1 = build_lvt_entry(0, LVT_ENTRY_MASKED);
    lapic->lvt_lint0 = *(uint32_t*) &lint0;
    lapic->lvt_lint1 = *(uint32_t*) &lint1;
    
    //to disable PIC, need to send specified commands to io ports correpsonding to it 
}

static struct IA32_APIC_MSR get_lapic_msr(unsigned int msr_num){
    uint32_t lapic_msr_contents_lowbits;
    uint32_t lapic_msr_contents_highbits;
    asm volatile("mov %2, %%ecx\n" 
                "rdmsr\n" : "=a" (lapic_msr_contents_lowbits), "=d" (lapic_msr_contents_highbits) : "r" (msr_num));
    
    struct IA32_APIC_MSR lapic_msr;
    memcpy(&lapic_msr, &lapic_msr_contents_lowbits, 4);
    memcpy(((char*)&lapic_msr + 4), &lapic_msr_contents_highbits, 4);
    return lapic_msr;
}

static write_lapic_msr(unsigned int msr_num, unsigned int lowerbits_data){
    //TODO: need to check CPUID to see if MSRs are supported before i do these instrs
    asm volatile("mov $0, %%edx\n"
                "wrmsr" : : "c" (msr_num), "a" (lowerbits_data));
}

static inline void enable_lapic(void){

    //global enable APIC MSR
    write_lapic_msr(IA32_LAPIC_MSR_NUM, (1 << APIC_MSR_GLOBALENABLE_BIT_OFFSET));

    //enable in LAPIC itself
    unsigned int enable_apic_bit = (1<<SPUR_VEC_REG_APIC_ENABLE_BIT_OFFSET);
    lapic->svr |= enable_apic_bit;
}

static inline void assign_spurious_interrupt_vector_num(uint8_t interrupt_vector_num){
    //spurious vec does what?
    lapic->svr |= interrupt_vector_num;
}

bool is_lapic_present(void){
    unsigned int res;
    asm volatile("mov $1, %%eax\n"
                "cpuid\n"
                "and $0x100, %%edx": "=d"(res));
    return res > 1? true : false;
}

void write_EOI(){
    //cpu must write here before interrupt handler finishes to indicate CPU has started handling the interrupt; doesnt matter what the val is
    lapic->eoi = 0;
}

/**
 * TODO: make it more configurable later for diff LVT local interrupts
 */
LVT_Entry_t build_lvt_entry(uint8_t vector, bool masked){
    
    LVT_Entry_t lvt_entry;

    lvt_entry.standard.interrupt_vector = vector;
    lvt_entry.standard.reserved = 0;
    lvt_entry.standard.mask = masked;
    lvt_entry.standard.flags = 0 | LVT_ENTRY_DELIV_MODE_FIXED | LVT_ENTRY_DELIV_STATUS_IDLE | LVT_ENTRY_ACTIVE_HIGH | LVT_ENTRY_EDGE_SENSITIVE;

    return lvt_entry;
}

// LVT_Entry_t build_lvt_timer(uint8_t vector, bool masked){
//     LVT_Entry_t lvt_timer;
//     lvt_timer.timer.interrupt_vector = vector;
//     lvt_timer.timer.deliv_status = LVT_ENTRY_DELIV_STATUS_IDLE;
//     lvt_timer.timer.mask = masked;
//     lvt_timer.timer.timer_mode = LVT_TIMER_MODE_PERIODIC;
//     lvt_timer.timer.reserved1 = 0;
//     lvt_timer.timer.reserved2 = 0;
//     lvt_timer.timer.reserved3 = 0;
//     return lvt_timer;
// }

/**
 * TODO: skip for now until i use this set of interrupts
 */
void config_LVT(void){
    LVT_Entry_t lvt_entry;
    //lvt_entry = build_lvt_timer(0x20, LVT_ENTRY_MASKED);
    //lapic->lvt_timer = *(uint32_t*) &lvt_entry;
}
