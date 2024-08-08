
#include <stddef.h>
#include <stdbool.h>
#include "font.h"
#include "string.h"
#include "driver.h"
#include "vmm.h"
#include "VGA_driver.h"
#include "terminal.h"

//holds info about framebuffer
struct framebuffer framebuffer;

//a full map of the framebuffer
Pixel_t * fb_pixel_map;

//TODO: what will i use the global cursors for? these hold indices of pixels
unsigned int global_cursorX;
unsigned int global_cursorY;

//TODO: should be a window class 

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
void * calc_fb_addy_from_pix_vals(unsigned int x, unsigned int y){
    return fb_pixel_map + (framebuffer.width*y + x);
}

//advance one row of pixels from a designated start
static inline uint8_t * advance_fb_row(uint8_t * start){
    return start + framebuffer.width*framebuffer.pixel.pixel_size;
}

uint8_t * decrement_text_row(uint8_t * start){
    size_t num_bytes_in_fb_row = framebuffer.pixel.pixel_size*framebuffer.width;
    size_t num_bytes_in_text_buff_row = num_bytes_in_fb_row*terminal->font.height;
    return start - num_bytes_in_text_buff_row;
}

//advance the number of pixel rows in the current terminal font from a designated start
uint8_t * advance_text_row(uint8_t * start){
    size_t num_bytes_in_fb_row = framebuffer.pixel.pixel_size*framebuffer.width;
    size_t num_bytes_in_text_buff_row = num_bytes_in_fb_row*terminal->font.height;
    return start + num_bytes_in_text_buff_row;
}

uint8_t * advance_x_text_rows(uint8_t * start, unsigned int row){
    //could just use a for loop with advance_fb_row, but i think this is more efficient
    size_t num_bytes_in_fb_row = framebuffer.pixel.pixel_size*framebuffer.width;
    size_t num_bytes_in_text_buff_row = num_bytes_in_fb_row*terminal->font.height;
    return start + num_bytes_in_text_buff_row*row;
}

//because the text cursor is only kept track of through a pointer, it isn't immediately clear what row or column the text cell that the cursor points to occupies, so some getters are made to accomplish that
unsigned int get_pixel_term_text_row(Pixel_t * pix){
    size_t total_pixels_in_text_buff_row = framebuffer.width*terminal->font.height;
    return (pix-terminal->start)/total_pixels_in_text_buff_row;
}
//col depends on where the row starts
unsigned int get_pixel_term_text_col(Pixel_t * row_start, Pixel_t * pix){
    return (pix-row_start)/terminal->font.width;
}

uint8_t * get_pix_row_start(Pixel_t * pix){
    return advance_x_text_rows(terminal->start, get_pixel_term_text_row(pix));
}

/**
 * Each bit in a bitmap represents an entire pixel, which in my QEMU VM is 3 bytes per pixel, so iteratign through each bit in a bitmap needs to advance the pointer by num bytes per pixel
 */
