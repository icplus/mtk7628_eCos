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

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/socket.h>

#include <net/route.h>
#include <cfg_net.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <sys/sockio.h>

//==============================================================================
//                                    MACROS
//==============================================================================
#define IP_ADDR_IDX		0
#define HW_ADDR_IDX		1

#define Swap32IfLE(l) \
    ((((l) & 0xff000000) >> 24) | \
     (((l) & 0x00ff0000) >> 8)  | \
     (((l) & 0x0000ff00) << 8)  | \
     (((l) & 0x000000ff) << 24))

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
struct arp_req{
	unsigned short	flg;
	unsigned int dst;
	char *buf;
};

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================





#if 0
void rtmsg(void)
{
	int s;
	struct rt_msghdr rtm;
		
	if (s < 0) {
		s = socket(PF_ROUTE, SOCK_RAW, 0);
		if (s < 0)
			NET_ERR(1, "socket");
	}
	
	bzero((char *)&rtm, sizeof(rt_msghdr));
	rtm->rtm_flags = flags;
	rtm->rtm_version = RTM_VERSION;
	
	switch (cmd) {
	case RTM_GET:
		rtm->rtm_addrs |= RTA_DST;
	}
}

/*
 * Display an arp entry
 */
void print_arp_entry(struct sockaddr_dl *sdl,
	struct sockaddr_inarp *addr, struct rt_msghdr *rtm)
{
	char ifname[IF_NAMESIZE];
	
	printf("%s at ", inet_ntoa(addr->sin_addr));
	if (sdl->sdl_alen)
		printf("%s", ether_ntoa((struct ether_addr *)LLADDR(sdl)));
	else
		printf("(incomplete)");
		
