/*
 * $Header: /MP3Stego/MP3Stego Encoder/l3bitstream.c 5     30/11/00 15:57 Fabienpe $
 */ 
#include <stdio.h>
#include <stdlib.h>

#include "../../stegolib/stego.h"

#include "l3bitstream.h" /* the public interface */
#include "l3psy.h"
#include "l3mdct.h"
#include "l3loop.h"
#include "formatBitstream.h" 
#include "huffman.h"
#include "bitstream.h"
#include "types.h"
#include "tables.h"
#include "error.h"

/*static int stereo = 1;*/
static bitstream_t *bs = NULL;

BF_FrameData    *frameData    = NULL;
BF_FrameResults *frameResults = NULL;

int PartHoldersInitialized = 0;

BF_PartHolder *headerPH;
BF_PartHolder *frameSIPH;
BF_PartHolder *channelSIPH[MAX_CHANNELS];
BF_PartHolder *spectrumSIPH[MAX_GRANULES][MAX_CHANNELS];
BF_PartHolder *scaleFactorsPH[MAX_GRANULES][MAX_CHANNELS];
BF_PartHolder *codedDataPH[MAX_GRANULES][MAX_CHANNELS];
BF_PartHolder *userSpectrumPH[MAX_GRANULES][MAX_CHANNELS];
BF_PartHolder *userFrameDataPH;

static int encodeSideInfo(L3_side_info_t  *si);
static void encodeMainData(int l3_enc[2][2][576], L3_side_info_t  *si, L3_scalefac_t   *scalefac);
static void write_ancillary_data(char *theData, int lengthInBits);
static void drain_into_ancillary_data(int lengthInBits);
static void Huffmancodebits(BF_PartHolder **pph, int *ix, gr_info *gi);


void putMyBits(uint32 val, uint16 len)
{
    putbits(bs, val, len);
}

/*
  L3_format_bitstream()
  
  This is called after a frame of audio has been quantized and coded.
  It will write the encoded audio to the bitstream. Note that
  from a layer3 encoder's perspective the bit stream is primarily
  a series of main_data() blocks, with header and side information
  inserted at the proper locations to maintain framing. (See Figure A.7
  in the IS).
*/

