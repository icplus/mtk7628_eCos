




#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>


#ifndef __ECOS 
#include <net/if_arp.h>
#include <linux/if.h>			/* for IFNAMSIZ and co... */
#include <linux/wireless.h>
#include <syslog.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#else
#include <network.h>
#include <sys/types.h>
#include <cyg/kernel/kapi.h>
#include <cyg/io/flash.h>
#include <cyg/infra/diag.h>
#include <ifaddrs.h>
#include <cyg/io/eth/eth_drv_stats.h>
#include <cyg/io/eth/eth_drv.h>
#include <cyg/io/eth/netdev.h>
#include <pkgconf/system.h>

#include <sys/mbuf.h>
#include <cyg/hal/hal_if.h>     // For CYGACC_CALL_IF_DELAY_US
#include <cyg/hal/hal_cache.h>  // For HAL_DCACHE_INVALIDATE
#include <cyg/io/eth/netdev.h>  // For struct eth_drv_sc
#include <cfg_id.h>
#include <net/netisr.h>
#endif

#include "rtdot1x.h"
#include "eloop.h"
#include "ieee802_1x.h"
#include "eapol_sm.h"
#include "ap.h"
#include "sta_info.h"
#include "radius_client.h"
#include "dot1xconfig.h"

//#define RT2860AP_SYSTEM_PATH   "/etc/Wireless/RT2860AP/RT2860AP.dat"

#define MemPool_TYPE_Header 3
#define MemPool_TYPE_CLUSTER 4
#define BUFFER_HEAD_RESVERD 64

extern	u_long mbtypes[];			/* XXX */
extern  struct	mbstat mbstat;
extern  union	mcluster *mclfree;
extern	char *mclrefcnt;


extern int rt28xx_ap_ioctl(
	PNET_DEV	pEndDev, 
	int			cmd, 
	caddr_t		pData);

unsigned char* Ecos_MemPool_Alloc (unsigned long Length,int Type);

struct hapd_interfaces {
	int count;
	rtapd **rtapd;
};
#ifdef LINUX
u32    RTDebugLevel = RT_DEBUG_ERROR;
#endif

#ifdef __ECOS
static Boolean Dot_Running=FALSE;
u32    DOT1X_RTDebugLevel = RT_DEBUG_TRACE;
#define STACK_SIZE 8192*2
//(CYGNUM_HAL_STACK_SIZE_MINIMUM)

//int Is_term;

unsigned char	Dot1x_main_stack[STACK_SIZE];
cyg_handle_t dot1x_thread;
cyg_thread dot1x_thread_struct;

struct hapd_interfaces interfaces;
#endif
/*
	========================================================================
	
	Routine Description:
		Compare two memory block

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		0:			memory is equal
		1:			pSrc1 memory is larger
		2:			pSrc2 memory is larger

	Note:
		
	========================================================================
*/
 void dot1xintr()
 {
            //diag_printf("dot1xintr\n");
            register struct mbuf *m;
            struct ifnet *ifp;
            int s;
            unsigned char *packet;
            int len;
        
            while (dot1xintrq.ifq_head) {
                s = splimp();
                IF_DEQUEUE(&dot1xintrq, m);
                splx(s);
                if (m == 0 || (m->m_flags & M_PKTHDR) == 0)
                    panic("dot1xintr");
        
                packet = m->m_data-14;//br may change something like ATE
                len = m->m_len+14;
        	DBGPRINT(RT_DEBUG_WARN, "%s :Receive  SA  (%02x:%02x:%02x:%02x:%02x:%02x)\n",__FUNCTION__,MAC2STR(packet));//PRINTF SA
		DBGPRINT(RT_DEBUG_WARN, "%s :Receive  DA (%02x:%02x:%02x:%02x:%02x:%02x)\n",__FUNCTION__,MAC2STR(packet+6));//PRINTF DA
                //hex_dump("dot1xintr  ===",packet,len);
        
                ifp = m->m_pkthdr.rcvif;
                Dot1xNetReceive(ifp, packet, len);
        
                m_freem(m);
            }
 }
#ifdef LINUX
u16	RTMPCompareMemory(void *pSrc1,void *pSrc2, u16 Length)
{
	char *pMem1;
	char *pMem2;
	u16	Index = 0;

	pMem1 = (char*) pSrc1;
	pMem2 = (char*) pSrc2;

	for (Index = 0; Index < Length; Index++)
	{
		if (pMem1[Index] > pMem2[Index])
			return (1);
		else if (pMem1[Index] < pMem2[Index])
			return (2);
	}

	// Equal
	return (0);
}
#endif //LINUX

