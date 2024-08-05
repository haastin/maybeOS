#ifndef __VGA_DRIVER_H__
#define __VGA_DRIVER_H__
#include <stdint.h>
#include "multiboot2.h"
#include "font.h"
#include <stddef.h>

struct framebuffer{
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

typedef struct terminal{

    //--- bounds of the terminal---
    unsigned int left_boundary_x;
    unsigned int right_boundary_x;
    unsigned int top_boundary_y;
    unsigned int bottom_boundary_y;

    unsigned int num_rows;
    unsigned int num_cols;
    
    Font font;

    Color background_color;
    Color cursor_color;
    Color font_color;

    /**
     * A terminal is logically divided into "text cells". There are no data structures that implement this explicitly, as the amount of mem needed to support it is excessive. A text cell is the bitmap for the current terminal's font size. The default terminal font is 8x16 pixels, so a bitmap, and thus a text cell, is made up of 2D array of pixels like so: Pixel_t text_cell[8][16]. So, when printing in text mode, the cursor will point to the top left pixel of the text cell, and the address of each pixel afterwards is calculated, as opposed to having a data structure to directly access it, as I mentioned before. 
     * 
     * A terminal's boundaries may not fit a whole number of text cells. Regardless, text cells will ALWAYS start from the first possible pixel on the left side of the terminal and the max possible amount of text cells will be accessed sequentially from the left. The pixels remaining after the last possible text cell in a row will only be accounted for when moving the cursor left, and the extra pixels remaining in the columns at the bottom of the terminal will never need to be interacted in the current framebuffer API
     */
    Pixel_t * start;
    Pixel_t * text_cursor;

} terminal_t;

extern struct framebuffer framebuffer;
extern terminal_t * terminal;

void initialize_framebuffer_attributes(struct multiboot_tag_framebuffer * framebuffer_info); 

void terminal_putchar(const uint32_t unicode_character_index, const Color char_color, const Color background_color);
void terminal_printstr(const char *string, size_t num_chars);
void set_background_color(Color background);
bool inc_textCursor(void);
bool dec_textCursor(void);


#endif /*__VGA_DRIVER_H__*/
