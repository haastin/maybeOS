#ifndef __DRIVER_H__
#define __DRIVER_H__

#include "vmm.h"
#include "driver.h"

bool init_MMIO_device(unsigned long starting_physical_address, size_t size);

#endif /*__DRIVER_H__*/

