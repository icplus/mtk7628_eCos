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
    wan_start.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/


//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <config.h>
#include <pkgconf/system.h>
#include <pkgconf/net.h>

#include <network.h>
#include <cfg_net.h>
#include <cfg_dns.h>
#include <sys_status.h>
#include <dhcpc.h>
#include <cfg_def.h>
#include <eventlog.h>
#ifdef INET6
#include <netinet6/nd6.h>
#endif

//==============================================================================
//                                    MACROS
//==============================================================================
#define WAN_DBG		diag_printf
#define WAN_log		diag_printf

#define IFMON_CMD_FULLUP		0
#define IFMON_CMD_FULLDOWN		1
#define IFMON_CMD_PRIMARY_DOWN		2

#define PPPOE_START_DELAY			10
#define PPTP_START_DELAY			15
#define PPTP_STOP_DELAY			30
#define L2TP_START_DELAY			15
#define L2TP_STOP_DELAY			30


//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
typedef struct dhcpc_ctx_s {
	struct ip_set *pipset;
	struct ip_set tmp_ipset;
	cyg_uint16 ipset_flags;
	cyg_uint8 state;
} dhcpc_ctx_t;

typedef struct static_ctx_s {
	struct ip_set *pipset;
	cyg_uint32 ipa;
	cyg_uint32 mask;
	cyg_uint32 gw;
	cyg_uint32 mtu;
	cyg_uint16 ipset_flags;
#ifdef INET6
	struct in6_addr ipa_v6;
	struct in6_addr mask_v6;
	struct in6_addr gw_v6;
#endif	
} static_ctx_t;

typedef struct ppp_ctx_s {
	struct ip_set *pipset;
	cyg_uint16 ipset_flags;
	cyg_uint8 state;
	cyg_uint8 staticip;
	cyg_uint32 conn_mode;
	cyg_uint32 prev_dns[2];
} ppp_ctx_t;

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
struct ip_set primary_wan_ip_set[PRIMARY_WANIP_NUM], wan_ip_set;
struct ip_set alias_wan_ip_set[WAN_IPALIAS_NUM]={{{0}}};

char wan_name[IFNAMSIZ];
struct in_addr pptp_srverip;

#if (MODULE_DHCPC == 1)
static dhcpc_ctx_t dhcpc_ctl = {pipset: NULL, state: DHCPSTATE_STOPPED};
#endif

static static_ctx_t static_ctl = {pipset: NULL};

static ppp_ctx_t ppp_ctl= {pipset: NULL, state: PPP_NO_ETH};
static int wanmode = NODEFMODE;

extern bool ppp_up_flag;

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================
int wan_set_ip(void);
int wan_del_ip(int cmd);
void WAN_MAC_clone(char *mac, int set);


//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
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
				 
extern int IfLib_IpDetach(char *pIfName);
extern void show_ip_set(struct ip_set *setp);

#if (MODULE_DHCPC == 1)
extern void DHCPC_init(char *intf, struct ip_set *setp, unsigned int req_ip);
extern void DHCPC_renew(const char *intf);
extern void DHCPC_free( char *intf);
extern void DHCPC_stop_DNS_update(const char *intf);
extern void DHCPC_start_DNS_update(const char *intf);
extern unsigned char DHCPC_state(const char *intf);
#ifdef	CONFIG_BPA
extern int bpa_start(char *username, char *password, char *server_ip);
extern void bpa_stop(void);
extern int bpa_run(void);
#endif	/*  CONFIG_BPA  */
#endif	/*  (MODULE_DHCPC == 1)  */

#if (MODULE_PPPOE == 1)
extern unsigned long ng_pppoe_doConnect(cyg_uint8 *);
extern void PPPOE_down(int);
extern void pppoe_cleanup(void);
#endif	/*  (MODULE_PPPOE == 1)  */

#if (MODULE_PPTP == 1)
extern void PPTP_down(void);
extern int PPTP_doConnect(struct ip_set *);
#endif	/*  (MODULE_PPTP == 1)  */

#if (MODULE_L2TP == 1)
extern void L2tp_down(void);
extern int L2tp_doConnect(struct ip_set *);
#ifdef CONFIG_L2TP_OVER_PPPOE
int l2tp_over_pppoe_flag = 0;
extern int l2tp_over_pppoe_mon_down(void);
#endif
#endif	/*  (MODULE_L2TP == 1)  */

extern cyg_bool_t netif_down(const char *intf);
extern cyg_bool_t netif_up(const char *intf);
extern cyg_bool_t netif_alias_cfg(const char *intf, struct ip_set *setp);

extern cyg_bool_t RT_add_default(const char *intf, unsigned int gw);
extern cyg_bool_t RT_del_default(const char *intf, unsigned int gw);

extern void RT_ifp_flush(char *ifname);
extern void RT_ifa_flush(unsigned int ipaddr);

extern void if_detach(struct ifnet *ifp);
#ifdef CONFIG_UPNP
extern void upnp_update_wanstatus(void);
#endif

#if MODULE_80211X_DAEMON
extern void Dot1x_Reboot(void);
#endif // MODULE_80211X_DAEMON //

//==============================================================================
//                              EXTERNAL VARIABLES
//==============================================================================
extern void *ppp_ifnet;


