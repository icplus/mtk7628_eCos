/*
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <pppd.h>
#include <errno.h>
#include <cfg_def.h>

typedef int STATUS;

bool ppp_up_flag = false;
/*
 * output - Output PPP packet.
 */
void
output(PPP_IF_VAR_T *pPppIf, u_char *p, int len)
{
    if (pppwrite(pPppIf, (char *) p, len) != OK) 
    {
	    MAINDEBUG("write error");
	    die(pPppIf, 1);
    }
}

/*
 * ppp_send_config - configure the transmit characteristics of
 * the ppp interface.
 */
void
ppp_send_config(PPP_IF_VAR_T *pPppIf, int mtu, u_long asyncmap, int pcomp, int accomp)
{
    u_int x;

    if (pPppIf->pIoctl(pPppIf, SIOCSIFMTU, (caddr_t) &mtu) < 0)
    {
	    MAINDEBUG("ioctl(SIOCSIFMTU) error");
	    die(pPppIf, 1);
    }

    if (pPppIf->pIoctl(pPppIf, PPPIOCSASYNCMAP, (caddr_t) &asyncmap) < 0) 
    {
	    MAINDEBUG("ioctl(PPPIOCSASYNCMAP) error");
	    die(pPppIf, 1);
    }

    if (pPppIf->pIoctl(pPppIf, PPPIOCGFLAGS, (caddr_t) &x) < 0)
    {
	    MAINDEBUG("ioctl (PPPIOCGFLAGS) error");
	    die(pPppIf, 1);
    }

    x = pcomp? x | SC_COMP_PROT: x &~ SC_COMP_PROT;
    x = accomp? x | SC_COMP_AC: x &~ SC_COMP_AC;
    if (pPppIf->pIoctl(pPppIf, PPPIOCSFLAGS, (caddr_t) &x) < 0)
    {
	    MAINDEBUG("ioctl(PPPIOCSFLAGS) error");
	    die(pPppIf, 1);
    }
}


/*
 * ppp_set_xaccm - set the extended transmit ACCM for the interface.
 */
void
ppp_set_xaccm(PPP_IF_VAR_T *pPppIf, ext_accm accm)
{
    if (pPppIf->pIoctl(pPppIf, PPPIOCSXASYNCMAP, (caddr_t) accm) < 0)
        MAINDEBUG("ioctl(set extended ACCM): error");
}


/*
 * ppp_recv_config - configure the receive-side characteristics of
 * the ppp interface.
 */
void
ppp_recv_config(PPP_IF_VAR_T *pPppIf, int mru, 
                u_long asyncmap, int pcomp,
		        int accomp)
{
    int x;

    if (pPppIf->pIoctl(pPppIf, PPPIOCSMRU, (caddr_t) &mru) < 0)
    {
        MAINDEBUG("ioctl(PPPIOCSMRU): error");
	    die(pPppIf, 1);
    }

    if (pPppIf->pIoctl(pPppIf, PPPIOCSRASYNCMAP, (caddr_t) &asyncmap) < 0)
    {
        MAINDEBUG("ioctl(PPPIOCSRASYNCMAP): error");
	    die(pPppIf, 1);
    }

    if (pPppIf->pIoctl(pPppIf, PPPIOCGFLAGS, (caddr_t) &x) < 0)
    {
        MAINDEBUG("ioctl (PPPIOCGFLAGS): error");
	    die(pPppIf, 1);
    }

    x = !accomp? x | SC_REJ_COMP_AC: x &~ SC_REJ_COMP_AC;
    if (pPppIf->pIoctl(pPppIf, PPPIOCSFLAGS, (caddr_t) &x) < 0) 
    {
        MAINDEBUG("ioctl(PPPIOCSFLAGS): error");
	    die(pPppIf, 1);
    }
}

/*
 * sifvjcomp - config tcp header compression
 */
int
sifvjcomp(pPppIf, vjcomp, cidcomp, maxcid)
    PPP_IF_VAR_T *pPppIf;
    int vjcomp, cidcomp, maxcid;
{
    u_int x;

    if (pPppIf->pIoctl(pPppIf, PPPIOCGFLAGS, (caddr_t) &x) < 0)
    {
	    MAINDEBUG("ioctl (PPPIOCGFLAGS) error");
	    return 0;
    }

    x = vjcomp ? x | SC_COMP_TCP: x &~ SC_COMP_TCP;
    x = cidcomp? x & ~SC_NO_TCP_CCID: x | SC_NO_TCP_CCID;
    if (pPppIf->pIoctl(pPppIf, PPPIOCSFLAGS, (caddr_t) &x) < 0)
    {
	    MAINDEBUG("ioctl(PPPIOCSFLAGS) error");
	    return 0;
    }

    if (pPppIf->pIoctl(pPppIf, PPPIOCSMAXCID, (caddr_t) &maxcid) < 0)
    {
        MAINDEBUG("ioctl(PPPIOCSFLAGS): error");
        return 0;
    }
    return 1;
}

