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
    config.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#ifndef CONFIG_H
#define CONFIG_H
//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <autoconf.h>
#include <str_def.h>
//==============================================================================
//                                    MACROS
//==============================================================================

#ifdef	CONFIG_PPPOE
	#define MODULE_PPPOE	1
#else
	#define MODULE_PPPOE	0
#endif

#ifdef	CONFIG_PPTP
	#define MODULE_PPTP		1
#else
	#define MODULE_PPTP		0
#endif

#ifdef	CONFIG_L2TP
	#define MODULE_L2TP		1
#else
	#define MODULE_L2TP		0
#endif

#ifdef	CONFIG_TELNET
	#define MODULE_TELNET	1
#else
	#define MODULE_TELNET	0
#endif

#ifdef	CONFIG_NTP
	#define MODULE_NTP		1
#else
	#define MODULE_NTP		0
#endif

#ifdef	CONFIG_DHCPS
	#define MODULE_DHCPS	1
#else
	#define MODULE_DHCPS	0
#endif

#ifdef	CONFIG_UPNP
	#define MODULE_UPNP		1
#else
	#define MODULE_UPNP		0
#endif

#ifdef	CONFIG_RIP
	#define MODULE_RIP		1
#else
	#define MODULE_RIP		0
#endif

#ifdef	CONFIG_DHCPC
	#define MODULE_DHCPC	1
#else
	#define MODULE_DHCPC	0
#endif

#ifdef	CONFIG_BPA
	#define MODULE_BPA	1
#else
	#define MODULE_BPA	0
#endif

#ifdef	CONFIG_DNS
	#define MODULE_DNS		1
#else
	#define MODULE_DNS		0
#endif

#ifdef	CONFIG_DDNS
	#define MODULE_DDNS		1
#else
	#define MODULE_DDNS		0
#endif

#ifdef	CONFIG_SYSLOG
	#define MODULE_SYSLOG	1
#else
	#define MODULE_SYSLOG	0
#endif

#ifdef	CONFIG_NAT
	#define MODULE_NAT		1
#else
	#define MODULE_NAT		0
#endif

#ifdef	CONFIG_FW
	#define MODULE_FW		1
#else
	#define MODULE_FW		0
#endif

#ifdef	CONFIG_QOS
	#define MODULE_QOS		1
#else
	#define MODULE_QOS		0
#endif

#ifdef	CONFIG_HTTPD
	#define MODULE_HTTPD	1
#else
	#define MODULE_HTTPD	0
#endif

#ifdef	CONFIG_BRIDGE
	#define MODULE_BRIDGE	1
#else
	#define MODULE_BRIDGE	0
#endif

#ifdef	CONFIG_WANIF
	#define MODULE_WANIF	1
#else
	#define MODULE_WANIF	0
#endif

#ifdef	CONFIG_LANIF
	#define MODULE_LANIF	1
#else
	#define MODULE_LANIF	0
#endif

#ifdef	CONFIG_WIRELESS
	#define MODULE_WIRELESSIF	1
#else
	#define MODULE_WIRELESSIF	0
#endif

#ifdef	CONFIG_80211X_DAEMON
	#define MODULE_80211X_DAEMON	1
#else
	#define MODULE_80211X_DAEMON	0
#endif

#ifdef	CONFIG_LLTD_DAEMON
	#define MODULE_LLTD_DAEMON	1
#else
	#define MODULE_LLTD_DAEMON	0
#endif

#ifdef	CONFIG_ATE_DAEMON
	#define MODULE_ATE_DAEMON	1
#else
	#define MODULE_ATE_DAEMON	0
#endif

#ifdef	CONFIG_SNMP
	#define MODULE_SNMP		1
#else
	#define MODULE_SNMP		0
#endif

#ifdef	CONFIG_MROUTED
	#define MODULE_MROUTED		1
#else
	#define MODULE_MROUTED		0
#endif

#ifdef	CONFIG_VPN
	#define	MODULE_VPN		1
#else
	#define	MODULE_VPN		0
#endif

#if (MODULE_FW == 1)
	#ifdef	CONFIG_FW_MACF
		#define MODULE_MACF		1
	#else
		#define MODULE_MACF		0
	#endif
	#ifdef	CONFIG_FW_URLF
		#define MODULE_URLF		1
	#else
		#define MODULE_URLF		0
	#endif
#endif

#ifdef	CONFIG_AP_CLIENT
	#define	MODULE_AP_CLIENT	1
#else
	#define	MODULE_AP_CLIENT	0
#endif

#ifdef CONFIG_RALINKETH_LONG_LOOP
	#define	MODULE_ETH_LONG_LOOP	1
#else
	#define	MODULE_ETH_LONG_LOOP	0
#endif

#define CYGPKG_NET v2_0
#define CYGPKG_NET_v2_0
#define CYGNUM_NET_VERSION_MAJOR 2
#define CYGNUM_NET_VERSION_MINOR 0
#define CYGNUM_NET_VERSION_RELEASE -1
#define CYGPKG_NET_FREEBSD_STACK v3_0
#define CYGPKG_NET_FREEBSD_STACK_v3_0
#define CYGNUM_NET_FREEBSD_STACK_VERSION_MAJOR 3
#define CYGNUM_NET_FREEBSD_STACK_VERSION_MINOR 0
#define CYGNUM_NET_FREEBSD_STACK_VERSION_RELEASE -1
/***** Networking stack proc output start *****/
#define CYGDAT_NET_STACK_CFG <pkgconf/net_freebsd_stack.h>
/***** Networking stack proc output end *****/
#define CYGPKG_NS_DNS v2_0
#define CYGPKG_NS_DNS_v2_0
#define CYGNUM_NS_DNS_VERSION_MAJOR 2
#define CYGNUM_NS_DNS_VERSION_MINOR 0
#define CYGNUM_NS_DNS_VERSION_RELEASE -1

#ifdef CYGINT_ISO_BSDTYPES
#undef CYGINT_ISO_BSDTYPES
#define CYGINT_ISO_BSDTYPES 1
#define CYGINT_ISO_BSDTYPES_1
#endif
#define CYGBLD_ISO_BSDTYPES_HEADER <sys/bsdtypes.h>

#define CYGINT_ISO_DNS 1
#define CYGBLD_ISO_DNS_HEADER <dns.h>

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================

#endif	// CONFIG_H


