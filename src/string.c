#include <kilo_string.h>

#if 0
static void *xmalloc( size_t size ){
     void *x = malloc(size);
     if ( !x ){
//          errExit("Unable to allocate memory!\n");
          perror("Unable to allocate Memory for Strings! ");
     }
     assert( x != NULL );
     return x;
}
#endif

static void *xcalloc(size_t num, size_t size){
     void *x = calloc( num, size );
     if ( !x ){
          perror("Unable to allocate Memory for Strings! ");
     } 
     return x;
}

static void *xrealloc( void *buff,size_t size ){
     void *new = realloc( buff,size );
     if ( !new ){
          perror("Unable to allocate Memory for Strings! ");
     }
     assert( new != NULL );
     return new;
}
char* _init_string( char *str ){
     if ( str != NULL ){
          fprintf(stderr,"Not a null string pointer!\n");
          return NULL;
     } else {
          strHdr *new = xcalloc( 1,sizeof(strHdr ) + MIN_SIZE );
          assert ( new );
          new->len = 0;
          new->cap = MIN_SIZE;
          assert( new->cap >= new->len + 1 ); // the invariant for the whole string process
          return new->str;
     }
}

char *string_grow( strHdr *x, size_t new){
     char *y = NULL;
     if ( x == NULL ){
          str_init(y);
          x = str_hdr(y);
     }
     assert( x );
     size_t alloc_size = MAX( 2*(x->cap) + 1, MAX( MIN_SIZE, new ) );
     assert( alloc_size >= x->cap );
     strHdr *n = xrealloc( x , alloc_size + offsetof( strHdr, str) );
     n->cap = ( alloc_size );
     assert( n->cap >= n->len + 1 ); // the invariant for the whole string process
     return n->str;
}

char *_string_append(char *x, const char *y){
     size_t len = strlen(y);
     size_t size = str_size(x);
     if ( str_size(x) + len > str_cap(x) ){
          x = string_grow( str_hdr(x), size + len );
     }

     assert( str_cap(x) >= size + len );
     memcpy( x + str_len(x) , y , len + 1 );
     str_hdr(x)->len += len;
     return x;
}

char *_string_print(char *x, const char *fmt, ... ){
     va_list args;
     va_start(args,fmt);
     size_t cap = str_cap(x);
     size_t len = vsnprintf(x,cap,fmt,args);
     va_start(args,fmt);
     if ( ( len + 1 ) * sizeof(char)  > cap ){
          x = string_grow( str_hdr(x) , len + 1);
          cap = str_cap(x);
          len = vsnprintf(x,cap,fmt,args);
          assert( len <= cap);
     }
     va_end(args);
     str_hdr(x)->len = len;
     return x;
}

char *_string_app_print( char *x , const char *fmt ,... ){
     va_list args;
     va_start(args,fmt);
     size_t size = str_size(x);
     size_t cap = str_cap(x);
     size_t len = vsnprintf( x + size - 1, cap - size , fmt, args );
     va_start(args,fmt);
     if ( ( len + 1 )*sizeof(char) > cap - size  ){
          x = string_grow( str_hdr(x), cap + size + len );
          cap = str_cap(x);
          len = vsnprintf( x + size - 1, cap - size, fmt, args );
          assert( len <= cap - size );
     }
     va_end(args);
     str_hdr(x)->len += len;
     return x;
}

void str_test(void){
     char *s1 = NULL;  
     str_init(s1);
     str_append(s1,"fuck");
     str_app_print( s1, "%s%d\n", "this",32 );
     str_print( s1, "%s\n","Working!");
     //str_append(s1," this shit\n");
     //for ( int i = 0; i < 32; i++ ){
     //     str_append(s1,"!");
     //}
     //str_append(s1,"\n");
     //printf("%s",s1);
//     str_print(s1,"%s\n","fuck");
     printf("%s",s1); 
     str_free(s1);
}