/*
 * sifup - Config the interface up and enable IP packets to pass.
 */
int
sifup(PPP_IF_VAR_T *pPppIf)
{
    int flags;
    u_int x;
	
    if (IfLib_GetFlag (&flags, pPppIf->pppname) == OK)
    {
        flags |= IFF_UP;
        IfLib_SetFlag(flags, pPppIf->pppname);
    }

    if (pPppIf->pIoctl(pPppIf, PPPIOCGFLAGS, (caddr_t) &x) < 0)
    {
        MAINDEBUG("ioctl (PPPIOCGFLAGS): error");
        return 0;
    }

    x |= SC_ENABLE_IP;
    if (pPppIf->pIoctl(pPppIf, PPPIOCSFLAGS, (caddr_t) &x) < 0)
    {
        MAINDEBUG("ioctl(PPPIOCSFLAGS): error");
        return 0;
    }

    return 1;
}

/*
 * sifdown - Config the interface down and disable IP.
 */
int
sifdown(PPP_IF_VAR_T *pPppIf)
{
    int flags;
    u_int x;
	//int rv,s;
	int rv;
    struct ppp_softc *sc;
	
	sc = pPppIf->pSc;

    rv = 1;
    if (pPppIf->pIoctl(pPppIf, PPPIOCGFLAGS, (caddr_t) &x) < 0)
    {
        MAINDEBUG("ioctl (PPPIOCGFLAGS): error");
        rv = 0;
    }
    else
    {
        x &= ~SC_ENABLE_IP;
        if (pPppIf->pIoctl(pPppIf, PPPIOCSFLAGS, (caddr_t) &x) < 0)
        {
            MAINDEBUG("ioctl(PPPIOCSFLAGS): error");
            rv = 0;
        }
    }
    if (IfLib_GetFlag (&flags, pPppIf->pppname) == OK)
    {
        flags &= ~IFF_UP;
        IfLib_SetFlag(flags, pPppIf->pppname);
    }

    return rv;
}

/*
 * sifaddr - Config the interface IP addresses and netmask.
 */
/*
 * SET_SA_FAMILY - set the sa_family field of a struct sockaddr,
 * if it exists.
 */
#define SET_SA_FAMILY(addr, family)		\
    BZERO((char *) &(addr), sizeof(addr));	\
    addr.sa_family = (family);			\

#include <network.h>
//struct bootp ppp_bootp_data;
//extern struct bootp lan_bootp_data;

#include <cfg_net.h>
#include <sys_status.h>
#include <eventlog.h>

int
sifaddr(PPP_IF_VAR_T *pPppIf)
{
	ipcp_options *ho = &pPppIf->ipcp_hisoptions;
	ipcp_options *go = &pPppIf->ipcp_gotoptions;
	lcp_options *lao = &pPppIf->lcp_allowoptions;
	lcp_options *lho = &pPppIf->lcp_hisoptions;
	
	if (pPppIf->defaultroute)
	{
		extern struct ip_set wan_ip_set;
		int ipmode;
		
		CFG_get(CFG_WAN_IP_MODE, &ipmode);
		
		#ifdef CONFIG_PPTP_PPPOE
		int pptp_wanif;
		if(ipmode==4){
		  CFG_get( CFG_PTP_WANIF, &pptp_wanif );
		  if(pptp_wanif==2)
			ipmode =3;//pppoe mode for pptp vpn
		}
        #endif
		#ifdef CONFIG_L2TP_OVER_PPPOE
		int l2tp_wanif;
		if(ipmode == L2TPMODE){
		  CFG_get( CFG_L2T_WANIF, &l2tp_wanif);
		  if((l2tp_wanif == 2) && (!strcmp(pPppIf->pppname,"ppp0")))
			ipmode = PPPOEMODE;//pppoe mode for l2tp over pppoe
		}
		#endif	
		buid_ip_set(&wan_ip_set,
					pPppIf->pppname,
					go->ouraddr,
					0xffffffff,
					ho->hisaddr,
					ho->hisaddr,
					ho->hisaddr,
					MIN(lao->mru? lao->mru: MTU,
					(lho->neg_mru? lho->mru: MTU)),
					ipmode,
					NULL,
					NULL);
					
		SYS_wan_mode = ipmode;
		
		// Only send here...
		//timeout(mon_snd_cmd, MON_CMD_WAN_INIT, 5);
		cyg_thread_delay(30);
		ppp_up_flag = true;
		mon_snd_cmd( MON_CMD_WAN_INIT );
	}
	else
	{
		static struct ip_set setp;
		int ipmode;
		
		CFG_get( CFG_WAN_IP_MODE, &ipmode);
		
		buid_ip_set(&setp, 
					pPppIf->pppname, 
					go->ouraddr, 
					0xffffffff,
					ho->hisaddr, 
					ho->hisaddr,
					ho->hisaddr,
					MIN(lao->mru? lao->mru: MTU, (lho->neg_mru? lho->mru: MTU)),
					ipmode,
					NULL,
					NULL);
					
		show_ip_set(&setp);
		netif_cfg(pPppIf->pppname, &setp);
		#ifdef CONFIG_PPTP_PPPOE
		extern struct ip_set ppp_ip_set[CONFIG_PPP_NUM_SESSIONS];
		int pptp_wanif;
		if(ipmode == 4){
	      CFG_get( CFG_PTP_WANIF, &pptp_wanif);
		  if(pptp_wanif==2){	
			memcpy(&ppp_ip_set[0],&setp,sizeof(struct ip_set));	
			timeout(mon_snd_cmd, MON_CMD_VPN_INIT, 10);//this is for pptp vpn
		  }
		}
		#endif		
	}
	
	return 1;
}

