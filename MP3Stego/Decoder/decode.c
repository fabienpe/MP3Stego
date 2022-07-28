/**********************************************************************
 * ISO MPEG Audio Subgroup Software Simulation Group (1996)
 * ISO 13818-3 MPEG-2 Audio Decoder - Lower Sampling Frequency Extension
 *
 * $Header: /MP3Stego/MP3Stego Decoder/decode.c 5     30/10/98 11:24 Fapp2 $
 *
 * $Id: decode.c,v 1.2 1996/03/28 03:13:37 rowlands Exp $
 *
 * $Log: /MP3Stego/MP3Stego Decoder/decode.c $
 * 
 * 5     30/10/98 11:24 Fapp2
 * 
 * 4     15/08/98 10:40 Fapp2
 * Started revision control on this file.
 * Revision 1.2  1996/03/28 03:13:37  rowlands
 * Merged layers 1-2 and layer 3 revisions
 *
 * Revision 1.1  1996/02/14 03:45:52  rowlands
 * Initial revision
 *
 * Received from FhG
 **********************************************************************/
/**********************************************************************
 *   date   programmers         comment                               *
 * 2/25/91  Douglas Wong,       start of version 1.0 records          *
 *          Davis Pan                                                 *
 * 3/06/91  Douglas Wong        rename: setup.h to dedef.h            *
 *                                      dfilter to defilter           *
 *                                      dwindow to dewindow           *
 *                              integrated "quantizer", "scalefactor" *
 *                              combined window_samples routine into  *
 *                              filter samples                        *
 * 3/31/91  Bill Aspromonte     replaced read_filter by               *
 *                              create_syn_filter and introduced a    *
 *                              new Sub-Band Synthesis routine called *
 *                              SubBandSynthesis()                    *
 * 5/10/91  Vish (PRISM)        Ported to Macintosh and Unix.         *
 *                              Changed "out_fifo()" so that last     *
 *                              unfilled block is also written out.   *
 *                              "create_syn_filter()" was modified so *
 *                              that calculation precision is same as *
 *                              in specification tables.              *
 *                              Changed "decode_scale()" to reflect   *
 *                              specifications.                       *
 *                              Removed all routines used by          *
 *                              "synchronize_buffer()".  This is now  *
 *                              replaced by "seek_sync()".            *
 *                              Incorporated Jean-Georges Fritsch's   *
 *                              "bitstream.c" package.                *
 *                              Deleted "reconstruct_sample()".       *
 * 27jun91  dpwe (Aware)        Passed outFile and &sampFrames as     *
 *                              args to out_fifo() - were global.     *
 *                              Moved "alloc_*" reader to common.c.   *
 *                              alloc, sblimit, stereo passed via new *
 *                              'frame_params struct (were globals).  *
 *                              Added JOINT STEREO decoding, lyrs I&II*
 *                              Affects: decode_bitalloc,buffer_samps *
 *                              Plus a few other cleanups.            *
 * 6/10/91   Earle Jennings     conditional expansion added in        *
 *                              II_dequantize_sample to handle range  *
 *                              problems in MSDOS version             *
 * 8/8/91    Jens Spille        Change for MS-C6.00                   *
 *10/1/91    S.I. Sudharsanan,  Ported to IBM AIX platform.           *
 *           Don H. Lee,                                              *
 *           Peter W. Farrett                                         *
 *10/3/91    Don H. Lee         implemented CRC-16 error protection   *
 *                              newly introduced functions are        *
 *                              buffer_CRC and recover_CRC_error.     *
 * 2/11/92  W. Joseph Carter    Ported new code to Macintosh.  Most   *
 *                              important fixes involved changing     *
 *                              16-bit ints to long or unsigned in    *
 *                              bit alloc routines for quant of 65535 *
 *                              and passing proper function args.     *
 *                              Removed "Other Joint Stereo" option   *
 *                              and made bitrate be total channel     *
 *                              bitrate, irrespective of the mode.    *
 *                              Fixed many small bugs & reorganized.  *
 * 7/27/92  Juan Pineda         Bug fix in SubBandSynthesis()         *
 *--------------------------------------------------------------------*
 * 6/14/92  Juan Pineda         Layer III decoding routines added.    *
 *          Amit Gulati         Follows CD 3-11172 rev2.  Contains    *
 *                              hacks deal with evolving available    *
 *                              layerIII bitstreams.  Some (minor)    *
 *                              modification of prior LI&II code.     *
 * 10/25/92 Amit Gulati         Updated layerIII routines. Added code *
 *                              for subblock_gain, switched block     *
 *                              modes, stereo pre-processing.         *
 *                              Corrected sign bits for huffman       *
 *                              decoding of quadruples region and     *
 *                              adjusted gain factor in III_dequant.  *
 * 11/21/92 Amit Gulati         Several layerIII bugs fixed.          *
 * 12/15/92 Amit Gulati         Corrected reordering (indexing)       *
 *          Stan Searing        within IMDCT routine.                 *
 *  8/24/93 Masahiro Iwadare    Included IS modification in Layer III.*
 *                              Changed for 1 pass decoding.          *
 *  9/07/93 Toshiyuki Ishino    Integrated Layer III with Ver 3.9.    *
 *--------------------------------------------------------------------*
 * 11/20/93 Masahiro Iwadare    Integrated Layer III with Ver 4.0.    *
 *--------------------------------------------------------------------*
 *  7/14/94 Juergen Koller      Bug fixes in Layer III code           *
 *--------------------------------------------------------------------*
 * 08/11/94 IIS                 Bug fixes in Layer III code           *
 *--------------------------------------------------------------------*
 * 9/20/94  Davis Pan           Modification to avoid premature       *
 *                              synchword detection                   *
 *--------------------------------------------------------------------*
 * 11/09/94 Jon Rowlands        Merged premature synchword detection  *
 *                              fix into layer III code version       *
 *--------------------------------------------------------------------*
 * 07/12/95 Soeren H. Nielsen   Changes for LSF Layer I and II        *
 *--------------------------------------------------------------------*
 *    8/95  Roland Bitto	    Adapted to MPEG2                      *
 *  9/8/95  Roalnd Bitto        Bugfix in Function III_stereo         *
 *--------------------------------------------------------------------*
 *  Aug 98  Fabien Petitcolas   Add stego stuff                       *
 **********************************************************************/

#include "common.h"
#include "decoder.h"
#include "huffman.h"

#include "../../stegolib/stego.h"

/***************************************************************
/*
/* This module contains the core of the decoder ie all the
/* computational routines. (Layer I and II only)
/* Functions are common to both layer unless
/* otherwise specified.
/*
/***************************************************************/

/*****************************************************************
/*
/* The following routines decode the system information
/*
/****************************************************************/

/************ Layer I, Layer II & Layer III ******************/

void decode_info(Bit_stream_struc *bs, frame_params *fr_ps)
{
    layer *hdr = fr_ps->header;

    hdr->version = get1bit(bs);
    hdr->lay = 4-getbits(bs,2);
    hdr->error_protection = !get1bit(bs); /* error protect. TRUE/FALSE */
    hdr->bitrate_index = getbits(bs,4);
    hdr->sampling_frequency = getbits(bs,2);
    hdr->padding = get1bit(bs);
    hdr->extension = get1bit(bs);
    hdr->mode = getbits(bs,2);
    hdr->mode_ext = getbits(bs,2);
    hdr->copyright = get1bit(bs);
    hdr->original = get1bit(bs);
    hdr->emphasis = getbits(bs,2);
}

/*******************************************************************
/*
/* The bit allocation information is decoded. Layer I
/* has 4 bit per subband whereas Layer II is Ws and bit rate
/* dependent.
/*
/********************************************************************/

/**************************** Layer II *************/

void II_decode_bitalloc(Bit_stream_struc *bs, 
                        unsigned int bit_alloc[2][SBLIMIT], 
                        frame_params *fr_ps)
{
    int i,j;
    int stereo = fr_ps->stereo;
    int sblimit = fr_ps->sblimit;
    int jsbound = fr_ps->jsbound;
    al_table *alloc = fr_ps->alloc;

    for (i=0;i<jsbound;i++) for (j=0;j<stereo;j++)
        bit_alloc[j][i] = (char) getbits(bs,(*alloc)[i][0].bits);

    for (i=jsbound;i<sblimit;i++) /* expand to 2 channels */
        bit_alloc[0][i] = bit_alloc[1][i] =
            (char) getbits(bs,(*alloc)[i][0].bits);

    for (i=sblimit;i<SBLIMIT;i++) for (j=0;j<stereo;j++)
        bit_alloc[j][i] = 0;
}

/**************************** Layer I *************/

void I_decode_bitalloc(Bit_stream_struc *bs, 
                       unsigned int bit_alloc[2][SBLIMIT], 
                       frame_params *fr_ps)
{
    int i,j;
    int stereo  = fr_ps->stereo;
    int sblimit = fr_ps->sblimit;
    int jsbound = fr_ps->jsbound;
    int b;

    for (i=0;i<jsbound;i++) for (j=0;j<stereo;j++)
        bit_alloc[j][i] = getbits(bs,4);
    for (i=jsbound;i<SBLIMIT;i++) {
        b = getbits(bs,4);
        for (j=0;j<stereo;j++)
            bit_alloc[j][i] = b;
    }
}

/*****************************************************************
/*
/* The following two functions implement the layer I and II
/* format of scale factor extraction. Layer I involves reading
/* 6 bit per subband as scale factor. Layer II requires reading
/* first the scfsi which in turn indicate the number of scale factors
/* transmitted.
/*    Layer I : I_decode_scale
/*   Layer II : II_decode_scale
/*
/****************************************************************/

