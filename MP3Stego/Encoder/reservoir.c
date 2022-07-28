/* $Header: /MP3Stego/MP3Stego Encoder/reservoir.c 5     30/11/00 15:57 Fabienpe $ */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "types.h"
#include "error.h"
#include "layer3.h"
#include "l3loop.h"
#include "huffman.h"
#include "l3bitstream.h"
#include "reservoir.h"

/* MP3STEGO-> */
#define MP3STEGO
/* ->MP3STEGO */

/* Layer3 bit reservoir: Described in C.1.5.4.2.2 of the IS */
static int ResvSize = 0; /* in bits */
static int ResvMax  = 0; /* in bits */

/*
  ResvFrameBegin:
  Called at the beginning of a frame. Updates the maximum
  size of the reservoir, and checks to make sure main_data_begin
  was set properly by the formatter
*/
void ResvFrameBegin(L3_side_info_t *l3_side, int mean_bits, int frameLength )
{
    int fullFrameBits;
    int expectedResvSize, resvLimit;

    if(config.mpeg.type==TYPE_MPEG_I ) resvLimit = 4088; /* main_data_begin has 9 bits in MPEG 1 */
    else                               resvLimit = 2040; /* main_data_begin has 8 bits in MPEG 2 */

/*
main_data_begin was set by the formatter to the
expected value for the next call -- this should
agree with our reservoir size
*/
    expectedResvSize = l3_side->main_data_begin * 8;
    if(expectedResvSize!=ResvSize) ERROR("ResvSize != expected size !!");
    fullFrameBits = mean_bits * config.mpeg.mode_gr;

/* determine maximum size of reservoir: ResvMax + frameLength <= 7680; */
    if(frameLength>7680) ResvMax = 0;
    else                 ResvMax = 7680 - frameLength;

/*
limit max size to resvLimit bits because
main_data_begin cannot indicate a
larger value
*/
    if(ResvMax>resvLimit) ResvMax = resvLimit;
}




/*
  ResvMaxBits:
  Called at the beginning of each granule to get the max bit
  allowance for the current granule based on reservoir size
  and perceptual entropy.
*/
int ResvMaxBits (L3_side_info_t *l3_side, double *pe, int mean_bits )
{
    int more_bits, max_bits, add_bits, over_bits;

    mean_bits /= config.wave.channels;
    max_bits = mean_bits;

    if(max_bits>4095) max_bits = 4095;
    if(ResvMax==0) return max_bits;

    more_bits = (int)(*pe * 3.1 - mean_bits);
    add_bits = 0;
    if(more_bits>100)
    {
	int frac = (ResvSize * 6) / 10;

	if(frac<more_bits) add_bits = frac;
	else               add_bits = more_bits;
    }
    over_bits = ResvSize - ((ResvMax * 8) / 10) - add_bits;
    if (over_bits>0) add_bits += over_bits;

    max_bits += add_bits;
    if(max_bits>4095) max_bits = 4095;
    return max_bits;
}


/*
  ResvAdjust:
  Called after a granule's bit allocation. Readjusts the size of
  the reservoir to reflect the granule's usage.
*/
void ResvAdjust(gr_info *gi, L3_side_info_t *l3_side, int mean_bits )
{
    ResvSize += (mean_bits / config.wave.channels) - gi->part2_3_length;

}

/*
  ResvFrameEnd:
  Called after all granules in a frame have been allocated. Makes sure
  that the reservoir size is within limits, possibly by adding stuffing
  bits. Note that stuffing bits are added by increasing a granule's
  part2_3_length. The bitstream formatter will detect this and write the
  appropriate stuffing bits to the bitstream.
*/
void ResvFrameEnd(L3_side_info_t *l3_side, int mean_bits )
{
    gr_info *gi;
    int gr, ch, ancillary_pad, stuffingBits;
    int over_bits;

    ancillary_pad = 0;

    /* just in case mean_bits is odd, this is necessary... */
    if((config.wave.channels==2) && (mean_bits & 1)) ResvSize += 1;

    over_bits = ResvSize - ResvMax;
    if(over_bits<0) over_bits = 0;
    
    ResvSize -= over_bits;
    stuffingBits = over_bits + ancillary_pad;

    /* we must be byte aligned */
    if((over_bits = ResvSize % 8))
    {
	stuffingBits += over_bits;
	ResvSize -= over_bits;
    }

    if(stuffingBits)
    {
	/*
	  plan a: put all into the first granule
	  This was preferred by someone designing a
	  real-time decoder...
	*/
	gi = (gr_info *) &(l3_side->gr[0].ch[0]);	
	
	if ( gi->part2_3_length + stuffingBits < 4095 )
    {
/* MP3STEGO-> */
#ifdef MP3STEGO
        if (stuffingBits % 2)
        {
            gi->part2_3_length += stuffingBits - 1;
            stuffingBits = 1;
            /* Spill one stuffing bit into ancillary data*/
            l3_side->resvDrain = stuffingBits;
        }
        else
            gi->part2_3_length += stuffingBits;
#else
/* ->MP3STEGO */
    gi->part2_3_length += stuffingBits;
#endif
    }
	else
	{
	    /* plan b: distribute throughout the granules */
	    for (gr = 0; gr < config.mpeg.mode_gr; gr++ )
		for (ch = 0; ch < config.wave.channels; ch++ )
		{
		    int extraBits, bitsThisGr;
		    gr_info *gi = (gr_info *) &(l3_side->gr[gr].ch[ch]);
		    if ( stuffingBits == 0 )
			break;
		    extraBits = 4095 - gi->part2_3_length;
		    bitsThisGr = extraBits < stuffingBits ? extraBits : stuffingBits;
/* MP3STEGO-> */
#ifdef STEGO
            if (bitsThisGr % 2)
                bitsThisGr -= 1;
#endif
/* ->MP3STEGO */
		    gi->part2_3_length += bitsThisGr;
		    stuffingBits -= bitsThisGr;
		}
	    /*
	      If any stuffing bits remain, we elect to spill them
	      into ancillary data. The bitstream formatter will do this if
	      l3side->resvDrain is set
	    */
	    l3_side->resvDrain = stuffingBits;
	}
    }
}


