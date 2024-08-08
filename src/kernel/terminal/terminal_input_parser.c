
/**normally, my understanding is that a tty subsystem would have the keyboard driver place raw keycodes into a buffer at a specific virtual device in a filesystem's buffer, and the that tty device would send these to the terminal emulator which would interpret these keycodes. however, for now I will fold this functionality into the kernel itself until a proper filesystem, etc. is set up; this way i can keep my console API the same and only move my calls from the kernel to a userspace terminal emulator when i get my fs, etc. set up*/
//TODO: this should go through another layer of OS-level reserved keycodes to make sure the incoming keycode seqeunces aren't meant to do application-independent OS-defined actions (like CMD+Q on Mac exiting an App, regardless of the app)

#include "string.h"
#include "keycodes.h"
#include <stdbool.h>
#include "ctype.h"
#include "VGA_driver.h"
#include "terminal.h"
#include "terminal_input_parser.h"
#include <stddef.h>
#include "utils.h"

#define TAB_TEXT_CELL_COUNT 4

char temp_curr_command_buff[SHELL_INPUT_BUFF_SIZE];

//the shell polls this flag to determine when it should start parsing input
bool input_ready;

//caps will only have its state changed based on a caps make code, and shift will have its state toggled on for make and off for break. the combination of 
bool caps_active;
bool shift_active;
bool modify_incoming_scancodes;

//in the case a signal-generating key is being pressed, this will be toggled on
bool catch_incoming_scancodes;

//in a terminal these keys have these special meanings; in other apps they may be used for other things, but since this file is meant to do the housekeeping work for a shell, these are defined as such here
const char modifier_keycodes[] = {
    KEY_CAPS_LOCK,
    KEY_L_SHIFT,
    KEY_R_SHIFT,
};

const char special_keycodes[] = {
    KEY_RIGHT_ARROW,
    KEY_LEFT_ARROW,
    KEY_DOWN_ARROW,
    KEY_UP_ARROW,
    KEY_TAB,
    KEY_DELETE_BACKSPACE,
    KEY_RETURN_ENTER,
};

const char signal_generating_keycodes[] = {
    KEY_L_CTRL,
    KEY_R_CTRL,
    KEY_L_ALT,
    KEY_R_ALT,
    KEY_L_GUI,
    KEY_R_GUI,
};

//these funcs determine what type of keycode this is

static inline bool isModifierKeyCode(unsigned short keycode){
    return isInArray(&modifier_keycodes, (int) keycode, (size_t) sizeof(modifier_keycodes));
}

static inline bool isSignalGeneratingKeyCode(unsigned short keycode){
    return isInArray(&signal_generating_keycodes, (int) keycode, (size_t) sizeof(signal_generating_keycodes));
}

static inline bool isSpecialKeyCode(unsigned short keycode){
    return isInArray(&special_keycodes, (int) keycode, (size_t) sizeof(special_keycodes));
}   

// hmm?
static inline bool isKeycodePrintable(unsigned short keycode){
    if(*base_printable_keycode_mappings[keycode] != 0){
        return true;
    }
    else{
        return false;
    }

}

//funcs dealing with keycode operations

static bool cmd_exists(unsigned short cmd_idx){
    if(terminal->input.command_buff[cmd_idx][0] == 0){
        return false;
    }
    else{
        return true;
    }
}

static void inc_cursors(void){
    if(terminal->input.cursor_pos_idx < terminal->input.end_idx){
        unplot_text_cursor(terminal->input.command_buff[0][terminal->input.cursor_pos_idx]);
        terminal->input.cursor_pos_idx++;
        inc_textCursor();
        plot_text_cursor(terminal->input.command_buff[0][terminal->input.cursor_pos_idx]);
    }
}

//TODO: if my goal is to have the text and ib cursor move in tandem, this violates that. honestly the API works rn for all the terminal driver ops i have implemented, but it does feel a bit hard to follow why certain things happen at certain points. i rely on comments to indicate this and remind me in the future for why the code is in a certain sequence, but would be nice to have cleaner code to aid in that
static void init_inputBuffCursor(void){
    
    unsigned char idx = 0;
    while(terminal->input.command_buff[0][idx] != 0){
        idx++;
    }
    terminal->input.end_idx = idx;
    terminal->input.cursor_pos_idx = terminal->input.end_idx;
}

