#include <assert.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <key_def.h>
#include <kilo_string.h>
#include <error.h>

#define DISPLAY_KEY_CODES 0

enum { 
     MAIN_BUFF_LEN= 5*1024*1024, // 5 MB for the main buffer
     DEFAULT_LEN = 256, 
     MAX_LINES = 256,
     MAX_COLUMNS= 256,
};

FILE *log_file;
#define LOG_ERROR( ... )  fprintf( log_file,__VA_ARGS__ ) 
#define LOG_LINE fprintf( log_file,"\n=============================================\n" );

#define CLEAR_LINE "\x1b[2K"
#define CLEAR_SCREEN "\x1b[J"
#define CURSOR_HOME "\x1b[H"
#define SET_CURSOR(x,y) "\x1b[" #x ";" #y "H" // sets cursor from top left
#define SET_CURSOR_FORMAT_STR "\x1b[%d;%dH" // sets cursor from top left
#define CURSOR_STR_LEN(d) ( 4 + ( d ) ) // arguments to CURSOR_STR_LEN is total number of digits in the x and y position combined
typedef struct StoreBuffer {
     char **buff; // buffer that will contains 'strings' as defined in string.c and kilo_string.h
     size_t lines;
} StoreBuffer;
StoreBuffer main_buffer;

struct termios terminal_state;

typedef struct term_info {
     struct {
          int row;
          int col;
     } ws;
     struct {
          int row;
          int col;
     } pos;
     int left_padding;
     struct termios terminal;
} term_info;
void *xmalloc( size_t size ){
     void *x = malloc(size);
     if ( !x ){
          errExit("Unable to allocate memory!\n");
     }
     assert( x != NULL );
     return x;
}

void *xcalloc(size_t num, size_t size){
     void *x = calloc( num, size );
     if ( !x ){
          errExit("Unable to allocate memory!\n");
     }
     return x; 
}

void *xrealloc( void *buff,size_t size ){
     void *new = realloc( buff,size );
     if ( !new ){
          errExit("Unable to allocate memory!\n");
     }
     assert( new != NULL );
     return new;
}

void xfree( void **buff ){
     assert( buff );
     if ( *buff == NULL ){
          LOG_ERROR( "\nFATAL: Trying to free from a null pointer!\n" );
          return;
     }
     free( *buff );
     *buff = NULL;
}

/* ==========================================================================================
 * | BUFFER FUNCTIONS |
 * ==========================================================================================
 */
void init_empty_buffer(StoreBuffer *s, size_t lines, size_t columns){
     s->lines = lines;
     s->buff = xcalloc( lines , sizeof( *(s->buff) ) ); // sizeof(char *)
     for ( char **x = s->buff; x != s->buff + lines ; x++ ){
          assert( *x == NULL );
          str_init_size( *x, columns );
     }
}

void buff_destroy( StoreBuffer *s ){
     for ( char **it = s->buff; it != s->buff + s->lines ; it++ ){
          str_free( *it );
          assert( *it == NULL );
     }
     free( s->buff );
     s->buff = NULL;
     s->lines = 0;
}

char *buff_get_current_line(){
     extern struct term_info active_term;
     // get the line that the cursor currently is in
     assert( active_term.pos.row > 0 );
     return main_buffer.buff[ active_term.pos.row - 1 ];
}

char *buff_get_line(int row){
     extern struct term_info active_term;
     // get the line that the cursor currently is in
     assert( active_term.pos.row > 0 );
     return main_buffer.buff[ row - 1 ];
}

/*======================== END OF BUFFER FUNCTIONS =========================================*/

/*
 * =========================================
 * | TERMINAL SETTINGS |
 * =========================================
 */

term_info active_term;

void disable_raw_mode(void){
     tcsetattr(STDIN_FILENO,TCSAFLUSH,&active_term.terminal);
}

void enable_raw_mode(void){
     struct termios raw = active_term.terminal;
     raw.c_lflag &= ~(ECHO); // Disable the ECHO bit, preventig display of input
     raw.c_iflag &= ~(IXON | ICRNL | INPCK | IGNBRK | ISTRIP);
     raw.c_lflag &= ~(ICANON | ISIG| IEXTEN);
#if  DISPLAY_KEY_CODES 
     raw.c_lflag |= (ECHO); // Enable the ECHO bit, for testing purposes 
     raw.c_lflag |= ISIG; // this is set only for testing puposes
#endif
     raw.c_oflag &= ~(OPOST);
     raw.c_cflag |= CS8;
     raw.c_lflag |=  IEXTEN;
     raw.c_cc[VMIN] = 1;
     raw.c_cc[VTIME] = 0;
     tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw);
     atexit(disable_raw_mode);
}

