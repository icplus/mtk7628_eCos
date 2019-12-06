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
#include <pkgconf/system.h>
#ifdef CYGBLD_DEVS_ETH_DEVICE_H    // Get the device config if it exists
#include CYGBLD_DEVS_ETH_DEVICE_H
#endif

#include <network.h>
#include <netinet/if_ether.h>
#include <sys_status.h>
#include <cfg_id.h>
#include <cfg_net.h>
#ifdef INET6
#include <netinet6/ip6_mroute.h>
#include <netinet6/nd6.h>
#endif

//==============================================================================
//                                    MACROS
//==============================================================================
///#ifdef   CONFIG_SYSLOG
///#include <eventlog.h>
///#define LOG(msg...)  SysLog(SYS_LOG_INFO, ##msg)
///#else
#define LOG(fmt,msg...) diag_printf(fmt "\n",##msg)
///#endif

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

//------------------------------------------------------------------------------
// FUNCTION
//      void buid_ip_set(struct ip_set *setp,
//                 const char *if_name,
//                 unsigned int addrs_ip,
//                 unsigned int addrs_netmask,
//                 unsigned int addrs_broadcast,
//                 unsigned int addrs_gateway,
//                 unsigned int addrs_server,
//                 unsigned char    mode,
//                 unsigned char    *data   )
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
void buid_ip_set(struct ip_set *setp,
                 const char *if_name,
                 unsigned int addrs_ip,
                 unsigned int addrs_netmask,
                 unsigned int addrs_broadcast,
                 unsigned int addrs_gateway,
                 unsigned int addrs_server,
                 unsigned int mtu,
                 unsigned char  mode,
                 const char *domain,
                 unsigned char  *data   )
{
    if(if_name ==  NULL)
        return;
    memset(setp, 0, sizeof(struct ip_set)); // clear
    strcpy(setp->ifname, if_name);
    setp->ip = addrs_ip;
    if(addrs_netmask || (setp->ip == 0))
        setp->netmask = addrs_netmask;
    else
    {
        // if no netmask, set class netmask
        if (IN_CLASSA(setp->ip))
            setp->netmask = IN_CLASSA_HOST;
        else if (IN_CLASSB(setp->ip))
            setp->netmask = IN_CLASSB_HOST;
        else
            setp->netmask = IN_CLASSC_HOST;
    }

    setp->server_ip = addrs_server;

    setp->gw_ip = addrs_gateway;
    if(addrs_broadcast)
        setp->broad_ip = addrs_broadcast;
    else
        setp->broad_ip=((setp->ip)&(setp->netmask))|~(setp->netmask);

    setp->mtu = mtu;

    setp->mode = mode;
    if(domain)
        strcpy(setp->domain, domain);
    setp->data = data;
}

//------------------------------------------------------------------------------
// FUNCTION
//      void show_ip_set(struct ip_set *setp)
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
void show_ip_set(struct ip_set *setp)
{
    char *modestr[]= {"UNKNOW", "manual", "DHCP", "PPPoE", "PPTP", "L2TP", "DHCP Proxy", "PPPoE2", "PPTP2"};

    LOG("-----------------------------------------");
    LOG("%s: Get IP by %s", setp->ifname, modestr[setp->mode]);
    LOG("IP address:%s ", inet_ntoa(*(struct in_addr *)&setp->ip));
    LOG("MASK:%s", inet_ntoa(*(struct in_addr *)&setp->netmask));
    LOG("Broadcast:%s", inet_ntoa(*(struct in_addr *)&setp->broad_ip));
    LOG("Gateway:%s", inet_ntoa(*(struct in_addr *)&setp->gw_ip));
    LOG("Server:%s", inet_ntoa(*(struct in_addr *)&setp->server_ip));
    LOG("mtu:%d", *(struct in_addr *)&setp->mtu);
    LOG("Domain Name:%s", setp->domain);
    LOG("-----------------------------------------");
}

