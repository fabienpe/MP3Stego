/*---------------------------------------------------------------------------
 * FILE			$Workfile: stego.h $ - Part of MP3 Stego
 *
 * AUTHOR		Copyright (c) 1998 - Fabien Petitcolas
 *                                   University of Cambridge
 *
 * PURPOSE		Encryption, compression and pseudo-random number functions for
 *              steganography. Header file.
 *
 * $Modtime: 12/09/02 10:09 $
 * $Revision: 8 $
 * $Header: /StegoLib/stego.h 8     12/09/02 10:09 Fabienpe $
 * $Log: /StegoLib/stego.h $
 * 
 * 8     12/09/02 10:09 Fabienpe
 * Bug in StegoOpenEmbeddedText
 * 
 * 7     19/03/02 10:22 Fabienpe
 * ZLib was updated
 * 
 * 6     30/11/00 15:57 Fabienpe
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

#if defined(_DEBUG)
extern FILE *fLog;
#endif

#define STEGO_VERSION ("1.1.17")

/* Encoding */
void StegoOpenEmbeddedText(char *pszFileName, size_t nMaxHiddenBits);
int  StegoGetNextBit();
void StegoCloseEmbeddedText();

/* Decoding */
void StegoCreateEmbeddedText();
void SaveHiddenBit(int bit);
void StegoFlushEmbeddedText(char *pszFileName);

#endif /* _STEGO_H_ */