/*
 * cifaddr - Clear the interface IP addresses, and delete routes
 * through the interface if possible.
 */
int
cifaddr(PPP_IF_VAR_T *pPppIf, int ouraddr, int hisaddr)
{
	int sockfd = 0;
	int status;
	struct ifreq ifr;
	
	if ( (sockfd = socket (AF_INET, SOCK_RAW, 0)) < 0) 
		return 0;
	
	/*
	* Deleting the interface address is required to correctly scrub the
	* routing table based on the current netmask.
	*/
	bzero ( (char *)&ifr, sizeof (struct ifreq));
	
	strcpy(ifr.ifr_name, pPppIf->pppname);
	ifr.ifr_addr.sa_len = sizeof (struct sockaddr_in);
	((struct sockaddr_in *) &ifr.ifr_addr)->sin_family = AF_INET;
	//((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr = ouraddr;
	//
	//
	//status = ioctl (sockfd, SIOCDIFADDR, (int)&ifr);
	//if (status < 0) 
	//{
	//	// Sometimes no address has been set, so ignore that error.
	//	if (errno != EADDRNOTAVAIL)
	//	{
	//		close (sockfd);
	//		return 0;
	//	}
	//}
	//close (sockfd);
	
    while (ioctl(sockfd, SIOCGIFADDR, (int) &ifr) == 0)
    {
        status = ioctl (sockfd, SIOCDIFADDR, (int)&ifr);
        if (status < 0) 
        {
            // Sometimes no address has been set, so ignore that error.
            if (errno != EADDRNOTAVAIL)
            {
                close (sockfd);
                return 0;
            }
        }
    }
    close (sockfd);
	
	return 1;
}

/*
 * cifdefaultroute - delete a default route through the address given.
 */
int
cifdefaultroute(u_long ouraddr)
{
    u_long default_route = 0;
    
    // Verify this interface is default route.
    if (default_route == htonl(ouraddr))
    {
        // Clear default route
        dodefaultroute(0);
    }

    return 1;
}

/*
 * sifdns - Config DNS proxy server.
 */
int sifdns(PPP_IF_VAR_T *pPppIf)
{
	int num = 0;
	in_addr_t dns[2] = {0};
	
	ipcp_options *go = &pPppIf->ipcp_gotoptions;
	
	if(go->neg_pdns)
	{
		diag_printf("Primary DNS address %s\n", ip_ntoa(go->pdnsaddr));
		SysLog(LOG_USER|SYS_LOG_NOTICE|LOGM_PPPD,"Got Primary DNS address %s",ip_ntoa(go->pdnsaddr));
		
		dns[num] = go->pdnsaddr;
		num++;
	}
	
	if(go->neg_sdns)
	{
		diag_printf("Secondary DNS address %s\n", ip_ntoa(go->sdnsaddr));
		SysLog(LOG_USER|SYS_LOG_NOTICE|LOGM_PPPD,"Got Secondary DNS address %s",ip_ntoa(go->sdnsaddr));
		
		dns[num] = go->sdnsaddr;
		num++;
	}
#if defined(CONFIG_PPTP_PPPOE) || defined(CONFIG_L2TP_OVER_PPPOE) 	
	if(!strcmp(pPppIf->pppname,"ppp1"))//note rule:ppp1 fixed for pptp vpn use. the wan mode:pppoe/pptp/l2tp use fixed ppp0. 
	{	//dns[0]=0x0a05050c;dns[1]=0x0a05050d;num=2;//for debug
		DNS_CFG_autodns_update_append(num, (char *)dns);
	}
	else
	{	//dns[0]=0x0a05050a;dns[1]=0x0a05050b;num=2;//for debug
		DNS_CFG_autodns_update(num, (char *)dns);
	}
#else	
	DNS_CFG_autodns_update(num, (char *)dns);
#endif//#ifdef CONFIG_PPTP_PPPOE
	return 1;
}

