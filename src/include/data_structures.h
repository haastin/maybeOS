#ifndef __DATA_STRUCTURES_H__
#define __DATA_STRUCTURES_H__

#include <stddef.h>
#include <stdint.h>

typedef struct{
    void ** data;
    size_t curr_size;
    size_t max_capacity;
} Array_t;

void init_array(Array_t *array);

#endif /*__DATA_STRUCTURES_H__*/