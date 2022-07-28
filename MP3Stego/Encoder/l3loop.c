/****************************************************************/
/* Optimized by 8hz, May 1998                                   */
/* Added precalc tables and rewritten some heavy calculations   */
/****************************************************************/

/* MP3STEGO                                                     */
/* Some lines added or modified for the information hidding     */
/*                                                              */
/* Fabien A.P. Petitcolas, August 1998                          */
/*                                                              */
/* $Header: /MP3Stego/MP3Stego Encoder/l3loop.c 7     19/03/02 10:56 Fabienpe $                                                   */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../stegolib/stego.h"

#include "types.h"
#include "error.h"
#include "tables.h"
#include "layer3.h"
#include "l3loop.h"
#include "huffman.h"
#include "l3bitstream.h"
#include "reservoir.h"
#include "assert.h"

int bin_search_StepSize(int desired_rate, double start, int ix[576],
           double xrs[576], gr_info * cod_info);

int count_bits();

float worst_xfsf_to_xmin_ratio();

#define PRECALC_SIZE 1024 /* WAS 256 !!! */
static double pow43[PRECALC_SIZE];
#include "sqrttab.h"

#ifndef HAVE_NINT
int nint(double in)
{
    if (in<0) return (int)(in - 0.5);
    else     return (int)(in + 0.5);
}

#endif


/*
  The following table is used to implement the scalefactor partitioning 
  for MPEG2 as described in section 2.4.3.2 of the IS. 
  The indexing corresponds to the way the tables are presented in the IS:
  [table_number][row_in_table][column of nr_of_sfb]
*/
static unsigned nr_of_sfb_block[6][3][4] =
{
  { {6, 5, 5, 5}, {9, 9, 9, 9}, {6, 9, 9, 9} },
  { {6, 5, 7, 3}, {9, 9, 12, 6}, {6, 9, 12, 6} },
  { {11, 10, 0, 0}, {18, 18, 0, 0}, {15,18,0,0} },
  { {7, 7, 7, 0}, {12, 12, 12, 0}, {6, 15, 12, 0} },
  { {6, 6, 6, 3}, {12, 9, 9, 6}, {6, 12, 9, 6} },
  { {8, 8, 5, 0}, {15,12,9,0}, {6,18,9,0} }
};

/* Table B.6: layer3 preemphasis */
int  pretab[21] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    1, 1, 1, 1, 2, 2, 3, 3, 3, 2 };

/* This is the scfsi_band table from 2.4.2.7 of the IS */
int scfsi_band_long[5] = { 0, 6, 11, 16, 21 };

struct
{
    unsigned region0_count;
    unsigned region1_count;
} subdv_table[ 23 ] =
{
{0, 0}, /* 0 bands */
{0, 0}, /* 1 bands */
{0, 0}, /* 2 bands */
{0, 0}, /* 3 bands */
{0, 0}, /* 4 bands */
{0, 1}, /* 5 bands */
{1, 1}, /* 6 bands */
{1, 1}, /* 7 bands */
{1, 2}, /* 8 bands */
{2, 2}, /* 9 bands */
{2, 3}, /* 10 bands */
{2, 3}, /* 11 bands */
{3, 4}, /* 12 bands */
{3, 4}, /* 13 bands */
{3, 4}, /* 14 bands */
{4, 5}, /* 15 bands */
{4, 5}, /* 16 bands */
{4, 6}, /* 17 bands */
{5, 6}, /* 18 bands */
{5, 6}, /* 19 bands */
{5, 7}, /* 20 bands */
{6, 7}, /* 21 bands */
{6, 7}, /* 22 bands */
};

int *scalefac_band_long  = &sfBandIndex[3].l[0];
int *scalefac_band_short = &sfBandIndex[3].s[0];

int quantanf_init(double xr[576]);
void calc_scfsi(double  xr[576], L3_side_info_t *l3_side, L3_psy_xmin_t *l3_xmin, int ch, int gr);
int part2_length(L3_scalefac_t *scalefac, int gr, int ch, L3_side_info_t *si);
int scale_bitcount(L3_scalefac_t *scalefac, gr_info *cod_info, int gr, int ch);
void calc_noise(double xr[576], int ix[576], gr_info *cod_info, double xfsf[4][CBLIMIT]);

int bin_search_StepSize(int desired_rate, double start, int *ix, double xrs[576], gr_info * cod_info);
int count_bits(int *ix /*int[576]*/, gr_info *cod_info);
void gr_deco(gr_info *cod_info);
int count_bit(int ix[576], unsigned int start, unsigned int end, unsigned int table);
int bigv_bitcount(int ix[576], gr_info *gi);
int choose_table(int max);
int new_choose_table(int ix[576], unsigned int begin, unsigned int end);
void bigv_tab_select(int ix[576], gr_info *cod_info);
void subdivide(gr_info *cod_info);
int count1_bitcount(int ix[ 576 ], gr_info *cod_info);
void calc_runlen(int ix[576], gr_info *cod_info);

void calc_xmin(double xr[2][2][576], L3_psy_ratio_t *ratio, gr_info *cod_info, L3_psy_xmin_t *l3_xmin, int gr, int ch);
int loop_break(L3_scalefac_t *scalefac, gr_info *cod_info, int gr, int ch);
void preemphasis(double xr[576], double xfsf[4][CBLIMIT], L3_psy_xmin_t  *l3_xmin, int gr, int ch, L3_side_info_t *l3_side);
int amp_scalefac_bands(double xr[576], double xfsf[4][CBLIMIT], L3_psy_xmin_t    *l3_xmin, L3_side_info_t *l3_side, L3_scalefac_t  *scalefac, int gr, int ch, int iteration);
void quantize(double xr[576], int ix[576], gr_info *cod_info);
int ix_max(int ix[576], unsigned int begin, unsigned int end);
double xr_max(double xr[576], unsigned int begin, unsigned int end);


void L3_loop_initialise()
{
    long i;

    for(i=0;i<PRECALC_SIZE;i++)
        pow43[i] = pow((double)i, 4.0/3.0);
}


/***************************************************************************/ 
/* The code selects the best quantizerStepSize for a particular set        */
/* of scalefacs                                                            */
/***************************************************************************/ 
/* MP3STEGO-> */
static int inner_loop(double xr[2][2][576],  int l3_enc[2][2][576], 
                      int max_bits, gr_info *cod_info, int gr, int ch ,
                      int hiddenBit, int part2length)