/************************** Layer I stuff ************************/

void I_decode_scale(Bit_stream_struc *bs, 
                    unsigned int bit_alloc[2][SBLIMIT], 
                    unsigned int scale_index[2][3][SBLIMIT], 
                    frame_params *fr_ps)
{
    int i,j;
    int stereo = fr_ps->stereo;
    int sblimit = fr_ps->sblimit;

    for (i=0;i<SBLIMIT;i++) for (j=0;j<stereo;j++)
        if (!bit_alloc[j][i])
            scale_index[j][0][i] = SCALE_RANGE-1;
        else                    /* 6 bit per scale factor */
            scale_index[j][0][i] =  getbits(bs,6);

}

/*************************** Layer II stuff ***************************/

void II_decode_scale(Bit_stream_struc *bs, unsigned int scfsi[2][SBLIMIT], 
                     unsigned int bit_alloc[2][SBLIMIT], 
                     unsigned int scale_index[2][3][SBLIMIT], frame_params *fr_ps)
{
    int i,j;
    int stereo = fr_ps->stereo;
    int sblimit = fr_ps->sblimit;
   
    for (i=0;i<sblimit;i++) for (j=0;j<stereo;j++) /* 2 bit scfsi */
        if (bit_alloc[j][i]) scfsi[j][i] = (char) getbits(bs,2);
    for (i=sblimit;i<SBLIMIT;i++) for (j=0;j<stereo;j++)   
        scfsi[j][i] = 0;

    for (i=0;i<sblimit;i++) for (j=0;j<stereo;j++) {
        if (bit_alloc[j][i])   
            switch (scfsi[j][i]) {
                /* all three scale factors transmitted */
             case 0 : scale_index[j][0][i] = getbits(bs,6);
                scale_index[j][1][i] = getbits(bs,6);
                scale_index[j][2][i] = getbits(bs,6);
                break;
                /* scale factor 1 & 3 transmitted */
             case 1 : scale_index[j][0][i] =
                 scale_index[j][1][i] = getbits(bs,6);
                scale_index[j][2][i] = getbits(bs,6);
                break;
                /* scale factor 1 & 2 transmitted */
             case 3 : scale_index[j][0][i] = getbits(bs,6);
                scale_index[j][1][i] =
                    scale_index[j][2][i] =  getbits(bs,6);
                break;
                /* only one scale factor transmitted */
             case 2 : scale_index[j][0][i] =
                 scale_index[j][1][i] =
                     scale_index[j][2][i] = getbits(bs,6);
                break;
                default : break;
            }
        else {
            scale_index[j][0][i] = scale_index[j][1][i] =
                scale_index[j][2][i] = SCALE_RANGE-1;
        }
    }
    for (i=sblimit;i<SBLIMIT;i++) for (j=0;j<stereo;j++) {
        scale_index[j][0][i] = scale_index[j][1][i] =
            scale_index[j][2][i] = SCALE_RANGE-1;
    }
}

/**************************************************************
/*
/*   The following two routines take care of reading the
/* compressed sample from the bit stream for both layer 1 and
/* layer 2. For layer 1, read the number of bits as indicated
/* by the bit_alloc information. For layer 2, if grouping is
/* indicated for a particular subband, then the sample size has
/* to be read from the bits_group and the merged samples has
/* to be decompose into the three distinct samples. Otherwise,
/* it is the same for as layer one.
/*
/**************************************************************/

/***************************** Layer I stuff ******************/

void I_buffer_sample(Bit_stream_struc *bs, 
                     unsigned int FAR sample[2][3][SBLIMIT], 
                     unsigned int bit_alloc[2][SBLIMIT], 
                     frame_params *fr_ps)
{
    int i,j,k;
    int stereo = fr_ps->stereo;
    int sblimit = fr_ps->sblimit;
    int jsbound = fr_ps->jsbound;
    unsigned int s;

    for (i=0;i<jsbound;i++) for (j=0;j<stereo;j++)
        if ( (k = bit_alloc[j][i]) == 0)
            sample[j][0][i] = 0;
        else 
            sample[j][0][i] = (unsigned int) getbits(bs,k+1);
    for (i=jsbound;i<SBLIMIT;i++) {
        if ( (k = bit_alloc[0][i]) == 0)
            s = 0;
        else 
            s = (unsigned int)getbits(bs,k+1);
        for (j=0;j<stereo;j++)
            sample[j][0][i]    = s;
    }
}

/*************************** Layer II stuff ************************/

void II_buffer_sample(Bit_stream_struc *bs,
                      unsigned int FAR sample[2][3][SBLIMIT],
                      unsigned int bit_alloc[2][SBLIMIT],
                      frame_params *fr_ps)
{
    int i,j,k,m;
    int stereo = fr_ps->stereo;
    int sblimit = fr_ps->sblimit;
    int jsbound = fr_ps->jsbound;
    al_table *alloc = fr_ps->alloc;

    for (i=0;i<sblimit;i++) for (j=0;j<((i<jsbound)?stereo:1);j++) {
        if (bit_alloc[j][i]) {
            /* check for grouping in subband */
            if ((*alloc)[i][bit_alloc[j][i]].group==3)
                for (m=0;m<3;m++) {
                    k = (*alloc)[i][bit_alloc[j][i]].bits;
                    sample[j][m][i] = (unsigned int) getbits(bs,k);
                }         
            else {              /* bit_alloc = 3, 5, 9 */
                unsigned int nlevels, c=0;

                nlevels = (*alloc)[i][bit_alloc[j][i]].steps;
                k=(*alloc)[i][bit_alloc[j][i]].bits;
                c = (unsigned int) getbits(bs, k);
                for (k=0;k<3;k++) {
                    sample[j][k][i] = c % nlevels;
                    c /= nlevels;
                }
            }
        }
        else {                  /* for no sample transmitted */
            for (k=0;k<3;k++) sample[j][k][i] = 0;
        }
        if(stereo == 2 && i>= jsbound) /* joint stereo : copy L to R */
            for (k=0;k<3;k++) sample[1][k][i] = sample[0][k][i];
    }
    for (i=sblimit;i<SBLIMIT;i++) for (j=0;j<stereo;j++) for (k=0;k<3;k++)
        sample[j][k][i] = 0;
}      

/**************************************************************
/*
/*   Restore the compressed sample to a factional number.
/*   first complement the MSB of the sample
/*    for layer I :
/*    Use s = (s' + 2^(-nb+1) ) * 2^nb / (2^nb-1)
/*   for Layer II :
/*   Use the formula s = s' * c + d
/*
/**************************************************************/

static double c[17] = { 1.33333333333, 1.60000000000, 1.14285714286,
                        1.77777777777, 1.06666666666, 1.03225806452,
                        1.01587301587, 1.00787401575, 1.00392156863,
                        1.00195694716, 1.00097751711, 1.00048851979,
                        1.00024420024, 1.00012208522, 1.00006103888,
                        1.00003051851, 1.00001525902 };

static double d[17] = { 0.500000000, 0.500000000, 0.250000000, 0.500000000,
                        0.125000000, 0.062500000, 0.031250000, 0.015625000,
                        0.007812500, 0.003906250, 0.001953125, 0.0009765625,
                        0.00048828125, 0.00024414063, 0.00012207031,
                        0.00006103516, 0.00003051758 };

/************************** Layer II stuff ************************/

void II_dequantize_sample(unsigned int FAR sample[2][3][SBLIMIT],
                          unsigned int bit_alloc[2][SBLIMIT], 
                          double FAR fraction[2][3][SBLIMIT],
                          frame_params *fr_ps)
{
    int i, j, k, x;
    int stereo = fr_ps->stereo;
    int sblimit = fr_ps->sblimit;
    al_table *alloc = fr_ps->alloc;

    for (i=0;i<sblimit;i++)  for (j=0;j<3;j++) for (k=0;k<stereo;k++)
        if (bit_alloc[k][i]) {

            /* locate MSB in the sample */
            x = 0;
#ifndef MS_DOS
            while ((1L<<x) < (*alloc)[i][bit_alloc[k][i]].steps) x++;
#else
            /* microsoft C thinks an int is a short */
            while (( (unsigned long) (1L<<(long)x) <
                    (unsigned long)( (*alloc)[i][bit_alloc[k][i]].steps)
                    ) && ( x < 16) ) x++;
#endif

            /* MSB inversion */
            if (((sample[k][j][i] >> x-1) & 1) == 1)
                fraction[k][j][i] = 0.0;
            else  fraction[k][j][i] = -1.0;

            /* Form a 2's complement sample */
            fraction[k][j][i] += (double) (sample[k][j][i] & ((1<<x-1)-1)) /
                (double) (1L<<x-1);

            /* Dequantize the sample */
            fraction[k][j][i] += d[(*alloc)[i][bit_alloc[k][i]].quant];
            fraction[k][j][i] *= c[(*alloc)[i][bit_alloc[k][i]].quant];
        }
        else fraction[k][j][i] = 0.0;   
   
    for (i=sblimit;i<SBLIMIT;i++) for (j=0;j<3;j++) for(k=0;k<stereo;k++)
        fraction[k][j][i] = 0.0;
}

/***************************** Layer I stuff ***********************/