static void initialize_new_input_line(void){
    //move the text cursor to a new line if it isn't at the beginning of one, and restart the prompt
   
    move_textCursor_to_new_line_start();

    //reset input buffer cursors here so that the last row of the input can be detected and the terminal can go to the proper row to print the new prompt

    init_inputBuffCursor();

    print_prompt();
    terminal->input.start = terminal->text_cursor;
    plot_text_cursor(terminal->input.command_buff[0][terminal->input.cursor_pos_idx]);

    terminal->input.curr_command_idx = 0;
}

/**
 * Takes care of checking if possible to print, and handles all cases of the cursor being at various positions in the input buffer
 */
static bool place_char_at_cursor_pos(char new_char){

    if(terminal->input.end_idx == (SHELL_INPUT_BUFF_SIZE-1)){
        return false;
    }

    //if the cursor is not at the end of the command string, the part of the string after the cursor will need to be moved rifght
    if(terminal->input.cursor_pos_idx != terminal->input.end_idx){
        
        char * cursor_pos = &terminal->input.command_buff[0][terminal->input.cursor_pos_idx];

        //shift everything in the current command buffer to the right one to make space for the new char being written
        memmove(cursor_pos+1, cursor_pos, terminal->input.end_idx - terminal->input.cursor_pos_idx);

    }     

    terminal->input.command_buff[0][terminal->input.cursor_pos_idx] = new_char;

    terminal->input.cursor_pos_idx++;
    terminal->input.end_idx++;

    //at this point the input buffer is up to date 
    
    //now update the graphic rendering to match the input buffer

    //even though the cursor and the input has been shifted right, the cursor points to the same char as it did in the beginning of this function, so that is why the current val of the input buffer cursor is ok to pass here 
    unplot_text_cursor(terminal->input.command_buff[0][terminal->input.cursor_pos_idx]);
   
    //if the cursor is not at the end, the existing part of the command at and after the cursor needs to be shifted right, and then the new char rendered at the original text cursor spot
    if(terminal->input.cursor_pos_idx != terminal->input.end_idx){
        
        //shift the string before writing the new char so that
        const char * shift_string = &terminal->input.command_buff[0][terminal->input.cursor_pos_idx];

        Pixel_t * temp = terminal->text_cursor;

        //need to skip the current text cell because this is where the new char will be printed
        inc_textCursor();

        print_str(shift_string, strlen(shift_string));

        terminal->text_cursor = temp;
    }
   
    //at this point the text cursor should be pointing to the text cell corresponding to the input buffer cursor at the START of this function, BEFORE the new char is written 

    print_char(new_char, terminal->font_color, terminal->background_color);

    inc_textCursor();

    plot_text_cursor(terminal->input.command_buff[0][terminal->input.cursor_pos_idx]);

    return true;
}

static char ** autofill_command(void){

}

static void log_and_reset_input_buffers(void){

    //if some command was actually typed before entering it, store the command
    if(terminal->input.command_buff[0][0] != 0){

        //move all commands up one row in the command buffer 
        for(char idx=MAX_NUM_COMMANDS_STORED-1; idx>0; idx--){
            memcpy(terminal->input.command_buff[idx], terminal->input.command_buff[idx-1], SHELL_INPUT_BUFF_SIZE);
        }

        //reset the next current input buffer only if it isn't already null
        memset(terminal->input.command_buff[0], 0, SHELL_INPUT_BUFF_SIZE);
    }

    
    //reset the temporary holding buffer regardless, because prev input may have been stored here before pressing up or down arrows, at which point at one of the prev command, the user could delete all chars, causing an empty buffer to be entered, even though there is some original input stored before pressing up or down
    memset(temp_curr_command_buff, 0, SHELL_INPUT_BUFF_SIZE);
}

static inline void preserve_last_written_command(void){
    //preserve the last written command (this will overwrite any previously stored commands originally stored before scrolling, if there is one stored)
    memcpy(temp_curr_command_buff, terminal->input.command_buff[0], SHELL_INPUT_BUFF_SIZE);
}

