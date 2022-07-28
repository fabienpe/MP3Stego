/**********************************************************************
 * ISO MPEG Audio Subgroup Software Simulation Group (1996)
 * ISO 13818-3 MPEG-2 Audio Decoder - Lower Sampling Frequency Extension
 *
 * $Header: /MP3Stego Decoder/decoder.h 5     15/08/98 10:40 Fapp2 $
 *
 * $Id: decoder.h,v 1.2 1996/03/28 03:13:37 rowlands Exp $
 *
 * $Log: /MP3Stego Decoder/decoder.h $
 * 
 * 5     15/08/98 10:40 Fapp2
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
 * 2/25/91  Doulas Wong,        start of version 1.0 records          *
 *          Davis Pan                                                 *
 * 5/10/91  Vish (PRISM)        Renamed and regrouped all ".h" files  *
 *                              into "common.h" and "decoder.h".      *
 *                              Ported to Macintosh and Unix.         *
 * 27jun91  dpwe (Aware)        New prototype for out_fifo()          *
 *                              Moved "alloc_*" stuff to common.h     *
 *                              Use ifdef PROTO_ARGS for prototypes   *
 *                              prototypes reflect frame_params struct*
 * 10/3/91  Don H. Lee          implemented CRC-16 error protection   *
 * 2/11/92  W. Joseph Carter    Ported new code to Macintosh.  Most   *
 *                              important fixes involved changing     *
 *                              16-bit ints to long or unsigned in    *
 *                              bit alloc routines for quant of 65535 *
 *                              and passing proper function args.     *
 *                              Removed "Other Joint Stereo" option   *
 *                              and made bitrate be total channel     *
 *                              bitrate, irrespective of the mode.    *
 *                              Fixed many small bugs & reorganized.  *
 *                              Modified some function prototypes.    *
 * 08/07/92 Mike Coleman        Made small changes for portability    *
 * 9/07/93  Toshiyuki Ishino    Integrated with Layer III.            *
 * 11/04/94 Jon Rowlands        fix protos for usage() and            *
 *                              recover_CRC_error()                   *
 *          Roland Bitto 	Adapted to MPEG2 low bitrate	      *
 **********************************************************************/

/***********************************************************************
*
*  Decoder Include Files
*
***********************************************************************/

/***********************************************************************
*
*  Decoder Definitions
*
***********************************************************************/

#define   DFLT_OPEXT        ".aif|.pcm"  /* default output file name extension */
#define   DFLT_AIFEXT       ".aif"
#define   DFLT_PCMEXT       ".pcm"
/* STEGO */
#define   DFLT_STEGEXT      ".txt"  /* default extracted data file extensino */
/* STEGO */
/*
 NOTE: The value of a multiple-character constant is
 implementation-defined.
*/
#if !defined(MS_DOS) && !defined(AIX)
#define   FILTYP_DEC_AIFF   'AIFF'
#define   FILTYP_DEC_BNRY   'TEXT'
#define   CREATR_DEC_AIFF   'Sd2a'
/*
  The following character constant is ASCII '????'
  It is declared in hex because the character
  constant contains a trigraph, causing an error in
  parsing with ANSI preprocessors.
*/
#define   CREATR_DEC_BNRY   0x3f3f3f3f
#else
#define   FILTYP_DEC_AIFF   "AIFF"
#define   FILTYP_DEC_BNRY   "TEXT"
#define   CREATR_DEC_AIFF   "Sd2a"
#define   CREATR_DEC_BNRY   "????"
#endif

#define   SYNC_WORD         (long) 0xfff
#define   SYNC_WORD_LNGTH   12

#define   MUTE              0

/***********************************************************************
*
*  Decoder Type Definitions
*
***********************************************************************/

/***********************************************************************
*
*  Decoder Variable External Declarations
*
***********************************************************************/

/***********************************************************************
*
*  Decoder Function Prototype Declarations
*
***********************************************************************/

/* The following functions are in the file "musicout.c" */

#ifdef   PROTO_ARGS
static void   usage(void);
#else
static void   usage();
#endif

/* The following functions are in the file "decode.c" */

#ifdef   PROTO_ARGS
extern void   decode_info(Bit_stream_struc*, frame_params*);
extern void   II_decode_bitalloc(Bit_stream_struc*, unsigned int[2][SBLIMIT],
                       frame_params*);
extern void   I_decode_bitalloc(Bit_stream_struc*, unsigned int[2][SBLIMIT],
                       frame_params*);
extern void   I_decode_scale(Bit_stream_struc*, unsigned int[2][SBLIMIT],
                       unsigned int[2][3][SBLIMIT], frame_params*);
