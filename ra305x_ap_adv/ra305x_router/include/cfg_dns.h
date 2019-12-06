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
    config_dns.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/
#ifndef __CFG_DNS_H__
#define __CFG_DNS_H__

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <sys/types.h>

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <cfg_net.h>


void DNS_CFG_autodns_update(int num, char *autodns);
int DNS_CFG_get_autodns(in_addr_t *dnslist, int num);
#if defined(CONFIG_PPTP_PPPOE) || defined(CONFIG_L2TP_OVER_PPPOE)
void DNS_CFG_autodns_update_append(int num, char *autodns);
#endif
in_addr_t DNS_group_get(int index);
in_addr_t DNS_dhcpd_dns_get(int index);

//DNS print level setting
#define DNS_DEBUG_OFF		0
#define DNS_DEBUG_ERROR		1

#endif // __CFG_DNS_H__