/* ->MP3STEGO */
{
    int bits, c1bits, bvbits;
    double *xrs;  /*  double[576] *xr; */
    int     *ix;  /*  int[576]    *ix; */
	int    embedRule = 0;

    xrs = &xr[gr][ch][0];
    ix  = l3_enc[gr][ch];

    if (max_bits<0) ERROR("Ehhh !?!, negative compression !?!");
    cod_info->quantizerStepSize -= 1.0;;
    do
    {
        do
        {
            cod_info->quantizerStepSize += 1.0;
            quantize(xrs,ix,cod_info);
        }
        while(ix_max(ix,0,576)>(8191+14));               /* within table range? */

        calc_runlen(ix,cod_info);                        /* rzero,count1,big_values*/
        bits = c1bits = count1_bitcount(ix,cod_info);    /* count1_table selection*/
        subdivide(cod_info);                             /* bigvalues sfb division */
        bigv_tab_select(ix,cod_info);                    /* codebook selection*/
        bits += bvbits = bigv_bitcount(ix, cod_info);  /* bit count */

/* MP3STEGO-> */
		switch (hiddenBit)
		{
		case 2:
			embedRule = 0;
			break;
		case 0:
		case 1:
			embedRule = ((bits + part2length) % 2) != hiddenBit;
			break;
		default:
			ERROR("inner_loop: unexpected hidden bit.");
		}
/* ->MP3STEGO */
    }
/* MP3STEGO-> */
    while((bits>max_bits) | embedRule);
/* ->MP3STEGO */
    return bits;
}



/************************************************************************/
/*  Function: The outer iteration loop controls the masking conditions  */
/*  of all scalefactorbands. It computes the best scalefac and          */
/*  global gain. This module calls the inner iteration loop             */
/************************************************************************/
static int outer_loop(double xr[2][2][576],     /*  magnitudes of the spectral values */
                       int max_bits,
                       L3_psy_xmin_t *l3_xmin,  /* the allowed distortion of the scalefactor */
                       int l3_enc[2][2][576],   /* vector of quantized values ix(0..575) */
                       L3_scalefac_t *scalefac, /* scalefactors */
                       int gr, int ch,
                       L3_side_info_t *side_info,
					   int hiddenBit, int mean_bits)
{
    int status ;
    int scalesave_l[CBLIMIT], scalesave_s[CBLIMIT][3];
    int sfb, bits, huff_bits, save_preflag, save_compress;
    double xfsf[4][CBLIMIT];
    int i, over, iteration;

    double  *xrs; 
    int     *ix;  
    gr_info *cod_info = &side_info->gr[gr].ch[ch].tt;

    xrs = (double *) &(xr[gr][ch][0]); 
    ix  = (int *) &(l3_enc[gr][ch][0]);

    iteration = 0;
    do 
    {
		iteration += 1;
		cod_info->part2_length = part2_length(scalefac,gr,ch,side_info);
        huff_bits = max_bits - cod_info->part2_length;
		
		if (iteration == 1)
        {
			bin_search_StepSize(max_bits,cod_info->quantizerStepSize,
								ix,xrs,cod_info); /* speeds things up a bit */
		}
/* MP3STEGO-> */
		bits = inner_loop(xr, l3_enc, huff_bits, cod_info, gr, ch, 
            hiddenBit, cod_info->part2_length);
/* ->MP3STEGO */
		
        /* distortion calculation */
        calc_noise(&xr[gr][ch][0], &l3_enc[gr][ch][0], cod_info, xfsf);


        for (sfb = 0; sfb < CBLIMIT; sfb++) /* save scaling factors */
			scalesave_l[sfb] = scalefac->l[gr][ch][sfb];

        for (sfb = 0; sfb < SFB_SMAX; sfb++)
			for (i = 0; i < 3; i++)
				scalesave_s[sfb][i] = scalefac->s[gr][ch][sfb][i];
        
        save_preflag  = cod_info->preflag;
        save_compress = cod_info->scalefac_compress;

        preemphasis(&xr[gr][ch][0],xfsf,l3_xmin,gr,ch,side_info);

        over = amp_scalefac_bands(&xr[gr][ch][0], xfsf, l3_xmin,
                                   side_info, scalefac, gr, ch, iteration);
		

		if ((status=loop_break(scalefac,cod_info,gr,ch))==0)
		{
			/*if (fr_ps->header->version == 1)                         */
			status = scale_bitcount(scalefac, cod_info, gr, ch);
			/*else                                                       */
			/*status = scale_bitcount_lsf(scalefac, cod_info, gr, ch); */
		}

	}
	while ((status==0)&&(over>0));

    cod_info->preflag = save_preflag;
    cod_info->scalefac_compress = save_compress;

    for(sfb=0;sfb<21;sfb++) scalefac->l[gr][ch][sfb] = scalesave_l[sfb];    

    for(i=0;i<3;i++)
        for(sfb=0;sfb<12;sfb++)
            scalefac->s[gr][ch][sfb][i] = scalesave_s[sfb][i];    

    cod_info->part2_length   = part2_length(scalefac,gr,ch,side_info);
    cod_info->part2_3_length = cod_info->part2_length + bits;

/* MP3STEGO-> */
	if (hiddenBit == 0 || hiddenBit == 1)
		assert(hiddenBit == (int)(cod_info->part2_3_length % 2));
/* ->MP3SETGO */
    return cod_info->part2_3_length;
}


/************************************************************************/
/*  iteration_loop()                                                    */
/************************************************************************/
void L3_iteration_loop(double           pe[][2], 
                       double           mdct_freq_org[2][2][576],
                       L3_psy_ratio_t   *ratio,
                       L3_side_info_t   *side_info, 
                       int              l3_enc[2][2][576],
                       int              mean_bits, 
                       L3_scalefac_t    *scalefactor) 
{
    L3_psy_xmin_t l3_xmin;
    gr_info *cod_info;
    int *main_data_begin;

    int max_bits;
    int ch, gr, sfb, i;
    static int firstcall = 1;

    double xr[2][2][576];

/* MP3STEGO-> */
	int hiddenBit;
/* ->MP3STEGO */

	main_data_begin = &side_info->main_data_begin;
    side_info->resvDrain = 0;

    if (firstcall)
    {
	*main_data_begin = 0;
        firstcall=0;
    }

    scalefac_band_long  = &sfBandIndex[config.mpeg.samplerate_index + (config.mpeg.type * 3)].l[0];
    scalefac_band_short = &sfBandIndex[config.mpeg.samplerate_index + (config.mpeg.type * 3)].s[0];

    for (gr = 0; gr < config.mpeg.mode_gr; gr++)
    {
        for (ch = 0; ch < config.wave.channels; ch++)
        {
            for (i = 0; i < 576; i++) 
                xr[gr][ch][i] = mdct_freq_org[gr][ch][i];
        }
    }

    ResvFrameBegin(side_info,mean_bits,config.mpeg.bits_per_frame);

    for(gr=0;gr<config.mpeg.mode_gr;gr++)
    {
        for(ch=0;ch<config.wave.channels;ch++)
        {
            cod_info = (gr_info *) &(side_info->gr[gr].ch[ch]);
            gr_deco(cod_info);
            calc_xmin(xr, ratio, cod_info, &l3_xmin, gr, ch);
	    
            if (config.mpeg.type==TYPE_MPEG_I) calc_scfsi(xr[gr][ch],side_info,&l3_xmin,ch,gr);
	    
/* calculation of number of available bit(per granule) */
		    max_bits = ResvMaxBits(side_info,&pe[gr][ch],mean_bits);
	    
/* reset of iteration variables */
            for(sfb=0;sfb<21;sfb++) scalefactor->l[gr][ch][sfb] = 0;
            for(sfb=0;sfb<13;sfb++)
                for (i=0;i<3;i++)
                    scalefactor->s[gr][ch][sfb][i] = 0;

			for (i = 0; i < 4; i++) cod_info->slen[i] = 0;
			cod_info->sfb_partition_table = &nr_of_sfb_block[0][0][0];
				cod_info->part2_3_length    = 0;
				cod_info->big_values        = 0;
				cod_info->count1            = 0;
				cod_info->scalefac_compress = 0;
				cod_info->table_select[0]   = 0;
				cod_info->table_select[1]   = 0;
				cod_info->table_select[2]   = 0;
				cod_info->subblock_gain[0]  = 0;
				cod_info->subblock_gain[1]  = 0;
				cod_info->subblock_gain[2]  = 0;
				cod_info->region0_count     = 0;
				cod_info->region1_count     = 0;
				cod_info->part2_length      = 0;
				cod_info->preflag           = 0;
				cod_info->scalefac_scale    = 0;
				cod_info->quantizerStepSize = 0.0;
				cod_info->count1table_select= 0;
			
/* MP3STEGO-> */
			hiddenBit = StegoGetNextBit();
#if defined(_DEBUG)
			fprintf(fLog, "Hidden bit: %d\n", hiddenBit);
#endif
/* ->MP3STEGO */

            /* all spectral values zero ? */
            if (fabs(xr_max(xr[gr][ch],0,576))!=0.0)
            {
                cod_info->quantizerStepSize = (double) quantanf_init(xr[gr][ch]);
/* MP3STEGO-> */
                cod_info->part2_3_length    = outer_loop(xr, max_bits, &l3_xmin, l3_enc,
                                                         scalefactor, gr, ch,side_info,
                                                         hiddenBit, mean_bits);
/* ->MP3STEGO */
            }
		
			ResvAdjust(cod_info, side_info, mean_bits);

			cod_info->global_gain = nint(cod_info->quantizerStepSize+210.0);

            if (cod_info->global_gain>=256) ERROR("gain>256, program error.");
        } /* for ch */
    } /* for gr */

    ResvFrameEnd(side_info,mean_bits);
}



