#include "VGA_driver.h"
#include <stddef.h>
#include <stdbool.h>
#include "font.h"
#include "string.h"
#include "driver.h"


typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} __attribute__((packed)) Pixel_t;

// typedef struct {
//     unsigned int font_width;
//     unsigned int font_height;
//     Pixel_t * pixel[DEFAULT_FONT_HEIGHT*DEFAULT_FONT_WIDTH];
// } __attribute__((packed)) Text_Cell_t;


struct framebuffer framebuffer;

Pixel_t ** framebuffer_map;

//TODO: package these fields below into some higher level data structure that abstracts the console details

Font curr_font;


//can still plot individual background pixels and characters diff colors, but besides special cases will stick with a default font
Color background_color;
Color cursor_color;
Color font_color;


uint32_t global_cursorX;
uint32_t global_cursorY;

uint32_t left_boundary_x;
uint32_t right_boundary_x;
uint32_t top_boundary_y;
uint32_t bottom_boundary_y;



struct text_mode{
    /*These values are calculated based on the dimensions of the current font*/
    unsigned int num_rows;
    unsigned int num_cols;
    unsigned int text_cell_width;
    unsigned int text_cell_height;
    //need malloc. later on this can be changed so that in a full GUI with multiple windows the memory the text buffer occupies won't necessarily be sequential, and that it can be resized; for now, i just assume it will take the full screen
    //TODO: change this and below to text cell structs
    uint8_t ** text_buffer;

    //points to the top left pixel of any given text cell. cannot represent an entir
    uint8_t * text_cursor;
} txt_mode;


static void update_cursor_positions(void);
static uint32_t calculate_offset_from_framebuffer_starting_address(uint32_t global_cursorX, uint32_t global_cursorY);
static void set_row_color(uint8_t * row, Color color);

static void init_text_mode_info(void);

void set_pixel_RGB(uint8_t * pixel_address, Color pixel_color){
    unsigned char curr_pixel_red = ((pixel_color >> RED_INDEX) & 0xFF);
    unsigned char curr_pixel_blue = ((pixel_color >> BLUE_INDEX) & 0xFF);
    unsigned char curr_pixel_green = ((pixel_color >> GREEN_INDEX) & 0xFF);

    *(pixel_address + framebuffer.pixel.red_bits_index/8) = curr_pixel_red;
    *(pixel_address + framebuffer.pixel.blue_bits_index/8) = curr_pixel_blue;
    *(pixel_address + framebuffer.pixel.green_bits_index/8) = curr_pixel_green;
}

static inline uint32_t calculate_offset_from_framebuffer_starting_address(uint32_t global_cursorX, uint32_t global_cursorY){
    return (framebuffer.width*global_cursorY + global_cursorX)*framebuffer.pixel.total_bits/8;
}

static inline uint8_t* get_curr_framebuffer_address(void){
    return framebuffer.starting_address + calculate_offset_from_framebuffer_starting_address(global_cursorX, global_cursorY);
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
    
    for(size_t char_y = 0; char_y < curr_font.height; char_y++){
        
        for(size_t char_x = 0; char_x < curr_font.width; char_x++){
            
            //keep track of what pixel we are currently drawing 
            uint8_t* curr_pixel_address = txt_mode.text_cursor + calculate_offset_from_framebuffer_starting_address((uint32_t)char_x, (uint32_t)char_y);
            
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
                if(char_color == DONT_SET){
                    //do nothing
                }
                else{
                    set_pixel_RGB(curr_pixel_address, char_color);
                }
            }

            bitmap_idx++;
        }
    }
}

