/**********************************************************************
 * ISO MPEG Audio Subgroup Software Simulation Group (1996)
 * ISO 13818-3 MPEG-2 Audio Decoder - Lower Sampling Frequency Extension
 *
 * $Header: /MP3Stego Decoder/huffman.c 3     15/08/98 10:40 Fapp2 $
 *
 * $Id: huffman.c,v 1.2 1996/03/28 03:13:37 rowlands Exp $
 *
 * $Log: /MP3Stego Decoder/huffman.c $
 * 
 * 3     15/08/98 10:40 Fapp2
 * Started revision control on this file.
 * Revision 1.2  1996/03/28 03:13:37  rowlands
 * Merged layers 1-2 and layer 3 revisions
 *
 * Revision 1.1  1996/02/14 03:45:52  rowlands
 * Initial revision
 *
 * Received from FhG
 **********************************************************************/
/**********************************************************************
 *   date   programmers                comment                        *
 *27.2.92   F.O.Witte                  (ITT Intermetall)              *
 *				       email: otto.witte@itt-sc.de    *
 *				       tel:   ++49 (761)517-125	      *
 *				       fax:   ++49 (761)517-880	      *
 *12.6.92   J. Pineda                  Added sign bit to decoder.     *
 * 08/24/93 M. Iwadare                 Changed for 1 pass decoding.   *
 *--------------------------------------------------------------------*
 *  7/14/94 Juergen Koller      Bug fixes in Layer III code           *
 *********************************************************************/	

#include "common.h"
#include "huffman.h"
#include "stdlib.h"
     
HUFFBITS dmask = 1 << (sizeof(HUFFBITS)*8-1);
unsigned int hs = sizeof(HUFFBITS)*8;

struct huffcodetab ht[HTN];	/* array of all huffcodtable headers	*/
				/* 0..31 Huffman code table 0..31	*/
				/* 32,33 count1-tables			*/

/* read the huffman encode table */
int read_huffcodetab(FILE *fi) 
{

  char line[100],command[40],huffdata[40];
  unsigned int t,i,j,k,nn,x,y,n=0;
  unsigned int xl, yl, len;
  HUFFBITS h;
  int	hsize;
  
  hsize = sizeof(HUFFBITS)*8; 
  do {
      fgets(line,99,fi);
  } while ((line[0] == '#') || (line[0] < ' ') );
  
  do {    
    while ((line[0]=='#') || (line[0] < ' ')) {
      fgets(line,99,fi);
    } 

    sscanf(line,"%s %s %u %u %u",command,ht[n].tablename,
			         &xl,&yl,&ht[n].linbits);
    if (strcmp(command,".end")==0)
      return n;
    else if (strcmp(command,".table")!=0) {
      fprintf(stderr,"huffman table %u data corrupted\n",n);
      return -1;
    }
    ht[n].linmax = (1<<ht[n].linbits)-1;
   
    sscanf(ht[n].tablename,"%u",&nn);
    if (nn != n) {
      fprintf(stderr,"wrong table number %u\n",n);
      return(-2);
    } 

    ht[n].xlen = xl;
    ht[n].ylen = yl;

    do {
      fgets(line,99,fi);
    } while ((line[0] == '#') || (line[0] < ' '));

    sscanf(line,"%s %u",command,&t);
    if (strcmp(command,".reference")==0) {
      ht[n].ref   = t;
      ht[n].table = ht[t].table;
      ht[n].hlen  = ht[t].hlen;
      if ( (xl != ht[t].xlen) ||
           (yl != ht[t].ylen)  ) {
        fprintf(stderr,"wrong table %u reference\n",n);
        return (-3);
      };
      do {
        fgets(line,99,fi);
      } while ((line[0] == '#') || (line[0] < ' ') );
    } 
    else {
      ht[n].ref  = -1;
      ht[n].table=(HUFFBITS *) calloc(xl*yl,sizeof(HUFFBITS));
      if (ht[n].table == NULL) {
         fprintf(stderr,"unsufficient heap error\n");
         return (-4);
      }
      ht[n].hlen=(unsigned char *) calloc(xl*yl,sizeof(unsigned char));
      if (ht[n].hlen == NULL) {
         fprintf(stderr,"unsufficient heap error\n");
         return (-4);
      }
      for (i=0; i<xl; i++) {
        for (j=0;j<yl; j++) {
	  if (xl>1) 
            sscanf(line,"%u %u %u %s",&x, &y, &len,huffdata);
	  else 
            sscanf(line,"%u %u %s",&x,&len,huffdata);
          h=0;k=0;
	  while (huffdata[k]) {
            h <<= 1;
            if (huffdata[k] == '1')
              h++;
            else if (huffdata[k] != '0'){
              fprintf(stderr,"huffman-table %u bit error\n",n);
              return (-5);
            };
            k++;
          };
          if (k != len) {
           fprintf(stderr,
              "warning: wrong codelen in table %u, pos [%2u][%2u]\n",
	       n,i,j);
          };
          ht[n].table[i*xl+j] = h;
          ht[n].hlen[i*xl+j] = (unsigned char) len;
	  do {
            fgets(line,99,fi);
          } while ((line[0] == '#') || (line[0] < ' '));
        }
      }
    }
    n++;
  } while (1);
}