//------------------------------------------------------------------------------
// FUNCTION
//      cyg_bool_t net_set_maca( const char *interface, char *mac_address )
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
cyg_bool_t net_set_maca( const char *interface, char *mac_address )
{
    int s, i;
    struct ifreq ifr;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        perror("socket");
        return false;
    }

    //LOG( "%s socket is %d:", interface, s );

    strcpy(ifr.ifr_name, interface);

    for ( i = 0; i < ETHER_ADDR_LEN; i++ )
        ifr.ifr_hwaddr.sa_data[i] = mac_address[i];

    /*LOG( "Mac addr %02x:%02x:%02x:%02x:%02x:%02x",
                 0xff&ifr.ifr_hwaddr.sa_data[0],
                 0xff&ifr.ifr_hwaddr.sa_data[1],
                 0xff&ifr.ifr_hwaddr.sa_data[2],
                 0xff&ifr.ifr_hwaddr.sa_data[3],
                 0xff&ifr.ifr_hwaddr.sa_data[4],
                 0xff&ifr.ifr_hwaddr.sa_data[5] );
    */

    if (ioctl(s, SIOCSIFHWADDR, &ifr))
    {
        perror("SIOCSIFHWADDR");
        close( s );
        return false;
    }

    //LOG( "%s ioctl(SIOCSIFHWADDR) succeeded", interface );
    close( s );

    return true;
}

//------------------------------------------------------------------------------
// FUNCTION
//      cyg_bool_t net_set_maca( const char *interface, char *mac_address )
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
char * net_get_maca( const char *interface )
{
    int s;
    struct ifreq ifr;
    static char mac[6];

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        perror("socket");
        return 0;
    }

    //LOG( "%s socket is %d:", interface, s );

    strcpy(ifr.ifr_name, interface);

    if (ioctl(s, SIOCGIFHWADDR, &ifr))
    {
        perror("SIOCGIFHWADDR");
        close( s );
        return 0;
    }

    memcpy(mac, &ifr.ifr_hwaddr.sa_data[0], ETHER_ADDR_LEN);
#if 0
    LOG( "Get Mac addr %02x:%02x:%02x:%02x:%02x:%02x",
         0xff&ifr.ifr_hwaddr.sa_data[0],
         0xff&ifr.ifr_hwaddr.sa_data[1],
         0xff&ifr.ifr_hwaddr.sa_data[2],
         0xff&ifr.ifr_hwaddr.sa_data[3],
         0xff&ifr.ifr_hwaddr.sa_data[4],
         0xff&ifr.ifr_hwaddr.sa_data[5] );

    LOG( "%s ioctl(SIOCGIFHWADDR) succeeded", interface );
#endif

    close( s );

    return mac;
}
//------------------------------------------------------------------------------
// FUNCTION
//      cyg_bool_t netif_cfg(const char *intf, struct ip_set *setp)
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
cyg_bool_t netif_cfg(const char *intf, struct ip_set *setp)
{
    struct sockaddr_in *addrp;
    struct ifreq ifr;
    int s;
    int one = 1;
    int flags = 0;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        return false;
    }

    // Get flag firstly
    strcpy(ifr.ifr_name, intf);
    if (ioctl(s, SIOCGIFFLAGS, &ifr))
    {
        perror("SIOCGIFFLAGS");
        goto bad;
    }

    flags = ifr.ifr_flags;
    if (flags & IFF_BROADCAST)
    {
        if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one)))
        {
            perror("setsockopt");
            goto bad;
        }
    }

    addrp = (struct sockaddr_in *) &ifr.ifr_addr;

    memset(addrp, 0, sizeof(*addrp));
    addrp->sin_family = AF_INET;
    addrp->sin_len = sizeof(*addrp);
    addrp->sin_port = 0;

    strcpy(ifr.ifr_name, intf);

    // set IP address
    addrp->sin_addr.s_addr = setp->ip;

    if (ioctl(s, SIOCSIFADDR, &ifr))
    {
        perror("SIOCSIFADDR");
        goto bad;
    }

    // set netmask
    addrp->sin_addr.s_addr = setp->netmask;
    if (ioctl(s, SIOCSIFNETMASK, &ifr))
    {
        perror("SIOCSIFNETMASK");
        goto bad;
    }

    if (flags & IFF_BROADCAST)
    {
        // set broadcast address
        addrp->sin_addr.s_addr = setp->broad_ip;
        if (ioctl(s, SIOCSIFBRDADDR, &ifr))
        {
            perror("SIOCSIFBRDADDR");
            goto bad;
        }
    }
    else if (flags & IFF_POINTOPOINT)
    {
        // set destination address
        addrp->sin_addr.s_addr = setp->broad_ip;
        if (ioctl(s, SIOCSIFDSTADDR, &ifr))
        {
            perror("SIOCSIFDSTADDR");
            goto bad;
        }
    }

    // set IP address
    // Must do this again
    addrp->sin_addr.s_addr = setp->ip;
    if (ioctl(s, SIOCSIFADDR, &ifr))
    {
        perror("SIOCSIFADDR");
    }

