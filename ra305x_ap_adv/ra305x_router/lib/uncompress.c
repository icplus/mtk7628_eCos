/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    uncompress.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#ifndef  UNCOMPRESS_C
#define  UNCOMPRESS_C

#include <sys/types.h>

// ---------lzma
#include "LzmaDec.h"

//#define BUFZONE	0x8000
#define LZMA_PROPS_SIZE	5
#define LZMA86_SIZE_OFFSET	(LZMA_PROPS_SIZE)
#define LZMA86_HEADER_SIZE	(LZMA86_SIZE_OFFSET + 8)
// ---------lzma


#ifndef NULL
#define NULL ((void *) 0)
#endif

//static uch *output_data;
//static ulg output_ptr;


//static void *malloc(int size);
//static void free(void *where);

// --------------lzma
static void *SzAlloc(void *p, size_t size) { p = p; return malloc(size); }
static void SzFree(void *p, void *address) { p = p; free(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };
// --------------lzma



#if 0
/* ===========================================================================
 * Write the output window window[0..outcnt-1] and update crc and bytes_out.
 * (Used for the decompressed data only.)
 */
static void flush_window(void)
{
	ulg c = crc;
	unsigned n;
	uch *in, *out, ch;

	in = window;
	out = &output_data[output_ptr];
	for (n = 0; n < outcnt; n++) {
		ch = *out++ = *in++;
		c = crc_32_tab[((int)c ^ ch) & 0xff] ^ (c >> 8);
	}
	crc = c;
	bytes_out += (ulg)outcnt;
	output_ptr += (ulg)outcnt;
	outcnt = 0;
}
#endif

int uncompress(char* dst, unsigned int *dlen, char *src, int size)
{
	int rc;
	//unsigned int dlen = dst - (char *)fh - BUFZONE;
	SizeT inSizePure;
	ELzmaStatus status;
	
	//output_data=dst;
	
	inSizePure = size - LZMA86_HEADER_SIZE;
	rc = LzmaDecode(dst, &dlen, src + LZMA86_HEADER_SIZE, &inSizePure, 
		src, LZMA_PROPS_SIZE, LZMA_FINISH_ANY, &status, &g_Alloc);
	//diag_printf("\n+error=[%d]+\n",rc);
	return rc;
}

//Input parameter:unsigned char * src compressed src
int lzma_org_len(unsigned char * src)
{	
	//lzma header:LZMA properties (5 bytes) and uncompressed size (8 bytes, little-endian)
	return((src[5])|(src[6]<<8)|(src[7]<<16)|(src[8]<<24));//eCos file size can't reach 8 bytes,so we just calculate the first 4 bytes here.
}

#endif

