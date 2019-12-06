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
    cfg_net.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/
#ifndef CFG_IF_H
#define CFG_IF_H

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
//#include <network.h>
#include <sys/types.h>

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <net/if.h>
#include <cfg_net.h>

//==============================================================================
//                                    MACROS
//==============================================================================
#define NET_ERR(level, msg)	\
{	diag_printf(msg); \
	return false; \
}

// mode state
#define NODEFMODE		0
#define STATICMODE		1
#define DHCPMODE		2
#define PPPOEMODE		3
#define PPTPMODE		4
#define L2TPMODE		5
//#define DHCPPXYMODE		6
#define DUALWAN_PPPOEMODE	7
#define DUALWAN_PPTP		8

enum
{
    PPP_NO_ETH = 0,
    PPP_WAIT_ETH,
    PPP_CHK_SRV,
    PPP_GET_ETH,
    PPP_IN_BEGIN,
    PPP_IN_ACTIVE
};


#define DISCONNECTED	0
#define CONNECTED		1
#define CONNECTING		2

#define ROUTE_ADD	0
#define ROUTE_DEL	1

//#define MAX_DNS_NUM		3

#define DOMAINNAME_LEN	256
//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
struct ip_set{
	char	ifname[IFNAMSIZ];			//	which interface to bind
	unsigned int mtu;			// interface MTU
	unsigned int ip;			//	IP address
	unsigned int netmask;		//	netmask
	unsigned int gw_ip;			//	gateway address
#ifdef INET6
	struct in6_addr ip_v6;
	struct in6_addr netmask_v6;
	struct in6_addr gw_ip_v6;
#endif	
    unsigned int server_ip;		//	server address
    unsigned int broad_ip;		//	broadcast address
    unsigned char mode; 		//	which mode to get IP, ex static, dhcp, pppoe...
    unsigned char up;			//	connected/disconnectd
    unsigned short flags;
    unsigned int tunnel_gw;		//	tunnel gateway address
    unsigned int tunnel_mode;
    int 		  conn_time;    //connection time
    unsigned char domain[DOMAINNAME_LEN]; //domain name
    int               time_correct;		//     system time have been correct
    void *data;					//	point to other related data
};

#define IPSET_FG_IFCLOSED	0x0001
#define IPSET_FG_DEFROUTE	0x0020
#define IPSET_FG_NATIF		0x0040
#define IPSET_FG_FWIF		0x0080

#define IPSET_FG_IFCONFIG	0x0100
#define IPSET_FG_NATCONFIG	0x0400
#define IPSET_FG_FWCONFIG	0x0800

// Simon, 2006/02/16, moved to init
//struct dns_group{
//	unsigned int dns[MAX_DNS_NUM];
//};

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
extern cyg_bool_t net_set_maca( const char *interface, char *mac_address );
extern cyg_bool_t netif_cfg(const char *intf, struct ip_set *setp);
extern cyg_bool_t netif_del(const char *intf, struct ip_set *setp);
extern cyg_bool_t netif_flag_get(const char *intf, int *flags);
extern int netif_flag_set(const char *intf, int flags, int set);
extern cyg_bool_t RT_add_del(const char *intf, struct in_addr *dst, struct in_addr *mask, struct in_addr *gateway, int metric, int flag, int action);
extern cyg_bool_t RT_del(const char *intf, struct in_addr *dst, struct in_addr *mask, struct in_addr *gw, int metric);
extern cyg_bool_t RT_add(const char *intf, struct in_addr *dst, struct in_addr *mask, struct in_addr *gw, int metric);

extern char* netarp_get(unsigned int addr, char *outbuf, unsigned short flag);
extern char *netarp_get_one(unsigned int adr, char *buf);
extern void netarp_get_one_by_mac(unsigned int *adr, char *buf);

#endif // CFG_IF_H

