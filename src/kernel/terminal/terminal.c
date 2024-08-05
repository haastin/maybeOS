#include "VGA_driver.h"
#include "font.h"
#include "string.h"

/*This file is meant to manage the UI for a terminal*/

//TODO: this is prob the wrong structure to have for a terminal. would like to get a true copy of a tty subsystem at some point. For now, terminal UI stuff is really sprinkled into the VGA driver, and this will just make calls to those funcs that the VGA driver offers

static const char * terminal_prompt_msg = "maybeOS > ";

void print_prompt(void){
    terminal_printstr(terminal_prompt_msg, strlen(terminal_prompt_msg));
}

void start_shell(){
    set_background_color(GRAY);
    print_prompt();

    //the shell starts here and never returns, but will only do something if a flag it polls is set, indicating a full line of input is meant to be processed by the shell. input is processed in a seperate file, but all of this processing is called from the keyboard ISR, which isn't great because it makes the response to a keyboard press longer. but, without a spinlock or anything else, it is the only way to ensure that the input is processed before more keypresses come in and override what is prev in the buffer
    shell_main();
}