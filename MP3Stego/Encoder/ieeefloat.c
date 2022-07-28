/* Copyright (C) 1988-1991 Apple Computer, Inc.
 * All Rights Reserved.
 *
 * Warranty Information
 * Even though Apple has reviewed this software, Apple makes no warranty
 * or representation, either express or implied, with respect to this
 * software, its quality, accuracy, merchantability, or fitness for a 
 * particular purpose.  As a result, this software is provided "as is,"
 * and you, its user, are assuming the entire risk as to its quality
 * and accuracy.
 *
 * This code may be used and freely distributed as long as it includes
 * this copyright notice and the warranty information.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *	Apple Macintosh, MPW 3.1 C compiler
 *	Apple Macintosh, THINK C compiler
 *	Silicon Graphics IRIS, MIPS compiler
 *	Cray X/MP and Y/MP
 *	Digital Equipment VAX
 *	Sequent Balance (Multiprocesor 386)
 *	NeXT
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 *
 * $Header: /MP3Stego Encoder/ieeefloat.c 3     15/08/98 10:40 Fapp2 $
 * 
 * $Id: ieeefloat.c,v 1.1 1993/06/11 17:45:46 malcolm Exp $
 *
 * $Log: /MP3Stego Encoder/ieeefloat.c $
 * 
 * 3     15/08/98 10:40 Fapp2
 * Started revision control on this file.
 * Revision 1.1  1993/06/11  17:45:46  malcolm
 * Initial revision
 *
 */

#include	<stdio.h>
#include	<math.h>
#include	"ieeefloat.h"


/****************************************************************
 * The following two routines make up for deficiencies in many
 * compilers to convert properly between unsigned integers and
 * floating-point.  Some compilers which have this bug are the
 * THINK_C compiler for the Macintosh and the C compiler for the
 * Silicon Graphics MIPS-based Iris.
 ****************************************************************/

#define FloatToUnsigned(f)	((unsigned long)(((long)((f) - 2147483648.0)) + 2147483647L + 1))
#define UnsignedToFloat(u)	(((defdouble)((long)((u) - 2147483647L - 1))) + 2147483648.0)


/****************************************************************
 * Single precision IEEE floating-point conversion routines
 ****************************************************************/

#define SEXP_MAX		255
#define SEXP_OFFSET		127
#define SEXP_SIZE		8
#define SEXP_POSITION	(32-SEXP_SIZE-1)


defdouble ConvertFromIeeeSingle(char *bytes)
{
	defdouble	f;
	long	mantissa, expon;
	long	bits;

	bits =	((unsigned long)(bytes[0] & 0xFF) << 24)
		|	((unsigned long)(bytes[1] & 0xFF) << 16)
		|	((unsigned long)(bytes[2] & 0xFF) << 8)
		|	 (unsigned long)(bytes[3] & 0xFF);		/* Assemble bytes into a long */

	if ((bits & 0x7FFFFFFF) == 0) {
		f = 0;
	}

	else {
		expon = (bits & 0x7F800000) >> SEXP_POSITION;
		if (expon == SEXP_MAX) {		/* Infinity or NaN */
			f = HUGE_VAL;		/* Map NaN's to infinity */
		}
		else {
			if (expon == 0) {	/* Denormalized number */
				mantissa = (bits & 0x7fffff);
				f = ldexp((defdouble)mantissa, expon - SEXP_OFFSET - SEXP_POSITION + 1);
			}
			else {				/* Normalized number */
				mantissa = (bits & 0x7fffff) + 0x800000;	/* Insert hidden bit */
				f = ldexp((defdouble)mantissa, expon - SEXP_OFFSET - SEXP_POSITION);
			}
		}
	}

	if (bits & 0x80000000)
		return -f;
	else
		return f;
}


/****************************************************************/


