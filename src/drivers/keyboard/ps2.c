#include <stdint.h>
#include "asm_helpers.h"
#include "ps2.h"
#include <stdbool.h>
#include "keycodes.h"
#include "terminal_input_parser.h"
#include "utils.h"

/**
 * Commands to the keyboard 
 */

#define RESET_KEYBOARD_CMD 0xFF
#define ECHO_KEYBOARD_CMD 0xEE
#define ENABLE_KEYBORD_SCANNING_CMD 0xF4

/**
 * Commands to the keyboard controller
 */
#define KBD_ACK_RESP 0xFA
#define KBD_ECHO_RESP 0xEE
//dont think there's a diff between an error of 00 and FF, but idk
#define KBD_ERROR_00_RESP 0x00
#define KBD_ERROR_FF_RESP 0xFF
#define KBD_SELF_TEST_PASS_RESP 0xAA
#define KBD_INTERFACE_TEST_RESP 0xAB
#define KBD_MOUSE_INTERFACE_TEST_RESP 0xA9

const char utility_scancodes[] = {
    KBD_ACK_RESP,
    KBD_ECHO_RESP,
    KBD_ERROR_00_RESP,
    KBD_ERROR_FF_RESP,
    KBD_SELF_TEST_PASS_RESP,
    KBD_INTERFACE_TEST_RESP,
    KBD_MOUSE_INTERFACE_TEST_RESP,
};

/**
 * Keyboard Controller Control Register Layout
 */
#define INTERRUPT_PORT_1_BIT_INDEX 0
#define INTERRUPT_PORT_2_BIT_INDEX 1
#define TRANSLATE_SCANCODE_BIT_INDEX 6

/**
 * Scancode 
 */


/**keycodes are mapped to the scancodes from keys they represent */
static const unsigned short ps2_set2_keycode[256] = {
    [0x1C] = KEY_A,
    [0x32] = KEY_B,
    [0x21] = KEY_C,
    [0x23] = KEY_D,
    [0x24] = KEY_E,
    [0x2B] = KEY_F,
    [0x34] = KEY_G,
    [0x33] = KEY_H,
    [0x43] = KEY_I,
    [0x3B] = KEY_J,
    [0x42] = KEY_K,
    [0x4B] = KEY_L,
    [0x3A] = KEY_M,
    [0x31] = KEY_N,
    [0x44] = KEY_O,
    [0x4D] = KEY_P,
    [0x15] = KEY_Q,
    [0x2D] = KEY_R,
    [0x1B] = KEY_S,
    [0x2C] = KEY_T,
    [0x3C] = KEY_U,
    [0x2A] = KEY_V,
    [0x1D] = KEY_W,
    [0x22] = KEY_X,
    [0x35] = KEY_Y,
    [0x1A] = KEY_Z,
    [0x45] = KEY_0_AND_RIGHT_PAREN,
    [0x16] = KEY_1_AND_EXCLAMATION,
    [0x1E] = KEY_2_AND_AT,
    [0x26] = KEY_3_AND_HASH,
    [0x25] = KEY_4_AND_DOLLAR,
    [0x2E] = KEY_5_AND_PERCENT,
    [0x36] = KEY_6_AND_CARET,
    [0x3D] = KEY_7_AND_AMPERSAND,
    [0x3E] = KEY_8_AND_ASTERISK,
    [0x46] = KEY_9_AND_LEFT_PAREN,    
    [0x29] = KEY_SPACEBAR,

    //on 2020 Mac, left side

    [0x0D] = KEY_TAB,
    [0x58] = KEY_CAPS_LOCK,
    [0x12] = KEY_L_SHIFT,
    [0x14] = KEY_L_CTRL,
    [0x1F] = KEY_L_GUI,
    [0x11] = KEY_L_ALT,
    [0x76] = KEY_ESCAPE,
    [0x0E] = KEY_GRAVE_ACCENT_TILDE,


    //on 2020 Mac, right side

    [0x59] = KEY_R_SHIFT,
    [0x27] = KEY_R_GUI,
    
    [0x41] = KEY_COMMA_LESS_THAN,
    [0x49] = KEY_PERIOD_GREATER_THAN,
    [0x4A] = KEY_SLASH_QUESTION,
    [0x4C] = KEY_SEMICOLON_COLON,
    [0x52] = KEY_APOSTROPHE_QUOTE,
    [0x54] = KEY_LEFT_BRACKET,
    [0x5B] = KEY_RIGHT_BRACKET,
    [0x5D] = KEY_BACKSLASH_AND_PIPE, 
    [0x4E] = KEY_MINUS_UNDERSCORE, 
    [0x55] = KEY_EQUAL_PLUS, 
    [0x66] = KEY_DELETE_BACKSPACE,
    [0x5A] = KEY_RETURN_ENTER, 

    //not sure about these below
    //[0x32] = KEY_NON_US_HASH_TILDE,
    [0x05] = KEY_F1,
    [0x06] = KEY_F2,
    [0x04] = KEY_F3,
    [0x0C] = KEY_F4,
    [0x03] = KEY_F5,
    [0x0B] = KEY_F6,
    [0x83] = KEY_F7,
    [0x0A] = KEY_F8,
    [0x01] = KEY_F9,
    [0x09] = KEY_F10,
    [0x78] = KEY_F11,
    [0x07] = KEY_F12,
    //[0x12] = KEY_PRINT_SCREEN, can't handle this key w/ my driver logic
    [0x7E] = KEY_SCROLL_LOCK,
    [0xE1] = KEY_PAUSE,
    [0x77] = KEY_NUM_LOCK_AND_CLEAR,
    //[0x4A] = KEY_KEYPAD_SLASH,
    [0x7C] = KEY_KEYPAD_ASTERISK,
    [0x7B] = KEY_KEYPAD_MINUS,
    [0x79] = KEY_KEYPAD_PLUS,
    //[0x5A] = KEY_KEYPAD_ENTER,
    [0x69] = KEY_KEYPAD_1_AND_END,
    [0x72] = KEY_KEYPAD_2_AND_DOWN_ARROW,
    [0x7A] = KEY_KEYPAD_3_AND_PAGE_DOWN,
    [0x6B] = KEY_KEYPAD_4_AND_LEFT_ARROW,
    [0x73] = KEY_KEYPAD_5,
    [0x74] = KEY_KEYPAD_6_AND_RIGHT_ARROW,
    [0x6C] = KEY_KEYPAD_7_AND_HOME,
    [0x75] = KEY_KEYPAD_8_AND_UP_ARROW,
    [0x7D] = KEY_KEYPAD_9_AND_PAGE_UP,
    [0x70] = KEY_KEYPAD_0_AND_INSERT,
    [0x71] = KEY_KEYPAD_DECIMAL,
    [0x67] = KEY_KEYPAD_EQUAL_SIGN,
};

