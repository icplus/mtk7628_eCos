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
    cfg_hdr.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#ifndef CFG_HDR_H
#define CFG_HDR_H

/*
//------------------------------------------------------------------------------
// file header format
//  each element is put as a TLV format 
//  0x20,'C','F','H'
//  id1,len1,val1,id2,len2,val2..
//------------------------------------------------------------------------------
#define CFG_FH_VER      1    //  file header version
#define CFG_FH_HDR_SZ   2    //  total file header size
#define CFG_FH_DESC1    3    //  description of image1
#define CFG_FH_FMW_VER 4    // firmware version 
#define CFG_FH_FMW_SZ  4    //  firmware size
#define CFG_FH_FMW_OFF 5    //  image1 storage offset
#define CFG_FH_FMW_RUN 6    //  image1 execution location
#define CFG_FH_FMW_CMP     7    //  image1 compress method
#define CFG_FH_CKSUM1   8    //  image1 check sum
*/

typedef struct fmw_hdr_t
{
    unsigned int magic;     // j 0x40
    unsigned int magic2;    // nop
    unsigned int run_loc;   // running location ( uncompress location )
//    unsigned int flags;     // flags for image, include compress method, chksum type
struct flag_t
{
#if BYTE_ORDER == BIG_ENDIAN
	int	:2,
		chksum:3,
		comp:3,
		:24;
#else
	int	comp:3,				// compress method
		chksum:3;			// image checksum method
#endif
} flags ;
    unsigned int ver;       // version info , 255.255.255.255
    unsigned int hwinfo;    // support h/w 
    unsigned int size;      // image size (compress)
    unsigned int timestamp; // time-stamp
    unsigned int chksum;    // image chksum (compress)
    unsigned int feature;   // s/w features
    char desc[24];          // description
} fmw_hdr ;

typedef enum {
	CHKSUM_NONE  = 0,  /* no checksum */
	CHKSUM_SUM   = 1,  /* 32bit sum checksum */
	CHKSUM_CRC32 = 2,  /* CRC-32 */
	CHKSUM_MD5 = 3,  /* MD5 sum */
} CHKSUM_t;

typedef enum {
	COMP_NONE  = 0,  /* no compression */
	COMP_GZIP = 1,  /* gzip */
} COMP_t;



#endif /* CFG_HDR_H */