void config_wans(void) 
{
	int i;

	for (i=0; i<PRIMARY_WANIP_NUM; i++) {
		/*  check if the ipmode is configured and is in connected state*/
		if (primary_wan_ip_set[i].mode ==  NODEFMODE || 
				primary_wan_ip_set[i].up != CONNECTED) 
			continue;
		/*  Check if the interface is already configured  */
		if (primary_wan_ip_set[i].flags & IPSET_FG_IFCONFIG)
			continue;
		if (primary_wan_ip_set[i].ip == 0 || 
				primary_wan_ip_set[i].ip == 0xffffffff) {
			primary_wan_ip_set[i].up = DISCONNECTED;
			show_ip_set(&primary_wan_ip_set[i]);
			WAN_DBG("ERROR: prohibit all 0 or 1 as interface IP\n");
			continue;
		}
		
		if ((primary_wan_ip_set[i].ip & primary_wan_ip_set[i].netmask) == 
				(SYS_lan_ip & SYS_lan_mask)) {
#define AUTO_FIX_IP_CONFLICTS 1
#ifndef AUTO_FIX_IP_CONFLICTS
			primary_wan_ip_set[i].up = DISCONNECTED;
			show_ip_set(&primary_wan_ip_set[i]);
			WAN_DBG("ERROR: same IP as LAN\n");
			continue;
#else
#define _STEP 7
			WAN_DBG("WARNING: same IP as LAN, auto increment lan's IP address(step = %d)\n", _STEP);
			{
				char buf[4 * sizeof "123"] = {0};
				in_addr_t ip = {0};
				unsigned char *ucp = (unsigned char *)&(SYS_lan_ip);

				ucp[2] = (ucp[2] + _STEP) & 0xff;
				strncpy(buf, inet_ntoa(*(struct in_addr *)&SYS_lan_ip), sizeof(buf));

				WAN_DBG("DEBUG: Updateing new lan ip = %s\n", buf);
				CFG_set_str(CFG_LAN_IP, buf);
				CFG_commit(CFG_LAN_IP | CFG_COM_SAVE);
				CFG_set_str(CFG_LAN_GW, buf);
				CFG_commit(CFG_LAN_GW | CFG_COM_SAVE);
#if 0
				/* It seems that Ralink eCos will automatically update the dhcpd information */
				CFG_get_str(CFG_LAN_DHCPD_START, buf);
				ip = inet_addr(buf);
				ucp = (unsigned char *)&ip;
				ucp[2] = (ucp[2] + _STEP) & 0xff;
				CFG_set_str(CFG_LAN_DHCPD_START, inet_ntoa(*(struct in_addr *)&ip));
				WAN_DBG("DEBUG: lan dhcpd start = %s\n", inet_ntoa(*(struct in_addr *)&ip));

				CFG_get_str(CFG_LAN_DHCPD_END, buf);
				ip = inet_addr(buf);
				ucp = (unsigned char *)&ip;
				ucp[2] = (ucp[2] + _STEP) & 0xff;
				CFG_set_str(CFG_LAN_DHCPD_END, inet_ntoa(*(struct in_addr *)&ip));
				WAN_DBG("DEBUG: lan dhcpd end = %s\n", inet_ntoa(*(struct in_addr *)&ip));
#endif
				/* Don't directly call sys_reboot(), give it time to do some cleanup */
				mon_snd_cmd(MON_CMD_REBOOT);
#undef _STEP
			}
#endif /* AUTO_FIX_IP_CONFLICTS */
		}
		if (!netif_cfg(primary_wan_ip_set[i].ifname, &primary_wan_ip_set[i])) {
			primary_wan_ip_set[i].up = DISCONNECTED; 
			WAN_DBG("%s:Network initialization failed for %s\n", __FUNCTION__, primary_wan_ip_set[i].ifname );
		}
		if (primary_wan_ip_set[i].flags & IPSET_FG_DEFROUTE) {
			/*  Add default route  */
			if (primary_wan_ip_set[i].gw_ip) {
    				/* gateway address is different subnet with interface */
    				if(primary_wan_ip_set[i].server_ip && 
    					(primary_wan_ip_set[i].gw_ip == primary_wan_ip_set[i].ip)) {
    						SysLog(LOG_USER|SYS_LOG_INFO|LOGM_SYSTEM, "set server IP as gateway");
    						RT_add_default(primary_wan_ip_set[i].ifname, primary_wan_ip_set[i].server_ip);
    					}
    				else
					RT_add_default(primary_wan_ip_set[i].ifname, primary_wan_ip_set[i].gw_ip);
			}
		}

#ifdef INET6 //Eddy6
		if (primary_wan_ip_set[i].flags & IPSET_FG_DEFROUTE) {
			/*	Add default route  */
			struct nd_defrouter new;

			bzero(&new, sizeof(struct nd_defrouter));	
			memcpy(&new.rtaddr,  &primary_wan_ip_set[i].gw_ip_v6, sizeof(struct in6_addr));
			defrouter_addreq(&new);
		}
#endif
		primary_wan_ip_set[i].conn_time = time(NULL);
		show_ip_set(&primary_wan_ip_set[i]);
		primary_wan_ip_set[i].flags |= IPSET_FG_IFCONFIG;
		WAN_DBG("interface %s init_net successed!\n", primary_wan_ip_set[i].ifname);

#ifdef RALINK_1X_SUPPORT
		Dot1x_Reboot();
#endif //RALINK_1X_SUPPORT
	}
	
	for (i=0; i<WAN_IPALIAS_NUM; i++) {
		if (alias_wan_ip_set[i].up == true) {
			if (!netif_alias_cfg(alias_wan_ip_set[i].ifname, &alias_wan_ip_set[i])) {
				alias_wan_ip_set[i].up = false;
				WAN_DBG("IP alias initialization failed for %s\n",alias_wan_ip_set[i].ifname );
			}
		}
	}
	cyg_thread_delay(2);
}


#if (MODULE_DHCPC == 1)
int dhcpc_mon(int cmd, void *context)
{
	int ipa = 0;
	dhcpc_ctx_t *pctrl = (dhcpc_ctx_t *) context;
	struct ip_set *pipset = pctrl->pipset;
#ifndef WAN_ALL_MODE_DO_MACCLONE
	int val;
	char mac[6];
#endif

	pctrl->state = DHCPC_state(WAN_NAME);
	switch (pctrl->state) {
	case DHCPSTATE_BOUND:
        	memcpy(pipset, &pctrl->tmp_ipset, sizeof(struct ip_set));
        	pipset->up = CONNECTED;
		pipset->flags = pctrl->ipset_flags;
		config_wans();
		break;

	default:
   		WAN_DBG("Getting IP from DHCP server, please wait...\n");
#ifndef WAN_ALL_MODE_DO_MACCLONE
    		if((CFG_get( CFG_WAN_DHCP_MAC_EN, &val) != -1) && (val == 1) && (CFG_get( CFG_WAN_DHCP_MAC, mac) != -1))
    			WAN_MAC_clone(mac, 1);
#endif
    		pipset->mode = DHCPMODE;
           	pipset->up = CONNECTING;
		pipset->flags = 0;
		/* pre-request IP */
		CFG_get( CFG_WAN_DHCP_REQIP, &ipa);
    		DHCPC_init(WAN_NAME, &pctrl->tmp_ipset, ipa);
    		DHCPC_renew(WAN_NAME);
	}
	return 0;
}


int dhcpc_mon_down(int cmd, void *context)
{
	dhcpc_ctx_t *pctrl = (dhcpc_ctx_t *) context;
	struct ip_set *pipset = pctrl->pipset;
	
	DHCPC_free(WAN_NAME);
	
#ifndef WAN_ALL_MODE_DO_MACCLONE
	/* use original MAC */
	WAN_MAC_clone(0,0);
#endif

	/*  Remove routes  */
//	RT_del_default(pipset->ifname, pipset->gw_ip);
	/* flush all route associated with interface(include arp...) */
	RT_ifp_flush(pipset->ifname);
	RT_ifa_flush(pipset->ip);

	/*  Remove interface IP  */
	if (!netif_del(pipset->ifname, NULL))
		WAN_DBG("dhcpc: wan del_ip failed\n");

	pipset->up = DISCONNECTED;
	pipset->conn_time = 0;
	pipset->flags |= IPSET_FG_IFCLOSED;
	pctrl->state = DHCPSTATE_STOPPED;
	pctrl->pipset = NULL;
	return 0;	
}
#endif	/*  MODULE_DHCPC  */


