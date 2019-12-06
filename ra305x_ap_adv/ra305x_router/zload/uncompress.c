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

#include "cfg_hdr.h"
#include "zwebmap.h"

// ---------lzma
#include "LzmaDec.h"

#define BUFZONE	0x8000
#define LZMA_PROPS_SIZE	5
#define LZMA86_SIZE_OFFSET	(LZMA_PROPS_SIZE)
#define LZMA86_HEADER_SIZE	(LZMA86_SIZE_OFFSET + 8)
// ---------lzma

//#define memzero(d, count) (memset((char*)(d), 0, (count)))

#ifndef NULL
#define NULL ((void *) 0)
#endif

/*
 * gzip declarations
 */
#define OF(args)  args
#define STATIC static

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

//#define WSIZE 0x8000		/* Window size must be at least 32k, */
				/* and a power of two */

//static uch *inbuf;		/* input buffer */
//static uch window[WSIZE];	/* Sliding window buffer */

//static unsigned insize;		/* valid bytes in inbuf */
//static unsigned inptr;		/* index of next byte to be processed in inbuf */
//static unsigned outcnt;		/* bytes in output buffer */

//#define get_byte()  (inbuf[inptr++])

/* Diagnostic functions */
#ifdef DEBUG
#  define Assert(cond,msg) {if(!(cond)) error(msg);}
#  define Trace(x) fprintf x
#  define Tracev(x) {if (verbose) fprintf x ;}
#  define Tracevv(x) {if (verbose>1) fprintf x ;}
#  define Tracec(c,x) {if (verbose && (c)) fprintf x ;}
#  define Tracecv(c,x) {if (verbose>1 && (c)) fprintf x ;}
#  define error(s)    err_str=s
#else
#  define Assert(cond,msg)
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c,x)
#  define Tracecv(c,x)
#  define error(s)
#endif

//static void flush_window(void);

//static uch *output_data;
//static ulg output_ptr;
//static ulg bytes_out;
#ifdef  DEBUG
static uch * err_str;
#endif

static void *malloc(int size);
//static void free(void *where);

// --------------lzma
static void *SzAlloc(void *p, size_t size) { p = p; return malloc(size); }
static void SzFree(void *p, void *address) { p = p; NULL; }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };
// --------------lzma

static ulg free_mem_ptr;
static ulg free_mem_ptr_end;

//void memset(char * p, int d, int len);
//void memcpy(void * to, void * from, int len );
//void putchar(int c);

#define io_putc(c) putchar(c)

//#define	gzip_mark
//#define	gzip_release
//static	inline void gzip_mark(void **p)		{return;}
//static	inline void gzip_release(void **p)	{return;}

extern char _ftext[];
extern char _fbss[];
extern char _binary_bin_gz_start[];
extern char _binary_bin_gz_end[];
extern char _webdata_start[];
extern char _webdata_end[];

fmw_hdr * fh=(fmw_hdr *) &_ftext[0];

//#include <inflate.c>
void *malloc(int size)
{
	void *p;

	//if (size <0) error("Malloc error\n");
	//if (free_mem_ptr <= 0) error("Memory error\n");

	free_mem_ptr = (free_mem_ptr + 3) & ~3;	/* Align */

	p = (void *)free_mem_ptr;
	free_mem_ptr += size;

//printf("malloc, p=%x sz=%x\n", p, size);
	//if (free_mem_ptr >= free_mem_ptr_end)
		//error("Out of memory");
	return p;
}

//void free(void *where)
//{ /* gzip_mark & gzip_release do the free */
//printf("free, p=%x\n", where);
//}
/*
void memset(char * p, int d, int len)
{
    register int i;

    for (i=0;i< len ;i++)
    {
        *p++= d & 0xff ;
    }
}*/

void memcpy(void * to, void * from, int len )
{
	register char * f=(char *)from;
	register char * t=(char *)to;
	register int i;

	for (i=0; i< len ; i++)
		t[i]=f[i];
}

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

int uncompress(char *src, int size, char*dst)
{
	int rc;
	unsigned int dlen = dst - (char *)fh - BUFZONE;
	SizeT inSizePure;
	ELzmaStatus status;

	inSizePure = size - LZMA86_HEADER_SIZE;
	rc = LzmaDecode(dst, &dlen, src + LZMA86_HEADER_SIZE, &inSizePure, 
		src, LZMA_PROPS_SIZE, LZMA_FINISH_ANY, &status, &g_Alloc);
	
	if (rc==0||rc==6)// rc==0:SZ_OK,rc==6:SZ_ERROR_INPUT_EOF
		return 1;//ok
	else	// ng , error code
		return 0;
}

#ifndef	CONFIG_HTTPD
int	ZWEB_LOC[1];
#endif

int load_image(char * buf)
{
	int rc;
	void (*jump)(void);

	extern void cache_sync_all(void);

	io_putc('Z');
	free_mem_ptr = (ulg)buf;
	free_mem_ptr_end = free_mem_ptr + 0x10000;

	jump = (void *)fh->run_loc;
	rc = uncompress((char *)&_binary_bin_gz_start[0], (ulg)&_binary_bin_gz_end[0] - (ulg)&_binary_bin_gz_start[0], (char *)jump);

	io_putc('z');
	if (!rc)
	{
		io_putc('E');
	}
	else
	{
		io_putc('O');
		io_putc('K');

		// Simon, 2006/06/12
		cache_sync_all();
		
		io_putc('\n');
		
		jump();     // else run the image
	}

	return rc;
}
#endif //UNCOMPRESS_C