#ifdef __ECOS
u16	DOT1X_RTMPCompareMemory(void *pSrc1,void *pSrc2, u16 Length)
{
	char *pMem1;
	char *pMem2;
	u16	Index = 0;

	pMem1 = (char*) pSrc1;
	pMem2 = (char*) pSrc2;

	for (Index = 0; Index < Length; Index++)
	{
		if (pMem1[Index] > pMem2[Index])
			return (1);
		else if (pMem1[Index] < pMem2[Index])
			return (2);
	}

	// Equal
	return (0);
}
#endif
int RT_ioctl(
		int 			sid, 
		int 			param, 
		char  			*data, 
		int 			data_len, 
		char 			*prefix_name, 
		unsigned char 	apidx, 
		int 			flags)
{
    char			name[12];
    int				ret = 1;
    struct iwreq	wrq;

#ifdef __ECOS
	cyg_netdevtab_entry_t *t;
	struct eth_drv_sc *sc;
	int found = 0;
#endif

	memset(name,'\0',12);
    	sprintf(name, "ra%d", apidx);
    	name[3] = '\0';
    	wrq.u.data.flags = flags;
	wrq.u.data.length = data_len;
    	wrq.u.data.pointer = (caddr_t) data;
#ifdef __ECOS
	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;
		if (strcmp(sc->dev_name, name) == 0) {
			found =1;
			break;
	        }
	}

	if ( found == 1 )
	{
#ifdef CONFIG_AP_SUPPORT	
		if ((ret=rt28xx_ap_ioctl(sc, param, (caddr_t)&wrq)) < 0)
		{
			diag_printf("RTPRIV_IOCTL_SET has error.\n");
		}
#endif		
	}
	else
	{
		diag_printf("Can't find ra0.\n");
		ret=1;
	}
		
#endif//__ECOS
#ifdef LINUX
    ret = ioctl(sid, param, &wrq);	
 #endif
    return ret;
}

void dot1x_set_IdleTimeoutAction(
		rtapd *rtapd,
		struct sta_info *sta,
		u32		idle_timeout)
{
	DOT1X_IDLE_TIMEOUT dot1x_idle_time;

	memset(&dot1x_idle_time, 0, sizeof(DOT1X_IDLE_TIMEOUT));

	memcpy(dot1x_idle_time.StaAddr, sta->addr, MAC_ADDR_LEN);
	
	dot1x_idle_time.idle_timeout = 
		((idle_timeout < DEFAULT_IDLE_INTERVAL) ? DEFAULT_IDLE_INTERVAL : idle_timeout);

	if (RT_ioctl(rtapd->ioctl_sock, 
				 RT_PRIV_IOCTL, 
				 (char *)&dot1x_idle_time, 
				 sizeof(DOT1X_IDLE_TIMEOUT), 
				 rtapd->prefix_wlan_name, sta->ApIdx, 
				 RT_OID_802_DOT1X_IDLE_TIMEOUT))
	{				   
    	DBGPRINT(RT_DEBUG_ERROR,"Failed to RT_OID_802_DOT1X_IDLE_TIMEOUT\n");
    	return;
	}   

}

static void Handle_reload_config(
	rtapd 	*rtapd)
{
	struct rtapd_config *newconf;	
	int i;

	DBGPRINT(RT_DEBUG_TRACE, "Reloading configuration\n");

	/* create new config */					
	newconf = Config_read(rtapd->ioctl_sock, rtapd->prefix_wlan_name);
	if (newconf == NULL)
    {
		DBGPRINT(RT_DEBUG_ERROR, "Failed to read new configuration file - continuing with old.\n");
		return;
	}

	/* TODO: update dynamic data based on changed configuration
	 * items (e.g., open/close sockets, remove stations added to
	 * deny list, etc.) */
	Radius_client_flush(rtapd);
	Config_free(rtapd->conf);
	rtapd->conf = newconf;
    Apd_free_stas(rtapd);

	/* when reStartAP, no need to reallocate sock
    for (i = 0; i < rtapd->conf->SsidNum; i++)
    {
        if (rtapd->sock[i] >= 0)
            close(rtapd->sock[i]);
            
	    rtapd->sock[i] = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	    if (rtapd->sock[i] < 0)
        {
		    perror("socket[PF_PACKET,SOCK_RAW]");
		    return;
	    }
    }*/

#if MULTIPLE_RADIUS
	for (i = 0; i < MAX_MBSSID_NUM; i++)
		rtapd->radius->mbss_auth_serv_sock[i] = -1;
#else
	rtapd->radius->auth_serv_sock = -1;
#endif

    if (Radius_client_init(rtapd))
    {
	    DBGPRINT(RT_DEBUG_ERROR,"RADIUS client initialization failed.\n");
	    return;
    }
#if MULTIPLE_RADIUS
	for (i = 0; i < rtapd->conf->SsidNum; i++)
		DBGPRINT(RT_DEBUG_TRACE, "auth_serv_sock[%d] = %d\n", i, rtapd->radius->mbss_auth_serv_sock[i]);
#else
    DBGPRINT(RT_DEBUG_TRACE,"rtapd->radius->auth_serv_sock = %d\n",rtapd->radius->auth_serv_sock);
#endif
	
}

