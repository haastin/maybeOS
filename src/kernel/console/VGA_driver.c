#include "VGA_driver.h"
#include <stddef.h>
#include <stdbool.h>
#include "font.h"

struct Framebuffer framebuffer;

typedef enum display_mode{
    TEXT_MODE,
    GRAPHICS_MODE
} DisplayMode;


#define RED_INDEX 16
#define GREEN_INDEX 8
#define BLUE_INDEX 0

DisplayMode display_mode;

uint32_t cursor_x;
uint32_t cursor_y;

uint32_t left_boundary_x;
uint32_t right_boundary_x;
uint32_t top_boundary_y;
uint32_t bottom_boundary_y;

Font curr_font;

#define DEFAULT_FONT gr928b_8x16

static void update_cursor_positions(void);

static void set_font(Font_Name font_name){

    //would prefer a hashmap here instead, we'll see
    switch(font_name){
        case(gr928b_8x16):
            initialize_font(font_name, &curr_font);
            break;
    }
}

static inline void set_display_mode(DisplayMode mode){
    display_mode = mode;
}

bool isCharBit(unsigned char* char_bitmap, unsigned char bits_shifted){

    //by shifting the glyph pointer we can have a font width or height larger than the bit size of the underlying platform
    unsigned char bytes_shifted = bits_shifted/8;
    char_bitmap += bytes_shifted;
    bits_shifted %= 8;

     //isolate the MSB of the bitmap since that is our current bit 
    unsigned char adjusted_bitmap = (*char_bitmap << bits_shifted);
    return (adjusted_bitmap & (1 << (curr_font.width-1))) ? true : false;
}

void set_pixel_RGB(uint8_t * pixel_address, Color pixel_color){
    unsigned char curr_pixel_red = ((pixel_color >> RED_INDEX) & 0xFF);
    unsigned char curr_pixel_green = ((pixel_color >> BLUE_INDEX) & 0xFF);
    unsigned char curr_pixel_blue = ((pixel_color >> GREEN_INDEX) & 0xFF);

    *(pixel_address + framebuffer.pixel.red_bits_index/8) = curr_pixel_red;
    *(pixel_address + framebuffer.pixel.blue_bits_index/8) = curr_pixel_green;
    *(pixel_address + framebuffer.pixel.green_bits_index/8) = curr_pixel_blue;
}

void terminal_putchar(uint32_t unicode_character_index, Color char_color, Color background_color){

    //the bitmap for the char index passed
    unsigned char* char_glyph = (unsigned char*)(curr_font.glyph + unicode_character_index*curr_font.bytesperglyph);
    
    //indicates which bit in the bitmap we are on
    unsigned char shift_counter = 0;
    
    uint8_t* framebuffer_starting_address = framebuffer.starting_address + (framebuffer.width*cursor_y + cursor_x)*framebuffer.pixel.total_bits/8;
    
    for(size_t char_y = 0; char_y < curr_font.height; char_y++){
        for(size_t char_x = 0; char_x < curr_font.width; char_x++){
            
            //increment a copy of the framebuffer address; the real one will be adjusted later 
            uint8_t* curr_pixel_address = framebuffer_starting_address + (framebuffer.width*char_y + char_x)*framebuffer.pixel.total_bits/8;
            
            if(char_color == DONT_SET){
                //used to not 
                set_pixel_RGB(curr_pixel_address, background_color);
            }
            else if(background_color  == DONT_SET){
                set_pixel_RGB(curr_pixel_address, char_color);
            }
            else{
                Color desired_color = isCharBit(char_glyph, shift_counter) ? char_color : background_color;
                set_pixel_RGB(curr_pixel_address, desired_color);
            }
            
            shift_counter++;
        }
    }
    update_cursor_positions();
}

/*Always assumes only a single character has been written*/
static void update_cursor_positions(void){

    if(right_boundary_x - cursor_x < curr_font.width){
        cursor_x = left_boundary_x;
        if(bottom_boundary_y- cursor_y < curr_font.height){
           //move screen up a line
        }
        else{
            cursor_y += curr_font.height;
        }
    }
    else{
        cursor_x += curr_font.width;
    }
}

void initialize_framebuffer_attributes(struct multiboot_tag_framebuffer * framebuffer_info){
    framebuffer.starting_address = (uint8_t *)(uint32_t)framebuffer_info->common.framebuffer_addr;
    framebuffer.width = framebuffer_info->common.framebuffer_width;
    framebuffer.height = framebuffer_info->common.framebuffer_height;

    framebuffer.pixel.total_bits = framebuffer_info->common.framebuffer_bpp;
    framebuffer.pixel.num_red_bits = framebuffer_info->framebuffer_red_mask_size;
    framebuffer.pixel.red_bits_index = framebuffer_info->framebuffer_red_field_position;
    framebuffer.pixel.num_green_bits = framebuffer_info->framebuffer_green_mask_size;
    framebuffer.pixel.green_bits_index = framebuffer_info->framebuffer_green_field_position;
    framebuffer.pixel.num_blue_bits = framebuffer_info->framebuffer_blue_mask_size;
    framebuffer.pixel.blue_bits_index = framebuffer_info->framebuffer_blue_field_position;

    set_display_mode(GRAPHICS_MODE);

    //change this to be some other index for either a logo i will print or a terminal
    cursor_x = 0;
    cursor_y = 0;
    
    /*sometimes we may want to shorten the length of a line to not be the entire screen, so in that
    case we will have boundaries. they are global vars here which implies i will only have one interface
    for a user for my OS- this may change*/
    left_boundary_x = 0;
    right_boundary_x = framebuffer.width;
    top_boundary_y = 0;
    bottom_boundary_y = framebuffer.height;

    set_font(DEFAULT_FONT);
}

void set_background_color(Color background){
    
    for(size_t x = 0; x < framebuffer.width; x++){
        for(size_t y=0; y< framebuffer.height; y++){
            uint8_t* fb_curraddy = framebuffer.starting_address + (framebuffer.width*y + x)*framebuffer.pixel.total_bits/8;
            set_pixel_RGB(fb_curraddy, background);
            }
    }
}

void print(void){
    set_background_color(MAROON);
    terminal_putchar("z", PURPLE, OLIVE);
    
}

//decide which font you want to use
//the index for cols and rows will change because bitmaps take units of pixels
