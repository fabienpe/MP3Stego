#ifndef L3PSY_DOT_H_
#define L3PSY_DOT_H_

#include "types.h"


#define HBLKSIZE    513
#define CBANDS       63 
#define CBANDS_s     42
#define BLKSIZE_s   256
#define HBLKSIZE_s  129
#define TCBMAX_l     63
#define TCBMAX_s     42
#define SBMAX_l      21
#define SBMAX_s      12

typedef int        ICB[CBANDS];
typedef int        IHBLK[HBLKSIZE];
typedef float      F32[32];
typedef float      F2_32[2][32];
typedef float      FCB[CBANDS];
typedef float      FCBCB[CBANDS][CBANDS];
typedef float      FBLK[BLKSIZE];
typedef float      FHBLK[HBLKSIZE];
typedef float      F2HBLK[2][HBLKSIZE];
typedef float      F22HBLK[2][2][HBLKSIZE];
typedef double     DCB[CBANDS];

/* #define switch_pe        1800 */
#define NORM_TYPE       0
#define START_TYPE      1
#define SHORT_TYPE      2
#define STOP_TYPE       3

/* l3psy.c */
#include "layer3.h"

void L3_psycho_initialise();
void L3_psycho_analize(int      channel,
                       short   *buffer, 
                       short    savebuf[1344],
                       float    snr32[32],
                       double   ratio_d[21],
                       double   ratio_ds[12][3],
                       double  *pe, 
                       gr_info *cod_info);
#endif
