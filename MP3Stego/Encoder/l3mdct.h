#ifndef L3_MDCT_H
#define L3_MDCT_H

#include "types.h"
#include "layer3.h"

void L3_mdct_initialise();
void L3_mdct_sub(double sb_sample[2][3][18][SBLIMIT], 
                 double (*mdct_freq)[2][576], 
                 L3_side_info_t *side_info);

#endif
