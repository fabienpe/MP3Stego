/* $Header: /MP3Stego Encoder/l3psy.c 3     15/08/98 10:40 Fapp2 $ */
#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "error.h"
#include "layer3.h"
#include "l3psy.h"

#include "fft.h"
#include "tables.h"

#define maximum(x,y) ( (x>y) ? x : y )
#define minimum(x,y) ( (x<y) ? x : y )

/* Static data for the Layer III psychoacoustic filter */

    static double ratio[2][21];
    static double ratio_s[2][12][3];
/* The static variables "r", "phi_sav", "new", "old" and "oldest" have    */
/* to be remembered for the unpredictability measure.  For "r" and        */
/* "phi_sav", the first index from the left is the channel select and     */
/* the second index is the "age" of the data.                             */
    static float   window_s[BLKSIZE_s] ;
    static int     new = 0, old = 1, oldest = 0;
    static int     flush, sync_flush, syncsize;
    static double 	cw[HBLKSIZE], eb[CBANDS];
    static double 	ctb[CBANDS];
    static double	SNR_l[CBANDS], SNR_s[CBANDS_s];
    static double	minval[CBANDS],qthr_l[CBANDS],norm_l[CBANDS];
    static double	qthr_s[CBANDS_s],norm_s[CBANDS_s];
    static double	nb_1[2][CBANDS], nb_2[2][CBANDS];
    static double	s3_l[CBANDS][CBANDS]; /* s3_s[CBANDS_s][CBANDS_s]; */

/* Scale Factor Bands */
    static int	cbw_l[SBMAX_l],bu_l[SBMAX_l],bo_l[SBMAX_l] ;
    static int	cbw_s[SBMAX_s],bu_s[SBMAX_s],bo_s[SBMAX_s] ;
    static double	w1_l[SBMAX_l], w2_l[SBMAX_l];
    static double	w1_s[SBMAX_s], w2_s[SBMAX_s];
    static double	en[SBMAX_l],   thm[SBMAX_l] ;
    static int	blocktype_old[2] ;
    static int	partition_l[HBLKSIZE],partition_s[HBLKSIZE_s];
    static float   *absthr;


/* The following static variables are constants.                           */
     static float   crit_band[27] = {0,  100,  200, 300, 400, 510, 630,  770,
                                     920, 1080, 1270,1480,1720,2000,2320, 2700,
                                     3150, 3700, 4400,5300,6400,7700,9500,12000,
                                     15500,25000,30000};

    static float	energy_s[3][256];
    static float   phi_s[3][256] ; /* 256 samples not 129 */

    static	int     numlines[CBANDS];
    static int     partition[HBLKSIZE];
    static float   cbval[CBANDS];
    static float   rnorm[CBANDS];
    static float   window[BLKSIZE];
    static double  tmn[CBANDS];
    static float   s[CBANDS][CBANDS];
    static float   lthr[2][HBLKSIZE];
    static float   r[2][2][HBLKSIZE];
    static float   phi_sav[2][2][HBLKSIZE];
   
    static float   nb[CBANDS]; 
    static float   cb[CBANDS];
    static float   ecb[CBANDS];
    static float   wsamp_r[BLKSIZE];
    static float   wsamp_i[BLKSIZE];
    static float   phi[BLKSIZE];
    static float   energy[BLKSIZE];
    static float   fthr[HBLKSIZE];

