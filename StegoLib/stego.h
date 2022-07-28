/*---------------------------------------------------------------------------
 * FILE			$Workfile: stego.h $ - Part of MP3 Stego
 *
 * AUTHOR		Copyright (c) 1998 - Fabien Petitcolas
 *                                   University of Cambridge
 *
 * PURPOSE		Encryption, compression and pseudo-random number functions for
 *              steganography. Header file.
 *
 * $Modtime: 15/08/98 10:38 $
 * $Revision: 4 $
 * $Header: /StegoLib/stego.h 4     15/08/98 10:38 Fapp2 $
 * $Log: /StegoLib/stego.h $
 * 
 * 4     15/08/98 10:38 Fapp2
 * Started revision control on this file.
 * 
 * 5     15/08/98 10:36 Fapp2
 * Started revision control on this file.
 *---------------------------------------------------------------------------
 */

#ifndef _STEGO_H_
#define _STEGO_H_

#if defined(DEBUG) | defined(_DEBUG)
extern FILE *fLog;
#endif

#define STEGO_VERSION ("1.1.15")

/* Encoding */
void StegoOpenEmbeddedText(char *pszFileName, size_t nMaxHiddenBits);
int  StegoGetNextBit();
void StegoCloseEmbeddedText();

/* Decoding */
void StegoCreateEmbeddedText();
void SaveHiddenBit(int bit);
void StegoFlushEmbeddedText(char *pszFileName);

#endif /* _STEGO_H_ */

