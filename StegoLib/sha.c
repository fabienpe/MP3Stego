/* $Header: /StegoLib/sha.c 4     15/08/98 10:39 Fapp2 $ */

#include <stdlib.h>
#include "sha.h"

static void SHA_Stream(FILE *stream, UINT32 *buffer);
static void nist_guts();

#ifdef LITTLE_ENDIAN    /* Imported from Peter Gutmann's implementation */

/* When run on a little-endian CPU we need to perform byte reversal on an
   array of longwords.  It is possible to make the code endianness-
   independant by fiddling around with data at the byte level, but this
   makes for very slow code, so we rely on the user to sort out endianness
   at compile time */

static void byteReverse( UINT32 *buffer, int byteCount )
{
    UINT32 value;
    int count;

    byteCount /= sizeof( UINT32 );
    for(count = 0; count < byteCount; count++)
	{
		value = (buffer[count] << 16) | (buffer[count] >> 16);
		buffer[count] = ((value & 0xFF00FF00L) >> 8) | ((value & 0x00FF00FFL)<<8);
	}
}
#endif /* LITTLE_ENDIAN */

int SHA_File(const char * filename, UINT32 *buffer)      /* Hash a file */
{
    FILE *infile;

    if ((infile = fopen(filename, "rb")) == NULL)
    {
		int i;
		for (i = 0; i < 5; i++)
			buffer[i] = 0xdeadbeef;
		return SHA_FAILURE;
    }
    (void) SHA_Stream(infile, buffer);
    fclose(infile);
    return SHA_SUCCESS;
}

void SHA_Memory(const char * mem, INT32 length, UINT32 buffer[5])    /* Hash a memory block */
{
	buffer[0]=0;
    nist_guts(FALSE, (FILE *) NULL, mem, length, buffer);
}

static void SHA_Stream(FILE *stream, UINT32 *buffer)
{
    nist_guts(TRUE, stream, (char *) NULL, 0l, buffer);
}

#define f0(x,y,z) (z ^ (x & (y ^ z)))           /* Magic functions */
#define f1(x,y,z) (x ^ y ^ z)
#define f2(x,y,z) ((x & y) | (z & (x | y)))
#define f3(x,y,z) (x ^ y ^ z)

#define K0 0x5a827999                           /* Magic constants */
#define K1 0x6ed9eba1
#define K2 0x8f1bbcdc
#define K3 0xca62c1d6

#define S(n, X) ((X << n) | (X >> (32 - n)))    /* Barrel roll */

#define r0(f, K) \
    temp = S(5, A) + f(B, C, D) + E + *p0++ + K; \
    E = D;  \
    D = C;  \
    C = S(30, B); \
    B = A;  \
    A = temp

#ifdef VERSION_0
#define r1(f, K) \
    temp = S(5, A) + f(B, C, D) + E + \
	   (*p0++ = *p1++ ^ *p2++ ^ *p3++ ^ *p4++) + K; \
    E = D;  \
    D = C;  \
    C = S(30, B); \
    B = A;  \
    A = temp
#else                   /* Version 1: Summer '94 update */
#define r1(f, K) \
    temp = *p1++ ^ *p2++ ^ *p3++ ^ *p4++; \
    temp = S(5, A) + f(B, C, D) + E + (*p0++ = S(1,temp)) + K; \
    E = D;  \
    D = C;  \
    C = S(30, B); \
    B = A;  \
    A = temp
#endif

