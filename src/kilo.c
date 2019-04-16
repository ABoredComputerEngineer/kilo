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

#define TEST 1

enum { 
     MAIN_BUFF_LEN= 5*1024*1024, // 5 MB for the main buffer
     DEFAULT_LEN = 256, 
     MAX_LINES = 256,
     BUFF_COLUMNS = 256,
};

FILE *log_file;
#define LOG_ERROR( ... )  fprintf( log_file,__VA_ARGS__ ) 
#define LOG_LINE fprintf( log_file,"\n=============================================\n" );

typedef struct StoreBuffer {
     char **buff; // buffer that will contains 'strings' as defined in string.c and kilo_string.h
     size_t lines;
} StoreBuffer;
StoreBuffer main_buffer;

struct termios terminal_state;
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

/*
 * =========================================
 * | TERMINAL SETTINGS |
 * =========================================
 */
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

term_info active_term;

void disable_raw_mode(void){
     tcsetattr(STDIN_FILENO,TCSAFLUSH,&active_term.terminal);
}

void enable_raw_mode(void){
     struct termios raw = active_term.terminal;
     //raw.c_lflag &= ~(ECHO); // Disable the ECHO bit, preventig display of input
     raw.c_iflag &= ~(IXON | ICRNL | INPCK | IGNBRK | ISTRIP);
     raw.c_lflag &= ~(ICANON | ISIG| IEXTEN);
#if  TEST
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
     xfree( (void **)&( main_buffer.buff ) );
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

/*
typedef struct StoreBuffer{
     char*  buff;
     size_t current_pos; // where the cursor currently is 
     size_t len; // the length of the string in buff 
} StoreBuffer;
*/

#if 0
void buff_write( char c ){
}
#endif

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
          default:{
               //buff_write( c );
               move_cursor(ARROW_RIGHT);
               break;
          }
     }
}

/** output **/

void print_tidles(char *new){
     char buff[DEFAULT_LEN] = "";

#if 0
     for ( size_t i = 0; i < active_term.ws.row - 1; i++ ){
          sprintf(buff,"%s~\r\n",buff);
     }
#else 
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
#endif
     str_append(new,buff);
     str_append(new,"~");
}

#define CLEAR_LINE "\x1b[2K"
#define CLEAR_SCREEN "\x1b[J"
#define CURSOR_HOME "\x1b[H"
#define SET_CURSOR(x,y) "\x1b[" #x ";" #y "H" // sets cursor from top left

#define SET_CURSOR_FORMAT_STR "\x1b[%d;%dH" // sets cursor from top left

#define CURSOR_STR_LEN(d) ( 4 + ( d ) ) // arguments to CURSOR_STR_LEN is total number of digits in the x and y position combined


#define BUFF_LINE( X ) ( &main_buffer.buff[ BUFF_COLUMNS * (X -1) ] ) 

void refresh_screen(void){
     char *s1 = NULL;
     str_init(s1);
//     str_append(s1,"\x1b[2K""\x1b[H");     
     str_append( s1, CLEAR_LINE CURSOR_HOME ); // clear the line and set cursor to home position
     print_tidles(s1);
     str_app_print( s1,SET_CURSOR_FORMAT_STR ,active_term.pos.row, active_term.left_padding + 1 );  // set cursor to the begining of current row
     //LOG_ERROR("Length of Line #%d : %zu\n", active_term.pos.row, main_buffer.line_len[ active_term.pos.row ]  );
     //LOG_ERROR("String printed: %s\n",BUFF_LINE(active_term.pos.row) );
     //strn_app_print( s1, main_buffer.line_len[ active_term.pos.row ] ,"%s",BUFF_LINE(active_term.pos.row)  );
     str_app_print( s1,SET_CURSOR_FORMAT_STR ,active_term.pos.row, active_term.pos.col );  // arguments are row followed by columns
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

#if 0
void init_empty_buffer(StoreBuffer *s, size_t lines, size_t columns){
     s->buff = xmalloc( lines * sizeof( *(s->buff) ) ); // sizeof(char *)
     s->line_len = xcalloc( lines , sizeof( *(s->line_len) ) ); // sizeof( size_t )
     s->line_cap = xcalloc( lines, sizeof( *(s->line_cap ) ) ); // sizeof ( size_t  )
     size_t *cap = s->line_cap;
     for ( char **x = s->buff; x != s->buff + lines ; x++ ){
          *x = xmalloc( columns * sizeof( char ) ); // sizeof( **x ) 
          *cap++ = columns;
}
#endif

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
#if 0
     active_term.left_padding = 2;
     active_term.pos.row = 1;
     active_term.pos.col = 1;
     
//     init_empty_buffer(&main_buffer);

     if ( tcgetattr(STDIN_FILENO,&active_term.terminal) == - 1 ){
          errExit("Unable to detect terminal for the stream!\n"); 
     }
     enable_raw_mode();
     get_terminal_size();
     write( STDIN_FILENO, "\x1b[2J",4 ); // clear the screen
     refresh_screen();
     print_welcome();
     write( STDIN_FILENO,"\x1b[1;3H",6 );
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
#if 0
         refresh_screen();
         read( STDIN_FILENO, str, 256 );
         printf("%d",str[0]);
         fflush(stdout);
#else
         refresh_screen();
         process_key();
#endif
     }
#endif
     return 0;
}