/************************************************************************/
/*  quantanf_init                                                       */
/************************************************************************/
int quantanf_init(double xr[576])
/* Function: Calculate the first quantization step quantanf.       */
{
    int i, tp = 0;
    double system_const, minlimit;
    double sfm = 0.0, sum1 = 0.0, sum2 = 0.0;
    
    system_const = 8.0;
    minlimit     = -100.0;

    for (i = 0; i < 576; i++)
        if (xr[i] != 0)
	{
            double tpd = xr[i]*xr[i];
            sum1 += log(tpd);
            sum2 += tpd;
        }

    if (sum2 != 0.0)
    {
        sfm = exp(sum1/576.0)/(sum2/576.0);
        tp  = nint(system_const*log(sfm));
	if (tp<minlimit) tp = (int)minlimit;
    }

    return(tp-70); /* SS 19-12-96. Starting value of
                      global_gain or quantizerStepSize 
                      has to be reduced for iteration_loop
                    */
}


/***************************************************************************/ 
/*        calc_scfsi                                                       */ 
/***************************************************************************/ 
/* calculation of the scalefactor select information (scfsi)        */

void calc_scfsi(double  xr[576], L3_side_info_t *l3_side,
	    L3_psy_xmin_t *l3_xmin, int ch, int gr)
{
    static int en_tot[2][2]; /* ch,gr */
    static int en[2][2][21];
    static int xm[2][2][21];
    static int xrmax[2][2];

    int en_tot_krit        = 10;
    int en_dif_krit        = 100;
    int en_scfsi_band_krit = 10;
    int xm_scfsi_band_krit = 10;

    int scfsi_band;
    unsigned scfsi_set;

    int sfb, start, end, i;
    int condition = 0;
    double temp, log2 = log(2.0);
    gr_info *cod_info = &l3_side->gr[gr].ch[ch].tt;

    xrmax[gr][ch] = xr_max(xr, 0, 576);
    scfsi_set = 0;

    /* the total energy of the granule */    
    for (temp = 0.0, i = 0; i < 576; i++)
        temp += xr[i] * xr[i];
    if (temp == 0.0)
        en_tot[gr][ch] = 0.0;
    else
        en_tot[gr][ch] = log(temp) / log2 ;

/* the energy of each scalefactor band, en */
/* the allowed distortion of each scalefactor band, xm */

    if (cod_info->window_switching_flag == 0 || cod_info->block_type!=2)
        for(sfb=0;sfb<21;sfb++)
        {
            start = scalefac_band_long[ sfb ];
            end   = scalefac_band_long[ sfb+1 ];

            for (temp = 0.0, i = start; i < end; i++)
                temp += xr[i] * xr[i];
            if (temp == 0.0)
                en[gr][ch][sfb] = 0.0;
            else
                en[gr][ch][sfb] = log(temp)/ log2;

            if (l3_xmin->l[gr][ch][sfb] == 0.0)
                xm[gr][ch][sfb] = 0.0;
            else
                xm[gr][ch][sfb] = log(l3_xmin->l[gr][ch][sfb]) / log2;
        }
    if (gr==1)
    {
        int gr2, tp;

        for(gr2=0;gr2<2;gr2++)
        {
/* The spectral values are not all zero */
            if (xrmax[ch][gr2]!=0.0)
                condition++;
/* None of the granules contains short blocks */
            if ((cod_info->window_switching_flag==0) || (cod_info->block_type!=2))
                condition++;
        }
        if (abs(en_tot[0]-en_tot[1])<en_tot_krit) condition++;
        for(tp=0,sfb=0;sfb<21;sfb++) 
            tp += abs(en[ch][0][sfb]-en[ch][1][sfb]);
        if (tp<en_dif_krit) 
            condition++;

        if (condition==6)
        {
            for(scfsi_band=0;scfsi_band<4;scfsi_band++)
            {
                int sum0 = 0, sum1 = 0;
                l3_side->scfsi[ch][scfsi_band] = 0;
                start = scfsi_band_long[scfsi_band];
                end   = scfsi_band_long[scfsi_band+1];
                for (sfb = start; sfb < end; sfb++)
                { 
                    sum0 += abs(en[ch][0][sfb] - en[ch][1][sfb]);
                    sum1 += abs(xm[ch][0][sfb] - xm[ch][1][sfb]);
                }

                if (sum0<en_scfsi_band_krit && sum1<xm_scfsi_band_krit)
		{
                    l3_side->scfsi[ch][scfsi_band] = 1;
		    scfsi_set |= (1 << scfsi_band);
		}
                else
                    l3_side->scfsi[ch][scfsi_band] = 0;
            } /* for scfsi_band */
        } /* if condition == 6 */
        else
            for(scfsi_band=0;scfsi_band<4;scfsi_band++)
                l3_side->scfsi[ch][scfsi_band] = 0;
    } /* if gr == 1 */
}




int part2_length(L3_scalefac_t *scalefac, 
                 int gr, int ch, 
                 L3_side_info_t *si)
