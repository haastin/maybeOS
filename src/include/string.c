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

char * strcpy(char* restrict s1, const char* restrict s2){

}

char * strncpy(char * restrict s1, const char* restrict s2, size_t n){

    for(size_t c=0; c<n; c++){
        if(s2[c] =='\0'){
            memset(s1 + c, '\0', n-c);
            break;
        }
        else{
            s1[c] = s2[c];
        }
    }
    return s1;
}

//comparison section of ISO C defintion of string.h

int strcmp(const char* s1, const char* s2){
    
    while(*s1 != '\0' && *s2 != '\0'){
        
        if (*s1 != *s2){
            return *s1 > *s2 ? 1 : -1;
        }
        s1++;
        s2++;
    }
    if (*s1 == '\0' && *s2 == '\0') {
        return 0;
    } else if (*s1 == '\0') {
        return -1;
    } else {
        return 1;
    }
}

//Functions in Misc section in ISO C
void * memset(void* buff, int c, size_t n){
    unsigned char *buff_ptr = (unsigned char*) buff;
    for(size_t i=0; i<n; i++){
        buff_ptr[i] = (unsigned char) c;
    }
    return buff;
}

size_t strlen(const char * s){
    size_t s_idx = 0;
    while(s[s_idx] != '\0'){
        s_idx++;
    }
    size_t res = s_idx + 1;
    return res;
}