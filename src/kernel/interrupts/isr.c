#include "VGA_driver.h"
#include "ps2.h"
#include "lapic.h"
#include "asm_helpers.h"
#include <stddef.h>
#include "utils.h"
#include "serial.h"

#define DIVIDE_ERROR 0
#define DOUBLE_FAULT 8
#define GENERAL_PROTECTION_EXCEPTION 13

#define TIMER_IRQ 0x20
#define KEYBOARD_IRQ 0x21

#define DUMMY_ERROR_CODE 0xDEADBEEF

//16 possible digits + prefix and null terminator
#define MAX_HEX_ADDRESS_STR_LEN  19 
#define MAX_MESSAGE_SIZE 1024

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


static void dump_state(struct process_context_t * process_frame){
    char eip_str[MAX_HEX_ADDRESS_STR_LEN];
    num_to_hex_string(process_frame->iret_eip, eip_str);
    char eflags_str[MAX_HEX_ADDRESS_STR_LEN];
    num_to_hex_string(process_frame->iret_eflags, eflags_str);
    char vec_num[MAX_HEX_ADDRESS_STR_LEN];
    num_to_hex_string(process_frame->vector_num,vec_num);

    char eip_msg[MAX_MESSAGE_SIZE] = "\nEIP: ";
    char eflags_msg[MAX_MESSAGE_SIZE] = "\nEflags: ";
    char vec_msg[MAX_MESSAGE_SIZE] = "\nInterrupt Vector: ";

    strcat(eip_msg, eip_str);
    strcat(eflags_msg, eflags_str);
    strcat(vec_msg, vec_num);

    char full_msg[MAX_MESSAGE_SIZE]; 
    strcat(strcat(strcat(full_msg, eip_msg), eflags_msg), vec_msg);

    if(process_frame->error_code != DUMMY_ERROR_CODE){
        
        char error_code[MAX_HEX_ADDRESS_STR_LEN]; 
        num_to_hex_string(process_frame->error_code, error_code);
        
        char error[MAX_MESSAGE_SIZE] = "\nError code: ";
        strcat(error, error_code);

        strcat(full_msg, error); 
    }
    //TODO: dump regs, stack? need better formatting funcs to make this easier
    
    send_string_to_serial_port(full_msg);
}


uint32_t interrupt_handler_dispatcher(struct process_context_t * process_frame){
    
    switch(process_frame->vector_num){
        case KEYBOARD_IRQ:
            uint8_t resp1 = readb_from_keyboard();
            handle_kbd_irq(resp1);
            break;
        default:
            dump_state(process_frame);
            asm volatile ("cli;\nhlt;");
            break;
    }
    write_EOI();
    return process_frame;
}
