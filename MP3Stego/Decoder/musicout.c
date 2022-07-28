/**********************************************************************
 * ISO MPEG Audio Subgroup Software Simulation Group (1996)
 * ISO 13818-3 MPEG-2 Audio Decoder - Lower Sampling Frequency Extension
 *
 * $Header: /MP3Stego/MP3Stego Decoder/musicout.c 9     30/11/00 15:56 Fabienpe $
 *
 * $Id: musicout.c,v 1.2 1996/03/28 03:13:37 rowlands Exp $
 *
 * $Log: /MP3Stego/MP3Stego Decoder/musicout.c $
 * 
 * 9     30/11/00 15:56 Fabienpe
 * 
 * 8     11/02/99 11:29 Fapp2
 * pszPassword... should be declared somewhere
 * 
 * 7     9/02/99 22:26 Fapp2
 * 
 * 6     30/10/98 11:24 Fapp2
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
 *   date   programmers                comment                        *
 * 2/25/91  Douglas Wong        start of version 1.0 records          *
 * 3/06/91  Douglas Wong        rename setup.h to dedef.h             *
 *                              removed extraneous variables          *
 *                              removed window_samples (now part of   *
 *                              filter_samples)                       *
 * 3/07/91  Davis Pan           changed output file to "codmusic"     *
 * 5/10/91  Vish (PRISM)        Ported to Macintosh and Unix.         *
 *                              Incorporated new "out_fifo()" which   *
 *                              writes out last incomplete buffer.    *
 *                              Incorporated all AIFF routines which  *
 *                              are also compatible with SUN.         *
 *                              Incorporated user interface for       *
 *                              specifying sound file names.          *
 *                              Also incorporated user interface for  *
 *                              writing AIFF compatible sound files.  *
 * 27jun91  dpwe (Aware)        Added musicout and &sample_frames as  *
 *                              args to out_fifo (were glob refs).    *
 *                              Used new 'frame_params' struct.       *
 *                              Clean,simplify, track clipped output  *
 *                              and total bits/frame received.        *
 * 7/10/91  Earle Jennings      changed to floats to FLOAT            *
 *10/ 1/91  S.I. Sudharsanan,   Ported to IBM AIX platform.           *
 *          Don H. Lee,                                               *
 *          Peter W. Farrett                                          *
 *10/ 3/91  Don H. Lee          implemented CRC-16 error protection   *
 *                              newly introduced functions are        *
 *                              buffer_CRC and recover_CRC_error      *
 *                              Additions and revisions are marked    *
 *                              with "dhl" for clarity                *
 * 2/11/92  W. Joseph Carter    Ported new code to Macintosh.  Most   *
 *                              important fixes involved changing     *
 *                              16-bit ints to long or unsigned in    *
 *                              bit alloc routines for quant of 65535 *
 *                              and passing proper function args.     *
 *                              Removed "Other Joint Stereo" option   *
 *                              and made bitrate be total channel     *
 *                              bitrate, irrespective of the mode.    *
 *                              Fixed many small bugs & reorganized.  *
 *19 aug 92 Soren H. Nielsen    Changed MS-DOS file name extensions.  *
 * 8/27/93 Seymour Shlien,      Fixes in Unix and MSDOS ports,        *
 *         Daniel Lauzon, and                                         *
 *         Bill Truerniet                                             *
 *--------------------------------------------------------------------*
 * 4/23/92  J. Pineda           Added code for layer III.  LayerIII   *
 *          Amit Gulati         decoding is currently performed in    *
 *                              two-passes for ease of sideinfo and   *
 *                              maindata buffering and decoding.      *
 *                              The second (computation) pass is      *
 *                              activated with "decode -3 <outfile>"  *
 * 10/25/92 Amit Gulati         Modified usage() for layerIII         *
 * 12/10/92 Amit Gulati         Changed processing order of re-order- *
 *                              -ing step.  Fixed adjustment of       *
 *                              main_data_end pointer to exclude      *
 *                              side information.                     *
 *  9/07/93 Toshiyuki Ishino    Integrated Layer III with Ver 3.9.    *
 *--------------------------------------------------------------------*
 * 11/20/93 Masahiro Iwadare    Integrated Layer III with Ver 4.0.    *
 *--------------------------------------------------------------------*
 *  7/14/94 Juergen Koller      Bug fixes in Layer III code           *
 *--------------------------------------------------------------------*
 * 08/11/94 IIS                 Bug fixes in Layer III code           *
 *--------------------------------------------------------------------*
 * 11/04/94 Jon Rowlands        Prototype fixes                       *
 *--------------------------------------------------------------------*
 *  7/12/95 Soeren H. Nielsen   Changes for LSF Layer I and II        *
 *--------------------------------------------------------------------*
 *  7/14/94 Juergen Koller      Bug fixes in Layer III code           *
 *--------------------------------------------------------------------*
 *     8/95 Roland Bitto        addapdet to MPEG 2                    *
 *--------------------------------------------------------------------*
 * 11/22/95 Heiko Purnhagen     skip ancillary data in bitstream      *
 *--------------------------------------------------------------------*
 * Aug 1998 Fabien Petitcolas   added stego decoding                  *
 **********************************************************************/

