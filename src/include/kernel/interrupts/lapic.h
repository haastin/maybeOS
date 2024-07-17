#include <stdint.h>

typedef struct {
    volatile uint32_t reserved1 __attribute__((aligned(16)));       // Offset 0x000 - 0x008
    volatile uint32_t reserved2 __attribute__((aligned(16)));       // Offset 0x000 - 0x008
    volatile uint32_t ID __attribute__((aligned(16)));                 // Offset 0x020
    volatile uint32_t version __attribute__((aligned(16)));            // Offset 0x030
    volatile uint32_t reserved3 __attribute__((aligned(16)));       // Offset 0x040 - 0x050
    volatile uint32_t reserved4 __attribute__((aligned(16)));
    volatile uint32_t reserved5 __attribute__((aligned(16)));
    volatile uint32_t reserved6 __attribute__((aligned(16)));
    volatile uint32_t tpr __attribute__((aligned(16)));                // Offset 0x080
    volatile uint32_t apr __attribute__((aligned(16)));                // Offset 0x090
    volatile uint32_t ppr __attribute__((aligned(16)));                // Offset 0x0A0
    volatile uint32_t eoi __attribute__((aligned(16)));                // Offset 0x0B0
    volatile uint32_t rrd __attribute__((aligned(16)));                // Offset 0x0C0
    volatile uint32_t ldr __attribute__((aligned(16)));                // Offset 0x0D0
    volatile uint32_t dfr __attribute__((aligned(16)));                // Offset 0x0E0
    volatile uint32_t svr __attribute__((aligned(16)));                // Offset 0x0F0
    volatile uint32_t isr1 __attribute__((aligned(16)));             // Offset 0x100 - 0x11C
    volatile uint32_t isr2 __attribute__((aligned(16)));
    volatile uint32_t isr3 __attribute__((aligned(16))); 
    volatile uint32_t isr4 __attribute__((aligned(16))); 
    volatile uint32_t isr5 __attribute__((aligned(16))); 
    volatile uint32_t isr6 __attribute__((aligned(16))); 
    volatile uint32_t isr7 __attribute__((aligned(16))); 
    volatile uint32_t isr8 __attribute__((aligned(16)));  
    volatile uint32_t tmr1 __attribute__((aligned(16)));             // Offset 0x180 - 0x19C
    volatile uint32_t tmr2 __attribute__((aligned(16)));
    volatile uint32_t tmr3 __attribute__((aligned(16)));
    volatile uint32_t tmr4 __attribute__((aligned(16)));
    volatile uint32_t tmr5 __attribute__((aligned(16)));
    volatile uint32_t tmr6 __attribute__((aligned(16)));
    volatile uint32_t tmr7 __attribute__((aligned(16)));
    volatile uint32_t tmr8 __attribute__((aligned(16)));
    volatile uint32_t irr1 __attribute__((aligned(16)));             // Offset 0x200 - 0x21C
    volatile uint32_t irr2 __attribute__((aligned(16)));
    volatile uint32_t irr3 __attribute__((aligned(16)));
    volatile uint32_t irr4 __attribute__((aligned(16)));
    volatile uint32_t irr5 __attribute__((aligned(16)));
    volatile uint32_t irr6 __attribute__((aligned(16)));
    volatile uint32_t irr7 __attribute__((aligned(16)));
    volatile uint32_t irr8 __attribute__((aligned(16)));
    volatile uint32_t esr __attribute__((aligned(16)));                // Offset 0x280
    volatile uint32_t reserved7 __attribute__((aligned(16)));       // Offset 0x290 - 0x2B0
    volatile uint32_t reserved8 __attribute__((aligned(16)));
    volatile uint32_t reserved9 __attribute__((aligned(16)));
    volatile uint32_t reserved10 __attribute__((aligned(16)));
    volatile uint32_t reserved11 __attribute__((aligned(16)));
    volatile uint32_t reserved12 __attribute__((aligned(16)));
    volatile uint32_t lvt_cmci __attribute__((aligned(16)));
    volatile uint32_t icr_low __attribute__((aligned(16)));            // Offset 0x300
    volatile uint32_t icr_high __attribute__((aligned(16)));           // Offset 0x310
    volatile uint32_t lvt_timer __attribute__((aligned(16)));          // Offset 0x320
    volatile uint32_t lvt_thermal __attribute__((aligned(16)));        // Offset 0x330
    volatile uint32_t lvt_performance __attribute__((aligned(16)));    // Offset 0x340
    volatile uint32_t lvt_lint0 __attribute__((aligned(16)));          // Offset 0x350
    volatile uint32_t lvt_lint1 __attribute__((aligned(16)));          // Offset 0x360
    volatile uint32_t lvt_error __attribute__((aligned(16)));          // Offset 0x370
    volatile uint32_t timer_initial_count __attribute__((aligned(16))); // Offset 0x380
    volatile uint32_t timer_current_count __attribute__((aligned(16))); // Offset 0x390
    volatile uint32_t reserved13 __attribute__((aligned(16)));       // Offset 0x3A0 - 0x3B0
    volatile uint32_t reserved14 __attribute__((aligned(16)));
    volatile uint32_t reserved15 __attribute__((aligned(16)));
    volatile uint32_t reserved16 __attribute__((aligned(16)));
    volatile uint32_t timer_divide_config __attribute__((aligned(16))); // Offset 0x3E0
    volatile uint32_t reserved17 __attribute__((aligned(16)));          // Offset 0x3F0
} LAPIC_Registers;


extern volatile uint32_t* lapic_base_address;

extern volatile LAPIC_Registers * lapic;

void initialize_lapic();

//used in my interrupt handlers
void write_EOI();