/**
 * Index into this table when a makecode prefix is detected
 * TODO: size of 0x80 is specified since highest scancode index val goes into 0x70s, so this save space instead of just specifying 0xff. hashmap would still be better.
 */
static const unsigned short ps2_set2_prefixed_keycodes[0x80]= {
    [0x14] = KEY_R_CTRL, 
    [0x11] = KEY_R_ALT, 
    //[0x2F] = KEY_APPS,
    
    [0x70] = KEY_INSERT,
    [0x6C] = KEY_HOME, 
    [0x7D] = KEY_PAGE_UP, 
    [0x71] = KEY_DELETE_FORWARD,
    [0x69] = KEY_END, 
    [0x7A] = KEY_PAGE_DOWN, 
    [0x74] = KEY_RIGHT_ARROW, 
    [0x6B] = KEY_LEFT_ARROW, 
    [0x72] = KEY_DOWN_ARROW, 
    [0x75] = KEY_UP_ARROW, 
};

//will not buffer any scancodes, will handle them immediately. if its a multibyte scancode with prefixes, we kepe track of them with these toggled states
static bool bufferedPrefix;
static bool bufferedBreakCodePrefix;

#define PS_SET2_SCANCODE_PREFIX_1 0xE0
#define PS_SET2_SCANCODE_PREFIX_2 0xF0

//not buffering any scancodes for now 
// uint8_t scancode_buff[RECEIVED_SCANCODE_BUFFER_SIZE];
// uint8_t scancode_buff_curr_index;

static inline bool out_buff_full();
static inline bool in_buff_full();
void deliver_keycode(unsigned short scancode);
static inline void send_kbd_controller_cmd(uint8_t cmd);

unsigned char get_scancode_set_version(){
    return 2;
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
}

bool controller_self_test(void){
    //save and rewrite control reg val just in case the self test resets the keyboard (tutorial recommends)
    uint8_t curr_control_reg = read_kbd_cntl_control_reg();
    send_kbd_controller_cmd(KBD_SELF_TEST_PASS_RESP);
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

    send_kbd_controller_cmd(KBD_INTERFACE_TEST_RESP);
    uint8_t kbd_res = readb_from_keyboard();
    
    if(multiple_interfaces){
    send_kbd_controller_cmd(KBD_MOUSE_INTERFACE_TEST_RESP);
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

static inline unsigned short set2_scancode_to_keycode(uint8_t scancode){
    unsigned short res_keycode;
    if(bufferedPrefix){
        res_keycode = ps2_set2_prefixed_keycodes[scancode];
    }
    else{
        res_keycode = ps2_set2_keycode[scancode];
    }
   
    return res_keycode;
}

static inline bool isBreakCode(uint8_t scancode){
    return scancode == PS_SET2_SCANCODE_PREFIX_2 ? true : false;
}

/**
 * * This func works for every key besides print screen (ie all other keys end after one of the two prefixes)
 */
static inline bool isCompleteKeycode(uint8_t scancode){
    if(scancode == PS_SET2_SCANCODE_PREFIX_1 || scancode == PS_SET2_SCANCODE_PREFIX_2){
        return false;
    }
    return true;
}

static bool isUtilityKeycode(uint8_t scancode){
    //if scancode val corresponds to ack, echo, etc.
    return isInArray(&utility_scancodes, scancode, (size_t) sizeof(utility_scancodes));
}

static void handle_utility_keycode(uint8_t scancode){
    return;
}

void handle_kbd_irq(uint8_t scancode){

    unsigned short keycode = set2_scancode_to_keycode(scancode);

    //scancode is meant for the driver
    if(isUtilityKeycode(scancode)){
        handle_utility_keycode(scancode);
    }
    //a multibyte scancode
    else if(!isCompleteKeycode(scancode)){
        if(isBreakCode(scancode)){
            bufferedBreakCodePrefix = true;

            //any char that is a prefix but not a break code will not have a breakcode prefix after it, so now we can confirm this is a specific case of a breakcode prefix
            bufferedPrefix = false;
        }
        else{
            bufferedPrefix = true;
        }
    }
    else{
        //TODO: need to handle special keys where \xE0 may not be followed by \xF0, but by another keycode, AND this keycode may not be the final keycode for the complete Make scancode sequence to indicate a particular key. For now I will ignore it because it applies to keys not used in a terminal
        // if(bufferedPrefix && potentially_more_bytes_in_full_scancode_seq_for_key){
            
        // }
        
        handle_keycode(keycode, bufferedBreakCodePrefix);
        
        bufferedBreakCodePrefix = false;
        bufferedPrefix = false;
    }
}