#ifndef __UTILS_H__
#define __UTILS_H__

#include "paging.h"
#include <stdint.h>

extern unsigned int _kernel_vm_offset;

#define phys_addy(virt_addy) ((uint32_t)(virt_addy) - ((uint32_t)(&_kernel_vm_offset)))

#define get_page_above_pfn(address) (((address) + (PAGE_SIZE-1))/PAGE_SIZE)

#define round_up_to_nearest_page(address) (((uintptr_t)(address) + (PAGE_SIZE-1)) & PAGE_FRAME_BITMASK)

#define get_pfn(address) ((address)/PAGE_SIZE)

// static inline void sendb_toport(int port, unsigned char data){
//     asm volatile("outb %1, %0" : :"d" ((unsigned short)port), "a" (data));
// }

// static inline unsigned char recvb_fromport(unsigned short port){
//     unsigned char res_data = 0;
//     asm volatile("inb %1, %0" : "=a" (res_data) : "d" (port) );
//     return res_data;
// }

// void sendb_toport(int port, unsigned char data);

// unsigned char recvb_fromport(unsigned short port);

#endif /*__UTILS_H__*/