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
    cmd_net.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/
//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <config.h>
#include <stdio.h>
#include <string.h>
#include <cli_cmd.h>

#include <network.h>
#include <cfg_net.h>
#include <sys_status.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <cyg/io/eth/eth_drv_stats.h>
#include <cyg/io/eth/eth_drv.h>
#include <cyg/io/eth/netdev.h>
#ifdef INET6
#include <netinet6/ip6_mroute.h>
#include <netinet6/nd6.h>
#include <netinet6/in6_var.h>
#endif
#include <sys/param.h>

#include <net/netdb.h>

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================
static int net_cmds_init(int argc, char* argv[]);
static int net_show_cmd(int argc, char* argv[]);
#if	MODULE_BRIDGE
static int net_brg_cmd(int argc, char* argv[]);
#endif
static int net_eth_cmd(int argc, char* argv[]);
static int net_ping_cmd(int argc, char* argv[]);
#ifdef CYGPKG_NET_INET6
static int net_ping6_cmd(int argc, char* argv[]);
#endif
static int net_dhcpc_cmd(int argc, char* argv[]);
static int net_dhcpd_cmd(int argc, char* argv[]);
static int net_arp_cmd(int argc, char* argv[]);
static int net_ipnat_cmd(int argc, char* argv[]);

#if (MODULE_PPPOE == 1)
static int net_pppoe_cmd(int argc, char* argv[]);
#endif

static int net_ntp_cmd(int argc, char* argv[]);
static int net_route_cmd(int argc, char* argv[]);
#if	MODULE_UPNP
extern int upp_cmd(int argc, char* argv[]);
#endif

#ifdef CONFIG_SNIFFER_DAEMON
extern int SnifferDebugCmd(int argc, char* argv[]);
#endif

static int net_http_cmd(int argc, char* argv[]);
static int ifconfig_cmd(int argc, char* argv[]);
static int net_timer_cmd(int argc, char* argv[]);
static int net_fw_cmd(int argc, char* argv[]);
static int net_mon_cmd(int argc, char* argv[]);
#ifdef CYGPKG_NET_INET6
static int rtadvd_cmd(int argc, char *argv[]);
static int pimd_cmd(int argc, char *argv[]);
static int net_route6_cmd(int argc, char* argv[]);
static int clear_neighbor_cache(int argc, char* argv[]);

#endif

#if	MODULE_DNS
static int net_dns_cmd(int argc, char* argv[]);
#endif

#if MODULE_ATE_DAEMON
static int ated_cmd(int argc, char* argv[]);
#endif // MODULE_ATE_DAEMON //

#if MODULE_80211X_DAEMON
static int rtdot1xd_cmd(int argc, char* argv[]);
#endif // MODULE_80211X_DAEMON //

#if MODULE_ETH_LONG_LOOP
static int ethlongloop_cmd(int argc, char* argv[]);
#endif

#if MODULE_LLTD_DAEMON
static int lltd_cmd(int argc, char* argv[]);
#endif // MODULE_LLTD_DAEMON //

#ifdef CONFIG_CLI_NET_CMD
#define CLI_NETCMD_FLAG			0
#else
#define CLI_NETCMD_FLAG 		CLI_CMD_FLAG_HIDE
#endif
extern int	ip6_accept_rtadv;	/* Acts as a host not a router */
extern int	ip6_forwarding;		/* act as router? */

#ifdef CONFIG_DHCPV6_SERVER
extern int  dhcp6s_start(int argc, char* argv[]);
static int dhcp6s_cmd(int argc, char* argv[]);
extern int  dhcp6s_stop(int argc, char* argv[]);
#endif 
#ifdef CONFIG_DHCPV6_CLIENT
extern int  dhcp6c_start(int argc, char* argv[]);
static int dhcp6c_cmd(int argc, char* argv[]);
extern int  dhcp6c_stop(int argc, char* argv[]);
extern int dhcp6c_release(void );
#endif


//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
extern void cyg_kmem_print_stats(void);
extern void show_network_tables(void *);
extern void show_network_interface(void *, char * intf);
extern int net_show_stat(void);
extern int get_def_route(unsigned long *);
extern int LAN_start(int);
extern int lan_set_ip(void);
extern int dump_phy(void);
extern int LAN_show(void);
extern int mii_read_register(int, int);
extern void mii_write_register(int, int, int);
extern int net_ping_test(unsigned int, int, int);
#ifdef CYGPKG_NET_INET6
extern int icmp_ping6(struct in6_addr *dst_v6, int count, char * name, int datasize);
#endif
extern unsigned int nstr2ip(char *);
#if MODULE_DHCPC
extern void DHCPC_release_one(const char *);
extern void DHCPC_renew(const char *);
extern int DHCPC_remaining_time(const char *);
#endif
#if MODULE_DHCPS
extern int DHCPD_dumplease(void *, int, void *);
#endif

extern void netarp_get_all(void);
extern void netarp_flush(void);
extern int netarp_add(struct in_addr *, char *, char *, int);
extern int netarp_del(struct in_addr *, char *);

#if MODULE_DNS
extern void DNS_shutdown(void);
extern void DNS_update(void);
extern void DNS_server_show(void);
extern void DNS_cache_dump(void);
extern void DNS_debug_level(int level);
#endif

