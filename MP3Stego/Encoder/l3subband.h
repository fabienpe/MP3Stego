#ifndef L3SUBBAND_H
#define L3SUBBAND_H

#include "types.h"

void L3_subband_initialise();
void L3_window_subband(short **buffer, double z[HAN_SIZE], int k);
void L3_filter_subband(double z[HAN_SIZE], double s[SBLIMIT]);

#endif
