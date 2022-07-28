/*---------------------------------------------------------------------------
 * FILE			$Workfile: tools.c $ - Part of MP3 Stego
 *
 * AUTHOR		Copyright (c) 1998 - Fabien Petitcolas
 *                                   University of Cambridge
 *
 * PURPOSE		Encryption, compression and pseudo-random number functions for
 *              steganography. Header file.
 *
 * $Modtime: 10/02/99 20:46 $
 * $Revision: 7 $
 * $Header: /StegoLib/tools.c 7     11/02/99 11:29 Fapp2 $
 * $Log: /StegoLib/tools.c $
 * 
 * 7     11/02/99 11:29 Fapp2
 * Minor warning
 * 
 * 6     9/02/99 22:26 Fapp2
 * Support for passphrase as command line parameter.
 * 
 * 5     15/08/98 10:38 Fapp2
 * Started revision control on this file.
 * 
 *---------------------------------------------------------------------------
 */
#ifndef WIN32
#ifndef UNIX
#define UNIX
#endif
#endif

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
#include <conio.h>
#define GETCHAR _getch()
#define stat _stat
#endif
#ifdef UNIX
#include <termios.h>
#include <unistd.h>
#define GETCHAR getchar()
#endif

#include "../gzlib/zlib.h"

#include "des.h"
#include "sha.h"
#include "error.h"
#include "tools.h"

extern char *pszPassword;
static char pszPass[MAX_LEN];

#if defined(DEBUG) | defined(_DEBUG)
/*#define DONT_ENCRYPT_OR_COMPRESS*/
#ifdef DONT_ENCRYPT_OR_COMPRESS
static void CopyFile(const char *in, const char *out);
#endif
#endif

/*---------------------------------------------------------------------------
 * Keep asking passphrase twice to the user until the two match              
 *---------------------------------------------------------------------------
 */
char *ReadPassPhrase(void)
{
    int i, ch;
    char tmp[MAX_LEN];
#ifdef UNIX
    struct termios attr;
#endif

    if (pszPassword != NULL)
    {
        strncpy(pszPass, pszPassword, MAX_LEN);
        return pszPass;
    }

    do {
        printf( "Enter a passphrase: " );
        fflush(stdout);
#ifdef UNIX
        /* Turn off echo */
        if (tcgetattr(STDIN_FILENO, &attr) != 0)
            ERROR("ReadPassPhrase: could not get terminal attributes.");
        attr.c_lflag &= ~(ECHO);
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &attr) != 0)
            ERROR("ReadPassPhrase: could not set terminal attributes.");
#endif
        for(i = 0; (i < MAX_LEN - 1) &&  ((ch = GETCHAR) != EOF) && (ch != '\n') && (ch != '\r'); i++)
            pszPass[i] = (char)ch;
        printf("\n");
        pszPass[i] = '\0';

        /* Request the passphrase twice for confirmation */
        printf( "Confirm your passphrase: " );
        fflush(stdout);
        for(i = 0; (i < MAX_LEN - 1) &&  ((ch = GETCHAR) != EOF) && (ch != '\n') && (ch != '\r'); i++)
            tmp[i] = (char)ch;
#ifdef UNIX
        /* Turn echo back*/
        attr.c_lflag |= ECHO;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &attr) != 0)
            ERROR("ReadPassPhrase: could not set terminal attributes.");
#endif
        printf("\n");
        tmp[i] = '\0';
    } while (strcmp(pszPass, tmp));

    /* Tidy */
    memset(tmp, 0, MAX_LEN);
    i = ch = 0;

    return pszPass;
}


/*---------------------------------------------------------------------------
 * Use the passphrase and SHA-1 to generate pseudo random bits.              
 * Each bit says whether a hidden bit should be embedded or not in the       
 * cover text. A counter is used introduce bias into the bit stream
 * by dropping 1 zero in COUNT_MAX. This increases the bandwidth at
 * the expense of security.
 *---------------------------------------------------------------------------
 */