void I_dequantize_sample(unsigned int FAR sample[2][3][SBLIMIT], 
                         double FAR fraction[2][3][SBLIMIT], 
                         unsigned int bit_alloc[2][SBLIMIT], 
                         frame_params *fr_ps)
{
    int i, nb, k;
    int stereo = fr_ps->stereo;
    int sblimit = fr_ps->sblimit;

    for (i=0;i<SBLIMIT;i++)
        for (k=0;k<stereo;k++)
            if (bit_alloc[k][i]) {
                nb = bit_alloc[k][i] + 1;
                if (((sample[k][0][i] >> nb-1) & 1) == 1) fraction[k][0][i] = 0.0;
                else fraction[k][0][i] = -1.0;
                fraction[k][0][i] += (double) (sample[k][0][i] & ((1<<nb-1)-1)) /
                    (double) (1L<<nb-1);

                fraction[k][0][i] =
                    (double) (fraction[k][0][i]+1.0/(double)(1L<<nb-1)) *
                        (double) (1L<<nb) / (double) ((1L<<nb)-1);
            }
            else fraction[k][0][i] = 0.0;
}

/************************************************************
/*
/*   Restore the original value of the sample ie multiply
/*    the fraction value by its scalefactor.
/*
/************************************************************/

/************************* Layer II Stuff **********************/

void II_denormalize_sample(double FAR fraction[2][3][SBLIMIT],
                           unsigned int scale_index[2][3][SBLIMIT],
                           frame_params *fr_ps,
                           int x)
{
    int i,j;
    int stereo = fr_ps->stereo;
    int sblimit = fr_ps->sblimit;

    for (i=0;i<sblimit;i++) for (j=0;j<stereo;j++) {
        fraction[j][0][i] *= multiple[scale_index[j][x][i]];
        fraction[j][1][i] *= multiple[scale_index[j][x][i]];
        fraction[j][2][i] *= multiple[scale_index[j][x][i]];
    }
}

/**************************** Layer I stuff ******************************/

void I_denormalize_sample(double FAR fraction[2][3][SBLIMIT],
                          unsigned int scale_index[2][3][SBLIMIT],
                          frame_params *fr_ps)
{
    int i,j;
    int stereo = fr_ps->stereo;
    int sblimit = fr_ps->sblimit;

    for (i=0;i<SBLIMIT;i++) for (j=0;j<stereo;j++)
        fraction[j][0][i] *= multiple[scale_index[j][0][i]];
}

/*****************************************************************
/*
/* The following are the subband synthesis routines. They apply
/* to both layer I and layer II stereo or mono. The user has to
/* decide what parameters are to be passed to the routines.
/*
/***************************************************************/

/*************************************************************
/*
/*   Pass the subband sample through the synthesis window
/*
/**************************************************************/

/* create in synthesis filter */

void create_syn_filter(double FAR filter[64][SBLIMIT])
{
    register int i,k;

    for (i=0; i<64; i++)
        for (k=0; k<32; k++) {
            if ((filter[i][k] = 1e9*cos((double)((PI64*i+PI4)*(2*k+1)))) >= 0)
                modf(filter[i][k]+0.5, &filter[i][k]);
            else
                modf(filter[i][k]-0.5, &filter[i][k]);
            filter[i][k] *= 1e-9;
        }
}

/***************************************************************
/*
/*   Window the restored sample
/*
/***************************************************************/

/* read in synthesis window */

void read_syn_window(double FAR window[HAN_SIZE])
{
    int i,j[4];
    FILE *fp;
    double f[4];
    char t[150];

    if (!(fp = OpenTableFile("dewindow") )) {
        printf("Please check synthesis window table 'dewindow'\n");
        exit(1);
    }
    for (i=0;i<512;i+=4) {
        fgets(t, 150, fp);
        sscanf(t,"D[%d] = %lf D[%d] = %lf D[%d] = %lf D[%d] = %lf\n",
               j, f,j+1,f+1,j+2,f+2,j+3,f+3);
        if (i==j[0]) {
            window[i] = f[0];
            window[i+1] = f[1];
            window[i+2] = f[2];
            window[i+3] = f[3];
        }
        else {
            printf("Check index in synthesis window table\n");
            exit(1);
        }
        fgets(t,150,fp);
    }
    fclose(fp);
}

int SubBandSynthesis (double *bandPtr, int channel, short *samples)
{
    register int i,j,k;
    register double *bufOffsetPtr, sum;
    register long foo;
    static int init = 1;
    typedef double NN[64][32];
    static NN FAR *filter;
    typedef double BB[2][2*HAN_SIZE];
    static BB FAR *buf;
    static int bufOffset[2] = {64,64};
    static double FAR *window;
    int clip = 0;               /* count & return how many samples clipped */

    if (init) {
        buf = (BB FAR *) mem_alloc(sizeof(BB),"BB");
        filter = (NN FAR *) mem_alloc(sizeof(NN), "NN");
        create_syn_filter(*filter);
        window = (double FAR *) mem_alloc(sizeof(double) * HAN_SIZE, "WIN");
        read_syn_window(window);
        init = 0;
    }
/*    if (channel == 0) */
    bufOffset[channel] = (bufOffset[channel] - 64) & 0x3ff;
    bufOffsetPtr = &((*buf)[channel][bufOffset[channel]]);

    for (i=0; i<64; i++) {
        sum = 0;
        for (k=0; k<32; k++)
            sum += bandPtr[k] * (*filter)[i][k];
        bufOffsetPtr[i] = sum;
    }
    /*  S(i,j) = D(j+32i) * U(j+32i+((i+1)>>1)*64)  */
    /*  samples(i,j) = MWindow(j+32i) * bufPtr(j+32i+((i+1)>>1)*64)  */
    for (j=0; j<32; j++) {
        sum = 0;
        for (i=0; i<16; i++) {
            k = j + (i<<5);
            sum += window[k] * (*buf) [channel] [( (k + ( ((i+1)>>1) <<6) ) +
                                                  bufOffset[channel]) & 0x3ff];
        }

/*      Casting truncates towards zero for both positive and negative numbers,
	the result is cross-over distortion,  1995-07-12 shn */

        if(sum > 0)
        {
          foo = (long)(sum * (double) SCALE + (double)0.5);
        }
        else
        {
           foo = (long)(sum * (double)SCALE -(double)0.5);
        } 

     if (foo >= (long) SCALE)      {samples[j] = (short)(SCALE-1); ++clip;}
     else if (foo < (long) -SCALE) {samples[j] = (short)(-SCALE);  ++clip;}
     else                           samples[j] =(short)foo;
    }
    return(clip);
}

void out_fifo(short FAR pcm_sample[2][SSLIMIT][SBLIMIT],
              int num, frame_params *fr_ps, int done,
              FILE *outFile, unsigned long *psampFrames)
{
    int i,j,l;
    int stereo = fr_ps->stereo;
    int sblimit = fr_ps->sblimit;
    static short int outsamp[1600];
    static long k = 0;

    if (!done)
        for (i=0;i<num;i++) for (j=0;j<SBLIMIT;j++) {
            (*psampFrames)++;
            for (l=0;l<stereo;l++) {
                if (!(k%1600) && k) {
		    /*
		      Samples are big-endian. If this is a little-endian machine
		      we must swap
		      */
		    if ( NativeByteOrder == order_unknown )
		    {
			NativeByteOrder = DetermineByteOrder();
			if ( NativeByteOrder == order_unknown )
			{
			    fprintf( stderr, "byte order not determined\n" );
			    exit( 1 );
			}
		    }
		    if ( NativeByteOrder == order_littleEndian )
			SwapBytesInWords( outsamp, 1600 );
		    
                    fwrite(outsamp,2,1600,outFile);
                    k = 0;
                }
                outsamp[k++] = pcm_sample[l][i][j];
            }
        }
    else {
        fwrite(outsamp,2,(int)k,outFile);
        k = 0;
    }
}

void  buffer_CRC(Bit_stream_struc  *bs, unsigned int  *old_crc)
{
    *old_crc = getbits(bs, 16);
}

void  recover_CRC_error(short FAR pcm_sample[2][SSLIMIT][SBLIMIT],
                        int error_count, frame_params *fr_ps, FILE *outFile,
                        unsigned long *psampFrames)
{
    int  stereo = fr_ps->stereo;
    int  num, done, i;
    int  samplesPerFrame, samplesPerSlot;
    layer  *hdr = fr_ps->header;
    long  offset;
    short  *temp;

    num = 3;
    if (hdr->lay == 1) num = 1;

    samplesPerSlot = SBLIMIT * num * stereo;
    samplesPerFrame = samplesPerSlot * 32;

    if (error_count == 1) {     /* replicate previous error_free frame */
        done = 1;
        /* flush out fifo */
        out_fifo(pcm_sample, num, fr_ps, done, outFile, psampFrames);
        /* go back to the beginning of the previous frame */
        offset = sizeof(short int) * samplesPerFrame;
        fseek(outFile, -offset, SEEK_CUR);
        done = 0;
        for (i = 0; i < SCALE_BLOCK; i++) {
            fread(pcm_sample, 2, samplesPerSlot, outFile);
            out_fifo(pcm_sample, num, fr_ps, done, outFile, psampFrames);
        }
    }
    else{                       /* mute the frame */
        temp = (short*) pcm_sample;
        done = 0;
        for (i = 0; i < 2*3*SBLIMIT; i++)
            *temp++ = MUTE;     /* MUTE value is in decoder.h */
        for (i = 0; i < SCALE_BLOCK; i++)
            out_fifo(pcm_sample, num, fr_ps, done, outFile, psampFrames);
    }
}

/************************* Layer III routines **********************/

