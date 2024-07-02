#include <stdint.h>
#include "include/asm_helpers.h"

/**
 * IO Port Locations on i440FX chipset, which QEMU i386 VM uses
 */
#define PS2_KEYBOARD_BYTE_IOPORT 0x60
#define PS2_KEYBOARD_CMD_IOPORT 0x64

/**
 * Commands to the IO ports 
 */


/**
 * 
 */
#define RECEIVED_SCANCODE_BUFFER_SIZE 255

uint8_t scancode_buff[RECEIVED_SCANCODE_BUFFER_SIZE];
uint8_t scancode_buff_curr_index;



void keyboard_irq_handler(){
    //recvbytefrom_ioport(PS2_KEYBOARD_BYTE_IOPORT);
    
}




unsigned char get_scancode_set_version(){

}