static void Handle_read(int sock, void *eloop_ctx, void *sock_ctx)
{              
	rtapd *rtapd = eloop_ctx;
	int len;
	unsigned char buf[3000];
	u8 *sa, *da, *pos, *pos_vlan, apidx=0, isVlanTag=0;
	u16 ethertype,i;
        priv_rec *rec;
        size_t left;
	u8 	RalinkIe[9] = {221, 7, 0x00, 0x0c, 0x43, 0x00, 0x00, 0x00, 0x00}; 

		
	len = recv(sock, buf, sizeof(buf), 0);
	if (len < 0)
    {
		perror("recv");
        Handle_term(15,eloop_ctx,sock_ctx);
        return;
	}

	rec = (priv_rec*)buf;
    left = len -sizeof(*rec)+1;
	if (left <= 0)
	{
		DBGPRINT(RT_DEBUG_ERROR," too short recv\n");
		return;
	}
	
    	sa = rec->saddr;
	da = rec->daddr;
	ethertype = rec->ethtype[0] << 8;
	ethertype |= rec->ethtype[1];
			
#ifdef ETH_P_VLAN
	if(ethertype == ETH_P_VLAN)
    {
    	pos_vlan = rec->xframe;

        if(left >= 4)
        {
			ethertype = *(pos_vlan+2) << 8;
			ethertype |= *(pos_vlan+3);
		}
			
		if((ethertype == ETH_P_PRE_AUTH) || (ethertype == ETH_P_PAE))
		{
			isVlanTag = 1;
			DBGPRINT(RT_DEBUG_TRACE,"Recv vlan tag for 802.1x. (%02x %02x)\n", *(pos_vlan), *(pos_vlan+1));		
		}
    }
#endif
	
	if ((ethertype == ETH_P_PRE_AUTH) || (ethertype == ETH_P_PAE))	
    {
        // search this packet is coming from which interface
		for (i = 0; i < rtapd->conf->SsidNum; i++)
		{		    
			if (memcmp(da, rtapd->own_addr[i], 6) == 0)
		    {
		        apidx = i;		        
		        break;
		    }
		}
		
		if(i >= rtapd->conf->SsidNum)
		{
	        DBGPRINT(RT_DEBUG_WARN, "Receive unexpected DA (%02x:%02x:%02x:%02x:%02x:%02x)\n",
										MAC2STR(da));
		    return;
		}

		if (ethertype == ETH_P_PRE_AUTH)
		{
			DBGPRINT(RT_DEBUG_TRACE, "Receive WPA2 pre-auth packet for %s%d\n", rtapd->prefix_wlan_name, apidx);
		}
		else
		{
			DBGPRINT(RT_DEBUG_TRACE, "Receive EAP packet for %s%d\n", rtapd->prefix_wlan_name, apidx);
		}
    }
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, "Receive unexpected ethertype 0x%04X!!!\n", ethertype);
		return;
	}

    pos = rec->xframe;
    
    //strip 4 bytes for valn tag
    if(isVlanTag)
    {
    	pos += 4;
    	left -= 4;
	}
    
	/* Check if this is a internal command or not */
	if (left == sizeof(RalinkIe) && 
#ifdef LINUX
		RTMPCompareMemory(pos, RalinkIe, 5) == 0
#endif //LINUX
#ifdef __ECOS
		DOT1X_RTMPCompareMemory(pos, RalinkIe, 5) == 0
#endif
	)
	{
		u8	icmd = *(pos + 5);
	
		switch(icmd)
		{
			case DOT1X_DISCONNECT_ENTRY:
			{
				struct sta_info *s;

				s = rtapd->sta_hash[STA_HASH(sa)];
				while (s != NULL && memcmp(s->addr, sa, 6) != 0)
				s = s->hnext;

				DBGPRINT(RT_DEBUG_TRACE, "Receive discard-notification form wireless driver.\n");
				if (s)
				{
					DBGPRINT(RT_DEBUG_TRACE,"This station(%02x:%02x:%02x:%02x:%02x:%02x) is removed.\n", MAC2STR(sa));
					Ap_free_sta(rtapd, s);						
				}
				else
				{
					DBGPRINT(RT_DEBUG_INFO, "This station(%02x:%02x:%02x:%02x:%02x:%02x) doesn't exist.\n", MAC2STR(sa));
				}
			}
			break;

			case DOT1X_RELOAD_CONFIG:
				Handle_reload_config(rtapd);
			break;

			default:
				DBGPRINT(RT_DEBUG_ERROR, "Unknown internal command(%d)!!!\n", icmd);
			break;
		}
	}
	else
	{
		/* Process the general EAP packet */
    	ieee802_1x_receive(rtapd, sa, &apidx, pos, left, ethertype, sock);
	}
}



