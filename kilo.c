#include <assert.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "error.c"
#include "key_def.h"
#include "settings.c"
//#include "trash.c"
#include "common.c"
#include "string.c"
extern struct term_info active_term;
extern void disable_raw_mode(void);
extern void enable_raw_mode(void);
extern void get_terminal_size(void);
size_t position = 0;
enum { BUFF_LEN = 256 };
void kilo_exit(void){
     disable_raw_mode();
     write(STDOUT_FILENO,"\x1b[2J",4);
     write(STDOUT_FILENO,"\x1b[H",3);
//     write(STDOUT_FILENO,"\x1b[?47l",6);
     exit(EXIT_SUCCESS);
}



/** input **/
void move_cursor(int c){
     switch ( c ){
          case ARROW_UP:
               if ( active_term.pos.row - 1 > 0 ) {
                    active_term.pos.row--;
               }
               break;
          case ARROW_DOWN:
               if ( active_term.pos.row + 1 <= active_term.ws.row ){
                    active_term.pos.row++;
               }
               break;
          case ARROW_LEFT:
               if ( active_term.pos.col - 1 >= 2 ){
                    active_term.pos.col--;
               }
               break;
          case ARROW_RIGHT:
               if ( active_term.pos.col + 1 < active_term.ws.col ){
                    active_term.pos.col++;
               }
               break;
     }
}
char get_key_press(void){
     int key = 0;
     char c;
     while ( ( key = read(STDIN_FILENO,&c,1) ) != 1 );
     if ( c == '\x1b' ){
          char seq[3];
          if ( read(STDIN_FILENO,&seq[0],1) != 1 )
               return c;
          if ( read(STDIN_FILENO,&seq[1],1) != 1 )
               return c;
          if ( seq[0] == '[' ){
               switch ( seq[1] ){
                    case 'A':
                         return ARROW_UP;
                         break;
                    case 'B':
                         return ARROW_DOWN;
                         break;
                    case 'C':
                         return ARROW_RIGHT;
                         break;
                    case '3':
                    case '2':
                         if ( read(STDIN_FILENO,&seq[2],1) != 1 ){
                              return c;
                         }
                         if ( seq[2] == '~' ){
                              return DELETE;
                         }
                         break;
                    case 'D':
                         return ARROW_LEFT;
                         break;
                    case '5':
                         return PAGE_UP;
                         break;
                    case '6':
                         return PAGE_DOWN;
                         break;
                    case '1' : case '7' : case '4' : case '8': {
                         if ( read(STDIN_FILENO,&seq[2], 1 ) != 1 ){
                              return c;
                         }
                         if (  seq[2] == '~' ){
                              return ( seq[1] == '1' || seq[1] == '7' )?HOME:END;
                         }
                         break;
                    }
                    case 'H':
                         return HOME;
                         break;
                    case 'F':
                         return END;
                         break;
                    case 'O':
                         if ( read(STDIN_FILENO,&seq[2],1) != 1 ){
                              return c;
                         }
                         if ( seq[2] == 'H' ){
                              return HOME;
                         } else if ( seq[2] == 'F' ){
                              return END;
                         }
               }
          }
     }
     return c;
}

void process_key(void){
     char c = get_key_press();
     switch ( c ){
          case CTRL_KEY('q'):
               kilo_exit();
               break;
          case CTRL_KEY('m'):
               active_term.pos.row++;
               active_term.pos.col = 2;
               break;
          case ARROW_LEFT:
          case ARROW_DOWN:
          case ARROW_RIGHT:
          case ARROW_UP:
               move_cursor(c);
               break;
          case HOME:
               active_term.pos.col = 2;
               break;
          case END:
               active_term.pos.col = active_term.ws.col;
               break;
          case PAGE_UP:
               active_term.pos.row = 1;
               break;
               break;
          case PAGE_DOWN:
               active_term.pos.row = active_term.ws.row;
               break;
          default:{
               move_cursor(ARROW_RIGHT);
               break;
          }
     }
}

/** output **/

void print_tidles(char *new){
     char buff[BUFF_LEN] = "";

     for ( size_t i = 0; i < active_term.ws.row - 1; i++ ){
          sprintf(buff,"%s~\r\n",buff);
     }
     str_append(new,buff);
     str_append(new,"~");
}
void refresh_screen(void){
     char *s1 = NULL;
     str_init(s1);
     str_append(s1,"\x1b[2K""\x1b[H");
     print_tidles(s1);
     str_app_print( s1, "\x1b[%d;%dH",active_term.pos.row, active_term.pos.col ); 
     if ( write(STDOUT_FILENO,s1,str_len(s1)) == -1 ){
          errExit("Error writing to stdout!\n");
     }
     str_free(s1);
}
void center_print(const char *message, int width, int row){
     while ( isspace(*message) ){
          message++;
     }
     size_t len = strlen(message);
     if ( len == 0 ){
          return;
     } else if ( width > active_term.ws.col ){
          center_print(message,active_term.ws.col,row);
          return;
     }
     if ( len < width ){
          center_print(message,len,row);
     } else {
          size_t mlen= width;
          if ( *(message + width) != ' ' &&  *(message+width) != '\0' ){
               for ( const char *s = message + width;\
                         *s != ' ' && s != message ;\
                         s-- ){
                    mlen--;
               }
               mlen = ( mlen == 0 )?width:mlen;
               assert( mlen>= 0 );
          }
          char buff[BUFF_LEN];
          int lpad = (active_term.ws.col - mlen)/2 ; 
          int x = snprintf(buff,BUFF_LEN,"\x1b[%d;%dH",row,lpad);
          write(STDOUT_FILENO,buff,x);
          printf("%.*s",(int)mlen,message);
          fflush(stdout);
          center_print(message+mlen,width,row+1); 
     }
}
void print_welcome(void){
     center_print("Welcome to this useless shit! Press any key to continue...",20, active_term.ws.row/2 - 2);
     write(STDOUT_FILENO,"\x1b[H",3);
}



int main(int argc, char *argv[]){
     if ( tcgetattr(STDIN_FILENO,&active_term.terminal) == - 1 ){
          errExit("Unable to detect terminal for the stream!\n"); 
     }
     enable_raw_mode();
     int count = 0;
     get_terminal_size();
     refresh_screen();
     print_welcome();
     char tmp;
     while ( read(STDIN_FILENO,&tmp,1) != 1 ); 
     write(STDIN_FILENO,"\x1b[2J\x1b[1;2H",4+6);
     if ( get_cursor_pos( &active_term.pos.row, &active_term.pos.col ) == -1 ){
          errExit( " Error getting cursor position!\n" );
     }
     while ( 1 ){
         refresh_screen();
//         printf("%d",count);
//         fflush(stdout);
//         count++;
      
         process_key();
    }

     return 0;
}
