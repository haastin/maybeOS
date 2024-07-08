#include "data_structures.h"
#include <limits.h>


void init_array(Array_t *array){
    array->curr_size = 0;
    array->max_capacity = INT32_MAX;
}