void Ecos_Handle_read(int sock, void *eloop_ctx, void *sock_ctx, unsigned char *buf, int len)
{              
	DBGPRINT(RT_DEBUG_TRACE," ===>Ecos_Handle_read\n");
	rtapd *rtapd = eloop_ctx;
	u8 *sa, *da, *pos, *pos_vlan, apidx=0, isVlanTag=0;
	u16 ethertype,i;
    priv_rec *rec;
    size_t left;
	u8 	RalinkIe[9] = {221, 7, 0x00, 0x0c, 0x43, 0x00, 0x00, 0x00, 0x00}; 


	rec = (priv_rec*)buf;
    left = len -sizeof(*rec)+1;
	if (left <= 0)
	{
		DBGPRINT(RT_DEBUG_ERROR," too short recv\n");
		DBGPRINT(RT_DEBUG_TRACE," <===Ecos_Handle_read\n");
		return;
	}
	
    	sa = rec->saddr;
	da = rec->daddr;
	ethertype = rec->ethtype[0] << 8;
	ethertype |= rec->ethtype[1];
			
#ifdef ETH_P_VLAN
	if(ethertype == ETH_P_VLAN)
    {
    	pos_vlan = rec->xframe;

        if(left >= 4)
        {
			ethertype = *(pos_vlan+2) << 8;
			ethertype |= *(pos_vlan+3);
		}
			
		if((ethertype == ETH_P_PRE_AUTH) || (ethertype == ETH_P_PAE))
		{
			isVlanTag = 1;
			DBGPRINT(RT_DEBUG_TRACE,"Recv vlan tag for 802.1x. (%02x %02x)\n", *(pos_vlan), *(pos_vlan+1));		
		}
    }
#endif
	
	if ((ethertype == ETH_P_PRE_AUTH) || (ethertype == ETH_P_PAE))	
    {
        // search this packet is coming from which interface
		for (i = 0; i < rtapd->conf->SsidNum; i++)
		{		    
			if (memcmp(da, rtapd->own_addr[i], 6) == 0)
		    {
		        apidx = i;		        
		        break;
		    }
		}
		
		if(i >= rtapd->conf->SsidNum)
		{
	        DBGPRINT(RT_DEBUG_WARN, "Receive unexpected DA (%02x:%02x:%02x:%02x:%02x:%02x)\n",
										MAC2STR(da));
				DBGPRINT(RT_DEBUG_TRACE," <===Ecos_Handle_read\n");
		    return;
		}

		if (ethertype == ETH_P_PRE_AUTH)
		{
			DBGPRINT(RT_DEBUG_TRACE, "Receive WPA2 pre-auth packet for %s%d\n", rtapd->prefix_wlan_name, apidx);
		}
		else
		{
			DBGPRINT(RT_DEBUG_TRACE, "Receive EAP packet for %s%d\n", rtapd->prefix_wlan_name, apidx);
		}
    }
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, "Receive unexpected ethertype 0x%04X!!!\n", ethertype);
			DBGPRINT(RT_DEBUG_TRACE," <===Ecos_Handle_read\n");
		return;
	}

    pos = rec->xframe;
    
    //strip 4 bytes for valn tag
    if(isVlanTag)
    {
    	pos += 4;
    	left -= 4;
	}
    
	/* Check if this is a internal command or not */
	if (left == sizeof(RalinkIe) && 
#ifdef LINUX
		RTMPCompareMemory(pos, RalinkIe, 5) == 0
#endif //LINUX
#ifdef __ECOS
		DOT1X_RTMPCompareMemory(pos, RalinkIe, 5) == 0
#endif
	)
	{
		u8	icmd = *(pos + 5);
	
		switch(icmd)
		{
			case DOT1X_DISCONNECT_ENTRY:
			{
				struct sta_info *s;

				s = rtapd->sta_hash[STA_HASH(sa)];
				while (s != NULL && memcmp(s->addr, sa, 6) != 0)
				s = s->hnext;

				DBGPRINT(RT_DEBUG_TRACE, "Receive discard-notification form wireless driver.\n");
				if (s)
				{
					DBGPRINT(RT_DEBUG_TRACE,"This station(%02x:%02x:%02x:%02x:%02x:%02x) is removed.\n", MAC2STR(sa));
					Ap_free_sta(rtapd, s);						
				}
				else
				{
					DBGPRINT(RT_DEBUG_INFO, "This station(%02x:%02x:%02x:%02x:%02x:%02x) doesn't exist.\n", MAC2STR(sa));
				}
			}
			break;

			case DOT1X_RELOAD_CONFIG:
				Handle_reload_config(rtapd);
			break;

			default:
				DBGPRINT(RT_DEBUG_ERROR, "Unknown internal command(%d)!!!\n", icmd);
			break;
		}
	}
	else
	{
		/* Process the general EAP packet */
    	ieee802_1x_receive(rtapd, sa, &apidx, pos, left, ethertype, sock);
	}
			DBGPRINT(RT_DEBUG_TRACE," <===Ecos_Handle_read\n");
}


