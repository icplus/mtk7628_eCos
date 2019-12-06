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
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sha1.h"


int main( int argc, char *argv[] )
{
    FILE *f;
	
	int j;
    sha1_context ctx;
    unsigned char sha1sum[20];
	
	if( argc < 2 )
    {
        printf( "\nUsage signagure <string>\n\n" );
    }
    else
    {
        sha1_starts( &ctx );
        sha1_update( &ctx, argv[1], strlen(argv[1]) );
        sha1_finish( &ctx, sha1sum );
		
		printf("\nargv[1] = %s\n", argv[1]);
		
		// Write C include file
		f = fopen("./include/dl_cookie.h", "w");
		if (f == 0)
		{
			printf("\nCannot open ./include/dl_cookie.h\n");
			return 1;
		}
		
		fprintf(f, "#ifndef	__DL_COOKIE_H__\n");
		fprintf(f, "#define __DL_COOKIE_H__\n");
		
		// Write download cookie signature
		fprintf(f, "#define	DL_COOKIE	\"");
		for (j=0; j<20; j++)
			fprintf(f, "\\x%02x", sha1sum[j]);
		fprintf(f, "\"\n");
		
		// End
		fprintf(f, "#endif\n");
		
		fclose(f);
    }
	
    return( 0 );
}

