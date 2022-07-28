#ifndef _OUR_ERROR_H
#define _OUR_ERROR_H

#include <stdio.h>

#define ERROR(X) {fprintf(stderr,"[ERROR] %s\n",X);exit(-1);}


#endif
