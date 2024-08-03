
#include "serial.h"
#include "asm_helpers.h"
#include <stdbool.h>
#include "string.h"
#include <stddef.h>

//TODO: this should prob not be hardcoded, assuming COM1 will be the port we always use
bool init_serial_port(void){

    sendb_toport(COM1+INTERRUPT_ENABLE_REG, DISABLE_INTERRUPTS);

    //set bits per second (baud) rate
    sendb_toport(COM1+LINE_CTL_REG, ENABLE_DLAB); 
    sendb_toport(COM1+DLAB_SET_DIVIOR_LO_REG, DEFAULT_BAUD_DIVISOR_LO);
    sendb_toport(COM1+DLAB_SET_DIVIOR_HI_REG, DEFAULT_BAUD_DIVISOR_HI);

    //set the configs of each frame (also clears DLAB)
    sendb_toport(COM1+LINE_CTL_REG, FRAME_8N1); 

    //configure the fifo buffers that will hold the data we send to/receieve from the "peripheral" device
    sendb_toport(COM1+FIFO_CTL_REGS, ENABLE_FIFO_BUFFERS | CLEAR_TRANSMIT_BUFFER | CLEAR_RECEIVE_BUFFER | SET_NUM_BYTES_BEFORE_INTERRUPT_TRIGGERED);

    //prepare to send data to the "peripheral" attached to the serial port 
    sendb_toport(COM1+MODEM_CTL_REG, ENABLE_IRQ_LINE | READY_TO_ESTABLISH_CONNECTION | REQUEST_TO_SEND_DATA);

    //---test if initialization was successful---
    sendb_toport(COM1+MODEM_CTL_REG, SET_LOOPBACK_MODE);
    //send test byte, which should appear in the receive buffer as well since loopback is set
    sendb_toport(COM1+TRANSMIT_REG, 0xAB);
    unsigned char res = recvb_fromport(COM1+RECEIEVE_REG);
    if(res != 0xAB){
        return false;
    }
    
    //test was successful, so put the system back in normal state with all pins enabled
    sendb_toport(COM1+MODEM_CTL_REG, ENABLE_IRQ_LINE|READY_TO_ESTABLISH_CONNECTION|REQUEST_TO_SEND_DATA|ENABLE_OUT1_PIN);

    return true;
}

void sendb_to_serial_port(char byte){
    sendb_toport(COM1+TRANSMIT_REG, byte);
}

void send_string_to_serial_port(char * str){
    size_t str_size = strlen(str);
    for(size_t letter=0; letter<str_size; letter++){
        sendb_to_serial_port(str[letter]);
    }
}