int staticip_mon(int cmd, void *context)
{
	cyg_uint32 ipa=0, mask=0;
	int i, val;
	static_ctx_t *pctrl = (static_ctx_t *) context;
	struct ip_set *pipset = pctrl->pipset;

#ifndef WAN_ALL_MODE_DO_MACCLONE
	char mac[6];
#endif
			
#ifndef WAN_ALL_MODE_DO_MACCLONE
    		if((CFG_get( CFG_WAN_DHCP_MAC_EN, &val) != -1) && (val == 1) && (CFG_get( CFG_WAN_DHCP_MAC, mac) != -1))
    			WAN_MAC_clone(mac, 1);
#endif

	buid_ip_set(pipset, WAN_NAME, pctrl->ipa, pctrl->mask, 0, pctrl->gw , 0, pctrl->mtu, STATICMODE, 
			NULL, NULL );
#ifdef INET6
	{
		memcpy(&pipset->ip_v6, &pctrl->ipa_v6, sizeof(struct in6_addr));
		memcpy(&pipset->netmask_v6, &pctrl->mask_v6, sizeof(struct in6_addr));
		memcpy(&pipset->gw_ip_v6, &pctrl->gw_v6, sizeof(struct in6_addr));
	}
#endif

	mask = pctrl->mask;
	/* IP alias */
	if (CFG_get(CFG_WAN_IP_MANY, &val) != -1 && val==1) {
		for( i=0 ; i< WAN_IPALIAS_NUM ; i++) {
			ipa = 0;
			if(CFG_get( CFG_WAN_IP_MANY_IP+i+1, &ipa) == -1)
				continue;
			//if(CFG_get( CFG_WAN_IP_MANY_MSK+i+1, &mask) == -1)
			//	continue;
			if(ipa && mask) {
				buid_ip_set(&alias_wan_ip_set[i], WAN_NAME, ipa, mask
					, 0, 0 , 0, 0, STATICMODE, NULL, NULL );

				alias_wan_ip_set[i].up = true;
			}
		}
	}
		
	pipset->up = CONNECTED;
	pipset->flags = pctrl->ipset_flags;
	
	/*  Configure wan interface ip and routes  */
	config_wans();
	return 0;
}


int staticip_mon_down(int cmd, void *context)
{
	int i;
	static_ctx_t *pctrl = (static_ctx_t *) context;
	struct ip_set *pipset = pctrl->pipset;

	/*  Delete alias IP  */
   	for( i=0; i<WAN_IPALIAS_NUM; i++) {
		if(alias_wan_ip_set[i].up == true) {
			if (!netif_del(alias_wan_ip_set[i].ifname, &alias_wan_ip_set[i]))
				WAN_DBG("wan_del_alias_ip failed\n");
			else
				alias_wan_ip_set[i].up = false;
		}
	}
	
#ifndef WAN_ALL_MODE_DO_MACCLONE
	/* use original MAC */
	WAN_MAC_clone(0,0);
#endif

	/*  Remove default route  */
	RT_del_default(pipset->ifname, pipset->gw_ip);

#ifdef INET6
	for (i=0; i<PRIMARY_WANIP_NUM; i++) {
		if (primary_wan_ip_set[i].up == CONNECTED) {
		//Eddy6
		int socv6;
		struct in6_aliasreq areq6;
		struct sockaddr_in6 *addrp6;
				
		socv6=socket(AF_INET6, SOCK_DGRAM, 0);
		if (socv6 == -1)
			goto next;
				
		memset(&areq6, 0, sizeof(struct in6_aliasreq));
		addrp6 = (struct sockaddr_in6 *) &areq6.ifra_addr;
		memset(addrp6, 0, sizeof(*addrp6));
		addrp6->sin6_family = AF_INET6;
		addrp6->sin6_len = sizeof(*addrp6);
		addrp6->sin6_port = 0;

		strcpy(areq6.ifra_name, primary_wan_ip_set[i].ifname);
		memcpy(&addrp6->sin6_addr, &primary_wan_ip_set[i].ip_v6, sizeof(struct in6_addr));

		areq6.ifra_prefixmask.sin6_len = sizeof(struct sockaddr_in6);
		areq6.ifra_prefixmask.sin6_family = AF_INET6;
		memcpy(&areq6.ifra_prefixmask.sin6_addr, &primary_wan_ip_set[i].netmask_v6, sizeof(struct in6_addr));
				
		areq6.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME; //Eddy6 Todo
		areq6.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;

		if (ioctl(socv6, SIOCAIFADDR_IN6, &areq6))
		{
			close(socv6);
			goto next;
		}
			
		/* Neighbor Solicitation Processing Anycast */
		areq6.ifra_addr.sin6_addr.s6_addr32[0] &= areq6.ifra_prefixmask.sin6_addr.s6_addr32[0];
		areq6.ifra_addr.sin6_addr.s6_addr32[1] &= areq6.ifra_prefixmask.sin6_addr.s6_addr32[1];
		areq6.ifra_addr.sin6_addr.s6_addr32[2] &= areq6.ifra_prefixmask.sin6_addr.s6_addr32[2];
		areq6.ifra_addr.sin6_addr.s6_addr32[3] &= areq6.ifra_prefixmask.sin6_addr.s6_addr32[3];
		areq6.ifra_flags |= IN6_IFF_ANYCAST;
		if (ioctl(socv6, SIOCDIFADDR_IN6, &areq6))
		{
			close(socv6);
			goto next;
		}
			
		close(socv6);

		/*	Del default route  */
		{
			struct nd_defrouter new;
		
			bzero(&new, sizeof(struct nd_defrouter));	
			memcpy(&new.rtaddr,  &primary_wan_ip_set[i].gw_ip_v6, sizeof(struct in6_addr));
		
			defrouter_delreq(&new); 			
		}
		}
	}
#endif /* INET6 */

next:
	/* flush all route associated with interface(include arp...) */
	RT_ifp_flush(pipset->ifname);
	RT_ifa_flush(pipset->ip);

	/*  Delete static IP  */
	if (!netif_del(pipset->ifname, NULL))
		WAN_DBG("wan_del_ip failed\n");


	pipset->up = DISCONNECTED;
	pipset->conn_time = 0;
	pipset->flags |= IPSET_FG_IFCLOSED;
	pctrl->pipset = NULL;

	return 0;
}


#if (MODULE_PPPOE == 1)
#ifdef	CONFIG_PPTP_PPPOE
extern int pptp_vpn_mon_down(void);
#endif
int pppoe_mon(int cmd, void *context)
{
	int flag;
	ppp_ctx_t *pctrl = (ppp_ctx_t *) context;
	struct ip_set *pipset = pctrl->pipset;
#ifndef WAN_ALL_MODE_DO_MACCLONE
	int val;
	char mac[6];
#endif


	switch(pctrl->state) {
	case PPP_NO_ETH:
		pipset->mode = PPPOEMODE;
		pipset->up = DISCONNECTED;
		pipset->flags = 0;
		pipset->data = NULL;
		netif_up(WAN_NAME);
		pctrl->state = PPP_GET_ETH;
#ifndef WAN_ALL_MODE_DO_MACCLONE
    		if((CFG_get( CFG_WAN_DHCP_MAC_EN, &val) != -1) && (val == 1) && (CFG_get( CFG_WAN_DHCP_MAC, mac) != -1))
    			WAN_MAC_clone(mac, 1);
#endif
		/*   Fall to PPP_GET_ETH*/
	case PPP_GET_ETH:
		pctrl->state = PPP_IN_BEGIN;
		ng_pppoe_doConnect(&pipset->up);
		break;
		
	case PPP_IN_BEGIN:
	case PPP_IN_ACTIVE:
		if (!netif_flag_get("ppp0", &flag) || !(flag&IFF_UP)) {
			WAN_DBG("Connecting to PPPOE server, please wait...2\n");
			ng_pppoe_doConnect(&pipset->up);
		} else {
		if(ppp_up_flag)
		{
			pctrl->state = PPP_IN_ACTIVE;
			memcpy(pipset, &wan_ip_set, sizeof(struct ip_set));
			pipset->up = CONNECTED;
			pipset->flags = pctrl->ipset_flags;

			/*  Configure wan interface ip and routes  */
			config_wans();
			ppp_up_flag = false;
		}
		}
		#ifdef	CONFIG_L2TP_OVER_PPPOE
		int ipmode =0,l2tp_wanif =0;			
		CFG_get(CFG_WAN_IP_MODE, &ipmode);
		CFG_get(CFG_L2T_WANIF, &l2tp_wanif);
		if((ipmode == L2TPMODE) && (l2tp_wanif==2))
		  l2tp_over_pppoe_flag = 1;
		#endif	
		break;

	default:
		WAN_DBG("PPPOE: Unknown wan state: %d\n", pctrl->state);
		break;
	}

	return 0;
}