#ifdef INET6
    {
        int socv6;
        struct in6_aliasreq areq6;
        struct sockaddr_in6 *addrp6;

        socv6=socket(AF_INET6, SOCK_DGRAM, 0);
        if (socv6 == -1)
        	{
		diag_printf("\n%s :IPv6 init fail\n",__FUNCTION__);
		//goto bad;
		 close(s);
		return true;
        	}

        memset(&areq6, 0, sizeof(struct in6_aliasreq));
        addrp6 = (struct sockaddr_in6 *) &areq6.ifra_addr;
        memset(addrp6, 0, sizeof(*addrp6));
        addrp6->sin6_family = AF_INET6;
        addrp6->sin6_len = sizeof(*addrp6);
        addrp6->sin6_port = 0;

        strcpy(areq6.ifra_name, intf);
        memcpy(&addrp6->sin6_addr, &setp->ip_v6, sizeof(struct in6_addr));

        areq6.ifra_prefixmask.sin6_len = sizeof(struct sockaddr_in6);
        areq6.ifra_prefixmask.sin6_family = AF_INET6;
        memcpy(&areq6.ifra_prefixmask.sin6_addr, &setp->netmask_v6, sizeof(struct in6_addr));

        areq6.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME; //Eddy6 Todo
        areq6.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;

        if (ioctl(socv6, SIOCAIFADDR_IN6, &areq6))
        {
		close(socv6);
		diag_printf("\n%s :IPv6 init fail\n",__FUNCTION__);
		//goto bad;
		 close(s);
		return true;
        }

	/* Neighbor Solicitation Processing Anycast */
	areq6.ifra_addr.sin6_addr.s6_addr32[0] &= areq6.ifra_prefixmask.sin6_addr.s6_addr32[0];
	areq6.ifra_addr.sin6_addr.s6_addr32[1] &= areq6.ifra_prefixmask.sin6_addr.s6_addr32[1];
	areq6.ifra_addr.sin6_addr.s6_addr32[2] &= areq6.ifra_prefixmask.sin6_addr.s6_addr32[2];
	areq6.ifra_addr.sin6_addr.s6_addr32[3] &= areq6.ifra_prefixmask.sin6_addr.s6_addr32[3];
	areq6.ifra_flags |= IN6_IFF_ANYCAST;
	if (ioctl(socv6, SIOCAIFADDR_IN6, &areq6))
	{
		close(socv6);
		diag_printf("\n%s :IPv6 init fail\n",__FUNCTION__);
		//goto bad;
		 close(s);
		return true;
	}

        close(socv6);
    }
#endif

#if 0 //Eddy TODO Check
    if(setp->mtu)
    {
        ifr.ifr_metric = setp->mtu;
        if (ioctl(s, SIOCSIFMTU, &ifr))
        {
            perror("SIOCSIFMTU");
            goto bad;
        }
    }
#endif

    close(s);
    return true;

bad:
    close(s);
    return false;
}