#if MODULE_80211X_DAEMON
extern int DOT1X_Start(void);
extern int DOT1X_Stop(void);
extern void Dot1x_Reboot(void);
extern void Dot1x_debug_level(int level);
#endif // MODULE_80211X_DAEMON //


#if MODULE_ETH_LONG_LOOP
extern void clear_max_gain_flag(int port_num);
extern void set_100M_extend_setting_manual(int port_num);
extern void set_100M_normal_setting(int port_num);
extern void set_10M_setting(int port_num);
#endif // MODULE_ETH_LONG_LOOP //


#ifdef CYGPKG_NET_INET6
extern int rtadvd_start(int argc, char *argv[]);
extern void rtadvd_stop();
extern int pimd_start(int argc, char *argv[]);
extern void pimd_stop();
static int set_host(int ,char**);
static int ipv6_tunnel_6to4_cmd(int argc,char *argv[]);

#endif
#ifdef CONFIG_mDNS_Bonjour
extern int  mDNS_responder_init(int argc,char * argv[]);
static int mdns_cmd(int argc ,  char *argv[]);

#endif

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
stCLI_cmd net_cmds[] =
{
	{ "NET" , &net_cmds_init, 0, (CLI_CMD_FLAG_HIDE|CLI_CMD_FLAG_FIRST), CLI_ROOT_CMD, 0 },
	{ "show" , &net_show_cmd, "show stat/kmem/route/<interface name>", 0, 0, 0 },
#if	MODULE_BRIDGE
	{ "br" , &net_brg_cmd, "bridge config", 0, 0, 0 },
#endif
	{ "eth" , &net_eth_cmd, "ether config", CLI_NETCMD_FLAG, 0, 0 },
	{ "mon" , &net_mon_cmd, "monitor cmd", CLI_NETCMD_FLAG, 0, 0 },
	{ "ping" , &net_ping_cmd, "ping dst_ip <timeout> <loop>", 0, 0, 0 },
#ifdef CYGPKG_NET_INET6
	{ "ping6" , &net_ping6_cmd, "ping6 dst_ip <interface> <count> <datasize>", 0, 0, 0 },
#endif
#if	MODULE_DHCPC
	{ "dhcpc" , &net_dhcpc_cmd, "dhcpc command", 0, 0, 0 },
#endif
#if	MODULE_DHCPS
	{ "dhcpd" , &net_dhcpd_cmd, "dhcpd command", 0, 0, 0 },
#endif
	{ "arp" , &net_arp_cmd, "arp command", 0, 0, 0 },
#if	(MODULE_PPPOE == 1)
	{ "pppoe", &net_pppoe_cmd, "pppoe command",0,0,0},
#endif
#if	MODULE_NTP
	{ "ntp", &net_ntp_cmd, "ntp command",0,0,0},
#endif
#if	MODULE_DNS
	{ "dns" , &net_dns_cmd, "dns;  dns restart;", 0, 0, 0 },
#endif
#if	MODULE_NAT
	{ "ipnat" , &net_ipnat_cmd, "ipnat command", 0, 0, 0 },
#endif
#if	MODULE_FW
	{ "fw" , &net_fw_cmd, "fw command",0,0,0},
#endif
	{ "route" , &net_route_cmd, "route config",0,0,0},
#ifdef CONFIG_SNIFFER_DAEMON
	{ "sniffer" , &SnifferDebugCmd, "sniffer debug <value>\n", 0, 0, 0},
#endif
#if	MODULE_UPNP
	{ "upnp" , &upp_cmd, "upnp show\nupnp debug <value>\n", 0, 0, 0 },
#endif
	{ "timer" , &net_timer_cmd, "show timer obj", CLI_NETCMD_FLAG, 0, 0 },
#ifdef	CONFIG_HTTPD
	{ "http" , &net_http_cmd, "http command", 0, 0, 0 },
#endif
	{ "ifconfig" , &ifconfig_cmd, "ifconfig xxx up/down", CLI_NETCMD_FLAG, 0, 0 },
#if MODULE_ATE_DAEMON
	{ "ated" , &ated_cmd, "", CLI_NETCMD_FLAG, 0, 0 },
#endif // MODULE_ATE_DAEMON //

#if MODULE_80211X_DAEMON
	{ "rtdot1xd" , &rtdot1xd_cmd, "", CLI_NETCMD_FLAG, 0, 0 },
#endif // MODULE_80211X_DAEMON //

#if MODULE_ETH_LONG_LOOP
	{ "ethlongloop" , &ethlongloop_cmd, "", CLI_NETCMD_FLAG, 0, 0 },
#endif // MODULE_ETH_LONG_LOOP //


#if MODULE_LLTD_DAEMON
	{ "lltd" , &lltd_cmd, "", CLI_NETCMD_FLAG, 0, 0 },
#endif // MODULE_LLTD_DAEMON //
#ifdef CYGPKG_NET_INET6
	{ "rtadvd" , &rtadvd_cmd, "", CLI_NETCMD_FLAG, 0, 0 },
	{ "pim6dd" , &pimd_cmd, "", CLI_NETCMD_FLAG, 0, 0 },
	{ "route6" , &net_route6_cmd, "", CLI_NETCMD_FLAG, 0, 0 },
#ifdef CONFIG_DHCPV6_CLIENT
	{ "dhcp6c",       &dhcp6c_cmd,			   "", CLI_NETCMD_FLAG, 0, 0},
#endif 
#ifdef CONFIG_DHCPV6_SERVER
	{ "dhcp6s", 	  &dhcp6s_cmd, 		   "", CLI_NETCMD_FLAG, 0, 0},
#endif
	{ "host",       &set_host,			   "", CLI_NETCMD_FLAG, 0, 0},
	{ "6to4",       &ipv6_tunnel_6to4_cmd,			   "", CLI_NETCMD_FLAG, 0, 0},
	{ "clear_neighbor_cache",       &clear_neighbor_cache,			   "", CLI_NETCMD_FLAG, 0, 0},
#endif
#ifdef CONFIG_mDNS_Bonjour
	{ "mDNS_Responder",		&mdns_cmd,			   "", CLI_NETCMD_FLAG, 0, 0},
#endif

};


