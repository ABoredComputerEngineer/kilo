#ifndef KILO_STRING_H

#define KILO_STRING_H

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <stdarg.h>

typedef struct strHdr {
     size_t cap; // The total number of bytes the current buffer can hold ( not inc. size of struct )
     size_t len; // The length of the string in the buffer
     char str[0];
} strHdr;
#define MAX(x,y) ( ( ( x ) > ( y ) ) ? ( x ) : ( y ) )
enum { MIN_SIZE = 256 };

#define str_hdr(x) ( (strHdr *)( (char *)x  - offsetof( strHdr , str ) ) )
#define str_len(x) ( ( x )?( str_hdr(x)->len ):0 ) // the string length i.e. w/o NULL character
#define str_size(x)  ( ( x )?( str_hdr(x)->len + 1 )*sizeof(*x): 0 ) // the string size i.e w/ NULL character
#define str_cap(x) ( ( x )?str_hdr(x)->cap:0 )
#define str_free(x) ( free( str_hdr(x) ), x = NULL )
#define str_init(x) ( x = _init_string( x ) )
#define str_init_size( x, size ) ( x = _init_string_with_size( x, size ) )
#define str_append(x,y) ( x = _string_append(x,y) )
#define str_print(x,y,...) ( x = _string_print(x, y, __VA_ARGS__ ) )
#define str_app_print(x, y , ... ) ( x = _string_app_print( x, y , __VA_ARGS__ ) )
#define strn_app( x, n , y ) ( x = _strn_append( x, n , y ) )
//#define strn_app_print(x,n, y , ... ) ( x = _stringn_app_print( x,n, y , __VA_ARGS__ ) )
#define str_app_char( x, c ) ( x = _string_app_char( x, c ) )
#define str_insert_char(x,c,pos) ( (x) = _insert_char( x, c, pos ) ) 
#define str_delete_char_pos( x, pos ) ( _delete_char_pos(x, pos ) ) 

char *_init_string( char *str );
char *_init_string_with_size( char *str, size_t size );
char *string_grow( strHdr *x, size_t new );
char *_string_append( char *x, const char *y );
char *_string_print( char *x, const char *fmt, ... );
char *_string_app_print( char *x, const char *fmt, ... );
char *_strn_append( char *dest, size_t n, char *src );
//char *_stringn_app_print( char *x, size_t n, const char *fmt, ... );
char *_string_app_char( char *x, char c ); // append a single character at the end of string
char *_insert_char( char *str, char c, size_t pos );
void _delete_char_pos(char *str, size_t pos );
void str_test( void );
#endif