#include "common.h"
#include "decoder.h"

#include "../../stegolib/stego.h"
char *pszPassword;

/********************************************************************
/*
/*        This part contains the MPEG I decoder for Layers I & II.
/*
/*********************************************************************/

/****************************************************************
/*
/*        For MS-DOS user (Turbo c) change all instance of malloc
/*        to _farmalloc and free to _farfree. Compiler model hugh
/*        Also make sure all the pointer specified are changed to far.
/*
/*****************************************************************/
/* local functions definition */

static void usage();
static void GetArguments();
static void print_header();

/*********************************************************************
/*
/* Core of the Layer II decoder.  Default layer is Layer II.
/*
/*********************************************************************/

/* Global variable definitions for "musicout.c" */

char *programName;
int main_data_slots();
int side_info_slots();
void print_header(void);

/* Implementations */

void main(int argc, char **argv)
{
    /*typedef short PCM[2][3][SBLIMIT];*/
    typedef short PCM[2][SSLIMIT][SBLIMIT];
    PCM FAR *pcm_sample;
    typedef unsigned int SAM[2][3][SBLIMIT];
    SAM FAR *sample;
    typedef double FRA[2][3][SBLIMIT];
    FRA FAR *fraction;
    typedef double VE[2][HAN_SIZE];
    VE FAR *w;

    Bit_stream_struc  bs;
    frame_params      fr_ps;
    layer             info;
    FILE              *musicout;
    unsigned long     sample_frames;

    int               i, j, k, stereo, done=FALSE, clip, sync; 
    int               error_protection, crc_error_count, total_error_count;
    unsigned int      old_crc, new_crc;
    unsigned int      bit_alloc[2][SBLIMIT], scfsi[2][SBLIMIT],
                      scale_index[2][3][SBLIMIT];
    unsigned long     bitsPerSlot, samplesPerFrame, frameNum = 0;
    unsigned long     frameBits, gotBits = 0;
    IFF_AIFF          pcm_aiff_data;
    Arguments_t       Arguments;
    int Max_gr;

    III_scalefac_t    III_scalefac;
    III_side_info_t   III_side_info;

/* MP3STEGO-> */
    pszPassword = NULL;
/* ->MP3STEGO */

#ifdef  MACINTOSH
    console_options.nrows = MAC_WINDOW_SIZE;
    argc = ccommand(&argv);
#endif

    print_header();

    /* Most large variables are declared dynamically to ensure
       compatibility with smaller machines */

    pcm_sample = (PCM FAR *) mem_alloc((long) sizeof(PCM), "PCM Samp");
    sample = (SAM FAR *) mem_alloc((long) sizeof(SAM), "Sample");
    fraction = (FRA FAR *) mem_alloc((long) sizeof(FRA), "fraction");
    w = (VE FAR *) mem_alloc((long) sizeof(VE), "w");

    fr_ps.header = &info;
    fr_ps.tab_num = -1;                /* no table loaded */
    fr_ps.alloc = NULL;
    for (i=0;i<HAN_SIZE;i++) for (j=0;j<2;j++) (*w)[j][i] = 0.0;

    Arguments.topSb = 0;
    GetArguments(argc, argv, &Arguments);

/* MP3STEGO-> */
    if (Arguments.extract_hidden)
		StegoCreateEmbeddedText(Arguments.hidden_file_name);
/* ->MP3STEGO */

    if ((musicout = fopen(Arguments.decoded_file_name, "w+b")) == NULL) {
          printf ("Could not create \"%s\".\n", Arguments.decoded_file_name);
          exit(1);
        }
    open_bit_stream_r(&bs, Arguments.encoded_file_name, BUFFER_SIZE);

    if (Arguments.need_aiff)
       if (aiff_seek_to_sound_data(musicout) == -1) {
          printf("Could not seek to PCM sound data in \"%s\".\n",
                 Arguments.decoded_file_name);
          exit(1);
       }

    sample_frames = 0;

    while (!end_bs(&bs)) {

       sync = seek_sync(&bs, SYNC_WORD, SYNC_WORD_LNGTH);
       frameBits = sstell(&bs) - gotBits;
       if (frameNum > 0)        /* don't want to print on 1st loop; no lay */
          if (frameBits%bitsPerSlot)
             fprintf(stderr,"Got %ld bits = %ld slots plus %ld\n",
                     frameBits, frameBits/bitsPerSlot, frameBits%bitsPerSlot);
       gotBits += frameBits;

       if (!sync) {
          printf("Frame cannot be located\n");
          printf("Input stream may be empty\n");
          done = TRUE;
          /* finally write out the buffer */
          if (info.lay != 1) out_fifo(*pcm_sample, 3, &fr_ps, done,
                                      musicout, &sample_frames);
          else               out_fifo(*pcm_sample, 1, &fr_ps, done,
                                      musicout, &sample_frames);
          break;
       }

       decode_info(&bs, &fr_ps);
       hdr_to_frps(&fr_ps);
       stereo = fr_ps.stereo;
       if (fr_ps.header->version == MPEG_PHASE2_LSF) {
		Max_gr = 1;
       }
       else
       { 
		Max_gr = 2;
       }

       error_protection = info.error_protection;
       crc_error_count = 0;
       total_error_count = 0;
       if (frameNum == 0) WriteHdr(&fr_ps, stdout);  /* printout layer/mode */

#ifdef ESPS
if (frameNum == 0 && Arguments.need_esps) {
	esps_write_header(musicout,(long) sample_frames, (double)
		s_freq[info.version][info.sampling_frequency] * 1000,
		(int) stereo, Arguments.decoded_file_name);
} /* MI */
#endif

#if defined(_DEBUG)
       fprintf(stderr, "\n[Frame %4lu]", frameNum++); fflush(stderr); 
#else
       fprintf(stderr, "\015[Frame %4lu]", frameNum++); fflush(stderr); 
#endif
       if (error_protection) buffer_CRC(&bs, &old_crc);

       switch (info.lay) {

          case 1: {
             bitsPerSlot = 32;        samplesPerFrame = 384;
             I_decode_bitalloc(&bs,bit_alloc,&fr_ps);
             I_decode_scale(&bs, bit_alloc, scale_index, &fr_ps);

             if (error_protection) {
                I_CRC_calc(&fr_ps, bit_alloc, &new_crc);
                if (new_crc != old_crc) {
                   crc_error_count++;
                   total_error_count++;
                   recover_CRC_error(*pcm_sample, crc_error_count,
                                     &fr_ps, musicout, &sample_frames);
                   break;
                }
                else crc_error_count = 0;
             }

             clip = 0;
             for (i=0;i<SCALE_BLOCK;i++) {
                I_buffer_sample(&bs,(*sample),bit_alloc,&fr_ps);
                I_dequantize_sample(*sample,*fraction,bit_alloc,&fr_ps);
                I_denormalize_sample((*fraction),scale_index,&fr_ps);

                if (Arguments.topSb>0)        /* clear channels to 0 */
                   for(j=Arguments.topSb; j<fr_ps.sblimit; ++j)
                      for(k=0; k<stereo; ++k)
                         (*fraction)[k][0][j] = 0;

                for (j=0;j<stereo;j++) {
                   clip += SubBandSynthesis (&((*fraction)[j][0][0]), j,
                                             &((*pcm_sample)[j][0][0]));
                }
                out_fifo(*pcm_sample, 1, &fr_ps, done,
                         musicout, &sample_frames);
             }
#if defined(_DEBUG)
             if (clip > 0) printf(" %d output samples clipped", clip);
#endif
             break;
          }

          case 2: {
             bitsPerSlot = 8;        samplesPerFrame = 1152;
             II_decode_bitalloc(&bs, bit_alloc, &fr_ps);
             II_decode_scale(&bs, scfsi, bit_alloc, scale_index, &fr_ps);

             if (error_protection) { 
                II_CRC_calc(&fr_ps, bit_alloc, scfsi, &new_crc);
                if (new_crc != old_crc) {
                   crc_error_count++;
                   total_error_count++;
                   recover_CRC_error(*pcm_sample, crc_error_count,
                                     &fr_ps, musicout, &sample_frames);
                   break;
                }
                else crc_error_count = 0;
             }

             clip = 0;
             for (i=0;i<SCALE_BLOCK;i++) {
                II_buffer_sample(&bs,(*sample),bit_alloc,&fr_ps);
                II_dequantize_sample((*sample),bit_alloc,(*fraction),&fr_ps);
                II_denormalize_sample((*fraction),scale_index,&fr_ps,i>>2);

                if (Arguments.topSb>0)        /* debug : clear channels to 0 */
                   for(j=Arguments.topSb; j<fr_ps.sblimit; ++j)
                      for(k=0; k<stereo; ++k)
                         (*fraction)[k][0][j] =
                         (*fraction)[k][1][j] =
                         (*fraction)[k][2][j] = 0;

                for (j=0;j<3;j++) for (k=0;k<stereo;k++) {
                   clip += SubBandSynthesis (&((*fraction)[k][j][0]), k,
                                             &((*pcm_sample)[k][j][0]));
                }
                out_fifo(*pcm_sample, 3, &fr_ps, done, musicout,
                         &sample_frames);
             }
#if defined(_DEBUG)
             if (clip > 0) printf(" %d samples clipped", clip);
#endif
             break;
          }

          case 3: {
             int nSlots;
             int gr, ch, ss, sb, main_data_end, flush_main ;
	     int  bytes_to_discard ;
	     static int frame_start = 0;

             bitsPerSlot = 8;        
             if (fr_ps.header->version == MPEG_PHASE2_LSF)
		samplesPerFrame = 576;
	     else
		samplesPerFrame = 1152;
 
             III_get_side_info(&bs, &III_side_info, &fr_ps);
             nSlots = main_data_slots(fr_ps);
             for (; nSlots > 0; nSlots--)  /* read main data. */
                hputbuf((unsigned int) getbits(&bs,8), 8);
	     main_data_end = hsstell() / 8; /*of privious frame*/
             if (flush_main=(hsstell() % bitsPerSlot)) { 
                hgetbits((int)(bitsPerSlot - flush_main));
		main_data_end ++;
	     }
             bytes_to_discard = frame_start - main_data_end
 			            - III_side_info.main_data_begin ;
             if (main_data_end > 4096)
             {   frame_start -= 4096;
                 rewindNbytes(4096);
             }

             frame_start += main_data_slots(fr_ps);
             if (bytes_to_discard < 0) {
         printf("Not enough main data to decode frame %d.  Frame discarded.\n", 
                        frameNum - 1); break;
             }
             for (; bytes_to_discard > 0; bytes_to_discard--) hgetbits(8);

             clip = 0;
             for (gr=0;gr<Max_gr;gr++) {
               double lr[2][SBLIMIT][SSLIMIT],ro[2][SBLIMIT][SSLIMIT];

               for (ch=0; ch<stereo; ch++) {
                 long int is[SBLIMIT][SSLIMIT];   /* Quantized samples. */
                 int part2_start;
                 part2_start = hsstell();
                 if (fr_ps.header->version != MPEG_PHASE2_LSF)
                 {
                    III_get_scale_factors(&III_scalefac,&III_side_info,gr,ch,
			   &fr_ps);
                 }
                 else
                 {
                    III_get_LSF_scale_factors(&III_scalefac,
                                                    &III_side_info,
                                                    gr,ch,&fr_ps);
                 }
                 III_hufman_decode(is, &III_side_info, ch, gr, part2_start,
                                   &fr_ps);
                 III_dequantize_sample(is, ro[ch], &III_scalefac,
                                   &(III_side_info.ch[ch].gr[gr]), ch, &fr_ps);
               }
               III_stereo(ro,lr,&III_scalefac,
                            &(III_side_info.ch[0].gr[gr]), &fr_ps);

               for (ch=0; ch<stereo; ch++) {
                    double re[SBLIMIT][SSLIMIT];
                    double hybridIn[SBLIMIT][SSLIMIT];/* Hybrid filter input */
                    double hybridOut[SBLIMIT][SSLIMIT];/* Hybrid filter out */
                    double polyPhaseIn[SBLIMIT];     /* PolyPhase Input. */

                    III_reorder (lr[ch],re,&(III_side_info.ch[ch].gr[gr]),
                                  &fr_ps);

                    III_antialias(re, hybridIn, /* Antialias butterflies. */
                                  &(III_side_info.ch[ch].gr[gr]), &fr_ps);

                    for (sb=0; sb<SBLIMIT; sb++) { /* Hybrid synthesis. */
                        III_hybrid(hybridIn[sb], hybridOut[sb], sb, ch,
                                   &(III_side_info.ch[ch].gr[gr]), &fr_ps);
                    }

                    for (ss=0;ss<18;ss++) /*Frequency inversion for polyphase.*/
                       for (sb=0; sb<SBLIMIT; sb++)
                          if ((ss%2) && (sb%2))
                             hybridOut[sb][ss] = -hybridOut[sb][ss];

                    for (ss=0;ss<18;ss++) { /* Polyphase synthesis */
                        for (sb=0; sb<SBLIMIT; sb++)
                            polyPhaseIn[sb] = hybridOut[sb][ss];
                        clip += SubBandSynthesis (polyPhaseIn, ch,
                                                  &((*pcm_sample)[ch][ss][0]));
                        }
                    }
                /* Output PCM sample points for one granule. */
                out_fifo(*pcm_sample, 18, &fr_ps, done, musicout,
                         &sample_frames);
             }
#if defined(_DEBUG)
             if (clip > 0) printf(" %d samples clipped.", clip);
#endif
             break;
          }
       }

/* skip ancillary data   HP 22-nov-95 */
       if (info.bitrate_index > 0) { /* if not free-format */
		long anc_len;

		anc_len = (int)((double)samplesPerFrame /
			       s_freq[info.version][info.sampling_frequency] *
			       (double)bitrate[info.version][info.lay-1][info.bitrate_index] /
			       (double)bitsPerSlot);
		if (info.padding)
			anc_len++;
		anc_len *= bitsPerSlot;
		anc_len -= sstell(&bs)-gotBits+SYNC_WORD_LNGTH;
		for (j=0; j<anc_len; j++)
			get1bit(&bs);
	}
    }

    if (Arguments.need_aiff) {
       pcm_aiff_data.numChannels       = stereo;
       pcm_aiff_data.numSampleFrames   = sample_frames;
       pcm_aiff_data.sampleSize        = 16;
       pcm_aiff_data.sampleRate        = s_freq[info.version][info.sampling_frequency]*1000;
       pcm_aiff_data.sampleType        = IFF_ID_SSND;
       pcm_aiff_data.blkAlgn.offset    = 0;
       pcm_aiff_data.blkAlgn.blockSize = 0;

       if (aiff_write_headers(musicout, &pcm_aiff_data) == -1) {
          printf("Could not write AIFF headers to \"%s\"\n",
                 Arguments.decoded_file_name);
          exit(2);
       }
    }

    printf("Avg slots/frame = %.3f; b/smp = %.2f; br = %.3f kbps\n",
           (FLOAT) gotBits / (frameNum * bitsPerSlot),
           (FLOAT) gotBits / (frameNum * samplesPerFrame),
           (FLOAT) gotBits / (frameNum * samplesPerFrame) *
           s_freq[info.version][info.sampling_frequency]);

    close_bit_stream_r(&bs);
    fclose(musicout);
/* MP3STEGO-> */
	StegoFlushEmbeddedText(Arguments.hidden_file_name);
/* ->MP3STEGO */

    /* for the correct AIFF header information */
    /*             on the Macintosh            */
    /* the file type and the file creator for  */
    /* Macintosh compatible Digidesign is set  */

#ifdef  MACINTOSH
    if (Arguments.need_aiff)
		set_mac_file_attr(Arguments.decoded_file_name, VOL_REF_NUM,
                                     CREATR_DEC_AIFF, FILTYP_DEC_AIFF);
    else	set_mac_file_attr(Arguments.decoded_file_name, VOL_REF_NUM,
                                     CREATR_DEC_BNRY, FILTYP_DEC_BNRY);
#endif

    printf("Decoding of \"%s\" is finished\n", Arguments.encoded_file_name);
    printf("The decoded PCM output file name is \"%s\"\n", Arguments.decoded_file_name);
    if (Arguments.need_aiff)
       printf("\"%s\" has been written with AIFF header information\n",
              Arguments.decoded_file_name);

    exit(0);
}

