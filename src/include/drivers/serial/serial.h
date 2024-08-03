#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <stdbool.h>

//TODO: this section is for serial ports that adhere to the RS-232 standard; should somehow make this more abstract to be able to pivot between diff serial port standards
//-------

//TODO: the port nums may actually depend on the system being run on, so really they shouldn't be hardcoded
#define COM1 0x3f8
#define COM2 0x2f8
#define COM3 0x3e8
#define COM4 0x2e8

//when reading
#define RECEIEVE_REG 0
//when writing
#define TRANSMIT_REG 0

#define DLAB_SET_DIVIOR_LO_REG 0
#define DLAB_SET_DIVIOR_HI_REG 1

#define INTERRUPT_ENABLE_REG 1

//when reading
#define INTERRUPT_ID_REG 2
//when writing
#define FIFO_CTL_REGS 2

#define LINE_CTL_REG 3
#define MODEM_CTL_REG 4
#define LINE_STATUS_REG 5
#define MODEM_STATUS_REG 6
#define SCRATCH_REG 7

//Operations

//Interrupt Enable Reg
#define DISABLE_INTERRUPTS 0

//Line Ctl Reg 
#define ENABLE_DLAB (1 << 7)
#define FRAME_8N1 0x3 //8 ascii bits, no parity bit, one stop bit per frame

//Special DLAB-enabled Regs
#define DEFAULT_BAUD_DIVISOR_LO 3
#define DEFAULT_BAUD_DIVISOR_HI 0

//FIFO Reg
#define ENABLE_FIFO_BUFFERS 1
#define CLEAR_TRANSMIT_BUFFER (1 << 2)
#define CLEAR_RECEIVE_BUFFER (1 << 1)

//there are 4 predefined values to choose from based on the 2 bits allocated for this field. 00 = 1, 01 = 4, 10 = 8, 11 = 14. By default I choose 14, but add more macros or change the val below to switch it
#define SET_NUM_BYTES_BEFORE_INTERRUPT_TRIGGERED (3 << 6)

//Modem Ctl Reg
#define ENABLE_OUT1_PIN (1 << 2) 
#define ENABLE_IRQ_LINE (1 << 3)
#define READY_TO_ESTABLISH_CONNECTION 1
#define REQUEST_TO_SEND_DATA (1 << 1)
#define SET_LOOPBACK_MODE (1 << 4)


//-----

bool init_serial_port(void);

void sendb_to_serial_port(char byte);

void send_string_to_serial_port(char *str);

#endif /*__SERIAL_H__*/

