#include "VGA_driver.h"
#include "ps2.h"
#include "lapic.h"
#include "asm_helpers.h"
#include <stddef.h>

#define DIVIDE_ERROR 0
#define DOUBLE_FAULT 8
#define GENERAL_PROTECTION_EXCEPTION 13

#define TIMER_IRQ 0x20
#define KEYBOARD_IRQ 0x21

/**
 * Although the process context may differ depending on if the interrupt was triggered in userspace vs kernelspace, we treat it like it doesn't matter, as the only difference is the usermode ESP and SS pushed to the stack, which won't be used by a handler anyways
 */
struct process_context_t {

/*the general purpose regs pushed by pusha*/
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    //pushl is used to push both of these vals, so they are 32bit on the stack
    unsigned int vector_num;
    unsigned int error_code;
    
/*the iret frame the cpu automatically adds to the stack when the interrupt is called*/
    uint32_t iret_eip;
    uint32_t iret_cs;
    uint32_t iret_eflags;
    uint32_t iret_esp;
    uint32_t iret_ss;
};


static void vector_0_handler(void){
    set_background_color(BLUE);
}


uint32_t interrupt_handler_dispatcher(struct process_context_t * process_frame){
    
    switch(process_frame->vector_num){
        case DIVIDE_ERROR:
            vector_0_handler();
            break;
        case DOUBLE_FAULT:
            set_background_color(GREEN);
            break;
        case GENERAL_PROTECTION_EXCEPTION:
            set_background_color(RED);
            break;
        case TIMER_IRQ:
            set_background_color(MAGENTA);
            break;
        case KEYBOARD_IRQ:
            uint8_t resp1 = readb_from_keyboard();
            handle_kbd_irq(resp1);
            break;

        
    }
    write_EOI();
    return process_frame;
}