void get_terminal_size(void){
     enum { BUFF_SIZE = 256 };
     if ( write(STDOUT_FILENO,"\x1b[999B\x1b[999C\x1b[6n",16) != 16  ){
          errExit("Error determining terminal size!\n");
     }
     char c[BUFF_SIZE]  = {};
     size_t i = 0,k;
     while (  ( k = read(STDIN_FILENO,c + i, 1) ) == 1 ){
          if ( *(c + i ) == 'R' ){
               break;
          }
          ++i;
     }
     if ( sscanf(c,"\x1b[%d;%dR",&active_term.ws.row,&active_term.ws.col) != 2 ){
          errExit("Error determining terminal size!\n");
     }
    write(STDOUT_FILENO,"\x1b[H",3);
}

int get_cursor_pos( int *row, int *col ){
     enum { BUFF_SIZE = 256 };
     if ( write(STDOUT_FILENO,"\x1b[6n",4) != 4){
          return -1;
     }
     char c[BUFF_SIZE]  = {};
     size_t i = 0,k;
     while (  ( k = read(STDIN_FILENO,c + i, 1) ) == 1 ){
          if ( *(c + i ) == 'R' ){
               break;
          }
          ++i;
     }
     if ( sscanf(c,"\x1b[%d;%dR",row,col) != 2 ){
          return -1;
     }
     return 1;
}

// ===============================================================================


void kilo_exit(void){
     disable_raw_mode();
     write(STDOUT_FILENO,"\x1b[2J",4);
     write(STDOUT_FILENO,"\x1b[H",3);
//     write(STDOUT_FILENO,"\x1b[?47l",6);
     fclose( log_file );
     buff_destroy( &main_buffer );
     exit(EXIT_SUCCESS);
}

void print_line(char *str, int row ){
     char *buff = buff_get_line(row);
     str_app_print( str,SET_CURSOR_FORMAT_STR ,row, active_term.left_padding + 1 );  // set cursor to the begining of current row
     str_append( str, buff );
}

