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
    dns_config.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <config.h>
#include <cyg/kernel/kapi.h>

#include <pkgconf/system.h>
#ifdef CYGBLD_DEVS_ETH_DEVICE_H    // Get the device config if it exists
#include CYGBLD_DEVS_ETH_DEVICE_H
#endif

#include <network.h>
#include <cfg_id.h>
#include <cfg_def.h>
#include <cfg_net.h>
#include <cfg_dns.h>
#include <sys_status.h>

//********************
// DNS options
//********************
#ifdef	CONFIG_DNS

#if	defined(CONFIG_DNS_CACHE_NUM)
int	dns_cache_size = CONFIG_DNS_CACHE_NUM;
#else
int	dns_cache_size = 100;
#endif

#endif

//**************************
// Configuration functions
//**************************
#if	!defined(CONFIG_AUTO_DNS_NUM)
#define	CONFIG_AUTO_DNS_NUM		3
#endif

#if	!defined(CONFIG_STATIC_DNS_NUM)
#define	CONFIG_STATIC_DNS_NUM	3
#endif

#if defined(CONFIG_PPTP_PPPOE) || defined(CONFIG_L2TP_OVER_PPPOE)
#define	CONFIG_OPPPOE_VPN_DNS_NUM 2
#else
#define	CONFIG_OPPPOE_VPN_DNS_NUM 0
#endif
#define	MAX_DNS_NUM	(CONFIG_AUTO_DNS_NUM + CONFIG_STATIC_DNS_NUM)

static	in_addr_t	wandnsg[CONFIG_AUTO_DNS_NUM + CONFIG_OPPPOE_VPN_DNS_NUM];
static	in_addr_t	staticdnsg[CONFIG_STATIC_DNS_NUM];

static	int             num_dnsg;
static	in_addr_t	dnsg[MAX_DNS_NUM];

#ifdef CYGPKG_NET_INET6
struct in6_addr dnsg6[MAX_DNS_NUM];
#endif 
int DNSDebugLevel = DNS_DEBUG_OFF;

//------------------------------------------------------------------------------
//
// DESCRIPTION
//		set DNS server automatically get from DHCP/PPPoE
//  
// PARAMETERS
//		num: number of dns
//		dns: DNS server	IP
//  
// RETURN
//		NONE
//  
//------------------------------------------------------------------------------
void DNS_CFG_autodns_update(int num, char *autodns)
{
	memset(wandnsg, 0, sizeof(wandnsg));
	
	if (num > CONFIG_AUTO_DNS_NUM)
		num = CONFIG_AUTO_DNS_NUM;
	
	if (num)
		memcpy(wandnsg, autodns, num*4);
}

#if defined(CONFIG_PPTP_PPPOE) || defined(CONFIG_L2TP_OVER_PPPOE)

void DNS_CFG_autodns_update_append(int num, char *autodns)
{
	if (num > CONFIG_OPPPOE_VPN_DNS_NUM)
		num = CONFIG_OPPPOE_VPN_DNS_NUM;
	
	if (num)
		memcpy(&wandnsg[CONFIG_AUTO_DNS_NUM], autodns, num*4);
}
#endif
#if defined(CONFIG_PPTP_PPPOE) || defined(CONFIG_L2TP_OVER_PPPOE)
int DNS_CFG_get_autodns(in_addr_t *dnslist, int num)
{
	int cnt, i, cp;

	cnt = (num > CONFIG_AUTO_DNS_NUM + CONFIG_OPPPOE_VPN_DNS_NUM) ? (CONFIG_AUTO_DNS_NUM + CONFIG_OPPPOE_VPN_DNS_NUM) : num;
	for (i=0, cp=0; i<cnt; i++) {
		if (wandnsg[i] == 0)
			continue;//break;
		dnslist[cp] = wandnsg[i];
		cp ++;
	}

	return cp;
}
#else
int DNS_CFG_get_autodns(in_addr_t *dnslist, int num)
{
	int cnt, i, cp;

	cnt = (num > CONFIG_AUTO_DNS_NUM + CONFIG_OPPPOE_VPN_DNS_NUM) ? (CONFIG_AUTO_DNS_NUM + CONFIG_OPPPOE_VPN_DNS_NUM) : num;
	for (i=0, cp=0; i<cnt; i++) {
		if (wandnsg[i] == 0)
			break;
		dnslist[i] = wandnsg[i];
		cp ++;
	}

	return cp;
}

#endif