int pppoe_mon_down(int cmd, void *context)
{
	ppp_ctx_t *pctrl = (ppp_ctx_t *) context;
	struct ip_set *pipset = pctrl->pipset;
	
	#ifdef	CONFIG_PPTP_PPPOE
	//for pppoe + pptp vpn case, shut down pptp first.
	int type, pptp_wanif;
	CFG_get(CFG_WAN_IP_MODE, &type);
	CFG_get(CFG_PTP_WANIF, &pptp_wanif);
	if((type == PPTPMODE) && (pptp_wanif == 2)){
	  pptp_vpn_mon_down();
	  //we need time to do shutdown
	  cyg_thread_delay(50);
	}
	#endif

	#ifdef CONFIG_L2TP_OVER_PPPOE
	int mode, l2tp_wanif;
	CFG_get(CFG_WAN_IP_MODE, &mode);
	CFG_get(CFG_L2T_WANIF, &l2tp_wanif);
	if((mode == L2TPMODE) && (l2tp_wanif == 2)){
	  l2tp_over_pppoe_flag = 0;
	  l2tp_over_pppoe_mon_down();
	  cyg_thread_delay(50);
	}
	#endif
	PPPOE_down((cmd == IFMON_CMD_FULLDOWN) ? 1 : 0);

#ifndef WAN_ALL_MODE_DO_MACCLONE
	/* use original MAC */
	WAN_MAC_clone(0,0);
#endif

	/* flush all route associated with interface(include arp...) */
	RT_ifp_flush(pipset->ifname);
	RT_ifa_flush(pipset->ip);

	pipset->up = DISCONNECTED;
	pipset->conn_time = 0;
	pipset->flags |= IPSET_FG_IFCLOSED;
	pctrl->state = PPP_NO_ETH;
	
	return 0;
}
#endif	/*  MODULE_PPPOE  */


#if (MODULE_PPTP == 1)
int pptp_mon(int cmd, void *context)
{
	int pptp_wanif=0;
	ppp_ctx_t *pctrl = (ppp_ctx_t *) context;
	struct ip_set *pipset = pctrl->pipset;
	
	switch(pctrl->state) {
	case PPP_NO_ETH:
		pipset->mode = PPTPMODE;
		pipset->up = DISCONNECTED;
		pipset->flags = 0;
		pipset->data = &primary_wan_ip_set[1];
		pctrl->state = PPP_WAIT_ETH;
	    	CFG_get( CFG_PTP_WANIF, &pptp_wanif );
		if (pptp_wanif)
		{
			/*  PPTP in static IP mode  */
			static_ctx_t *pstatic = &static_ctl;
			
			pctrl->staticip = 1;
			pstatic->pipset = &primary_wan_ip_set[1];
			pstatic->ipset_flags = IPSET_FG_NATIF|IPSET_FG_FWIF;
			CFG_get(CFG_PTP_IP,&pstatic->ipa);
			CFG_get(CFG_PTP_MSK,&pstatic->mask);
			CFG_get( CFG_PTP_GW, &pstatic->gw);
			pstatic->mtu = 0;
		}
		else
		{
			/*  PPTP in DHCP mode  */
			dhcpc_ctx_t *pdhcpc = &dhcpc_ctl;
			
			pdhcpc->pipset = &primary_wan_ip_set[1];
			pdhcpc->ipset_flags = IPSET_FG_NATIF|IPSET_FG_FWIF;
			pctrl->staticip = 0;
			strcpy(pdhcpc->pipset->ifname, WAN_NAME);
		}
	case PPP_WAIT_ETH:
		if (pctrl->staticip)
		{
			static_ctx_t *pstatic = &static_ctl;
			staticip_mon(cmd, pstatic);
			if (CFG_get(CFG_PTP_DNS, &pctrl->prev_dns[0]) >= 0)
				DNS_CFG_autodns_update(1, (char *)pctrl->prev_dns);
		}
		else
		{
			dhcpc_ctx_t *pdhcpc = &dhcpc_ctl;
			dhcpc_mon(cmd, pdhcpc);
		}
		
		if (primary_wan_ip_set[1].up != CONNECTED)
			break;
		else {
			pctrl->state = PPP_CHK_SRV;
			DNS_CFG_get_autodns(pctrl->prev_dns, sizeof(pctrl->prev_dns)/sizeof(cyg_uint32));
		}
		/*  Fall to PPP_CHK_SRV  */

	case PPP_CHK_SRV:
			{
			struct in_addr dst, mask, gw, hostmask;
			int i, fqdn=0;
			char sever_name[256];
			struct hostent *hostinfo;
			DNS_CFG_autodns_update(2, (char *)pctrl->prev_dns);

			hostmask.s_addr = 	0xffffffff;
			mask.s_addr = 		primary_wan_ip_set[1].netmask;
			gw.s_addr = 		primary_wan_ip_set[1].gw_ip;
			dst.s_addr = 		primary_wan_ip_set[1].server_ip;

			/* Add new route for Russia DualWAN case */
			if (pctrl->staticip == 0)
			{
				if ((dst.s_addr & mask.s_addr) != (gw.s_addr & mask.s_addr))
					RT_add_del(NULL, &dst, &hostmask, &gw, 1, 0, ROUTE_ADD);
			}

			/*   Set route to the DNS server  */
			for (i=0; i<2; i++) {
				if ((dst.s_addr = pctrl->prev_dns[i]) == 0)
					continue;

				if ((dst.s_addr&mask.s_addr) != (gw.s_addr & mask.s_addr)) 
					RT_add_del(NULL, &dst, &hostmask, &gw, 1, 0, ROUTE_ADD);
			}

			CFG_get( CFG_PTP_SVRD_SEL, &fqdn );
			if (fqdn) {
				CFG_get( CFG_PTP_SVRDM, &sever_name );
				hostinfo = gethostbyname(sever_name);
				if (hostinfo && (hostinfo->h_length == 4)) {
					memcpy(&(dst.s_addr), hostinfo->h_addr_list[0], 4);
					pipset->tunnel_gw = dst.s_addr;
				} else{
					timeout(mon_snd_cmd, MON_CMD_WAN_INIT, 200);
					return 0;
				}
			} else {
				CFG_get(CFG_PTP_SVR,&dst.s_addr);
				pipset->tunnel_gw = dst.s_addr;	
			}

			/*  PPTP tunnel server might be not in the WAN domain.  */
			if((gw.s_addr &mask.s_addr)!= (dst.s_addr & mask.s_addr)) {
				if(dst.s_addr)
					RT_add_del(NULL, &dst, &hostmask, &gw, 1, 0, ROUTE_ADD);
    				}
			}
		/*  Fall to PPP_GET_ETH  */
	case PPP_GET_ETH:
		DNS_CFG_autodns_update(2, (char *)pctrl->prev_dns);
		pctrl->state = PPP_IN_BEGIN;
//		pipset->up = CONNECTING;
		//RT_add_default(primary_wan_ip_set[1].ifname, primary_wan_ip_set[1].gw_ip);
		PPTP_doConnect(pipset);
//		cyg_thread_delay(PPTP_START_DELAY);  //wait for pptp to startup
		break;

	case PPP_IN_BEGIN:
	case PPP_IN_ACTIVE:
		if(ppp_up_flag)
		{
		if (pctrl->staticip == 0)
			DHCPC_stop_DNS_update(WAN_NAME);
		memcpy(pipset, &wan_ip_set, sizeof(struct ip_set));
		pipset->tunnel_mode = 1;
		pipset->data = &primary_wan_ip_set[1];
		pipset->up = CONNECTED;
		pipset->flags = pctrl->ipset_flags;
		pctrl->state = PPP_IN_ACTIVE;

		//RT_del_default(primary_wan_ip_set[1].ifname, primary_wan_ip_set[1].gw_ip);
		/*  Configure wan interface ip and routes  */
		config_wans();
		ppp_up_flag = false;
		}
		break;

	default:
		WAN_DBG("PPTP: Unknown wan state: %d\n", pctrl->state);
		break;
	}

	return 0;
}


