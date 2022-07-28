/*--------------------------------------------------------------------
 *
 * Contents: Error handling.
 *
 * Purpose:  
 *
 * Created:  Fabien A. P. Petitcolas
 *
 * Modified: Encryption, compression and pseudo-random number
 *           functions for steganography.
 *
 * History:
 *
 * Copyright (c) 1998, Fabien A. P. Petitcolas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted for noncommercial research and academic
 * use only, provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *   Each individual file must retain its own copyright notice.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions, the following disclaimer and the
 *   list of contributors in the documentation and/or other materials
 *   provided with the distribution.
 *
 * - Modification of the program or portion of it is allowed provided
 *   that the modified files carry prominent notices stating where and
 *   when they have been changed. If you do modify this program you
 *   should send to the contributors a general description of the
 *   changes and send them a copy of your changes at their request. By
 *   sending any changes to this program to the contributors, you are
 *   granting a license on such changes under the same terms and
 *   conditions as provided in this license agreement.  However, the
 *   contributors are under no obligation to accept your changes.
 *
 * - All noncommercial advertising materials mentioning features or
 *   use of this software must display the following acknowledgement:
 *
 *   This product includes software developed by Fabien A. P. Petitcolas
 *   when he was with the University of Cambridge.
 *
 * THIS SOFTWARE IS NOT INTENDED FOR ANY COMMERCIAL APPLICATION AND IS
 * PROVIDED BY FABIEN A. P. PETITCOLAS `AS IS', WITH ALL FAULTS AND ANY
 * EXPRESS OR IMPLIED REPRESENTATIONS OR WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED REPRESENTATIONS OR WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE, TITLE OR NONINFRINGEMENT OF
 * INTELLECTUAL PROPERTY ARE DISCLAIMED. IN NO EVENT SHALL FABIEN A.
 * PETITCOLAS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *--------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>

#include "error.h"
#include "stego.h"
#include "tools.h"

#define STEGO_BUFFER_SIZE 5  /* Buffer size in bytes */

static FILE *fEmbeddedText = NULL;     /* File containing the hidden data */
static unsigned char *pBuffer = NULL;  /* Buffer                          */
static char pszTemp[256];              /* Name of temporary file that will*/
                                       /* be read or written by the stego */
char pszPassPhrase[MAX_LEN];           /* Passphrase for encryption and   */
                                       /* bit selection                   */
static size_t nBufferIndex = 0;        /* Byte index within the buffer    */
static size_t lData = 0;               /* Length of hidden data after     */
                                       /* compression and encryption      */

/* STEGO */
#if defined(_DEBUG)
extern FILE *fLog = NULL;
extern FILE *fEmbedded = NULL;
#endif
/* STEGO */

/*---------------------------------------------------------------------------
 * Open the file that contains the data to be hidden, compress it and
 * and encrypt it in a temporary file
 *---------------------------------------------------------------------------
 */
void StegoOpenEmbeddedText(char *pszFileName, size_t nMaxHiddenBits)
{
    size_t nEmbed = 0, nDontEmbed = 0, nRandomBits = 0;

#if defined(_DEBUG)
    fEmbedded = fopen("Embedded_bits.txt", "wb");
#endif
   
    strcpy(pszPassPhrase, ReadPassPhrase());

    GetTemporaryFileName(pszTemp);

    lData = CompressEncryptFile(pszFileName, pszTemp, pszPassPhrase, 1);

    GetPseudoRandomBit(RESET);
    while (nEmbed < ((lData * 8) + 32))
    {
        if (GetPseudoRandomBit(NEXT) == EMBED)
            nEmbed++;
        else
            nDontEmbed++;
    }
    nRandomBits = nEmbed + nDontEmbed;
    GetPseudoRandomBit(RESET);

    if (nRandomBits > nMaxHiddenBits)
        ERROR("StegoOpenEmbeddedText: data file too long. You can hide roughly %d bits.", nMaxHiddenBits);

	if ((fEmbeddedText = fopen(pszTemp, "rb")) == NULL)
        ERROR("StegoOpenEmbeddedText: data file not found.");

#if defined(_DEBUG)
    printf("\n\n");
#endif
}

/*---------------------------------------------------------------------------
 * Returns the next bit to be hidden or '2' if nothing should be hidden      
 * at this location                                                          
 *---------------------------------------------------------------------------
 */
