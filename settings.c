
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
     struct termios terminal;
} term_info;

term_info active_term;

void disable_raw_mode(void){
     tcsetattr(STDIN_FILENO,TCSAFLUSH,&active_term.terminal);
}

void enable_raw_mode(void){
     struct termios raw = active_term.terminal;
     raw.c_lflag &= ~(ECHO); // Disable the ECHO bit, preventig display of input
     raw.c_iflag &= ~(IXON | ICRNL | INPCK | IGNBRK | ISTRIP);
     raw.c_lflag &= ~(ICANON | ISIG| IEXTEN);
     raw.c_oflag &= ~(OPOST);
     raw.c_cflag |= CS8;
     raw.c_lflag |=  IEXTEN;
     raw.c_cc[VMIN] = 0;
     raw.c_cc[VTIME] = 1;
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