void L3para_read();
void L3_psycho_initialise()
{
    unsigned int   i, j;
    float          freq_mult, bval_lo;
    double         temp1,temp2,temp3;


     switch(config.wave.samplerate)
     {
        case 32000: absthr = absthr_0; 
                    break;
        case 44100: absthr = absthr_1;
                    break;
        case 48000: absthr = absthr_2;
                    break;
        default:    ERROR("[l3_psy], invalid sampling frequency");
     }

     sync_flush = 768; 
     flush      = 576; 
     syncsize   = 1344;

/* calculate HANN window coefficients */
     for(i=0;i<BLKSIZE;i++)  window[i]  = 0.5*(1-cos(2.0*PI*(i-0.5)/BLKSIZE));
     for(i=0;i<BLKSIZE_s;i++)window_s[i]= 0.5*(1-cos(2.0*PI*(i-0.5)/BLKSIZE_s));

/* reset states used in unpredictability measure */
     for(i=0;i<HBLKSIZE;i++)
     {
        r[0][0][i]=r[1][0][i]=r[0][1][i]=r[1][1][i]=0;
        phi_sav[0][0][i]=phi_sav[1][0][i]=0;
        phi_sav[0][1][i]=phi_sav[1][1][i]=0;
        lthr[0][i] = 60802371420160.0;
        lthr[1][i] = 60802371420160.0;
     }

/*****************************************************************************
 * Initialization: Compute the following constants for use later             *
 *    partition[HBLKSIZE] = the partition number associated with each        *
 *                          frequency line                                   *
 *    cbval[CBANDS]       = the center (average) bark value of each          *
 *                          partition                                        *
 *    numlines[CBANDS]    = the number of frequency lines in each partition  *
 *    tmn[CBANDS]         = tone masking noise                               *
 *****************************************************************************/
/* compute fft frequency multiplicand */
     freq_mult = config.wave.samplerate/BLKSIZE;
/* calculate fft frequency, then bval of each line (use fthr[] as tmp storage)*/
     for(i=0;i<HBLKSIZE;i++){
        temp1 = i*freq_mult;
        j = 1;
        while(temp1>crit_band[j])j++;
        fthr[i]=j-1+(temp1-crit_band[j-1])/(crit_band[j]-crit_band[j-1]);
     }
     partition[0] = 0;
/* temp2 is the counter of the number of frequency lines in each partition */
     temp2 = 1;
     cbval[0]=fthr[0];
     bval_lo=fthr[0];
     for(i=1;i<HBLKSIZE;i++){
        if((fthr[i]-bval_lo)>0.33){
           partition[i]=partition[i-1]+1;
           cbval[partition[i-1]] = cbval[partition[i-1]]/temp2;
           cbval[partition[i]] = fthr[i];
           bval_lo = fthr[i];
           numlines[partition[i-1]] = temp2;
           temp2 = 1;
        }
        else {
           partition[i]=partition[i-1];
           cbval[partition[i]] += fthr[i];
           temp2++;
        }
     }
     numlines[partition[i-1]] = temp2;
     cbval[partition[i-1]] = cbval[partition[i-1]]/temp2;
 
/************************************************************************
 * Now compute the spreading function, s[j][i], the value of the spread-*
 * ing function, centered at band j, for band i, store for later use    *
 ************************************************************************/
     for(j=0;j<CBANDS;j++){
        for(i=0;i<CBANDS;i++){
           temp1 = (cbval[i] - cbval[j])*1.05;
           if(temp1>=0.5 && temp1<=2.5){
              temp2 = temp1 - 0.5;
              temp2 = 8.0 * (temp2*temp2 - 2.0 * temp2);
           }
           else temp2 = 0;
           temp1 += 0.474;
           temp3 = 15.811389+7.5*temp1-17.5*sqrt((double) (1.0+temp1*temp1));
           if(temp3 <= -100) s[i][j] = 0;
           else {
              temp3 = (temp2 + temp3)*LN_TO_LOG10;
              s[i][j] = exp(temp3);
           }
        }
     }

  /* Calculate Tone Masking Noise values */
     for(j=0;j<CBANDS;j++){
        temp1 = 15.5 + cbval[j];
        tmn[j] = (temp1>24.5) ? temp1 : 24.5;
  /* Calculate normalization factors for the net spreading functions */
        rnorm[j] = 0;
        for(i=0;i<CBANDS;i++){
           rnorm[j] += s[j][i];
        }
     }
     L3para_read();
}


