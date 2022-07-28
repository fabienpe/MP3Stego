/*************************************************************************/
/*                                                                       */
/* 8hz-mp3: Multiplatform MPEG 1, Layer 3 encoder.                       */
/*                                                                       */
/* May 1998: 8hz, added WAV support, optmized calculations, added some   */
/*           precalc tables. Gained 60-70% over ISO implementation.      */
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*************************************************************************/

/* STEGO                                                        */
/* Some lines added or modified for the information hidding     */
/*                                                              */
/* Fabien A.P. Petitcolas, August 1998                          */
/* $Header: /MP3Stego/MP3Stego Encoder/main.c 8     30/11/00 15:57 Fabienpe $
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../../stegolib/stego.h"

#include "types.h"
#include "error.h"
#include "wave.h"
#include "layer3.h"

config_t config;
/* MP3STEGO-> */
char *pszPassword;
/* ->MP3STEGO */

static void print_header()
{
    fprintf(stderr,"MP3StegoEncoder %s\n", STEGO_VERSION);
    fprintf(stderr,"See README file for copyright info\n");
}

static void print_usage()
{
    fprintf(stderr,"USAGE   :  encode [options] <infile> <outfile>\n");
    fprintf(stderr,"OPTIONS : -h            this help message\n");
    fprintf(stderr,"          -b <bitrate>  set the bitrate, default 128kbit\n");
    fprintf(stderr,"          -c            set copyright flag, default off\n");
    fprintf(stderr,"          -o            set original flag, default off\n");
/* MP3STEGO-> */
    fprintf(stderr,"          -E <filename> name of the file to be hidden\n");
    fprintf(stderr,"          -P <text>     passphrase used for embedding\n");
/* ->MP3STEGO */
    fprintf(stderr,"\n"); 
}

static void set_defaults()
{
    config.byte_order = DetermineByteOrder();
    if(config.byte_order==order_unknown) ERROR("Can't determine byte order");

    config.mpeg.type = TYPE_MPEG_I;
    config.mpeg.layr = LAYR_III;
    config.mpeg.mode = MODE_STEREO;
    config.mpeg.bitr = 128;
    config.mpeg.psyc = PSYC_ATT;
    config.mpeg.emph = EMPH_NONE; 
    config.mpeg.crc  = 0;
    config.mpeg.ext  = 0;
    config.mpeg.mode_ext  = 0;
    config.mpeg.copyright = 0;
    config.mpeg.original  = 0;  
}

static bool parse_command(int argc, char** argv)
{
    int i = 0;

    if(argc<3) return false;
	
/* MP3STEGO-> */
    config.pszDataFile = NULL;
/* ->MP3STEGO */

    while(argv[++i][0]=='-')
        switch(argv[i][1])
        {
            case 'b' : config.mpeg.bitr = atoi(argv[++i]);
                       break;
            case 'c' : config.mpeg.copyright = 1;
                       break;
            case 'o' : config.mpeg.original  = 1;
                       break;
/* MP3STEGO-> */
            case 'E' : config.pszDataFile = argv[++i];
                       break;
            case 'P' : pszPassword = argv[++i];
                       break;
/* ->MP3STEGO */
            case 'h' :
            default  : return false;
       }

    if((argc-i)!=2) return false;
    config.infile  = argv[i++];
    config.outfile = argv[i];
    return true;
}

static int find_samplerate_index(long freq)
{
    static long mpeg1[3] = {44100, 48000, 32000};
    static long mpeg2[3] = {22050, 24000, 16000};
    long *table;
    int i;

    table = (config.mpeg.type==TYPE_MPEG_I)?mpeg1:mpeg2;
    for(i=0;i<3;i++)
        if(freq==table[i]) return i;

    ERROR("Invalid samplerate");
    return -1;
}

static int find_bitrate_index(int bitr)
{
    static long mpeg1[15] = {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320};
    static long mpeg2[15] = {0, 8,16,24,32,40,48,56, 64, 80, 96,112,128,144,160};
    long *table;
    int i;

    table = (config.mpeg.type==TYPE_MPEG_I)?mpeg1:mpeg2;
    for(i=0;i<15;i++)
        if(bitr==table[i]) return i;

    ERROR("Invalid bitrate");
    return -1;
}

static void check_config()
{
    static char *mode_names[4]    = { "stereo", "j-stereo", "dual-ch", "mono" };
    static char *layer_names[3]   = { "I", "II", "III" };
    static char *version_names[2] = { "MPEG-II (LSF)", "MPEG-I" };
    static char *psy_names[3]     = { "", "MUSICAM", "AT&T" };
    static char *demp_names[4]    = { "none", "50/15us", "", "CITT" };

    config.mpeg.samplerate_index = find_samplerate_index(config.wave.samplerate);
    config.mpeg.bitrate_index    = find_bitrate_index(config.mpeg.bitr);

	if (config.wave.channels == 1)
		config.mpeg.mode = MODE_MONO;

   printf("%s layer %s, %s  Psychoacoustic Model: %s\n",
           version_names[config.mpeg.type],
           layer_names[config.mpeg.layr], 
           mode_names[config.mpeg.mode],
           psy_names[config.mpeg.psyc]);
   printf("Bitrate=%d kbps  ",config.mpeg.bitr );
   printf("De-emphasis: %s  CRC: %s %s %s\n",
          demp_names[config.mpeg.emph], 
          ((config.mpeg.crc)?"on":"off"),
          ((config.mpeg.original)?"Original":""),
          ((config.mpeg.copyright)?"(C)":""));

}



int main(int argc, char **argv)
{
    time_t end_time;
    
/* MP3STEGO-> */
    pszPassword = NULL;

#if defined(_DEBUG)
	if (fLog == NULL)
	{
		fLog = fopen("MP3_Stego.txt", "w");
		if (fLog == NULL) ERROR("Could no create log file");
	}
#endif
/* ->MP3STEGO */

    time(&config.start_time);
    print_header();
    set_defaults();
    if(!parse_command(argc,argv)) { print_usage(); return -1; }
    if(!wave_open()) ERROR("Unable to open input file...");
    check_config();

    printf("Encoding \"%s\" to \"%s\"\n", config.infile, config.outfile);
    switch(config.mpeg.layr)
    {
        case LAYR_I  :
        case LAYR_II :
        default      :
            ERROR("Layer not supported");
        case LAYR_III:
            L3_compress();
            break;
    }
    wave_close();

    time(&end_time);
    end_time -= config.start_time;
    fprintf(stdout," Finished in %2ld:%2ld:%2ld\n",
            end_time/3600,(end_time/60)%60,end_time%60);
    return 0;
} 