#include <cfg_def.h>
static int net_mon_cmd(int argc, char* argv[])
{
	int cmd;
	if (argc<1) return -1;
	if (1==sscanf(argv[0],"%x",&cmd))
	{
		printf("send cmd:%08x\n", cmd);
		mon_snd_cmd(cmd);
	}
	return 0;
}


static int net_cmds_init(int argc, char* argv[])
{
	// test cmds
	return CLI_init_cmd(&net_cmds[0], CLI_NUM_OF_CMDS(net_cmds) );
}


static int net_show_cmd(int argc, char* argv[])
{
    if (argc == 0)
    {
        printf("show stat/kmem/route/<interface name>\n");
        return -1;
    }
    else if (!strcmp(argv[0], "stat"))
    {
	    netif_show_stat(1);
	}
    else if (!strcmp(argv[0], "interface"))
    {
	    netif_show_stat(0);
	}
	else if (!strcmp(argv[0], "kmem"))
	{
	    cyg_kmem_print_stats();

#ifdef CYGDBG_NET_TIMING_STATS
	    show_net_times();
#endif
    }
    else if (!strcmp(argv[0], "route"))
    {
        show_network_tables(&printf);
    }
    else
		show_network_interface(&printf, argv[0]);

    return 0;
}

#if	MODULE_BRIDGE
static int net_brg_cmd(int argc, char* argv[])
{
	if ( argc < 1 )
	{
		printf("br show\n\r");
		return -1;
	}
	BRG_show();

    return 0;
}
#endif


static int net_eth_cmd(int argc, char* argv[])
{
	if ( argc < 1 )
	{
		printf("ether start/stop/restart/net/show/phy\n\r");
		return -1;
	}

	if ( !strcmp(argv[0], "start" ))
		LAN_start(1);
	else if ( !strcmp(argv[0], "stop" ))
		LAN_start(0);
	else if ( !strcmp(argv[0], "restart" ))
		LAN_start(2);
	else if ( !strcmp(argv[0], "net" ))
		lan_set_ip();
	else if ( !strcmp(argv[0], "phy" ))
		dump_phy();
    else
		LAN_show();

    return 0;
}


static int net_ping_cmd(int argc, char* argv[])
{
	int loop=1, i, rtt, dst;
	int ping_time=3000;

	if (argc < 1)
	{
		printf("ping dst_ip <timeout> <loop>.\n");
		return -1;
	}

	if (argc > 1)	// timeout
		ping_time=atoi(argv[1]);

	if (argc > 2)	// loop count
		loop=atoi(argv[2]);

	if((dst = nstr2ip(argv[0])) == 0)
	{
		printf("unknown host\n");
		return -1;
	}

	if (loop <0 ||loop > 10000)
		{
		diag_printf("[%s:%d]loop <0 or loop >10000\n",__FUNCTION__,__LINE__);
		return -1;
		}
	for(i=0; i<loop; i++)
	{
		if((rtt = net_ping_test(dst, i, ping_time)) < 0)
			printf("timeout\n");
	}
	return 0;
}


