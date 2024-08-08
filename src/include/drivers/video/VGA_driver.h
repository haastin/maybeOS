#ifndef __VGA_DRIVER_H__
#define __VGA_DRIVER_H__
#include <stdint.h>
#include "multiboot2.h"
#include "font.h"
#include <stddef.h>

struct framebuffer{
    uint8_t * starting_address;
    unsigned int width;
    unsigned int height;
    unsigned long pitch;
    struct Pixel{
        uint8_t total_bits;
        uint8_t num_red_bits;
        uint8_t red_bits_index;
        uint8_t num_green_bits;
        uint8_t green_bits_index;
        uint8_t num_blue_bits;
        uint8_t blue_bits_index;
        unsigned char pixel_size;
    } pixel;
};


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
    DONT_SET    = -1,
} Color;

//Bit indexes of each 8bit RGB in a Color entry
#define RED_INDEX 16
#define GREEN_INDEX 8
#define BLUE_INDEX 0

//TODO: should really be classes with subclasses. the topmost class should prob be a window class, and a termnal should just be an implementation of a window
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} __attribute__((packed)) Pixel_t;

extern struct framebuffer framebuffer;

void initialize_framebuffer_attributes(struct multiboot_tag_framebuffer * framebuffer_info);
void set_background_color(Color background); 

void * calc_fb_addy_from_pix_vals(unsigned int x, unsigned int y);

//functions pertaining to moving throughout the terminal in "text mode"
uint8_t * decrement_text_row(uint8_t * start);
uint8_t *advance_text_row(uint8_t *start);
uint8_t *advance_x_text_rows(uint8_t *start, unsigned int row);
unsigned int get_pixel_term_text_row(Pixel_t *pix);
unsigned int get_pixel_term_text_col(Pixel_t *row_start, Pixel_t *pix);
uint8_t *get_pix_row_start(Pixel_t *pix);

//the API used in "text mode" to print to the current location of the text cursor
void print_char(const uint32_t unicode_character_index, const Color char_color, const Color background_color);
void print_str(const char *string, size_t num_chars);

//rendering and unrendering the cursor must often be done seperately from printing
void unplot_text_cursor(char char_at_cursor_loc);
void plot_text_cursor(char char_at_cursor_loc);

//for when a cell needs to be cleared of its current char to just match the background
void clear_x_cells_at(uint8_t * location, unsigned long x);
void clear_x_cells(unsigned long x);

//move the text cursor to the start of the line after it (or the start of the same line if the text cursor is in the last row of the terminal)
bool move_textCursor_to_new_line_start(void);

//operations to move the current location of the text cursor
bool inc_textCursor(void);
bool dec_textCursor(void);
bool inc_textCursor_x_times(unsigned int x);

#endif /*__VGA_DRIVER_H__*/
