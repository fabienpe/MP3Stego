/* STEGO                                                        */
/* Some lines added or modified for the information hidding     */
/*                                                              */
/* Fabien A.P. Petitcolas, August 1998                          */
/* $Header: /MP3Stego/MP3Stego Encoder/layer3.c 6     30/11/00 15:57 Fabienpe $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../stegolib/stego.h"

#include "types.h"
#include "wave.h"
#include "error.h"

#include "layer3.h"
#include "l3psy.h"
#include "l3subband.h"
#include "l3mdct.h"
#include "l3loop.h"
#include "l3bitstream.h"
#include "bitstream.h"

static void update_status(int frames_processed)
{
/* MP3STEGO-> */
#if defined(_DEBUG)
	fprintf(fLog, "[Frame %6d of %6ld] (%2.2f%%)\n", 
            frames_processed,config.mpeg.total_frames,
            (float)((float)frames_processed/config.mpeg.total_frames)*100); 
    printf("\n[Frame %6d of %6ld] (%2.2f%%)", 
            frames_processed,config.mpeg.total_frames,
            (float)((float)frames_processed/config.mpeg.total_frames)*100); 
#else
/* ->MP3STEGO */
    printf("\015[Frame %6d of %6ld] (%2.2f%%)", 
            frames_processed,config.mpeg.total_frames,
            (float)((float)frames_processed/config.mpeg.total_frames)*100); 
/* MP3STEGO-> */
#endif
/* ->MP3STEGO */
    fflush(stdout);
}


void L3_compress()
{
	int             frames_processed;
    static short    buffer[2][1152];
    int             channel;

    int             i;
    int             gr;
    short           sam[2][1344];
    float           snr32[32];
    L3_psy_ratio_t  ratio;
    double          pe[2][2];
    L3_side_info_t  side_info;
    short          *buffer_window[2];
    double          win_que[2][HAN_SIZE];
    double          l3_sb_sample[2][3][18][SBLIMIT];
    double          mdct_freq[2][2][576];
    int             l3_enc[2][2][576];
    L3_scalefac_t   scalefactor;
    bitstream_t     bs;
 
    double          avg_slots_per_frame;
    double          frac_slots_per_frame;
    long            whole_slots_per_frame;
    double          slot_lag;
    
    int             mean_bits;
    unsigned long   sent_bits  = 0;
    unsigned long   frame_bits = 0;
    int             sideinfo_len;

    open_bit_stream_w(&bs, config.outfile, BUFFER_SIZE);
    
    memset((char*)snr32,0,sizeof(snr32));
    memset((char *)sam,0,sizeof(sam));
    memset((char *)&side_info,0,sizeof(L3_side_info_t));

    L3_psycho_initialise();
    L3_subband_initialise();
    L3_mdct_initialise();
    L3_loop_initialise();

    config.mpeg.mode_gr           = (config.mpeg.type==TYPE_MPEG_I)?   2:  1;
    config.mpeg.samples_per_frame = (config.mpeg.type==TYPE_MPEG_I)?1152:576;
    config.mpeg.total_frames      = config.wave.total_samples/config.mpeg.samples_per_frame;
    config.mpeg.bits_per_slot     = 8;
    frames_processed              = 0;

/* MP3STEGO-> */
    if (config.pszDataFile)
    {
      printf("Hiding \"%s\"\n", config.pszDataFile);
      StegoOpenEmbeddedText(config.pszDataFile, config.mpeg.total_frames * 
        config.mpeg.mode_gr * config.wave.channels);
    }
/* ->MP3STEGO */

    sideinfo_len = 32;
    if(config.mpeg.type==TYPE_MPEG_I)
    {   /* MPEG 1 */
        if(config.wave.channels==1) sideinfo_len += 136;
        else                        sideinfo_len += 256;
    }
    else
    {   /* MPEG 2 */
        if(config.wave.channels==1) sideinfo_len += 72;
        else                        sideinfo_len += 136;
    }
    if(config.mpeg.crc) sideinfo_len += 16;

/* Figure average number of 'slots' per frame. */
    avg_slots_per_frame   = ((double)config.mpeg.samples_per_frame / 
                             ((double)config.wave.samplerate/1000)) *
                            ((double)config.mpeg.bitr /
                             (double)config.mpeg.bits_per_slot);
    whole_slots_per_frame = (int)avg_slots_per_frame;
    frac_slots_per_frame  = avg_slots_per_frame - (double)whole_slots_per_frame;
    slot_lag              = -frac_slots_per_frame;
    if(frac_slots_per_frame==0) config.mpeg.padding = 0;

    while(wave_get(buffer))
    {
        update_status(frames_processed++);

        buffer_window[0] = buffer[0];
        buffer_window[1] = buffer[1];

        if(frac_slots_per_frame!=0)
            if(slot_lag>(frac_slots_per_frame-1.0))
            { /* No padding for this frame */
                slot_lag    -= frac_slots_per_frame;
                config.mpeg.padding = 0;
            }
            else 
            { /* Padding for this frame  */
                slot_lag    += (1-frac_slots_per_frame);
                config.mpeg.padding = 1;
            }
       config.mpeg.bits_per_frame = 8*(whole_slots_per_frame + config.mpeg.padding);
       mean_bits = (config.mpeg.bits_per_frame - sideinfo_len) / config.mpeg.mode_gr;

/* psychoacousic model */
        for(gr=0;gr<config.mpeg.mode_gr;gr++)
            for(channel=0;channel<config.wave.channels;channel++)
                L3_psycho_analize(channel,
                                  &buffer[channel][gr*576],
                                  &sam[channel][0],&snr32[0],
                                  &ratio.l[gr][channel][0],
                                  &ratio.s[gr][channel][0],
                                  &pe[gr][channel],
                                  &side_info.gr[gr].ch[channel].tt);

/* polyphase filtering */
        for(gr=0;gr<config.mpeg.mode_gr;gr++)
            for(channel=0;channel<config.wave.channels;channel++)
                for(i=0;i<18;i++)
                {
                    L3_window_subband(&buffer_window[channel],
                                      &win_que[channel][0],
                                      channel);
                    L3_filter_subband(&win_que[channel][0],
                                      &l3_sb_sample[channel][gr+1][i][0]);
                }

/* apply mdct to the polyphase output */
        L3_mdct_sub(l3_sb_sample, mdct_freq, &side_info);

/* bit and noise allocation */
        L3_iteration_loop(pe,mdct_freq,&ratio,&side_info,
                          l3_enc, mean_bits,&scalefactor);


/* write the frame to the bitstream */

       L3_format_bitstream(l3_enc,&side_info,&scalefactor, 
                           &bs,mdct_freq,NULL,0);

       frame_bits = sstell(&bs) - sent_bits;

       if(frame_bits%config.mpeg.bits_per_slot) ERROR("This shouldn't happen, program error.");

       sent_bits += frame_bits;
    }    

/* MP3STEGO-> */
    if (config.pszDataFile)
		StegoCloseEmbeddedText();
/* ->MP3STEGO */

    L3_FlushBitstream();
    close_bit_stream_w(&bs);
}


