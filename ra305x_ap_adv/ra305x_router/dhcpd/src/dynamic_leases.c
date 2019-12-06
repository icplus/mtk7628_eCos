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
#include <unistd.h>
#include <network.h>
#include <arpa/inet.h>

#include <string.h>
#include <stdlib.h>

#include <lib_util.h>
#include <lib_packet.h>
#include <cfg_def.h>
#include <cfg_net.h>

#include <leases.h>


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
static void dleases2cfg(char *buf, int id)
{
    CFG_set(CFG_LAN_DHCPD_DLEASE+id, buf);
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
void DHCPD_WriteDynLeases(void)
{
    char lease_str[100];
    int oldnum=0, newnum;

    while (CFG_get(CFG_LAN_DHCPD_DLEASE+oldnum+1, lease_str) != -1)
        oldnum++;

    newnum = DHCPD_dumplease(dleases2cfg, SHOW_DYNAMIC);

    while (newnum < oldnum)
    {
        CFG_del(CFG_LAN_DHCPD_DLEASE+oldnum);
        oldnum--;
    }

    CFG_commit(CFG_NO_EVENT);   //Would it better? do'nt save immediately?
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
void DHCPD_LoadDynLeases(void)
{
    int i=0;
    char lease_str[100];
    char *arglists[5];
    struct in_addr sa;
    char mac[6];

    while(++i && (CFG_get(CFG_LAN_DHCPD_DLEASE+i, lease_str) != -1))
    {
        if (str2arglist(lease_str, arglists, ';', 5) != 5)
            continue;

        inet_aton(arglists[1], &sa);
        ether_aton_r(arglists[2], mac);

        if (DHCPD_AddLease(mac, sa, atoi(arglists[4]), arglists[0], 1) == NULL)
        {
            diag_printf("no space to add lease\n");
            return;
        }
    }

    return;
}