int Apd_init_sockets(rtapd *rtapd)
{
DBGPRINT(RT_DEBUG_TRACE, "===>Apd_init_sockets\n");
    struct ifreq ifr;
#ifndef __ECOS
	struct sockaddr_ll baddr;
#else
	struct protoent *p;
	struct sockaddr_in addr;
	char buf[6];
	//buf = (char *) malloc(6);

#endif
    int i;

	// 3. Get wireless interface MAC address
    for(i = 0; i < rtapd->conf->SsidNum; i++)
    {

    	memset(&ifr, 0, sizeof(ifr));
		sprintf(ifr.ifr_name, "%s%d",rtapd->prefix_wlan_name, i);
#ifdef LINUX 
		int s = -1;
	
		s = socket(AF_INET, SOCK_DGRAM, 0); 

		if (s < 0)
        {
            perror("socket[AF_INET,SOCK_DGRAM]");
    		return -1;
        }

    	//sprintf(ifr.ifr_name, "ra%d", i);
    
		// Get MAC address
    	if (ioctl(s, SIOCGIFHWADDR, &ifr) != 0)
    	{
        	perror("ioctl(SIOCGIFHWADDR)");
			close(s);
        	return -1;
    	}

    	DBGPRINT(RT_DEBUG_INFO," Device %s has ifr.ifr_hwaddr.sa_family %d\n",ifr.ifr_name, ifr.ifr_hwaddr.sa_family);
		if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER)
    	{
			DBGPRINT(RT_DEBUG_ERROR,"IF-%s : Invalid HW-addr family 0x%04x\n", ifr.ifr_name, ifr.ifr_hwaddr.sa_family);
			close(s);
			return -1;
		}

		memcpy(rtapd->own_addr[i], ifr.ifr_hwaddr.sa_data, ETH_ALEN);
#endif

#ifdef __ECOS
    	memset(buf, 0, 6);
	if((RT_ioctl(0, RT_PRIV_IOCTL, buf, 6, rtapd->prefix_wlan_name, i, RT_OID_802_11_MAC_ADDRESS)) != 0)
	{
		DBGPRINT(RT_DEBUG_ERROR,"ioctl failed for RT_OID_802_11_MAC_ADDRESS(len=%d, ifname=%s%d)\n", 6, rtapd->prefix_wlan_name,i);
		//free(buf);
		return -1;
	}
	memcpy(rtapd->own_addr[i], buf, ETH_ALEN);
	//free(buf);
#endif
    	DBGPRINT(RT_DEBUG_TRACE, "IF-%s MAC Address = " MACSTR "\n", ifr.ifr_name, MAC2STR(rtapd->own_addr[i]));

#ifdef LINUX 
		close(s);
#endif
	}	

DBGPRINT(RT_DEBUG_TRACE, "<===Apd_init_sockets\n");
	return 0;
}

static void Apd_cleanup(rtapd *rtapd)
{
	int i;

	for (i = 0; i < MAX_MBSSID_NUM; i++)
	{
		if (rtapd->wlan_sock[i] >= 0)
			close(rtapd->wlan_sock[i]);
		if (rtapd->eth_sock[i] >= 0)
			close(rtapd->eth_sock[i]);	
		if (rtapd->radius->mbss_auth_serv_sock[i] >=0)
			close(rtapd->radius->mbss_auth_serv_sock[i] );
	}	
	if (rtapd->ioctl_sock >= 0)
		close(rtapd->ioctl_sock);
    
	Radius_client_deinit(rtapd);

	Config_free(rtapd->conf);
	rtapd->conf = NULL;

	free(rtapd->prefix_wlan_name);
}

static int Apd_setup_interface(rtapd *rtapd)
{   
#if MULTIPLE_RADIUS
	int		i;
#endif
	DBGPRINT(RT_DEBUG_TRACE, "===>Apd_setup_interface\n");
	if (Apd_init_sockets(rtapd))
		return -1;    
    
	if (Radius_client_init(rtapd))
    {
		DBGPRINT(RT_DEBUG_ERROR,"RADIUS client initialization failed.\n");
		return -1;
	}

	if (ieee802_1x_init(rtapd))
    {
		DBGPRINT(RT_DEBUG_ERROR,"IEEE 802.1X initialization failed.\n");
		return -1;
	}
#if MULTIPLE_RADIUS
	for (i = 0; i < rtapd->conf->SsidNum; i++)
		DBGPRINT(RT_DEBUG_TRACE,"auth_serv_sock[%d] = %d\n", i, rtapd->radius->mbss_auth_serv_sock[i]);
#else	
    DBGPRINT(RT_DEBUG_TRACE,"rtapd->radius->auth_serv_sock = %d\n",rtapd->radius->auth_serv_sock);
#endif

	DBGPRINT(RT_DEBUG_TRACE, "<===Apd_setup_interface\n");
	return 0;
}