void terminal_printstr(const char * string, size_t num_chars){
    for(size_t idx=0; idx<num_chars; idx++){
        terminal_putchar(string[idx], font_color, DONT_SET);
        //update_cursor_positions();
        inc_textCursor();
        plot_text_cursor();
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

    unsigned long framebuffer_mmio_length = (framebuffer.width*framebuffer.height)*(framebuffer_info->common.framebuffer_bpp/8);
    init_MMIO_device(framebuffer.starting_address, framebuffer_mmio_length);
    init_MMIO_device(0xDEADBEEF, 0xB00B);
    set_font(DEFAULT_FONT);
    init_text_mode_info();
    plot_text_cursor();
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

// static inline void set_cursorX_left_boundary(unsigned int pixel_offset_from_left){
//     left_boundary_x = pixel_offset_from_left;
// }

// static inline void set_cursorX_right_bounary(unsigned int pixel_offset_from_right){
//     right_boundary_x = framebuffer.width - pixel_offset_from_right;
// }

// static inline void set_cursorY_top_boundary(unsigned int pixel_offset_from_top){
//     top_boundary_y = pixel_offset_from_top;
// }

// static inline void set_cursorY_bottom_bounary(unsigned int pixel_offset_from_bottom){
//     bottom_boundary_y = framebuffer.height - pixel_offset_from_bottom;
// }

// static inline void set_cursorX(unsigned int new_x){
//     global_cursorX = new_x;
// }

// static inline void set_cursorY(unsigned int new_y){
//     global_cursorY = new_y;
// }

// void set_screen_env(unsigned int cursorX, unsigned int cursorY, unsigned int pixel_offset_from_left, unsigned int pixel_offset_from_right, unsigned int pixel_offset_from_top, unsigned int pixel_offset_from_bottom){
//     set_cursorX(cursorX);
//     set_cursorY(cursorY);
//     set_cursorX_left_boundary(pixel_offset_from_left);
//     set_cursorX_right_bounary(pixel_offset_from_right);
//     set_cursorY_top_boundary(pixel_offset_from_top);
//     set_cursorY_bottom_bounary(pixel_offset_from_bottom);
// }

/**
 * * It is up to a calling program if the cursor should be moved right or not; the default behavior will be to move it right unless its the very bottom right cell, upon which it won't move.*/
bool inc_textCursor(void){

    //if col is at the very bottom right of the bounded box
    if(txt_mode.text_cursor != &txt_mode.text_buffer[txt_mode.num_rows-1][txt_mode.num_cols-1]){
        txt_mode.text_cursor++;
    }
}


/**
 * *It is up to the calling program if the cursor should be moved left or not; the default behavior is that the cursor will be moved left until it reaches the evry top left of the screen, upon which it won't move left anymore.
 */
bool dec_textCursor(void){
    if(txt_mode.text_cursor != &txt_mode.text_buffer[0][0]){
        txt_mode.text_cursor--;
    }
    else{
        return false;
    }
}

/**
 * * When all rows are shifted up by one, the top row will be discarded. The very bottom row would remain the same as the row above (since it is copied there), so it is initialized to just be the a blank row matching the formatting of the rest of the text buffer.
 */
void shift_text_screen_up_one_row(void){
    for(size_t row_in_textbuff=0; row_in_textbuff < txt_mode.num_rows-1; row_in_textbuff++){
        /**
         * num_cols is in units of text cells
         * text_cell_width is in units of pixels/text cell
         * pixel_size is in units of bytes/pixel
         * 
         * the result of this calculation will thus be in bytes
         */
        size_t num_bytes_in_text_buff_row = txt_mode.num_cols*txt_mode.text_cell_width*framebuffer.pixel.pixel_size;
        memcpy(txt_mode.text_buffer[row_in_textbuff][0], txt_mode.text_buffer[row_in_textbuff + 1][0], num_bytes_in_text_buff_row);
    }
    set_row_color(txt_mode.text_buffer[txt_mode.num_rows-1][0], background_color);
}

static void set_row_color(uint8_t * row, Color color){
    uint8_t * temp = txt_mode.text_cursor;
    txt_mode.text_cursor = row;
    for(size_t col_idx=0; col_idx<txt_mode.num_cols; col_idx++){
        terminal_putchar(0, DONT_SET, background_color);
    }
    txt_mode.text_cursor = temp;
}

void plot_text_cursor(void){
    terminal_putchar(0, DONT_SET, cursor_color);
}

static void init_text_mode_info(void){
    txt_mode.text_cell_height = curr_font.height;
    txt_mode.text_cell_width = curr_font.width;

    txt_mode.num_cols = framebuffer.width / txt_mode.text_cell_width;
    txt_mode.num_rows = framebuffer.height / txt_mode.text_cell_height;

    txt_mode.text_buffer = framebuffer.starting_address;

    txt_mode.text_cursor = txt_mode.text_buffer;
}


