#include "VGA_driver.h"
#include <stddef.h>
#include <stdbool.h>
#include "font.h"

struct Framebuffer framebuffer;

typedef enum display_mode{
    TEXT_MODE,
    GRAPHICS_MODE
} DisplayMode;

//assumes R G B have 0, 1, and 2 byte indices, respectively
typedef enum {
    BLACK       = 0x000000, // 0x00 (R) 0x00 (G) 0x00 (B)
    WHITE       = 0xFFFFFF, // 0xFF (R) 0xFF (G) 0xFF (B)
    RED         = 0xFF0000, // 0xFF (R) 0x00 (G) 0x00 (B)
    GREEN       = 0x00FF00, // 0x00 (R) 0xFF (G) 0x00 (B)
    BLUE        = 0x0000FF, // 0x00 (R) 0x00 (G) 0xFF (B)
    YELLOW      = 0xFFFF00, // 0xFF (R) 0xFF (G) 0x00 (B)
    MAGENTA     = 0xFF00FF, // 0xFF (R) 0x00 (G) 0xFF (B)
    CYAN        = 0x00FFFF, // 0x00 (R) 0xFF (G) 0xFF (B)
    GRAY        = 0x808080, // 0x80 (R) 0x80 (G) 0x80 (B)
    MAROON      = 0x800000, // 0x80 (R) 0x00 (G) 0x00 (B)
    OLIVE       = 0x808000, // 0x80 (R) 0x80 (G) 0x00 (B)
    DARK_GREEN  = 0x008000, // 0x00 (R) 0x80 (G) 0x00 (B)
    PURPLE      = 0x800080, // 0x80 (R) 0x00 (G) 0x80 (B)
    TEAL        = 0x008080, // 0x00 (R) 0x80 (G) 0x80 (B)
    NAVY        = 0x000080, // 0x00 (R) 0x00 (G) 0x80 (B)
    SILVER      = 0xC0C0C0,  // 0xC0 (R) 0xC0 (G) 0xC0 (B)

    RED_INDEX = 16,
    GREEN_INDEX = 8,
    BLUE_INDEX = 0
} Color;

/*hash map that maps font names to PSF * ? 
need to implement/find a freestanding hashmap implementation and implement malloc*/

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

void terminal_putchar(uint32_t unicode_character_index, Color char_color, Color background_color){

    //the bitmap for the char index passed
    unsigned char* char_glyph = (unsigned char*)(curr_font.glyph + unicode_character_index*curr_font.bytesperglyph);
    
    //indicates which bit in the bitmap we are on
    unsigned char shift_counter = 0;
    
    //we preserve the starting location of our cursor pointer x and y
    uint32_t starting_cursorx = cursor_x;
    uint32_t starting_cursory = cursor_y;
    uint8_t* starting_address = framebuffer.starting_address + (framebuffer.width*cursor_y + cursor_x)*framebuffer.pixel.total_bits/8;
    
    for(size_t char_y = 0; char_y < curr_font.height; char_y++){
        for(size_t char_x = 0; char_x < curr_font.width; char_x++){

            unsigned char bytes_shifted = shift_counter/8;
            char_glyph += bytes_shifted;
            shift_counter %= 8;
           //isolate the MSB of the bitmap since that is our current bit 
            unsigned char adjusted_bitmap = (*char_glyph << shift_counter);          
            unsigned char curr_bit_val = (adjusted_bitmap & (1 << 7));
            bool isChar = curr_bit_val ? true : false;
            
            uint8_t* curr_pixel_address = starting_address + (framebuffer.width*char_y + char_x)*framebuffer.pixel.total_bits/8;

            char curr_pixel_red;
            char curr_pixel_green;
            char curr_pixel_blue;
            if(curr_bit_val == 1){
                isChar = true;
            }
           
            if (isChar){
                curr_pixel_red = ((char_color >> RED_INDEX) & 0xFF);
                curr_pixel_green = ((char_color >> BLUE_INDEX) & 0xFF);
                curr_pixel_blue = ((char_color >> GREEN_INDEX) & 0xFF);
            }
            else{
                curr_pixel_red = ((background_color >> RED_INDEX) & 0xFF);
                curr_pixel_green = ((background_color >> BLUE_INDEX) & 0xFF);
                curr_pixel_blue = ((background_color >> GREEN_INDEX) & 0xFF);
            }
            
            *(curr_pixel_address + framebuffer.pixel.red_bits_index/8) = curr_pixel_red;
            *(curr_pixel_address + framebuffer.pixel.blue_bits_index/8) = curr_pixel_green;
            *(curr_pixel_address + framebuffer.pixel.green_bits_index/8) = curr_pixel_blue;

            shift_counter++;
        }
    }
    cursor_x += curr_font.width;
}

/*Always assumes only a single character has been written*/
static void update_cursor_positions(void){

     if(right_boundary_x - cursor_x < curr_font.width){
        cursor_x = left_boundary_x;
        if(cursor_y == bottom_boundary_y){
           //move screen up
        }
        else{
            cursor_y++;
        }
    }
    else{
        cursor_x += curr_font.width;
    }
}

// static inline bool need_newline_to_write(unsigned int num_chars_to_write){
//     if(right_boundary_x - cursor_x < num_chars_to_write*curr_psf_font_file->font_width){
//         return true;
//     }
//     return false;
// }

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


void print(void){
    unsigned char test_letter = 'a';
    //terminal_putchar(test_letter, PURPLE, OLIVE);
    //terminal_putchar(55, RED, TEAL);
    for(size_t x = 0; x < 5; x++){
        terminal_putchar(48, PURPLE, TEAL);
        for(size_t y=0; y< framebuffer.height; y++){
            
            //*(framebuffer.starting_address + (y*framebuffer.width + x)*3 + framebuffer.pixel.green_bits_index/8) = 0xFF;
            }
        }
}

//decide which font you want to use
//the index for cols and rows will change because bitmaps take units of pixels