//------------------------------------------------------------------------------
// FUNCTION
//
// DESCRIPTION
//		retrieve DNS server form setting
//  
// PARAMETERS
//		idx: index which DNS server to retrieve
//  
// RETURN
//		DNS server IP
//  
//------------------------------------------------------------------------------
in_addr_t DNS_group_get(int index)
{
	if (index >= MAX_DNS_NUM)
		return 0;
	
	return dnsg[index];
}

#ifdef	CONFIG_DHCPS
in_addr_t DNS_dhcpd_dns_get(int index)
{
#ifdef	CONFIG_DNS
	int val;
	
	CFG_get(CFG_DNS_EN, &val);
	if (val == 1)
	{
		// Return DNS masquerate IP
		if (index == 0)
			return SYS_lan_ip;
	}
	else
#endif
	{
		// Pass DNS to dhcpd
		if (index < MAX_DNS_NUM)
		{
			return dnsg[index];
		}
	}
	
	return 0;
}
#endif

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//		set DNS server by priority
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
static void load_dns(in_addr_t *server, int num)
{
	int i, j;
	in_addr_t dns;
	
	for (i=0; i<num; i++)
	{
		dns = server[i];
		if (dns == 0)
			continue;//break;//haitao for pptp_pppoe
		
		// Simon patch temporary
		if (dns == SYS_lan_ip
#ifdef	CONFIG_WANIF
			|| dns == SYS_wan_ip
#endif
			)
			continue;
		
		// Search duplicate
		for (j=0; j<num_dnsg; j++)
		{
			if (dnsg[j] == dns)
				break;
		}
		
		// load if it is a new one
		if (j==num_dnsg)
		{
			dnsg[num_dnsg] = dns;
			num_dnsg++;
		}
	}
}

void DNS_config(void)
{
	int i, j;
	in_addr_t dns;
	
	int static_enabled = 0;
	int static_first = 0;
	in_addr_t def_dns = 0;
	//in_addr_t builtin_dns = inet_addr("8.8.8.8");

	
	// Clear table
	num_dnsg = 0;
	memset(dnsg, 0, sizeof(dnsg));
	memset(staticdnsg, 0, sizeof(staticdnsg));
	
	// Read static table
	CFG_get( CFG_DNS_FIX, &static_enabled);
	if (static_enabled)
	{
		for (i=0, j=0; j<CONFIG_STATIC_DNS_NUM; i++)
		{
			// Search configuration data base until not found
			if (CFG_get(CFG_DNS_SVR+i+1, &dns) == -1)
				break;
			
			if (dns != 0)
			{
				staticdnsg[j] = dns;
				j++;
			}
		}
	}
	
	//
	// Update dynamic table and static table
	//
	CFG_get(CFG_DNS_FIX_FIRST, &static_first);
	if (static_first)
	{
		load_dns(staticdnsg, CONFIG_STATIC_DNS_NUM);
		load_dns(wandnsg, (CONFIG_AUTO_DNS_NUM + CONFIG_OPPPOE_VPN_DNS_NUM));
	}
	else
	{
		load_dns(wandnsg, (CONFIG_AUTO_DNS_NUM + CONFIG_OPPPOE_VPN_DNS_NUM));
		load_dns(staticdnsg, CONFIG_STATIC_DNS_NUM);
	}
	
	//
	// Set default if not found
	//
	CFG_get(CFG_DNS_DEF, &def_dns);
	
	if (num_dnsg == 0)
	{
		if (def_dns)
			dnsg[0] = def_dns;
		/*
		else
			dnsg[0] = builtin_dns;
			*/
		
		num_dnsg++;
	}
/*	else
		// Update default dns
		if (wandnsg[0] != 0 && wandnsg[0] != def_dns && 
			wandnsg[0] != builtin_dns)
		//if (wandnsg[0] != 0)
		{
			CFG_set_no_event(CFG_DNS_DEF, &wandnsg[0]);
			CFG_commit(0);
		}
	}*/
}

void DNS_server_show(void)
{
	int i;
	struct in_addr dns;
	
	for (i = 0; i < num_dnsg; i++)
	{
		dns.s_addr = dnsg[i];
		diag_printf("DNS server %d: %s\n", i, inet_ntoa(dns));
	}
}

void DNS_debug_level(int level)
{
    if (level > 0)
        DNSDebugLevel = DNS_DEBUG_ERROR;
    else
        DNSDebugLevel = DNS_DEBUG_OFF;

    diag_printf("DNS debug level = %d\n", DNSDebugLevel);
}