/***************************************************************************/ 
/* calculates the number of bits needed to encode the scalefacs in the     */
/* main data block                                                         */
/***************************************************************************/ 
{
    int slen1, slen2, bits, partition;
    gr_info *gi = &si->gr[gr].ch[ch].tt;

    bits = 0;
    if (config.mpeg.type==TYPE_MPEG_I)
    {
	static int slen1_tab[16] = { 0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4 };
	static int slen2_tab[16] = { 0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3 };

	slen1 = slen1_tab[ gi->scalefac_compress ];
	slen2 = slen2_tab[ gi->scalefac_compress ];

	if ((gi->window_switching_flag == 1) && (gi->block_type == 2))
	{
	    if (gi->mixed_block_flag)
	    {
		bits += (8 * slen1) + (9 * slen1) + (18 * slen2);
	    }
	    else
	    {
		bits += (18 * slen1) + (18 * slen2);
	    }
	}
	else
	{
	    if ((gr == 0) || (si->scfsi[ch][0] == 0))
		bits += (6 * slen1);

	    if ((gr == 0) || (si->scfsi[ch][1] == 0))
		/*  bits += (6 * slen1);  This is wrong SS 19-12-96 */
		bits += (5 * slen1);

	    if ((gr == 0) || (si->scfsi[ch][2] == 0))
		/*  bits += (6 * slen1);   This is wrong SS 19-12-96 */
		bits += (5 * slen2);

	    if ((gr == 0) || (si->scfsi[ch][3] == 0))
		/* bits += (6 * slen1);   This is wrong SS 19-12-96 */
		bits += (5 * slen2);
	}
    }
    else
    {   /* MPEG 2 */
	if (!gi->sfb_partition_table) ERROR("(!gi->sfb_partition_table)");
	for (partition = 0; partition < 4; partition++)
	    bits += gi->slen[partition] * gi->sfb_partition_table[partition];
    }
    return bits;
}





int scale_bitcount(L3_scalefac_t *scalefac, 
                   gr_info *cod_info,
		   int gr, int ch)
/*************************************************************************/
/* Also calculates the number of bits necessary to code the scalefactors. */
/*************************************************************************/
{
    int i, k, sfb, max_slen1 = 0, max_slen2 = 0, /*a, b, */ ep = 2;

    static int slen1[16] = { 0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4 };
    static int slen2[16] = { 0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3 };
    static int pow2[5]   = { 1, 2, 4, 8, 16 };

    if (cod_info->window_switching_flag != 0 && cod_info->block_type == 2)
    {
        if (cod_info->mixed_block_flag == 0) 
        {
            /* a = 18; b = 18;  */
            for (i = 0; i < 3; i++)
            {
                for (sfb = 0; sfb < 6; sfb++)
                    if (scalefac->s[gr][ch][sfb][i] > max_slen1)
                        max_slen1 = scalefac->s[gr][ch][sfb][i];
                for (sfb = 6; sfb < 12; sfb++)
                    if (scalefac->s[gr][ch][sfb][i] > max_slen2)
                        max_slen2 = scalefac->s[gr][ch][sfb][i];
            }
        }
        else
        {/* mixed_block_flag = 1 */
            /* a = 17; b = 18;  */
            for (sfb = 0; sfb < 8; sfb++)
                if (scalefac->l[gr][ch][sfb] > max_slen1)
                    max_slen1 = scalefac->l[gr][ch][sfb];
            for (i = 0; i < 3; i++)
            {
                for (sfb = 3; sfb < 6; sfb++)
                    if (scalefac->s[gr][ch][sfb][i] > max_slen1)
                        max_slen1 = scalefac->s[gr][ch][sfb][i];
                for (sfb = 6; sfb < 12; sfb++)
                    if (scalefac->s[gr][ch][sfb][i] > max_slen2)
                        max_slen2 = scalefac->s[gr][ch][sfb][i];
            }
        }
    }
    else
    { /* block_type == 1,2,or 3 */
        /* a = 11; b = 10;   */
        for (sfb = 0; sfb < 11; sfb++)
            if (scalefac->l[gr][ch][sfb] > max_slen1)
                max_slen1 = scalefac->l[gr][ch][sfb];
        for (sfb = 11; sfb < 21; sfb++)
            if (scalefac->l[gr][ch][sfb] > max_slen2)
                max_slen2 = scalefac->l[gr][ch][sfb];
    }

    for (k = 0; k < 16; k++)
    {
        if ((max_slen1 < pow2[slen1[k]]) && (max_slen2 < pow2[slen2[k]]))
        { 
            ep = 0;
            break;
        } 
    }

    if (ep == 0) cod_info->scalefac_compress = k;
    return ep;
}



void calc_noise(double xr[576], 
                int ix[576], 
                gr_info *cod_info,
	            double xfsf[4][CBLIMIT])
/*************************************************************************/
/*   Function: Calculate the distortion introduced by the quantization   */
/*   in each scale factor band.                                          */
/*************************************************************************/
{
    int start, end, sfb, l, i;
    double sum,step,bw;
    double (*xr_s)[192][3]; /*D192_3 *xr_s;*/
    int    (*ix_s)[192][3]; /*I192_3 *ix_s;*/

    xr_s = (double (*)[192][3]) xr;
    ix_s = (int    (*)[192][3]) ix;

    step = pow(2.0, (cod_info->quantizerStepSize) * 0.25);
    for(sfb=0;sfb<cod_info->sfb_lmax;sfb++)
    {
        start = scalefac_band_long[sfb];
        end   = scalefac_band_long[sfb+1];
        bw    = end - start;

        for(sum=0.0,l=start;l<end;l++)
        {
            double temp;
            if (ix[l]<PRECALC_SIZE)
                temp = fabs(xr[l]) - pow43[ix[l]] * step;
            else
            {
               temp = fabs(xr[l]) - pow((double)ix[l],4.0/3.0)*step;
#if defined (_DEBUG)
                printf("EHHHHHHH !?!?! ---> %d\n",ix[l]);
#endif
            }
            sum += temp * temp; 
        }
        xfsf[0][sfb] = sum / bw;
    }

    for (i = 0; i < 3; i++)
    {
        step = pow(2.0, (cod_info->quantizerStepSize) * 0.25); /* subblock_gain ? */
        for (sfb = cod_info->sfb_smax; sfb < 12; sfb++)
        {
            start = scalefac_band_short[ sfb ];
            end   = scalefac_band_short[ sfb+1 ];
	    bw = end - start;
            

            for (sum = 0.0, l = start; l < end; l++)
            {

                double temp;
                if ((*ix_s)[l][i]<PRECALC_SIZE)
                    temp = fabs((*xr_s)[l][i]) - pow43[(*ix_s)[l][i]] * step;
                else
                {
                    temp = fabs((*xr_s)[l][i]) - pow((double)(*ix_s)[l][i],4.0/3.0)*step;
#if defined (_DEBUG)
                    printf("EHHHHHHH !?!?! ---> %d\n",(*ix_s)[l][i]);
#endif
                }
                sum += temp * temp;
            }       
            xfsf[i+1][sfb] = sum / bw;
        }
    }
}





