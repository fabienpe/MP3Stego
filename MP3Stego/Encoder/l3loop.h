#ifndef L3LOOP_H
#define L3LOOP_H

#include "types.h"
#include "layer3.h"

#define e              2.71828182845
#define CBLIMIT       21
#define SFB_LMAX 22
#define SFB_SMAX 13


void L3_loop_initialise();

void L3_iteration_loop(double          pe[][2], 
                       double          mdct_freq_org[2][2][576], 
                       L3_psy_ratio_t *ratio,
       		       L3_side_info_t *side_info, 
                       int             l3_enc[2][2][576],
		       int             mean_bits, 
		       L3_scalefac_t  *scalefacitor );

#endif