void
L3_format_bitstream(int        l3_enc[2][2][576],
		      L3_side_info_t   *l3_side,
		      L3_scalefac_t    *scalefac,
		      bitstream_t      *in_bs,
		      double           (*xr)[2][576],
		      char             *ancillary,
		      int              ancillary_bits)
{
    int gr, ch, i;
    bs = in_bs;
    
    if (frameData == NULL)
    {
		frameData = calloc(1, sizeof(*frameData));
		if (!frameData) ERROR("ENOMEM !");
    }
    if (frameResults == NULL)
    {
		frameResults = calloc(1, sizeof(*frameData));
		if (!frameData) ERROR("ENOMEM !!");
	}
	if (!PartHoldersInitialized)
    {
		headerPH = BF_newPartHolder(12);
		frameSIPH = BF_newPartHolder(12);

		for (ch = 0; ch < MAX_CHANNELS; ch++)
			channelSIPH[ch] = BF_newPartHolder(8);

		for (gr = 0; gr < MAX_GRANULES; gr++)	
			for (ch = 0; ch < MAX_CHANNELS; ch++)
			{
				spectrumSIPH[gr][ch]   = BF_newPartHolder(32);
				scaleFactorsPH[gr][ch] = BF_newPartHolder(64);
				codedDataPH[gr][ch]    = BF_newPartHolder(576);
				userSpectrumPH[gr][ch] = BF_newPartHolder(4);
			}
		userFrameDataPH = BF_newPartHolder(8);
		PartHoldersInitialized = 1;
    }

    for (gr = 0; gr < config.mpeg.mode_gr; gr++)
	for (ch =  0; ch < config.wave.channels; ch++)
	{
	    int *pi = &l3_enc[gr][ch][0];
	    double *pr = &xr[gr][ch][0];
	    for (i = 0; i < 576; i++, pr++, pi++)
	    {
		if ((*pr < 0) && (*pi > 0))
		    *pi *= -1;
	    }
	}

    encodeSideInfo(l3_side);
    encodeMainData(l3_enc, l3_side, scalefac);
    write_ancillary_data(ancillary, ancillary_bits);

    if (l3_side->resvDrain)
	drain_into_ancillary_data(l3_side->resvDrain);

    /*
      Put frameData together for the call
      to BitstreamFrame()
    */
    frameData->putbits     = putMyBits;
    frameData->frameLength = config.mpeg.bits_per_frame;
    frameData->nGranules   = config.mpeg.mode_gr;
    frameData->nChannels   = config.wave.channels;
    frameData->header      = headerPH->part;
    frameData->frameSI     = frameSIPH->part;

    for (ch = 0; ch < config.wave.channels; ch++)
		frameData->channelSI[ch] = channelSIPH[ch]->part;

    for (gr = 0; gr < config.mpeg.mode_gr; gr++)
	for (ch = 0; ch < config.wave.channels; ch++)
	{
	    frameData->spectrumSI[gr][ch]   = spectrumSIPH[gr][ch]->part;
	    frameData->scaleFactors[gr][ch] = scaleFactorsPH[gr][ch]->part;
	    frameData->codedData[gr][ch]    = codedDataPH[gr][ch]->part;
	    frameData->userSpectrum[gr][ch] = userSpectrumPH[gr][ch]->part;
	}
    
	frameData->userFrameData = userFrameDataPH->part;

    BF_BitstreamFrame(frameData, frameResults);

    /* we set this here -- it will be tested in the next loops iteration */
    l3_side->main_data_begin = frameResults->nextBackPtr;
}

void L3_FlushBitstream()
{
    if (!PartHoldersInitialized) ERROR("Not properly initialised !!");
		BF_FlushBitstream(frameData, frameResults);
}

static unsigned slen1_tab[16] = { 0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4 };
static unsigned slen2_tab[16] = { 0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3 };