//------------------------------------------------------------------------------
// FUNCTION
//      cyg_bool_t netif_alias_cfg(const char *intf, struct ip_set *setp)
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
cyg_bool_t netif_alias_cfg(const char *intf, struct ip_set *setp)
{
    struct sockaddr_in *addrp, *maskp;
    int s;
    struct ifaliasreq   ifa;

    s = socket(AF_INET, SOCK_RAW, 0);
    if (s < 0)
    {
        perror("socket");
        return false;
    }

    strcpy(ifa.ifra_name, intf);

    addrp = (struct sockaddr_in *) &ifa.ifra_addr;
    memset(addrp, 0, sizeof(*addrp));
    addrp->sin_family = AF_INET;
    addrp->sin_len = sizeof(*addrp);
    addrp->sin_addr.s_addr = setp->ip;

    maskp = (struct sockaddr_in *) &ifa.ifra_mask;
    memset(maskp, 0, sizeof(*maskp));
    maskp->sin_family = AF_INET;
    maskp->sin_len = sizeof(*maskp);
    maskp->sin_addr.s_addr = setp->netmask;

    if (ioctl(s, SIOCAIFADDR, &ifa))
    {
        perror("SIOCAIFADDR");
        if(errno != EEXIST)
        {
            close(s);
            return false;
        }
    }

    close(s);

    return true;
}

//------------------------------------------------------------------------------
// FUNCTION
//      cyg_bool_t netif_del(const char *intf, struct ip_set *setp)
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
cyg_bool_t netif_del(const char *intf, struct ip_set *setp)
{
    struct sockaddr_in *addrp;
    struct ifreq ifr;
    int s;
    int one = 1;

    if(!intf)
        return false;
    LOG("netif_del:%s", intf);
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        perror("socket");
        return false;
    }

    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one)))
    {
        perror("setsockopt");
        close(s);
        return false;
    }

    addrp = (struct sockaddr_in *) &ifr.ifr_addr;
    memset(addrp, 0, sizeof(*addrp));
    addrp->sin_family = AF_INET;
    addrp->sin_len = sizeof(*addrp);
    addrp->sin_port = 0;

    strcpy(ifr.ifr_name, intf);
    if(!setp)
    {
        if (ioctl(s, SIOCGIFADDR, &ifr))
        {
            perror("SIOCGIFADDR");
            close(s);
            return false;
        }
    }
    else
        addrp->sin_addr.s_addr = setp->ip;

    if (ioctl(s, SIOCDIFADDR, &ifr))
    {
        perror("SIOCDIFADDR");
        close(s);
        return false;
    }

bad:
    close(s);

    return true;
}

//------------------------------------------------------------------------------
// FUNCTION
//      cyg_bool_t netif_flag_get(const char *intf, int *flags)
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
cyg_bool_t netif_flag_get(const char *intf, int *flags)
{
    struct ifreq ifr;
    int s;

    s = socket(AF_INET, SOCK_RAW, 0);
    if (s < 0)
    {
        perror("socket");
        return -1;
    }

    strcpy(ifr.ifr_name, intf);

    if (ioctl(s, SIOCGIFFLAGS, &ifr))
    {
        perror("SIOCGIFFLAGS");
        close(s);
        return false;
    }
    *flags = ifr.ifr_flags;
    close(s);
    return true;
}

//------------------------------------------------------------------------------
// FUNCTION
//      cyg_bool_t netif_flag_set(const char *intf, int flags, int set)
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
cyg_bool_t netif_flag_set(const char *intf, int flags, int set)
{
    struct ifreq ifr;
    int s;

    s = socket(AF_INET, SOCK_RAW, 0);
    if (s < 0)
    {
        perror("socket");
        return -1;
    }

    strcpy(ifr.ifr_name, intf);

    if (ioctl(s, SIOCGIFFLAGS, &ifr))
    {
        perror("SIOCGIFFLAGS");
        close(s);
        return false;
    }

    if(set)
        ifr.ifr_flags |= flags;
    else
        ifr.ifr_flags &= ~flags;

    if (ioctl(s, SIOCSIFFLAGS, &ifr))
    {
        perror("SIOCSIFFLAGS");
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
inline cyg_bool_t netif_up(const char *intf)
{
    return netif_flag_set(intf, IFF_UP | IFF_BROADCAST | IFF_RUNNING|IFF_MULTICAST, 1);
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
inline cyg_bool_t netif_down(const char *intf)
{
    return netif_flag_set(intf, IFF_UP | IFF_BROADCAST | IFF_RUNNING|IFF_MULTICAST, 0);
}

