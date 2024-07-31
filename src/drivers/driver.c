
#include <stddef.h>
#include "vmm.h"
#include "driver.h"

/**
 * This assumes that every MMIO device wants to have an identity map 
 */
bool init_MMIO_device(unsigned long starting_physical_address, size_t size){
    //vmalloc needs to be fully operational at this point
    void * virtual_mapping = vmalloc_request_virtual_address(starting_physical_address, size, VM_AREA_MMIO | VM_AREA_WRITE);
    if(!virtual_mapping){
        //TODO: printk device init failed
        return false;
    }
    return true;
}