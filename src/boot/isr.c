#include "VGA_driver.h"
#include <stddef.h>

#define DIVIDE_ERROR 0
#define DOUBLE_FAULT 8
#define GENERAL_PROTECTION_EXCEPTION 13

struct process_context_t {

    unsigned int vector_num;
    unsigned int error_code;

/*the general purpose regs pushed by pusha*/
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    
/*the iret frame the cpu automatically adds to the stack when the interrupt is called*/
    uint32_t iret_eip;
    uint32_t iret_cs;
    uint32_t iret_eflags;
    uint32_t iret_esp;
    uint32_t iret_ss;
};

static void vector_0_handler(){
    set_background_color(BLUE);
}

void interrupt_handler_dispatcher(struct process_context_t * process_frame){
    
    switch(process_frame->vector_num){
        case DIVIDE_ERROR:
            vector_0_handler();
            break;
        case DOUBLE_FAULT:
            set_background_color(GREEN);
            break;
        case GENERAL_PROTECTION_EXCEPTION:
            set_background_color(PURPLE);
        case 33:
            set_background_color(YELLOW);
            break;

        
    }
}
