#include "asm_helpers.h"
inline void sendbyteto_ioport(int port, unsigned char data){
    asm volatile("outb %0, %1" : :"r" (port), "r" (data));
}
inline unsigned char recvbytefrom_ioport(int port){
    unsigned char res_data = 0;
    asm volatile("inb %0, %1" : "=r" (res_data) : "r" (port) );
    return res_data;
}