void calc_xmin(double xr[2][2][576], 
               L3_psy_ratio_t *ratio,
	       gr_info *cod_info, 
               L3_psy_xmin_t *l3_xmin,
	       int gr, int ch)
/*************************************************************************/
/*  Calculate the allowed distortion for each scalefactor band,          */
/*  as determined by the psychoacoustic model.                           */
/*  xmin(sb) = ratio(sb) * en(sb) / bw(sb)                               */
/*************************************************************************/
{
    int start, end, sfb, l, b;
    double en, bw;

    double (*xr_s)[192][3]; /*D192_3 *xr_s;*/

    xr_s = (double (*)[192][3]) xr[gr][ch] ;

    for (sfb = cod_info->sfb_smax; sfb < SFB_SMAX - 1; sfb++)
    {
        start = scalefac_band_short[ sfb ];
        end   = scalefac_band_short[ sfb + 1 ];
	bw = end - start;
        for (b = 0; b < 3; b++)
        {
            for (en = 0.0, l = start; l < end; l++)
                en += (*xr_s)[l][b] * (*xr_s)[l][b];
            l3_xmin->s[gr][ch][sfb][b] = ratio->s[gr][ch][sfb][b] * en / bw;
        }
    }

    for (sfb = 0; sfb < cod_info->sfb_lmax; sfb++)
    {
        start = scalefac_band_long[ sfb ];
        end   = scalefac_band_long[ sfb+1 ];
	bw = end - start;

        for (en = 0.0, l = start; l < end; l++)
            en += xr[gr][ch][l] * xr[gr][ch][l];
        l3_xmin->l[gr][ch][sfb] = ratio->l[gr][ch][sfb] * en / bw;
    }
}



int loop_break(L3_scalefac_t *scalefac, 
               gr_info *cod_info,
	       int gr, int ch)
/*************************************************************************/
/*  Function: Returns zero if there is a scalefac which has not been     */
/*   amplified. Otherwise it returns one.                                */
/*************************************************************************/
{
    int i, sfb, temp = 1;

    for (sfb = 0; sfb < cod_info->sfb_lmax; sfb++)
        if (scalefac->l[gr][ch][sfb] == 0)
            temp = 0;

    for (sfb = cod_info->sfb_smax; sfb < 12; sfb++)
        for (i = 0; i < 3; i++)
            if (scalefac->s[gr][ch][sfb][i] == 0)
                temp = 0;
    return temp;
}





void preemphasis(double xr[576], 
                 double xfsf[4][CBLIMIT],
       	         L3_psy_xmin_t  *l3_xmin,
	         int gr, int ch, 
                 L3_side_info_t *l3_side)
/*************************************************************************/
/*  See ISO 11172-3  section  C.1.5.4.3.4                                */
/*************************************************************************/
{
    int i, sfb, start, end, scfsi_band, over;
    double ifqstep;
    gr_info *cod_info = &l3_side->gr[gr].ch[ch].tt;

    if (gr == 1)
    {
/*
If the second granule is being coded and scfsi is active in
at least one scfsi_band, the preemphasis in the second granule
is set equal to the setting in the first granule
*/
	for (scfsi_band = 0; scfsi_band < 4; scfsi_band++)
	    if (l3_side->scfsi[ch][scfsi_band])
	    {
		cod_info->preflag = l3_side->gr[0].ch[ch].tt.preflag;
		return;
	    }
	
    }

/*
Preemphasis is switched on if in all the upper four scalefactor
bands the actual distortion exceeds the threshold after the
first call of the inner loop
*/
    if (cod_info->block_type != 2 && cod_info->preflag == 0)
    {	
	over = 0;
	for (sfb = 17; sfb < 21; sfb++)
	    if (xfsf[0][sfb] > l3_xmin->l[gr][ch][sfb])
		over++;

	if (over == 4)
	{
	    cod_info->preflag = 1;
	    ifqstep = (cod_info->scalefac_scale == 0) ? SQRT2
		: pow(2.0, (0.5 * (1.0 + (double) cod_info->scalefac_scale)));

	    for (sfb = 0; sfb < cod_info->sfb_lmax; sfb++)
	    {
		l3_xmin->l[gr][ch][sfb] *= pow(ifqstep, 2.0 * (double) pretab[sfb]);
		start = scalefac_band_long[ sfb ];
		end   = scalefac_band_long[ sfb+1 ];
		for(i = start; i < end; i++)
		    xr[i] *= pow(ifqstep, (double) pretab[sfb]);
	    }
	}
    }
}




int amp_scalefac_bands(double xr[576], 
                       double xfsf[4][CBLIMIT],
		       L3_psy_xmin_t    *l3_xmin, 
                       L3_side_info_t *l3_side,
	               L3_scalefac_t  *scalefac,
		       int gr, int ch, int iteration)
/*************************************************************************/
/*  Amplify the scalefactor bands that violate the masking threshold.    */
/*  See ISO 11172-3 Section C.1.5.4.3.5                                  */
/*************************************************************************/
{
    int start, end, l, sfb, i, scfsi_band, over = 0;
    double ifqstep, ifqstep2;
    double (*xr_s)[192][3];
    gr_info *cod_info, *gr0;
    int copySF, preventSF;
    cod_info = &l3_side->gr[gr].ch[ch].tt;
    gr0      = &l3_side->gr[0].ch[ch].tt;

    xr_s = (double (*)[192][3]) xr;
    copySF = 0;
    preventSF = 0;

    if (cod_info->scalefac_scale == 0)
	ifqstep = SQRT2;
    else
	ifqstep = pow(2.0, 0.5 * (1.0 + (double) cod_info->scalefac_scale));

    if (gr == 1)
    {
	/*
	  If the second granule is being coded and scfsi is active in at
	  least one scfsi_band...
	*/
	for (scfsi_band = 0; scfsi_band < 4; scfsi_band++)
	    if (l3_side->scfsi[ch][scfsi_band])
	    {
		/*
		  a) ifqstep has to be set similar to the
		   first granule...
		*/
		if (gr0->scalefac_scale == 0)
		    ifqstep = SQRT2;
		else
		    ifqstep = pow(2.0, 0.5 * (1.0 + (double) gr0->scalefac_scale));

		if (iteration == 1)
		{
		    /*
		      b) If it is the first iteration, the scalefactors
		      of scalefactor bands in which scfsi is enabled
		      must be taken from the first granule
		    */  
		    copySF = 1;
		}
		else
		{
		    /*
		      c) If it is not the first iteration, the amplification
		      must be prevented for scalefactor bands in which
		      scfsi is enabled
		    */
		    preventSF = 1;
		}
		break;
	    }
	
    }

    ifqstep2 = ifqstep * ifqstep;
    scfsi_band = 0;
    
    for (sfb = 0; sfb < cod_info->sfb_lmax; sfb++)
    {
	if (copySF || preventSF)
	{
	    if (sfb == scfsi_band_long[scfsi_band + 1])
		scfsi_band += 1;
	    if (l3_side->scfsi[ch][scfsi_band])
	    {
		if (copySF)
		    scalefac->l[gr][ch][sfb] = scalefac->l[0][ch][sfb];
		continue;
	    }
	}	    
	if (xfsf[0][sfb] > l3_xmin->l[gr][ch][sfb])
	{
	    over++;
	    l3_xmin->l[gr][ch][sfb] *= ifqstep2;
	    scalefac->l[gr][ch][sfb]++;
	    start = scalefac_band_long[sfb];
	    end   = scalefac_band_long[sfb+1];
	    for (l = start; l < end; l++)
		xr[l] *= ifqstep;
	}
    }

/* Note that scfsi is not enabled for frames containing short blocks */
    for (i = 0; i < 3; i++)
        for (sfb = cod_info->sfb_smax; sfb < 12; sfb++)
            if (xfsf[i+1][sfb] > l3_xmin->s[gr][ch][sfb][i])
            {
                over++;
                l3_xmin->s[gr][ch][sfb][i] *= ifqstep2;
                scalefac->s[gr][ch][sfb][i]++;
                start = scalefac_band_short[sfb];
                end   = scalefac_band_short[sfb+1];
                for (l = start; l < end; l++)
                    (*xr_s)[l][i] *= ifqstep;
            }
    return over;
}







