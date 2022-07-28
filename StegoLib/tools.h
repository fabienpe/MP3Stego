/*---------------------------------------------------------------------------
 *
 * AUTHOR		Copyright (c) 1998 - Fabien Petitcolas
 *
 * PURPOSE		Encryption, compression and pseudo-random number functions for
 *              steganography. Header file.
 *
 *---------------------------------------------------------------------------
 */#ifndef _TOOLS_H_
#define _TOOLS_H_

#define MAX_LEN    (256) /* Maximum length of the passphrase */
#define BLOCK_LEN  (8)   /* Encryption block length          */

#define NEXT       (0)   /* Commands for the pseudo random   */
#define RESET      (1)   /* number generator                 */

#define DONT_EMBED (0)   /* Commabds returned by the psudo   */
#define EMBED      (1)   /* random number generator          */
#define DO_NOTHING (2)

#define COUNT_MAX  (3)   /* To increase the bandwidth, we    */
                         /* introduce a bias in the ranfom   */
                         /* bit generator by skiping one 0   */
                         /* every COUNT_MAX 0                */

#define TMP_FILE_EXT ("tmp")

void GetTemporaryFileName(char pszTemp[256]);

char *ReadPassPhrase(void);

int GetPseudoRandomBit(int cmd);

size_t CompressEncryptFile(const char *pszInput, const char *pszOutput,
                           const char *pszPassPhrase, int bCompEnc);

void Compress(const char *pszInput, const char *pszOutput);
void Uncompress(const char *pszInput, const char *pszOutput);

void Encrypt(const char *pszInput, const char *pszOutput, 
               const char *pszPassPhrase, int bEncrypt);

#endif /* _TOOLS_H_ */

