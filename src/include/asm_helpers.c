void sendb_toport(int port, unsigned char data){
    asm volatile("outb %1, %0" : :"d" ((unsigned short)port), "a" (data));
}

unsigned char recvb_fromport(unsigned short port){
    unsigned char res_data = 0;
    asm volatile("inb %1, %0" : "=a" (res_data) : "d" (port) );
    return res_data;
}