static void nist_guts(int file_flag, FILE *stream, char *mem, INT32 length, UINT32 *buf)
{
    int i, nread, nbits;
    union longbyte d;
    UINT32 hi_length, lo_length;
    int padded;
    char *s;

    register UINT32 *p0, *p1, *p2, *p3, *p4;
    UINT32 A, B, C, D, E, temp;

    UINT32 h0, h1, h2, h3, h4;

    h0 = 0x67452301;                            /* Accumulators */
    h1 = 0xefcdab89;
    h2 = 0x98badcfe;
    h3 = 0x10325476;
    h4 = 0xc3d2e1f0;

    padded = FALSE;
    s = mem;
    for (hi_length = lo_length = 0; ;)  /* Process 16 longs at a time */
    {
	if (file_flag)
	{
		nread = (int)fread(d.B, 1, 64, stream);  /* Read as 64 bytes */
	}
	else
	{
		if (length < 64) nread = (int) length;
		else             nread = 64;
		length -= nread;
		memcpy(d.B, s, nread);
		s += nread;
	}
	if (nread < 64)   /* Partial block? */
	{
		nbits = nread << 3;               /* Length: bits */
		if ((lo_length += nbits) < (unsigned long)nbits)
			hi_length++;              /* 64-bit integer */

		if (nread < 64 && ! padded)  /* Append a single bit */
		{
			d.B[nread++] = (char) 0x80; /* Using up next byte */
			padded = TRUE;       /* Single bit once */
		}
		for (i = nread; i < 64; i++) /* Pad with nulls */
			d.B[i] = 0;
		if (nread <= 56)   /* Room for length in this block */
		{
			d.W[14] = hi_length;
			d.W[15] = lo_length;
#ifdef LITTLE_ENDIAN
	      byteReverse(d.W, 56 );
#endif /* LITTLE_ENDIAN */
		}
#ifdef LITTLE_ENDIAN
	   else byteReverse(d.W, 64 );
#endif /* LITTLE_ENDIAN */
	}
	else    /* Full block -- get efficient */
	{
		if ((lo_length += 512) < 512)
			hi_length++;    /* 64-bit integer */
#ifdef LITTLE_ENDIAN
	   byteReverse(d.W, 64 );
#endif /* LITTLE_ENDIAN */
	}

	p0 = d.W;
	A = h0; B = h1; C = h2; D = h3; E = h4;

	r0(f0,K0); r0(f0,K0); r0(f0,K0); r0(f0,K0); r0(f0,K0);
	r0(f0,K0); r0(f0,K0); r0(f0,K0); r0(f0,K0); r0(f0,K0);
	r0(f0,K0); r0(f0,K0); r0(f0,K0); r0(f0,K0); r0(f0,K0);
	r0(f0,K0);

	p1 = &d.W[13]; p2 = &d.W[8]; p3 = &d.W[2]; p4 = &d.W[0];

		   r1(f0,K0); r1(f0,K0); r1(f0,K0); r1(f0,K0);
	r1(f1,K1); r1(f1,K1); r1(f1,K1); r1(f1,K1); r1(f1,K1);
	r1(f1,K1); r1(f1,K1); r1(f1,K1); r1(f1,K1); r1(f1,K1);
	r1(f1,K1); r1(f1,K1); r1(f1,K1); r1(f1,K1); r1(f1,K1);
	r1(f1,K1); r1(f1,K1); r1(f1,K1); r1(f1,K1); r1(f1,K1);
	r1(f2,K2); r1(f2,K2); r1(f2,K2); r1(f2,K2); r1(f2,K2);
	r1(f2,K2); r1(f2,K2); r1(f2,K2); r1(f2,K2); r1(f2,K2);
	r1(f2,K2); r1(f2,K2); r1(f2,K2); r1(f2,K2); r1(f2,K2);
	r1(f2,K2); r1(f2,K2); r1(f2,K2); r1(f2,K2); r1(f2,K2);
	r1(f3,K3); r1(f3,K3); r1(f3,K3); r1(f3,K3); r1(f3,K3);
	r1(f3,K3); r1(f3,K3); r1(f3,K3); r1(f3,K3); r1(f3,K3);
	r1(f3,K3); r1(f3,K3); r1(f3,K3); r1(f3,K3); r1(f3,K3);
	r1(f3,K3); r1(f3,K3); r1(f3,K3); r1(f3,K3); r1(f3,K3);

	h0 += A; h1 += B; h2 += C; h3 += D; h4 += E;

	if (nread <= 56) break; /* If it's greater, length in next block */
    }
    buf[0] = h0;
    buf[1] = h1; buf[2] = h2; buf[3] = h3; buf[4] = h4;
}
