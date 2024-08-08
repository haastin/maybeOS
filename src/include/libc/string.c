#include "string.h"
#include <stdint.h>

//copying functions

void * memcpy(void* dest_buffer, const void* src_buffer, size_t num_bytes_to_copy){
    uint8_t * dest = (uint8_t *) dest_buffer;
    const uint8_t * src = (uint8_t *) src_buffer;

    for(size_t index=0; index < num_bytes_to_copy; index++){
        dest[index] = src[index];
    }

    return dest_buffer;
}

void * memmove(void * dest, const void *src, size_t n){
    unsigned char * dest_p = (unsigned char *) dest;
    const unsigned char * src_p = (const unsigned char *) src;

    //the second way to perform this in the else will work no matter what but is slightly less efficient, so see if the first can be used if poss

    //if the destination buffer starts before the source buffer, the source buffer will succesfully copy all of its data to the dest even if it itself gets overwritten
    if(dest < src){
        while(n--){
            *dest_p = *src_p;
            dest_p++;
            src_p++;
        }
    }
    //if the dest buff starts after the source and they overlap, then its possible the source buff could start coping data already written to the dest buff, which inadvertently overwrote itself, and so would then be re-copying bytes it already copied earlier. because of this, we start from the end of the source buff and dest buff and copy backwards so that source buff will have already written its bytes that overlap with the dest buff before those start getting overwritten
    else{
        dest_p += n;
        src_p += n;
        while(n--){
            dest_p--;
            src_p--;
            *dest_p = *src_p; 
        }
    }

    return dest;
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

//concat section

void * strcat(char * restrict s1, char * restrict s2){
    size_t s1_end = strlen(s1)-1;
    size_t s2_len = strlen(s2);
    for(size_t idx=0; idx<s2_len; idx++){
        if(s2[idx] == '\0'){
            break;
        }
        s1[s1_end + idx] = s2[idx];
    }
    return s1;
}

void * strncat(char * restrict s1, char * restrict s2, size_t n){
    size_t s1_end = strlen(s1)-1;
    for(size_t idx=0; idx<n; idx++){
        if(s2[idx] == '\0'){
            break;
        }
        s1[s1_end + idx] = s2[idx];
    }
    s1[s1_end + n] = '\0';
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

//Search functions 

void *memchr(const void * s, int c, size_t n){
    for(size_t idx=0;idx<n;idx++){
        if (((unsigned char *) s)[idx] == (unsigned char)c){
            return s;
        }
    }
    return NULL;
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
    return s_idx;
}