static void encodeMainData(int             l3_enc[2][2][576],
                           L3_side_info_t  *si,
                           L3_scalefac_t   *scalefac)
{
    int i, gr, ch, sfb, window;

    for (gr = 0; gr < config.mpeg.mode_gr; gr++)
	for (ch = 0; ch < config.wave.channels; ch++)
	    scaleFactorsPH[gr][ch]->part->nrEntries = 0;

    for (gr = 0; gr < config.mpeg.mode_gr; gr++)
	for (ch = 0; ch < config.wave.channels; ch++)
	    codedDataPH[gr][ch]->part->nrEntries = 0;

    if (config.mpeg.type==TYPE_MPEG_I)
    {  /* MPEG 1 */
		for (gr = 0; gr < 2; gr++)
		{
		for (ch = 0; ch < config.wave.channels; ch++)
		{
			BF_PartHolder **pph = &scaleFactorsPH[gr][ch];		
			gr_info *gi = &(si->gr[gr].ch[ch].tt);
			unsigned slen1 = slen1_tab[gi->scalefac_compress];
			unsigned slen2 = slen2_tab[gi->scalefac_compress];
			int *ix = &l3_enc[gr][ch][0];

		if ((gi->window_switching_flag == 1) && (gi->block_type == 2))
		{
		    if (gi->mixed_block_flag)
		    {
			for (sfb = 0; sfb < 8; sfb++)
			    *pph = BF_addEntry(*pph,  scalefac->l[gr][ch][sfb], slen1);

			for (sfb = 3; sfb < 6; sfb++)
			    for (window = 0; window < 3; window++)
				*pph = BF_addEntry(*pph,  scalefac->s[gr][ch][sfb][window], slen1);

			for (sfb = 6; sfb < 12; sfb++)
			    for (window = 0; window < 3; window++)
				*pph = BF_addEntry(*pph,  scalefac->s[gr][ch][sfb][window], slen2);

		    }
		    else
		    {
			for (sfb = 0; sfb < 6; sfb++)
			    for (window = 0; window < 3; window++)
				*pph = BF_addEntry(*pph,  scalefac->s[gr][ch][sfb][window], slen1);

			for (sfb = 6; sfb < 12; sfb++)
			    for (window = 0; window < 3; window++)
				*pph = BF_addEntry(*pph,  scalefac->s[gr][ch][sfb][window], slen2);
		    }
		}
		else
		{
		    if ((gr == 0) || (si->scfsi[ch][0] == 0))
			for (sfb = 0; sfb < 6; sfb++)
			    *pph = BF_addEntry(*pph,  scalefac->l[gr][ch][sfb], slen1);

		    if ((gr == 0) || (si->scfsi[ch][1] == 0))
			for (sfb = 6; sfb < 11; sfb++)
			    *pph = BF_addEntry(*pph,  scalefac->l[gr][ch][sfb], slen1);

		    if ((gr == 0) || (si->scfsi[ch][2] == 0))
			for (sfb = 11; sfb < 16; sfb++)
			    *pph = BF_addEntry(*pph,  scalefac->l[gr][ch][sfb], slen2);

		    if ((gr == 0) || (si->scfsi[ch][3] == 0))
			for (sfb = 16; sfb < 21; sfb++)
			    *pph = BF_addEntry(*pph,  scalefac->l[gr][ch][sfb], slen2);
		}
		Huffmancodebits(&codedDataPH[gr][ch], ix, gi);
	    } /* for ch */
	} /* for gr */
    }
    else
    {  /* MPEG 2 */
	gr = 0;
	for (ch = 0; ch < config.wave.channels; ch++)
	{
	    BF_PartHolder **pph = &scaleFactorsPH[gr][ch];		
	    gr_info *gi = &(si->gr[gr].ch[ch].tt);
	    int *ix = &l3_enc[gr][ch][0];
	    int sfb_partition;

	    if (! gi->sfb_partition_table) ERROR("ERROR in part table");

	    if ((gi->window_switching_flag == 1) && (gi->block_type == 2))
	    {
		if (gi->mixed_block_flag)
		{
		    sfb_partition = 0;
		    for (sfb = 0; sfb < 8; sfb++)
			*pph = BF_addEntry(*pph,  scalefac->l[gr][ch][sfb], gi->slen[sfb_partition]);

		    for (sfb = 3, sfb_partition = 1; sfb_partition < 4; sfb_partition++)
		    {
			int sfbs = gi->sfb_partition_table[sfb_partition] / 3;
			int slen = gi->slen[sfb_partition];
			for (i = 0; i < sfbs; i++, sfb++)
			    for (window = 0; window < 3; window++)
				*pph = BF_addEntry(*pph,  scalefac->s[gr][ch][sfb][window], slen);
		    }
		}
		else
		{
		    for (sfb = 0, sfb_partition = 0; sfb_partition < 4; sfb_partition++)
		    {
			int sfbs = gi->sfb_partition_table[sfb_partition] / 3;
			int slen = gi->slen[sfb_partition];
			for (i = 0; i < sfbs; i++, sfb++)
			    for (window = 0; window < 3; window++)
				*pph = BF_addEntry(*pph,  scalefac->s[gr][ch][sfb][window], slen);
		    }
		}
	    }
	    else
	    {
		for (sfb = 0, sfb_partition = 0; sfb_partition < 4; sfb_partition++)
		{
		    int sfbs = gi->sfb_partition_table[sfb_partition];
		    int slen = gi->slen[sfb_partition];
		    for (i = 0; i < sfbs; i++, sfb++)
			*pph = BF_addEntry(*pph,  scalefac->l[gr][ch][sfb], slen);
		}
	    }
	    Huffmancodebits(&codedDataPH[gr][ch], ix, gi);
	} /* for ch */
    }
} /* main_data */