int StegoGetNextBit()
{
	unsigned char *p;
	static int bNeedMoreData = 1, bFinished = 0, bEOF = 0, bBody = 0;
	static size_t nBufferIndex, maxBufferIndex, nBitIndex;
	size_t nRead;
	int bit;
	
	if (fEmbeddedText)
	{
		if (bFinished)
			return 2;
        if (GetPseudoRandomBit(NEXT) != EMBED)
            return 2;
		if (pBuffer == NULL)
		{
			pBuffer = (unsigned char *)malloc(STEGO_BUFFER_SIZE);
			if (pBuffer == NULL)
                ERROR("StegoGetNextBit: not enough memory");
            memset(pBuffer, 0, STEGO_BUFFER_SIZE);
		}
        if (bBody == 0)
        {
            bBody = 1;
            memcpy(pBuffer, &lData, sizeof(lData));
            nRead = sizeof(lData);
            maxBufferIndex = nRead - 1;
            bNeedMoreData = 0;
        }
        if (bNeedMoreData)
		{
			nRead = fread(pBuffer, 1, STEGO_BUFFER_SIZE, fEmbeddedText);
			bEOF = feof(fEmbeddedText);
			if ((nRead < STEGO_BUFFER_SIZE) && !bEOF) 
                ERROR("StegoGetNextBit: error reading data file");
            if (nRead == 0)
            {
                bFinished = 1;
                return 2;
            }
            else
			    maxBufferIndex = nRead - 1;
			bNeedMoreData = 0;
		}
		p = pBuffer + nBufferIndex;
		bit = (int)((*p >> nBitIndex) & 0x1);
		nBitIndex = (nBitIndex + 1) % 8;
		if (nBitIndex == 0)
		{
			nBufferIndex = (nBufferIndex + 1) % (maxBufferIndex + 1);
			if (nBufferIndex == 0)
			{
				if (bEOF) /* then ther's no more data to embedd */
					bFinished = 1;
				else
					bNeedMoreData = 1;
			}
		}
#if defined(_DEBUG)
       fwrite(&bit, 1, 1, fEmbedded);
#endif
		return bit;
	}
	return 2;
}

/*---------------------------------------------------------------------------
 * Close the compressed & encrypted data file and delete it                  
 *---------------------------------------------------------------------------
 */
void StegoCloseEmbeddedText()
{
#if defined(_DEBUG)
       fclose(fEmbedded);
#endif

    if (fclose(fEmbeddedText) != 0) ERROR("Data file not closed properly.");

    /* Tidy */
    if (pBuffer) free(pBuffer);
    memset(pszPassPhrase, 0, strlen(pszPassPhrase));
    remove(pszTemp);
}


/*---------------------------------------------------------------------------
 * Prepare the decoding
 *---------------------------------------------------------------------------
 */
void StegoCreateEmbeddedText()
{
#if defined(_DEBUG)
    fEmbedded = fopen("Extracted_bits.txt", "wb");
#endif
    strcpy(pszPassPhrase, ReadPassPhrase());
    GetPseudoRandomBit(RESET);
    GetTemporaryFileName(pszTemp);
    if ((fEmbeddedText = fopen(pszTemp, "wb")) == NULL)
        ERROR("StegoCreateEmbeddedText: could not create data file.\n");
}

/*---------------------------------------------------------------------------
 * Append new extracted bit to temporary file if this bit is selected
 * by the pseudo random bit generator
 *---------------------------------------------------------------------------
 */
void SaveHiddenBit(int bit)
{
	unsigned char *p;
	static int bFinished = 0, bBody = 0;
	static size_t nBitIndex;
	size_t nWritten;
    static size_t nSaved = 0;

	if (fEmbeddedText && !bFinished && (GetPseudoRandomBit(NEXT) == EMBED))
	{
#if defined(_DEBUG)
        printf("%d", bit);
        fwrite(&bit, 1, 1, fEmbedded);
#endif
		if (pBuffer == NULL)
		{
			pBuffer = (unsigned char *)malloc(STEGO_BUFFER_SIZE);
			if (pBuffer == NULL)
                ERROR("SaveHiddenBit: not enough memory\n");
			memset(pBuffer, 0, STEGO_BUFFER_SIZE);
		}
        
        if (bBody)
            if (++nSaved == (lData * 8))
                bFinished = 1;

		p = pBuffer + nBufferIndex;
		if (bit) *p |= (1 << nBitIndex);
		nBitIndex = (nBitIndex + 1) % 8;

		if (nBitIndex == 0)
		{
    		nBufferIndex = (nBufferIndex + 1) % STEGO_BUFFER_SIZE;
            if (bBody)
            {
                if (nBufferIndex == 0)
                {
                    /* Buffer is full */
			        nWritten = fwrite(pBuffer, 1, STEGO_BUFFER_SIZE, fEmbeddedText);
			        if (nWritten != STEGO_BUFFER_SIZE)
                        ERROR("SaveHiddenBit: error writting data file\n");
			        memset(pBuffer, 0, STEGO_BUFFER_SIZE);
                }
            }
            else
            {
                if (nBufferIndex == sizeof(lData))
                {
                    memcpy(&lData, pBuffer, sizeof(lData));
                    memset(pBuffer, 0, STEGO_BUFFER_SIZE);
                    bBody = 1;
                    nBufferIndex = nBitIndex = 0;
                }
            }
		}
	}
}

/*---------------------------------------------------------------------------
 * Finish decoding
 *---------------------------------------------------------------------------
 */
void StegoFlushEmbeddedText(char *pszFileName)
{
#if defined(_DEBUG)
       fclose(fEmbedded);
#endif

	if (fEmbeddedText)
	{
		fwrite(pBuffer, 1, nBufferIndex, fEmbeddedText);
		if (fclose(fEmbeddedText) != 0)
            ERROR("StegoFlushEmbeddedText: data file not closed properly.\n");
                
        CompressEncryptFile(pszTemp, pszFileName, pszPassPhrase, 0);

        /* Tidy */
        if (pBuffer) free(pBuffer);
        memset(pszPassPhrase, 0, strlen(pszPassPhrase));
        remove(pszTemp);
	}
}