void ConvertToIeeeSingle(defdouble num, char *bytes)
{
	long	sign;
	register long bits;

	if (num < 0) {	/* Can't distinguish a negative zero */
		sign = 0x80000000;
		num *= -1;
	} else {
		sign = 0;
	}

	if (num == 0) {
		bits = 0;
	}

	else {
		defdouble fMant;
		int expon;

		fMant = frexp(num, &expon);

		if ((expon > (SEXP_MAX-SEXP_OFFSET+1)) || !(fMant < 1)) {
			/* NaN's and infinities fail second test */
			bits = sign | 0x7F800000;		/* +/- infinity */
		}

		else {
			long mantissa;

			if (expon < -(SEXP_OFFSET-2)) {	/* Smaller than normalized */
				int shift = (SEXP_POSITION+1) + (SEXP_OFFSET-2) + expon;
				if (shift < 0) {	/* Way too small: flush to zero */
					bits = sign;
				}
				else {			/* Nonzero denormalized number */
					mantissa = (long)(fMant * (1L << shift));
					bits = sign | mantissa;
				}
			}

			else {				/* Normalized number */
				mantissa = (long)floor(fMant * (1L << (SEXP_POSITION+1)));
				mantissa -= (1L << SEXP_POSITION);			/* Hide MSB */
				bits = sign | ((long)((expon + SEXP_OFFSET - 1)) << SEXP_POSITION) | mantissa;
			}
		}
	}

	bytes[0] = bits >> 24;	/* Copy to byte string */
	bytes[1] = bits >> 16;
	bytes[2] = bits >> 8;
	bytes[3] = bits;
}


/****************************************************************
 * Double precision IEEE floating-point conversion routines
 ****************************************************************/

#define DEXP_MAX		2047
#define DEXP_OFFSET		1023
#define DEXP_SIZE		11
#define DEXP_POSITION	(32-DEXP_SIZE-1)


defdouble ConvertFromIeeeDouble(char *bytes)
{
	defdouble	f;
	long	mantissa, expon;
	unsigned long first, second;

	first = ((unsigned long)(bytes[0] & 0xFF) << 24)
		|	((unsigned long)(bytes[1] & 0xFF) << 16)
		|	((unsigned long)(bytes[2] & 0xFF) << 8)
		|	 (unsigned long)(bytes[3] & 0xFF);
	second= ((unsigned long)(bytes[4] & 0xFF) << 24)
		|	((unsigned long)(bytes[5] & 0xFF) << 16)
		|	((unsigned long)(bytes[6] & 0xFF) << 8)
		|	 (unsigned long)(bytes[7] & 0xFF);
	
	if (first == 0 && second == 0) {
		f = 0;
	}

	else {
		expon = (first & 0x7FF00000) >> DEXP_POSITION;
		if (expon == DEXP_MAX) {		/* Infinity or NaN */
			f = HUGE_VAL;		/* Map NaN's to infinity */
		}
		else {
			if (expon == 0) {	/* Denormalized number */
				mantissa = (first & 0x000FFFFF);
				f = ldexp((defdouble)mantissa, expon - DEXP_OFFSET - DEXP_POSITION + 1);
				f += ldexp(UnsignedToFloat(second), expon - DEXP_OFFSET - DEXP_POSITION + 1 - 32);
			}
			else {				/* Normalized number */
				mantissa = (first & 0x000FFFFF) + 0x00100000;	/* Insert hidden bit */
				f = ldexp((defdouble)mantissa, expon - DEXP_OFFSET - DEXP_POSITION);
				f += ldexp(UnsignedToFloat(second), expon - DEXP_OFFSET - DEXP_POSITION - 32);
			}
		}
	}

	if (first & 0x80000000)
		return -f;
	else
		return f;
}


/****************************************************************/


void ConvertToIeeeDouble(defdouble num, char *bytes)
{
	long	sign;
	long	first, second;

	if (num < 0) {	/* Can't distinguish a negative zero */
		sign = 0x80000000;
		num *= -1;
	} else {
		sign = 0;
	}

	if (num == 0) {
		first = 0;
		second = 0;
	}

	else {
		defdouble fMant, fsMant;
		int expon;

		fMant = frexp(num, &expon);

		if ((expon > (DEXP_MAX-DEXP_OFFSET+1)) || !(fMant < 1)) {
			/* NaN's and infinities fail second test */
			first = sign | 0x7FF00000;		/* +/- infinity */
			second = 0;
		}

		else {
			long mantissa;

			if (expon < -(DEXP_OFFSET-2)) {	/* Smaller than normalized */
				int shift = (DEXP_POSITION+1) + (DEXP_OFFSET-2) + expon;
				if (shift < 0) {	/* Too small for something in the MS word */
					first = sign;
					shift += 32;
					if (shift < 0) {	/* Way too small: flush to zero */
						second = 0;
					}
					else {			/* Pretty small demorn */
						second = FloatToUnsigned(floor(ldexp(fMant, shift)));
					}
				}
				else {			/* Nonzero denormalized number */
					fsMant = ldexp(fMant, shift);
					mantissa = (long)floor(fsMant);
					first = sign | mantissa;
					second = FloatToUnsigned(floor(ldexp(fsMant - mantissa, 32)));
				}
			}

			else {				/* Normalized number */
				fsMant = ldexp(fMant, DEXP_POSITION+1);
				mantissa = (long)floor(fsMant);
				mantissa -= (1L << DEXP_POSITION);			/* Hide MSB */
				fsMant -= (1L << DEXP_POSITION);
				first = sign | ((long)((expon + DEXP_OFFSET - 1)) << DEXP_POSITION) | mantissa;
				second = FloatToUnsigned(floor(ldexp(fsMant - mantissa, 32)));
			}
		}
	}
	
	bytes[0] = first >> 24;
	bytes[1] = first >> 16;
	bytes[2] = first >> 8;
	bytes[3] = first;
	bytes[4] = second >> 24;
	bytes[5] = second >> 16;
	bytes[6] = second >> 8;
	bytes[7] = second;
}


