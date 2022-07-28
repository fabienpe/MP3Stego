
#include "error.h"

#include <stdlib.h>

//#define ERROR(X) {fprintf(stderr,"[ERROR] %s\n",X);exit(-1);}

#ifdef WIN32
#ifndef __STDC__
#define __STDC__
#endif
#endif 

#ifdef __STDC__
void ERROR(char *format, ...)
{
    va_list args;
    va_start(args, format);
#else /*__STDC__*/
void ERROR(va_alist) va_dcl
{
    va_list args;
    char* format;

    va_start(args);
    format = va_arg(args, char*);
#endif /*__STDC__*/

    fprintf(stderr, "[ERROR]");
    (void) vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
    exit(-1);
}

#ifdef __STDC__
void MESSAGE(char* format, ...)
{
    va_list args;
    va_start(args, format);
#else /*__STDC__*/
void MESSAGE(va_alist)
    va_dcl
{
    va_list args;
    char* format;
    va_start(args);
    format = va_arg(args, char*);
#endif /*__STDC__*/

#ifdef VERBOSE
    (void) vfprintf(stderr, format, args);
    fputc('\n', stderr);
#endif
    va_end(args);
}

