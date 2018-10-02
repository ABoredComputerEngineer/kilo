#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "error.c"
#include "key_def.h"
//#include "trash.c"

struct termios terminal_state;


void disable_raw_mode(void){
     tcsetattr(STDIN_FILENO,TCSAFLUSH,&terminal_state);
}
void enable_raw_mode(void){
     struct termios raw = terminal_state;
//     raw.c_lflag &= ~(ECHO); // Disable the ECHO bit, preventig display of input
     raw.c_iflag &= ~(IXON | ICRNL | INPCK | IGNBRK | ISTRIP);
     raw.c_lflag &= ~(ICANON | ISIG| IEXTEN);
     raw.c_oflag &= ~(OPOST);
     raw.c_cflag |= CS8;
     raw.c_lflag |=  IEXTEN;
     raw.c_cc[VMIN] = 1;
     raw.c_cc[VTIME] = 0;
     tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw);
     atexit(disable_raw_mode);
}

void input(){
     char c[2];
     int count = 0;
     while ( 1 ){
          read(STDIN_FILENO,c,1);
          count+=1;
          if ( *c == CTRL_Q )
               break;
          if ( iscntrl(*c) ){
               printf("%d ",*c);
          } else {
               printf("%d('%c') ",*c , *c );
          }
     }
     printf("\r\n%d\r\n",count);
     fflush(stdout);
}

int main(int argc, char *argv[]){
//     term_test();
     if ( tcgetattr(STDIN_FILENO,&terminal_state) == - 1 ){
          errExit("Unable to detect terminal for the stream!\n"); 
     }
     enable_raw_mode();
     input();
     //disable_raw_mode();
     input();
     return 0;
}
