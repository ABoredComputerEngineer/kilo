#include <gnu/libc-version.h>
#include "error.h"
extern void disable_raw_mode(void);
#define exit(x) ( disable_raw_mode(), exit(x) )
static void terminate(bool useExit2){
     char *s;
     s = getenv("EF_DUMPCORE");
     if ( s && *s != '\0' ){
          abort();
     } else if ( useExit2 ){
          _exit(EXIT_FAILURE);
     } else {
          exit(EXIT_FAILURE);
     }
}

static void outErrMsg(bool useErrText, int err, bool flushStdout,const char *fmt, va_list argList){
     enum { BUFF_SIZE = 512 };
    /* char buff[BUFF_SIZE], userMsg[BUFF_SIZE], errText[BUFF_SIZE];
     vsnprintf(userMsg,BUFF_SIZE,fmt,argList);
     if ( useErrText ){
          snprintf(errText,BUFF_SIZE,"[%s %s]",\
                    ( err>0 && err<= MAX_ENAME )?ename[err]:"?UNKOWN?",\
                    strerror(err));
     } else {
          snprintf(errText,BUFF_SIZE,":");
     }
     snprintf(buff,BUFF_SIZE,"ERROR%s %s\n",errText,userMsg);
     if ( flushStdout ){
          fflush(stdout);
     }
     fputs(buff,stderr);
     fflush(stderr); */
     perror("Error!: ");
     printf("\n");
     vfprintf(stderr,fmt,argList);
     fflush(stderr);
}

void errMsg(const char *fmt, ... ){
     va_list args;
     int tmp_err = errno;
     va_start(args,fmt);
     outErrMsg(true,tmp_err,true,fmt,args);
     va_end(args);
     errno = tmp_err;
}

void errExit(const char *fmt, ... ){
     va_list args;
     va_start(args,fmt);
     outErrMsg(true,errno,true,fmt,args);
     va_end(args);
     terminate(true);
}
void err_exit(const char *fmt, ... ){
     va_list args;
     va_start(args,fmt);
     outErrMsg(true,errno,false,fmt,args);
     va_end(args);
     terminate(false);
}

void errExitEN(int errnum,const char *fmt, ... ){
     va_list args;
     va_start(args,fmt);
     outErrMsg(true,errnum,true,fmt,args);
     va_end(args);
     terminate(true);
}

void fatal(const char *fmt, ... ){
     va_list args;
     va_start(args,fmt);
     outErrMsg(false,0,true,fmt,args);
     va_end(args);
     terminate(true);
}

void usageError(const char *fmt, ... ){
     va_list args;
     fflush(stdout);
     va_start(args,fmt);
     fprintf(stderr,"Usage:");
     vfprintf(stderr,fmt,args);
     va_end(args);
     fflush(stderr);
}

void cmdLineError(const char *fmt, ... ){
     va_list args;
     fflush(stdout);
     va_start(args,fmt);
     fprintf(stderr,"Command Line Usage:");
     vfprintf(stderr,fmt,args);
     va_end(args);
     fflush(stderr);
}
#undef exit