/****************************************************************
 * Extended precision IEEE floating-point conversion routines
 ****************************************************************/

defdouble ConvertFromIeeeExtended(char *bytes)
{
	defdouble	f;
	long	expon;
	unsigned long hiMant, loMant;

#ifdef	TEST	
printf("ConvertFromIEEEExtended(%lx,%lx,%lx,%lx,%lx,%lx,%lx,%lx,%lx,%lx\r",
	(long)bytes[0], (long)bytes[1], (long)bytes[2], (long)bytes[3], 
	(long)bytes[4], (long)bytes[5], (long)bytes[6], 
	(long)bytes[7], (long)bytes[8], (long)bytes[9]);
#endif
	
	expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
	hiMant	=	((unsigned long)(bytes[2] & 0xFF) << 24)
			|	((unsigned long)(bytes[3] & 0xFF) << 16)
			|	((unsigned long)(bytes[4] & 0xFF) << 8)
			|	((unsigned long)(bytes[5] & 0xFF));
	loMant	=	((unsigned long)(bytes[6] & 0xFF) << 24)
			|	((unsigned long)(bytes[7] & 0xFF) << 16)
			|	((unsigned long)(bytes[8] & 0xFF) << 8)
			|	((unsigned long)(bytes[9] & 0xFF));

	if (expon == 0 && hiMant == 0 && loMant == 0) {
		f = 0;
	}
	else {
		if (expon == 0x7FFF) {	/* Infinity or NaN */
			f = HUGE_VAL;
		}
		else {
			expon -= 16383;
			f  = ldexp(UnsignedToFloat(hiMant), expon-=31);
			f += ldexp(UnsignedToFloat(loMant), expon-=32);
		}
	}

	if (bytes[0] & 0x80)
		return -f;
	else
		return f;
}


/****************************************************************/


void ConvertToIeeeExtended(defdouble num, char *bytes)
{
	int	sign;
	int expon;
	defdouble fMant, fsMant;
	unsigned long hiMant, loMant;

	if (num < 0) {
		sign = 0x8000;
		num *= -1;
	} else {
		sign = 0;
	}

	if (num == 0) {
		expon = 0; hiMant = 0; loMant = 0;
	}
	else {
		fMant = frexp(num, &expon);
		if ((expon > 16384) || !(fMant < 1)) {	/* Infinity or NaN */
			expon = sign|0x7FFF; hiMant = 0; loMant = 0; /* infinity */
		}
		else {	/* Finite */
			expon += 16382;
			if (expon < 0) {	/* denormalized */
				fMant = ldexp(fMant, expon);
				expon = 0;
			}
			expon |= sign;
			fMant = ldexp(fMant, 32);          fsMant = floor(fMant); hiMant = FloatToUnsigned(fsMant);
			fMant = ldexp(fMant - fsMant, 32); fsMant = floor(fMant); loMant = FloatToUnsigned(fsMant);
		}
	}
	
	bytes[0] = expon >> 8;
	bytes[1] = expon;
	bytes[2] = hiMant >> 24;
	bytes[3] = hiMant >> 16;
	bytes[4] = hiMant >> 8;
	bytes[5] = hiMant;
	bytes[6] = loMant >> 24;
	bytes[7] = loMant >> 16;
	bytes[8] = loMant >> 8;
	bytes[9] = loMant;
}
