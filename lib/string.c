#include "string.h"
#include <stdint.h>

void * memcpy(void* dest_buffer, const void* src_buffer, size_t num_bytes_to_copy){
    uint8_t * dest = (uint8_t *) dest_buffer;
    const uint8_t * src = (uint8_t *) src_buffer;

    for(size_t index=0; index < num_bytes_to_copy; index++){
        dest[index] = src[index];
    }

    return dest_buffer;
}