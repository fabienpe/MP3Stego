#ifndef WAVE_H
#define WAVE_H

#include <stdio.h>
#include "types.h"

bool  wave_open();
int   wave_get(short buffer[2][1152]);
void  wave_close();

#endif