#ifdef CYGPKG_NET_INET6
//ping6 fe80::0200:00ff:fe00:0100 1 2
//ping6 ff1e::1:2 1 1452
static int net_ping6_cmd(int argc, char* argv[])
{
	char interface_name[20];
	int count=3, datasize=64;
	struct in6_addr dst_v6;
	int i =0;

	struct addrinfo hints, *res;
	int gai_error;
	if (argc < 1)
	{
		printf("ping6 dst_ip < -I interface_name> <-c count> <-s datasize>\n");
		return -1;
	}
	bzero(&dst_v6, sizeof(struct in6_addr));
	//inet_pton(AF_INET6, argv[0], &dst_v6);

	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET6;
	gai_error = getaddrinfo(argv[0], NULL, &hints, &res);
	if (gai_error) {
		diag_printf("\n%s: %s: %s\n", __FUNCTION__,argv[0],
			gai_strerror(gai_error));
		return 1;
	}
	dst_v6 = ((struct sockaddr_in6 *)res->ai_addr)->sin6_addr;

	freeaddrinfo(res);

	if((dst_v6.s6_addr32[0] == 0)
		&& (dst_v6.s6_addr32[1] == 0)
		&& (dst_v6.s6_addr32[2] == 0)
		&& (dst_v6.s6_addr32[3] == 0) )
	{
		printf("unknown host\n");
		return -1;
	}

	bzero(interface_name, 20);
	i = 1;
	while ( i < argc)
		{
		if (!strcmp(argv[i], "-I" ))
			{
			if (i< argc -1){
			strcpy(interface_name,argv[i+1]);
			i++;
			i++;
			continue;
			if ((!strcmp(argv[i], "-s" ))  || (!strcmp(argv[i], "-c" )) )
				goto error;
						} else
			goto error;
			}
		else if (!strcmp(argv[i], "-c" ))
			{
			if (i< argc -1){
			count=atoi(argv[i+1]);
			i++;
			i++;
			continue;
			} else
			goto error;
		}
		else if (!strcmp(argv[i], "-s" ))
			{
				if (i< argc -1){
				datasize=atoi(argv[i+1]);
				i++;
				i++;
				continue;
					} else
					goto error;
			}
	else
		goto error;

	}

	 if (strlen(interface_name)==0 &&( IN6_IS_ADDR_MULTICAST(&dst_v6)||
		IN6_IS_ADDR_LINKLOCAL(&dst_v6)||IN6_IS_ADDR_SITELOCAL(&dst_v6)))
		{
		printf("interface_name  is needed for multicast and linklocal address\n");
		return -1;
		}

	 if (strlen(interface_name)&&strcmp(interface_name, "eth0" ) && strcmp(interface_name, "eth1" ) && strcmp(interface_name, "ra0" ))
 	{
	 printf("interface name  is UNknow\n");
	 return -1;
 	}



	icmp_ping6(&dst_v6, count, interface_name, datasize);

	return 0;
error:

	printf("ping6 dst_ip < -I interface_name> <-c count> <-s datasize>\n");
	return -1;
}
#endif


#if	MODULE_DHCPC
static int net_dhcpc_cmd(int argc, char* argv[])
{
	if ( argc < 2 )
	{
		printf("dhcpc release/renew/remain ifname\n\r");
		return -1;
	}

	if ( !strcmp(argv[0], "release" ))
		DHCPC_release_one(argv[1]);
	else if (!strcmp(argv[0], "renew" ))
		DHCPC_renew(argv[1]);
	else if (!strcmp(argv[0], "remain" ))
		printf("remaining time:%d secs\n", DHCPC_remaining_time(argv[1]));

	return 0;
}
#endif


#if	MODULE_DHCPS
void DHD_entry(char *buf, int id, void *arg1)
{
	printf("%s\n", buf);
}

static int net_dhcpd_cmd(int argc, char* argv[])
{

	if ( argc < 1 )
	{
		printf("dhcpd leases/all\n\r");
		return -1;
	}

	if ( !strcmp(argv[0], "leases" ))
		DHCPD_dumplease(DHD_entry, 3, NULL);
	else if ( !strcmp(argv[0], "all" ))
		DHCPD_dumplease(DHD_entry, 0xb, NULL);

	return 0;
}
#endif


static int net_arp_cmd(int argc, char* argv[])
{
	struct in_addr	host;
	char *ifname = NULL;
	char *hwaddr;
	int flags=0;

	if ( argc < 1 )
	{
		printf("Usage: arp show/flush/add/del\n");
		return -1;
	}

	if ( !strcmp(argv[0], "show" ))
		netarp_get_all();
	else if ( !strcmp(argv[0], "flush" ))
		netarp_flush();
	else if ( !strcmp(argv[0], "add")) {
		if (argc < 3) {
			printf("Usage: arp add ip hwmac [flags] [if]\n");
			return -1;
		}
		host.s_addr = inet_addr(argv[1]);
		hwaddr = ether_aton(argv[2]);
		if (argc >=4)
			sscanf(argv[3], "%x", &flags);
		if (argc >=5)
			ifname = argv[4];
		netarp_add(&host, hwaddr, ifname, flags);
	}
	else if ( !strcmp(argv[0], "del")) {
		if (argc < 2) {
			printf("Uasge: arp del ip [if]\n");
			return -1;
		}
		host.s_addr = inet_addr(argv[1]);
		if (argc >=3)
			ifname = argv[2];
		netarp_del(&host, ifname);
	}
	return 0;
}


#if MODULE_DNS
static int net_dns_cmd(int argc, char* argv[])
{
        if (( argc == 1 ) && (!strcmp(argv[0], "restart" )))
        {
                DNS_shutdown();
                DNS_update();
        }
	else if (( argc == 2 ) && (!strcmp(argv[0], "debug" )))
	{
                int value = 0;

                value = atoi(argv[1]);
                DNS_debug_level(value);
	}
        else
        {
                DNS_server_show();
                DNS_cache_dump();
        }

        return 0;
}
#endif


#if	MODULE_NAT
static int net_ipnat_cmd(int argc, char* argv[])
{
	ipnat_cmd(argc, argv);
	return 0;
}
#endif


#ifdef	CONFIG_FW
static int net_fw_cmd(int argc, char* argv[])
{


     return FW_cmd(argc,argv);
}
#endif


#if (MODULE_PPPOE == 1)
static int net_pppoe_cmd(int argc, char* argv[])
{
	 return pppoe_cmd(argc,argv);
}
#endif


#if MODULE_NTP
static int net_ntp_cmd(int argc, char* argv[])
{
	ntp_getlocaltime(1);
	return 0;
}
#endif