char *stringdup(char *in)
{
    char* dup;
    
    if ((dup = (char *)malloc(strlen(in) + 1)) == NULL)
        return NULL;

    (void) strcpy(dup, in);
    return (dup);
}
////////////////////////////////////////////////////////////////////////
// The following need to implement

STATUS IfLib_GetFlag (int *pFlags, char *pIfName)
{
    struct ifreq  ifr;
    int so;
    int status;
    
    strncpy (ifr.ifr_name, pIfName, sizeof (ifr.ifr_name));
    if ((so = socket (AF_INET, SOCK_RAW, 0)) < 0)
        return ERROR;

    status = ioctl (so, SIOCGIFFLAGS, (int)&ifr);
        (void)close (so);
    
    if (status != 0)
        return ERROR;
        
    *pFlags = ifr.ifr_flags;	
    return OK; 
	
}
STATUS IfLib_SetFlag(int flags, char *pIfName)
{
    struct ifreq  ifr;
    int so;
    int status;
    
    strncpy (ifr.ifr_name, pIfName, sizeof (ifr.ifr_name));	
    
    ifr.ifr_flags = (short) flags;	
    
    if ((so = socket (AF_INET, SOCK_RAW, 0)) < 0)
        return ERROR;
    
    status = ioctl (so, SIOCSIFFLAGS, (int)&ifr);
    (void)close (so);
    
    if (status != 0)
      return ERROR;
	
    return OK;	
    
}
static int rtm_seq;	
int dodefaultroute (u_int32_t g, int cmd)
{
	int routes;
    struct {
	struct rt_msghdr	hdr;
	struct sockaddr_in	dst;
	struct sockaddr_in	gway;
	struct sockaddr_in	mask;
    } rtmsg;

    if ((routes = socket(PF_ROUTE, SOCK_RAW, AF_INET)) < 0) {
    	#if 0
	syslog(SYS_LOG_ERR, "Couldn't %s default route: socket: %m",
	       cmd=='s'? "add": "delete");
	#endif       
	return 0;
    }

    memset(&rtmsg, 0, sizeof(rtmsg));
    rtmsg.hdr.rtm_type = cmd == 's'? RTM_ADD: RTM_DELETE;
    rtmsg.hdr.rtm_flags = RTF_UP | RTF_GATEWAY;
    rtmsg.hdr.rtm_version = RTM_VERSION;
    rtmsg.hdr.rtm_seq = ++rtm_seq;
    rtmsg.hdr.rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;
    rtmsg.dst.sin_len = sizeof(rtmsg.dst);
    rtmsg.dst.sin_family = AF_INET;
    rtmsg.gway.sin_len = sizeof(rtmsg.gway);
    rtmsg.gway.sin_family = AF_INET;
    rtmsg.gway.sin_addr.s_addr = g;
    rtmsg.mask.sin_len = sizeof(rtmsg.dst);
    rtmsg.mask.sin_family = AF_INET;

    rtmsg.hdr.rtm_msglen = sizeof(rtmsg);
    if (write(routes, &rtmsg, sizeof(rtmsg)) < 0) {
	//diag_printf("Couldn't %s default route",cmd=='s'? "add": "delete");
	close(routes);
	return 0;
    }

    close(routes);
    //default_route_gateway = (cmd == 's')? g: 0;
    return 1;
}
//int HostTbl_GetByName (char *name)
//{
//}						

//#define _string(s) #s
//#define	STR(s)	_string(s)

void
netif_set_mtu(PPP_IF_VAR_T *pPppIf, int mtu)
{
	if (pPppIf->pIoctl(pPppIf, SIOCSIFMTU, (caddr_t) &mtu) < 0)
    {
	    MAINDEBUG("ioctl(SIOCSIFMTU) error");
	}    
}		

int
netif_get_mtu(PPP_IF_VAR_T *pPppIf)
{
	int mtu;
	
	if (pPppIf->pIoctl(pPppIf, SIOCGIFMTU, (caddr_t) &mtu) < 0)
    {
	    MAINDEBUG("ioctl(SIOCGIFMTU) error");
	}    
	return mtu;
}


