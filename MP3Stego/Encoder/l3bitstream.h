#ifndef L3_BITSTREAM_H
#define L3_BITSTREAM_H

#include "types.h"
#include "bitstream.h"
#include "layer3.h"

typedef   bitstream_t L3_bitstream_t;

void L3_format_bitstream(int              l3_enc[2][2][576],
                         L3_side_info_t  *l3_side,
			 L3_scalefac_t   *scalefac,
			 L3_bitstream_t  *in_bs,
			 double           (*xr)[2][576],
			 char             *ancillary,
			 int              anc_bits);

int HuffmanCode(int table_select, int x, int y, unsigned *code,
                unsigned int *extword, int *codebits, int *extbits);

void L3_FlushBitstream();
int abs_and_sign(int *x); /* returns signx and changes *x to abs(*x) */

#endif
