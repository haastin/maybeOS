#include "VGA_driver.h"
#include <stddef.h>
#include <stdbool.h>
#include "font.h"
#include "string.h"
#include "driver.h"
#include "vmm.h"

//holds info about framebuffer
struct framebuffer framebuffer;

//a full map of the framebuffer
Pixel_t * fb_pixel_map;

//TODO: what will i use the global cursors for? these hold indices of pixels
unsigned int global_cursorX;
unsigned int global_cursorY;

//TODO: should be curr window
//the current terminal being written
terminal_t * terminal;


void set_pixel_RGB(uint8_t * pixel_color_address, Color pixel_color){
    
    unsigned char red_val = ((pixel_color >> RED_INDEX) & 0xFF);
    unsigned char blue_val = ((pixel_color >> BLUE_INDEX) & 0xFF);
    unsigned char green_val = ((pixel_color >> GREEN_INDEX) & 0xFF);

    *(pixel_color_address + framebuffer.pixel.red_bits_index/8) = red_val;
    *(pixel_color_address + framebuffer.pixel.blue_bits_index/8) = blue_val;
    *(pixel_color_address + framebuffer.pixel.green_bits_index/8) = green_val;
}

static inline unsigned char* get_char_font_bitmap_start(uint32_t unicode_character_index){
    unsigned char* char_bitmap = terminal->font.glyph + unicode_character_index*terminal->font.bytesperglyph;
    return char_bitmap;
}

//calc address from the fb start using pixel values
static inline void * calc_fb_addy_from_pix_vals(unsigned int x, unsigned int y){
    return fb_pixel_map + (framebuffer.width*y + x);
}

//advance one row of pixels from a designated start
static inline uint8_t * advance_fb_row(uint8_t * start){
    return start + framebuffer.width*framebuffer.pixel.pixel_size;
}

//advance the number of pixel rows in the current terminal font from a designated start
static inline uint8_t * advance_text_row(uint8_t * start){
    size_t num_bytes_in_fb_row = framebuffer.pixel.pixel_size*framebuffer.width;
    size_t num_bytes_in_text_buff_row = num_bytes_in_fb_row*terminal->font.height;
    return start + num_bytes_in_text_buff_row;
}

static inline uint8_t * advance_x_text_rows(uint8_t * start, unsigned int row){
    //could just use a for loop with advance_fb_row, but i think this is more efficient
    size_t num_bytes_in_fb_row = framebuffer.pixel.pixel_size*framebuffer.width;
    size_t num_bytes_in_text_buff_row = num_bytes_in_fb_row*terminal->font.height;
    return start + num_bytes_in_text_buff_row*row;
}

//these two functions calculate the pixels remaining at the end of a row or column in the terminal which aren't enough in number to create another text cell in the row/col, but still need to be accounted for in various operations
static inline unsigned char get_term_row_overflow_pix_count(void){
    return (terminal->right_boundary_x - terminal->left_boundary_x) % terminal->font.width;
}

static inline unsigned char get_term_col_overflow_pix_count(void){
    return (terminal->bottom_boundary_y - terminal->top_boundary_y) % terminal->font.height;
}

//TODO: could the efficiency of these funcs below be improved?
//because the text cursor is only kept track of through a pointer, it isn't immediately clear what row or column the text cell that the cursor points to occupies, so some getters are made to accomplish that
static unsigned int get_pixel_term_text_row(Pixel_t * pix){
    size_t num_bytes_in_fb_row = framebuffer.pixel.pixel_size*framebuffer.width;
    size_t num_bytes_in_text_buff_row = num_bytes_in_fb_row*terminal->font.height;
    return (pix-terminal->start)/num_bytes_in_text_buff_row;
}
//col depends on where the row starts
static unsigned int get_pixel_term_text_col(Pixel_t * row_start, Pixel_t * pix){
    return (pix-row_start)/terminal->font.width;
}

/**
 * Each bit in a bitmap represents an entire pixel, which in my QEMU VM is 3 bytes per pixel, so iteratign through each bit in a bitmap needs to advance the pointer by num bytes per pixel
 */
void terminal_putchar(const uint32_t unicode_character_index, const Color char_color, const Color background_color){

    //the bitmap for the char index passed
    unsigned char* char_glyph = get_char_font_bitmap_start(unicode_character_index);
    
    //indicates which bit in the bitmap we are on
    unsigned char bitmap_idx = 0;

    Pixel_t * curr_row = terminal->text_cursor;
    
    for(size_t char_y = 0; char_y < terminal->font.height; char_y++){

        //curr_row is updated after the next loop
        Pixel_t* curr_pixel_address = curr_row;
        
        for(size_t char_x = 0; char_x < terminal->font.width; char_x++){
            
            bool isChar = isCharBit(char_glyph, bitmap_idx, &terminal->font);
            
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
            curr_pixel_address++;
        }

        curr_row = advance_fb_row(curr_row);
    }
}

void set_background_color(Color new_background){
    Pixel_t * curr_pix = fb_pixel_map;
    for(size_t y=0; y< framebuffer.height; y++){
        for(size_t x = 0; x < framebuffer.width; x++){
            set_pixel_RGB(curr_pix, new_background);
            curr_pix++;
        }
    }
}

