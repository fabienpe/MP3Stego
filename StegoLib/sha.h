/* Implementation of NIST's Secure Hash Algorithm (FIPS 180)
 * Lightly bummed for execution efficiency.
 *
 * Jim Gillogly 3 May 1993
 *
 * 27 Aug 93: imported LITTLE_ENDIAN mods from Peter Gutmann's implementation
 * 5 Jul 94: Modified for NSA fix
 *
 * Compile: cc -O -o sha sha.c
 *
 * To remove the test wrapper and use just the nist_hash() routine,
 *      compile with -DONT_WRAP
 *
 * To reverse byte order for little-endian machines, use -DLITTLE_ENDIAN
 *
 * To get the original SHA definition before the 1994 fix, use -DVERSION_0
 *
 * Usage: sha [-vt] [filename ...]
 *
 *      -v switch: output the filename as well
 *      -t switch: suppress spaces between 32-bit blocks
 *
 *      If no input files are specified, process standard input.
 *
 * Output: 40-hex-digit digest of each file specified (160 bits)
 *
 * Synopsis of the function calls:
 *
 *   SHA_File(char *filename, unsigned long *buffer)
 *      Filename is a file to be opened and processed.
 *      buffer is a user-supplied array of 5 or more longs.
 *      The 5-word buffer is filled with 160 bits of non-terminated hash.
 *      Returns 0 if successful, non-zero if bad file.
 *
 *   void SHA_Stream(FILE *stream, unsigned long *buffer)
 *      Input is from already-opened stream, not file.
 *
 *   void SHA_Memory(char *mem, long length, unsigned long *buffer)
 *      Input is a memory block "length" bytes long.
 *
 * Caveat:
 *      Not tested for case that requires the high word of the length,
 *      which would be files larger than 1/2 gig or so.
 *
 * Limitation:
 *      SHA_Memory (the memory block function) will deal with blocks no longer
 *      than 4 gigabytes; for longer samples, the stream version will
 *      probably be most convenient (e.g. perl moby_data.pl | sha).
 *
 * Bugs:
 *      The standard is defined for bit strings; I assume bytes.
 *
 * Copyright 1993, Dr. James J. Gillogly
 * This code may be freely used in any application.
 */

/* $Header: /StegoLib/sha.h 4     15/08/98 10:39 Fapp2 $ */

#ifndef _SHALIB_SHA_H_
#define _SHALIB_SHA_H_

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN
#endif
#define MEMTEST    /* Test implementation before doing anything */

/*#define VERSION_0  Define this to get the original SHA definition*/

#include <stdio.h>
#include <memory.h>

#define VERBOSE

#if defined (_WIN32)
typedef unsigned __int32  UINT32;
typedef __int32      INT32;
#else
typedef unsigned long int UINT32;
typedef long int     INT32;
#endif

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define SHA_SUCCESS 0
#define SHA_FAILURE -1
        

/* External entries */
int SHA_Test();
int SHA_File(const char * filename, UINT32 *buffer);
void SHA_Memory(const char * mem, INT32 length, UINT32 buffer[5]);

#ifndef ONT_WRAP        /* Using just the hash routine itself */
#define HASH_SIZE 5     /* Produces 160-bit digest of the message */
#endif /* ONT_WRAP */

/*#define LITTLE_ENDIAN*/

union longbyte
{
    UINT32 W[80];        /* Process 16 32-bit words at a time   */
    char B[320];         /* But read them as bytes for counting */
};
           
           
#endif /* _SHALIB_SHA_H_ */


