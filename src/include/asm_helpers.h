#ifndef __ASM_HELPERS_H__
#define __ASM_HELPERS_H__

extern unsigned int _kernel_vm_offset;

#define phys_addy(virt_addy) ((virt_addy) - _kernel_vm_offset)

void sendb_toport(int port, unsigned char data);

unsigned char recvb_fromport(unsigned short port);

#endif /*__ASM_HELPERS_H__*/