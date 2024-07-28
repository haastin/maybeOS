#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <stddef.h>

/**
 * TODO: get userspace infra set up
 */
typedef struct{
    unsigned long start_address;
    size_t length;
    unsigned int flags;
    vm_area_t * next_area; 
} vm_area_t;

typedef struct {

    vm_area_t * vm_areas;
    unsigned long pgd;

} process_t;

#endif /*__PROCESS_H__*/