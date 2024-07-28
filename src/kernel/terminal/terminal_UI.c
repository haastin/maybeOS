#include "VGA_driver.h"
#include "font.h"
#include "string.h"

/*This file is meant to manage the UI for a terminal*/

//at some point would like to clean up the UI and not have the entire screen be treated as a terminal; as part of that would like to have some nice spacing, etc as seen below
// #define TERMINAL_SIDE_GAP_SIZE 1
// #define TERMINAL_TOP_AND_BOT_GAP_SIZE 5


static char * terminal_prompt_msg = "maybeOS > ";

void start_shell(){
    set_background_color(GRAY);
    terminal_printstr(terminal_prompt_msg, strlen(terminal_prompt_msg));
    shell_main();
}