void plot_text_cursor(void){
    terminal_putchar(0, DONT_SET, terminal->cursor_color);
}

static void set_row_color(uint8_t * row){
    uint8_t * temp = terminal->text_cursor;
    terminal->text_cursor = row;
    for(size_t col_idx=0; col_idx<terminal->num_cols; col_idx++){
        terminal_putchar(0, DONT_SET, terminal->background_color);
    }
    terminal->text_cursor = temp;
}

/**
 * * When all rows are shifted up by one, the top row will be discarded. The very bottom row would remain the same as the row above (since it is copied there), so it is initialized to just be the a blank row matching the formatting of the rest of the text buffer.
 */
void shift_text_screen_up_one_row(void){
    
    size_t num_bytes_in_fb_row = framebuffer.pixel.pixel_size*framebuffer.width;
    size_t num_bytes_in_text_buff_row = num_bytes_in_fb_row*terminal->font.height;
    
    uint8_t * curr_text_row = terminal->start;
    
    for(size_t row_in_textbuff=0; row_in_textbuff < terminal->num_rows-1; row_in_textbuff++){
      
        uint8_t * next_text_row = advance_text_row(curr_text_row);
        memcpy(curr_text_row, next_text_row, num_bytes_in_text_buff_row);
        curr_text_row = next_text_row;
    }
    //at this point, the current text row will be the last one
    set_row_color(curr_text_row);
}

/**
 * * It is up to a calling program if the cursor should be moved right or not; the default behavior will be to move it right unless its the very bottom right cell, upon which it won't move.*/
bool inc_textCursor(void){

    Pixel_t * row_start = get_pixel_term_text_row(terminal->text_cursor);
    
    unsigned int curr_pix_col_idx = get_pixel_term_text_col(row_start, terminal->text_cursor);
    unsigned int last_pix_col_idx_in_one_row = terminal->num_cols-1;

    //if the cursor is at the last text cell in a row
    if(curr_pix_col_idx == last_pix_col_idx_in_one_row){
        
        Pixel_t * last_term_text_row = (Pixel_t *)  advance_x_text_rows(terminal->start, terminal->num_rows-1);

        //if the text cursor points to the last poss text cell in this terminal
        if(row_start == last_term_text_row){
            shift_text_screen_up_one_row();
            terminal->text_cursor = last_term_text_row;
        }
        else{
            terminal->text_cursor = advance_text_row(row_start);
            //this is an alternate implementation of advancing the cursor, but i think advancing the text row is faster for now
            // unsigned char overflow_pixels = get_term_row_overflow_pix_count();
            // terminal->text_cursor += (overflow_pixels + terminal->font.width);
        }

    }
    else{
        //the cursor is not at the last text cell in a row
        terminal->text_cursor += terminal->font.width;
    }

    return true;
}


/**
 * *It is up to the calling program if the cursor should be moved left or not; the default behavior is that the cursor will be moved left until it reaches the evry top left of the screen, upon which it won't move left anymore.
 */
bool dec_textCursor(void){

    if(terminal->text_cursor == terminal->start){
        return false;
    }

    Pixel_t * row_start = get_pixel_term_text_row(terminal->text_cursor);
    
    if(terminal->text_cursor == row_start){
        unsigned char overflow_pixels = get_term_row_overflow_pix_count();
        terminal->text_cursor -= (overflow_pixels + terminal->font.width);
    }
    else{
        terminal->text_cursor  -= terminal->font.width;
    }

    return true;
}

void terminal_printstr(const char * string, size_t num_chars){
    for(size_t idx=0; idx<num_chars; idx++){
        terminal_putchar(string[idx], terminal->font_color, DONT_SET);
        inc_textCursor();
    }
    plot_text_cursor();
}

terminal_t * init_terminal(void){
    terminal_t * new_terminal = kmalloc(sizeof(terminal_t));
    
    new_terminal->background_color = GRAY;
    new_terminal->font_color = BLACK;
    new_terminal->cursor_color = SILVER;

    set_font(DEFAULT_FONT, &new_terminal->font);
    
    //TODO: as long as my GUI is just a shell interface, these will be constant, but once an event-driven UI is made, these must be passed
    new_terminal->top_boundary_y = 20;
    new_terminal->bottom_boundary_y = framebuffer.height - 20;
    new_terminal->left_boundary_x = 20;
    new_terminal->right_boundary_x = framebuffer.width - 20;

    new_terminal->num_cols = (new_terminal->right_boundary_x - new_terminal->left_boundary_x)/new_terminal->font.width;
    new_terminal->num_rows = (new_terminal->bottom_boundary_y - new_terminal->top_boundary_y)/new_terminal->font.height;

    new_terminal->start = calc_fb_addy_from_pix_vals(new_terminal->left_boundary_x, new_terminal->top_boundary_y);
    
    new_terminal->text_cursor = new_terminal->start;

    return new_terminal;
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
    framebuffer.pixel.pixel_size = framebuffer.pixel.total_bits/8;
 
    unsigned long framebuffer_mmio_length = (framebuffer.width*framebuffer.height)*(framebuffer_info->common.framebuffer_bpp/8);
    init_MMIO_device(framebuffer.starting_address, framebuffer_mmio_length);

    fb_pixel_map = framebuffer.starting_address;

    terminal = (terminal_t *) init_terminal();
}