void III_get_side_info(Bit_stream_struc *bs,
                       III_side_info_t *si, 
                       frame_params *fr_ps)
{
   int ch, gr, i;
   int stereo = fr_ps->stereo;
   
   if(fr_ps->header->version != MPEG_PHASE2_LSF)
   {
      si->main_data_begin = getbits(bs, 9);
      if (stereo == 1)
         si->private_bits = getbits(bs,5);
         else si->private_bits = getbits(bs,3);

       for (ch=0; ch<stereo; ch++)
           for (i=0; i<4; i++)
           si->ch[ch].scfsi[i] = get1bit(bs);
 
  
        for (gr=0; gr< 2 ; gr++) {
           for (ch=0; ch<stereo; ch++) {
              si->ch[ch].gr[gr].part2_3_length = getbits(bs, 12);
			  
			  /* STEGO */
			  //SaveHiddenBit(si->ch[ch].gr[gr].part2_3_length % 2);
			  /* STEGO */

              si->ch[ch].gr[gr].big_values = getbits(bs, 9);
              si->ch[ch].gr[gr].global_gain = getbits(bs, 8);
              si->ch[ch].gr[gr].scalefac_compress = getbits(bs, 4);
              si->ch[ch].gr[gr].window_switching_flag = get1bit(bs);
              if (si->ch[ch].gr[gr].window_switching_flag) {
                 si->ch[ch].gr[gr].block_type = getbits(bs, 2);
                 si->ch[ch].gr[gr].mixed_block_flag = get1bit(bs);
                 for (i=0; i<2; i++)
                    si->ch[ch].gr[gr].table_select[i] = getbits(bs, 5);
                 for (i=0; i<3; i++)
                    si->ch[ch].gr[gr].subblock_gain[i] = getbits(bs, 3);
               
            /* Set region_count parameters since they are implicit in this case. */
            
                 if (si->ch[ch].gr[gr].block_type == 0) {
                   printf("Side info bad: block_type == 0 in split block.\n");
                   exit(0);
                 }
                 else if (si->ch[ch].gr[gr].block_type == 2
                      && si->ch[ch].gr[gr].mixed_block_flag == 0)
                             si->ch[ch].gr[gr].region0_count = 8; /* MI 9; */
                 else si->ch[ch].gr[gr].region0_count = 7; /* MI 8; */
                      si->ch[ch].gr[gr].region1_count = 20 -
						si->ch[ch].gr[gr].region0_count;
             }
            else {
               for (i=0; i<3; i++)
                   si->ch[ch].gr[gr].table_select[i] = getbits(bs, 5);
               si->ch[ch].gr[gr].region0_count = getbits(bs, 4);
               si->ch[ch].gr[gr].region1_count = getbits(bs, 3);
               si->ch[ch].gr[gr].block_type = 0;
            }      
         si->ch[ch].gr[gr].preflag = get1bit(bs);
         si->ch[ch].gr[gr].scalefac_scale = get1bit(bs);
         si->ch[ch].gr[gr].count1table_select = get1bit(bs);
         }
      }
    }     
    else /* Layer 3 LSF */
    {     
        si->main_data_begin = getbits(bs, 8);
        if (stereo == 1)
                 si->private_bits = getbits(bs,1);
         else si->private_bits = getbits(bs,2);

        for (gr=0; gr< 1 ; gr++) {
           for (ch=0; ch<stereo; ch++) {
              si->ch[ch].gr[gr].part2_3_length = getbits(bs, 12);

			  /* STEGO */
			  //SaveHiddenBit(si->ch[ch].gr[gr].part2_3_length % 2);
			  /* STEGO */

              si->ch[ch].gr[gr].big_values = getbits(bs, 9);
              si->ch[ch].gr[gr].global_gain = getbits(bs, 8);
              si->ch[ch].gr[gr].scalefac_compress = getbits(bs, 9);
              si->ch[ch].gr[gr].window_switching_flag = get1bit(bs);
              if (si->ch[ch].gr[gr].window_switching_flag) {
                 si->ch[ch].gr[gr].block_type = getbits(bs, 2);
                 si->ch[ch].gr[gr].mixed_block_flag = get1bit(bs);
                 for (i=0; i<2; i++)
                    si->ch[ch].gr[gr].table_select[i] = getbits(bs, 5);
                 for (i=0; i<3; i++)
                    si->ch[ch].gr[gr].subblock_gain[i] = getbits(bs, 3);
               
            /* Set region_count parameters since they are implicit in this case. */
            
                 if (si->ch[ch].gr[gr].block_type == 0) {
                   printf("Side info bad: block_type == 0 in split block.\n");
                   exit(0);
                 }
                 else if (si->ch[ch].gr[gr].block_type == 2
                      && si->ch[ch].gr[gr].mixed_block_flag == 0)
                             si->ch[ch].gr[gr].region0_count = 8; /* MI 9; */
                 else si->ch[ch].gr[gr].region0_count = 7; /* MI 8; */
                      si->ch[ch].gr[gr].region1_count = 20 - si->ch[ch].gr[gr].region0_count;

              }
              else {
                for (i=0; i<3; i++)
                   si->ch[ch].gr[gr].table_select[i] = getbits(bs, 5);
               si->ch[ch].gr[gr].region0_count = getbits(bs, 4);
               si->ch[ch].gr[gr].region1_count = getbits(bs, 3);
               si->ch[ch].gr[gr].block_type = 0;
              }      

         si->ch[ch].gr[gr].scalefac_scale = get1bit(bs);
         si->ch[ch].gr[gr].count1table_select = get1bit(bs);
         }
     }
  }
}

void III_put_side_info(Bit_stream_struc *bs, III_side_info_t *si,
                       frame_params *fr_ps)
{
   int ch, gr, i;
   int stereo = fr_ps->stereo;

 if(fr_ps->header->version != MPEG_PHASE2_LSF)
 {
   putbits(bs, si->main_data_begin,9);
   if (stereo == 1)
      putbits(bs, si->private_bits, 5);
      else putbits(bs, si->private_bits, 3);

   for (ch=0; ch<stereo; ch++)
      for (i=0; i<4; i++)
         put1bit(bs, si->ch[ch].scfsi[i]);

   for (gr=0; gr<2; gr++) {
      for (ch=0; ch<stereo; ch++) {
         putbits(bs, si->ch[ch].gr[gr].part2_3_length, 12);
         putbits(bs, si->ch[ch].gr[gr].big_values, 9);
         putbits(bs, si->ch[ch].gr[gr].global_gain, 8);
         putbits(bs, si->ch[ch].gr[gr].scalefac_compress, 4);
         put1bit(bs, si->ch[ch].gr[gr].window_switching_flag);
         if (si->ch[ch].gr[gr].window_switching_flag) {
            putbits(bs, si->ch[ch].gr[gr].block_type, 2);
            put1bit(bs, si->ch[ch].gr[gr].mixed_block_flag);
            for (i=0; i<2; i++)
               putbits(bs, si->ch[ch].gr[gr].table_select[i], 5);
            for (i=0; i<3; i++)
               putbits(bs, si->ch[ch].gr[gr].subblock_gain[i], 3);
            }
         else {
            for (i=0; i<3; i++)
            putbits(bs, si->ch[ch].gr[gr].table_select[i], 5);
            putbits(bs, si->ch[ch].gr[gr].region0_count, 4);
            putbits(bs, si->ch[ch].gr[gr].region1_count, 3);
            }      

         put1bit(bs, si->ch[ch].gr[gr].preflag);
         put1bit(bs, si->ch[ch].gr[gr].scalefac_scale);
         put1bit(bs, si->ch[ch].gr[gr].count1table_select);
         }
      }
 }
 else /* Layer 3 LSF */
 {
    putbits(bs, si->main_data_begin,8);
    if (stereo == 1)
      putbits(bs, si->private_bits, 1);
      else putbits(bs, si->private_bits, 2);

   
     for (gr=0; gr<1; gr++) {
       for (ch=0; ch<stereo; ch++) {
         putbits(bs, si->ch[ch].gr[gr].part2_3_length, 12);
         putbits(bs, si->ch[ch].gr[gr].big_values, 9);
         putbits(bs, si->ch[ch].gr[gr].global_gain, 8);
         putbits(bs, si->ch[ch].gr[gr].scalefac_compress, 9);
         put1bit(bs, si->ch[ch].gr[gr].window_switching_flag);
         if (si->ch[ch].gr[gr].window_switching_flag) {
            putbits(bs, si->ch[ch].gr[gr].block_type, 2);
            put1bit(bs, si->ch[ch].gr[gr].mixed_block_flag);
            for (i=0; i<2; i++)
               putbits(bs, si->ch[ch].gr[gr].table_select[i], 5);
            for (i=0; i<3; i++)
               putbits(bs, si->ch[ch].gr[gr].subblock_gain[i], 3);
            }
         else 
           {  
            for (i=0; i<3; i++)
            putbits(bs, si->ch[ch].gr[gr].table_select[i], 5);
            putbits(bs, si->ch[ch].gr[gr].region0_count, 4);
            putbits(bs, si->ch[ch].gr[gr].region1_count, 3);
           }      

         put1bit(bs, si->ch[ch].gr[gr].scalefac_scale);
         put1bit(bs, si->ch[ch].gr[gr].count1table_select);
       }
     }
  }
}

struct {
   int l[5];
   int s[3];} sfbtable = {{0, 6, 11, 16, 21},
                          {0, 6, 12}};
                         
int slen[2][16] = {{0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4},
                   {0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3}};
