#pragma once
#ifndef ERR_HDR
#define ERR_HDR
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>
//#include "error_functions.h"
#define MAX(x,y) ( ( ( x ) > ( y ) ) ? ( x ) : ( y ) )
#define MIN(x,y) ( ( ( x ) < ( y ) ) ? ( x ) : ( y ) )


/* Diagonses error from system calls and library functions */
void errExit(const char *fmt,...);
void err_Exit(const char *fmt,...);
void errExitEN(int errnum,const char *fmt,...);
void errMsg(const char *fmt,...);

/* Diagnoses other types of errors */
void fatal(const char *fmt,...);
void usageErr(const char *fmt,...); // Checks errors in usage of command line arguments
void cmdLineErr(const char *fmt,...); // Checks errors in the given command line arguments for the program

#endif
