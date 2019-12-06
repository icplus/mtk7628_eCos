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

#define NPKTFTR 2
//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <network.h>
#include <sys/mbuf.h>
#include <netinet/if_ether.h>
#include <net/if_dl.h>
#include <lib_packet.h>
#if NPKTFTR > 0
#include <net/pktftr.h>
#endif

//==============================================================================
//                                    MACROS
//==============================================================================

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
struct eth_input_func *eth_input_forward_funclist = NULL;
struct eth_input_func *eth_input_host_funclist = NULL;
struct pktsocket *pktsocket_all = NULL;

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================
static char * _ether_aton __P((char *, struct ether_addr *));

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================



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
char *ether_ntoa(e)
	struct ether_addr *e;
{
	static char a[] = "00:00:00:00:00:00";
	unsigned char *p;
	if(e)
	{
		p=(unsigned char *)&e->octet[0];
		(void)sprintf(a, "%02X:%02X:%02X:%02X:%02X:%02X", p[0], p[1], p[2], p[3], p[4], p[5]);
	}
	return (a);
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
static char *_ether_aton(s, e)
	char *s;
	struct ether_addr *e;
{
	int i;
	long l;
	char *pp;

	while (isspace(*s))
		s++;

	/* expect 6 hex octets separated by ':' or space/NUL if last octet */
	for (i = 0; i < 6; i++) {
		l = strtol(s, &pp, 16);
		if (pp == s || l > 0xFF || l < 0)
			return (NULL);
		if (!(*pp == ':' || (i == 5 && (isspace(*pp) || *pp == '\0'))))
			return (NULL);
//Eddy		e->ether_addr_octet[i] = (u_char)l;
		e->octet[i] = (u_char)l;
		s = pp + 1;
	}

