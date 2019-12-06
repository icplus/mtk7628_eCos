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
    lib_util.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/
#ifndef	__LIB_UTIL_H__
#define	__LIB_UTIL_H__

#include <time.h>

int str2arglist(char *buf, char *list[], char c, int max);
//time_t GMTtime(int flag);
//time_t GMTtime_offset(int flag);

//struct ether_addr;
//char *ether_ntoa(struct ether_addr *e);

#endif	//__LIB_UTIL_H__