/* read the huffman decoder table */
int read_decoder_table(FILE *fi) 
{
  int n,i,nn,t;
  unsigned int v0,v1;
  char command[100],line[100];
  for (n=0;n<HTN;n++) {
    /* .table number treelen xlen ylen linbits */ 
    do {
      fgets(line,99,fi);
    } while ((line[0] == '#') || (line[0] < ' '));
     
    sscanf(line,"%s %s %u %u %u %u",command,ht[n].tablename,
           &ht[n].treelen, &ht[n].xlen, &ht[n].ylen, &ht[n].linbits);
    if (strcmp(command,".end")==0)
      return n;
    else if (strcmp(command,".table")!=0) {
      fprintf(stderr,"huffman table %u data corrupted\n",n);
      return -1;
    }
    ht[n].linmax = (1<<ht[n].linbits)-1;
   
    sscanf(ht[n].tablename,"%u",&nn);
    if (nn != n) {
      fprintf(stderr,"wrong table number %u\n",n);
      return(-2);
    } 
    do {
      fgets(line,99,fi);
    } while ((line[0] == '#') || (line[0] < ' '));

    sscanf(line,"%s %u",command,&t);
    if (strcmp(command,".reference")==0) {
      ht[n].ref   = t;
      ht[n].val   = ht[t].val;
      ht[n].treelen  = ht[t].treelen;
      if ( (ht[n].xlen != ht[t].xlen) ||
           (ht[n].ylen != ht[t].ylen)  ) {
        fprintf(stderr,"wrong table %u reference\n",n);
        return (-3);
      };
      while ((line[0] == '#') || (line[0] < ' ') ) {
        fgets(line,99,fi);
      }
    }    
    else if (strcmp(command,".treedata")==0) {
      ht[n].ref  = -1;
      ht[n].val = (unsigned char (*)[2]) 
        calloc(2*(ht[n].treelen),sizeof(unsigned char));
      if (ht[n].val == NULL) {
    	fprintf(stderr, "heaperror at table %d\n",n);
    	exit (-10);
      }
      for (i=0;i<ht[n].treelen; i++) {
        fscanf(fi,"%x %x",&v0, &v1);
        ht[n].val[i][0]=(unsigned char)v0;
        ht[n].val[i][1]=(unsigned char)v1;
      }
      fgets(line,99,fi); /* read the rest of the line */
    }
    else {
      fprintf(stderr,"huffman decodertable error at table %d\n",n);
    }
  }
  return n;
}


/* do the huffman coding,  */
/* note! for counta,countb - the 4 bit value is passed in y, set x to 0 */
/* return value: 0-no error, 1 decode error				*/
void huffman_coder(unsigned int x, unsigned int y, struct huffcodetab *h, 
                   Bit_stream_struc *bs)
