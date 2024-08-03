#include "utils.h"
#include <stddef.h>
#include "kheap.h"

size_t hex_num_digit_count(unsigned long num){
    unsigned long res = num;
    unsigned long count = 0;
    do{
        res /= 16;
        count++;
    } while(res != 0);

    return count;
}

char num_to_ascii(unsigned char num){
    if(num >= 0 && num < 10){
        return '0' + num;
    }
    else if(num >= 10 && num < 16){
        return 'A' + (num-10);
    }
}

char * num_to_hex_string(unsigned long num, void * buff){
    size_t num_digits = hex_num_digit_count(num);
    size_t str_num_size = num_digits + 3; //the num plus the prefix and null terminator
    char * res_num = (char *) buff;
    res_num[0] = '0';
    res_num[1] = 'x';
    res_num[str_num_size-1] = '\0';

    unsigned long res = num;
    unsigned char remainder = res % 16;
    res /= 16;
    size_t last_digit_starting_idx = str_num_size-2;

    for(unsigned char digit_idx=0;digit_idx<num_digits;digit_idx++){
        res_num[last_digit_starting_idx - digit_idx] = num_to_ascii(remainder);
        remainder = res % 16;
        res /= 16;
    }
    return res_num;
}