#include <stdint.h>

struct Framebuffer{
    uint8_t * starting_address;
    uint32_t width;
    uint32_t height;
    struct Pixel{
        uint8_t total_bits;
        uint8_t num_red_bits;
        uint8_t red_bits_index;
        uint8_t num_green_bits;
        uint8_t green_bits_index;
        uint8_t num_blue_bits;
        uint8_t blue_bits_index;
    } pixel;
};

extern struct Framebuffer framebuffer;


void print(void);
