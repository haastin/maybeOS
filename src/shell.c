
#include <stdbool.h>
#include "string.h"
#include "terminal.h"

const char * maybeOS_logo =

        "                             ==(W{==========-      /===-                        \n"
        "                              ||  (.--.)         /===-_---~~~~~~~~~------____  \n"
        "                              | \\_,|**|,__      |===-~___                _,-' `\n"
        "                 -==\\\\        `\\ ' `--'   ),    `//~\\\\   ~~~~`---.___.-~~      \n"
        "             ______-==|        /`\\_. .__\\/ \\    | |  \\\\           _-~`         \n"
        "       __--~~~  ,-/-==\\\\      (   | .  |~~~~|   | |   `\\        ,'             \n"
        "    _-~       /'    |  \\\\     )__/==0==-\\<>/   / /      \\      /               \n"
        "  .'        /       |   \\\\      /~\\___/~~\\/  /' /        \\   /'                \n"
        " /  ____  /         |    \\`\\.__/-~~   \\  |_/'  /          \\/'                  \n"
        "/-'~    ~~~~~---__  |     ~-/~         ( )   /'        _--~`                   \n"
        "                  \\_|      /        _) | ;  ),   __--~~                        \n"
        "                    '~~--_/      _-~/- |/ \\   '-~ \\                            \n"
        "                   {\\__--_/}    / \\\\_>-|)<__\\      \\                           \n"
        "                   /'   (_/  _-~  | |__>--<__|      |                          \n"
        "                  |   _/) )-~     | |__>--<__|      |                          \n"
        "                  / /~ ,_/       / /__>---<__/      |                          \n"
        "                 o-o _//        /-~_>---<__-~      /                           \n"
        "                 (^(~          /~_>---<__-      _-~                            \n"
        "                ,/|           /__>--<__/     _-~                                \n"
        "             ,//('(          |__>--<__|     /  -Alex Wargacki  .----_          \n"
        "            ( ( '))          |__>--<__|    |                 /' _---_~\\        \n"
        "         `-)) )) (           |__>--<__|    |               /'  /     ~\\`\\      \n"
        "        ,/,'//( (             \\__>--<__\\    \\            /'  //        ||      \n"
        "      ,( ( ((, ))              ~-__>--<_~-_  ~--____---~' _/'/        /'       \n"
        "    `~/  )` ) ,/|                 ~-_~>--<_/-__       __-~ _/                  \n"
        "  ._-~//( )/ )) `                    ~~-'_/_/ /~~~~~~~__--~                    \n"
        "   ;'( ')/ ,)(                              ~~~~~~~~~~                         \n"
        "  ' ') '( (/                                                                    \n"
        "    '   '  `";

extern void shell_main(void);

const char * delims = " \0";

const char * maybeOS_cmd = "maybeOS";
const char * echo_cmd = "echo";

//TODO: have implemented a few basic builtin funcs, need to do more. also need to implement a true parser, lexical analyzer, etc., the whole shebang. for now, this is good enough. 

static char * echo(char * input_param){
    char * output = input_param;
    return output;
}

static char * maybeOS(){
    return maybeOS_logo;
}



static char* parse_input(char * input){

}

void shell_input(char * input){

    size_t input_str_size = strlen(input);

    char * input_cpy = (char*) kmalloc(strlen(input) +1);
    strncpy(input_cpy, input, strlen(input));
    //TODO: this should obvi be a true parser with a dynamic container to hold the command and args, and a lookup table to store the command names with their correpsonding built-in functions
    char * cmd = strtok(input_cpy, delims);
    char * param = NULL;
    char* out;
    if(cmd){
        param = input + strlen(cmd) +1;
    
    if(strcmp(cmd, maybeOS_cmd) == 0){
        out = maybeOS();
    }
    else if(strcmp(cmd, echo_cmd) == 0){
        out = echo(param);
    }
    else{
        out = "Unrecognized command. Please try again. ";
    }
    }
    print_shell_output(out);
    kfree(input_cpy);
    return;
}

void shell_main(void){

    while(true){
        
    }
}