/* x      x-value */
/* y      y-value */
/* h      pointer to huffman code record 	*/
/* bs     pointer to open write bitstream 	*/
{
  HUFFBITS huffbits; /* data left aligned */
  HUFFBITS linbitsX; 
  HUFFBITS linbitsY;
  unsigned int len;
  unsigned int xl1 = h->xlen-1;
  unsigned int yl1 = h->ylen-1;
  linbitsX = 0;
  linbitsY = 0;
  if (h->table == NULL) return;
  if (((x < xl1) || (xl1==0)) && (y < yl1)) {
    huffbits = h->table[x*(h->xlen)+y];
    len = h->hlen[x*(h->xlen)+y];
    putbits(bs,huffbits,len);
    return;
  }  
  else if (x >= xl1) {
    linbitsX = x-xl1;
    if (linbitsX > h->linmax) {
      fprintf(stderr,"warning: Huffman X table overflow\n");
      linbitsX= h->linmax;
    };
    if (y >= yl1) {
      huffbits = h->table[(h->ylen)*(h->xlen)-1];
      len = h->hlen[(h->ylen)*(h->xlen)-1];
      putbits(bs,huffbits,len);
      linbitsY = y-yl1;
      if (linbitsY > h->linmax) {
        fprintf(stderr,"warning: Huffman Y table overflow\n");
        linbitsY = h->linmax;
      };
      if (h->linbits) {
        putbits(bs,linbitsX,h->linbits);
        putbits(bs,linbitsY,h->linbits);
      }
    } 
    else { /* x>= h->xlen, y<h->ylen */
      huffbits = h->table[(h->ylen)*xl1+y];
      len = h->hlen[(h->ylen)*xl1+y];
      putbits(bs,huffbits,len);
      if (h->linbits) {
        putbits(bs,linbitsX,h->linbits);
      }
    }
  }
  else  { /* ((x < h->xlen) && (y>=h->ylen)) */
    huffbits = h->table[(h->ylen)*x+yl1];
    len = h->hlen[(h->ylen)*x+yl1];
    putbits(bs,huffbits,len);
    linbitsY = y-yl1;
    if (linbitsY > h->linmax) {
      fprintf(stderr,"warning: Huffman Y table overflow\n");
      linbitsY = h->linmax;
    };
    if (h->linbits) {
       putbits(bs,linbitsY,h->linbits);
    }
  }
}

/* do the huffman-decoding 						*/
/* note! for counta,countb -the 4 bit value is returned in y, discard x */
int huffman_decoder(struct huffcodetab *h, /* unsigned */ int *x,
                    /* unsigned */ int *y, int *v, int *w)
/* h   pointer to huffman code record	*/
/* x   returns decoded x value 		*/
/* y   returns decoded y value		*/
{  
  HUFFBITS level;
  int point = 0;
  int error = 1;
  level     = dmask;
  if (h->val == NULL) return 2;

  /* table 0 needs no bits */
  if ( h->treelen == 0)
  {  *x = *y = 0;
     return 0;
  }


  /* Lookup in Huffman table. */

  do {
    if (h->val[point][0]==0) {   /*end of tree*/
      *x = h->val[point][1] >> 4;
      *y = h->val[point][1] & 0xf;

      error = 0;
      break;
    } 
    if (hget1bit()) {
      while (h->val[point][1] >= MXOFF) point += h->val[point][1]; 
      point += h->val[point][1];
    }
    else {
      while (h->val[point][0] >= MXOFF) point += h->val[point][0]; 
      point += h->val[point][0];
    }
    level >>= 1;
  } while (level  || (point < ht->treelen) );
  
  /* Check for error. */
  
  if (error) { /* set x and y to a medium value as a simple concealment */
    printf("Illegal Huffman code in data.\n");
    *x = (h->xlen-1 << 1);
    *y = (h->ylen-1 << 1);
  }

  /* Process sign encodings for quadruples tables. */

  if (h->tablename[0] == '3'
      && (h->tablename[1] == '2' || h->tablename[1] == '3')) {
     *v = (*y>>3) & 1;
     *w = (*y>>2) & 1;
     *x = (*y>>1) & 1;
     *y = *y & 1;

     /* v, w, x and y are reversed in the bitstream. 
        switch them around to make test bistream work. */
     
/*   {int i=*v; *v=*y; *y=i; i=*w; *w=*x; *x=i;}  MI */

     if (*v)
        if (hget1bit() == 1) *v = -*v;
     if (*w)
        if (hget1bit() == 1) *w = -*w;
     if (*x)
        if (hget1bit() == 1) *x = -*x;
     if (*y)
        if (hget1bit() == 1) *y = -*y;
     }
     
  /* Process sign and escape encodings for dual tables. */
  
  else {
  
      /* x and y are reversed in the test bitstream.
         Reverse x and y here to make test bitstream work. */
	 
/*    removed 11/11/92 -ag  
		{int i=*x; *x=*y; *y=i;} 
*/      
     if (h->linbits)
       if ((h->xlen-1) == *x) 
         *x += hgetbits(h->linbits);
     if (*x)
        if (hget1bit() == 1) *x = -*x;
     if (h->linbits)	  
       if ((h->ylen-1) == *y)
         *y += hgetbits(h->linbits);
     if (*y)
        if (hget1bit() == 1) *y = -*y;
     }
	  
  return error;  
}