#ifndef __ECOS
static void usage(void)
{
	DBGPRINT(RT_DEBUG_OFF, "USAGE :  	rtdot1xd [optional command]\n");
	DBGPRINT(RT_DEBUG_OFF, "[optional command] : \n");
	DBGPRINT(RT_DEBUG_OFF, "-i <card_number> : indicate which card is used\n");
	DBGPRINT(RT_DEBUG_OFF, "-d <debug_level> : set debug level\n");
	
	exit(1);
}
#endif
static rtapd * Apd_init(const char *prefix_name)
{
	rtapd *rtapd;
	int		i;
	DBGPRINT(RT_DEBUG_TRACE,"Enter Apd_init\n");
	rtapd = malloc(sizeof(*rtapd));
	if (rtapd == NULL)
    {
		DBGPRINT(RT_DEBUG_ERROR,"Could not allocate memory for rtapd data\n");
		goto fail;
	}
	memset(rtapd, 0, sizeof(*rtapd));

	rtapd->prefix_wlan_name = strdup(prefix_name);
	if (rtapd->prefix_wlan_name == NULL)
    {
		DBGPRINT(RT_DEBUG_ERROR,"Could not allocate memory for prefix_wlan_name\n");
		goto fail;
	}

    // init ioctl socket
    rtapd->ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (rtapd->ioctl_sock < 0)
    {
	    DBGPRINT(RT_DEBUG_ERROR,"Could not init ioctl socket \n");	
	    goto fail;
    }
   

	rtapd->conf = Config_read(rtapd->ioctl_sock, rtapd->prefix_wlan_name);
	if (rtapd->conf == NULL)
    {
		DBGPRINT(RT_DEBUG_ERROR,"Could not allocate memory for rtapd->conf \n");
		goto fail;
	}

	for (i = 0; i < MAX_MBSSID_NUM; i++)
	{
		rtapd->wlan_sock[i] = -1;
		rtapd->eth_sock[i] = -1;
	}
	return rtapd;

fail:
	if (rtapd) {		
		if (rtapd->conf)
			Config_free(rtapd->conf);

		if (rtapd->prefix_wlan_name)
			free(rtapd->prefix_wlan_name);

		free(rtapd);
	}
	return NULL;
	
}

static void Handle_usr1(int sig, void *eloop_ctx, void *signal_ctx)
{
	struct hapd_interfaces *rtapds = (struct hapd_interfaces *) eloop_ctx;
	struct rtapd_config *newconf;
	int i;

	DBGPRINT(RT_DEBUG_TRACE,"Reloading configuration\n");
	for (i = 0; i < rtapds->count; i++)
    {
		rtapd *rtapd = rtapds->rtapd[i];
		newconf = Config_read(rtapd->ioctl_sock, rtapd->prefix_wlan_name);
		if (newconf == NULL)
        {
			DBGPRINT(RT_DEBUG_ERROR,"Failed to read new configuration file - continuing with old.\n");
			continue;
		}

		/* TODO: update dynamic data based on changed configuration
		 * items (e.g., open/close sockets, remove stations added to
		 * deny list, etc.) */
		Radius_client_flush(rtapd);
		Config_free(rtapd->conf);
		rtapd->conf = newconf;
        Apd_free_stas(rtapd);

#if MULTIPLE_RADIUS
		for (i = 0; i < MAX_MBSSID_NUM; i++)
			rtapd->radius->mbss_auth_serv_sock[i] = -1;
#else
		rtapd->radius->auth_serv_sock = -1;
#endif

	    if (Radius_client_init(rtapd))
        {
		    DBGPRINT(RT_DEBUG_ERROR,"RADIUS client initialization failed.\n");
		    return;
	    }
#if MULTIPLE_RADIUS
		for (i = 0; i < rtapd->conf->SsidNum; i++)
			DBGPRINT(RT_DEBUG_TRACE, "auth_serv_sock[%d] = %d\n", i, rtapd->radius->mbss_auth_serv_sock[i]);
#else
        DBGPRINT(RT_DEBUG_TRACE,"rtapd->radius->auth_serv_sock = %d\n",rtapd->radius->auth_serv_sock);
#endif
	}
}