void L3_psycho_analize(int      channel,
                       short   *buffer, 
                       short    savebuf[1344], 
                       float    snr32[32],
                       double   ratio_d[21], 
                       double   ratio_ds[12][3],
		       double  *pe, 
                       gr_info *cod_info )
{
    int blocktype;
    unsigned int   b, i, j, k;
    double         r_prime, phi_prime; /* not float */
    double         temp1,temp2,temp3;

    double   thr[CBANDS];
    int      sb,sblock;

    for ( j = 0; j < 21; j++ ) ratio_d[j] = ratio[channel][j];
	for ( j = 0; j < 12; j++ )
	    for ( i = 0; i < 3; i++ )
		ratio_ds[j][i] = ratio_s[channel][j][i];
	
    if ( channel == 0 )
        if ( new == 0 )
	{
	    new = 1;
	    old = 0;
	    oldest = 1;
	}
	else
	{
	    new = 0;
	    old = 1;
	    oldest = 0;
	}


/**********************************************************************
*  Delay signal by sync_flush=768 samples                             *
**********************************************************************/
	
	for ( j = 0; j < sync_flush; j++ ) /* for long window samples */
	    savebuf[j] = savebuf[j+flush];
	
	for ( j = sync_flush; j < syncsize; j++ )
	    savebuf[j] = *buffer++;
	
	for ( j = 0; j < BLKSIZE; j++ )
	{ /**window data with HANN window**/
	    wsamp_r[j] = window[j] * savebuf[j];  
	    wsamp_i[j] = 0.0;
	}


/**********************************************************************
*    compute unpredicatability of first six spectral lines            * 
**********************************************************************/

	fft( wsamp_r, wsamp_i, energy, phi, 1024 );		/**long FFT**/
	for ( j = 0; j < 6; j++ )
	{	 /* calculate unpredictability measure cw */
	    r_prime   = 2.0 * r[channel][old][j] - r[channel][oldest][j];
	    phi_prime = 2.0 * phi_sav[channel][old][j]-phi_sav[channel][oldest][j];
	    r[channel][new][j] = sqrt((double) energy[j]);
	    phi_sav[channel][new][j] = phi[j];
	    temp1 = r[channel][new][j] * cos((double) phi[j]) - r_prime * cos(phi_prime);
	    temp2 = r[channel][new][j] * sin((double) phi[j]) - r_prime * sin(phi_prime);
	    temp3 = r[channel][new][j] + fabs(r_prime);
	    
	    if ( temp3 != 0.0 ) cw[j] = sqrt( temp1*temp1+temp2*temp2 ) / temp3;
	    else                cw[j] = 0;
	}


/**********************************************************************
*     compute unpredicatibility of next 200 spectral lines            *
**********************************************************************/ 
	for(sblock=0;sblock<3;sblock++)
	{ /**window data with HANN window**/
	    for ( j = 0, k = 128 * (2 + sblock); j < 256; j++, k++ )
	    {
		wsamp_r[j] = window_s[j] * savebuf[k]; 
		wsamp_i[j] = 0.0;
	    }							/* short FFT*/
	    
	    fft( wsamp_r, wsamp_i, &energy_s[sblock][0], &phi_s[sblock][0], 256 );
        }
 
        sblock = 1;

	for ( j = 6; j < 206; j += 4 )
	{/* calculate unpredictability measure cw */
	    double r2, phi2, temp1, temp2, temp3;
	    
	    k = (j+2) / 4; 
	    r_prime   = 2.0 * sqrt((double) energy_s[0][k])
		            - sqrt((double) energy_s[2][k]);
	    phi_prime = 2.0 * phi_s[0][k] - phi_s[2][k];
	    r2        = sqrt((double) energy_s[1][k]);
	    phi2      = phi_s[1][k];
	    temp1     = r2 * cos( phi2 ) - r_prime * cos( phi_prime );
	    temp2     = r2 * sin( phi2 ) - r_prime * sin( phi_prime );
	    temp3     = r2 + fabs( r_prime );
	    if ( temp3 != 0.0 )
		cw[j] = sqrt( temp1 * temp1 + temp2 * temp2 ) / temp3;
	    else
		cw[j] = 0.0;
	    cw[j+1] = cw[j+2] = cw[j+3] = cw[j];
	}


/**********************************************************************
*    Set unpredicatiblility of remaining spectral lines to 0.4        *
**********************************************************************/
	for ( j = 206; j < HBLKSIZE; j++ ) cw[j] = 0.4;
	


/**********************************************************************
*    Calculate the energy and the unpredictability in the threshold   *
*    calculation partitions                                           *
**********************************************************************/

	for ( b = 0; b < CBANDS; b++ )
	{
	    eb[b] = 0.0;
	    cb[b] = 0.0;
	}
	for ( j = 0; j < HBLKSIZE; j++ )
	{
	    int tp = partition_l[j];
	    if ( tp >= 0 )
	    {
		eb[tp] += energy[j];
		cb[tp] += cw[j] * energy[j];
	    }
	}


/**********************************************************************
*      convolve the partitioned energy and unpredictability           *
*      with the spreading function, s3_l[b][k]                        *
******************************************************************** */
	
	for ( b = 0; b < CBANDS; b++ )
	{
	    ecb[b] = 0.0;
	    ctb[b] = 0.0;
	}
	for ( b = 0;b < CBANDS; b++ )
	{
	    for ( k = 0; k < CBANDS; k++ )
	    {
		ecb[b] += s3_l[b][k] * eb[k];	/* sprdngf for Layer III */
		ctb[b] += s3_l[b][k] * cb[k];
	    }
	}

	/* calculate the tonality of each threshold calculation partition */
	/* calculate the SNR in each threshhold calculation partition */

	for ( b = 0; b < CBANDS; b++ )
	{
	    double cbb,tbb;
	    if (ecb[b] != 0.0 )
            {
		cbb = ctb[b]/ecb[b];
                if (cbb <0.01) cbb = 0.01;
		cbb = log( cbb);
            }
	    else cbb = 0.0 ;

	    tbb = -0.299 - 0.43*cbb;  /* conv1=-0.299, conv2=-0.43 */
	    tbb = minimum( 1.0, maximum( 0.0, tbb) ) ;  /* 0<tbb<1 */
	    SNR_l[b] = maximum( minval[b], 29.0*tbb+6.0*(1.0-tbb) );
	}	/* TMN=29.0,NMT=6.0 for all calculation partitions */
	
	for ( b = 0; b < CBANDS; b++ ) /* calculate the threshold for each partition */
	    nb[b] = ecb[b] * norm_l[b] * exp( -SNR_l[b] * LN_TO_LOG10 );

	for ( b = 0; b < CBANDS; b++ )
	{ /* pre-echo control */
	    double temp_1; /* BUG of IS */
	    temp_1 = minimum( nb[b], minimum(2.0*nb_1[channel][b],16.0*nb_2[channel][b]) );
	    thr[b] = maximum( qthr_l[b], temp_1 );
	    nb_2[channel][b] = nb_1[channel][b];
	    nb_1[channel][b] = nb[b];
	}

	*pe = 0.0;		/*  calculate perceptual entropy */
	for ( b = 0; b < CBANDS; b++ )
	{
	    double tp ;
	    tp = minimum( 0.0, log((thr[b]+1.0) / (eb[b]+1.0) ) ) ; /*not log*/
	    *pe -= numlines[b] * tp ;
	}
	
#define switch_pe  1800
        blocktype = NORM_TYPE;
	
	if ( *pe < switch_pe )
	{				/* no attack : use long blocks */
	    switch( blocktype_old[channel] ) 
	    {
	      case NORM_TYPE:
	      case STOP_TYPE:
		blocktype = NORM_TYPE;
		break;
    
	      case SHORT_TYPE:
		blocktype = STOP_TYPE;
		break;
    
	      case START_TYPE:
		fprintf( stderr, "Error in block selecting\n" );
		abort();
		break; /* problem */
	    }

	    /* threshold calculation (part 2) */
	    for ( sb = 0; sb < SBMAX_l; sb++ )
	    {
		en[sb] = w1_l[sb] * eb[bu_l[sb]] + w2_l[sb] * eb[bo_l[sb]];
		thm[sb] = w1_l[sb] *thr[bu_l[sb]] + w2_l[sb] * thr[bo_l[sb]];
		for ( b = bu_l[sb]+1; b < bo_l[sb]; b++ )
		{
		    en[sb]  += eb[b];
		    thm[sb] += thr[b];
		}
		if ( en[sb] != 0.0 )
		    ratio[channel][sb] = thm[sb]/en[sb];
		else
		    ratio[channel][sb] = 0.0;
	    }
	}
	else 
	{
	    /* attack : use short blocks */
	    blocktype = SHORT_TYPE;
	    
	    if(blocktype_old[channel]==NORM_TYPE) blocktype_old[channel] = START_TYPE;
	    if(blocktype_old[channel]==STOP_TYPE) blocktype_old[channel] = SHORT_TYPE ;
	    
/* threshold calculation for short blocks */
	    for ( sblock = 0; sblock < 3; sblock++ )
	    {
		for ( b = 0; b < CBANDS_s; b++ )
		{
		    eb[b] = 0.0;
		    ecb[b] = 0.0;
		}
		for ( j = 0; j < HBLKSIZE_s; j++ ) eb[partition_s[j]] += energy_s[sblock][j];
		for ( b = 0; b < CBANDS_s; b++ )
                    for ( k = 0; k < CBANDS_s; k++ )
			ecb[b] += s3_l[b][k] * eb[k];
		for ( b = 0; b < CBANDS_s; b++ )
		{
		    nb[b]  = ecb[b] * norm_l[b] * exp((double)SNR_s[b]*LN_TO_LOG10);
		    thr[b] = maximum(qthr_s[b],nb[b]);
		}
		for ( sb = 0; sb < SBMAX_s; sb++ )
		{
		    en[sb]  = w1_s[sb] * eb[bu_s[sb]] + w2_s[sb] * eb[bo_s[sb]];
		    thm[sb] = w1_s[sb] *thr[bu_s[sb]] + w2_s[sb] * thr[bo_s[sb]];
		    for ( b = bu_s[sb]+1; b < bo_s[sb]; b++ )
		    {
			en[sb]  += eb[b];
			thm[sb] += thr[b];
		    }
		    if(en[sb]!=0.0) ratio_s[channel][sb][sblock] = thm[sb]/en[sb];
		    else            ratio_s[channel][sb][sblock] = 0.0;
		}
	    }
	} 
	
	cod_info->block_type = blocktype_old[channel];
	blocktype_old[channel] = blocktype;

	if ( cod_info->block_type == NORM_TYPE )
	    cod_info->window_switching_flag = 0;
	else
	    cod_info->window_switching_flag = 1;
	cod_info->mixed_block_flag = 0;

}