	/* return character after the octets ala strtol(3) */
	return (pp);
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
struct ether_addr *ether_aton(s)
	char *s;
{
	static struct ether_addr n;

	return (_ether_aton(s, &n) ? &n : NULL);
}


void ether_aton_r(char *s, char *mac)
{
	memset(mac, 0, 6);
	_ether_aton(s, (struct ether_addr *)mac);
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
int LIB_ether_input_register(struct ifnet *ifp, pfun func, int flag)
{
	struct eth_input_func *newfunc, **qname;
	int s;
	
	if(!func || !ifp)
		return 0;
	
	newfunc = (struct eth_input_func *)malloc(sizeof(struct eth_input_func));
	if(!newfunc)
		return 0;
	newfunc->ifp = ifp;
	newfunc->func = func;
	newfunc->next = NULL;
	
	if(flag == FORWARDQ)
		qname = &eth_input_forward_funclist;
	else
		qname = &eth_input_host_funclist;
	s = splimp();
	newfunc->next = *qname;
	*qname = newfunc;
	splx(s);
	
	return (int)newfunc;
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
int LIB_ether_input_register_tail(struct ifnet *ifp, pfun func, int flag)
{
	struct eth_input_func *newentry, **qname;
	struct eth_input_func *oldentry;
	int s;
	
	if(!func || !ifp)
		return 0;
	
	if(flag == FORWARDQ)
		qname = &eth_input_forward_funclist;
	else
		qname = &eth_input_host_funclist;
	
	oldentry = *qname;
	while(oldentry){
		if(oldentry->next == NULL)
			break;
		oldentry = oldentry->next;
	}
		
	newentry = (struct eth_input_func *)malloc(sizeof(struct eth_input_func));
	if(!newentry)
		return 0;
		
	newentry->ifp = ifp;
	newentry->func = func;
	newentry->next = NULL;
	
	s = splimp();
	if(oldentry)
		oldentry->next = newentry;
	else
		*qname = newentry;
	splx(s);
	
	return (int)newentry;
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
int LIB_ether_input_unregister(struct ifnet *ifp, pfun func, int flag)
{
	struct eth_input_func *entry, **qname;
	struct eth_input_func *prv = 0;
	int s;
	
	if(flag == FORWARDQ)
		qname = &eth_input_forward_funclist;
	else
		qname = &eth_input_host_funclist;
		
	s = splimp();
	entry = *qname;
	while(entry)
	{
		if((entry->ifp == ifp) && (entry->func == func))
		{
			if(prv)
				prv->next = entry->next;
			else
				*qname = entry->next;
			splx(s);
			free(entry);
			return 0;
		}
		prv = entry;
		entry = entry ->next;
	}
	splx(s);
	
	return -1;
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
inline int LIB_ether_input_func(struct ifnet *ifp, char *head, struct mbuf *m, int flag)
{
	int rc, s;
	struct eth_input_func *entry;
	
	if(flag == FORWARDQ)
		entry = eth_input_forward_funclist;
	else
		entry = eth_input_host_funclist;
	s = splimp();
	while(entry)
	{
		if((entry->ifp == m->m_pkthdr.rcvif) && (rc = entry->func(ifp, head, m, entry))){
            splx(s);
			return rc;
		}
		
		entry = entry ->next;
	}
	splx(s);
	
	return 0;
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
void link_addr(const char *addr,struct sockaddr_dl *sdl)
{
        char *octetp, *octetplim;
        int len;
        const char *start;
        
        start = addr;
        octetplim = sdl->sdl_len + (char *)(void *)sdl;
        octetp = sdl->sdl_data;

        (void)memset(&sdl->sdl_family, 0, (size_t)sdl->sdl_len - 1);
        sdl->sdl_family = AF_LINK;

        /* First, try to read a device name and unit, and a
         * colon---i.e., match "[a-zA-Z][a-zA-Z]+[0-9]+:".  If no
         * device is named, skip to reading an address.
         */
        while (*addr != 0 && isalpha(*addr)) addr++;

        if (*addr != 0 && addr != start) {
                const char *start2 = addr;
                while (*addr != 0 && isdigit(*addr)) addr++;
                if (start2 != addr && *addr == '\0') {
                        sdl->sdl_nlen = addr - start;
                        (void)memcpy(sdl->sdl_data, start, sdl->sdl_nlen);
                        octetp = &sdl->sdl_data[sdl->sdl_nlen];
                        addr++;
                } else
                        addr = start;
        } else
                addr = start;

        sdl->sdl_alen = octetp - LLADDR(sdl);
        len = octetp - (char *)(void *)sdl;
        if ((size_t) len > sizeof(*sdl))
                sdl->sdl_len = len;
        return;
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
int LIB_pktsocket_create(char *inf, pfun func)
{
	struct ifnet *ifp;
	struct pktsocket *pkts;
	int s;
	
	ifp = ifunit(inf);
	if((pkts = malloc(sizeof(struct pktsocket))) == NULL)
	{
		return (int)NULL;
	}
	cyg_mbox_create(&(pkts->Qhandler), &(pkts->Qobject));
	if((pkts->handler = LIB_ether_input_register(ifp, func, HOSTQ)) == NULL)
	{
		cyg_mbox_delete(pkts->Qhandler);
		free(pkts);
		return (int)NULL;
	}
	s = splnet ();
	pkts->next = pktsocket_all;
	pktsocket_all = pkts;
	splx (s);
	return (int)pkts;
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
void LIB_pktsocket_close(int s)
{
	struct pktsocket *pkts =(struct pktsocket *)s;
	struct pktsocket *temps, *pre=0;
	struct rcv_info *info;
	int _ms;
	
	_ms = splnet();
	for(temps = pktsocket_all; temps; temps = temps->next)
    {
    	if(temps == pkts)
    	{
    		if(pre)
    			pre->next = temps->next;
    		else
    			pktsocket_all = temps->next;
    			
    		break;
    	}
    	pre = temps;
    }
    splx(_ms);
	if(!temps)	return;
	
	LIB_ether_input_unregister(pkts->handler->ifp, pkts->handler->func, HOSTQ);
	/* flush all memories in quene */
	while(cyg_mbox_peek(pkts->Qhandler))
	{
		info = (struct rcv_info *)cyg_mbox_tryget(pkts->Qhandler);
		if(info->m)
			m_freem(info->m);
		free(info);
	}
	cyg_mbox_delete(pkts->Qhandler);
	free(pkts);
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
int LIB_pktsocket_enQ(struct ifnet *ifp, char *head, struct mbuf *m, struct eth_input_func *hn)
{
	struct pktsocket *pkts;
	struct rcv_info *info;
	
    for(pkts = pktsocket_all; pkts; pkts = pkts->next)
    {
    	if(pkts->handler == hn)
    		break;
    }
    if(!pkts) 
    	return -1;
    	
   	if((info = (struct rcv_info *)malloc(sizeof(struct rcv_info))) == NULL)
    	return -1;
    info->ifp = ifp;
    info->m = m;
    info->head = head;
    info->len = m->m_pkthdr.len;
    if(!cyg_mbox_tryput(pkts->Qhandler, info))
    {
		/* fail to put message, free info and mbuf */
		free(info);
		return -1;
	}
	return 0;
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
struct rcv_info * LIB_pktsocket_deQ(struct pktsocket *pkts, struct timeval *tv)
{
	struct pktsocket *temps;
	cyg_tick_count_t then;
	cyg_resolution_t resolution = 
        cyg_clock_get_resolution(cyg_real_time_clock());
   
    /* check pkts availablility */
    for(temps = pktsocket_all; temps; temps = temps->next)
    {
    	if(temps == pkts)
    		break;
    }
    if(!temps) 
    	return 0;
    then = tv->tv_sec;   
	then *= 1000000000; // into nS - we know there is room in a tick_count_t
    then = (then / resolution.dividend) * resolution.divisor; // into system ticks
   
	return (struct rcv_info *)cyg_mbox_timed_get(pkts->Qhandler, cyg_current_time()+then);
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
int LIB_rcvinfo_read(int fd, char *buf, int len)
{
	int clen;
	struct rcv_info *info=(struct rcv_info *)fd;
	
	if(info->m == NULL)
		return -1;
		
	if((info->len) >= len)
		clen = len;
	else
		clen = info->len;
	//memcpy(buf, info->buf, clen);
	m_copydata(info->m, (char *)(info->buf)-(char *)(info->m->m_data), clen, buf);
	info->buf += len;
	return clen;
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
void LIB_rcvinfo_close(int fd)
{
	struct rcv_info *info=(struct rcv_info *)fd;
	if(info->m)
		m_freem(info->m);
	free(info);
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
void LIB_rcvinfo_getifp(int fd, int *ifp)
{
	struct rcv_info *info=(struct rcv_info *)fd;
	*ifp = (int)(info->ifp);
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
int LIB_etherpacket_send(struct ifnet *ifp, struct mbuf *m, unsigned char *dst_hwaddr, char *data, int len)
{
	int oldlev;
	struct sockaddr sa;
	struct ether_header *eh;
	
	if(m == NULL)
	{
    	if ((m = m_devget (data, len, 0, ifp, NULL)) == NULL)
	    	return false;
	}
	
	if (dst_hwaddr == NULL) {
		/*  Assume the sender has construct the entire frame  */
		oldlev = splnet();
		if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING)) {
			splx(oldlev);
			goto send_bad;
		}
		
		IF_ENQUEUE(&ifp->if_snd, m);
		if ((ifp->if_flags & IFF_OACTIVE) == 0)
			(*ifp->if_start)(ifp);
		splx(oldlev);
	}
	else {
		/* construct ether head */
		sa.sa_family = AF_UNSPEC;
		sa.sa_len = sizeof(sa);
		eh = (struct ether_header *)sa.sa_data;
		bcopy((caddr_t)dst_hwaddr, (caddr_t)eh->ether_dhost, sizeof(eh->ether_dhost));
	
		eh->ether_type = htons(ETHERTYPE_IP);	/* if_output will not swap */
	
		/* call interface's output routine */
		oldlev = splnet ();
		(* ifp->if_output) (ifp, m, &sa, (struct rtentry *)NULL);
		splx (oldlev);
	}
    return true;

send_bad:
	if (m != NULL)
		m_freem(m);
	return false;
}


#if NPKTFTR > 0
//------------------------------------------------------------------------------
// FUNCTION: LIB_pktftr_reg
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
static int pktftr_hdl = (int) LIB_etherpacket_send;
int LIB_pktftr_reg(char *ifname, caddr_t in_filter, caddr_t out_filter)
{
	struct ifnet *ifp;
	struct pktftr_s *pfilter;
	int s, return_val;

	if (pktftr_hdl == (int)LIB_etherpacket_send)
		pktftr_hdl &= 0xffffff;

	if ((ifp=ifunit(ifname)) == NULL)
		return -1;

	if (in_filter == NULL && out_filter == NULL)
		return -1;

	pfilter = (struct pktftr_s *) malloc(sizeof(struct pktftr_s));
	if (pfilter == NULL)
		return -1;

	pfilter->in_filter = in_filter;
	pfilter->out_filter = out_filter;
	pfilter->next = NULL;
	pfilter->hdl = -1;

	s = splnet();
	/*  XXX: Allow only one filter per interface now.  */
	if (ifp->if_pktftr == NULL) {
		pfilter->hdl = pktftr_hdl ++;
		ifp->if_pktftr = pfilter;
                return_val = pfilter->hdl;
	} else {
	        free(pfilter);
                return_val = -1;
	}
	splx(s);

	return return_val;
}


//------------------------------------------------------------------------------
// FUNCTION: LIB_pktftr_dereg
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
int LIB_pktftr_dereg(char *ifname, int hdl)
{
	struct ifnet *ifp;
	struct pktftr_s *pfilter=NULL;
	int s;

	if ((ifp=ifunit(ifname)) == NULL)
		return -1;

	s = splnet();
	if ((ifp->if_pktftr != NULL) && (ifp->if_pktftr->hdl == hdl)) {
		pfilter = ifp->if_pktftr;
		ifp->if_pktftr = NULL;
	}
	splx(s);
	
	if (pfilter != NULL) {
		free(pfilter);
		return 0;
	}

	return -1;
}



#endif	/*  NPKTFTR > 0  */