extern void   II_decode_scale(Bit_stream_struc*, unsigned int[2][SBLIMIT],
                       unsigned int[2][SBLIMIT], unsigned int[2][3][SBLIMIT],
                       frame_params*);
extern void   I_buffer_sample(Bit_stream_struc*, unsigned int[2][3][SBLIMIT],
                       unsigned int[2][SBLIMIT], frame_params*);
extern void   II_buffer_sample(Bit_stream_struc*, unsigned int[2][3][SBLIMIT],
                       unsigned int[2][SBLIMIT], frame_params*);
extern void   read_quantizer_table(double[17], double[17]);
extern void   II_dequantize_sample(unsigned int[2][3][SBLIMIT], 
                       unsigned int[2][SBLIMIT], double[2][3][SBLIMIT],
                       frame_params*);
extern void   I_dequantize_sample(unsigned int[2][3][SBLIMIT],
                       double[2][3][SBLIMIT], unsigned int[2][SBLIMIT],
                       frame_params*);
extern void   read_scale_factor(double[SCALE_RANGE]);
extern void   II_denormalize_sample(double[2][3][SBLIMIT],
                       unsigned int[2][3][SBLIMIT], frame_params*, int);
extern void   I_denormalize_sample(double[2][3][SBLIMIT],
                       unsigned int[2][3][SBLIMIT], frame_params*);
extern void   create_syn_filter(double[64][SBLIMIT]);
extern int    SubBandSynthesis (double*, int, short*);
extern void   read_syn_window(double[HAN_SIZE]);
extern void   window_sample(double*, double*);
extern void   out_fifo(short[2][SSLIMIT][SBLIMIT], int, frame_params*, int,
                       FILE*, unsigned long*);
extern void   buffer_CRC(Bit_stream_struc*, unsigned int*);
extern void   recover_CRC_error(short[2][SSLIMIT][SBLIMIT], int, frame_params*,
                       FILE*, unsigned long*);
extern void   III_dequantize_sample(long int[SBLIMIT][SSLIMIT],
			double [SBLIMIT][SSLIMIT], III_scalefac_t *,
                        struct gr_info_s *, int, frame_params *);
extern void   III_antialias(double[SBLIMIT][SSLIMIT], double[SBLIMIT][SSLIMIT], 
                          struct gr_info_s *, frame_params *);
extern void   inv_mdct(double[18], double[36], int);
extern void   III_hybrid(double[SSLIMIT], double[SSLIMIT] , int, int,
                       struct gr_info_s *, frame_params *);
extern void   III_get_side_info(Bit_stream_struc *,
                                 III_side_info_t *,
                                 frame_params *);

extern void   III_put_side_info(Bit_stream_struc *,
                                 III_side_info_t *,
                                 frame_params *);


extern void III_get_scale_factors(III_scalefac_t *,
                                 III_side_info_t *, 
                                 int, 
                                 int, 
                                 frame_params *);
extern void III_get_LSF_scale_data(III_scalefac_t *scalefac,
                            III_side_info_t *si,
                            int gr, int ch,
                            frame_params *fr_ps);
extern void III_get_LSF_scale_factors(III_scalefac_t *scalefac,
                               III_side_info_t *si,
                               int gr, int ch,
                               frame_params *fr_ps);
extern void III_hufman_decode(long int is[SBLIMIT][SSLIMIT],
                  III_side_info_t *si,
                  int ch, int gr, int part2_start,
                  frame_params *fr_ps);
extern void III_reorder (double xr[SBLIMIT][SSLIMIT],
             double ro[SBLIMIT][SSLIMIT],
             struct gr_info_s *gr_info,
             frame_params *fr_ps) ;
extern void III_stereo(double xr[2][SBLIMIT][SSLIMIT],
                double lr[2][SBLIMIT][SSLIMIT],
                III_scalefac_t *scalefac,
                struct gr_info_s *gr_info,
                frame_params *fr_ps);

#else
extern void   decode_info();
extern void   II_decode_bitalloc();
extern void   I_decode_bitalloc();
extern void   I_decode_scale();
extern void   II_decode_scale();
extern void   I_buffer_sample();
extern void   II_buffer_sample();
extern void   read_quantizer_table();
extern void   II_dequantize_sample();
extern void   I_dequantize_sample();
extern void   read_scale_factor();
extern void   II_denormalize_sample();
extern void   I_denormalize_sample();
extern void   create_syn_filter();
extern int    SubBandSynthesis ();
extern void   read_syn_window();
extern void   window_sample();
extern void   out_fifo();
extern void   buffer_CRC();
extern void   recover_CRC_error();
extern void   III_dequantize_sample();
extern void   III_antialias();
extern void   inv_mdct();
extern void   III_hybrid();
#endif
