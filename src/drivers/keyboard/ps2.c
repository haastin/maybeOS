#include <stdint.h>
#include "asm_helpers.h"
#include "ps2.h"
#include <stdbool.h>

/**
 * Commands to the IO ports 
 */

#define RESET_KEYBOARD_CMD 0xFF
#define ECHO_KEYBOARD_CMD 0xEE
#define ENABLE_KEYBORD_SCANNING_CMD 0xF4

/**
 * 
 */

#define KBD_CONT_SELF_TEST_CMD 0xAA
#define KBD_CONT_KBD_INTERFACE_TEST_CMD 0xAB
#define KBD_CONT_MOUSE_INTERFACE_TEST_CMD 0xA9

/**
 * Keyboard Controller Control Register Layout
 */
#define INTERRUPT_PORT_1_BIT_INDEX 0
#define INTERRUPT_PORT_2_BIT_INDEX 1
#define TRANSLATE_SCANCODE_BIT_INDEX 6

//
#define RECEIVED_SCANCODE_BUFFER_SIZE 255

uint8_t scancode_buff[RECEIVED_SCANCODE_BUFFER_SIZE];
uint8_t scancode_buff_curr_index;

static inline bool out_buff_full();
static inline bool in_buff_full();

static inline void send_kbd_controller_cmd(uint8_t cmd);

unsigned char get_scancode_set_version(){
    return 1;
}

uint8_t * clear_input_buffer(void){
    uint8_t buff[256];
    uint8_t buff_index = 0;
    bool isfull = in_buff_full();
    while(isfull){
        buff[buff_index % 256] = readb_from_keyboard();
        buff_index++;
    }
    buff[buff_index] = '\0';
    return buff;
}

void sendb_to_keyboard(uint8_t byte_to_keyboard){
    sendb_toport(KEYBOARD_IOPORT,byte_to_keyboard);
}

uint8_t readb_from_keyboard(){
    return recvb_fromport(KEYBOARD_IOPORT);
}

uint8_t get_keyboardcontroller_status_reg(){
    return recvb_fromport(KEYBOARD_CONTROLLER_IOPORT);
}

void reset_keyboard(void){
    sendb_to_keyboard(RESET_KEYBOARD_CMD);
}

static inline bool out_buff_full(){
    struct ps2_status_reg curr = *(struct ps2_status_reg*) get_keyboardcontroller_status_reg();
    return curr.out_buff_full; 
}

static inline bool in_buff_full(){
    struct ps2_status_reg curr = *(struct ps2_status_reg*) get_keyboardcontroller_status_reg();
    return curr.in_buff_full; 
}

uint8_t ping_keyboard(){
    sendb_to_keyboard(ECHO_KEYBOARD_CMD);
    uint8_t status = get_keyboardcontroller_status_reg();
    uint8_t res = readb_from_keyboard();
    return res;
}

static inline void send_kbd_controller_cmd(uint8_t cmd){
    sendb_toport(KEYBOARD_CONTROLLER_IOPORT,cmd);
}

void inline send_kbd_controller_cmd_with_param(uint8_t cmd, uint8_t parameter){
    sendb_toport(KEYBOARD_CONTROLLER_IOPORT,cmd);
    sendb_toport(KEYBOARD_IOPORT, parameter);
}

static inline void disable_ps2devices(){
    send_kbd_controller_cmd(KBD_CONT_DISABLE_MOUSE_INTERFACE_CMD);
    send_kbd_controller_cmd(KBD_CONT_DISABLE_KEYBOARD_INTERFACE_CMD);
}

void initialize_ps2keyboard(void){
    scancode_buff_curr_index = 0;
    disable_ps2devices(); //make sure nothing can be sent to the kbd cont
    readb_from_keyboard(); //flush the kbd cont input buffer
    initialize_kbd_control_reg();
    bool test_pass = controller_self_test();
    while(!test_pass){
        controller_self_test();
    }
    
    bool multiple_interfaces = ps2_contoller_has_multiple_ports();

    unsigned char interface_avail = test_kbd_cont_interfaces(multiple_interfaces);
    
    if(interface_avail == -1){
        //TODO: write to a log or something here
    }
    else{
        enable_keyboard_interfaces(interface_avail);
        enable_interrupt_ports(interface_avail);
    }
    //uint8_t status_reg = get_keyboardcontroller_status_reg();
    //uint8_t ctl_reg = read_kbd_cntl_control_reg();
    
    reset_keyboard();
    sendb_to_keyboard(ENABLE_KEYBORD_SCANNING_CMD);
    while(true){}
    uint8_t status_reg_a = get_keyboardcontroller_status_reg();
    uint8_t ctl_reg_a = read_kbd_cntl_control_reg();
   
    uint8_t resp1 = readb_from_keyboard();
    uint8_t resp2 = readb_from_keyboard();
}

