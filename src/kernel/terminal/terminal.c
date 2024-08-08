#include "VGA_driver.h"
#include "font.h"
#include "string.h"
#include "terminal.h"

//TODO: this is prob the wrong structure to have for a terminal. would like to get a true copy of a tty subsystem at some point. For now, terminal UI stuff is really sprinkled into the VGA driver, and this will just make calls to those funcs that the VGA driver offers

terminal_t * terminal;

const char * terminal_prompt_msg = "maybeOS > ";

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

void init_terminal_subsystem(void){

    set_background_color(GRAY);

    terminal = (terminal_t *) init_terminal();
    
    print_prompt();
    plot_text_cursor(terminal->input.command_buff[0][terminal->input.cursor_pos_idx]);

    //the start of the input buffer for a terminal is the address of the first text cell AFTER the prompt
    terminal->input.start = terminal->text_cursor;
}

void print_prompt(void){
    print_str(terminal_prompt_msg, strlen(terminal_prompt_msg));
}

void start_shell(){
    
    //the shell starts here and never returns, but will only do something if a flag it polls is set, indicating a full line of input is meant to be processed by the shell. input is processed in a seperate file, but all of this processing is called from the keyboard ISR, which isn't great because it makes the response to a keyboard press longer. but, without a spinlock or anything else, it is the only way to ensure that the input is processed before more keypresses come in and override what is prev in the buffer
    shell_main();
}