#ifndef __ASM_HELPERS_H__
#define __ASM_HELPERS_H__

void sendb_toport(int port, unsigned char data);

unsigned char recvb_fromport(unsigned short port);

#endif /*__ASM_HELPERS_H__*/