/* $Header: /MP3Stego Encoder/wave.c 4     15/08/98 10:41 Fapp2 $ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "portableio.h"

#include "types.h"
#include "error.h"
#include "wave.h"

static bool checkString(FILE *file,char *string)
{
    char temp[1024];
    size_t length = strlen(string);
    if(fread(temp,1,length,file)!=length) ERROR("Premature EOF in input !?!");
    temp[length] = (char)0;
    return !strcmp(temp,string);
}

void wave_close()
{
    fclose(config.wave.file);
}

bool wave_open()
{
    static char    *channel_mappings[] = {NULL,"mono","stereo"};
    unsigned short  wFormatTag;
    unsigned long   dAvgBytesPerSec;
    unsigned short  wBlockAlign;
    long            filesize;
    long            header_size;

    if((config.wave.file = fopen(config.infile,"rb")) == NULL) ERROR("Unable to open file");

    if(!checkString(config.wave.file,"RIFF")) ERROR("Input not a MS-RIFF file");
    filesize = Read32BitsLowHigh(config.wave.file); /* complete wave chunk size */
    printf("Microsoft RIFF, ");

    if(!checkString(config.wave.file,"WAVE")) ERROR("Input not WAVE audio");
    printf("WAVE audio, ");

/* WAVE FMT format chunk */
    if(!checkString(config.wave.file,"fmt "))  ERROR("Can't find format chunk");
/* my total header size calculations don't work, so this is bogus... */
    header_size = Read32BitsLowHigh(config.wave.file); /* chunk size */

    wFormatTag = Read16BitsLowHigh(config.wave.file);
    if(wFormatTag!=0x0001) ERROR("Unknown WAVE format");
    config.wave.type = WAVE_RIFF_PCM;

    config.wave.channels   = Read16BitsLowHigh(config.wave.file);
    config.wave.samplerate = Read32BitsLowHigh(config.wave.file);
    dAvgBytesPerSec        = Read32BitsLowHigh(config.wave.file);
    wBlockAlign            = Read16BitsLowHigh(config.wave.file);

/* PCM specific */
    if(config.wave.channels>2) ERROR("More than 2 channels\n");
    printf("PCM, %s %ldHz ", 
           channel_mappings[config.wave.channels], config.wave.samplerate);
    config.wave.bits       = Read16BitsLowHigh(config.wave.file);
    if(config.wave.bits!=16) ERROR("NOT 16 Bit !!!\n");
    printf("%dbit, ", config.wave.bits);

    if(!checkString(config.wave.file,"data")) ERROR("Can't find data chunk");

    header_size = ftell(config.wave.file);
    fseek(config.wave.file, 0, SEEK_END);
    filesize = ftell(config.wave.file);
    fseek(config.wave.file, header_size, SEEK_SET);

    config.wave.total_samples = (filesize-header_size)/(2*config.wave.channels);
    config.wave.length = config.wave.total_samples/config.wave.samplerate;
    printf("Length: %2ld:%2ld:%2ld\n", (config.wave.length/3600), 
                                       (config.wave.length/60)%60,
                                       (config.wave.length%60)); 
    return true;
}

static int read_samples(short *sample_buffer,
                        int    frame_size)
{
    int samples_read;

    switch(config.wave.type)
    {
        default          :
            ERROR("[read_samples], wave filetype not supported");

        case WAVE_RIFF_PCM :
            samples_read = fread(sample_buffer,sizeof(short),frame_size, config.wave.file);
    
            /* Microsoft PCM Samples are little-endian, */
            /* we must swap if this is a big-endian machine */

           if(config.byte_order==order_bigEndian) 
               SwapBytesInWords(sample_buffer,samples_read);

           if(samples_read<frame_size && samples_read>0) 
           { /* Pad sample with zero's */
               while(samples_read<frame_size) sample_buffer[samples_read++] = 0;
           }

           break;
    }
    return samples_read;
}


int wave_get(short buffer[2][1152])
/* expects an interleaved 16bit pcm stream from read_samples, which it */
/* de-interleaves into buffer                                          */
{
    static short temp_buf[2304];
    int          samples_read;
    int          j;

    switch(config.mpeg.mode)
    {
        case MODE_MONO  :
            samples_read = read_samples(temp_buf,config.mpeg.samples_per_frame);
            for(j=0;j<1152;j++)
            {
                buffer[0][j] = temp_buf[j];
                buffer[1][j] = 0;
            }
            break;

        case MODE_STEREO : 
        case MODE_JSTEREO:
        case MODE_DUAL   :
            samples_read = read_samples(temp_buf,config.mpeg.samples_per_frame*2);
            for(j=0;j<1152;j++)
            {
                buffer[0][j] = temp_buf[2*j];
                buffer[1][j] = temp_buf[2*j+1];
            }
            break;

        default          :
            ERROR("[wav_get], channel mode not supported");
    }
    return samples_read;
}
 
