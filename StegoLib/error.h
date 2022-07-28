#ifndef _OUR_ERROR_H
#define _OUR_ERROR_H

#include <stdio.h>
#include <stdarg.h>

//#define ERROR(X) {fprintf(stderr,"[ERROR] %s\n",X);exit(-1);}
#ifdef __STDC__
void ERROR(char *format, ...);
#else /*__STDC__*/
void ERROR(va_alist);
#endif /*__STDC__*/

#endif