int pptp_mon_down(int cmd, void *context)
{
	ppp_ctx_t *pctrl = (ppp_ctx_t *) context;
	struct ip_set *pipset = pctrl->pipset;
	
	/*  shutdown pptp  */
	PPTP_down();    		
	cyg_thread_delay(PPTP_STOP_DELAY);  //wait for pptp clean up
	pctrl->state = PPP_CHK_SRV;
	if (pctrl->staticip == 0)
		DHCPC_start_DNS_update(WAN_NAME);

	/*  Remove all routes related with pptp interface */
	RT_ifp_flush(pipset->ifname);
	RT_ifa_flush(pipset->ip);
	/*  Remove pptp IP address  */
	if (!netif_del(pipset->ifname, NULL))
		WAN_DBG("pptp:del pptp ip failed\n");
		
	if (cmd == IFMON_CMD_FULLDOWN) {
		if(pctrl->staticip == 0) {
			/* PPTP in DHCP mode */
			dhcpc_mon_down(cmd, &dhcpc_ctl);
		} else {
			/*  PPTP in statick mode  */
			staticip_mon_down(cmd, &static_ctl);
		}
		pctrl->state = PPP_NO_ETH;
		pipset->data = NULL;
		pipset->flags |= IPSET_FG_IFCLOSED;
	}
	
	pipset->up = DISCONNECTED;
	pipset->conn_time = 0;

	return 0;
}
#endif	/*  MODULE_PPTP  */


#if (MODULE_L2TP == 1)
int l2tp_mon(int cmd, void *context)
{
	int l2tp_wanif=0;
	ppp_ctx_t *pctrl = (ppp_ctx_t *) context;
	struct ip_set *pipset = pctrl->pipset;

	switch(pctrl->state) {
	case PPP_NO_ETH:
		pipset->mode = L2TPMODE;
		pipset->up = DISCONNECTED;
		pipset->flags = 0;
		pipset->data = &primary_wan_ip_set[1];
		pctrl->state = PPP_WAIT_ETH;
	    	CFG_get( CFG_L2T_WANIF, &l2tp_wanif );
		if (l2tp_wanif)
		{
			/* L2TP in static IP mode  */
			static_ctx_t *pstatic = &static_ctl;
			
			pctrl->staticip = 1;
			pstatic->pipset = &primary_wan_ip_set[1];
			pstatic->ipset_flags = IPSET_FG_NATIF|IPSET_FG_FWIF;
			CFG_get(CFG_L2T_IP, &pstatic->ipa);
			CFG_get(CFG_L2T_MSK, &pstatic->mask);
			CFG_get(CFG_L2T_GW, &pstatic->gw);
			pstatic->mtu = 0;
		} 
		else 
		{
			/*  L2TP in DHCP mode  */
			dhcpc_ctx_t *pdhcpc = &dhcpc_ctl;

			pdhcpc->pipset = &primary_wan_ip_set[1];
			pdhcpc->ipset_flags = IPSET_FG_NATIF|IPSET_FG_FWIF;
			pctrl->staticip = 0;
			strcpy(pdhcpc->pipset->ifname, WAN_NAME);
		}
	case PPP_WAIT_ETH:
		if (pctrl->staticip)
		{
			static_ctx_t *pstatic = &static_ctl;
			staticip_mon(cmd, pstatic);
			if (CFG_get(CFG_L2T_DNS, &pctrl->prev_dns[0]) >= 0)
				DNS_CFG_autodns_update(1, (char *)pctrl->prev_dns);
		}
		else
		{
			dhcpc_ctx_t *pdhcpc = &dhcpc_ctl;
			dhcpc_mon(cmd, pdhcpc);
		}
		
		if (primary_wan_ip_set[1].up != CONNECTED)
			break;
		else {
			pctrl->state = PPP_CHK_SRV;
			DNS_CFG_get_autodns(pctrl->prev_dns, sizeof(pctrl->prev_dns)/sizeof(cyg_uint32));
		}

		/*  Fall to PPP_CHK_SRV  */

	case PPP_CHK_SRV:
			{
			struct in_addr dst, mask, gw, hostmask;
			int i,fqdn=0;
			char sever_name[256];
			struct hostent *hostinfo;
			DNS_CFG_autodns_update(2, (char *)pctrl->prev_dns);

			hostmask.s_addr = 	0xffffffff;
			mask.s_addr = 		primary_wan_ip_set[1].netmask;
			gw.s_addr = 		primary_wan_ip_set[1].gw_ip;
			dst.s_addr =		primary_wan_ip_set[1].server_ip;


			/* Add new route for Russia DualWAN case*/
			if (pctrl->staticip == 0)
			{
				if ((dst.s_addr & mask.s_addr) != (gw.s_addr & mask.s_addr))
					RT_add_del(NULL, &dst, &hostmask, &gw, 1, 0, ROUTE_ADD);
			}

			/*   Set route to the DNS server  */
			for (i=0; i<2; i++) {
				if ((dst.s_addr = pctrl->prev_dns[i]) == 0)
					continue;

				if ((dst.s_addr&mask.s_addr) != (gw.s_addr & mask.s_addr)) 
					RT_add_del(NULL, &dst, &hostmask, &gw, 1, 0, ROUTE_ADD);
			}
			
			CFG_get( CFG_L2T_SVRD_SEL, &fqdn );
			if (fqdn) {
				CFG_get( CFG_L2T_SVRDM, &sever_name );
				hostinfo = gethostbyname(sever_name);
				if (hostinfo && (hostinfo->h_length == 4)) {
					memcpy(&(dst.s_addr), hostinfo->h_addr_list[0], 4);
					pipset->tunnel_gw = dst.s_addr;
				} else{
					timeout(mon_snd_cmd, MON_CMD_WAN_INIT, 200);
					return 0;
				}
			} else {
				CFG_get(CFG_L2T_SVR,&dst.s_addr);
				pipset->tunnel_gw = dst.s_addr;	
			}
			/*  l2tp tunnel server might be not in the WAN domain.  */
			if((gw.s_addr &mask.s_addr)!= (dst.s_addr & mask.s_addr)) {
				if(dst.s_addr)
					RT_add_del(NULL, &dst, &hostmask, &gw, 1, 0, ROUTE_ADD);
    			}	
			}

		/*  Fall to PPP_GET_ETH  */
	case PPP_GET_ETH:
		DNS_CFG_autodns_update(2, (char *)pctrl->prev_dns);
		pctrl->state = PPP_IN_BEGIN;
//		pipset->up = CONNECTING;
		//RT_add_default(primary_wan_ip_set[1].ifname, primary_wan_ip_set[1].gw_ip);
		L2tp_doConnect(pipset);
		break;

	case PPP_IN_BEGIN:
	case PPP_IN_ACTIVE:
		if(ppp_up_flag)
		{
		if (pctrl->staticip == 0)
			DHCPC_stop_DNS_update(WAN_NAME);
		memcpy(pipset, &wan_ip_set, sizeof(struct ip_set));
		pipset->data = &primary_wan_ip_set[1];
		pipset->tunnel_mode = 1;
		pipset->up = CONNECTED;
		pipset->flags = pctrl->ipset_flags;
		pctrl->state = PPP_IN_ACTIVE;
		
		//RT_del_default(primary_wan_ip_set[1].ifname, primary_wan_ip_set[1].gw_ip);
		/*  Configure wan interface ip and routes  */
		config_wans();
		ppp_up_flag = false;
		}
		break;
	default:
		WAN_DBG("Unknown L2TP wan state: %d\n", pctrl->state);
		break;
	}
	
	return 0;
}


