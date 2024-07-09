#include <stdbool.h>
/**
 * IO Port Locations on i440FX chipset, which QEMU i386 VM uses
 */
#define KEYBOARD_IOPORT 0x60
#define KEYBOARD_CONTROLLER_IOPORT 0x64

#define KBD_CONT_READ_CNTL_REG_CMD 0x20
#define KBD_CONT_WRITE_CNTL_REG_CMD 0x60

#define KBD_CONT_DISABLE_MOUSE_INTERFACE_CMD 0xA7
#define KBD_CONT_ENABLE_MOUSE_INTERFACE_CMD 0xA8

#define KBD_CONT_DISABLE_KEYBOARD_INTERFACE_CMD 0xAD
#define KBD_CONT_ENABLE_KEYBOARD_INTERFACE_CMD 0xAE

#define ENABLE_KEYBOARD_CMD 0xF4


struct ps2_status_reg{
    unsigned int out_buff_full : 1;
    unsigned int in_buff_full : 1;
    unsigned int sys : 1;
    unsigned int A2 : 1;
    unsigned int inhibit : 1;
    unsigned int transmit_timeout : 1;
    unsigned int recv_timeout : 1;
    unsigned int parity_error : 1;
} __attribute__((packed));

uint8_t * clear_input_buffer(void);
//maybe another file will need these?
void sendb_to_keyboard(uint8_t byte_to_keyboard);
uint8_t readb_from_keyboard();
uint8_t get_keyboardcontroller_status_reg();
void reset_keyboard(void);

uint8_t ping_keyboard();
void initialize_ps2keyboard(void);
bool controller_self_test(void);
static uint8_t read_kbd_cntl_control_reg(void);
static void write_kbd_ctl_control_reg(uint8_t new_control_reg_val);
bool ps2_contoller_has_multiple_ports(void);
unsigned char test_kbd_cont_interfaces(bool multiple_interfaces);
void place_scancode_in_buff(uint8_t scancode);


void send_kbd_controller_cmd_with_param(uint8_t cmd, uint8_t parameter);