static inline void copy_diff_cmd_to_curr_cmd_buffer(unsigned short cmd_indx){
    memcpy(terminal->input.command_buff[0], terminal->input.command_buff[cmd_indx], SHELL_INPUT_BUFF_SIZE);
}

static void dec_cursors(void){
    if(terminal->input.cursor_pos_idx > 0){
        unplot_text_cursor(terminal->input.command_buff[0][terminal->input.cursor_pos_idx]);
        terminal->input.cursor_pos_idx--;
        dec_textCursor();
        plot_text_cursor(terminal->input.command_buff[0][terminal->input.cursor_pos_idx]);
    }
}

static void replace_curr_cmd_with_diff_cmd(unsigned short diff_cmd_indx){

    unplot_text_cursor(terminal->input.command_buff[0][terminal->input.cursor_pos_idx]);

    //before altering the input buffer, clear the screen from its content
    clear_x_cells_at(terminal->input.start, terminal->input.end_idx);    

    copy_diff_cmd_to_curr_cmd_buffer(diff_cmd_indx);
    
    init_inputBuffCursor();

    //start printing the diff cmd form the beginning of the line
    terminal->text_cursor = terminal->input.start;

    const char * new_curr_cmd = &terminal->input.command_buff[0];

    print_str(new_curr_cmd, strlen(new_curr_cmd));

    plot_text_cursor(terminal->input.command_buff[0][terminal->input.cursor_pos_idx]);

}



//keycode handlers

static void handle_signal_generating_keycode(unsigned short keycode){

}


static void handle_special_keycode(unsigned short keycode, bool isBreakCode){

    switch(keycode){
        case(KEY_RIGHT_ARROW):
            inc_cursors();
            break;

        case(KEY_LEFT_ARROW):
            dec_cursors();
            break;

        case(KEY_DOWN_ARROW):

            //dont need to check if a command exists because we came from an existing command

            if(terminal->input.curr_command_idx > 0){

                terminal->input.curr_command_idx--;
                
                //if the user is going back to their OG input, re-initialize the curr command buffer to their command stored before going to previous commands
                if(terminal->input.curr_command_idx == 0){
                    memcpy(terminal->input.command_buff[0], temp_curr_command_buff, SHELL_INPUT_BUFF_SIZE);
                }

                replace_curr_cmd_with_diff_cmd(terminal->input.curr_command_idx);
                
            }
            break;

        case(KEY_UP_ARROW):

            if(terminal->input.curr_command_idx < MAX_NUM_COMMANDS_STORED-1){

                if(cmd_exists(terminal->input.curr_command_idx+1)){

                    //a user can store the current command they were typing before going back to potentially re-execute or alter and then execute a previous command (but if a user goes back to a prev command and alters it, then goes up or down after that, the edited prev command will not be stored)
                    if(terminal->input.curr_command_idx == 0){
                        preserve_last_written_command();
                    }
                    replace_curr_cmd_with_diff_cmd(++terminal->input.curr_command_idx);
                }
            }
            break;

        case(KEY_TAB):
            //TODO: honestly ignoring this for now because i think this requires a redesign since the way other terminals seem to handle this is by having some input buffer and placing all keypresses in it (that aren't signal generating, etc), so a \t would be placed in the buff. then when rendering/deleting, there seems to be some layer that will interpret these special keycodes and render them accordingly. so i think i just need a cleaner API for the VGA driver and terminal driver overall, which i think would be helped by switching to C++, or at least putting more effort into incoporating OO in the design 
            //if there's no preceding chars, move the cursor tab distance
            if(terminal->input.cursor_pos_idx == 0){
                unplot_text_cursor(terminal->input.command_buff[0][terminal->input.cursor_pos_idx]);
                inc_textCursor_x_times(TAB_TEXT_CELL_COUNT);
                plot_text_cursor(terminal->input.command_buff[0][terminal->input.cursor_pos_idx]);
            }
            else{
                autofill_command();
            }
            break;

        case(KEY_DELETE_BACKSPACE):

            if(!isBreakCode && terminal->input.cursor_pos_idx != 0){

                unplot_text_cursor(terminal->input.command_buff[0][terminal->input.cursor_pos_idx]);
                
                //if the cursor is at the end of the command input
                if(terminal->input.cursor_pos_idx == terminal->input.end_idx){

                    terminal->input.cursor_pos_idx--;
                    terminal->input.end_idx--;

                    terminal->input.command_buff[0][terminal->input.cursor_pos_idx] = 0;

                    dec_textCursor();

                    plot_text_cursor(terminal->input.command_buff[0][terminal->input.cursor_pos_idx]);
                }
                //if its not, part of the command input string at and after the cursor will have to be moved left one
                else{

                    char * curr_pos = &terminal->input.command_buff[0][terminal->input.cursor_pos_idx];

                    //the extra +1 will copy the null terminator as well
                    memcpy(curr_pos-1, curr_pos, terminal->input.end_idx - terminal->input.cursor_pos_idx + 1);
                    
                    clear_x_cells(terminal->input.end_idx - terminal->input.cursor_pos_idx);

                    terminal->input.end_idx--;
                    terminal->input.cursor_pos_idx--;

                    dec_textCursor();

                    char * altered_cmd = &terminal->input.command_buff[0][terminal->input.cursor_pos_idx];

                    Pixel_t * temp = terminal->text_cursor;
                    
                    print_str(altered_cmd, strlen(altered_cmd));

                    terminal->text_cursor = temp;
                    
                    plot_text_cursor(terminal->input.command_buff[0][terminal->input.cursor_pos_idx]);

                }
                
            }

            break;
        
        case(KEY_RETURN_ENTER):

            if(!isBreakCode){

                //TODO: without any locks, we cannot let the shell have access to the buff on its own; we must have the shell processing and output all be completed from here, which itself came from the ISR. this needs to be fixed in the future
        
                shell_input(terminal->input.command_buff[0]);

                unplot_text_cursor(terminal->input.command_buff[0][terminal->input.cursor_pos_idx]);
                
                log_and_reset_input_buffers();

                initialize_new_input_line();
            }

                break;
    }
}


