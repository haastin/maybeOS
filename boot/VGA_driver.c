#include "VGA_driver.h"

#define FRAMEBUFFER_ADDRESS_OFFSET 8
#define FRAMEBUFFER_HEIGHT_OFFSET 24
#define FRAMEBUFFER_WIDTH_OFFSET 20
#define FRAMEBUFFER_BITSPERPIXEL_OFFSET 28

struct Framebuffer framebuffer;

void print(void){
    
    for(int x = 0; x < framebuffer.width; x++){
        for(int y=0; y< framebuffer.height; y++){
            *(framebuffer.starting_address + (y*framebuffer.width + x)*3 + framebuffer.pixel.red_bits_index/8) = 0xFF;
            }
        }
}

//decide which font you want to use
//the index for cols and rows will change because bitmaps take units of pixels
