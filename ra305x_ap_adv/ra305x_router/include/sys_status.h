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
    sys_status.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#ifndef	SYS_STATUS_H
#define SYS_STATUS_H
//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <config.h>
#include <cfg_net.h>

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
extern struct ip_set primary_lan_ip_set;
#define PRIMARY_WANIP_NUM		2
extern struct ip_set primary_wan_ip_set[PRIMARY_WANIP_NUM];
#define WAN_IPALIAS_NUM 	10
extern struct ip_set alias_wan_ip_set[WAN_IPALIAS_NUM];

//==============================================================================
//                                    MACROS
//==============================================================================
// LAN status
#ifndef IFNAMSIZ
#define IFNAMSIZ			16
#endif


extern char lan_name[IFNAMSIZ];
#define LAN_NAME			lan_name

#define SYS_lan_if			(primary_lan_ip_set.ifname)
#define SYS_lan_ip			(primary_lan_ip_set.ip)
#define SYS_lan_mask		(primary_lan_ip_set.netmask)
#define SYS_lan_gw			(primary_lan_ip_set.gw_ip)
#define SYS_lan_brstip		(primary_lan_ip_set.broad_ip)
#define SYS_lan_mode		(primary_lan_ip_set.mode)
#define SYS_lan_up			(primary_lan_ip_set.up)

// WAN status
extern char wan_name[IFNAMSIZ];
#define WAN_NAME			wan_name
		
#define SYS_wan_if			(primary_wan_ip_set[0].ifname)		
#define SYS_wan_ip			(primary_wan_ip_set[0].ip)
#define SYS_wan_mask		(primary_wan_ip_set[0].netmask)
#define SYS_wan_gw			(primary_wan_ip_set[0].gw_ip)
#define SYS_wan_brstip		(primary_lan_ip_set[0].broad_ip)
#define SYS_wan_mode		(primary_wan_ip_set[0].mode)
#define SYS_wan_up			(primary_wan_ip_set[0].up)
#define SYS_wan_data		(primary_wan_ip_set[0].data)
#define SYS_wan_domain		(primary_wan_ip_set[0].domain)
#define SYS_wan_conntime    (primary_wan_ip_set[0].conn_time)
#define SYS_wan_mtu			(primary_wan_ip_set[0].mtu)
#define SYS_wan_serverip	(primary_wan_ip_set[0].server_ip)

// DNS status
#define SYS_dns_1			DNS_group_get(0)
#define SYS_dns_2			DNS_group_get(1)

//System time correct
#define SYS_time_correct		primary_wan_ip_set[0].time_correct

#define SYS_lan_mac         (net_get_maca(LAN_NAME))
#define SYS_wan_mac         (net_get_maca(WAN_NAME))
#define NSTR(x)			inet_ntoa(*(struct in_addr *)&x)
#define ESTR(x)			ether_ntoa((struct ether_addr *)x)
//DHCPD
#define SYS_dhcpd_leases

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

#endif //SYS_STATUS_H