static inline void handle_modifier_keycode(unsigned short keycode, bool isBreakCode){
    //every time a condition is met to change this, it flips. this will replicate existing OS implementations besides the case of two shifts being pressed at once, which in my case will cause no modifications to occur, unless caps lock is also on. essentially, every time a make or break for any modifier keycode comes in, this flips
   
    switch(keycode){
        case(KEY_CAPS_LOCK):
            if(!isBreakCode){
                //a caps lock key does nothing when it is released; all actions involving it occur when it is pressed
                caps_active = !caps_active;
            }
            break;
        case(KEY_L_SHIFT):
            if(!isBreakCode){
                shift_active = true;
            }
            else{
                shift_active = false;
            }
            break;
        case(KEY_R_SHIFT):
            //same as L shift
            if(!isBreakCode){
                shift_active = true;
            }
            else{
                shift_active = false;
            }
            break;
    }
    modify_incoming_scancodes = shift_active || caps_active;
}

static void handle_printable_keycode(unsigned short keycode){

    //if the input buffer isn't full
    if(terminal->input.end_idx != (SHELL_INPUT_BUFF_SIZE-1)){

        char new_char = *base_printable_keycode_mappings[keycode];
        
        if(modify_incoming_scancodes){
            if(shift_active){
                new_char = *modified_printable_keycode_mappings[keycode];
            }
            else if(caps_active){
                if(isalpha(new_char)){
                    new_char = *modified_printable_keycode_mappings[keycode];
                }
            }
        }
        
        place_char_at_cursor_pos(new_char);
        
    }

}

void handle_keycode(unsigned short keycode, bool isBreakCode){

    if(catch_incoming_scancodes){
        //TODO: handle signals for cancelling input, copying to clipboard, etc
    }
    else if(isSpecialKeyCode(keycode)){
        handle_special_keycode(keycode, isBreakCode);
    }
    else if(isModifierKeyCode(keycode)){
        handle_modifier_keycode(keycode, isBreakCode);
    }
    else if(isSignalGeneratingKeyCode(keycode)){
        handle_signal_generating_keycode(keycode);
    }
    else if(isKeycodePrintable(keycode)){
        if(!isBreakCode){
            handle_printable_keycode(keycode);
        }
    }
    else{
        //in this case there is a bug and the keycode does not fit any category assigned in this file
    }
}