	if (if_indextoname(sdl->sdl_index, ifname) != NULL)
		printf(" on %s", ifname);
	if (rtm->rtm_rmx.rmx_expire == 0)
		printf(" permanent");
	if (addr->sin_other & SIN_PROXY)
		printf(" published (proxy only)");
	if (rtm->rtm_addrs & RTA_NETMASK) {
		addr = (struct sockaddr_inarp *)
			(ROUNDUP(sdl->sdl_len) + (char *)sdl);
		if (addr->sin_addr.s_addr == 0xffffffff)
			printf(" published");
		if (addr->sin_len != 8)
			printf("(weird)");
	}
	printf("\n");

}
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
int netarpcmd(int cmd, int ip, char *hwaddr, int *pflags, struct ifnet *ifp)
{
    //ruct arpreq       arpRequest;
    int                 rc = 0;
#if 0 //Eddy6 no arpioctl
    // fill in arp request structure 
    bzero ((caddr_t) &arpRequest, sizeof (struct arpreq));

    arpRequest.arp_pa.sa_family = AF_INET;
    ip = Swap32IfLE(ip);
    ((struct sockaddr_in *)&(arpRequest.arp_pa))->sin_addr.s_addr = htonl(ip);
    
    arpRequest.arp_ha.sa_family = AF_UNSPEC;
    if (hwaddr != NULL)
    {
        bcopy ((caddr_t) hwaddr ,(caddr_t) arpRequest.arp_ha.sa_data, 6);
    }
	
	if(pflags)
   		arpRequest.arp_flags = *pflags;
   
    if (ifp != NULL)
        arpRequest.ifp = ifp;
	
	if((rc = arpioctl(cmd, (caddr_t)(&arpRequest))) == 0)
	{
		if(hwaddr != NULL)
        {
        	bcopy ((caddr_t) arpRequest.arp_ha.sa_data, (caddr_t) hwaddr, 6);
        }
		
		if(pflags != NULL)
        {
    		*pflags = arpRequest.arp_flags;
        }
	}
#endif	
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
static int dump_arp(struct radix_node *rn, void *req)
{
    struct rtentry *rt = (struct rtentry *)rn;
    struct sockaddr *dst;
    struct sockaddr_dl *sdl;

    dst = rt_key(rt);
    sdl = (struct sockaddr_dl *)rt->rt_gateway;
    if ((rt->rt_flags & (RTF_UP | RTF_LLINFO)) == (RTF_UP | RTF_LLINFO)) 
    {
    		if(((struct arp_req *)req)->buf)
    		{
    			switch(((struct arp_req *)req)->flg)
    			{
    				case IP_ADDR_IDX:
    					if(((struct arp_req *)req)->dst == ((struct sockaddr_in *)dst)->sin_addr.s_addr)
    						memcpy(((struct arp_req *)req)->buf, LLADDR(sdl), 6);
    					break;
    				case HW_ADDR_IDX:
    					if(!memcmp(((struct arp_req *)req)->buf, LLADDR(sdl), 6))
    						*(int *)(((struct arp_req *)req)->dst) = ((struct sockaddr_in *)dst)->sin_addr.s_addr;
    					break;
    			}
    		}
    		else 
    		{
    			diag_printf("%s at ", inet_ntoa(((struct sockaddr_in *)dst)->sin_addr));
    			diag_printf("%s\n", ether_ntoa((struct ether_addr *)LLADDR(sdl)));
    		}
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
char* netarp_get(unsigned int addr, char *outbuf, unsigned short flag)
{
	int i, error;
    struct radix_node_head *rnh;
    struct arp_req req;
	
	memset(&req, 0, sizeof(struct arp_req));
	if(addr && outbuf) {
		req.flg = flag;
		req.dst = addr;
		req.buf = outbuf;
	}
	
    cyg_scheduler_lock();
    for (i = 1; i <= AF_MAX; i++) {
        if ((rnh = rt_tables[i]) != NULL) {
            error = rnh->rnh_walktree(rnh, dump_arp, &req);
        }
    }

    cyg_scheduler_unlock();
	
	return outbuf;
}

//------------------------------------------------------------------------------
// FUNCTION
//		char *netarp_get_one(unsigned int adr, char *buf)
//
// DESCRIPTION
//		use IP address as index for search hardware address in ARP table
//  
// PARAMETERS
//		adr : value of IP address
//  	buf : pointer to hardware address
//
// RETURN
//		pointer to hardware address
//  
//------------------------------------------------------------------------------
char *netarp_get_one(unsigned int adr, char *buf)
{
	return netarp_get(adr, buf, IP_ADDR_IDX);
}

//------------------------------------------------------------------------------
// FUNCTION
//		void netarp_get_one_by_mac(unsigned int *adr, char *buf)
//
// DESCRIPTION
//		use hardware address as index for search IP in ARP table
//  
// PARAMETERS
//		adr : pointer to IP address
//  	buf : pointer to hardware address
//
// RETURN
//		NONE
//  
//------------------------------------------------------------------------------
void netarp_get_one_by_mac(unsigned int *adr, char *buf)
{
	if(!buf) return;
	netarp_get((int)adr, buf, HW_ADDR_IDX);
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
inline void netarp_get_all(void)
{
	netarp_get(NULL, NULL, IP_ADDR_IDX);
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
inline void netarp_flush(void)
{
	netarpcmd(SIOCFLUSHARP, 0, NULL, NULL, NULL);
}


int netarp_add(struct in_addr *host, char *macs, char *ifname, int inflag)
{
	struct ifnet *ifp = NULL;
	int flags = inflag;
	
	if (ifname != NULL)
		ifp = ifunit(ifname);
	
	return netarpcmd(SIOCSARP, host->s_addr, macs, &flags, ifp);
}


int netarp_del(struct in_addr *host, char *ifname)
{
	struct ifnet *ifp = NULL;
	
	if (ifname != NULL)
		ifp = ifunit(ifname);
	
	return netarpcmd(SIOCDARP, host->s_addr, NULL, NULL, ifp);
	
}