static int net_route_cmd(int argc, char* argv[])
{
	int addflag = 0;
	struct in_addr dst, mask, gw;
	char *ifname=NULL;
	struct in_addr *pgw = NULL;

	if ( argc < 1)
		goto __usage;

	if (!strcmp(argv[0], "flush" ))
	{
		RT_staticrt_flush();
		return 0;
	}
	else if (!strcmp(argv[0], "tree" ))
	{
        extern void rn_treeview(struct radix_node * rn, int depth);
        rn_treeview(rt_tables[AF_INET]->rnh_treetop, 0);
        return 0;
	}

	if ( argc < 4 )
		goto __usage;

    if ( !strcmp(argv[0], "add" ))
		addflag = 1;
	else if ( !strcmp(argv[0], "del" ))
		addflag = 0;
    else
		goto __usage;

	dst.s_addr = inet_addr(argv[1]);
	mask.s_addr = inet_addr(argv[2]);
	if (argv[3][0] != '-') {
		gw.s_addr = inet_addr(argv[3]);
		pgw = &gw;
	}
	if (argc > 4)
		ifname = argv[4];

	if(addflag)
		RT_add(ifname, &dst, &mask, pgw, 1);
	else
		RT_del(ifname, &dst, &mask, pgw, 1);
    return 0;

__usage:
    printf("route add <ip> <mask> <gateway> [if]\n\r");
    printf("route del <ip> <mask> <gateway> [if]\n\r");
    printf("route flush\n\r");
    printf("route tree\n\r");
	return 0;
}


#ifdef	CONFIG_HTTPD
static int net_http_cmd(int argc, char* argv[])
{
	if ( argc < 1 )
	{
		printf("http logout\n\r");
		return -1;
	}

	if ( !strcmp(argv[0], "logout" ))
		HTTP_logout(0);

	return 0;
}
#endif