bool controller_self_test(void){
    //save and rewrite control reg val just in case the self test resets the keyboard (tutorial recommends)
    uint8_t curr_control_reg = read_kbd_cntl_control_reg();
    send_kbd_controller_cmd(KBD_CONT_SELF_TEST_CMD);
    write_kbd_ctl_control_reg(curr_control_reg);
    uint8_t kbd_cont_response = readb_from_keyboard();
    if (kbd_cont_response == 0x55){
        return true;
    }
    return false;
}

void initialize_kbd_control_reg(){
    uint8_t kbd_cont_control_reg = read_kbd_cntl_control_reg();
    kbd_cont_control_reg &= ~(1 << INTERRUPT_PORT_1_BIT_INDEX);
    kbd_cont_control_reg &= ~(1 << INTERRUPT_PORT_2_BIT_INDEX);
    kbd_cont_control_reg &= ~(1 << TRANSLATE_SCANCODE_BIT_INDEX);
    write_kbd_ctl_control_reg(kbd_cont_control_reg);
}

static inline uint8_t read_kbd_cntl_control_reg(void){
    send_kbd_controller_cmd(KBD_CONT_READ_CNTL_REG_CMD);
    uint8_t kbd_cont_control_reg = readb_from_keyboard();
    return kbd_cont_control_reg;
}

static inline void write_kbd_ctl_control_reg(uint8_t new_control_reg_val){
    send_kbd_controller_cmd_with_param(KBD_CONT_WRITE_CNTL_REG_CMD, new_control_reg_val);
}

bool ps2_contoller_has_multiple_ports(void){
    uint8_t ctl_reg = read_kbd_cntl_control_reg();
    bool res = ctl_reg & (1 << 5);
    return res;
}

/**
 * TODO: instead of just checking if the res is 0, check the error code and return it somehow to a log or other func
 */
unsigned char test_kbd_cont_interfaces(bool multiple_interfaces){
    
    uint8_t mouse_res = -1; //assume only one port

    send_kbd_controller_cmd(KBD_CONT_KBD_INTERFACE_TEST_CMD);
    uint8_t kbd_res = readb_from_keyboard();
    
    if(multiple_interfaces){
    send_kbd_controller_cmd(KBD_CONT_MOUSE_INTERFACE_TEST_CMD);
    mouse_res = readb_from_keyboard();
    }

    if(kbd_res == 0 & mouse_res == 0){
        return 2;
    }
    else if(kbd_res == 0){
        return 0;
    }
    else if(mouse_res == 0){
        return 1;
    }
    else{
        return -1;
    }
}

void enable_keyboard_interfaces(unsigned char avail_interfaces_code){

    switch(avail_interfaces_code){
        case(0):
            send_kbd_controller_cmd(KBD_CONT_ENABLE_KEYBOARD_INTERFACE_CMD);
            break;
        case(1):
            send_kbd_controller_cmd(KBD_CONT_ENABLE_MOUSE_INTERFACE_CMD);
            break;
        case(2):
            send_kbd_controller_cmd(KBD_CONT_ENABLE_KEYBOARD_INTERFACE_CMD);
            send_kbd_controller_cmd(KBD_CONT_ENABLE_MOUSE_INTERFACE_CMD);
            break;
    }
}

void enable_interrupt_ports(unsigned char avail_interfaces_code){
    uint8_t kbd_ctl_reg = read_kbd_cntl_control_reg();
    switch(avail_interfaces_code){
        case(0):
            kbd_ctl_reg |= (1 << INTERRUPT_PORT_1_BIT_INDEX);
            break;
        case(1):
            kbd_ctl_reg |= (1 << INTERRUPT_PORT_2_BIT_INDEX);
            break;
        case(2):
            kbd_ctl_reg |= (1 << INTERRUPT_PORT_1_BIT_INDEX);
            kbd_ctl_reg |= (1 << INTERRUPT_PORT_2_BIT_INDEX);
            break;
    }
    write_kbd_ctl_control_reg(kbd_ctl_reg);
}

void place_scancode_in_buff(uint8_t scancode){
    
    scancode_buff[scancode_buff_curr_index % RECEIVED_SCANCODE_BUFFER_SIZE] = scancode;
    scancode_buff_curr_index++;

    
}