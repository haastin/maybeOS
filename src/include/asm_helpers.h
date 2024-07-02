#ifndef __ASM_HELPERS_H__
#define __ASM_HELPERS_H__
inline void sendbyteto_ioport(int port, unsigned char data);
inline unsigned char recvbytefrom_ioport(int port);

#endif /*__ASM_HELPERS_H__*/