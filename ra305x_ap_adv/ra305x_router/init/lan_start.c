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
    lan_start.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

//
//	LAN setup
//
//
#include <stdio.h>
#include <pkgconf/system.h>
#include <pkgconf/net.h>
#include <network.h>
#include <arpa/inet.h>

#include <config.h>
#include <cfg_net.h>
#include <cfg_def.h>
#include <sys_status.h>
#include <dhcpc.h>

#include <cyg/io/eth/netdev.h>  // For struct eth_drv_sc
#include <cyg/io/eth/eth_drv.h> // For eth_drv_netde

#define	MY_ADDRS_IP 	 	192.168.1.228
#define	MY_ADDRS_NETMASK 	255.255.255.0
#define	MY_ADDRS_BROADCAST 	192.168.1.255
#define MY_ADDRS_GATEWAY 	192.168.1.253
#define MY_ADDRS_SERVER 	192.168.1.129

#define _string(s) #s
#define	STR(s)	_string(s)

extern void ra305x_restart(void);
extern void ra305x_close(void);

extern void long_loop_start();
extern void long_loop_stop();


#ifdef CONFIG_WIRELESS
#ifdef CONFIG_AP_SUPPORT
extern int rt28xx_ap_ioctl(
	struct eth_drv_sc	*pEndDev, 
	int			cmd, 
	caddr_t		pData);
#endif

#ifdef CONFIG_STA_SUPPORT
extern int rt28xx_sta_ioctl(
	struct eth_drv_sc	*pEndDev, 
	int			cmd, 
	caddr_t		pData);
#endif

//#define SIOCIWFIRSTPRIV 0x00
#define SIOCIWFIRSTPRIV	0x8BE0

#define RT_PRIV_IOCTL								(SIOCIWFIRSTPRIV + 0x01)
#define RTPRIV_IOCTL_SET							(SIOCIWFIRSTPRIV + 0x02)

struct iw_point
{
	void            *pointer;
	unsigned short  length;
	unsigned short  flags;
};
	
union iwreq_data
{
	struct iw_point data;
};

struct iwreq {
	union
	{
		char    ifrn_name[IFNAMSIZ];    /* if name, e.g. "eth0" */
	} ifr_ifrn;
	
	union   iwreq_data      u;
};
#endif // CONFIG_WIRELESS //

struct ip_set primary_lan_ip_set, dhcp_ip_set, lo_ip_set;
cyg_bool_t   lan_up = false;
char lan_name[IFNAMSIZ];
char lo_name[IFNAMSIZ]="lo0";

cyg_uint8 lan_dhcpcstate = 0;

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
extern void DHCPC_init(char *intf, struct ip_set *setp, cyg_uint8 *state , unsigned int req_ip);
extern void DHCPC_renew(const char *intf);
extern void DHCPC_free( char *intf);
extern void DHCPC_start_dhcp_mgt_thread(void);

extern void buid_ip_set(struct ip_set *setp,
                 const char *if_name,
                 unsigned int addrs_ip,
                 unsigned int addrs_netmask,
                 unsigned int addrs_broadcast,
                 unsigned int addrs_gateway,
                 unsigned int addrs_server,
                 unsigned int mtu,
                 unsigned char	mode,
                 const char *domain,
                 unsigned char	*data);

void RT_ifp_flush(char *ifname);
void RT_ifa_flush(unsigned int ipaddr);

extern cyg_bool_t netif_down(const char *intf);
extern cyg_bool_t netif_up(const char *intf);

extern void show_ip_set(struct ip_set *setp);

extern int ifr_link_cmd(char * ifn, int cmd, void *data);
extern int ifr_link_set(char * ifn, int mode);

int lan_set_phy(void)
{
    int mode=0;

	CFG_get( CFG_LAN_PHY, &mode ); 

	if (mode < 5) {
		return ifr_link_set(LAN_NAME, mode);
	}
	return -1;
}

#if MODULE_80211X_DAEMON
extern void Dot1x_Reboot(void);
#endif // MODULE_80211X_DAEMON //