void print_char(const uint32_t unicode_character_index, const Color char_color, const Color background_color){

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

void unplot_text_cursor(char char_at_cursor_loc){
    print_char(char_at_cursor_loc, DONT_SET, terminal->background_color);
}

void plot_text_cursor(char char_at_cursor_loc){
    print_char(char_at_cursor_loc, DONT_SET, terminal->cursor_color);
}

void clear_x_cells(unsigned long x){

    Pixel_t * temp = terminal->text_cursor;
    
    for(size_t col_idx=0; col_idx< x; col_idx++){
        
        print_char(0, terminal->background_color, terminal->background_color);
        
        //after setting the last text cell to the bg color, don't increment the cursor, because in the case where we clear all cells in the very bottom of the terminal, inc_textCursor will trigger the whole process of moving the terminal lines up again, which calls this again, in an infinite loop. we don't actually care about the pos of the cursor after this since it is being reset to its OG value, so it doesnt hurt to not increment after clearing the last cell
        if(col_idx != x-1){
            inc_textCursor();
        } 
    }

    terminal->text_cursor = temp;
}

void clear_x_cells_at(uint8_t * location, unsigned long x){

    Pixel_t * temp = terminal->text_cursor;
    terminal->text_cursor = location;

    for(size_t col_idx=0; col_idx< x; col_idx++){
        
        print_char(0, terminal->background_color, terminal->background_color);
        
        //after setting the last text cell to the bg color, don't increment the cursor, because in the case where we clear all cells in the very bottom of the terminal, inc_textCursor will trigger the whole process of moving the terminal lines up again, which calls this again, in an infinite loop. we don't actually care about the pos of the cursor after this since it is being reset to its OG value, so it doesnt hurt to not increment after clearing the last cell
        if(col_idx != x-1){
            inc_textCursor();
        } 
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
}

bool move_textCursor_to_new_line_start(void){

    //total cells used for an input = num_chars_in_curr_input_buff + num_text_cells_used_by_prompt 
    unsigned short total_cells_used = terminal->input.end_idx + strlen(terminal_prompt_msg);

    //round up for how many rows are needed
    unsigned char rows_needed = (total_cells_used + (terminal->num_cols-1))/terminal->num_cols;

    unsigned int row_idx = get_pixel_term_text_row(terminal->input.start);

    unsigned int next_input_row_idx = row_idx + rows_needed;

    if(next_input_row_idx > (terminal->num_rows-1)){
        shift_text_screen_up_one_row();
        terminal->text_cursor = advance_x_text_rows(terminal->start, (terminal->num_rows-1));
        clear_x_cells(terminal->num_cols);
    }
    else{
        terminal->text_cursor = advance_x_text_rows(terminal->start, next_input_row_idx);
    }
}

/**
 * * It is up to a calling program if the cursor should be moved right or not; the default behavior will be to move it right unless its the very bottom right cell, upon which it won't move.*/
bool inc_textCursor(void){

    unsigned int row_idx = get_pixel_term_text_row(terminal->text_cursor);
    Pixel_t * row_start = advance_x_text_rows(terminal->start, row_idx);

    unsigned int curr_pix_col_idx = get_pixel_term_text_col(row_start, terminal->text_cursor);
    unsigned int last_pix_col_idx_in_one_row = terminal->num_cols-1;

    //if the cursor is at the last text cell in a row
    if(curr_pix_col_idx == last_pix_col_idx_in_one_row){
        
        Pixel_t * last_term_text_row = (Pixel_t *)  advance_x_text_rows(terminal->start, terminal->num_rows-1);

        //if the text cursor points to the last poss text cell in this terminal
        if(row_start == last_term_text_row){
            shift_text_screen_up_one_row();
            terminal->text_cursor = last_term_text_row;
            terminal->input.start = decrement_text_row(terminal->input.start);
             //at this point, the current text row will be the last one
            clear_x_cells(terminal->num_cols);
        }
        else{
            terminal->text_cursor = advance_text_row(row_start);
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

    Pixel_t * row_start = get_pix_row_start(terminal->text_cursor);
    
    if(terminal->text_cursor == row_start){

        Pixel_t * one_row_back = (Pixel_t *) decrement_text_row(row_start);
        terminal->text_cursor = one_row_back + terminal->font.width*(terminal->num_cols-1);
    }
    else{
        terminal->text_cursor  -= terminal->font.width;
    }

    return true;
}

bool inc_textCursor_x_times(unsigned int x){
    for(unsigned int idx=0; idx<x; idx++){
        if(inc_textCursor()){

        }
        else{
            return false;
        }
    }
    return false;
}

void print_str(const char * string, size_t num_chars){
        
    for(size_t idx=0; idx<num_chars; idx++){
        print_char(string[idx], terminal->font_color, terminal->background_color);
        inc_textCursor();
    }
}

void initialize_framebuffer_attributes(struct multiboot_tag_framebuffer * framebuffer_info){
    
    framebuffer.starting_address = (uint8_t *)(uint32_t)framebuffer_info->common.framebuffer_addr;
    framebuffer.width = framebuffer_info->common.framebuffer_width;
    framebuffer.height = framebuffer_info->common.framebuffer_height;
    framebuffer.pitch = framebuffer_info->common.framebuffer_pitch;

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
}