int l2tp_mon_down(int cmd, void *context)
{
//	int i;
//    	struct in_addr dst, mask, gw;
	ppp_ctx_t *pctrl = (ppp_ctx_t *) context;
	struct ip_set *pipset = pctrl->pipset;

	/*  Shutdown L2TP  */
	L2tp_down();    		
	cyg_thread_delay(L2TP_STOP_DELAY);  //wait for l2tp clean up
	pctrl->state = PPP_CHK_SRV;
	if (pctrl->staticip == 0)
		DHCPC_start_DNS_update(WAN_NAME);
	
	/*  Remove all routes related with pptp interface */
	RT_ifp_flush(pipset->ifname);
	RT_ifa_flush(pipset->ip);

	/*  Remove L2TP IP address  */
	if (!netif_del(pipset->ifname, NULL))
		WAN_DBG("l2tp:del l2tp ip failed\n");
	
	if (cmd == IFMON_CMD_FULLDOWN) {
		if(pctrl->staticip == 0) {
			/* PPTP in DHCP mode */
			dhcpc_mon_down(cmd, &dhcpc_ctl);
		} else {
			/*  PPTP in statick mode  */
			staticip_mon_down(cmd, &static_ctl);
		}
		pctrl->state = PPP_NO_ETH;
		pipset->data = NULL;
		pipset->flags |= IPSET_FG_IFCLOSED;
	}
 
	pipset->up = DISCONNECTED;
	pipset->conn_time = 0;

	return 0;
}
#endif	/*  MODULE_L2TP  */


/*  */
int Wan_stop_stage2(void) 
{
	int i;
	
	WAN_DBG("wanstop2: mode=%d,%d\n", SYS_wan_mode, wanmode);
	switch(SYS_wan_mode) {
#if (MODULE_PPPOE == 1)
	case PPPOEMODE:	//pppoe mode
#endif
#if (MODULE_PPTP == 1)
	case PPTPMODE:	//pptp mode
#endif
#if (MODULE_L2TP == 1)
	case L2TPMODE:
#endif
		/*  Remove PPP interface  */
		if (ppp_ifnet)
			if_detach(ppp_ifnet);
		ppp_ifnet = NULL;
		if (primary_wan_ip_set[0].flags & IPSET_FG_IFCLOSED) {
			ppp_ctl.pipset = NULL;
			ppp_ctl.state = PPP_NO_ETH;
		}
		else {
			ppp_ctl.pipset->ip = 0;
			ppp_ctl.pipset->gw_ip = 0;
			ppp_ctl.pipset->netmask = 0;
		}
	}
	
	if (primary_wan_ip_set[0].flags & IPSET_FG_IFCLOSED)
		buid_ip_set(&primary_wan_ip_set[0], WAN_NAME, 0, 0
			, 0, 0, 0, 0, NODEFMODE, NULL, NULL );

	for (i=1; i<PRIMARY_WANIP_NUM; i++) {
		if (primary_wan_ip_set[i].flags & IPSET_FG_IFCLOSED) {
			memset(&primary_wan_ip_set[i], 0, sizeof(struct ip_set));
			primary_wan_ip_set[i].mode = NODEFMODE;
			primary_wan_ip_set[i].up = DISCONNECTED;
		}
	}
	
	return 0;
}