static unsigned int crc = 0;

static int encodeSideInfo(L3_side_info_t  *si)
{
    int gr, ch, scfsi_band, region, window, bits_sent;
    
    headerPH->part->nrEntries = 0;
    headerPH = BF_addEntry(headerPH, 0xfff,                          12);
    headerPH = BF_addEntry(headerPH, config.mpeg.type,               1);
/* HEADER HARDCODED SHOULDN`T BE THIS WAY ! */
    headerPH = BF_addEntry(headerPH, 1/*config.mpeg.layr*/,          2);
    headerPH = BF_addEntry(headerPH, !config.mpeg.crc,               1);
    headerPH = BF_addEntry(headerPH, config.mpeg.bitrate_index,      4);
    headerPH = BF_addEntry(headerPH, config.mpeg.samplerate_index,   2);
    headerPH = BF_addEntry(headerPH, config.mpeg.padding,            1);
    headerPH = BF_addEntry(headerPH, config.mpeg.ext,                1);
    headerPH = BF_addEntry(headerPH, config.mpeg.mode,               2);
    headerPH = BF_addEntry(headerPH, config.mpeg.mode_ext,           2);
    headerPH = BF_addEntry(headerPH, config.mpeg.copyright,          1);
    headerPH = BF_addEntry(headerPH, config.mpeg.original,           1);
    headerPH = BF_addEntry(headerPH, config.mpeg.emph,               2);
    
    bits_sent = 32;

    if (config.mpeg.crc)
    {
	headerPH = BF_addEntry(headerPH, crc, 16);
	bits_sent += 16;
    }

    frameSIPH->part->nrEntries = 0;

    for (ch = 0; ch < config.wave.channels; ch++)
	channelSIPH[ch]->part->nrEntries = 0;

    for (gr = 0; gr < config.mpeg.mode_gr; gr++)
	for (ch = 0; ch < config.wave.channels; ch++)
	    spectrumSIPH[gr][ch]->part->nrEntries = 0;

    if (config.mpeg.type == TYPE_MPEG_I)
    {  /* MPEG1 */
	frameSIPH = BF_addEntry(frameSIPH, si->main_data_begin, 9);

	if (config.wave.channels == 2)
	    frameSIPH = BF_addEntry(frameSIPH, si->private_bits, 3);
	else
	    frameSIPH = BF_addEntry(frameSIPH, si->private_bits, 5);
	
	for (ch = 0; ch < config.wave.channels; ch++)
	    for (scfsi_band = 0; scfsi_band < 4; scfsi_band++)
	    {
		BF_PartHolder **pph = &channelSIPH[ch];
		*pph = BF_addEntry(*pph, si->scfsi[ch][scfsi_band], 1);
	    }

	for (gr = 0; gr < 2; gr++)
	    for (ch = 0; ch < config.wave.channels ; ch++)
	    {
		BF_PartHolder **pph = &spectrumSIPH[gr][ch];
		gr_info *gi = &(si->gr[gr].ch[ch].tt);
		*pph = BF_addEntry(*pph, gi->part2_3_length,        12);
		*pph = BF_addEntry(*pph, gi->big_values,            9);
		*pph = BF_addEntry(*pph, gi->global_gain,           8);
		*pph = BF_addEntry(*pph, gi->scalefac_compress,     4);
		*pph = BF_addEntry(*pph, gi->window_switching_flag, 1);

		if (gi->window_switching_flag)
		{   
		    *pph = BF_addEntry(*pph, gi->block_type,       2);
		    *pph = BF_addEntry(*pph, gi->mixed_block_flag, 1);

		    for (region = 0; region < 2; region++)
			*pph = BF_addEntry(*pph, gi->table_select[region],  5);
		    for (window = 0; window < 3; window++)
			*pph = BF_addEntry(*pph, gi->subblock_gain[window], 3);
		}
		else
		{
		    if (gi->block_type!=0) ERROR("ERROR; invalid blocktype");
		    for (region = 0; region < 3; region++)
			*pph = BF_addEntry(*pph, gi->table_select[region], 5);

		    *pph = BF_addEntry(*pph, gi->region0_count, 4);
		    *pph = BF_addEntry(*pph, gi->region1_count, 3);
		}

		*pph = BF_addEntry(*pph, gi->preflag,            1);
		*pph = BF_addEntry(*pph, gi->scalefac_scale,     1);
		*pph = BF_addEntry(*pph, gi->count1table_select, 1);
	    }

	if (config.wave.channels == 2)
	    bits_sent += 256;
	else
	    bits_sent += 136;
    }
    else
    {  /* MPEG2 */
	frameSIPH = BF_addEntry(frameSIPH, si->main_data_begin, 8);

	if (config.wave.channels  == 2)
	    frameSIPH = BF_addEntry(frameSIPH, si->private_bits, 2);
	else
	    frameSIPH = BF_addEntry(frameSIPH, si->private_bits, 1);
	
	gr = 0;
	for (ch = 0; ch < config.wave.channels ; ch++)
	{
	    BF_PartHolder **pph = &spectrumSIPH[gr][ch];
	    gr_info *gi = &(si->gr[gr].ch[ch].tt);
	    *pph = BF_addEntry(*pph, gi->part2_3_length,        12);
	    *pph = BF_addEntry(*pph, gi->big_values,            9);
	    *pph = BF_addEntry(*pph, gi->global_gain,           8);
	    *pph = BF_addEntry(*pph, gi->scalefac_compress,     9);
	    *pph = BF_addEntry(*pph, gi->window_switching_flag, 1);

	    if (gi->window_switching_flag)
	    {   
		    *pph = BF_addEntry(*pph, gi->block_type,       2);
		    *pph = BF_addEntry(*pph, gi->mixed_block_flag, 1);

		    for (region = 0; region < 2; region++)
    		    *pph = BF_addEntry(*pph, gi->table_select[region],  5);
		    for (window = 0; window < 3; window++)
    		    *pph = BF_addEntry(*pph, gi->subblock_gain[window], 3);
	    }
	    else
	    {	
            for (region = 0; region < 3; region++)
    		    *pph = BF_addEntry(*pph, gi->table_select[region], 5);

            *pph = BF_addEntry(*pph, gi->region0_count, 4);
            *pph = BF_addEntry(*pph, gi->region1_count, 3);
	    }

	    *pph = BF_addEntry(*pph, gi->scalefac_scale,     1);
	    *pph = BF_addEntry(*pph, gi->count1table_select, 1);
	}
	if (config.wave.channels==2) bits_sent += 136;
	else                           bits_sent += 72;
    }
    return bits_sent;
}

