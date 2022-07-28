/* $Header: /MP3Stego Encoder/l3subband.c 2     15/08/98 10:40 Fapp2 $ */
#include <math.h>

#include "types.h"
#include "tables.h" 
#include "l3subband.h"


static off[2]    = {0,0};
static double x[2][HAN_SIZE];
static double filter[SBLIMIT][64];

void L3_subband_initialise()
{
    int i,j,k;

    for(i=0;i<2;i++) 
        for(j=0;j<HAN_SIZE;j++) x[i][j] = 0;
        
/* create_ana_filter */
/************************************************************************
* PURPOSE:  Calculates the analysis filter bank coefficients
* SEMANTICS:
* Calculates the analysis filterbank coefficients and rounds to the
* 9th decimal place accuracy of the filterbank tables in the ISO
* document.  The coefficients are stored in #filter#
************************************************************************/
   for (i=0; i<32; i++)
      for (k=0; k<64; k++) 
      {
          if ((filter[i][k] = 1e9*cos((double)((2*i+1)*(16-k)*PI64))) >= 0)
               modf(filter[i][k]+0.5, &filter[i][k]);
          else modf(filter[i][k]-0.5, &filter[i][k]);
          filter[i][k] *= 1e-9;
      } 
}


void L3_window_subband(short **buffer, double z[HAN_SIZE], int k)
/************************************************************************
* PURPOSE:  Overlapping window on PCM samples
* SEMANTICS:
* 32 16-bit pcm samples are scaled to fractional 2's complement and
* concatenated to the end of the window buffer #x#. The updated window
* buffer #x# is then windowed by the analysis window #enwindow# to produce the
* windowed sample #z#
************************************************************************/
{
    int i;

    /* replace 32 oldest samples with 32 new samples */
    for (i=0;i<32;i++) x[k][31-i+off[k]] = (double)*(*buffer)++/SCALE;

    /* shift samples into proper window positions */
    for (i=0;i<HAN_SIZE;i++) z[i] = x[k][(i+off[k])&(HAN_SIZE-1)] * enwindow[i];

    off[k] += 480;              /* offset is modulo (HAN_SIZE)*/
    off[k] &= HAN_SIZE-1;
}
 

void L3_filter_subband(double z[HAN_SIZE], double s[SBLIMIT])
 /************************************************************************
* PURPOSE:  Calculates the analysis filter bank coefficients
* SEMANTICS:
*      The windowed samples #z# is filtered by the digital filter matrix #filter#
* to produce the subband samples #s#. This done by first selectively
* picking out values from the windowed samples, and then multiplying
* them by the filter matrix, producing 32 subband samples.
*
************************************************************************/
{
   double y[64];
   register int i,j;

   for (i=0;i<64;i++) 
       for (j=0, y[i] = 0;j<8;j++) y[i] += z[i+(j<<6)];

   for (i=0;i<SBLIMIT;i++)
       for (j=0, s[i]= 0;j<64;j++) s[i] += filter[i][j] * y[j];
}

