#include <stdbool.h>
#include "ctype.h"

bool isprint(int c) {
    return (c >= 0x20 && c <= 0x7E);
}

bool isdigit(int c) {
    return (c >= '0' && c <= '9');
}

bool isalpha(int c) {
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

bool isalnum(int c) {
    return isdigit(c) || isalpha(c);
}