static void write_ancillary_data(char *theData, int lengthInBits)
{
    int bytesToSend = lengthInBits / 8;
    int remainingBits = lengthInBits % 8;
    unsigned wrd;
    int i;

    userFrameDataPH->part->nrEntries = 0;

    for (i = 0; i < bytesToSend; i++)
    {
	wrd = theData[i];
	userFrameDataPH = BF_addEntry(userFrameDataPH, wrd, 8);
    }
    if (remainingBits)
    {
	/* right-justify remaining bits */
	wrd = theData[bytesToSend] >> (8 - remainingBits);
	userFrameDataPH = BF_addEntry(userFrameDataPH, wrd, remainingBits);
    }
    
}

/*
  Some combinations of bitrate, Fs, and stereo make it impossible to stuff
  out a frame using just main_data, due to the limited number of bits to
  indicate main_data_length. In these situations, we put stuffing bits into
  the ancillary data...
*/
static void
drain_into_ancillary_data(int lengthInBits)
{
    /*
     */
    int wordsToSend   = lengthInBits / 32;
    int remainingBits = lengthInBits % 32;
    int i;

    /*
      userFrameDataPH->part->nrEntries set by call to write_ancillary_data()
    */

    for (i = 0; i < wordsToSend; i++)
	userFrameDataPH = BF_addEntry(userFrameDataPH, 0, 32);
    if (remainingBits)
	userFrameDataPH = BF_addEntry(userFrameDataPH, 0, remainingBits);    
}

