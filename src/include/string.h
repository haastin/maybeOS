#ifndef __STRING_H__
#define __STRING_H__

#include <stddef.h>

void * memcpy(void* dest_buffer, const void* src_buffer, size_t num_bytes_to_copy);
char * strncpy(char * restrict s1, const char* restrict s2, size_t n);
int strcmp(const char* s1, const char* s2);
void * memset(void* buff, int c, size_t n);
#endif //__STRING_H__