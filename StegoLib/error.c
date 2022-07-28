/*--------------------------------------------------------------------
 *
 * Contents: Error handling.
 *
 * Purpose:  
 *
 * Created:  Fabien A. P. Petitcolas
 *
 * Modified: 
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


#include "error.h"

#include <stdlib.h>

#ifdef WIN32
#ifndef __STDC__
#define __STDC__
#endif
#endif 

#ifdef __STDC__
void ERROR(char *format, ...)
{
    va_list args;
    va_start(args, format);
#else /*__STDC__*/
void ERROR(va_alist) va_dcl
{
    va_list args;
    char* format;

    va_start(args);
    format = va_arg(args, char*);
#endif /*__STDC__*/

    fprintf(stderr, "[ERROR]");
    (void) vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
    exit(-1);
}

#ifdef __STDC__
void MESSAGE(char* format, ...)
{
    va_list args;
    va_start(args, format);
#else /*__STDC__*/
void MESSAGE(va_alist)
    va_dcl
{
    va_list args;
    char* format;
    va_start(args);
    format = va_arg(args, char*);
#endif /*__STDC__*/

#ifdef VERBOSE
    (void) vfprintf(stderr, format, args);
    fputc('\n', stderr);
#endif
    va_end(args);
}