int GetPseudoRandomBit(int cmd)
{
    char tmp[MAX_LEN + 20];
    int  res;
    static UINT32 hash[5];
    static int nBlockIndex = 0, nBitIndex = 0, bInit = 1, count = 0;


    if (bInit || cmd == RESET)
    {
        nBlockIndex = 0;
        nBitIndex = 0;
        count = 0;
        memset(tmp, 0, MAX_LEN + 20);
        memset(hash, 0, 20);
        memcpy(tmp, pszPass, strlen(pszPass));
        SHA_Memory(tmp, strlen(pszPass), hash);
        memset(tmp, 0, MAX_LEN + 20);
    }

    if (bInit) bInit = 0;

    switch (cmd)
    {
    case RESET: /* Restart the random bit generator */
        return DO_NOTHING;

    case NEXT: /* The next bit: EMBED, DONT_EMBED or DO_NOTHING */
        if ((hash[nBlockIndex] >> nBitIndex) & 0x1)
            res = EMBED;
        else
        {
            count++;
            res = DONT_EMBED;
        }

        nBitIndex = (nBitIndex + 1) % 32;
        if (nBitIndex == 0)
        {
            nBlockIndex = (nBlockIndex + 1) % 5;
            if (nBlockIndex == 0)
            {
                /* Hash previous hash with password */
                memcpy(tmp, hash, 20);
                memcpy(tmp + 20, pszPass, strlen(pszPass));
                SHA_Memory(tmp, 20 + strlen(pszPass), hash);
                memset(tmp, 0, MAX_LEN + 20);
            }
        }
        
        /* Introduce some bias */
        if (count == COUNT_MAX)
        {
            /* skip this DONT_EMBED */
            count = 0;
#if defined(DEBUG) | defined(_DEBUG)
            printf("<*>");
#endif
            /* return the next "command" */
            return GetPseudoRandomBit(NEXT);
        }
        else
        {
#if defined(DEBUG) | defined(_DEBUG)
            printf("<%d>", res);
#endif
            return res;
        }

    default:
        ERROR("GetPseudoRandomBit: Unknown command.");
        return DO_NOTHING;
    }
}

/*---------------------------------------------------------------------------
 * Generate a temporary file name                                            
 *---------------------------------------------------------------------------
 */
void GetTemporaryFileName(char pszTemp[256])
{
    char buf[256];

    if((tmpnam(buf)) == NULL)
        ERROR("GetTemporaryFileName: could not create temporary file.");

#ifdef WIN32
    strcpy(pszTemp, buf + 1); /* Dirty trick to bypass the / so the    */
                              /* file won't be created in the root dir */
#else
    strcpy(pszTemp, buf);
#endif
    strcat(pszTemp, TMP_FILE_EXT);
}

/*---------------------------------------------------------------------------
 * Compress and encrypt the datafile.                                        
 *---------------------------------------------------------------------------
 */
size_t CompressEncryptFile(const char *pszInput, const char *pszOutput,
                           const char *pszPassPhrase, int bCompEnc)
{
    char tmp[256];
    struct stat stats;
    size_t res = 0;

    GetTemporaryFileName(tmp);

    /* Compress-encrypt or decrypt-uncompress depending on bCompEnc */
    if (bCompEnc)
    {
#if defined(DONT_ENCRYPT_OR_COMPRESS)
        CopyFile(pszInput, pszOutput);
#else
        Compress(pszInput, tmp);
        Encrypt(tmp, pszOutput, pszPassPhrase, bCompEnc);
#endif
                
        if(stat(pszOutput, &stats) != 0)
            ERROR("CompressEncryptFile: could not determine file size.");
        res = stats.st_size;
    }
    else
    {
#if defined(DONT_ENCRYPT_OR_COMPRESS)
        CopyFile(pszInput, pszOutput);
#else
        Encrypt(pszInput, tmp, pszPassPhrase, bCompEnc);
        Uncompress(tmp, pszOutput);
#endif
    }

    if (remove(tmp))
            ERROR("CompressEncryptFile: could not delete temporary file.");

    return res;
}

