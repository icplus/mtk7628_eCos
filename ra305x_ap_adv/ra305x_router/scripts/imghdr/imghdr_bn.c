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
 ***************************************************************************/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>

#include "cfg_hdr.h"
//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
	
//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================



//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================
static void usage();
static int build_hdr( char *infile, char *outfile, char *vstr, char *btstr, char *infostr);
//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================



//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	char *infile, *outfile, *info;

	if (argc != 6) {
		usage();
	}
	infile = argv[1];
	outfile = argv[2];

	if (build_hdr( infile, outfile, argv[3], argv[4], argv[5]) < 0) {
		return -1;
	}
	return 0;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
static void usage()
{
	fprintf(stderr, "usage: imghdr infile outfile ver buildtime info\n");
	exit(2);
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
//  
//------------------------------------------------------------------------------

static int build_hdr( char *infile, char *outfile, char *vstr, char *btstr, char *info)
{
	FILE	*ifp=0, *ofp=0;
	struct stat st;
	int rc;
	int len;
	char *fbuf=0;
	unsigned int chksum;
	unsigned int data;
	int i;
	fmw_hdr *phdr;
	unsigned int ver=0;
	unsigned int bt=0;

	if (rc=stat(infile, &st))
	{
		printf("stat err =%d, %s\n", rc,strerror(rc));
		goto bad;	
	}

	if (st.st_size==0)
	{
		printf("infile(%s) size=0\n", infile);
		goto bad;
	}

	/* open source file to read */
	if ((ifp = fopen(infile, "r")) == NULL ) {
		fprintf(stderr, "err: fopen(%s)\n", infile);
		goto bad;
	}

	if ((ofp = fopen(outfile, "w")) == NULL ) {
		fprintf(stderr, "err: fopen(%s)\n", outfile);
		goto bad;
	}

	len=(int)st.st_size;
	printf("Image size=%d (0x%x)\n",len, len);

	if ((fbuf=(char*) malloc( len+256 ))==0)
	{
		printf("malloc err\n");
		goto bad;	
	}
	memset(fbuf, 0, len+256);

	if ( (rc=fread(fbuf, len, 1,  ifp)) != 1 ) {
		printf("fread %s errr: %d\n",infile, rc);
		goto bad;
	}
	len=(len+3)&~3;

	sscanf(vstr,"%x", &ver);
	ver=ntohl(ver);
	sscanf(btstr,"%u", &bt);

	phdr=(fmw_hdr *)fbuf;

//printf("ver=%08x bt=%08x info=%-24s\n", ver, bt, info);
	strncpy(phdr->desc, info, sizeof(phdr->desc));
	phdr->desc[sizeof(phdr->desc)-1]=0;

	phdr->ver=ntohl(ver);
	phdr->timestamp=ntohl(bt);
	phdr->size=ntohl(len);
	phdr->flags.comp=COMP_GZIP;
	phdr->flags.chksum=CHKSUM_SUM;

	phdr->chksum=chksum=0;
	
	for (i=0; i<len; i+=4)
	{
		memcpy(&data, fbuf+i, 4);
		chksum+=ntohl(data);
	}
	
	chksum ^= 0xffffffff;
	chksum++;
	
	phdr->chksum=ntohl(chksum);
	
	printf("chksum=0x%08X\n", phdr->chksum);
	
	if ( (rc=fwrite(fbuf,len, 1,ofp)) != 1 ) {
		printf("fwrtie %s errr: %d\n",outfile, rc);
		goto bad;
	}

bad:
	if (fbuf)
		free(fbuf);

	if (ifp)
		fclose(ifp);

	if (ofp)
		fclose(ofp);

	return -1;
}

