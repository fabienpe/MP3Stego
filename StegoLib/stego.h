/*---------------------------------------------------------------------------
 *
 * AUTHOR		Copyright (c) 1998 - Fabien Petitcolas
 *
 * PURPOSE		Encryption, compression and pseudo-random number functions for
 *              steganography. Header file.
 *
 *---------------------------------------------------------------------------
 */

#ifndef _STEGO_H_
#define _STEGO_H_

#if defined(_DEBUG)
extern FILE *fLog;
#endif

#define STEGO_VERSION ("1.1.19")

/* Encoding */
void StegoOpenEmbeddedText(char *pszFileName, size_t nMaxHiddenBits);
int  StegoGetNextBit();
void StegoCloseEmbeddedText();

/* Decoding */
void StegoCreateEmbeddedText();
void SaveHiddenBit(int bit);
void StegoFlushEmbeddedText(char *pszFileName);

#endif /* _STEGO_H_ */

