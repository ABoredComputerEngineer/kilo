
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
     if ( x ){
          return x;
     } 
     errExit("Unable to allocate memory!\n");
}

void *xrealloc( void *buff,size_t size ){
     void *new = realloc( buff,size );
     if ( !new ){
          errExit("Unable to allocate memory!\n");
     }
     assert( new != NULL );
     return new;
}