//------------------------------------------------------------------------------
// FUNCTION
//		int lan_set_ip()
//
// DESCRIPTION
//  	This function was called by LAN_start(1) and do set WAN interface by "WAN_IPMODE"
//		LAN_IPMODE: 0 - NO defined, can use to disable WAN
//					1 - static 
//					2 - DHCP clinet
//
// PARAMETERS
//		NONE
//  
// RETURN
//		NONE
//  
//------------------------------------------------------------------------------
int lan_set_ip(void)
{
	char ipa[20];
	char mask[20];
	char gw[20];

#ifdef	CONFIG_DHCPC
    int ipmode;
    
    CFG_get( CFG_LAN_IP_MODE, &ipmode);
	
    if (ipmode == 0) //use_dhcp
    {
		
        // Perform a complete initialization, using BOOTP/DHCP
        lan_up = true;
        if (lan_dhcpcstate != DHCPSTATE_BOUND)
		{
    		diag_printf("Getting IP from DHCP server, please wait...\n");
    		DHCPC_init( LAN_NAME, &dhcp_ip_set, &lan_dhcpcstate, 0);
    		DHCPC_renew(LAN_NAME);
            
            lan_up = false;
	    primary_lan_ip_set.up = DISCONNECTED;
        }
        else
		{
        	memcpy(&primary_lan_ip_set, &dhcp_ip_set, sizeof(struct ip_set));
		primary_lan_ip_set.up = CONNECTED;
        }
        
    }
    else
#endif
    {
		lan_up = true;
	
		CFG_get_str( CFG_LAN_IP, ipa);
		CFG_get_str( CFG_LAN_MSK, mask);
		CFG_get_str( CFG_LAN_GW, gw);
	
		buid_ip_set(&primary_lan_ip_set, LAN_NAME, inet_addr(ipa), inet_addr(mask)
			, 0, inet_addr(gw) , 0, 0, STATICMODE, NULL, NULL );
#ifdef INET6
		{
			char ipa_v6[40];
			char mask_v6[40];

			CFG_get_str( CFG_LAN_IP_v6, ipa_v6);
			CFG_get_str( CFG_LAN_MSK_v6, mask_v6);

			diag_printf("LAN AF_INET6, %s\n", ipa_v6);
			inet_pton(AF_INET6, &ipa_v6[0], &primary_lan_ip_set.ip_v6);
			inet_pton(AF_INET6, &mask_v6[0], &primary_lan_ip_set.netmask_v6);
		}
#endif
		primary_lan_ip_set.up = CONNECTED;
	
    } // not use dhcp

    if (lan_up)
    {    
    	cyg_thread_delay(50); //without this, can not set interface??
    	if (!netif_cfg(LAN_NAME, &primary_lan_ip_set))
    	 	lan_up = false; 
    }

    if ( lan_up == true )
	{
    	show_ip_set(&primary_lan_ip_set);
    	diag_printf("interface %s init_net successed!\n", LAN_NAME);
#ifdef RALINK_1X_SUPPORT
	Dot1x_Reboot();
#endif //RALINK_1X_SUPPORT
    }
    else
	{
    	diag_printf("Network initialization failed for %s\n",LAN_NAME );
    }
	
	return 0;
}


int lan_del_ip(void)
{
#ifdef	CONFIG_DHCPC
	int ipmode;
	
	CFG_get( CFG_LAN_IP_MODE, &ipmode);
    
    if (ipmode!=0) //use_dhcp
    	DHCPC_free(LAN_NAME); // if no dhcpc mode, free dhcpc data
#endif
    	
    if (!netif_del(LAN_NAME, NULL))
        printf("lan_del_ip failed\n");
	
    RT_ifp_flush(LAN_NAME);
    RT_ifa_flush(primary_lan_ip_set.ip);
	
	return 0;
}

extern  opmode;
int LAN_start(int cmd)
{
	if (strcmp(LAN_NAME, "") == 0)
		return -1;
		
    switch (cmd)
    {
	case 0:
#if	(MODULE_ETH_LONG_LOOP == 1)		
		long_loop_stop();
#endif
		lan_del_ip();
		netif_down(LAN_NAME);
		if ((opmode == 2) || (opmode == 3))
			netif_down(CONFIG_DEFAULT_WAN_NAME);
			//ra305x_close();	
		break;
		
	case 2:
		LAN_start(0);
		LAN_start(1);
		break;

	case 1:
	default:
		ra305x_close();//eth hardware reset
		ra305x_restart();
		lan_set_phy();
		lan_set_ip();
		netif_up(LAN_NAME);
		if ((opmode == 2) || (opmode == 3)) {
			int mode=0;
		    
			CFG_get( CFG_WAN_PHY, &mode ); 
			if (mode < 5) {
				ifr_link_set(CONFIG_DEFAULT_WAN_NAME, mode);
			}			
			netif_up(CONFIG_DEFAULT_WAN_NAME);
		}
#if	(MODULE_ETH_LONG_LOOP == 1)		
		long_loop_start();
#endif
		break;
    }
	
	return 0;
}