/** input **/
void move_cursor(int c){
     switch ( c ){
          case ARROW_UP:
               if ( active_term.pos.row - 1 > 0 ) {
                    active_term.pos.row--;
                    char *x = buff_get_line( active_term.pos.row );
                    int new_pos = str_len( x ) + 1 + active_term.left_padding;
                    active_term.pos.col = ( active_term.pos.col < new_pos )? active_term.pos.col : new_pos  ;
               }
               break;
          case ARROW_DOWN:
               if ( active_term.pos.row + 1 <= active_term.ws.row ){
                    active_term.pos.row++;
                    char *x = buff_get_line( active_term.pos.row );
                    int new_pos = str_len( x ) + 1 + active_term.left_padding;
                    active_term.pos.col = ( active_term.pos.col < new_pos )? active_term.pos.col : new_pos  ;
               }
               break;
          case ARROW_LEFT:
               if ( active_term.pos.col - 1 >= active_term.left_padding + 1 ){
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

#define CASE1( X , Y ) \
     case X :\
          LOG_ERROR("Key Entered :" #Y "\n" );\
          return Y;


char get_key_press(void){
     int key = 0;
     char c;
     while ( ( key = read(STDIN_FILENO,&c,1) ) != 1 ); // loop until a key is pressed
     if ( c == '\x1b' ){ // if the key is an escape sequence which is the string "^["
          char seq[5];
          if ( read(STDIN_FILENO,&seq[0],1) != 1 )
               return c;
          if ( seq[0] == '[' ){ // the input escape sequence is ^[ [
               if ( read(STDIN_FILENO,&seq[1],1) != 1 ) return c;
               switch ( seq[1] ){
                    CASE1('A',ARROW_UP)
                    CASE1('B',ARROW_DOWN)
                    CASE1('C',ARROW_RIGHT)
                    CASE1('D',ARROW_LEFT)
                    CASE1('H',HOME)
                    CASE1('F',END)
                    case '1': case '2': case '3': case '4': case '5': case  '6': case '7': case '8': case '9':{
                         int d;
                         if ( ( d = read( STDIN_FILENO, &seq[2],2) ) < 1 ){ // Try to read at least one character
                              return c;
                         }
                         if ( d == 1 ){
                              assert( seq[2] == '~' );
                              LOG_ERROR( "Sequence entered: <Esc>%c%c%c\t",seq[0],seq[1],seq[2] ); 
                              switch ( seq[1] ){
                                   CASE1( '1', HOME )
                                   CASE1( '2', INSERT)
                                   CASE1( '3', DELETE )
                                   CASE1( '4', END )
                                   CASE1( '5', PAGE_UP )
                                   CASE1( '6', PAGE_DOWN )
                                   CASE1( '7', HOME )
                                   CASE1( '8', END )
                                   default:
                                        LOG_ERROR( "At Line: %d, Invalid sequence entered: \n",__LINE__);
                                        return HOME;
                                        break;
                              }
                         } else if ( d == 2 ){
                              assert( seq[3] == '~' );
                              int key = ( seq[1] & 0xf ) * 10  + ( seq[2] & 0xf );  // convert "ab" ( string ) to ab ( number )
                              LOG_ERROR( "Sequence entered: <Esc>%c%c%c%c\t",seq[0],seq[1],seq[2],seq[3] ); 
//                              LOG_ERROR( "Value of d : %d, key : %d \n", d, key );
                              switch ( key ){
                                   CASE1( 15, F5 )
                                   CASE1( 17, F6 )
                                   CASE1( 18, F7 )
                                   CASE1( 20, F9 )
                                   CASE1( 21, F10 )
                                   CASE1( 24, F12 )
                                   default:
                                        LOG_ERROR( "At Line: %d, Invalid sequence entered:\n",__LINE__);
                                        LOG_ERROR( "Value of d : %d, key : %d \n", d, key );
                                        return HOME;
                                        break;
                              }
                         }
                    }
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


void buff_write( char c ){
     char *x = buff_get_current_line();
     int col = active_term.pos.col - active_term.left_padding ;
     assert( col > 0 );
     if ( str_len(x) > 0 && ( col - 1 ) < str_len(x) ){
          str_insert_char( x,c,col-1 ); // insert character 'c' to the left of given pos ( col - 1 )
          //x[col - 1] = c;
     } else {
          str_app_char( x, c ); // add character to the end
     }
     LOG_ERROR("Writing character %c in index %d\n", c, active_term.pos.row - 1 );
}

void delete_left_char(){
     char *x = buff_get_current_line( );
     int col = active_term.pos.col - active_term.left_padding; // the column position of the cursor
     if ( str_len(x) > 0 && col > 1 ){
          assert( col - 2 >= 0 );
          str_delete_char_pos( x , col - 2 );
     }
}


void process_key(void){
     char c = get_key_press();
     switch ( c ){
          case CTRL_KEY('q'):
               kilo_exit();
               break;
          case CTRL_KEY('m'):
               active_term.pos.row++;
               active_term.pos.col = active_term.left_padding + 1;
               break;
          case ARROW_LEFT:
          case ARROW_DOWN:
          case ARROW_RIGHT:
          case ARROW_UP:
               move_cursor(c);
               break;
          case HOME:
               active_term.pos.col = 2;
               active_term.pos.row = 1;
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
          case BACKSPACE:
               delete_left_char( );
               move_cursor( ARROW_LEFT );
               LOG_ERROR( "Hit the backspace!\n" );
               break;
          default:{
               if ( isprint(c) ){
                    buff_write( c );
               }
               move_cursor(ARROW_RIGHT);
               break;
          }
     }
}

/** output **/

void print_tidles(char *new){
     char buff[DEFAULT_LEN] = "";
     char *s = buff;
     for ( int row = 0;\
               ( row < active_term.ws.row - 1 ) && ( s < buff+DEFAULT_LEN );\
               row++ ){
          for ( int j = 0; j < active_term.left_padding; j++ ){
               *s++ = '~';
          }
          *s++ = '\r';
          *s++ = '\n';
     }
     str_append(new,buff);
     str_append(new,"~");
}




void refresh_screen(void){
     assert( active_term.pos.row > 0 );
     char *s1 = NULL;
     str_init(s1);
     str_app_print( s1,SET_CURSOR_FORMAT_STR ,active_term.pos.row, active_term.pos.col ); // set the cursor to the new line
     str_append( s1, CLEAR_LINE CURSOR_HOME ); // clear the line and set cursor to home position
     print_tidles(s1); // print tiddles, the cursor has to be in HOME position
     print_line( s1, active_term.pos.row ); // print the line in the current row
//     LOG_ERROR("String printed: %s\n",buff );
     str_app_print( s1,SET_CURSOR_FORMAT_STR ,active_term.pos.row, active_term.pos.col ); // set the cursor to the current position
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
          char buff[DEFAULT_LEN];
          int lpad = (active_term.ws.col - mlen)/2 ; 
          int x = snprintf(buff,DEFAULT_LEN,"\x1b[%d;%dH",row,lpad);
          write(STDOUT_FILENO,buff,x);
          printf("%.*s",(int)mlen,message);
          fflush(stdout);
          center_print(message+mlen,width,row+1); 
     }
}
void print_welcome(void){
     center_print("Welcome to this useless shit! Press any key to continue...",20, active_term.ws.row/2 - 2);
     write( STDOUT_FILENO,SET_CURSOR(1,2), CURSOR_STR_LEN(2) );      
     write(STDOUT_FILENO,"\x1b[1;2H",6);
}


void buff_init_test( ){
     StoreBuffer s;
     enum { TEST_LINES = 10, TEST_COLS = 30 };
     init_empty_buffer( &s, TEST_LINES, TEST_COLS );
     char **x = s.buff;
     for ( size_t i = 0 ; i < TEST_LINES ; i++ ){
          assert( str_cap( *x ) == TEST_COLS );
          assert( str_len( *x ) == 0 ); 
          x++;
     }
     LOG_ERROR( "Buffer Allocated successfully!\nAdding data to Buffer \n" );
     for ( char **it = s.buff; it != s.buff + TEST_LINES ; it++ ){
          str_append( *it, "Fuck this shit" );
     }
     str_append( s.buff[3], " No! Fuck THIS shit" );
     str_append( s.buff[4], " Fuuuuuuuck!" );
     LOG_ERROR( "Testing Buffer... \n" );
     LOG_ERROR( "Lines allocated : %d , Columns allocated: %d\n", TEST_LINES, TEST_COLS );
     LOG_ERROR( "Contents are:\n ");
     for ( char **it = s.buff; it != s.buff + TEST_LINES ; it++ ){
          int x = ( int ) ( it - s.buff );
          LOG_ERROR( "Line %d, Length : %zu, Capacity: %zu :\n%s\n\n", x, str_len( *it ), str_cap( *it ), *it ); 
     }
     LOG_ERROR( "Finished Buffer Testing" );
     LOG_LINE;
     buff_destroy( &s );
}


int main(int argc, char *argv[]){
     log_file = fopen( "test/log.txt", "w" );
     str_test();
     buff_init_test();
#if 1
     active_term.left_padding = 1;
     active_term.pos.row = 1;
     active_term.pos.col = 1;
     
     init_empty_buffer(&main_buffer,MAX_LINES,MAX_COLUMNS);

     if ( tcgetattr(STDIN_FILENO,&active_term.terminal) == - 1 ){
          errExit("Unable to detect terminal for the stream!\n"); 
     }
     enable_raw_mode();
     get_terminal_size();
     write( STDIN_FILENO, "\x1b[2J",4 ); // clear the screen
     refresh_screen();
     print_welcome();
     write( STDIN_FILENO,"\x1b[1;2H",6 );
     char tmp;
     while ( read(STDIN_FILENO,&tmp,1) != 1 );  // Until the user presses a key keep displaying welcome message
//     write(STDIN_FILENO, CLEAR_SCREEN SET_CURSOR(1,3) , 4 + 6 ); // ( 4 + 6 ) is the length of the string to be written
     write(STDIN_FILENO,"\x1b[2J\x1b[1;3H",4+6); // clear the screen and put cursor at home position
     if ( get_cursor_pos( &active_term.pos.row, &active_term.pos.col ) == -1 ){
          errExit( " Error getting cursor position!\n" );
     }
     char str[256] = {};
     (void)str;
     while ( 1 ){
#if DISPLAY_KEY_CODES
         read( STDIN_FILENO, str, 256 );
         printf("%d ", str[0] );
         fflush( stdout );
#else
         refresh_screen();
         process_key();
#endif
     }
#endif
     return 0;
}

