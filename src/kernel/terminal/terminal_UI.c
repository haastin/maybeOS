#include "VGA_driver.h"
#include "font.h"
#include "string.h"

/*This file is meant to manage the UI for a terminal*/

//TODO: this is prob the wrong structure to have for a terminal. would like to get a true copy of a tty subsystem at some point. For now, terminal UI stuff is really sprinkled into the VGA driver, and this will just make calls to those funcs that the VGA driver offers

static const char * terminal_prompt_msg = "maybeOS > ";

void start_shell(){
    set_background_color(GRAY);
    terminal_printstr(terminal_prompt_msg, strlen(terminal_prompt_msg));
    shell_main();
}