#ifdef CONFIG_WIRELESS
int Wireless_down()
{
	char str[16];
	char ifName[16];
	char *pEnd;
	int value = 0;
	int i;
	cyg_bool_t result;

	//Apcli
	CFG_get_str(CFG_WLN_ApCliEnable, str);
	value = strtol(str, &pEnd, 10);
	if (value)
	{
		sprintf(ifName, "apcli0");
		result = netif_down(ifName);
	}

	//Wds
	CFG_get_str(CFG_WLN_WdsEnable, str);
	value = strtol(str, &pEnd, 10);
	if (value)
	{
		for (i = 0; i < 4; i++)
		{
			sprintf(ifName, "wds%d", i);
			result = netif_down(ifName);
		}
	}

	//Multiple BSS    
	CFG_get_str(CFG_WLN_BssidNum, str);
	value = strtol(str, &pEnd, 10);
	if(value > 16) return;
	for (i = 1; i < 4; i++)//make sure all mbss can be down
	{
		sprintf(ifName, "ra%d", i);
		result = netif_down(ifName);
	}
	
	result = netif_down("ra0");
	if (!result)
		return -1;
	
	return 0;
}

extern int opmode;
int Wireless_start()
{
	char str[20];
	char ifName[IFNAMSIZ];
    char *pEnd;
    int value = 0;
    int i;
    cyg_bool_t result;
    cyg_netdevtab_entry_t *t;
    struct eth_drv_sc *sc;
    struct iwreq param;
    int debugMode = 0;    
#ifdef CONFIG_BRIDGE
	char temp_cfg[128];
	extern char bridge_cfg[128];
#endif
    
	result = netif_up("ra0");
    if (!result)
                return -1;
    
#ifdef CONFIG_BRIDGE
	sprintf(temp_cfg, "%s, %s", bridge_cfg, "ra0");
	sprintf(bridge_cfg, "%s", temp_cfg);
#endif

    //Multiple BSS    
    CFG_get_str(CFG_WLN_BssidNum, str);
    value = strtol(str, &pEnd, 10);
	if(value > 16) return -1;
    for (i = 1; i < value; i++)
    {
        sprintf(ifName, "ra%d", i);
        result = netif_up(ifName);
#ifdef CONFIG_BRIDGE
	if (result) {
		sprintf(temp_cfg, "%s, ra%d", bridge_cfg, i);
		sprintf(bridge_cfg, "%s", temp_cfg);
	}
#endif
    }

    //Apcli
    CFG_get_str(CFG_WLN_ApCliEnable, str);
    value = strtol(str, &pEnd, 10);
    if (value)
    {
        sprintf(ifName, "apcli0");
        result = netif_up(ifName);
#ifdef CONFIG_BRIDGE
    if ((result) && ((opmode == 1) || (opmode == 3))) {
			sprintf(temp_cfg, "%s, %s", bridge_cfg, "apcli0");
			sprintf(bridge_cfg, "%s", temp_cfg);
	}
#endif
    }

    //Wds
    CFG_get_str(CFG_WLN_WdsEnable, str);
    value = strtol(str, &pEnd, 10);
    if (value)
    {
        for (i = 0; i < 4; i++)
        {
            sprintf(ifName, "wds%d", i);
            result = netif_up(ifName);
#ifdef CONFIG_BRIDGE
	if (result) {
			sprintf(temp_cfg, "%s wds%d", bridge_cfg, i);
			sprintf(bridge_cfg, "%s", temp_cfg);
	}								 
#endif
        }
    }

    //Debug Mode
    CFG_get_str(CFG_LOG_MODE, str);
    value = strtol(str, &pEnd, 10);
    if  (value & 0x02)
        debugMode = 3;
    else
        debugMode = 1;

    sprintf(ifName, "Debug=%d", debugMode);
	param.u.data.pointer = ifName;
	param.u.data.length = strlen(ifName);    
	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;
		if (strcmp(sc->dev_name, "ra0") == 0) {
#ifdef CONFIG_AP_SUPPORT			
    		rt28xx_ap_ioctl(sc, RTPRIV_IOCTL_SET, (caddr_t)&param);
#endif
#ifdef CONFIG_STA_SUPPORT			
			rt28xx_sta_ioctl(sc, RTPRIV_IOCTL_SET, (caddr_t)&param);
#endif

			break;
        }
	}

	return 0;
}

int WLAN_start(int cmd)
{
	switch (cmd)
	{
		case 0:
			Wireless_down();
			break;

		case 2:
			Wireless_down();
			Wireless_start();
			break;

		case 1:
		default:
			Wireless_start();
			break;
	}

	return 0;
}
#endif // CONFIG_WIRELESS //

int LAN_show(void)
{
	return 0;
}


void lo_set_ip(void)
{
	buid_ip_set(&lo_ip_set, lo_name, inet_addr("127.0.0.1"), inet_addr("255.0.0.0")
			, 0, 0 , 0, 0, STATICMODE, NULL, NULL );
			
	netif_cfg(lo_name, &lo_ip_set);
}