//------------------------------------------------------------------------------
// FUNCTION
//		int wan_set_ip(void)
//
// DESCRIPTION
//  	This function was called by WAN_start(1) and do set WAN interface by "WAN_IPMODE"
//		WAN_IPMODE: 0 - NO defined, can use to disable WAN
//					1 - static 
//					2 - DHCP clinet
//					3 - PPPOE
//		then set default route
//
// PARAMETERS
//		NONE
//  
// RETURN
//		-1: not yet
//		 0: some interfaces are up
//		 1: main wan interface is up
//  
//------------------------------------------------------------------------------
 int wan_set_ip(void)
{
	int retval=-1;
	#ifdef CONFIG_L2TP_OVER_PPPOE
	int l2tp_wanif = 0;
	#endif
#ifdef WAN_ALL_MODE_DO_MACCLONE
	int val;
	char mac[6];
#endif
	if (wanmode == NODEFMODE)
		CFG_get( CFG_WAN_IP_MODE, &wanmode );
	#ifdef	CONFIG_PPTP_PPPOE
	int pptp_wanif=0;
	//Added by haitao,for pppoe-->pptp vpn case
	CFG_get( CFG_PTP_WANIF, &pptp_wanif );
	if(wanmode ==PPTPMODE && pptp_wanif ==2)
			wanmode=PPPOEMODE;
	#endif

#ifdef WAN_ALL_MODE_DO_MACCLONE
	if((CFG_get( CFG_WAN_DHCP_MAC_EN, &val) != -1) && (val == 1) && 
			(CFG_get( CFG_WAN_DHCP_MAC, mac) != -1))
    		WAN_MAC_clone(mac, 1);
#endif

	switch(wanmode) 
	{
#if (MODULE_DHCPC == 1)
	case DHCPMODE: //dhcp mode
		if (dhcpc_ctl.state == DHCPSTATE_STOPPED) {
			dhcpc_ctl.pipset = &primary_wan_ip_set[0];
			dhcpc_ctl.ipset_flags = IPSET_FG_DEFROUTE|IPSET_FG_NATIF|IPSET_FG_FWIF;
		}
		dhcpc_mon(IFMON_CMD_FULLUP, &dhcpc_ctl);
#ifdef	CONFIG_BPA
		if ((primary_wan_ip_set[0].up ==CONNECTED) && 
			CFG_get(CFG_WAN_DHCP_BPA_EN, &val) != -1 && val==1) {
			char username[64], password[64], server[64];
			
			if((CFG_get(CFG_WAN_DHCP_BPA_USER, username)!= -1) &&
				(CFG_get(CFG_WAN_DHCP_BPA_PASS, password)!= -1) &&
				(CFG_get(CFG_WAN_DHCP_BPA_SERV, server)!= -1))	{
				
				/*  All information are ready now, login to the server.  */
				bpa_start(username, password, server);
			}
		}
				
#endif	/*  CONFIG_BPA   */
		if (primary_wan_ip_set[0].up ==CONNECTED)
			retval = 1;
	        break;
#endif /*  (MODULE_DHCPC == 1)  */

	case STATICMODE: //static mode 
		static_ctl.ipa = static_ctl.mask = 0;
		static_ctl.gw = static_ctl.mtu = 0;
		CFG_get( CFG_WAN_IP, &static_ctl.ipa);
		CFG_get( CFG_WAN_MSK, &static_ctl.mask);
		CFG_get( CFG_WAN_GW, &static_ctl.gw);
		CFG_get( CFG_WAN_MTU, &static_ctl.mtu);
		static_ctl.pipset = &primary_wan_ip_set[0];
		static_ctl.ipset_flags = IPSET_FG_DEFROUTE|IPSET_FG_NATIF|IPSET_FG_FWIF;
#ifdef INET6
		{
			char ipa_v6[40];
			char mask_v6[40];
			char gw_v6[40];
			
			CFG_get_str( CFG_WAN_IP_v6, ipa_v6);
			CFG_get_str( CFG_WAN_MSK_v6, mask_v6);
			CFG_get_str( CFG_WAN_GW_v6, gw_v6);
		
			diag_printf("WAN IPv6 IP = %s\n", ipa_v6);
			diag_printf("WAN IPv6 Mask = %s\n", mask_v6);
			diag_printf("WAN IPv6 GW = %s\n", gw_v6);
			inet_pton(AF_INET6, ipa_v6, &static_ctl.ipa_v6);
			inet_pton(AF_INET6, mask_v6, &static_ctl.mask_v6);
			inet_pton(AF_INET6, gw_v6, &static_ctl.gw_v6);
		}
#endif


		staticip_mon(IFMON_CMD_FULLUP, &static_ctl);
		if (primary_wan_ip_set[0].up ==CONNECTED)
			retval = 1;
		break;

#if (MODULE_PPPOE == 1)
	case PPPOEMODE:	//pppoe mode
		if (ppp_ctl.pipset == NULL) {
			ppp_ctl.pipset = &primary_wan_ip_set[0];
			ppp_ctl.ipset_flags = IPSET_FG_DEFROUTE|IPSET_FG_NATIF|IPSET_FG_FWIF;
			ppp_ctl.state = PPP_NO_ETH;
			CFG_get(CFG_POE_AUTO, &ppp_ctl.conn_mode);
			ppp_ctl.prev_dns[0] = ppp_ctl.prev_dns[1] = 0;
			strcpy(ppp_ctl.pipset->ifname, "ppp0");
		}
		pppoe_mon(IFMON_CMD_FULLUP, &ppp_ctl);
		if (primary_wan_ip_set[0].up ==CONNECTED)
			retval = 1;
		break;
#endif	/*  (MODULE_PPPOE == 1)  */

#if (MODULE_PPTP == 1)
	case PPTPMODE:	//pptp mode
		if (ppp_ctl.pipset == NULL) {
			ppp_ctl.pipset = &primary_wan_ip_set[0];
			ppp_ctl.ipset_flags = IPSET_FG_DEFROUTE|IPSET_FG_NATIF|IPSET_FG_FWIF;
			ppp_ctl.state = PPP_NO_ETH;
			ppp_ctl.prev_dns[0] = ppp_ctl.prev_dns[1] = 0;
			strcpy(ppp_ctl.pipset->ifname, "ppp0");
		}
		pptp_mon(IFMON_CMD_FULLUP, &ppp_ctl);
		if (primary_wan_ip_set[0].up ==CONNECTED)
			retval = 1;
		else if (primary_wan_ip_set[1].up == CONNECTED)
			retval = 0;

		break;
#endif	/*  (MODULE_PPTP == 1)  */

#if (MODULE_L2TP == 1)
	case L2TPMODE:	//L2TP mode
        #ifdef CONFIG_L2TP_OVER_PPPOE
		  CFG_get( CFG_L2T_WANIF, &l2tp_wanif);
		  if(l2tp_wanif == 2){//l2tp over pppoe mode,call pppoe_mon firstly.
		    wanmode=PPPOEMODE;
		    if (ppp_ctl.pipset == NULL) {
			  ppp_ctl.pipset = &primary_wan_ip_set[0];
			  ppp_ctl.ipset_flags = IPSET_FG_DEFROUTE|IPSET_FG_NATIF|IPSET_FG_FWIF;
			  ppp_ctl.state = PPP_NO_ETH;
			  CFG_get(CFG_POE_AUTO, &ppp_ctl.conn_mode);
			  ppp_ctl.prev_dns[0] = ppp_ctl.prev_dns[1] = 0;
			  strcpy(ppp_ctl.pipset->ifname, "ppp0");
			}
			pppoe_mon(IFMON_CMD_FULLUP, &ppp_ctl);
			if (primary_wan_ip_set[0].up ==CONNECTED)
			  retval = 1;
		  }else
	   #endif
		  {
		  if (ppp_ctl.pipset == NULL) {
			ppp_ctl.pipset = &primary_wan_ip_set[0];
			ppp_ctl.ipset_flags = IPSET_FG_DEFROUTE|IPSET_FG_NATIF|IPSET_FG_FWIF;
			ppp_ctl.state = PPP_NO_ETH;
			ppp_ctl.prev_dns[0] = ppp_ctl.prev_dns[1] = 0;
			strcpy(ppp_ctl.pipset->ifname, "ppp0");
		 }
		 l2tp_mon(IFMON_CMD_FULLUP, &ppp_ctl);
		 if (primary_wan_ip_set[0].up ==CONNECTED)
			retval = 1;
		 else if (primary_wan_ip_set[1].up == CONNECTED)
			retval = 0;		  
		 }
		break;
#endif 	/*  (MODULE_L2TP == 1)  */

	case NODEFMODE:
	default:
		wanmode = NODEFMODE;
		return -1;
	}

	return retval;
}