void Handle_term(int sig, void *eloop_ctx, void *signal_ctx)
{
	//FILE    *f;
	//char    buf[256], *pos;
	//int     line = 0, i;
    //int     filesize,cur = 0;
    //char    *ini_buffer;             /* storage area for .INI file */

	DBGPRINT(RT_DEBUG_ERROR,"Signal %d received - terminating\n", sig);
	eloop_terminate();
}

#ifdef LINUX
int main(int argc, char *argv[])
#endif //LINUX//
#ifdef __ECOS
static void Cyg_DOT1X(cyg_addrword_t param)
#endif //__ECOS//
{
        int ret = 1, i;
        pid_t auth_pid=1;
        char prefix_name[IFNAMSIZ+1];


  	strcpy(prefix_name, "ra");
        DBGPRINT(RT_DEBUG_ERROR,"Start DOT1X Thread....\n");

        // set number of configuration file 1
        interfaces.count = 1;
        interfaces.rtapd = malloc(sizeof(rtapd *));
        if (interfaces.rtapd == NULL)
        {
            DBGPRINT(RT_DEBUG_ERROR,"malloc failed\n");
            exit(1);    
        }
        eloop_init(&interfaces);
        interfaces.rtapd[0] = Apd_init(prefix_name);
        if (!interfaces.rtapd[0])
            goto out;

        if (Apd_setup_interface(interfaces.rtapd[0]))
                goto out;

        eloop_run();

        Apd_free_stas(interfaces.rtapd[0]);
        ret=0;
        
out:
        for (i = 0; i < interfaces.count; i++)
        {
                if (!interfaces.rtapd[i])
                        continue;

                Apd_cleanup(interfaces.rtapd[i]);
                free(interfaces.rtapd[i]);
        }

        free(interfaces.rtapd);
        interfaces.rtapd = NULL;
        eloop_destroy();
        DBGPRINT(RT_DEBUG_ERROR,"DOT1X_exit\n");
        cyg_thread_exit();
        return 0;
}


#ifdef __ECOS
int DOT1X_Start(void)
{
	NDIS_802_11_AUTHENTICATION_MODE     AuthMode = Ndis802_11AuthModeMax;
	int                             IEEE8021X=0;
	char str[20],ifName[IFNAMSIZ];
	char *pEnd;
	int value = 0;
	int i=0;
	bool need_trigger=FALSE;
	bool any_if_up=FALSE;
	int flag;

	if(interfaces.rtapd != NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR,"DOT1X_Start already run\n");
		return;
        }

	CFG_get_str(CFG_WLN_BssidNum, str);
    	value = strtol(str, &pEnd, 10);
		
	for(i=0 ; i <value && i <16; i++)
	{
		memset(ifName,0x00,IFNAMSIZ);
		sprintf(ifName, "ra%d", i);
		if(netif_flag_get(ifName, &flag) && (flag&IFF_UP))
		{
			DBGPRINT(RT_DEBUG_ERROR,"%s is up ! \n",ifName);	
			any_if_up = TRUE;
		}
	}

	if(any_if_up == FALSE)
		return;
				

	
    	int ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);

	if (ioctl_sock < 0)
    	{
	    DBGPRINT(RT_DEBUG_ERROR,"Could not init ioctl socket \n");	
	    return;
    	}

	for(i=0 ; i <value ; i++)
	{
		AuthMode = Ndis802_11AuthModeOpen;
		IEEE8021X = 0;
		if((RT_ioctl(ioctl_sock, RT_PRIV_IOCTL, &AuthMode, sizeof(AuthMode), 0, i, OID_802_11_AUTHENTICATION_MODE)) != 0)
		{
			close(ioctl_sock);
			DBGPRINT(RT_DEBUG_ERROR,"ioctl failed for OID_802_11_AUTHENTICATION_MODE\n");
			return ;
		}

		if((RT_ioctl(ioctl_sock, RT_PRIV_IOCTL, &IEEE8021X, sizeof(IEEE8021X), 0, i, OID_802_11_SET_IEEE8021X)) != 0)
		{
			close(ioctl_sock);
			DBGPRINT(RT_DEBUG_ERROR,"ioctl failed for OID_802_11_SET_IEEE8021X\n");
			return ;
		}

		DBGPRINT(RT_DEBUG_ERROR,"AuthMode=%s, IEEE8021X=%d\n", GetAuthMode(AuthMode), IEEE8021X);
		if(AuthMode == Ndis802_11AuthModeWPA 
		|| AuthMode == Ndis802_11AuthModeWPA2 
		|| AuthMode == Ndis802_11AuthModeWPA1WPA2
		|| IEEE8021X == 1)
		{
			need_trigger = TRUE;
		}

	}
	close(ioctl_sock);
	register_netisr(NETISR_1X,dot1xintr);
	if(need_trigger == TRUE)
	{
		Dot_Running=TRUE;
                DBGPRINT(RT_DEBUG_ERROR,"Start DOT1X_Start.....(stack size=%d)\n",STACK_SIZE);
	    	cyg_thread_create(5,
	                       Cyg_DOT1X,
	                       0,
	                       "dot1x_daemon",
	                       Dot1x_main_stack,
	                       sizeof(Dot1x_main_stack),
	                       &dot1x_thread,
	                       &dot1x_thread_struct);

		cyg_thread_resume( dot1x_thread );
                cyg_thread_delay(100);
		return 0;
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR,"Don't need trigger the 1x Daemon\n");
	}
}