/*
  Note the discussion of huffmancodebits() on pages 28
  and 29 of the IS, as well as the definitions of the side
  information on pages 26 and 27.
  */
static void
Huffmancodebits(BF_PartHolder **pph, int *ix, gr_info *gi)
{
    int L3_huffman_coder_count1(BF_PartHolder **pph, struct huffcodetab *h, int v, int w, int x, int y);
    int bigv_bitcount(int ix[576], gr_info *cod_info);

    int region1Start;
    int region2Start;
    int i, bigvalues, count1End;
    int v, w, x, y, bits, cbits, xbits, stuffingBits;
    unsigned int code, ext;
    struct huffcodetab *h;
    int bvbits, c1bits, tablezeros, r0, r1, r2, rt, *pr;
    int bitsWritten = 0;
    int idx = 0;
	
    tablezeros = 0;
    r0 = r1 = r2 = 0;
    
    /* 1: Write the bigvalues */
    bigvalues = gi->big_values * 2;
    if (bigvalues)
    {
	if (!(gi->mixed_block_flag) && gi->window_switching_flag && (gi->block_type == 2))
	{ /* Three short blocks */
	    /*
	      Within each scalefactor band, data is given for successive
	      time windows, beginning with window 0 and ending with window 2.
	      Within each window, the quantized values are then arranged in
	      order of increasing frequency...
	      */
	    int sfb, window, line, start, end;

	    int (*ix_s)[192][3]; 
	    int *scalefac = &sfBandIndex[config.mpeg.samplerate_index+(config.mpeg.type*3)].s[0];
	    
	    ix_s = (int (*)[192][3]) ix;
	    region1Start = 12;
	    region2Start = 576;

	    for (sfb = 0; sfb < 13; sfb++)
	    {
		unsigned tableindex = 100;
		start = scalefac[sfb];
		end   = scalefac[sfb+1];

		if (start < region1Start)
		    tableindex = gi->table_select[0];
		else
		    tableindex = gi->table_select[1];
		if (tableindex >= 32) ERROR("tableindex error");

		for (window = 0; window < 3; window++)
		    for (line = start; line < end; line += 2)
		    {
			x = (*ix_s)[line][window];
			y = (*ix_s)[line + 1][window];
			if ((idx >= 576) || (idx < 0)) ERROR("id_x out of range");
			bits = HuffmanCode(tableindex, x, y, &code, &ext, &cbits, &xbits);
			*pph = BF_addEntry(*pph,  code, cbits);
			*pph = BF_addEntry(*pph,  ext, xbits);
			bitsWritten += bits;
		    }
		
	    }
	}
	else
	    if (gi->mixed_block_flag && gi->block_type == 2)
	    {  /* Mixed blocks long, short */
		int sfb, window, line, start, end;
		unsigned tableindex;
		int (*ix_s)[192][3];
                int *scalefac = &sfBandIndex[config.mpeg.samplerate_index+(config.mpeg.type*3)].s[0];
		
		ix_s = (int (*)[192][3]) ix;

		/* Write the long block region */
		tableindex = gi->table_select[0];
		if (tableindex)
		    for (i = 0; i < 36; i += 2)
		    {
			x = ix[i];
			y = ix[i + 1];
			bits = HuffmanCode(tableindex, x, y, &code, &ext, &cbits, &xbits);
			*pph = BF_addEntry(*pph,  code, cbits);
			*pph = BF_addEntry(*pph,  ext, xbits);
			bitsWritten += bits;
			
		    }
		/* Write the short block region */
		tableindex = gi->table_select[1];
		if (tableindex >= 32) ERROR("index error");

		for (sfb = 3; sfb < 13; sfb++)
		{
		    start = scalefac[sfb];
		    end   = scalefac[sfb+1];           
		    
		    for (window = 0; window < 3; window++)
			for (line = start; line < end; line += 2)
			{
			    x = (*ix_s)[line][window];
			    y = (*ix_s)[line + 1][window];
			    bits = HuffmanCode(tableindex, x, y, &code, &ext, &cbits, &xbits);
			    *pph = BF_addEntry(*pph,  code, cbits);
			    *pph = BF_addEntry(*pph,  ext, xbits);
			    bitsWritten += bits;
			}
		}

	    }
	    else
	    { /* Long blocks */
                int *scalefac = &sfBandIndex[config.mpeg.samplerate_index+(config.mpeg.type*3)].l[0];
		unsigned scalefac_index = 100;
		
		if (gi->mixed_block_flag)
		{
		    region1Start = 36;
		    region2Start = 576;
		}
		else
		{
		    scalefac_index = gi->region0_count + 1;
		    if (scalefac_index >= 23) ERROR("scalefactor error");
		    region1Start = scalefac[scalefac_index];
		    scalefac_index += gi->region1_count + 1;
		    if (scalefac_index >= 23) ERROR("scalefactor error");   
		    region2Start = scalefac[scalefac_index];
		    if (region1Start != gi->address1) ERROR("regionStart error");
		}
		for (i = 0; i < bigvalues; i += 2)
		{
		    unsigned tableindex = 100;
		    /* get table pointer */
		    if (i < region1Start)
		    {
			tableindex = gi->table_select[0];
			pr = &r0;
		    }
		    else
			if (i < region2Start)
			{
			    tableindex = gi->table_select[1];
			    pr = &r1;
			}
			else
			{
			    tableindex = gi->table_select[2];
			    pr = &r2;
			}
		    if (tableindex >= 32) ERROR("table index error");
		    h = &ht[tableindex];
		    /* get huffman code */
		    x = ix[i];
		    y = ix[i + 1];
		    if (tableindex)
		    {
			bits = HuffmanCode(tableindex, x, y, &code, &ext, &cbits, &xbits);
			*pph = BF_addEntry(*pph,  code, cbits);
			*pph = BF_addEntry(*pph,  ext, xbits);
			bitsWritten += rt = bits;
			*pr += rt;
		    }
		    else
		    {
			tablezeros += 1;
			*pr = 0;
		    }
		}
	    }
    }
    bvbits = bitsWritten; 

    /* 2: Write count1 area */
    if ((gi->count1table_select >= 2)) ERROR("gi error");
    h = &ht[gi->count1table_select + 32];
    count1End = bigvalues + (gi->count1 * 4);
    if (count1End > 576) ERROR("countEnd error");
    for (i = bigvalues; i < count1End; i += 4)
    {
	v = ix[i];
	w = ix[i+1];
	x = ix[i+2];
	y = ix[i+3];
	bitsWritten += L3_huffman_coder_count1(pph, h, v, w, x, y);
    }
    c1bits = bitsWritten - bvbits;
    if ((stuffingBits = gi->part2_3_length - gi->part2_length - bitsWritten))
    {
	int stuffingWords = stuffingBits / 32;
	int remainingBits = stuffingBits % 32;
	if (stuffingBits <= 0) ERROR("stufbits error");

	/*
	  Due to the nature of the Huffman code
	  tables, we will pad with ones
	*/
	while (stuffingWords--)
	    *pph = BF_addEntry(*pph, ~0, 32);
	if (remainingBits)
	    *pph = BF_addEntry(*pph, ~0, remainingBits);
	bitsWritten += stuffingBits;
    }
    if (bitsWritten != gi->part2_3_length - gi->part2_length) ERROR("bits write error");

/* MP3STEGO-> */
#if defined(_DEBUG)
	fprintf(fLog, "- %d Huffman bits written (%02d + %02d), part2_length = %d, part2_3_length = %d, %d stuffing\n",
	    bitsWritten, bvbits, c1bits, gi->part2_length, gi->part2_3_length, stuffingBits);
#endif
/* ->MP3STEGO */
}

