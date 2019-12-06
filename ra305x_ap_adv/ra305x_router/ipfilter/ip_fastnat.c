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

#define FAST_PATH
#ifdef FAST_PATH

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <network.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/udp.h>
#include <ip_compat.h>
#include <ip_fil.h>
#include <ip_nat.h>
#include <ip_frag.h>
#include <cfg_net.h>
#include <sys_status.h>

//==============================================================================
//                                    MACROS
//==============================================================================

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
extern u_long fr_tcpidletimeout;
extern int (*ip_fastpath)();

#ifdef CONFIG_DROP_ILLEGAL_TCPRST
extern u_short	ip_id;
#endif

#ifdef CONFIG_NAT_FORCE_TCPWIN
extern void nat_adj_tcpwindow(tcphdr_t *, int, u_short *);
#endif


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
int nat_set_fcache(fr_info_t *fin, nat_t *nat)
{
	struct sockaddr_in sin = {sizeof(sin), AF_INET };
	char interface_name[16];

if (strstr(getifname(nat->nat_ifp),"eth"))
	sprintf(interface_name,"%s%d",getifname(nat->nat_ifp),((struct ifnet *)nat->nat_ifp)->if_unit);
else
	sprintf(interface_name,"%s",getifname(nat->nat_ifp));
	/* through ALG can not set into cache */	
	if(	(nat->nat_aps != NULL) ||
		(nat->nat_flags & IPN_FASTCACHE))
		return -1;
	if((nat->nat_p != IPPROTO_TCP) && (nat->nat_p != IPPROTO_UDP))
		return -1;
	if(strcmp(interface_name, SYS_wan_if))
		return -1;
	if(nat->nat_inip.s_addr == SYS_lan_ip)
		return -1;
	if(nat->nat_inip.s_addr == SYS_wan_ip)
		return -1;
	
	/* obtain destination route entry */
	sin.sin_addr = nat->nat_oip;
	nat->nat_rt = (struct rtentry *)rtalloc1((struct sockaddr *)&sin, 0, 0UL);
	if(nat->nat_rt == 0)
		return -1;
	/* obtain source route entry and interface */
	sin.sin_addr = nat->nat_inip;
	nat->nat_ort = (struct rtentry *)rtalloc1((struct sockaddr *)&sin, 0, 0UL);
	if(nat->nat_ort == 0)
	{
		rtfree(nat->nat_rt);
		nat->nat_rt = 0;
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
void nat_unset_fcache(nat_t *nat)
{
	nat->nat_flags &= ~IPN_FASTCACHE;
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
void nat_check_fcache(nat_t *nat, tcphdr_t *tcp)
{
	frentry_t *fr;
	
	fr = nat->nat_fstate.rule;
	/* only FW pass seesion can fast-out */
	if((fr = nat->nat_fstate.rule))
	{
		if(!(fr->fr_flags & FR_PASS))
			return;
	}
	
	if(nat->nat_rt && nat->nat_ort)
	{
		switch(nat->nat_p)
		{
			case IPPROTO_TCP:
				if((nat->nat_tcpstate[nat->nat_dir] >= TCPS_CLOSE_WAIT) || (nat->nat_tcpstate[1 - nat->nat_dir] >= TCPS_CLOSE_WAIT))
					nat->nat_flags &= ~IPN_FASTCACHE;
				else if((nat->nat_tcpstate[nat->nat_dir] > TCPS_SYN_RECEIVED) && (nat->nat_tcpstate[1 - nat->nat_dir] > TCPS_SYN_RECEIVED))
				{
					nat->nat_flags |= IPN_FASTCACHE;
				}
				
				break;
			case IPPROTO_UDP:
				nat->nat_flags |= IPN_FASTCACHE;
				break;
			default:
				break;
		}
		
	}
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
void flush_fcache(void)
{
	register struct nat *nat;
	for (nat = nat_instances; nat; nat = nat->nat_next)
	{
		// Simon test here...
		nat->nat_fstate.rule = 0;
		
		if(nat->nat_flags & IPN_FASTCACHE)
			nat->nat_flags &= ~IPN_FASTCACHE;
	}
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
int ipnat_fastpath(struct ifnet *ifp, struct mbuf **mp)
{
	register struct mbuf *m = *mp;
	register struct ip *ip;
	register tcphdr_t *tcp;
	register nat_t *nat;
	struct ifnet *difp;
	struct rtentry *drt;
	struct sockaddr_in sin = {sizeof(sin), AF_INET };
	struct sockaddr_in *dst;
	u_short *csump = NULL;
	u_int hlen, hv;
	
	if(m->m_pkthdr.len < 40)
		 goto out;
		
	ip = mtod(m, struct ip *);

	if ((m->m_len < 24) ||
        (ip->ip_v != 4) ||
        (ip->ip_hl != sizeof(struct ip)/4) ||
        (m->m_pkthdr.len < ntohs(ip->ip_len)) ||
        ((ntohs(ip->ip_off) & 0x3fff) != 0) ||
        (ip->ip_ttl <= IPTTLDEC))
    {
        goto out;
    }

	
	hlen = ip->ip_hl << 2;
	/* we do not deal with wrong IP checksum */
	if(in_cksum(m, hlen) != 0)
		goto out;
	tcp = (tcphdr_t *)((char *)ip + hlen);
	
	/* inbound */
#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
	/* Port Restricted NAT */
	hv = NAT_HASH_FN(ip->ip_dst.s_addr, tcp->th_dport, ipf_nattable_sz);
#else
	/*  Symmertic NAT  */
	hv = NAT_HASH_FN(ip->ip_dst.s_addr, tcp->th_dport, 0xffffffff);
	hv = NAT_HASH_FN(ip->ip_src.s_addr, hv + tcp->th_sport, ipf_nattable_sz);
#endif
	nat = nat_table[1][hv];
	//diag_printf("[hv=%d, ip_dst=%x, dport=%x, ip_src=%x, sport=%x]\n", hv, ip->ip_dst.s_addr, tcp->th_dport, ip->ip_src.s_addr, tcp->th_sport);
	for (; nat; nat = nat->nat_hnext[1]) 
	{
		if (nat->nat_flags & IPN_DELETE)
			continue;

		if(	((nat->nat_flags & IPN_FASTCACHE)==0) ||
			(ifp != nat->nat_ifp) ||
			(nat->nat_oip.s_addr != ip->ip_src.s_addr) ||
			(nat->nat_outip.s_addr != ip->ip_dst.s_addr) ||
			(ip->ip_p != nat->nat_p) ||
			(tcp->th_sport != nat->nat_oport) ||
			(tcp->th_dport != nat->nat_outport))
			continue;
		
		if((drt = nat->nat_ort) == NULL)
			goto out;
		difp = drt->rt_ifp;
		/* we do not deal packet need to fragment */
		if(difp->if_mtu < ntohs(ip->ip_len))
			goto out;
		switch(ip->ip_p)
		{
			case IPPROTO_TCP:
				if (tcp->th_flags & (TH_RST | TH_FIN | TH_SYN))
        			goto out;
				
				csump = &(tcp->th_sum);
				tcp->th_dport = nat->nat_inport;
				nat->nat_age = fr_tcpidletimeout;
				break;
			case IPPROTO_UDP:
				if(((udphdr_t *)tcp)->uh_sum)
					csump = &(((udphdr_t *)tcp)->uh_sum);
				tcp->th_dport = nat->nat_inport;
				nat->nat_age = fr_defnatage;
				break;
			default:
				goto out;
		}
		
		if (csump) 
		{
			if (nat->nat_dir == NAT_OUTBOUND)
				fix_incksum((ip->ip_len-hlen), csump, nat->nat_sumd[0]);
			else
				fix_outcksum((ip->ip_len-hlen), csump, nat->nat_sumd[0]);
		}
		//diag_printf("inbound >> nat:%x\n", nat);
		ip->ip_dst = nat->nat_inip;
		break;
	}
	
	if(nat)
		goto fastout;
	/* outbound */	

#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
	/*  Port Restricted NAT  */
	hv = NAT_HASH_FN(ip->ip_src.s_addr, tcp->th_sport, ipf_nattable_sz);
#else
	/*  Symmertic NAT  */
	//hv = NAT_HASH_FN(ip->ip_src.s_addr, tcp->th_sport, 0xffffffff);
	//hv = NAT_HASH_FN(ip->ip_dst.s_addr, hv +  tcp->th_dport, ipf_nattable_sz);
#endif
	nat = nat_table[0][hv];
	//diag_printf("[out: hv=%d, nat=%x, ip_src=%x, sport=%x, ip_dst=%x, dport=%x]\n", hv, nat, ip->ip_src.s_addr, tcp->th_sport, ip->ip_dst.s_addr, tcp->th_dport);
	for (; nat; nat = nat->nat_hnext[0]) 
	{
		//diag_printf("@@@@@ IPN_FASTCACHE:%x, ifp:%x, nat->nat_ort->rt_ifp:%x\n", (nat->nat_flags & IPN_FASTCACHE), ifp, nat->nat_ort->rt_ifp);
		if (nat->nat_flags & IPN_DELETE)
			continue;
		if(	((nat->nat_flags & IPN_FASTCACHE)==0) ||
			(ifp != nat->nat_ort->rt_ifp) ||
			(nat->nat_inip.s_addr != ip->ip_src.s_addr) ||
			(nat->nat_oip.s_addr != ip->ip_dst.s_addr) ||
			(ip->ip_p != nat->nat_p) ||
			(tcp->th_sport != nat->nat_inport) ||
			(tcp->th_dport != nat->nat_oport))
			continue;
		
		if((drt = nat->nat_rt) == NULL)
			goto out;
		difp = drt->rt_ifp;
		/* we do not deal packet need to fragment */
		if(difp->if_mtu < ntohs(ip->ip_len))
			goto out;
		switch(ip->ip_p)
		{
			case IPPROTO_TCP:
				if (tcp->th_flags & (TH_RST | TH_FIN | TH_SYN))
					goto out;
				csump = &(tcp->th_sum);
				tcp->th_sport = nat->nat_outport;
				nat->nat_age = fr_tcpidletimeout;
#ifdef CONFIG_NAT_FORCE_TCPWIN
				nat_adj_tcpwindow(tcp, (ip->ip_len-hlen), csump);
#endif
				break;
			case IPPROTO_UDP:
				if(((udphdr_t *)tcp)->uh_sum)
					csump = &(((udphdr_t *)tcp)->uh_sum);
				tcp->th_sport = nat->nat_outport;
				nat->nat_age = fr_defnatage;
				break;
			default:
				goto out;
					
		}
#ifdef CONFIG_DROP_ILLEGAL_TCPRST
		ip->ip_id = htons(ip_id++);
#endif
		if (csump) 
		{
			if (nat->nat_dir == NAT_OUTBOUND)
				fix_outcksum((ip->ip_len-hlen), csump, nat->nat_sumd[1]);
			else
				fix_incksum((ip->ip_len-hlen), csump, nat->nat_sumd[1]);
		}

		//diag_printf("outbound >> nat:%x\n", nat);
		ip->ip_src = nat->nat_outip;
		//difp = nat->nat_ifp;
		//sin.sin_addr = nat->nat_oip;
		//*(unsigned long *)0xb4000090 = 'O';
		break;
	}	
	
	if(nat == NULL)
		goto out;	
fastout:
	//nat->nat_bytes += ip->ip_len;
	//nat->nat_pkts++;
	
	if (drt->rt_flags & RTF_GATEWAY)
		dst = (struct sockaddr_in *)(drt->rt_gateway);
	else
	{
		sin.sin_addr = ip->ip_dst;
		dst = &sin;
	}
	ip->ip_ttl -= IPTTLDEC;
	ip->ip_sum = 0;
	ip->ip_sum = in_cksum(m, hlen);
	//diag_printf("+++++difp:%x, ip->ip_sum:%x\n", difp, ip->ip_sum);

	(*difp->if_output)(difp, m, (struct sockaddr *)dst, drt);
	
	return 1;
	
out:
	/* we have to ensure only one Mbuf */
	if(m->m_next)
	{
		//diag_printf("WARNING: pull up to one mbuf!\n");
		*mp = m_pullup2(m, m->m_pkthdr.len);
		if(*mp == NULL)
			return 2;
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
void ip_fastpath_init(void)
{
	ip_fastpath = ipnat_fastpath;
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
void ip_fastpath_remove(void)
{
	ip_fastpath = NULL;
}

#endif // FAST_PATH