static void print_header()
{
    fprintf(stderr,"MP3StegoEncoder %s\n", STEGO_VERSION);
    fprintf(stderr,"See README file for copyright info\n");
}  

static void usage()  /* print syntax & exit */
{
   fprintf(stderr,"USAGE   : %s [-X][-A][-s sb] inputBS [outPCM [outhidden]]\n", programName);
/* STEGO */
   fprintf(stderr,"OPTIONS : -X         extract hidden data\n");
   fprintf(stderr,"          -P <text>  passphrase used for embedding\n");
/* STEGO */
   fprintf(stderr,"          -A         write an AIFF output PCM sound file\n");
   fprintf(stderr,"          -s <sb>    resynth only up to this sb (debugging only)\n");
   fprintf(stderr,"          inputBS    input bit stream of encoded audio\n");
   fprintf(stderr,"          outPCM     output PCM sound file (dflt inputBS+%s)\n",DFLT_OPEXT);
   fprintf(stderr,"          outhidden  output hidden text file (dflt inputBS+%s)\n",DFLT_STEGEXT);
   exit(1);
}

static void GetArguments(int argc, char **argv, Arguments_t *Arguments) 
{
   int i=0, err=0;
   
   programName = argv[0];
   
   if (argc == 1) usage();

   Arguments->need_aiff = FALSE;
   Arguments->need_esps = FALSE;	/* MI */
   /* STEGO */
   Arguments->extract_hidden = FALSE;
   Arguments->hidden_file_name[0] = '\0';
   /* STEGO */
   Arguments->encoded_file_name[0] = '\0';
   Arguments->decoded_file_name[0] = '\0';


   while ((++i < argc) && (err == 0))
   {
       char c, *token, *arg, *nextArg;
       int  argUsed;
       
       token = argv[i];
       if (*token++ == '-')
       {
           if (i+1 < argc)
               nextArg = argv[i+1];
           else
               nextArg = "";
           argUsed = 0;
           while(c = *token++)
           {
               if (*token /* NumericQ(token) */)
                   arg = token;
               else
                   arg = nextArg;
               
               switch (c)
               {
               case 's': Arguments->topSb = atoi(arg); argUsed = 1;
                   if (Arguments->topSb<1 || Arguments->topSb>SBLIMIT)
                   {
                       fprintf(stderr, "%s: -s band %s not %d..%d\n",
                           programName, arg, 1, SBLIMIT);
                       err = 1;
                   }
                   break;
               case 'A':
                   Arguments->need_aiff = TRUE;
                   break;
               case 'E':
                   Arguments->need_esps = TRUE;
                   break;	/* MI */
/* MP3STEGO-> */
               case 'X':
                   Arguments->extract_hidden = TRUE;
                   break;
               case 'P':
                   pszPassword = argv[++i];
                   break;
/* ->MP3STEGO */
               default:
                   fprintf(stderr,"%s: unrecognized option %c\n",
                       programName, c);
                   err = 1;
                   break;
               } /* switch (c)*/
               
               if (argUsed)
               {
                   if (arg == token)
                       token = ""; /* no more from token */
                   else
                       ++i; /* skip arg we used */
                   arg = "";
                   argUsed = 0;
               }
           } /* while(c = *token++) */
       }
       else
       {
           if (Arguments->encoded_file_name[0] == '\0')
               strcpy(Arguments->encoded_file_name, argv[i]);
           else
           {
               if (Arguments->decoded_file_name[0] == '\0')
                   strcpy(Arguments->decoded_file_name, argv[i]);
               else
               {
                   /* STEGO */
                   if (Arguments->hidden_file_name[0] == '\0')
                       strcpy(Arguments->hidden_file_name, argv[i]);
                   else
                   {
                       fprintf(stderr, "%s: excess arg %s\n", 
                           programName, argv[i]);
                       err = 1;
                   }
               }
           }
       }
   } /* while((++i < argc) && (err == 0)) */
       
   if (err || Arguments->encoded_file_name[0] == '\0') usage();  /* never returns */
   
   if (Arguments->decoded_file_name[0] == '\0')
   {
       strcpy(Arguments->decoded_file_name, Arguments->encoded_file_name);
       if (Arguments->need_aiff)
           strcat(Arguments->decoded_file_name, DFLT_AIFEXT);
       else
           strcat(Arguments->decoded_file_name, DFLT_PCMEXT);

   }

   if ((Arguments->extract_hidden) && (Arguments->hidden_file_name[0] == '\0'))
   {
       strcpy(Arguments->hidden_file_name, Arguments->encoded_file_name);
       strcat(Arguments->hidden_file_name, DFLT_STEGEXT);
   }
   
  
   /* report results of dialog / command line */
   printf("Input file = '%s'  output file = '%s'\n",
       Arguments->encoded_file_name, Arguments->decoded_file_name);
   if (Arguments->need_aiff) printf("Output file written in AIFF format\n");
   if (Arguments->need_esps) printf("Output file written in ESPS format\n"); /* MI */
   if (Arguments->extract_hidden) printf("Will attempt to extract hidden information. Output: %s\n",
       Arguments->hidden_file_name);
}