int abs_and_sign(int *x)
{
    if (*x > 0) return 0;
    *x *= -1;
    return 1;
}

int L3_huffman_coder_count1(BF_PartHolder **pph, struct huffcodetab *h, int v, int w, int x, int y)
{
    HUFFBITS huffbits;
    unsigned int signv, signw, signx, signy, p;
    int len;
    int totalBits = 0;
    
    signv = abs_and_sign(&v);
    signw = abs_and_sign(&w);
    signx = abs_and_sign(&x);
    signy = abs_and_sign(&y);
    
    p = v + (w << 1) + (x << 2) + (y << 3);
    huffbits = h->table[p];
    len = h->hlen[p];
    *pph = BF_addEntry(*pph,  huffbits, len);
    totalBits += len;
    if (v)
    {
	*pph = BF_addEntry(*pph,  signv, 1);
	totalBits += 1;
    }
    if (w)
    {
	*pph = BF_addEntry(*pph,  signw, 1);
	totalBits += 1;
    }

    if (x)
    {
	*pph = BF_addEntry(*pph,  signx, 1);
	totalBits += 1;
    }
    if (y)
    {
	*pph = BF_addEntry(*pph,  signy, 1);
	totalBits += 1;
    }
    return totalBits;
}

/* Implements the pseudocode of page 98 of the IS */
int HuffmanCode(int table_select, int x, int y, unsigned int *code, 
                unsigned int *ext, int *cbits, int *xbits)
{
    unsigned signx, signy, linbitsx, linbitsy, linbits, xlen, ylen, idx;
    struct huffcodetab *h;

    *cbits = 0;
    *xbits = 0;
    *code  = 0;
    *ext   = 0;
    
    if (table_select==0) return 0;
    
    signx = abs_and_sign(&x);
    signy = abs_and_sign(&y);
    h = &(ht[table_select]);
    xlen = h->xlen;
    ylen = h->ylen;
    linbits = h->linbits;
    linbitsx = linbitsy = 0;

    if (table_select > 15)
    { /* ESC-table is used */
	if (x > 14)
	{
	    linbitsx = x - 15;
	    if (linbitsx > h->linmax) ERROR("linbits error");
	    x = 15;
	}
	if (y > 14)
	{
	    linbitsy = y - 15;
	    if (linbitsy > h->linmax) ERROR("linbits error");;
	    y = 15;
	}

	idx = (x * ylen) + y;
	*code  = h->table[idx];
	*cbits = h->hlen [idx];
	if (x > 14)
	{
	    *ext   |= linbitsx;
	    *xbits += linbits;
	}
	if (x != 0)
	{
	    *ext <<= 1;
	    *ext |= signx;
	    *xbits += 1;
	}
	if (y > 14)
	{
	    *ext <<= linbits;
	    *ext |= linbitsy;
	    *xbits += linbits;
	}
	if (y != 0)
	{
	    *ext <<= 1;
	    *ext |= signy;
	    *xbits += 1;
	}
    }
    else
    { /* No ESC-words */
	idx = (x * ylen) + y;
	*code = h->table[idx];
	*cbits += h->hlen[idx];
	if (x != 0)
	{
	    *code <<= 1;
	    *code |= signx;
	    *cbits += 1;
	}
	if (y != 0)
	{
	    *code <<= 1;
	    *code |= signy;
	    *cbits += 1;
	}
    }
    if ((*cbits > 32) || (*xbits > 32)) ERROR("cbits error");
    return *cbits + *xbits;
}