void quantize(double xr[576], int ix[576], gr_info *cod_info)
/*************************************************************************/
/*   Function: Quantization of the vector xr (-> ix)                    */
/*************************************************************************/
{
    register int i,b;
    int          idx,l_end, s_start;
    double       step,quantizerStepSize;
    float dbl;
    long ln;
    double (*xr_s)[192][3];
    int    (*ix_s)[192][3];

    xr_s = (double (*)[192][3]) xr;
    ix_s = (int    (*)[192][3]) ix;

    quantizerStepSize = (double) cod_info->quantizerStepSize;

    if (cod_info->quantizerStepSize==0.0) step = 1.0;
    else                                 step = pow(2.0,quantizerStepSize*0.25);

    if (cod_info->window_switching_flag != 0 && cod_info->block_type == 2)
	if (cod_info->mixed_block_flag == 0)
	{
	    s_start = 0;
	    l_end   = 0;
	}
	else
	{
	    s_start = 6 * 2;
	    l_end   = 18 * 2;
	}
    else
    {
	s_start = 192;
	l_end   = 576;
    }

    for(i=0;i<l_end;i++)
    {
        dbl = fabs(xr[i])/step;

        ln = (long)dbl;
        if (ln<10000)
        {
            idx=int2idx[ln];
            if (dbl<idx2dbl[idx+1]) ix[i] = idx;
            else                   ix[i] = idx+1;
        }
        else
        {
            dbl = sqrt(sqrt(dbl)*dbl);
            if (dbl<0.0946) ix[i] = (int)(dbl - 0.5946);
            else           ix[i] = (int)(dbl + 0.4154);
        }
    }
    for(;i<576;i++) ix[i] = 0;
    
    if (s_start < 192)
	for (b = 0; b < 3; b++)
	{
	    step = pow(2.0, (quantizerStepSize + 8.0 * (double) cod_info->subblock_gain[b]) * 0.25);
	    for (i=s_start;i<192;i++)
            {
                dbl = fabs((*xr_s)[i][b])/step;

                ln = (long)dbl;
                if (ln<10000)
                {
                    idx=int2idx[ln];
                    if (dbl<idx2dbl[idx+1]) (*ix_s)[i][b] = idx;
                    else                   (*ix_s)[i][b] = idx+1;
                }
                else
                {
                    dbl = sqrt(sqrt(dbl)*dbl);
                    if (dbl<0.0946) (*ix_s)[i][b] = (int)(dbl - 0.5946);
                    else           (*ix_s)[i][b] = (int)(dbl + 0.4154);
                }

            }
	}
 }




int ix_max(int ix[576], unsigned int begin, unsigned int end)
/*************************************************************************/
/*  Function: Calculate the maximum of ix from 0 to 575                  */
/*************************************************************************/
{
    register int i;
    register int x;
    register int max = 0;

    for(i=begin;i<end;i++)
    { 
        x = abs(ix[i]);
        if (x>max) max=x;
    }
    return max;
}



/* THIS FUNCTION HAS BEEN CHANGED ..... */
double xr_max(double xr[576], unsigned int begin, unsigned int end)
/*************************************************************************/
/* Function: Calculate the maximum of xr[576]  from 0 to 575             */
/*************************************************************************/
{
    register int i;
    double max = 0.0, temp;

    for(i=begin;i<end;i++)
    {
        temp = fabs(xr[i]);
        if (temp>max) max=temp;
    }
    return max;
}




/*************************************************************************/
/*   Noiseless coding  -- Huffman coding                                 */
/*************************************************************************/






void calc_runlen(int ix[576], gr_info *cod_info)
/*************************************************************************/
/* Function: Calculation of rzero, count1, big_values                    */
/* (Partitions ix into big values, quadruples and zeros).                */
/*************************************************************************/
{
    int i;
    int rzero = 0; 

    if (cod_info->window_switching_flag && (cod_info->block_type == 2))
    {  /* short blocks */
        cod_info->count1 = 0;
        cod_info->big_values = 288;
    }
    else
    {
        for (i = 576; i > 1; i -= 2)
            if (ix[i-1] == 0 && ix[i-2] == 0)
                rzero++;
            else
                break;
        
        cod_info->count1 = 0 ;
        for (; i > 3; i -= 4)
            if (abs(ix[i-1]) <= 1
              && abs(ix[i-2]) <= 1
              && abs(ix[i-3]) <= 1
              && abs(ix[i-4]) <= 1)
                cod_info->count1++;
            else
                break;
        
        cod_info->big_values = i/2;
    }
    if ((2*rzero+4*cod_info->count1+2*cod_info->big_values)!=576) ERROR("bladibla!=576");
}




/* THIS FUNCTION HAS BEEN CHANGED ..... */
int count1_bitcount(int ix[576], gr_info *cod_info)
/*************************************************************************/
/* Determines the number of bits to encode the quadruples.               */
/*************************************************************************/
{
    int p, i, k;
    int v, w, x, y, signbits;
    int sum0 = 0,
        sum1 = 0;

    for(i=cod_info->big_values*2, k=0; k<cod_info->count1; i+=4, k++)
    {
        v = abs(ix[i]);
        w = abs(ix[i+1]);
        x = abs(ix[i+2]);
        y = abs(ix[i+3]);

        p = v + (w<<1) + (x<<2) + (y<<3);
        
        signbits = 0;
        if (v!=0) signbits++;
        if (w!=0) signbits++;
        if (x!=0) signbits++;
        if (y!=0) signbits++;

        sum0 += signbits;
        sum1 += signbits;

        sum0 += ht[32].hlen[p];
        sum1 += ht[33].hlen[p];
    }

    if (sum0<sum1)
    {
        cod_info->count1table_select = 0;
        return sum0;
    }
    else
    {
        cod_info->count1table_select = 1;
        return sum1;
    }
}