int DOT1X_Stop(void)
{
	int i;

	DBGPRINT(RT_DEBUG_ERROR,"DOT1X_Stop\n");
	if(interfaces.rtapd != NULL)
	{
                eloop_terminate();
                cyg_thread_delay(300);
                cyg_thread_delete(dot1x_thread);
	}
	else
		DBGPRINT(RT_DEBUG_ERROR,"1x daemon not running interfaces.rtapd == NULL\n");
}

void dot1x_hex_dump(char *str, unsigned char *pSrcBufVA, unsigned int SrcBufLen)
{
	unsigned char *pt;
	int x;
	pt = pSrcBufVA;
	diag_printf("%s: %p, len = %d\n",str,  pSrcBufVA, SrcBufLen);
	for (x=0; x<SrcBufLen; x++)
		{
		if (x % 16 == 0) 
			diag_printf("0x%04x : ", x);
		diag_printf("%02x ", ((unsigned char)pt[x]));
		if (x%16 == 15) diag_printf("\n");
		}
	diag_printf("\n");
}


void EcosSendToEth(char* iface, char* buf, size_t len)
{
	cyg_netdevtab_entry_t *t;
	struct eth_drv_sc *sc;
	struct ifnet *ifp;
	int found = 0;
	 struct mbuf *m = NULL;
		
	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;
		if (strcmp(sc->dev_name, iface) == 0) {
			found =1;
			break;
	        }
	}

	if ( found == 1 )
	{
		//ret=rt28xx_ap_ioctl(sc, param, (caddr_t)&wrq);
		diag_printf("find %s\n",iface);
		ifp = &sc->sc_arpcom.ac_if;
		m=Ecos_MemPool_Alloc(24,MemPool_TYPE_CLUSTER);
		//ether_output_frame(ifp, m);
		if(m == NULL)
		{
			diag_printf("m == NULL %s\n",iface);
		}
		else
		{
			m->m_pkthdr.rcvif = ifp;
			m->m_data = buf;
			m->m_pkthdr.len = len;
			m->m_len = m->m_pkthdr.len;

			if (IF_QFULL(&ifp->if_snd)) {
		                // Let the interface try a dequeue anyway, in case the
		                // interface has "got better" from whatever made the queue
		                // fill up - being unplugged for example.
		                if ((ifp->if_flags & IFF_OACTIVE) == 0)
		                    (*ifp->if_start)(ifp);
				IF_DROP(&ifp->if_snd);
				//senderr(ENOBUFS);
			}
			ifp->if_obytes += m->m_pkthdr.len;
			IF_ENQUEUE(&ifp->if_snd, m);

			if (m->m_flags & M_MCAST)
				ifp->if_omcasts++;
			if ((ifp->if_flags & IFF_OACTIVE) == 0)
				(*ifp->if_start)(ifp);
			//m_freem(m);
		}
	}
	else
	{
		diag_printf("not find %s\n",iface);
	}

}

unsigned char* Ecos_MemPool_Alloc (
	unsigned long Length,
	int Type)
{
    struct mbuf *pMBuf = NULL;

	switch (Type)
	{        
                case MemPool_TYPE_Header:
                        MGETHDR(pMBuf, M_DONTWAIT, MT_DATA);
                        break;
		case MemPool_TYPE_CLUSTER:
			MGETHDR(pMBuf, M_DONTWAIT, MT_DATA);
			if (pMBuf== NULL)
				return NULL;
			MCLGET(pMBuf, M_DONTWAIT);
			if ((pMBuf->m_flags & M_EXT) == 0)
                        {
				m_freem(pMBuf);
                                return NULL;
                        }
                        break;
                default:
                        DBGPRINT(RT_DEBUG_ERROR, ("%s: Unknown Type %d\n", __FUNCTION__, Type));
                        break;
	}
    
    return pMBuf;
}


void Dot1x_Reboot(void)
{
        DBGPRINT(RT_DEBUG_ERROR, "Dot1x_Reboot\n");
	DOT1X_Stop();
	DOT1X_Start();
}

void Dot1x_debug_level(int level)
{
        if (level >= 0)
                DOT1X_RTDebugLevel = level;

        DBGPRINT(RT_DEBUG_ERROR, "Dot1x debug level = %d\n", DOT1X_RTDebugLevel);
}
#endif//__ECOS//
