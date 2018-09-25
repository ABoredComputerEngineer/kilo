#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include "trash.c"

struct termios terminal_state;

void enable_raw_mode(void){
     struct termios raw = terminal_state;
     raw.c_lflag &= ~(ECHO); // Disable the ECHO bit, preventig display of input
     tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw);
}

void disable_raw_mode(void){
     tcsetattr(STDIN_FILENO,TCSAFLUSH,&terminal_state);
}


int main(int argc, char *argv[]){
//     term_test();
     tcgetattr(STDIN_FILENO,&terminal_state);
     enable_raw_mode();
     input();
     disable_raw_mode();
     input();
     atexit(disable_raw_mode);
     return 0;
}
