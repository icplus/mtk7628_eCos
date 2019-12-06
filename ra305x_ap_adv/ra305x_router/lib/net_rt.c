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
#define _KERNEL
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/route.h>
#include <network.h>
#include <cfg_net.h>
#include <cfg_id.h>
#include <sys_status.h>
//==============================================================================
//                                    MACROS
//==============================================================================
#define RT_DBG	diag_printf

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================
static long inet_IoctlCall (int code, struct ifreq *ifrp);
static void ip_forward_cache_clear(void);
static int inet_Ioctl(char *interfaceName, int code, int arg);
static int inet_IoctlGet(char *interfaceName, int code, int *val);
static int inet_IoctlSet(char *interfaceName, int code, int val);


//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
extern int str2arglist(char *, char **, char, int);


//------------------------------------------------------------------------------
// FUNCTION
//		cyg_bool_t netrt_add_del(const char *intf, struct in_addr *dst, struct in_addr *mask, struct in_addr *gateway, int metric, int action)
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
cyg_bool_t RT_add_del(const char *intf, struct in_addr *dst, struct in_addr *mask, struct in_addr *gateway, int metric, int flag, int action)
{
	struct sockaddr_in addr, *addrp;
    int s;
	struct ecos_rtentry route;
	
	s= socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("socket");
        return false;
    }
    addrp = (struct sockaddr_in *)&addr;
    memset(addrp, 0, sizeof(*addrp));
    memset(&route, 0, sizeof(route));
	
	
    route.rt_dev = intf;
    route.rt_flags = RTF_UP;
    route.rt_metric = metric;
      
    addrp->sin_family = AF_INET;
    addrp->sin_port = 0;
    addrp->sin_len = sizeof(*addrp);
    addrp->sin_addr.s_addr = (dst->s_addr & mask->s_addr); 
    memcpy(&route.rt_dst, addrp, sizeof(*addrp));
    addrp->sin_addr = *mask;
    memcpy(&route.rt_genmask, addrp, sizeof(*addrp));
	if(mask->s_addr == 0xffffffff)
		route.rt_flags |= RTF_HOST;
		
	if(gateway) {
    	addrp->sin_addr = *gateway;
   		memcpy(&route.rt_gateway, addrp, sizeof(*addrp));
   		route.rt_flags |= RTF_GATEWAY;
   	}
	
	route.rt_flags |= flag;
	
    if (ioctl(s, (action ? SIOCDELRT : SIOCADDRT), &route)) {
     	diag_printf("Route - dst: %s",
     		inet_ntoa(((struct sockaddr_in *)&route.rt_dst)->sin_addr));
     	diag_printf(", mask: %s",
     		inet_ntoa(((struct sockaddr_in *)&route.rt_genmask)->sin_addr));
     	diag_printf(", gateway: %s\n",
     		inet_ntoa(((struct sockaddr_in *)&route.rt_gateway)->sin_addr));
     	if (errno != EEXIST) {
     		perror(action ? "SIOCDELRT" : "SIOCADDRT");
     	}
     	close(s);
        return false;
     }
    close(s); 
    
	return true;
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
inline  cyg_bool_t RT_add(const char *intf, struct in_addr *dst, struct in_addr *mask, struct in_addr *gw, int metric)
{
	return RT_add_del(intf, dst, mask, gw, metric, RTF_STATIC, ROUTE_ADD);
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
inline  cyg_bool_t RT_del(const char *intf, struct in_addr *dst, struct in_addr *mask, struct in_addr *gw, int metric)
{
	return RT_add_del(intf, dst, mask, gw, metric, RTF_STATIC, ROUTE_DEL);
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
inline  cyg_bool_t RT_add_default(const char *intf, unsigned int gw)
{
	struct in_addr dst, mask; 
	
	dst.s_addr = mask.s_addr = 0;
	
	return RT_add_del(intf, &dst, &mask, (struct in_addr *)&gw, 0, 0, ROUTE_ADD);
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
inline  cyg_bool_t RT_del_default(const char *intf, unsigned int gw)
{
	struct in_addr dst, mask;
	
	dst.s_addr = mask.s_addr = 0;
	
	return RT_add_del(intf, &dst, &mask, (struct in_addr *)&gw, 0, 0,ROUTE_DEL);
}


//------------------------------------------------------------------------------
// FUNCTION
//		void RT_staticrt_parse(void)
//
// DESCRIPTION
//		get static route string from CFG and set into routing table
//		string format: <destination IP>;<netmask>;<gateway>;[metric];[if];[enable] 
//  
// PARAMETERS
//		NONE
//  
// RETURN
//		NONE
//  
//------------------------------------------------------------------------------
static	int RT_staticrt_parse
(
	char *rtent,
	struct in_addr *dst,
	struct in_addr *mask,
	struct in_addr *gw,
	int *metric
)
{
	int argc;
	char *arglists[8];

	argc = str2arglist(rtent, arglists, ';', 8);
	switch (argc)
	{
	case 3:
		*metric = 2;
		break;
	case 4:
		*metric = atoi(arglists[3]);
		break;
	case 6:
		/*  Check if the rule is enabled  */
		if (strcmp("1", arglists[5]) != 0)
			return -1;
		if (arglists[3][0] != 0)
			*metric = atoi(arglists[3]);
		else
			*metric = 2;
		break;
	default:
		return -1;
	}

	// Do range check
	if(Check_IP(arglists[0])<0)
		return -1;
	if(Check_MASK(arglists[1])<0)
	    return -1;	
	if(Check_IP(arglists[2])<0)
	    return -1;	    		

	dst->s_addr = inet_addr(arglists[0]);
	mask->s_addr = inet_addr(arglists[1]);
	gw->s_addr = inet_addr(arglists[2]);
	return 0;
}


void RT_staticrt_allset(void)
{
	int i=0;
	char rtent[100];

	struct in_addr dst, mask, gw;
	int metric;

#ifdef	CONFIG_FIXED_STATIC_RT_TABLE
	for (i=0; i<CONFIG_STATIC_RT_NUM; i++)
	{
		if (CFG_get(CFG_RT_ENTRY+i+1, rtent) == -1)
			continue;
#else
	for (i=0; CFG_get(CFG_RT_ENTRY+i+1, rtent) != -1; i++)
	{
#endif
		if (RT_staticrt_parse(rtent, &dst, &mask, &gw, &metric) != 0)
			continue;

		RT_add(NULL, &dst, &mask, &gw, metric);
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
int RT_add_del_staticrt(char *rtent, int set)
{
	char buf[100];
	struct in_addr dst, mask, gw;
	int metric;
	int rc;
	
	memcpy(buf, rtent, 100);

	if (RT_staticrt_parse(buf, &dst, &mask, &gw, &metric) != 0)
		return -1;

	/* no need ?? */
	//dst.s_addr = htonl( ntohl(dst.s_addr) & ntohl(mask.s_addr));
	if (set)
		rc = RT_add(NULL, &dst, &mask, &gw, metric);
	else
		rc = RT_del(NULL, &dst, &mask, &gw, metric);

	ip_forward_cache_clear();
	RT_DBG("[RT]: %s dst:%x mask:%x gw:%x rc:%s\n", set ? "add" : "del", dst.s_addr, mask.s_addr, gw.s_addr, (rc==0) ? "OK" : "FAIL");
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
inline int RT_add_staticrt(char *rtent)
{
	return RT_add_del_staticrt(rtent, 1);
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
inline int RT_del_staticrt(char *rtent)
{
	return RT_add_del_staticrt(rtent, 0);
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
static int staticrt_delete( struct radix_node *rn, void *vifp )
{
	struct rtentry *rt = (struct rtentry *)rn;
	
	if((rt->rt_flags & RTF_STATIC) && !(rt->rt_flags & RTF_LLINFO))
	{
		rtrequest(RTM_DELETE, rt_key(rt), rt->rt_gateway, rt_mask(rt),
              0, NULL);
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
void RT_staticrt_flush(void)
{
	struct radix_node_head *rnh;
	int i;

	for (i = 1; i <= AF_MAX; i++) {
        if ((rnh = rt_tables[i]) != NULL) {
            rnh->rnh_walktree(rnh, staticrt_delete, NULL);
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
void RT_staticrt_renew(void)
{
	RT_staticrt_flush();
	
	RT_staticrt_allset();
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
static int ifprt_delete( struct radix_node *rn, void *ifp )
{
	struct rtentry *rt = (struct rtentry *)rn;
	
	if(rt->rt_ifp == ifp)
	{
		rtrequest(RTM_DELETE, rt_key(rt), rt->rt_gateway, rt_mask(rt),
              rt->rt_flags, NULL);
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
void RT_ifp_flush(char *ifname)
{
	struct radix_node_head *rnh;
	int i;
	struct ifnet *ifp;
	
	ifp = ifunit(ifname);
	for (i = 1; i <= AF_MAX; i++) {
        if ((rnh = rt_tables[i]) != NULL) {
            rnh->rnh_walktree(rnh, ifprt_delete, ifp);
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
static int ifart_delete( struct radix_node *rn, struct in_addr *addr )
{
	struct rtentry *rt = (struct rtentry *)rn;
	
	if (((struct sockaddr_in *)rt_key(rt))->sin_addr.s_addr == addr->s_addr)
	{
		rtrequest(RTM_DELETE, rt_key(rt), rt->rt_gateway, rt_mask(rt),
              rt->rt_flags, NULL);
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
void RT_ifa_flush(unsigned int ipaddr)
{
	struct radix_node_head *rnh;
	int i;
	struct in_addr addr;
	
	addr.s_addr = ipaddr;
	for (i = 1; i <= AF_MAX; i++) {
        if ((rnh = rt_tables[i]) != NULL) {
            rnh->rnh_walktree(rnh, ifart_delete, &addr);
        }
    }
	
}


/*---------------------------------------------------------------------------
* DESCRIPTION: 
*   This routine sets IP and mask of the specified interface.
*---------------------------------------------------------------------------*/
#define OK		0
#define ERROR		(-1)
typedef unsigned long UINT32;
typedef long STATUS;
int inet_SetIpConfigs (UINT32 ip, UINT32 mask, char *pIfName)
{
    UINT32  old_ip=0xffffffff, old_mask=0xffffffff, old_net=0xffffffff;

    if(pIfName == NULL)
        return ERROR;

    //
    // Get old Ip and old mask, to delete the route entry
    // of this interface
    //
    inet_GetIpAddr(&old_ip, pIfName);
    inet_GetSubnetMask(&old_mask, pIfName);

    if ((old_ip != 0xffffffff &&
        ((old_net = (old_ip & old_mask)) != (ip & mask))) ||
        (old_ip == ip && old_mask != mask))
    {
        inet_RouteDel(old_net, old_ip, old_mask);
    } 
    
    if(inet_SetSubnetMask (mask, pIfName) != OK || inet_SetIpAddr (ip, pIfName) != OK)
        return ERROR;

    //Ipc_IfCallback(pIfName, ip, mask);

    return OK;
}


/*---------------------------------------------------------------------------------
* DESCRIPTION:  This function gets active IP of a specify interface. 
*---------------------------------------------------------------------------------*/
int inet_IfIpAddr(UINT32 *pIpAddr, char *pIfName)
{

    if (pIpAddr == NULL || pIfName == NULL)
        return ERROR;

    if (inet_GetIpAddr(pIpAddr, pIfName) == OK)
    {
        return OK;
    }
    else
        return ERROR;
}


/*--------------------------------------------------------------------*/
int inet_SetSubnetMask (UINT32 mask, char *pIfName)
{
    return inet_Ioctl (pIfName, SIOCSIFNETMASK, (int)mask);
}


/*--------------------------------------------------------------------*/
int inet_GetSubnetMask(UINT32 *pMask, char *pIfName)
{
    return inet_Ioctl (pIfName, SIOCGIFNETMASK, (int)pMask);
}


/*--------------------------------------------------------------------*/
int inet_SetIpAddr (UINT32 ip, char *pIfName)
{
    return inet_Ioctl (pIfName, SIOCSIFADDR, ip);
}


/*--------------------------------------------------------------------*/
int inet_GetIpAddr (UINT32 *pIp, char *pIfName)
{
    return inet_Ioctl (pIfName, SIOCGIFADDR, (int)pIp);
}


/*--------------------------------------------------------------------*/
static int inet_Ioctl(char *interfaceName, int code, int arg)
{
    int status;
   
    switch (code)
    {
    case SIOCSIFADDR:
    case SIOCSIFBRDADDR:
    case SIOCSIFDSTADDR:
    case SIOCSIFNETMASK:
    case SIOCSIFFLAGS:
    case SIOCSIFMETRIC:
        status = inet_IoctlSet (interfaceName, code, arg);
        break;
        
    case SIOCGIFNETMASK:
    case SIOCGIFFLAGS:
    case SIOCGIFADDR:
    case SIOCGIFBRDADDR:
    case SIOCGIFDSTADDR:
    case SIOCGIFMETRIC:
    //case SIOCGIFNAME:
        status = inet_IoctlGet (interfaceName, code, (int *)arg);
        break;

    default:
        //(void)errnoSet (EOPNOTSUPP); /* not supported operation */
        status = ERROR;
        break;
    }
    
    return status;
}


/*--------------------------------------------------------------------*/
static int inet_IoctlGet(char *interfaceName, int code, int *val)
{
    struct ifreq  ifr;

    //if (code == SIOCGIFNAME)
    //{
    //    ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr = htonl(*val);
    //}
    //else
    {
        strncpy (ifr.ifr_name, interfaceName, sizeof (ifr.ifr_name));
    }

    if (inet_IoctlCall (code, &ifr) == ERROR)
        return ERROR;
    
    switch (code)
    {
    case SIOCGIFFLAGS:
        *val = ifr.ifr_flags;
        break;
        
    case SIOCGIFMETRIC:
        *val = ifr.ifr_metric;
        break;

    //case SIOCGIFNAME:
    //    strncpy (interfaceName, ifr.ifr_name, sizeof (ifr.ifr_name));
    //    break;

    default:
        *val = ntohl(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr);
        break;
    }
    
    return OK;
}


/*--------------------------------------------------------------------*/
static int inet_IoctlSet(char *interfaceName, int code, int val)
{
    struct ifreq ifr;
    
    strncpy (ifr.ifr_name, interfaceName, sizeof (ifr.ifr_name));
    
    switch (code)
    {
    case SIOCSIFFLAGS:
        ifr.ifr_flags = (short) val;
        break;
        
    case SIOCSIFMETRIC:
        ifr.ifr_metric = val;
        break;
    
    default:
        bzero ((caddr_t) &ifr.ifr_addr, sizeof (ifr.ifr_addr));
        ifr.ifr_addr.sa_len = sizeof (struct sockaddr_in);
        ifr.ifr_addr.sa_family = AF_INET;
        ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr = htonl(val);
        break;
    }
    
    return inet_IoctlCall (code, &ifr);
}


/*--------------------------------------------------------------------
*   DESCRIPTION: 
*   This routine makes ioctl call to socket layer
*--------------------------------------------------------------------*/
static STATUS inet_IoctlCall (int code, struct ifreq *ifrp)
{
    int so;
    int status;
    
    if ((so = socket (AF_INET, SOCK_RAW, 0)) < 0)
        return ERROR;
    
    status = ioctl (so, code, (int)ifrp);
    (void)close (so);
    
    if (status != 0)
    {
        //if (status != ERROR)	/* iosIoctl() can return ERROR */
        //    (void)errnoSet (status);
        
        return ERROR;
    }
    return OK;
}


/*-----------------------------------------------------------------------------------------*/
int inet_RouteDel (unsigned long dst, unsigned long gateway, unsigned long mask)
{
    struct
    {
	    struct	rt_msghdr m_rtm;
        struct  sockaddr_in m_space[8];
    }
    m_rtmsg;

    int s;
    int i;

    s = socket(PF_ROUTE, SOCK_RAW, 0);
    if (s == -1)
        return -1;

    // Set rt_msghdr
	memset(&m_rtmsg, 0, sizeof(m_rtmsg));
    
#define rtm m_rtmsg.m_rtm

    rtm.rtm_msglen = sizeof(m_rtmsg);
	rtm.rtm_type = RTM_DELETE;
	rtm.rtm_flags = RTF_UP;
	rtm.rtm_version = RTM_VERSION;

    rtm.rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;

    // Setup the followed 8 socket_in;
    for (i=0; i<8; i++)
    {
        m_rtmsg.m_space[i].sin_len = 16;
        m_rtmsg.m_space[i].sin_family = AF_INET;
    }

    m_rtmsg.m_space[RTAX_DST].sin_addr.s_addr = htonl(dst);
    m_rtmsg.m_space[RTAX_GATEWAY].sin_addr.s_addr = htonl(gateway);
    m_rtmsg.m_space[RTAX_NETMASK].sin_addr.s_addr = htonl(mask);

    // Write route socekt to add the entry
    write(s, (char *)&m_rtmsg, sizeof(m_rtmsg));
    
    close(s);
    
    return 0;
}


/*-----------------------------------------------------------------------------------------*/
extern struct	route ipforward_rt;
static void ip_forward_cache_clear(void)
{
	if (ipforward_rt.ro_rt) {
		RTFREE(ipforward_rt.ro_rt);
		ipforward_rt.ro_rt = 0;
	}
}