void subdivide(gr_info *cod_info)
/*************************************************************************/
/* presumable subdivides the bigvalue region which will use separate Huffman tables.  */
/*************************************************************************/
{
    int scfb_anz = 0;
    int bigvalues_region;
    
    if (cod_info->big_values == 0)
    { /* no big_values region */
        cod_info->region0_count = 0;
        cod_info->region1_count = 0;
    }
    else
    {
        bigvalues_region = 2 * cod_info->big_values;

        if ((cod_info->window_switching_flag == 0))
        { /* long blocks */
            int thiscount, index;
            /* Calculate scfb_anz */
            while (scalefac_band_long[scfb_anz] < bigvalues_region)
                scfb_anz++;
            if (scfb_anz>=23) ERROR("scbf >= 23 !!");

            cod_info->region0_count = subdv_table[scfb_anz].region0_count;
            thiscount = cod_info->region0_count;
            index = thiscount + 1;
            while (thiscount && (scalefac_band_long[index] > bigvalues_region))
            {
                thiscount -= 1;
                index -= 1;
            }
            cod_info->region0_count = thiscount;

            cod_info->region1_count = subdv_table[scfb_anz].region1_count;
            index = cod_info->region0_count + cod_info->region1_count + 2;
            thiscount = cod_info->region1_count;
            while (thiscount && (scalefac_band_long[index] > bigvalues_region))
            {
                thiscount -= 1;
                index -= 1;
            }
            cod_info->region1_count = thiscount;
            cod_info->address1 = scalefac_band_long[cod_info->region0_count+1];
            cod_info->address2 = scalefac_band_long[cod_info->region0_count
                                                    + cod_info->region1_count + 2 ];
            cod_info->address3 = bigvalues_region;
        }
        else
        {
            if ((cod_info->block_type == 2) && (cod_info->mixed_block_flag == 0))
            { 
                cod_info->region0_count =  8;
                cod_info->region1_count =  36;
                cod_info->address1 = 36;
                cod_info->address2 = bigvalues_region;
                cod_info->address3 = 0;  
            }
            else
            {
                cod_info->region0_count = 7;
                cod_info->region1_count = 13;
                cod_info->address1 = scalefac_band_long[cod_info->region0_count+1];
                cod_info->address2 = bigvalues_region;
                cod_info->address3 = 0;
            }
        }
    }
}






void bigv_tab_select(int ix[576], gr_info *cod_info)
/*************************************************************************/
/*   Function: Select huffman code tables for bigvalues regions */
/*************************************************************************/
{
    cod_info->table_select[0] = 0;
    cod_info->table_select[1] = 0;
    cod_info->table_select[2] = 0;
    
    if (cod_info->window_switching_flag && (cod_info->block_type==2))
    {
        /*
          Within each scalefactor band, data is given for successive
          time windows, beginning with window 0 and ending with window 2.
          Within each window, the quantized values are then arranged in
          order of increasing frequency...
          */
        int sfb, window, line, start, end, max1, max2, x, y;
        int region1Start;
        int *pmax;

        region1Start = 12;
        max1 = max2 = 0;

        for(sfb=0;sfb<13;sfb++)
        {
            start = scalefac_band_short[ sfb ];
            end   = scalefac_band_short[ sfb+1 ];
            
            if (start < region1Start)
                pmax = &max1;
            else
                pmax = &max2;
            
            for(window=0;window<3;window++)
                for(line=start;line<end;line+=2)
                {
                    if ((line<0)||(line>576)) ERROR("Invalid line selected...");

                    x = abs(ix[ (line * 3) + window ]);
                    y = abs(ix[ ((line + 1) * 3) + window ]);
                    *pmax = *pmax > x ? *pmax : x;
                    *pmax = *pmax > y ? *pmax : y;
                }
        }
        cod_info->table_select[0] = choose_table(max1);
        cod_info->table_select[1] = choose_table(max2);
    }
    else
    {
        if (cod_info->address1 > 0)
            cod_info->table_select[0] = new_choose_table(ix, 0, cod_info->address1);

        if (cod_info->address2 > cod_info->address1)
            cod_info->table_select[1] = new_choose_table(ix, cod_info->address1, cod_info->address2);

        if (cod_info->big_values * 2 > cod_info->address2)
            cod_info->table_select[2] = new_choose_table(ix, cod_info->address2, cod_info->big_values * 2);
    }
}





int new_choose_table(int ix[576], unsigned int begin, unsigned int end)
/*************************************************************************/
/*  Choose the Huffman table that will encode ix[begin..end] with             */
/*  the fewest bits.                                                          */
/*  Note: This code contains knowledge about the sizes and characteristics    */
/*  of the Huffman tables as defined in the IS (Table B.7), and will not work */
/*  with any arbitrary tables.                                                */
/*************************************************************************/
{
    int i, max;
    int choice[2];
    int sum[2];

    max = ix_max(ix,begin,end);
    if (max==0) return 0;

    choice[0] = 0;
    choice[1] = 0;

    if (max<15)
    {
	/* try tables with no linbits */
        for (i = 0; i < 14; i++)
            if (ht[i].xlen > max)
	    {
		choice[ 0 ] = i;
                break;
	    }
	if (!choice[0]) ERROR("Can't choose a table ....");

	sum[ 0 ] = count_bit(ix, begin, end, choice[0]);

	switch (choice[0])
	{
	  case 2:
	    sum[ 1 ] = count_bit(ix, begin, end, 3);
	    if (sum[1] <= sum[0])
		choice[ 0 ] = 3;
	    break;

	  case 5:
	    sum[ 1 ] = count_bit(ix, begin, end, 6);
	    if (sum[1] <= sum[0])
		choice[ 0 ] = 6;
	    break;

	  case 7:
	    sum[ 1 ] = count_bit(ix, begin, end, 8);
	    if (sum[1] <= sum[0])
	    {
		choice[ 0 ] = 8;
		sum[ 0 ] = sum[ 1 ];
	    }
	    sum[ 1 ] = count_bit(ix, begin, end, 9);
	    if (sum[1] <= sum[0])
		choice[ 0 ] = 9;
	    break;

	  case 10:
	    sum[ 1 ] = count_bit(ix, begin, end, 11);
	    if (sum[1] <= sum[0])
	    {
		choice[ 0 ] = 11;
		sum[ 0 ] = sum[ 1 ];
	    }
	    sum[ 1 ] = count_bit(ix, begin, end, 12);
	    if (sum[1] <= sum[0])
		choice[ 0 ] = 12;
	    break;

	  case 13:
	    sum[ 1 ] = count_bit(ix, begin, end, 15);
	    if (sum[1] <= sum[0])
		choice[ 0 ] = 15;
	    break;

	  default:
	    break;
	}
    }
    else
    {
	/* try tables with linbits */
	max -= 15;
	
	for(i=15;i<24;i++)
	{
	    if (ht[i].linmax>=max)
	    {
		choice[ 0 ] = i;
		break;
	    }
	}
	for(i=24;i<32;i++)
	{
	    if (ht[i].linmax>=max)
	    {
		choice[ 1 ] = i;
		break;
	    }
	}
        if (!choice[0] || !choice[1]) ERROR("Hmmmppfff...");
	
	sum[0] = count_bit(ix,begin,end,choice[0]);
	sum[1] = count_bit(ix,begin,end,choice[1]);
	if (sum[1]<sum[0]) choice[0] = choice[1];
    }
    return choice[0];
}




