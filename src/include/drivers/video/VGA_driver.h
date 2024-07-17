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

extern struct framebuffer framebuffer;
extern Font curr_font;

void initialize_framebuffer_attributes(struct multiboot_tag_framebuffer * framebuffer_info); 

void terminal_putchar(const uint32_t unicode_character_index, const Color char_color, const Color background_color);
void terminal_printstr(const char *string, size_t num_chars);
void set_background_color(Color background);
bool inc_textCursor(void);
bool dec_textCursor(void);


#endif /*__VGA_DRIVER_H__*/

void init_new_window();