//------------------------------------------------------------------------------
// FUNCTION
//		int wan_del_ip()
//
// DESCRIPTION
//		This function was called by WAN_start(0) and do purge all configurations
//	of WAN interface and clean up default route
//  
// PARAMETERS
//		NONE
//  
// RETURN
//		NONE
//  
//------------------------------------------------------------------------------
 int wan_del_ip(int cmd)
{
//	int ipmode, i;
	//int wanmode;

	//CFG_get( CFG_WAN_IP_MODE, &wanmode );
	
	
	WAN_DBG("free previous mode:%d,%d\n", primary_wan_ip_set[0].mode,  wanmode);
  	switch(wanmode)
    	{
#if (MODULE_DHCPC == 1)
    	case DHCPMODE:
#ifdef	CONFIG_BPA
		if(bpa_run()) {
			bpa_stop();
			/*  Wait until BPA stopped.  */
			cyg_thread_delay(20);
		}
#endif	/*  CONFIG_BPA  */
		if (dhcpc_ctl.state != DHCPSTATE_STOPPED) {
			dhcpc_mon_down(cmd, &dhcpc_ctl);
			if (wanmode == DHCPMODE)
				dhcpc_ctl.pipset = &primary_wan_ip_set[0];
		}
    		break;
#endif	/*  (MODULE_DHCPC == 1)  */

#if (MODULE_PPPOE == 1)
    	case PPPOEMODE: // pppoe
		if (ppp_ctl.pipset != NULL) {
    			pppoe_mon_down(cmd, &ppp_ctl);
		}
    		break;	
#endif	/*  (MODULE_PPPOE == 1)  */

    	case STATICMODE: // static 
		staticip_mon_down(cmd, &static_ctl);
		if (wanmode == STATICMODE)
			static_ctl.pipset = &primary_wan_ip_set[0];
    		break;
		
#if (MODULE_PPTP == 1)
	case PPTPMODE:
		if (ppp_ctl.pipset != NULL) {
			pptp_mon_down(cmd, &ppp_ctl);
		}
    		break;	
#endif	/*  (MODULE_PPTP == 1)  */

#if (MODULE_L2TP == 1)
	case L2TPMODE:
		if (ppp_ctl.pipset != NULL) {
			l2tp_mon_down(cmd, &ppp_ctl);
		}
    		break;	
#endif	/*  (MODULE_L2TP == 1)  */

	case NODEFMODE:
	default:
		return 0;    	
    	}

	/* flush DNS server setting */
	DNS_CFG_autodns_update(0, (char *)0);

/* FIXME: Shouldn't it all be ifdef? */
#ifndef WAN_ALL_MODE_DO_MACCLONE
		/* use original MAC */
    	WAN_MAC_clone(0,0);
#endif /* WAN_ALL_MODE_DO_MACCLONE */

    return 0;
}    


int ifr_link_cmd(char * ifn, int cmd, void *data)
{
    int s;
    struct ifreq ifr;
	int rc=0;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        return -1;
    }

	strcpy(ifr.ifr_name, ifn);
	ifr.ifr_data = (char *) data ;

    if (ioctl(s, cmd, &ifr)) {
		rc=-1;
		goto end;
    }
end:
    close(s);
    return rc;
}


int ifr_link_check(char * ifn)
{
	int status=0;
	ifr_link_cmd(ifn, SIOCGIFPHY, &status);
	return status;
}


int ifr_link_set(char * ifn, int mode)
{
	int status=mode;
	return ifr_link_cmd(ifn, SIOCSIFPHY, &status);
}


//------------------------------------------------------------------------------
// FUNCTION
//		int wan_set_phy_mode()
//
// DESCRIPTION
//  	Set the wan port mode with CFG_WAN_PHY 
//
// PARAMETERS
//		NONE
//  
// RETURN
//		NONE
//  
//------------------------------------------------------------------------------
int wan_set_phy(void)
{
    int mode=0;
    
	CFG_get( CFG_WAN_PHY, &mode ); 
	if (mode < 5) {
		return ifr_link_set(WAN_NAME, mode);
	}
	return -1;
}


//------------------------------------------------------------------------------
// FUNCTION
//		int WAN_start(int cmd)
//
// DESCRIPTION
//		deal with WAN interface down or up
//  
// PARAMETERS
//		cmd :	0 - clean and down WAN interface
//				1 - bring up WAN interface
//
// RETURN
//		NONE
//
//------------------------------------------------------------------------------

int WAN_start(int cmd)
{
    char * ifn=WAN_NAME;
    int rc=0;
    
    if(strcmp(WAN_NAME, "") == 0)
		return -1;

    switch (cmd)
    {
	case 0:
            rc = wan_del_ip(IFMON_CMD_FULLDOWN);
	    wanmode = NODEFMODE;
            netif_down(ifn);
	    wan_set_phy();
#ifdef CONFIG_UPNP
	    upnp_update_wanstatus();
#endif
		/* Added by Lea.
		 * In AP client mode, the interface 'apcliX'should always be up for later
		 * reconnect.
		 */
		{
			int val;

			if (0 == strncmp(WAN_NAME, "apcli", sizeof("apcli") - 1)
					&& (CFG_get(CFG_WLN_ApCliAutoConnect, &val) != -1
						&& val == 1)) {
				netif_up(ifn);
			}
		}
            break;
            
	case 2:
            rc = WAN_start(0);
            rc = WAN_start(1);
            break;
	    
	case 3:
	    rc = wan_del_ip(IFMON_CMD_PRIMARY_DOWN);
#ifdef CONFIG_UPNP
	    upnp_update_wanstatus();
#endif
	    break;
	    
	case 1:
	default:
	    wan_set_phy();
            rc = wan_set_ip();
            netif_up(ifn);
#ifdef CONFIG_UPNP
	    upnp_update_wanstatus();
#endif
		/* Added by Lea.
		 * In AP client mode, the interface 'apcliX'should always be up for later
		 * reconnect.
		 */
		{
			int val;

			if (0 == strncmp(WAN_NAME, "apcli", sizeof("apcli") - 1)
					&& (CFG_get(CFG_WLN_ApCliAutoConnect, &val) != -1
						&& val == 1)) {
				netif_up(ifn);
			}
		}
            break;
    }
    return rc;
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
void WAN_MAC_clone(char *mac, int set)
{
	/* MAC clone */
	if(set)
	{
		net_set_maca(WAN_NAME, mac);
	}
	else 
	{
		char newmac[6];
		
		CFG_get_mac(1, newmac);
		net_set_maca(WAN_NAME, newmac);
	}
}


//------------------------------------------------------------------------------
// FUNCTION
//		int WAN_conn_state(int mode)
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//		0: disconnt
//  	1: connected
//
//------------------------------------------------------------------------------
int WAN_conn_state(int mode)
{
	int flag;
	 
	switch(mode)
	{
#ifdef CONFIG_DHCPC
	case DHCPMODE:
		if(dhcpc_ctl.state != DHCPSTATE_BOUND) 
        	return 0;
        else
        	return 1;
		break;
#endif        
	case PPPOEMODE:	
		if(!netif_flag_get("ppp0", &flag) || !(flag&IFF_UP)) 
			return 0;
		else
			return 1;
		break;
	case STATICMODE:
	default:
		return 1;
	}
}


