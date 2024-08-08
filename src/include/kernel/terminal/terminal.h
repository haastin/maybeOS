#ifndef __TERMINAL_H__
#define __TERMINAL_H__

#include "font.h"
#include "VGA_driver.h"

#define SHELL_INPUT_BUFF_SIZE 256

#define MAX_NUM_COMMANDS_STORED 25

typedef struct {
    
    Pixel_t * start;

    //the currnet command will always be stored in index 0, and prev commands in increasing indices. this will buffer text until a newline is dedicated, or until a keycode sequence to kill a program has been passed, which will reset the buffer. input is only sent to the shell when the shell can process it.
    char command_buff[MAX_NUM_COMMANDS_STORED][SHELL_INPUT_BUFF_SIZE];

    //is the index of the null terminator of the input command
    unsigned short end_idx;

    //keep track of pos in the buff the cursor is at (aka the index that will have the next char placed in it)
    unsigned short cursor_pos_idx;

    //keeps track of what command in the history is currently being buffered
    unsigned short curr_command_idx;

} input_buff_t;

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

    //Additional information is kept about the current line receiving input in the terminal
    input_buff_t input;

} terminal_t;


extern terminal_t * terminal;

extern const char * terminal_prompt_msg;

void init_terminal_subsystem();

void start_shell();

void print_prompt(void);

#endif /*__TERMINAL_H__*/