#include "psy_data.h"
void L3para_read()
{
int curr_line = 0;
char *temp;
   double freq_tp;
   static double bval_l[CBANDS], bval_s[CBANDS];
   int    cbmax=0, cbmax_tp;
   static double s3_s[CBANDS][CBANDS];

   char tp[256];
   int  sbmax ;
   int  i,j,k,k2,loop, part_max ;

/* Read long block data */
      for(loop=0;loop<6;loop++)
      {
temp = psy_data[curr_line++];
	sscanf(temp,"freq = %lf partition = %d\n",&freq_tp,&cbmax_tp);
	cbmax_tp++;

	if (config.wave.samplerate == freq_tp)
	  {
	     cbmax = cbmax_tp;
	     for(i=0,k2=0;i<cbmax_tp;i++)
	       {
temp = psy_data[curr_line++];
		sscanf(temp,
		  "No=%d #lines=%d minval=%lf qthr=%lf norm=%lf bval=%lf\n",
		  &j,&numlines[i],&minval[i],&qthr_l[i],&norm_l[i],&bval_l[i]);
	        if (j!=i)
	         { printf("please check \"psy_data\"");
		   exit(-1);
	         }
		for(k=0;k<numlines[i];k++)
		  partition_l[k2++] = i ;
		}
	   }
	   else
	   {
	     for(j=0;j<cbmax_tp;j++)
	       {
char *temp;
temp = psy_data[curr_line++];
	        sscanf(temp,"No=%d %s\n",&i,tp);
	        if (j!=i)
	         { printf("please check \"psy_data.\"\n");
		   exit(-1);
	         }
	       }
	   }
       }

/************************************************************************
 * Now compute the spreading function, s[j][i], the value of the spread-*
 * ing function, centered at band j, for band i, store for later use    *
 ************************************************************************/
	  part_max = cbmax ;
          for(i=0;i<part_max;i++)
	  {
              double tempx,x,tempy,temp;
              for(j=0;j<part_max;j++)
              {
                  tempx = (bval_l[i] - bval_l[j])*1.05;

                  if (j>=i) tempx = (bval_l[i] - bval_l[j])*3.0;
                  else      tempx = (bval_l[i] - bval_l[j])*1.5;

                  if(tempx>=0.5 && tempx<=2.5)
	          {
                      temp = tempx - 0.5;
                      x = 8.0 * (temp*temp - 2.0 * temp);
                  } 
                  else x = 0.0;
                  tempx += 0.474;
                  tempy = 15.811389 + 7.5*tempx - 17.5*sqrt(1.0+tempx*tempx);
                  if (tempy <= -60.0) s3_l[i][j] = 0.0;
                  else                s3_l[i][j] = exp( (x + tempy)*LN_TO_LOG10 );
              }
          }


/* Read short block data */

      for(loop=0;loop<6;loop++)
      {
temp = psy_data[curr_line++];
	sscanf(temp,"freq = %lf partition = %d\n",&freq_tp,&cbmax_tp);
	cbmax_tp++;

	if (config.wave.samplerate==freq_tp)
	  {
	     cbmax = cbmax_tp;
	     for(i=0,k2=0;i<cbmax_tp;i++)
	       {
temp = psy_data[curr_line++];
		sscanf(temp,
		  "No=%d #lines=%d qthr=%lf norm=%lf SNR=%lf bval=%lf\n",
		   &j,&numlines[i],&qthr_s[i],&norm_s[i],&SNR_s[i],&bval_s[i]);
	        if (j!=i)
	         { printf("please check \"psy_data\"");
		   exit(-1);
	         }
		for(k=0;k<numlines[i];k++)
		  partition_s[k2++] = i ;
		}
	   }
	   else
	   {
	     for(j=0;j<cbmax_tp;j++)
	       {
temp = psy_data[curr_line++];
	        sscanf(temp,"No=%d %s\n",&i,tp);
	        if (j!=i)
	         { printf("please check \"psy_data.\"\n");
		   exit(-1);
	         }
	       }
	   }
       }

/************************************************************************
 * Now compute the spreading function, s[j][i], the value of the spread-*
 * ing function, centered at band j, for band i, store for later use    *
 ************************************************************************/
	  part_max = cbmax ;
          for(i=0;i<part_max;i++)
	  {
	  double tempx,x,tempy,temp;
            for(j=0;j<part_max;j++)
	    {
             tempx = (bval_s[i] - bval_s[j])*1.05;
             if (j>=i) tempx = (bval_s[i] - bval_s[j])*3.0;
               else    tempx = (bval_s[i] - bval_s[j])*1.5;
             if(tempx>=0.5 && tempx<=2.5)
	     {
               temp = tempx - 0.5;
               x = 8.0 * (temp*temp - 2.0 * temp);
             }
             else x = 0.0;
             tempx += 0.474;
             tempy = 15.811389 + 7.5*tempx - 17.5*sqrt(1.0+tempx*tempx);
             if (tempy <= -60.0) s3_s[i][j] = 0.0;
             else                s3_s[i][j] = exp( (x + tempy)*LN_TO_LOG10 );
            }
          }
/* Read long block data for converting threshold calculation 
   partitions to scale factor bands */

      for(loop=0;loop<6;loop++)
      {
temp = psy_data[curr_line++];
	sscanf(temp,"freq=%lf sb=%d\n",&freq_tp,&sbmax);
	sbmax++;

	if (config.wave.samplerate== freq_tp)
	  {
	     for(i=0;i<sbmax;i++)
	      {
temp = psy_data[curr_line++];
		sscanf(temp,
		  "sb=%d cbw=%d bu=%d bo=%d w1=%lf w2=%lf\n",
		  &j,&cbw_l[i],&bu_l[i],&bo_l[i],&w1_l[i],&w2_l[i]);
	        if (j!=i)
	         { printf("30:please check \"psy_data\"\n");
		   exit(-1);
	         }
	        if (i!=0)
		 if ( (bo_l[i] != (bu_l[i]+cbw_l[i])) ||
				 (fabs(1.0-w1_l[i]-w2_l[i-1]) > 0.01 ) )
	         { printf("31:please check \"psy_data.\"\n");
		   exit(-1);
	         }
	      }
	   }
	   else
	   {
	     for(j=0;j<sbmax;j++)
	       {
temp = psy_data[curr_line++];
	        sscanf(temp,"sb=%d %s\n",&i,tp);
	        if (j!=i)
	         { printf("please check \"psy_data.\"\n");
		   exit(-1);
	         }
	       }
	   }
       }

/* Read short block data for converting threshold calculation 
   partitions to scale factor bands */

      for(loop=0;loop<6;loop++)
      {
temp = psy_data[curr_line++];
	sscanf(temp,"freq=%lf sb=%d\n",&freq_tp,&sbmax);
	sbmax++;

	if (config.wave.samplerate == freq_tp)
	  {
	     for(i=0;i<sbmax;i++)
	      {
temp = psy_data[curr_line++];
		sscanf(temp,
		  "sb=%d cbw=%d bu=%d bo=%d w1=%lf w2=%lf\n",
		  &j,&cbw_s[i],&bu_s[i],&bo_s[i],&w1_s[i],&w2_s[i]);
	        if (j!=i)
	         { printf("30:please check \"psy_data\"\n");
		   exit(-1);
	         }
	        if (i!=0)
		 if ( (bo_s[i] != (bu_s[i]+cbw_s[i])) ||
				 (fabs(1.0-w1_s[i]-w2_s[i-1]) > 0.01 ) )
	         { printf("31:please check \"psy_data.\"\n");
		   exit(-1);
	         }
	      }
	   }
	   else
	   {
	     for(j=0;j<sbmax;j++)
	       {
temp = psy_data[curr_line++];
	        sscanf(temp,"sb=%d %s\n",&i,tp);
	        if (j!=i)
	         { printf("please check \"psy_data.\"\n");
		   exit(-1);
	         }
	       }
	   }
       }

}