/*---------------------------------------------------------------------------
 * Compress a file.                                                          
 *---------------------------------------------------------------------------
 */
void Compress(const char *pszInput, const char *pszOutput)
{
    char buf[256];
    gzFile fout;
    FILE *fin;
    int len;

    if ((fin = fopen(pszInput, "rb")) == NULL)
        ERROR("Compress: could not open input file.");

    if ((fout = gzopen(pszOutput, "wb")) == NULL) 
        ERROR("Compress: could create compressed file.");

    for (;;)
	{
        len = fread(buf, 1, sizeof(buf), fin);
        if (ferror(fin)) ERROR("Compress: error reading datafile.");
        if (len == 0) break;
        if (gzwrite(fout, buf, (unsigned)len) != len)
            ERROR("Compress: unexpected error during compression.");
    }

    if (fclose(fin))
        ERROR("Compress: error closing the input file.");;

    if (gzclose(fout) != Z_OK)
        ERROR("Compress: unexpected error while closing the compressed file.");
}

/*---------------------------------------------------------------------------
 * Uncompress a file.                                                        
 *---------------------------------------------------------------------------
 */
void Uncompress(const char *pszInput, const char *pszOutput)
{
    char buf[256];
    gzFile fout;
    FILE *fin;
    int len;

    if ((fin = gzopen(pszInput, "rb")) == NULL)
        ERROR("Compress: could not open input file.");

    if ((fout = fopen(pszOutput, "wb")) == NULL) 
        ERROR("Compress: could create compressed file.");

    for (;;)
	{
        len = gzread(fin, buf, sizeof(buf));
        if (len < 0) ERROR("Uncompress: error reading datafile.");
        if (len == 0) break;
        if ((int)fwrite(buf, 1, (unsigned)len, fout) != len)
            ERROR("Uncompress: unexpected error during compression.");
    }

    if (gzclose(fin) != Z_OK)
        ERROR("Uncompress: unexpected error while closing the compressed file.");

    if (fclose(fout))
        ERROR("Uncompress: error closing the input file.");
}

/*--------------------------------------------------------------------------
 * Triple DES encryption/decryption function on file
 *--------------------------------------------------------------------------
 */