static int ifconfig_cmd(int argc, char* argv[])
{

    if (argc == 0)
    {
        char *new_argv[1] = {"interface"};
        net_show_cmd(1, new_argv);
    }
    else if(argc == 1)
    {
        net_show_cmd(argc, argv);
    }
    else if(argc == 2) // ifconfig ra0 up;  ifconfig ra0 down
    {
	if ( !strcmp(argv[1], "up" ))
		netif_up(argv[0]);
        if ( !strcmp(argv[1], "down" ))
		netif_down(argv[0]);
    }
    else if(argc == 3)
    {
#ifdef INET6
	//ifconfig eth0 mtu 1280
	if ( !strcmp(argv[1], "mtu" )) {
		struct sockaddr_in *addrp;
		struct ifreq ifr;
		int s;

		strcpy(ifr.ifr_name, argv[0]);
		s = socket(AF_INET, SOCK_DGRAM, 0);
		if (s < 0)
		{
			return false;
		}

		ifr.ifr_mtu = atoi(argv[2]);
		if (ioctl(s, SIOCSIFMTU, &ifr))
		{
			perror("SIOCSIFMTU");
		}
		close(s);
	} else
#endif
	{
	    	struct ip_set the_ip_set;
			buid_ip_set(&the_ip_set, argv[0], inet_addr(argv[1]), inet_addr(argv[2])
				, 0, 0 , 0, 0, STATICMODE, NULL, NULL );
			netif_cfg(argv[0], &the_ip_set);
			netif_up(argv[0]);
			netif_up(argv[0]);
			netif_up(argv[0]);
	        net_show_cmd(1, argv);
	}
    }
#ifdef INET6
//ifconfig eth1 inet6 3ffe:501:ffff:0:20c:43ff:fe76:2066 prefixlen 64 alias
//ifconfig eth1 inet6 8000::20c:43ff:fe76:2066 prefixlen 64 alias
//ifconfig eth1 inet6 fec0::20c:43ff:fe76:2066 prefixlen 64 alias
    else if(argc >= 6)
    {
		int socv6;
		struct in6_aliasreq areq6;
		struct sockaddr_in6 *addrp6;
		struct in6_addr ipa_v6;
		struct in6_addr mask_v6;
		int prefixlen = 0;

		inet_pton(AF_INET6, argv[2], &ipa_v6);

		socv6=socket(AF_INET6, SOCK_DGRAM, 0);
		if (socv6 == -1) {
			diag_printf("%s:%d --------------------> Bad fd\n", __FUNCTION__, __LINE__);
			return -1;
		  }

		memset(&areq6, 0, sizeof(struct in6_aliasreq));
		addrp6 = (struct sockaddr_in6 *) &areq6.ifra_addr;
		memset(addrp6, 0, sizeof(*addrp6));
		if (( !strcmp(argv[4], "default" )) ||( !strcmp(argv[4], "64" ))) {
			prefixlen = 64;
			inet_pton(AF_INET6, "ffff:ffff:ffff:ffff:0000:0000:0000:0000", &mask_v6);
		} else if ( !strcmp(argv[4], "32" )) {
			prefixlen = 32;
			inet_pton(AF_INET6, "ffff:ffff:0000:0000:0000:0000:0000:0000", &mask_v6);
		} else if ( !strcmp(argv[4], "96" )) {
			prefixlen = 96;
			inet_pton(AF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:0000:0000", &mask_v6);
		} else if ( !strcmp(argv[4], "128" )) {
			prefixlen = 128;
			inet_pton(AF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", &mask_v6);
		} else {
			prefixlen = atoi(argv[4]);
			in6_prefixlen2mask(&mask_v6,prefixlen);
		}
		if ((argc == 7) && (!strcmp(argv[6], "anycast" )))
			areq6.ifra_flags |= IN6_IFF_ANYCAST;

		addrp6->sin6_family = AF_INET6;
		addrp6->sin6_len = sizeof(*addrp6);
		addrp6->sin6_port = 0;

		strcpy(areq6.ifra_name, argv[0]);
		memcpy(&addrp6->sin6_addr, &ipa_v6, sizeof(struct in6_addr));

		areq6.ifra_prefixmask.sin6_len = sizeof(struct sockaddr_in6);
		areq6.ifra_prefixmask.sin6_family = AF_INET6;
		memcpy(&areq6.ifra_prefixmask.sin6_addr, &mask_v6, sizeof(struct in6_addr));

		areq6.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME; //Eddy6 Todo
		areq6.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;

		if ( !strcmp(argv[5], "alias" )) {
			if (ioctl(socv6, SIOCAIFADDR_IN6, &areq6))
			{
				diag_printf("%s(%d): SIOCSIFADDR Fail\n", __FUNCTION__, __LINE__);
				close(socv6);
				return -1;
			}
		} else if ( !strcmp(argv[5], "delete" )) {
			if (ioctl(socv6, SIOCDIFADDR_IN6, &areq6))
			{
				diag_printf("%s(%d): SIOCSIFADDR Fail\n", __FUNCTION__, __LINE__);
				close(socv6);
				return -1;
			}
		}

		close(socv6);
    }
#endif /* INET6 */
    else
	{
        printf("ifconfig <interface> mtu <value>\n");
        printf("ifconfig <interface> <ip address> <netmask>\n");
        printf("ifconfig <interface> inet6 <ip6 address> prefixlen 64 <alias|delete> <anycast>\n");
        return -1;
    }
    return 0;
}

static int net_timer_cmd(int argc, char* argv[])
{
//Eddy6 	list_timeout_timer();
	return 0;
}

#if MODULE_ATE_DAEMON
extern int CmdATE(int argc, char* argv[]);
static int ated_cmd(int argc, char* argv[])
{
    CmdATE(argc, argv);
    return 0;
}
#endif // MODULE_ATE_DAEMON //


#if MODULE_80211X_DAEMON
static int rtdot1xd_cmd(int argc, char* argv[])
{
	if ( argc == 1 )
        {
        	if ( !strcmp(argv[0], "start" ))
                        DOT1X_Start();
        	else if (!strcmp(argv[0], "stop" ))
                        DOT1X_Stop();
        	else if (!strcmp(argv[0], "restart" ))
                        Dot1x_Reboot();
                else
                        goto help;
        } else if (( argc == 2 ) && (!strcmp(argv[0], "debug" ))) {
                int value = 0;

                value = atoi(argv[1]);
                Dot1x_debug_level(value);
        } else {
                goto help;
	}

	return 0;
help:
        printf("rtdot1xd start/stop/restart\n\r");
	printf("rtdot1xd debug <level>\n\r");
	return 0;
}
#endif // MODULE_80211X_DAEMON //

#if MODULE_ETH_LONG_LOOP
static int ethlongloop_cmd(int argc, char* argv[])
{
	char port=10;
	if ( argc == 2 )
	{
	    port=atoi(argv[0]);
		
	    if(port >= 0 && port <= 5)
	    {
	    	if ( !strcmp(argv[1], "100M" ))
	    	{
	        	clear_max_gain_flag(port);
				set_100M_normal_setting(port);
				printf("Port#%d set to 100M normal\n",port);
	    	}
	    	else if (!strcmp(argv[1], "100Ex" ))
			{
				clear_max_gain_flag(port);
				set_100M_extend_setting_manual(port);
				printf("Port#%d set to 100M Extend\n",port);
			}

	    	else if (!strcmp(argv[1], "10M" ))
			{
				set_10M_setting(port);
				printf("Port#%d set to 10M\n",port);
			}
	        else
	        	goto help;
	    } 
	    else 
		{
	    	goto help;
		}
	}
	else
		goto help;

	return 0;
help:
    printf("Usage: ethlongloop portNum 100M/100Ex/10M\n\r");
	return 0;
}
#endif // MODULE_ETH_LONG_LOOP //


#if MODULE_LLTD_DAEMON
static int lltd_cmd(int argc, char* argv[])
{
    LLTP_Start();
    return 0;
}
#endif // MODULE_LLTD_DAEMON //

#ifdef CYGPKG_NET_INET6
static int rtadvd_cmd(int argc, char* argv[])
{
	if ( argc > 0 )
        {
		if ( !strcmp(argv[0], "start" ))
			rtadvd_start(argc, argv);
		else if (!strcmp(argv[0], "stop" ))
			rtadvd_stop();
	} else {
		printf("rtadvd start\n\r");
		printf("rtadvd stop\n\r");
	}
	return 0;
}

static int pimd_cmd(int argc, char* argv[])
{
	if ( argc > 0 )
        {
		if ( !strcmp(argv[0], "start" ))
			pimd_start(argc, argv);
		else if (!strcmp(argv[0], "stop" ))
			pimd_stop();
	} else {
		printf("pim6dd start\n\r");
		printf("pim6dd stop\n\r");
	}
	return 0;
}

//route6 add net default gateway fe80::200:ff:fe00:a0a0 eth1
//route6 delete net default gateway fe80::200:ff:fe00:a0a0 eth1
//route6 add net 3ffe:501:ffff::: gateway fe80::200:ff:fe00:a0a0 eth1
//route6 delete net 3ffe:501:ffff::: gateway fe80::200:ff:fe00:a0a0 eth1
static int net_route6_cmd(int argc, char* argv[])
{
	if ( argc == 6)
        {
		struct ifnet *ifp = NULL;

        	ifp = ifunit(argv[5]);
		if (ifp) {
				struct sockaddr_in6 def, mask, gate;
				struct rtentry *newrt = NULL;
				int s;
				int error;

				Bzero(&def, sizeof(def));
				Bzero(&mask, sizeof(mask));
				Bzero(&gate, sizeof(gate));

				def.sin6_len = mask.sin6_len = gate.sin6_len
					= sizeof(struct sockaddr_in6);
				def.sin6_family = mask.sin6_family = gate.sin6_family = AF_INET6;

				inet_pton(AF_INET6, argv[4], &gate.sin6_addr);
				if (IN6_IS_ADDR_LINKLOCAL(&gate.sin6_addr)||IN6_IS_ADDR_SITELOCAL(&gate.sin6_addr))
					gate.sin6_addr.s6_addr16[1] = htons(ifp->if_index);

				s = splnet();
				if (!strcmp(argv[0], "add" )) {
						if (strcmp(argv[2], "default" ))
							{
							inet_pton(AF_INET6, argv[2], &def.sin6_addr);
							inet_pton(AF_INET6, "FFFF:FFFF:FFFF:FFFF::", &mask.sin6_addr);

							}
						int ret=0;
						ret=rtrequest(RTM_ADD, (struct sockaddr *)&def,
						(struct sockaddr *)&gate, (struct sockaddr *)&mask,
						RTF_GATEWAY, &newrt);
						if(ret)
							diag_printf("\nERROR: add route entry fail\n");
				} else if (!strcmp(argv[0], "delete" )) {
					if (strcmp(argv[2], "default" ))
						{
						inet_pton(AF_INET6, argv[2], &def.sin6_addr);
						inet_pton(AF_INET6, "FFFF:FFFF:FFFF:FFFF::", &mask.sin6_addr);

						}
					rtrequest(RTM_DELETE, (struct sockaddr *)&def,
					(struct sockaddr *)&gate, (struct sockaddr *)&mask,
					RTF_GATEWAY, &newrt);
				}
				splx(s);
		}
	} else {
		printf("route6 <add|delete> net <default|prefix> gateway <gateway ip> <interface>\n\r");
	}
	return 0;
}

#ifdef CONFIG_DHCPV6_SERVER
static int dhcp6s_cmd(int argc, char* argv[])
{

	if ( argc > 0 )
        {
		if ( !strcmp(argv[0], "start" ))
			dhcp6s_start(argc, argv);
		else if (!strcmp(argv[0], "stop" ))		
			dhcp6s_stop(argc, argv);
	} else {
		printf("dhcp6s start\n\r");
		printf("dhcp6s stop\n\r");		 	
	}


}
#endif

#ifdef CONFIG_DHCPV6_CLIENT
static int dhcp6c_cmd(int argc, char* argv[])
{

	if ( argc > 0 )
        {
		if ( !strcmp(argv[0], "start" ))
			dhcp6c_start(argc, argv);
		else if (!strcmp(argv[0], "stop" ))		
			dhcp6c_stop(argc, argv);
		else if (!strcmp(argv[0],"release"))
			dhcp6c_release();
	} else {
		printf("dhcp6c start/stop/release\n\r");
		
	}


}
#endif
static int set_host(int argc,char*argv[])
{
	ip6_forwarding=0;
	ip6_accept_rtadv=1;
	diag_printf("\nSet ip6_forwarding=%d\nSet ip6_accept_rtadv = %d\n",ip6_forwarding,ip6_accept_rtadv);
}


char default_gateway[50];
static int tunnel_6to4_start(int argc,char * argv[])
{
	char tm_argv[7][50];
	char *ptr[7];
	extern  struct ip_set primary_wan_ip_set[PRIMARY_WANIP_NUM];
	unsigned short int ip_a,ip_b;
	int i=0;
	unsigned int ip=0;



	memset(tm_argv,0,sizeof(tm_argv));
	ip = ntohl(primary_wan_ip_set[0].ip);
	ip_a=(ip>>16) & 0xffff;
	ip_b = (ip) & 0xffff;

	
	for (i=0;i<7;i++)
		ptr[i]=tm_argv[i];	
	/*
	*ifconfig stf0 inet6 2002:c00:000c::1 prefixlen 16 alias
	*/

	strcpy(tm_argv[0],"stf0");
	strcpy(tm_argv[1],"inet6");
	sprintf(tm_argv[2],"2002:%x:%x::1",ip_a,ip_b);
	strcpy(tm_argv[3],"prefixlen");
	strcpy(tm_argv[4],"16");	
	strcpy(tm_argv[5],"alias");

	ifconfig_cmd(6,ptr);
	/*
	*ifconfig eth0 inet6 2002:c00:000c::1 prefixlen 64 alias
	*/
	strcpy(tm_argv[0],"eth0");
	strcpy(tm_argv[1],"inet6");
	sprintf(tm_argv[2],"2002:%x:%x::2",ip_a,ip_b);
	strcpy(tm_argv[3],"prefixlen");
	strcpy(tm_argv[4],"64");	
	strcpy(tm_argv[5],"alias");
	ifconfig_cmd(6,ptr);
	/*
	*route6 add net default gateway fe80::200:ff:fe00:a0a0 eth1
	*/

	strcpy(tm_argv[0],"add");
	strcpy(tm_argv[1],"net");
	strcpy(tm_argv[2],"default");
	strcpy(tm_argv[3],"gateway");	
	if (argc >=2)
		{
		if (!strcmp(argv[1],"default_gateway"))
			strcpy(tm_argv[4],argv[2]);
		} else 
		strcpy(tm_argv[4],"2002:c058:6301::1");
		strcpy(tm_argv[5],"stf0");
	strcpy(default_gateway,tm_argv[4]);

	net_route6_cmd(6,ptr);

	/*
	*rtadvd start eth0
	*/
	strcpy(tm_argv[0],"start");
	strcpy(tm_argv[1],"eth0");
	sprintf(tm_argv[2],"addrs=2002:%x:%x::",ip_a,ip_b);
	rtadvd_cmd(3,ptr);

	diag_printf("\n%s:6to4 tunnel SETUP finished\n",__FUNCTION__);

	
}


static int tunnel_6to4_stop(int argc, char *argv[])
	{
		char tm_argv[7][50];
		char *ptr[7];
		extern	struct ip_set primary_wan_ip_set[PRIMARY_WANIP_NUM];
		unsigned short int ip_a,ip_b;
		int i=0;
		unsigned int ip=0;



		memset(tm_argv,0,sizeof(tm_argv));
		ip = ntohl(primary_wan_ip_set[0].ip);
		ip_a=(ip>>16) & 0xffff;
		ip_b = (ip) & 0xffff;

		for (i=0;i<7;i++)
			ptr[i]=tm_argv[i];	
		/*
		*ifconfig stf0 inet6 2002:c00:000c::1 prefixlen 16 delete
		*/
	
		strcpy(tm_argv[0],"stf0");
		strcpy(tm_argv[1],"inet6");
		sprintf(tm_argv[2],"2002:%x:%x::1",ip_a,ip_b);
		strcpy(tm_argv[3],"prefixlen");
		strcpy(tm_argv[4],"16");	
		strcpy(tm_argv[5],"delete");
	
		ifconfig_cmd(6,ptr);
		/*
		*ifconfig eth0 inet6 2002:c00:000c::1 prefixlen 64 delete
		*/
		strcpy(tm_argv[0],"eth0");
		strcpy(tm_argv[1],"inet6");
		sprintf(tm_argv[2],"2002:%x:%x::2",ip_a,ip_b);
		strcpy(tm_argv[3],"prefixlen");
		strcpy(tm_argv[4],"64");	
		strcpy(tm_argv[5],"delete");
		ifconfig_cmd(6,ptr);
		/*
		*route6 delete net default gateway fe80::200:ff:fe00:a0a0 eth1
		*/
	
		strcpy(tm_argv[0],"delete");
		strcpy(tm_argv[1],"net");
		strcpy(tm_argv[2],"default");
		strcpy(tm_argv[3],"gateway");	
		strcpy(tm_argv[4],default_gateway);
		strcpy(tm_argv[5],"stf0");

		net_route6_cmd(6,ptr);
	
		/*
		*rtadvd stop eth0
		*/
		strcpy(tm_argv[0],"stop");
		strcpy(tm_argv[1],"eth0");
		rtadvd_cmd(2,ptr);
	
		diag_printf("\n%s:6to4 tunnel STOP finished\n",__FUNCTION__);
	
		
	}


static int ipv6_tunnel_6to4_cmd(int argc,char *argv[])
{
	if ( argc > 0 )
        {
		if ( !strcmp(argv[0], "start" ))
			tunnel_6to4_start(argc, argv);
		else if (!strcmp(argv[0], "stop" ))		
			tunnel_6to4_stop(argc, argv);
	} else {
		printf("6to4 start/stop  [default_gateway <gateway>]\n\r");	
	}

}



#endif


#ifdef CONFIG_mDNS_Bonjour

extern volatile unsigned char gStopNow;
static int mdns_cmd(int argc ,  char *argv[])
{

if (argc >0)
	{
	if (strcmp(argv[0],"start")==0)
			{
			char tm_argv[7][50];
			char *ptr[7];
			int i=0;


			memset(tm_argv,0,sizeof(tm_argv));
			for (i=0;i<7;i++)
				ptr[i]=tm_argv[i];	

			/*
			*route add 224.0.0.251 255.0.0.0 10.10.10.1 eth0
			*/
		
			strcpy(tm_argv[0],"add");
			strcpy(tm_argv[1],"224.0.0.251");
			strcpy(tm_argv[2],"255.0.0.0");
			strcpy(tm_argv[3],"10.10.10.1");	
			strcpy(tm_argv[4],"eth0");
			net_route_cmd(5,ptr);
			mDNS_responder_init( argc, argv);
		}
	else if(strcmp(argv[0],"stop")==0)
		gStopNow =1;
	else
		diag_printf("mDNS_Responder start/stop\n");
	}else
	diag_printf("mDNS_Responder start/stop\n");


return 0;
	
}
#endif

