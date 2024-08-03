#include <stddef.h>
#include <stdbool.h>

bool test_strncat(char * s1, char * s2, size_t n, const char * expected_res){
    
    strncat(s1, s2, n);
    if(!strcmp(expected_res, s1)){
        return false;
    }
    else{
        return true;
    }
}