void Encrypt(const char *pszInput, const char *pszOutput, 
               const char *pszPassPhrase, int bEncrypt)
{
    unsigned char pIV[8], bufIn[BLOCK_LEN], bufOut[BLOCK_LEN], rem, *p;
    int bFinished = 0, bInit = 1, i;
    des_cblock       pKeys[3];
	des_key_schedule pSchedule[3];
    size_t           nRead;
    UINT32           hash[5];
    FILE *fin, *fout;

    memset(bufIn, 0, sizeof(bufIn));
    memset(pKeys, 0, sizeof(pKeys));
    memset(bufOut, 0, sizeof(bufOut));
    memset(pSchedule, 0, sizeof(pSchedule));

    if ((fin = fopen(pszInput, "rb")) == NULL)
        ERROR("Encrypt: could not open input file.");

    if ((fout= fopen(pszOutput, "wb")) == NULL)
        ERROR("Encrypt: could not create encrypted file.");

	/* Use the hash of the pass-phrase to generate three keys */
	/* Each key is used to prepare a key schedule             */
    SHA_Memory(pszPassPhrase, strlen(pszPassPhrase), hash);

    for (i = 0; i < 3; i++)
	{
		memcpy(&pKeys[i], ((char *)hash) + 6 * i, 8);
		des_set_odd_parity(&pKeys[i]);
		if (des_is_weak_key(&pKeys[i]))
            ERROR("Encrypt: choose another passphrase.");

#if defined(_DEBUG) | defined(DEBUG)
        des_cblock_print_file(pKeys[i]);
#endif

        if (des_set_key(&pKeys[i], pSchedule[i]))
            ERROR("Encrypt: unexpected error while preparing the key schedule.");
		memset(pKeys[i], 0, 8);
	}

	/* The initialisation vector is initialised to 0 */
	memset(pIV, 0, sizeof(pIV));
        
    while (!bFinished)
    {
        if (bEncrypt)
        {
            /* Encrypt */
            nRead = fread(bufIn, 1, BLOCK_LEN, fin);

            if (feof(fin))
            {
                /* Padding */
		        p = bufIn + (nRead % 8);
		        srand((unsigned int)time(NULL));
		        for(i = 7 - (nRead % 8); i > 0; i--)
			        *p++ = (unsigned char)(rand() & 0xff);

                /* The last byte contains the number */
                /* of byte allocated for data        */
		        *p = (unsigned char)(nRead & 0xff);
                bFinished = 1;
            }

            des_ede3_cbc_encrypt((des_cblock *)bufIn, (des_cblock *)bufOut, BLOCK_LEN,
                pSchedule[0], pSchedule[1], pSchedule[2], (des_cblock *)pIV, bEncrypt);

            if (fwrite(bufOut, 1, BLOCK_LEN, fout) != BLOCK_LEN) 
                ERROR("Encrypt: error while writing the enciphered file");

#if defined(_DEBUG) | defined(DEBUG)
            des_cblock_print_file(bufOut);
#endif
        }
        else
        {
            /* Decrypt */
            nRead = fread(bufIn, 1, BLOCK_LEN, fin);

            if (feof(fin))
            {
                rem = bufOut[BLOCK_LEN - 1];
		        if (rem > 7) ERROR("Encrypt: unexpected end of cipher message.");
		        if (fwrite(bufOut, 1, rem, fout) != rem)
                    ERROR("Encrypt: error while writing the enciphered file");
                break;
            }

            if (bInit)
                bInit = 0;
            else
                if (fwrite(bufOut, 1, BLOCK_LEN, fout) != BLOCK_LEN) 
                    ERROR("Encrypt: error while writing the enciphered file.");

            if (nRead == BLOCK_LEN)
            {
		        des_ede3_cbc_encrypt((des_cblock *)bufIn, (des_cblock *)bufOut, BLOCK_LEN, 
                    pSchedule[0], pSchedule[1], pSchedule[2], (des_cblock *)pIV, bEncrypt);

#if defined(_DEBUG) | defined(DEBUG)
                des_cblock_print_file(bufIn);
#endif
            }
            else
                fprintf(stderr, "Encrypt: unexpected end of enciphered file. Output will be truncated.");
        }
    }

    if (fclose(fin))
        ERROR("Encrypt: error closing the input file.");

    if (fclose(fout))
        ERROR("Encrypt: error closing the encrypted file.");

    /* Set to zero sensitive data */
    memset(pIV, 0, sizeof(pIV));
    memset(bufIn, 0, sizeof(bufIn));
    memset(pKeys, 0, sizeof(pKeys));
    memset(bufOut, 0, sizeof(bufOut));
    memset(pSchedule, 0, sizeof(pSchedule));

#if defined(_DEBUG) | defined(DEBUG)
    printf("\n\n");
#endif
}


/*--------------------------------------------------------------------------
 * Debugging stuff... shall be removed
 *--------------------------------------------------------------------------
 */
#if defined(DONT_ENCRYPT_OR_COMPRESS)
static void CopyFile(const char *in, const char *out)
{
    unsigned char ch;
    int nRead;
    FILE *fin, *fout;

    fin = fopen(in, "rb");
    fout = fopen(out, "wb");
    while (!feof(fin))
    {
        nRead = fread(&ch, 1, 1, fin);
        fwrite(&ch, 1, nRead, fout);
    }
    fclose(fin);
    fclose(fout);
}
#endif
