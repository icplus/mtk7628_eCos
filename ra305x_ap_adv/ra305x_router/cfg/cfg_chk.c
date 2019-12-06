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
    cfg_chk.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <cfg_def.h>
#include <cfg_range.h>

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================

int CFG_chk_bool(int id, void * val, void * parm)
{
    int i=*(int *)val;
    if (( i < 0 ) || ( i > 1 ))
        return -CFG_ERANGE ;
    else
        return CFG_OK ;
}

int CFG_chk_string(int id, void * val, void * parm)
{
    char * str=(char *)val;
	if ( strlen(str) > CFG_STR_MAX_SZ )
		return -CFG_ERANGE ;
	else
		return CFG_OK ;
}

int CFG_chk_passwd(int id, void * val, void * parm)
{
    return CFG_OK;
}

int CFG_chk_macaddr(int id, void * val, void * parm)
{
    return CFG_OK;
}

int CFG_chk_ipaddr(int id, void * val, void * parm)
{
    return CFG_OK;
}

int CFG_chk_netmask(int id, void * val, void * parm)
{
    return CFG_OK;
}

int CFG_chk_tcpip_port(int id, void * val, void * parm)
{
    return CFG_OK;
}

