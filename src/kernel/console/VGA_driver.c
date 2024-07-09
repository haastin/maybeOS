#include "VGA_driver.h"
#include <stddef.h>
#include <stdbool.h>
#include "font.h"
#include "string.h"


typedef enum display_mode{
    TEXT_MODE,
    GRAPHICS_MODE
} DisplayMode;

struct Framebuffer framebuffer;

//TODO: package these fields below into some higher level data structure that abstracts the console details

Font curr_font;

DisplayMode display_mode;

//can still plot individual background pixels and characters diff colors, but besides special cases will stick with a default font
Color background_color;
Color font_color;

uint32_t cursor_x;
uint32_t cursor_y;

uint32_t left_boundary_x;
uint32_t right_boundary_x;
uint32_t top_boundary_y;
uint32_t bottom_boundary_y;



static void update_cursor_positions(void);
static uint32_t calculate_offset_from_framebuffer_starting_address(uint32_t cursor_x, uint32_t cursor_y);


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

static inline uint32_t calculate_offset_from_framebuffer_starting_address(uint32_t cursor_x, uint32_t cursor_y){
    return (framebuffer.width*cursor_y + cursor_x)*framebuffer.pixel.total_bits/8;
}

static inline uint8_t* get_curr_framebuffer_address(void){
    return framebuffer.starting_address + calculate_offset_from_framebuffer_starting_address(cursor_x, cursor_y);
}

static inline unsigned char* get_char_bitmap_starting_address(uint32_t unicode_character_index){
    unsigned char* char_bitmap = curr_font.glyph + unicode_character_index*curr_font.bytesperglyph;
    return char_bitmap;
}

/**
 * Each bit in a bitmap represents an entire pixel, which in my QEMU VM is 3 bytes per pixel, so iteratign through each bit in a bitmap needs to advance the pointer by num bytes per pixel
 */
void terminal_putchar(const uint32_t unicode_character_index, const Color char_color, const Color background_color){

    //the bitmap for the char index passed
    unsigned char* char_glyph = get_char_bitmap_starting_address(unicode_character_index);
    
    //indicates which bit in the bitmap we are on
    unsigned char bitmap_idx = 0;
    
    uint8_t* framebuffer_starting_address = get_curr_framebuffer_address();
    
    for(size_t char_y = 0; char_y < curr_font.height; char_y++){
        for(size_t char_x = 0; char_x < curr_font.width; char_x++){
            
            //keep track of what pixel we are currently drawing 
            uint8_t* curr_pixel_address = framebuffer_starting_address + calculate_offset_from_framebuffer_starting_address((uint32_t)char_x, (uint32_t)char_y);
            
            bool isChar = isCharBit(char_glyph, bitmap_idx);
            
            if(!isChar){
                if(background_color == DONT_SET){
                    //do nothing
                }
                else{
                    set_pixel_RGB(curr_pixel_address, background_color);
                }
            }
            else{
                set_pixel_RGB(curr_pixel_address, char_color);
            }

            bitmap_idx++;
        }
    }
    update_cursor_positions();
}

void terminal_printstr(const char * string, size_t num_chars){
    for(size_t idx=0; idx<num_chars; idx++){
        terminal_putchar(string[idx], font_color, DONT_SET);
    }
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

void set_background_color(Color new_background){
    
    background_color = new_background;
    uint8_t * framebuff_start = framebuffer.starting_address;
    for(size_t x = 0; x < framebuffer.width; x++){
        for(size_t y=0; y< framebuffer.height; y++){
            uint8_t* fb_curraddy = framebuff_start + calculate_offset_from_framebuffer_starting_address((uint32_t)x, (uint32_t)y);
            set_pixel_RGB(fb_curraddy, background_color);
        }
    }
}

void set_cursors_to_default_pos(void){
    cursor_x = 5;
    cursor_y = 2;
    left_boundary_x = 5;
    right_boundary_x = framebuffer.width - 5;
    top_boundary_y = 2;
    bottom_boundary_y = framebuffer.height - 2;
}

void initialize_shell_UI(){
    set_background_color(GRAY);
    set_cursors_to_default_pos();
    char * welcome_msg = "Welcome to maybeOS! My name is LALALALALALALALALALALAALALALALALALALALALALAALALALALALALALALALALALALALALALALALALALALALALALALAAL and it rhymbes with AUstin!";
    terminal_printstr(welcome_msg, strlen(welcome_msg));
}

void print(void){
return;
}

//decide which font you want to use
//the index for cols and rows will change because bitmaps take units of pixels