int choose_table(int max)
/*************************************************************************/
/*            choose table                                               */
/*************************************************************************/
{
    int  i, choice;

    if (max==0) return 0;
    
    max = abs(max);    
    choice = 0;

    if (max<15)
    {
        for(i=0;i<15;i++)
        {
            if (ht[i].xlen>max)
            {
		choice = i;
		break;
            }
        }
    }
    else
    {	
	max -= 15;
	for(i=15;i<32;i++)
	{
	    if (ht[i].linmax>=max)
	    {
		choice = i;
		break;
	    }
	}
    }
    if (!choice) ERROR("Can't decide what table to use...");
    return choice;
}




int bigv_bitcount(int ix[576], gr_info *gi)
/*************************************************************************/
/* Function: Count the number of bits necessary to code the bigvalues region.  */
/*************************************************************************/
{
    int bits = 0;
    
    if (gi->window_switching_flag && gi->block_type == 2)
    {
/* Within each scalefactor band, data is given for successive
   time windows, beginning with window 0 and ending with window 2.
   Within each window, the quantized values are then arranged in
   order of increasing frequency...
*/
        int sfb, window, line, start, end;
        int (*ix_s)[192][3];

        if (gi->mixed_block_flag)
        {
            unsigned int table;

            if ((table = gi->table_select[0]) != 0)
                bits += count_bit(ix, 0, gi->address1, table);
            sfb = 2;
        }
        else
            sfb = 0;

        ix_s = (int (*)[192][3]) &ix[0];

        for (; sfb < 13; sfb++)
        {
            unsigned tableindex = 100;

            start = scalefac_band_short[ sfb   ];
            end   = scalefac_band_short[ sfb+1 ];

            if (start<12) tableindex = gi->table_select[ 0 ];
            else         tableindex = gi->table_select[ 1 ];
            if (tableindex>=32) ERROR("No valid table...");

            for (window = 0; window < 3; window++)
                for (line = start; line < end; line += 2)
                {
                    unsigned int code, ext;
                    int cbits, xbits;
                    int x = (*ix_s)[line][window];
                    int y = (*ix_s)[line + 1][window];
                    bits += HuffmanCode(tableindex, x, y, &code, &ext, &cbits, &xbits);
                }
        }
    }
    else
    {
        unsigned int table;
        
        if ((table=gi->table_select[0]) != 0)  /* region0 */ 
            bits += count_bit(ix, 0, gi->address1, table);
        if ((table=gi->table_select[1]) != 0)  /* region1 */ 
            bits += count_bit(ix, gi->address1, gi->address2, table);
        if ((table=gi->table_select[2]) != 0)  /* region2 */ 
            bits += count_bit(ix, gi->address2, gi->address3, table);
    }
    return bits;
}



int count_bit(int ix[576], 
              unsigned int start, 
              unsigned int end,
              unsigned int table)
/*************************************************************************/
/* Function: Count the number of bits necessary to code the subregion. */
/*************************************************************************/
{
    unsigned            linbits, ylen;
    register int        i, sum;
    register int        x,y;
    struct huffcodetab *h;

    if (table==0) return 0;

    h   = &(ht[table]);
    sum = 0;

    ylen    = h->ylen;
    linbits = h->linbits;

    if (table>15)
    { /* ESC-table is used */
        for(i=start;i<end;i+=2)
        {
            x = ix[i];
            y = ix[i+1];
            if (x>14)
            {
                x = 15;
                sum += linbits;
            }
            if (y>14)
            {
                y = 15;
                sum += linbits;
            }

            sum += h->hlen[(x*ylen)+y];

            if (x!=0) sum++;
            if (y!=0) sum++;
        }
    }
    else
    { /* No ESC-words */
        for(i=start;i<end;i+=2)
        {
            x = ix[i];
            y = ix[i+1];

            sum  += h->hlen[(x*ylen)+y];

            if (x!=0) sum++;
            if (y!=0) sum++;
        }
    }

    return sum;
}






/*
  Seymour's comment:  Jan 8 1995
  When mixed_block_flag is set, the low subbands 0-1 undergo the long
  window transform and are each split into 18 frequency lines, while
  the remaining 30 subbands undergo the short window transform and are
  each split into 6 frequency lines. A problem now arises, as neither
  the short or long scale factor bands apply to this mixed spectrum.
  The standard resolves this situation by using the first 8 long scale
  factor bands for the low spectrum and the short scale factor bands
  in the range of 3 to 11 (inclusive) for the remaining frequency lines.
  These scale factor bands do not match exactly to the 0-1 subbands
  for all sampling frequencies (32,44.1 and 48 kHz); however they
  were designed so that there would not be a frequency gap or overlap
  at the switch over point. (Note multiply short frequency lines by 3
  to account for wider frequency line.) 
*/



/*************************************************************************/
/*            gr_deco                                                    */
/*************************************************************************/

void gr_deco(gr_info *cod_info)
{
    if (cod_info->window_switching_flag != 0 && cod_info->block_type == 2)
        if (cod_info->mixed_block_flag == 0)
        {
            cod_info->sfb_lmax = 0; /* No sb*/
            cod_info->sfb_smax = 0;
        }
        else
        {
            cod_info->sfb_lmax = 8;
            cod_info->sfb_smax = 3;
        }
    else
    {
        cod_info->sfb_lmax = SFB_LMAX - 1;
        cod_info->sfb_smax = SFB_SMAX - 1;    /* No sb */
    }
}








/* The following optional code written by Seymour Shlien
   will speed up the outer_loop code which is called
   by iteration_loop. When BIN_SEARCH is defined, the
   outer_loop function precedes the call to the function inner_loop
   with a call to bin_search gain defined below, which
   returns a good starting quantizerStepSize.
*/

int count_bits(int *ix /*int[576]*/, gr_info *cod_info)
{
    int bits,max;

    calc_runlen(ix,cod_info);		  /* rzero,count1,big_values*/

    max = ix_max(ix, 0,576);
    if (max > 8192) return 100000;         /* report unsuitable quantizer */

    bits = count1_bitcount(ix, cod_info); /* count1_table selection*/
    subdivide(cod_info);		  /* bigvalues sfb division */
    bigv_tab_select(ix,cod_info);         /* codebook selection*/
    bits += bigv_bitcount(ix,cod_info);	  /* bit count */
    return bits;
}



int bin_search_StepSize(int desired_rate, double start, int *ix,
                        double xrs[576], gr_info * cod_info)
{
    int top,bot,next,last;
    int bit;

    top  = (int)start;
    next = (int)start;
    bot  = 200;

    do
    {
        last = next;
        next = ((long)(top+bot) >> 1);

        cod_info->quantizerStepSize = next;
        quantize(xrs,ix,cod_info);
        bit = count_bits(ix,cod_info);

        if (bit>desired_rate) top = next;
        else                  bot = next;
    }
    while((bit!=desired_rate) && abs(last-next)>1);

    return next;
}


