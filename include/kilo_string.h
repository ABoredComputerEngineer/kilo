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
#define str_append(x,y) ( x = _string_append(x,y) )
#define str_print(x,y,...) ( x = _string_print(x, y, __VA_ARGS__ ) )
#define str_app_print(x, y , ... ) ( x = _string_app_print( x, y , __VA_ARGS__ ) )
#define strn_app_print(x,n, y , ... ) ( x = _stringn_app_print( x,n, y , __VA_ARGS__ ) )

char *_init_string( char *str );
char *string_grow( strHdr *x, size_t new );
char *_string_append( char *x, const char *y );
char *_string_print( char *x, const char *fmt, ... );
char *_string_app_print( char *x, const char *fmt, ... );
char *_stringn_app_print( char *x, size_t n, const char *fmt, ... );
void str_test( void );
#endif
