
/**normally, my understanding is that a tty subsystem would have the keyboard driver place raw keycodes into a buffer at a specific virtual device in a filesystem's buffer, and the that tty device would send these to the terminal emulator which would interpret these keycodes. however, for now I will fold this functionality into the kernel itself until a proper filesystem, etc. is set up; this way i can keep my console API the same and only move my calls from the kernel to a userspace terminal emulator when i get my fs, etc. set up*/

//on second thought, this file should be responsible for updating the screen with input as well as directing input to the shell program when it makes sense to do so

#include "string.h"
#include "keycodes.h"
#include <stdbool.h>
#include "ctype.h"


#define SHELL_INPUT_BUFF_SIZE 256
#define NUM_COMMANDS_STORED 100

//this will buffer text until a newline is dedicated, or until a keycode sequence to kill a program has been passed, which will reset the buffer. input is only sent to the shell when the shell can process it. 
static char shell_input_buff[SHELL_INPUT_BUFF_SIZE];
unsigned short shell_input_buff_idx=0;

static char command_history_buff[SHELL_INPUT_BUFF_SIZE][NUM_COMMANDS_STORED];

bool input_ready;

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
};

const char signal_generating_keycodes[] = {
    KEY_L_CTRL,
    KEY_R_CTRL,
    KEY_L_ALT,
    KEY_R_ALT,
    KEY_L_GUI,
    KEY_R_GUI,
};


void handle_nonprintable_keycode(unsigned short keycode);

static bool isInArray(const void *array, int c, size_t array_len);

static bool isModifierKeyCode(unsigned short keycode);

static bool isSignalGeneratingKeyCode(unsigned short keycode);

static bool isSpecialKeyCode(unsigned short keycode);

static char *get_prev_command(void);

static char **autofill_command(void);


static void handle_modifier_keycode(unsigned short keycode){

}

static void handle_signal_generating_keycode(unsigned short keycode){

}

static void handle_printable_keycode(unsigned short keycode){

}

static inline bool isKeycodePrintable(unsigned short keycode){
    if(base_printable_keycode_mappings[keycode] != 0){
        return true;
    }
    else{
        return false;
    }

}

static inline bool isInArray(const void * array, int c, size_t array_len){
    return memchr(array, c, array_len) == NULL ? false : true;
}

static inline bool isModifierKeyCode(unsigned short keycode){
    return isInArray(&modifier_keycodes, (int) keycode, (size_t) sizeof(modifier_keycodes));
}

static inline bool isSignalGeneratingKeyCode(unsigned short keycode){
    return isInArray(&signal_generating_keycodes, (int) keycode, (size_t) sizeof(signal_generating_keycodes));
}

static inline bool isSpecialKeyCode(unsigned short keycode){
    return isInArray(&special_keycodes, (int) keycode, (size_t) sizeof(special_keycodes));
}   

static char * get_prev_command(void){

}

static char ** autofill_command(void){

}

static void handle_special_keycode(unsigned short keycode){
    
    switch(keycode){
        case(KEY_RIGHT_ARROW):
            //move cursor right, if there's a char there
            break;
        case(KEY_LEFT_ARROW):
            //move cursor left, is there's a char there
            break;
        case(KEY_DOWN_ARROW):
            break;
        case(KEY_UP_ARROW):
            break;
        case(KEY_TAB):
            //if there's no preceding chars, move the cursor tab distance
            if(shell_input_buff_idx == 0){
                //move cursor a distance of tab
            }
            //if chars before tab, autofill is attempted
            else{
                char** possible_commands = autofill_command();
            }
            break;
    }
}

void handle_keycode(unsigned short keycode){

    if(isSpecialKeyCode(keycode)){
        handle_special_keycode(keycode);
    }
    else if(isModifierKeyCode(keycode)){
        handle_modifier_keycode(keycode);
    }
    else if(isSignalGeneratingKeyCode(keycode)){
        handle_signal_generating_keycode(keycode);
    }
    else if(isKeycodePrintable(keycode)){
        handle_printable_keycode(keycode);
    }
    else{
        //in this case there is a bug and the keycode does not fit any category assigned in this file
    }
}