struct  {
   int l[23];
   int s[14];} sfBandIndex[6] =
    {{{0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
     {0,4,8,12,18,24,32,42,56,74,100,132,174,192}},
    {{0,6,12,18,24,30,36,44,54,66,80,96,114,136,162,194,232,278,330,394,464,540,576},
     {0,4,8,12,18,26,36,48,62,80,104,136,180,192}},
    {{0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
     {0,4,8,12,18,26,36,48,62,80,104,134,174,192}},

    {{0,4,8,12,16,20,24,30,36,44,52,62,74,90,110,134,162,196,238,288,342,418,576},
     {0,4,8,12,16,22,30,40,52,66,84,106,136,192}},
    {{0,4,8,12,16,20,24,30,36,42,50,60,72,88,106,128,156,190,230,276,330,384,576},
     {0,4,8,12,16,22,28,38,50,64,80,100,126,192}},
    {{0,4,8,12,16,20,24,30,36,44,54,66,82,102,126,156,194,240,296,364,448,550,576},
     {0,4,8,12,16,22,30,42,58,78,104,138,180,192}}};



void III_get_scale_factors(III_scalefac_t *scalefac,
                           III_side_info_t *si,
                           int gr, int ch,
                           frame_params *fr_ps)
{
    int sfb, i, window;
    struct gr_info_s *gr_info = &(si->ch[ch].gr[gr]);

    if (gr_info->window_switching_flag && (gr_info->block_type == 2)) { 
      if (gr_info->mixed_block_flag) { /* MIXED */ /* NEW - ag 11/25 */
         for (sfb = 0; sfb < 8; sfb++)
            (*scalefac)[ch].l[sfb] = hgetbits( 
                 slen[0][gr_info->scalefac_compress]);
         for (sfb = 3; sfb < 6; sfb++)
            for (window=0; window<3; window++)
               (*scalefac)[ch].s[window][sfb] = hgetbits(
                 slen[0][gr_info->scalefac_compress]);
         for (sfb = 6; sfb < 12; sfb++)
            for (window=0; window<3; window++)
               (*scalefac)[ch].s[window][sfb] = hgetbits(
                 slen[1][gr_info->scalefac_compress]);
         for (sfb=12,window=0; window<3; window++)
            (*scalefac)[ch].s[window][sfb] = 0;
      }
      else {  /* SHORT*/
         for (i=0; i<2; i++) 
            for (sfb = sfbtable.s[i]; sfb < sfbtable.s[i+1]; sfb++)
               for (window=0; window<3; window++)
                  (*scalefac)[ch].s[window][sfb] = hgetbits( 
                    slen[i][gr_info->scalefac_compress]);
         for (sfb=12,window=0; window<3; window++)
            (*scalefac)[ch].s[window][sfb] = 0;
      }
    }          
    else {   /* LONG types 0,1,3 */
        for (i=0; i<4; i++) {
           if ((si->ch[ch].scfsi[i] == 0) || (gr == 0))
              for (sfb = sfbtable.l[i]; sfb < sfbtable.l[i+1]; sfb++)
                  (*scalefac)[ch].l[sfb] = hgetbits(
                 slen[(i<2)?0:1][gr_info->scalefac_compress]);
        }
        (*scalefac)[ch].l[22] = 0; 
    }
}

/****************** new MPEG2 stuf  ***********/

static unsigned nr_of_sfb_block[6][3][4] = {{{6, 5, 5, 5},{ 9, 9, 9, 9 },{6, 9, 9, 9}},
                                         {{6, 5, 7, 3},{ 9, 9, 12, 6},{6, 9, 12, 6}},
                                         {{11, 10, 0, 0},{ 18, 18, 0, 0},{15,18,0,0 }},
                                         {{7, 7, 7, 0},{ 12, 12, 12, 0},{6, 15, 12, 0}},
                                         {{6, 6, 6, 3},{12, 9, 9, 6},{6, 12, 9, 6}},
                                         {{8, 8, 5, 0},{15,12,9,0},{6,18,9,0}}};
static unsigned scalefac_buffer[54];

void III_get_LSF_scale_data(III_scalefac_t *scalefac,
                            III_side_info_t *si,
                            int gr, int ch,
                            frame_params *fr_ps)
{
    short i, j, k;
    short blocktypenumber, blocknumber;

    struct gr_info_s *gr_info = &(si->ch[ch].gr[gr]);
    unsigned scalefac_comp, int_scalefac_comp, new_slen[4];
   
    layer *hdr = fr_ps->header;
    scalefac_comp =  gr_info->scalefac_compress;

    blocktypenumber = 0;
    if ((gr_info->block_type == 2) && (gr_info->mixed_block_flag == 0))
                                                        blocktypenumber = 1;

   if ((gr_info->block_type == 2) && (gr_info->mixed_block_flag == 1))
                                                        blocktypenumber = 2;

    if(!((( hdr->mode_ext == 1) || (hdr->mode_ext == 3)) && (ch == 1)))
    {
	if(scalefac_comp < 400)
        {
		new_slen[0] = (scalefac_comp >> 4) / 5 ;
		new_slen[1] = (scalefac_comp >> 4) % 5 ;
		new_slen[2] = (scalefac_comp % 16) >> 2 ;
		new_slen[3] = (scalefac_comp % 4);
                si->ch[ch].gr[gr].preflag = 0;

                blocknumber = 0;
         }

	else if( scalefac_comp  < 500)
        {
		new_slen[0] = ((scalefac_comp - 400 )  >> 2) / 5 ;
		new_slen[1] = ((scalefac_comp - 400) >> 2) % 5 ;
		new_slen[2] = (scalefac_comp - 400 ) % 4 ;
		new_slen[3] = 0;
                si->ch[ch].gr[gr].preflag = 0;
                blocknumber = 1;

        }

	else if( scalefac_comp  < 512)
        {
		new_slen[0] = (scalefac_comp - 500 ) / 3 ;
		new_slen[1] = (scalefac_comp - 500)  % 3 ;
		new_slen[2] = 0 ;
		new_slen[3] = 0;
                si->ch[ch].gr[gr].preflag = 1;
                blocknumber = 2;

        }
     }

    if((((hdr->mode_ext == 1) || (hdr->mode_ext == 3)) && (ch == 1)))
    {
      /*   intensity_scale = scalefac_comp %2; */
         int_scalefac_comp = scalefac_comp >> 1;
	
        if(int_scalefac_comp  < 180)
        {
		new_slen[0] = int_scalefac_comp  / 36 ;
		new_slen[1] = (int_scalefac_comp % 36 ) / 6 ;
		new_slen[2] = (int_scalefac_comp % 36) % 6;
		new_slen[3] = 0;
                si->ch[ch].gr[gr].preflag = 0;
                blocknumber = 3;

         }

	else if( int_scalefac_comp  < 244)
        {
		new_slen[0] = ((int_scalefac_comp - 180 )  % 64 ) >> 4 ;
		new_slen[1] = ((int_scalefac_comp - 180) % 16) >> 2 ;
		new_slen[2] = (int_scalefac_comp - 180 ) % 4 ;
		new_slen[3] = 0;
                si->ch[ch].gr[gr].preflag = 0;
                blocknumber = 4;

        }

	else if( int_scalefac_comp  < 255)
        {
		new_slen[0] = (int_scalefac_comp - 244 ) / 3 ;
		new_slen[1] = (int_scalefac_comp - 244 )  % 3 ;
		new_slen[2] = 0 ;
		new_slen[3] = 0;
                si->ch[ch].gr[gr].preflag = 0;
                blocknumber = 5;

        }
     }
     
     for(i=0;i< 45;i++) scalefac_buffer[i] = 0;

     k = 0;
     for(i = 0;i < 4;i++)
     { 
      	for(j = 0; j < nr_of_sfb_block[blocknumber][blocktypenumber][i]; j++)
        {
           if(new_slen[i] == 0)
           {
	        scalefac_buffer[k] = 0;
           }
           else
           {   
     	       scalefac_buffer[k] =  hgetbits(new_slen[i]);
           }
           k++;
 
        }
     }

}



void III_get_LSF_scale_factors(III_scalefac_t *scalefac,
                               III_side_info_t *si,
                               int gr, int ch,
                               frame_params *fr_ps)
{
    int sfb, k = 0, window;
    struct gr_info_s *gr_info = &(si->ch[ch].gr[gr]);
 
    III_get_LSF_scale_data(scalefac, si, gr, ch, fr_ps);


    if (gr_info->window_switching_flag && (gr_info->block_type == 2)) { 
      if (gr_info->mixed_block_flag) 
      {                                       /* MIXED */ /* NEW - ag 11/25 */
         for (sfb = 0; sfb < 8; sfb++)
         {
              (*scalefac)[ch].l[sfb] = scalefac_buffer[k];
              k++;
         } 
         for (sfb = 3; sfb < 12; sfb++)
            for (window=0; window<3; window++)
            {
               (*scalefac)[ch].s[window][sfb] = scalefac_buffer[k];
               k++;
            }
            for (sfb=12,window=0; window<3; window++)
                     (*scalefac)[ch].s[window][sfb] = 0;

       }
       else {  /* SHORT*/
           for (sfb = 0; sfb < 12; sfb++)
               for (window=0; window<3; window++)
               {
                  (*scalefac)[ch].s[window][sfb] = scalefac_buffer[k];
                  k++;
               }
               for (sfb=12,window=0; window<3; window++)
                                   (*scalefac)[ch].s[window][sfb] = 0;
      }
    }          
    else {   /* LONG types 0,1,3 */

           for (sfb = 0; sfb < 21; sfb++)
            {
                  (*scalefac)[ch].l[sfb] = scalefac_buffer[k];
                   k++;
            }
            (*scalefac)[ch].l[22] = 0; 

     
    }
}



/* Already declared in huffman.c
struct huffcodetab ht[HTN];
*/
int huffman_initialized = FALSE;

void initialize_huffman() {
   FILE *fi;
  
   if (huffman_initialized) return;
   if (!(fi = OpenTableFile("huffdec") )) {
      printf("Please check huffman table 'huffdec'\n");
      exit(1);
   }

   if (fi==NULL) {

      fprintf(stderr,"decoder table open error\n");

      exit(3);

      }

   if (read_decoder_table(fi) != HTN) {
      fprintf(stderr,"decoder table read error\n");
      exit(4);
      }
huffman_initialized = TRUE;
}

void III_hufman_decode(long int is[SBLIMIT][SSLIMIT],
                  III_side_info_t *si,
                  int ch, int gr, int part2_start,
                  frame_params *fr_ps)
{
   int i, x, y;
   int v, w;
   struct huffcodetab *h;
   int region1Start;
   int region2Start;
   int sfreq;
   int currentBit, grBits;
   my_gr_info *gi;
   int bt = (*si).ch[ch].gr[gr].window_switching_flag && ((*si).ch[ch].gr[gr].block_type == 2);

   gi = (my_gr_info *) &(*si).ch[ch].gr[gr];
   sfreq = fr_ps->header->sampling_frequency + (fr_ps->header->version * 3);
   initialize_huffman();

   /* Find region boundary for short block case. */
   
   if ( ((*si).ch[ch].gr[gr].window_switching_flag) && 
        ((*si).ch[ch].gr[gr].block_type == 2) ) { 
   
      /* Region2. */
        region1Start = 36;  /* sfb[9/3]*3=36 */
        region2Start = 576; /* No Region2 for short block case. */
   }
   else {          /* Find region boundary for long block case. */

      region1Start = sfBandIndex[sfreq]
                           .l[(*si).ch[ch].gr[gr].region0_count + 1]; /* MI */
      region2Start = sfBandIndex[sfreq]
                              .l[(*si).ch[ch].gr[gr].region0_count +
                              (*si).ch[ch].gr[gr].region1_count + 2]; /* MI */
      }


   grBits     = part2_start + (*si).ch[ch].gr[gr].part2_3_length;
   currentBit = hsstell();

   /* Read bigvalues area. */
   for (i=0; i<(*si).ch[ch].gr[gr].big_values*2; i+=2) {
      if      (i<region1Start) h = &ht[(*si).ch[ch].gr[gr].table_select[0]];
      else if (i<region2Start) h = &ht[(*si).ch[ch].gr[gr].table_select[1]];
           else                h = &ht[(*si).ch[ch].gr[gr].table_select[2]];
      huffman_decoder(h, &x, &y, &v, &w);
      is[i/SSLIMIT][i%SSLIMIT] = x;
      is[(i+1)/SSLIMIT][(i+1)%SSLIMIT] = y;
      }

   grBits     = part2_start + (*si).ch[ch].gr[gr].part2_3_length;
   currentBit = hsstell();

   /* Read count1 area. */
   h = &ht[(*si).ch[ch].gr[gr].count1table_select+32];
   while ((hsstell() < part2_start + (*si).ch[ch].gr[gr].part2_3_length ) &&
     ( i < SSLIMIT*SBLIMIT )) {
      huffman_decoder(h, &x, &y, &v, &w);
      is[i/SSLIMIT][i%SSLIMIT] = v;
      is[(i+1)/SSLIMIT][(i+1)%SSLIMIT] = w;
      is[(i+2)/SSLIMIT][(i+2)%SSLIMIT] = x;
      is[(i+3)/SSLIMIT][(i+3)%SSLIMIT] = y;
      i += 4;
      }

   grBits     = part2_start + (*si).ch[ch].gr[gr].part2_3_length;
   currentBit = hsstell();

   if (hsstell() > part2_start + (*si).ch[ch].gr[gr].part2_3_length)
   {  i -=4;
      rewindNbits(hsstell()-part2_start - (*si).ch[ch].gr[gr].part2_3_length);
   }

   /* Dismiss stuffing Bits */
   grBits     = part2_start + (*si).ch[ch].gr[gr].part2_3_length;
   currentBit = hsstell();
   if ( currentBit < grBits )
      hgetbits( grBits - currentBit );

   SaveHiddenBit((*si).ch[ch].gr[gr].part2_3_length % 2);

   /* Zero out rest. */
   for (; i<SSLIMIT*SBLIMIT; i++)
      is[i/SSLIMIT][i%SSLIMIT] = 0;
}


int pretab[22] = {0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,3,3,3,2,0};

void III_dequantize_sample(long int is[SBLIMIT][SSLIMIT],
                           double xr[SBLIMIT][SSLIMIT],
                           III_scalefac_t *scalefac,
                           struct gr_info_s *gr_info,
                           int ch,
                           frame_params *fr_ps)
{
   int ss,sb,cb=0,sfreq;
   int stereo = fr_ps->stereo;
   int next_cb_boundary, cb_begin, cb_width, sign;

   sfreq=fr_ps->header->sampling_frequency + (fr_ps->header->version * 3);

   /* choose correct scalefactor band per block type, initalize boundary */

   if (gr_info->window_switching_flag && (gr_info->block_type == 2) )
      if (gr_info->mixed_block_flag) 
         next_cb_boundary=sfBandIndex[sfreq].l[1];  /* LONG blocks: 0,1,3 */
      else {
         next_cb_boundary=sfBandIndex[sfreq].s[1]*3; /* pure SHORT block */
    cb_width = sfBandIndex[sfreq].s[1];
    cb_begin = 0;
      }  
   else 
      next_cb_boundary=sfBandIndex[sfreq].l[1];  /* LONG blocks: 0,1,3 */

   /* apply formula per block type */

   for (sb=0 ; sb < SBLIMIT ; sb++)
      for (ss=0 ; ss < SSLIMIT ; ss++) {

         if ( (sb*18)+ss == next_cb_boundary)  { /* Adjust critical band boundary */
            if (gr_info->window_switching_flag && (gr_info->block_type == 2)) {
               if (gr_info->mixed_block_flag)  {
                  if (((sb*18)+ss) == sfBandIndex[sfreq].l[8])  {
                     next_cb_boundary=sfBandIndex[sfreq].s[4]*3; 
                     cb = 3;
                     cb_width = sfBandIndex[sfreq].s[cb+1] - 
                                sfBandIndex[sfreq].s[cb];
                     cb_begin = sfBandIndex[sfreq].s[cb]*3;      
                  }
                  else if (((sb*18)+ss) < sfBandIndex[sfreq].l[8]) 
                     next_cb_boundary = sfBandIndex[sfreq].l[(++cb)+1];
                  else {
                     next_cb_boundary = sfBandIndex[sfreq].s[(++cb)+1]*3;
                     cb_width = sfBandIndex[sfreq].s[cb+1] - 
                                    sfBandIndex[sfreq].s[cb];
                     cb_begin = sfBandIndex[sfreq].s[cb]*3;      
                  }   
               }
               else  {
                  next_cb_boundary = sfBandIndex[sfreq].s[(++cb)+1]*3;
                  cb_width = sfBandIndex[sfreq].s[cb+1] - 
                               sfBandIndex[sfreq].s[cb];
                cb_begin = sfBandIndex[sfreq].s[cb]*3;      
               } 
            }
            else /* long blocks */
               next_cb_boundary = sfBandIndex[sfreq].l[(++cb)+1];
         }

         /* Compute overall (global) scaling. */

         xr[sb][ss] = pow( 2.0 , (0.25 * (gr_info->global_gain - 210.0)));

         /* Do long/short dependent scaling operations. */
        
         if (gr_info->window_switching_flag && (
            ((gr_info->block_type == 2) && (gr_info->mixed_block_flag == 0)) ||
            ((gr_info->block_type == 2) && gr_info->mixed_block_flag && (sb >= 2)) )) {

            xr[sb][ss] *= pow(2.0, 0.25 * -8.0 * 
                    gr_info->subblock_gain[(((sb*18)+ss) - cb_begin)/cb_width]);
            xr[sb][ss] *= pow(2.0, 0.25 * -2.0 * (1.0+gr_info->scalefac_scale)
              * (*scalefac)[ch].s[(((sb*18)+ss) - cb_begin)/cb_width][cb]);
         }
         else {   /* LONG block types 0,1,3 & 1st 2 subbands of switched blocks */
            xr[sb][ss] *= pow(2.0, -0.5 * (1.0+gr_info->scalefac_scale)
                                        * ((*scalefac)[ch].l[cb]
                                        + gr_info->preflag * pretab[cb]));
         }

         /* Scale quantized value. */
        
         sign = (is[sb][ss]<0) ? 1 : 0; 
         xr[sb][ss] *= pow( (double) abs(is[sb][ss]), ((double)4.0/3.0) );
         if (sign) xr[sb][ss] = -xr[sb][ss];
      }
}


void III_reorder (double xr[SBLIMIT][SSLIMIT],
             double ro[SBLIMIT][SSLIMIT],
             struct gr_info_s *gr_info,
             frame_params *fr_ps) 
{
   int sfreq;
   int sfb, sfb_start, sfb_lines;
   int sb, ss, window, freq, src_line, des_line;

   sfreq=fr_ps->header->sampling_frequency + (fr_ps->header->version * 3);

   for(sb=0;sb<SBLIMIT;sb++)
      for(ss=0;ss<SSLIMIT;ss++) 
         ro[sb][ss] = 0;

   if (gr_info->window_switching_flag && (gr_info->block_type == 2)) {
      if (gr_info->mixed_block_flag) {
         /* NO REORDER FOR LOW 2 SUBBANDS */
         for (sb=0 ; sb < 2 ; sb++)
            for (ss=0 ; ss < SSLIMIT ; ss++) {
               ro[sb][ss] = xr[sb][ss];
            }
         /* REORDERING FOR REST SWITCHED SHORT */
         for(sfb=3,sfb_start=sfBandIndex[sfreq].s[3],
            sfb_lines=sfBandIndex[sfreq].s[4] - sfb_start; 
            sfb < 13; sfb++,sfb_start=sfBandIndex[sfreq].s[sfb],
            (sfb_lines=sfBandIndex[sfreq].s[sfb+1] - sfb_start))
               for(window=0; window<3; window++)
                  for(freq=0;freq<sfb_lines;freq++) {
                     src_line = sfb_start*3 + window*sfb_lines + freq; 
                     des_line = (sfb_start*3) + window + (freq*3);
                     ro[des_line/SSLIMIT][des_line%SSLIMIT] = 
                                    xr[src_line/SSLIMIT][src_line%SSLIMIT];
               }
      } 
      else {  /* pure short */
         for(sfb=0,sfb_start=0,sfb_lines=sfBandIndex[sfreq].s[1]; 
            sfb < 13; sfb++,sfb_start=sfBandIndex[sfreq].s[sfb],
            (sfb_lines=sfBandIndex[sfreq].s[sfb+1] - sfb_start))
               for(window=0; window<3; window++)
                  for(freq=0;freq<sfb_lines;freq++) {
                     src_line = sfb_start*3 + window*sfb_lines + freq; 
                     des_line = (sfb_start*3) + window + (freq*3);
                     ro[des_line/SSLIMIT][des_line%SSLIMIT] = 
                                    xr[src_line/SSLIMIT][src_line%SSLIMIT];
               }
      }
   }
   else {   /*long blocks */
      for (sb=0 ; sb < SBLIMIT ; sb++)
         for (ss=0 ; ss < SSLIMIT ; ss++) 
            ro[sb][ss] = xr[sb][ss];
   }
}

static void III_i_stereo_k_values(int is_pos, double io,
                                  int i,double FAR k[2][576])
{
   if(is_pos == 0)
   { 
      k[0][i] = 1;
      k[1][i] = 1;
   }
   else if ((is_pos % 2) == 1)
   {
      k[0][i] = pow(io,(double)((is_pos + 1)/2));
      k[1][i] = 1;
   }
   else
   {
      k[0][i] = 1;
      k[1][i] = pow(io,(double)(is_pos/2));
   }
}


void III_stereo(double xr[2][SBLIMIT][SSLIMIT],
                double lr[2][SBLIMIT][SSLIMIT],
                III_scalefac_t *scalefac,
                struct gr_info_s *gr_info,
                frame_params *fr_ps)
{
   int sfreq;
   int stereo = fr_ps->stereo;
   int ms_stereo = (fr_ps->header->mode == MPG_MD_JOINT_STEREO) &&
                   (fr_ps->header->mode_ext & 0x2); 
   int i_stereo = (fr_ps->header->mode == MPG_MD_JOINT_STEREO) &&
                  (fr_ps->header->mode_ext & 0x1);
   int sfb;
   int i,j,sb,ss,ch;
   short is_pos[SBLIMIT*SSLIMIT]; 
   double is_ratio[SBLIMIT*SSLIMIT];
   double io;
   double FAR k[2][SBLIMIT*SSLIMIT];

   int lsf	= (fr_ps->header->version == MPEG_PHASE2_LSF);

    if(  (gr_info->scalefac_compress % 2) == 1)
     {
       io = (double)0.707106781188;
     }
     else
     {
       io = (double)0.840896415256;
     }


   sfreq=fr_ps->header->sampling_frequency + (fr_ps->header->version * 3);

  
   /* intialization */
   for ( i=0; i<SBLIMIT*SSLIMIT; i++ )
      is_pos[i] = 7;

   if ((stereo == 2) && i_stereo )
   {  if (gr_info->window_switching_flag && (gr_info->block_type == 2))
      {  if( gr_info->mixed_block_flag )
         {  int max_sfb = 0;

            for ( j=0; j<3; j++ )
            {  int sfbcnt;
               sfbcnt = 2;
               for( sfb=12; sfb >=3; sfb-- )
               {  int lines;
                  lines = sfBandIndex[sfreq].s[sfb+1]-sfBandIndex[sfreq].s[sfb];
                  i = 3*sfBandIndex[sfreq].s[sfb] + (j+1) * lines - 1;
                  while ( lines > 0 )
                  {  if ( xr[1][i/SSLIMIT][i%SSLIMIT] != 0.0 )
                     {  sfbcnt = sfb;
                        sfb = -10;
                        lines = -10;
                     }
                     lines--;
                     i--;
                  }
               }
               sfb = sfbcnt + 1;

               if ( sfb > max_sfb )
                  max_sfb = sfb;

               while( sfb<12 )
               {  sb = sfBandIndex[sfreq].s[sfb+1]-sfBandIndex[sfreq].s[sfb];
                  i = 3*sfBandIndex[sfreq].s[sfb] + j * sb;
                  for ( ; sb > 0; sb--)
                  {  is_pos[i] = (*scalefac)[1].s[j][sfb];
                     if ( is_pos[i] != 7 )
                         if( lsf )
                         {
                              III_i_stereo_k_values(is_pos[i],io,i,k);
                         }
                         else
                         {
                             is_ratio[i] = tan((double)is_pos[i] * (PI / 12));
                         }
                     i++;
                  }
                  sfb++;
               }

               sb = sfBandIndex[sfreq].s[12]-sfBandIndex[sfreq].s[11];
               sfb = 3*sfBandIndex[sfreq].s[11] + j * sb;
               sb = sfBandIndex[sfreq].s[13]-sfBandIndex[sfreq].s[12];

               i = 3*sfBandIndex[sfreq].s[11] + j * sb;
               for ( ; sb > 0; sb-- )
               {  is_pos[i] = is_pos[sfb];
                  is_ratio[i] = is_ratio[sfb];
                  k[0][i] = k[0][sfb];
                  k[1][i] = k[1][sfb];
                  i++;
               }
             }
             if ( max_sfb <= 3 )
             {  i = 2;
                ss = 17;
                sb = -1;
                while ( i >= 0 )
                {  if ( xr[1][i][ss] != 0.0 )
                   {  sb = i*18+ss;
                      i = -1;
                   } else
                   {  ss--;
                      if ( ss < 0 )
                      {  i--;
                         ss = 17;
                      }
                   }
                }
                i = 0;
                while ( sfBandIndex[sfreq].l[i] <= sb )
                   i++;
                sfb = i;
                i = sfBandIndex[sfreq].l[i];
                for ( ; sfb<8; sfb++ )
                {  sb = sfBandIndex[sfreq].l[sfb+1]-sfBandIndex[sfreq].l[sfb];
                   for ( ; sb > 0; sb--)
                   {  is_pos[i] = (*scalefac)[1].l[sfb];
                      if ( is_pos[i] != 7 )
                         if ( lsf )
                         {
                              III_i_stereo_k_values(is_pos[i],io,i,k);
                         }
                         else
                         {
                             is_ratio[i] = tan((double)is_pos[i] * (PI / 12));
                         }
                      i++;
                   }
                }
            }
         } else
         {  for ( j=0; j<3; j++ )
            {  int sfbcnt;
               sfbcnt = -1;
               for( sfb=12; sfb >=0; sfb-- )
               {  int lines;
                  lines = sfBandIndex[sfreq].s[sfb+1]-sfBandIndex[sfreq].s[sfb];
                  i = 3*sfBandIndex[sfreq].s[sfb] + (j+1) * lines - 1;
                  while ( lines > 0 )
                  {  if ( xr[1][i/SSLIMIT][i%SSLIMIT] != 0.0 )
                     {  sfbcnt = sfb;
                        sfb = -10;
                        lines = -10;
                     }
                     lines--;
                     i--;
                  }
               }
               sfb = sfbcnt + 1;
               while( sfb<12 )
               {  sb = sfBandIndex[sfreq].s[sfb+1]-sfBandIndex[sfreq].s[sfb];
                  i = 3*sfBandIndex[sfreq].s[sfb] + j * sb;
                  for ( ; sb > 0; sb--)
                  {  is_pos[i] = (*scalefac)[1].s[j][sfb];
                     if ( is_pos[i] != 7 )
                         if( lsf )
                         {
                              III_i_stereo_k_values(is_pos[i],io,i,k);
                         }
                         else
                         {
                             is_ratio[i] = tan( (double)is_pos[i] * (PI / 12));
                         }
                     i++;
                  }
                  sfb++;
               }

               sb = sfBandIndex[sfreq].s[12]-sfBandIndex[sfreq].s[11];
               sfb = 3*sfBandIndex[sfreq].s[11] + j * sb;
               sb = sfBandIndex[sfreq].s[13]-sfBandIndex[sfreq].s[12];

               i = 3*sfBandIndex[sfreq].s[11] + j * sb;
               for ( ; sb > 0; sb-- )
               {  is_pos[i] = is_pos[sfb];
                  is_ratio[i] = is_ratio[sfb];
                  k[0][i] = k[0][sfb];
                  k[1][i] = k[1][sfb];
                  i++;
               }
            }
         }
      } else
      {  i = 31;
         ss = 17;
         sb = 0;
         while ( i >= 0 )
         {  if ( xr[1][i][ss] != 0.0 )
            {  sb = i*18+ss;
               i = -1;
            } else
            {  ss--;
               if ( ss < 0 )
               {  i--;
                  ss = 17;
               }
            }
         }
         i = 0;
         while ( sfBandIndex[sfreq].l[i] <= sb )
            i++;
         sfb = i;
         i = sfBandIndex[sfreq].l[i];
         for ( ; sfb<21; sfb++ )
         {  sb = sfBandIndex[sfreq].l[sfb+1] - sfBandIndex[sfreq].l[sfb];
            for ( ; sb > 0; sb--)
            {  is_pos[i] = (*scalefac)[1].l[sfb];
               if ( is_pos[i] != 7 )
                     if( lsf )
                     {
                           III_i_stereo_k_values(is_pos[i],io,i,k);
                     }
                     else
                     {
                          is_ratio[i] = tan((double)is_pos[i] * (PI / 12));
                     }
               i++;
            }
         }
         sfb = sfBandIndex[sfreq].l[20];
         for ( sb = 576 - sfBandIndex[sfreq].l[21]; sb > 0; sb-- )
         {  is_pos[i] = is_pos[sfb];
            is_ratio[i] = is_ratio[sfb];
            k[0][i] = k[0][sfb];
            k[1][i] = k[1][sfb];
            i++;
         }
      }
   }

   for(ch=0;ch<2;ch++)
      for(sb=0;sb<SBLIMIT;sb++)
         for(ss=0;ss<SSLIMIT;ss++) 
            lr[ch][sb][ss] = 0;

   if (stereo==2) 
      for(sb=0;sb<SBLIMIT;sb++)
         for(ss=0;ss<SSLIMIT;ss++) {
            i = (sb*18)+ss;
            if ( is_pos[i] == 7 ) {
               if ( ms_stereo ) {
                  lr[0][sb][ss] = (xr[0][sb][ss]+xr[1][sb][ss])/(double)1.41421356;
                  lr[1][sb][ss] = (xr[0][sb][ss]-xr[1][sb][ss])/(double)1.41421356;
               }
               else {
                  lr[0][sb][ss] = xr[0][sb][ss];
                  lr[1][sb][ss] = xr[1][sb][ss];
               }
            }
            else if (i_stereo ) {
                if ( lsf )
                {
                  lr[0][sb][ss] = xr[0][sb][ss] * k[0][i];
                  lr[1][sb][ss] = xr[0][sb][ss] * k[1][i]; 
                }
               else
                {
                  lr[0][sb][ss] = xr[0][sb][ss] * (is_ratio[i]/(1+is_ratio[i]));
                  lr[1][sb][ss] = xr[0][sb][ss] * (1/(1+is_ratio[i])); 
                }
            }
            else {
               printf("Error in streo processing\n");
            }
         }
   else  /* mono , bypass xr[0][][] to lr[0][][]*/
      for(sb=0;sb<SBLIMIT;sb++)
         for(ss=0;ss<SSLIMIT;ss++)
            lr[0][sb][ss] = xr[0][sb][ss];

}

double Ci[8]={-0.6,-0.535,-0.33,-0.185,-0.095,-0.041,-0.0142,-0.0037};

void III_antialias(double xr[SBLIMIT][SSLIMIT],
                   double hybridIn[SBLIMIT][SSLIMIT],
                   struct gr_info_s *gr_info,
                   frame_params *fr_ps)
{
   static int    init = 1;
   static double ca[8],cs[8];
   double        bu,bd;  /* upper and lower butterfly inputs */
   int           ss,sb,sblim;

   if (init) {
      int i;
      double    sq;
      for (i=0;i<8;i++) {
         sq=sqrt(1.0+Ci[i]*Ci[i]);
         cs[i] = 1.0/sq;
         ca[i] = Ci[i]/sq;
      }
      init = 0;
   }
   
   /* clear all inputs */  
      
    for(sb=0;sb<SBLIMIT;sb++)
       for(ss=0;ss<SSLIMIT;ss++)
          hybridIn[sb][ss] = xr[sb][ss];

   if  (gr_info->window_switching_flag && (gr_info->block_type == 2) &&
       !gr_info->mixed_block_flag ) return;

   if ( gr_info->window_switching_flag && gr_info->mixed_block_flag &&
     (gr_info->block_type == 2))
      sblim = 1;
   else
      sblim = SBLIMIT-1;

   /* 31 alias-reduction operations between each pair of sub-bands */
   /* with 8 butterflies between each pair                         */

   for(sb=0;sb<sblim;sb++)   
      for(ss=0;ss<8;ss++) {      
         bu = xr[sb][17-ss];
         bd = xr[sb+1][ss];
         hybridIn[sb][17-ss] = (bu * cs[ss]) - (bd * ca[ss]);
         hybridIn[sb+1][ss] = (bd * cs[ss]) + (bu * ca[ss]);
         }  
}

void inv_mdct(double in[18], double out[36], int block_type)
{
/*------------------------------------------------------------------*/
/*                                                                  */
/*    Function: Calculation of the inverse MDCT                     */
/*    In the case of short blocks the 3 output vectors are already  */
/*    overlapped and added in this modul.                           */
/*                                                                  */
/*    New layer3                                                    */
/*                                                                  */
/*------------------------------------------------------------------*/

    int     i,m,N,p;
    double  tmp[12],sum;
    static  double  win[4][36];
    static  int init=0;
    static  double COS[4*36];

    if (init == 0) {

    /* type 0 */
      for(i=0;i<36;i++)
         win[0][i] = sin( PI/36 *(i+0.5) );

    /* type 1*/
      for(i=0;i<18;i++)
         win[1][i] = sin( PI/36 *(i+0.5) );
      for(i=18;i<24;i++)
         win[1][i] = 1.0;
      for(i=24;i<30;i++)
         win[1][i] = sin( PI/12 *(i+0.5-18) );
      for(i=30;i<36;i++)
         win[1][i] = 0.0;

    /* type 3*/
      for(i=0;i<6;i++)
         win[3][i] = 0.0;
      for(i=6;i<12;i++)
         win[3][i] = sin( PI/12 *(i+0.5-6) );
      for(i=12;i<18;i++)
         win[3][i] =1.0;
      for(i=18;i<36;i++)
         win[3][i] = sin( PI/36*(i+0.5) );

    /* type 2*/
      for(i=0;i<12;i++)
         win[2][i] = sin( PI/12*(i+0.5) ) ;
      for(i=12;i<36;i++)
         win[2][i] = 0.0 ;

      for (i=0; i<4*36; i++)
         COS[i] = cos(PI/(2*36) * i);

      init++;
    }

    for(i=0;i<36;i++)
       out[i]=0;

    if(block_type == 2){
       N=12;
       for(i=0;i<3;i++){
          for(p= 0;p<N;p++){
             sum = 0.0;
             for(m=0;m<N/2;m++)
                sum += in[i+3*m] * cos( PI/(2*N)*(2*p+1+N/2)*(2*m+1) );
             tmp[p] = sum * win[block_type][p] ;
          }
          for(p=0;p<N;p++)
             out[6*i+p+6] += tmp[p];
       }
    }
    else{
      N=36;
      for(p= 0;p<N;p++){
         sum = 0.0;
         for(m=0;m<N/2;m++)
           sum += in[m] * COS[((2*p+1+N/2)*(2*m+1))%(4*36)];
         out[p] = sum * win[block_type][p];
      }
    }
}

void III_hybrid(double fsIn[SSLIMIT], double tsOut[SSLIMIT],
                int sb, int ch, struct gr_info_s *gr_info,
                frame_params *fr_ps)
/* fsIn    freq samples per subband in */
/* tsOut   time samples per subband out */
{
   int ss;
   double rawout[36];
   static double prevblck[2][SBLIMIT][SSLIMIT];
   static int init = 1;
   int bt;

   if (init) {
      int i,j,k;
      
      for(i=0;i<2;i++)
         for(j=0;j<SBLIMIT;j++)
            for(k=0;k<SSLIMIT;k++)
               prevblck[i][j][k]=0.0;
      init = 0;
   }

   bt = (gr_info->window_switching_flag && gr_info->mixed_block_flag &&
          (sb < 2)) ? 0 : gr_info->block_type; 

   inv_mdct( fsIn, rawout, bt);

   /* overlap addition */
   for(ss=0; ss<SSLIMIT; ss++) {
      tsOut[ss] = rawout[ss] + prevblck[ch][sb][ss];
      prevblck[ch][sb][ss] = rawout[ss+18];
   }
}

/* Return the number of slots for main data of current frame, */

int main_data_slots(frame_params fr_ps)
{
    int nSlots;

   nSlots = (144 * bitrate[fr_ps.header->version][2][fr_ps.header->bitrate_index])
	    / s_freq[fr_ps.header->version][fr_ps.header->sampling_frequency];
  if ( fr_ps.header->version != MPEG_PHASE2_LSF ) {
       if (fr_ps.stereo == 1) nSlots -= 17; else nSlots -=32;
  }
  else
  {
      nSlots = nSlots / 2;
      if (fr_ps.stereo == 1) nSlots -= 9; else nSlots -=17;
  }

  if (fr_ps.header->padding) nSlots++;
      nSlots -= 4;
  if (fr_ps.header->error_protection) nSlots -= 2;

  return(nSlots);
}
