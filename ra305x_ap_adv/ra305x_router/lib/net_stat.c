#include <pkgconf/system.h>
#include <pkgconf/isoinfra.h>
#include <pkgconf/net.h>
#include <pkgconf/kernel.h>

#include <cyg/infra/cyg_trac.h>        /* tracing macros */
#include <cyg/infra/cyg_ass.h>         /* assertion macros */

#include <cyg/kernel/kapi.h>
#ifdef CYGPKG_KERNEL_INSTRUMENT
#include <cyg/kernel/instrmnt.h>
#include <cyg/kernel/instrument_desc.h>
#endif

#include <network.h>

#include <unistd.h>
#include <ctype.h>

/* ================================================================= */
/* Include all the necessary network headers by hand. We need to do
 * this so that _KERNEL is correctly defined, or not, for specific
 * headers so we can use both the external API and get at internal
 * kernel structures.
 */


#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/time.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <net/route.h>
#include <net/if_dl.h>

#include <sys/protosw.h>
#include <netinet/in_pcb.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/ip_var.h>
#include <netinet/icmp_var.h>
#include <netinet/udp_var.h>
#include <netinet/tcp_var.h>

#include <cyg/io/eth/eth_drv_stats.h>

#include <cfg_net.h>

extern char * CGI_get_wlan_tx_rx_count(void);

static long long wlan_tx_cnt_base;
static long long wlan_rx_cnt_base;

int netif_show_stat(int display_level)
{
    struct ifconf ifconf;
    char conf[1024];
    int err;
    int sock;

    /* Start by opening a socket and getting a list of all the
     * interfaces present.
     */
    {
        memset( conf, 0, sizeof(conf) );
    
        sock = socket( AF_INET, SOCK_DGRAM, 0 );
        if(sock < 0)
        	return -1;
    
        ifconf.ifc_len = sizeof(conf);
        ifconf.ifc_buf = (caddr_t)conf;

        err = ioctl( sock, SIOCGIFCONF, &ifconf );
    }

    // "Interfaces"
    show_network_interface(&printf, NULL);

    /* Now the protocols. For each of the main protocols: IP,
     * ICMP, UDP, TCP print a table of useful information derived
     * from the in-kernel data structures. Note that this only
     * works for the BSD stacks.
     */
    // "Protocols"
    if (display_level == 1)        
    {
        printf("---- IP ----\n");
        
		printf("Received\n" );
        printf("       Total %u\n", ipstat.ips_total );
		printf("         Bad %u\n",
                         ipstat.ips_badsum+
                         ipstat.ips_tooshort+
                         ipstat.ips_toosmall+
                         ipstat.ips_badhlen+
                         ipstat.ips_badlen+
                         ipstat.ips_noproto+
                         ipstat.ips_toolong
                    );
		printf( " Reassembled %u\n", ipstat.ips_reassembled );
		printf( "   Delivered %u\n", ipstat.ips_delivered );
		printf( "Sent \n" );
		printf( "       Total %u\n", ipstat.ips_localout );
        printf( "         Raw %d\n", ipstat.ips_rawout );
        printf( "  Fragmented %u\n", ipstat.ips_fragmented );

        printf("---- ICMP ----\n");

        printf( "Received\n" );
        printf( "        ECHO %u\n", icmpstat.icps_inhist[ICMP_ECHO] );
        printf( "  ECHO REPLY %u\n", icmpstat.icps_inhist[ICMP_ECHOREPLY] );
        printf( "     UNREACH %u\n", icmpstat.icps_inhist[ICMP_UNREACH] );
        printf( "    REDIRECT %u\n", icmpstat.icps_inhist[ICMP_REDIRECT] );
        printf( "       Other %u\n",
                 icmpstat.icps_inhist[ICMP_SOURCEQUENCH]+
                 icmpstat.icps_inhist[ICMP_ROUTERADVERT]+
                 icmpstat.icps_inhist[ICMP_ROUTERSOLICIT]+
                 icmpstat.icps_inhist[ICMP_TIMXCEED]+
                 icmpstat.icps_inhist[ICMP_PARAMPROB]+
                 icmpstat.icps_inhist[ICMP_TSTAMP]+
                 icmpstat.icps_inhist[ICMP_TSTAMPREPLY]+
                 icmpstat.icps_inhist[ICMP_IREQ]+
                 icmpstat.icps_inhist[ICMP_IREQREPLY]+
                 icmpstat.icps_inhist[ICMP_MASKREQ]+
                 icmpstat.icps_inhist[ICMP_MASKREPLY]
             );
        printf( "         Bad %u\n",
                 icmpstat.icps_badcode+
                 icmpstat.icps_tooshort+
                 icmpstat.icps_checksum+
                 icmpstat.icps_badlen+
                         icmpstat.icps_bmcastecho
              );

        printf( "Sent\n" );
        printf( "        ECHO %u\n", icmpstat.icps_outhist[ICMP_ECHO] );
        printf( "  ECHO REPLY %u\n", icmpstat.icps_outhist[ICMP_ECHOREPLY] );
        printf( "     UNREACH %u\n", icmpstat.icps_outhist[ICMP_UNREACH] );
        printf( "    REDIRECT %u\n", icmpstat.icps_outhist[ICMP_REDIRECT] );
        printf( "       Other %u\n", 
                 icmpstat.icps_inhist[ICMP_SOURCEQUENCH]+                             
                 icmpstat.icps_outhist[ICMP_ROUTERADVERT]+
                 icmpstat.icps_outhist[ICMP_ROUTERSOLICIT]+
                 icmpstat.icps_outhist[ICMP_TIMXCEED]+
                 icmpstat.icps_outhist[ICMP_PARAMPROB]+
                 icmpstat.icps_outhist[ICMP_TSTAMP]+
                 icmpstat.icps_outhist[ICMP_TSTAMPREPLY]+
                 icmpstat.icps_outhist[ICMP_IREQ]+
                 icmpstat.icps_outhist[ICMP_IREQREPLY]+
                 icmpstat.icps_outhist[ICMP_MASKREQ]+
                 icmpstat.icps_outhist[ICMP_MASKREPLY]
            );

        printf("---- UDP ---- \n");

        printf( "Received\n" );
        printf( "       Total %u\n", udpstat.udps_ipackets );
        printf( "         Bad %u\n",
                         udpstat.udps_hdrops+
                         udpstat.udps_badsum+
                         udpstat.udps_badlen+
                         udpstat.udps_noport+
                         udpstat.udps_noportbcast+
                         udpstat.udps_fullsock
                    );
        printf( "Sent\n" );
        printf( "       Total %u\n", udpstat.udps_opackets );

        printf("---- TCP ---- \n");	
        printf("Connections\n" );
        printf( "   Initiated %u\n", tcpstat.tcps_connattempt );
        printf( "    Accepted %u\n", tcpstat.tcps_accepts );
        printf( " Established %u\n", tcpstat.tcps_connects );
        printf( "      Closed %u\n", tcpstat.tcps_closed );

        printf( "Received\n" );
        printf( "     Packets %u\n", tcpstat.tcps_rcvtotal );
        printf( "Data Packets %u\n", tcpstat.tcps_rcvpack );
        printf( "       Bytes %u\n", tcpstat.tcps_rcvbyte );
        printf( "Sent\n" );
        printf( "     Packets %u\n", tcpstat.tcps_sndtotal );
        printf( "Data Packets %u\n", tcpstat.tcps_sndpack );
        printf( "       Bytes %u\n", tcpstat.tcps_sndbyte );

    }

    close( sock );
    
    return 1;
}

unsigned int netif_clear_cnt(const char * ifn)
{
	char * tx_rx_count = NULL;
	long long wlanTX = 0, wlanRX = 0;
	char	* tok = NULL;
	int i;

	tx_rx_count = CGI_get_wlan_tx_rx_count();

	for (i = 0, tok = strtok(tx_rx_count,"\n"); tok; tok = strtok(NULL,"\n"), i++)
	{
		if (i == 0)
			wlanRX = atoi(tok);
		else if (i == 1)
			wlanTX = atoi(tok);
	}
	
	struct ifnet * ifp;

	ifp=ifunit(ifn);
	if (ifp==0) return 0;
	ifp->if_opackets=0;
	ifp->if_oerrors=0;
	ifp->if_ipackets=0;
	ifp->if_ierrors=0;
	ifp->if_obytes=0;
	ifp->if_ibytes=0;

	wlan_rx_cnt_base = wlanRX;
	wlan_tx_cnt_base = wlanTX;
	
    return 1;
}	

unsigned int netif_get_cnt(const char * ifn, int id)
{
	char * tx_rx_count = NULL;
	long  long wlanTX = 0, wlanRX = 0;
	char	* tok = NULL;
	int i;

	tx_rx_count = CGI_get_wlan_tx_rx_count();

	for (i = 0, tok = strtok(tx_rx_count,"\n"); tok; tok = strtok(NULL,"\n"), i++)
	{
		if (i == 0)
			wlanRX = atoi(tok);
		else if(i == 1)
			wlanTX = atoi(tok);
	}
	
#if 1 //new
        unsigned long long ret;
	struct ifnet * ifp;

	ifp=ifunit(ifn);
	if (ifp==0) return 0;
	switch (id)
	{
		case 1: //tx ok
			ret=ifp->if_opackets;
			break;

		case 2: // tx bad
			ret=ifp->if_oerrors;
			break;

		case 3: //rx ok
			ret=ifp->if_ipackets;
			break;

		case 4: // rx bad
			ret=ifp->if_ierrors;
			break;
		
		case 5: //tx bytes
			ret=ifp->if_obytes;
			break;

		case 6: // rx bytes
			ret=ifp->if_ibytes;
			break;
			
		case 7: // wlan tx ok
			ret=wlanTX - wlan_tx_cnt_base;
			break;		

		case 8: // wlan rx ok
			ret=wlanRX - wlan_rx_cnt_base;
			break;
			
		default:
			ret=0;
			break;
	}
    return ret;
#else
    struct ether_drv_stats ethstats;
    int sock;
    unsigned int ret;

    sock = socket( AF_INET, SOCK_RAW, 0 );

    strcpy(ethstats.ifreq.ifr_name, ifn );

    ioctl( sock, SIOCGIFSTATS, &ethstats ) ;

    close(sock);

    switch (id)
    {
        case 1:
            ret = ethstats.tx_count ;
            break;

        case 2:
            ret = ethstats.tx_dropped ;
            break;

        case 3:
            ret = ethstats.rx_count ;
            break;

        case 4:
            ret = ethstats.rx_crc_errors ;
            break;
            
        default:
            ret = 0;
            break;
        //ethstats.interrupts;
    }
    return ret;
#endif


}

#if 1
/*----------------------------------------------------------------------
* ROUTINE NAME - rtlib_DefaultRoute
*-----------------------------------------------------------------------
* DESCRIPTION: 
*   This routine get the default route from the routing table
*
* INPUT:
*   destination   -- inet addr or name of route destination
*   gateway       -- Internet address to assign to interface
*
* RETURN:    
*   OK, or ERROR if the gateway cannot be get.
*----------------------------------------------------------------------*/
int get_def_route(unsigned long *gateway)
{
    static  int rtm_id = 0;

    struct
    {
	    struct	rt_msghdr m_rtm;
        struct  sockaddr_in m_space[8];
    }
    m_rtmsg;

    int s;
    int i;
    int len;

    int seq = rtm_id++;
    pid_t tid = (pid_t) cyg_thread_self();

    // Allocate a route socket;
    s = socket(PF_ROUTE, SOCK_RAW, 0);
    if (s == -1)
        return -1;

    // Set rt_msghdr
	memset(&m_rtmsg, 0, sizeof(m_rtmsg));
    
#define rtm m_rtmsg.m_rtm

    rtm.rtm_msglen = sizeof(m_rtmsg);
	rtm.rtm_type = RTM_GET;
	rtm.rtm_flags = RTF_UP | RTF_GATEWAY;
	rtm.rtm_version = RTM_VERSION;
    rtm.rtm_pid = tid;
    rtm.rtm_seq = seq;

    rtm.rtm_addrs = RTA_DST | RTA_GATEWAY;

    // Setup the followed 8 socket_in;
    for (i=0; i<8; i++)
    {
        m_rtmsg.m_space[i].sin_len = 16;
        m_rtmsg.m_space[i].sin_family = AF_INET;
    }

    // Write route socekt to inform that we want to get
    if (write(s, (char *)&m_rtmsg, sizeof(m_rtmsg)) != sizeof(m_rtmsg))
    {
        *gateway = 0;
    }
    else
    {
        // Should change to select .... to prevent dead lock!!!
        while (1)
        {
            len = read(s, (char *)&m_rtmsg, sizeof(m_rtmsg));
            if (len < 0)
            {
                *gateway = 0;
                break;
            }
            else if (len >= sizeof(struct rt_msghdr))
            {
                if (rtm.rtm_pid == tid && rtm.rtm_seq == seq)
                {
                    *gateway = ntohl(m_rtmsg.m_space[1].sin_addr.s_addr);
                    break;
                }
            }
        }
    }

    close(s);

#undef rtm

    return 0;
}
#endif

void netif_get_stat(const char * ifn, char * buf, int id)
{
    struct ifreq ifreq;
    int sock;

    sock = socket( AF_INET, SOCK_DGRAM, 0 );
	if(sock < 0)
		return;
    strcpy(ifreq.ifr_name, ifn );

    switch (id)
    {
        case 0: // ip
            /* Get the interface's address and display it. */
            if( ioctl( sock, SIOCGIFADDR, &ifreq ) >= 0 )
            {
                struct sockaddr_in *inaddr =
                    (struct sockaddr_in *)&ifreq.ifr_addr;

                sprintf( buf, "%s", inet_ntoa(inaddr->sin_addr));
            }
            else
                sprintf( buf, "0.0.0.0" );
            break;

        case 1: // net-mask 
            if( ioctl( sock, SIOCGIFNETMASK, &ifreq ) >= 0 )
            {
                struct sockaddr_in *inaddr =
                    (struct sockaddr_in *)&ifreq.ifr_addr;

                sprintf( buf, "%s", inet_ntoa(inaddr->sin_addr));
            }
            else
                sprintf( buf, "0.0.0.0" );
            break;

        case 2:
            {
extern struct ip_set primary_lan_ip_set;
                
                sprintf( buf, "%s", inet_ntoa(*(struct in_addr *)&primary_lan_ip_set.gw_ip) );
			}
			break;

        case 3: // mac address
            if( ioctl( sock, SIOCGIFHWADDR, &ifreq ) >= 0 )
            {
				sprintf(buf, "%s",ether_ntoa(ifreq.ifr_hwaddr.sa_data));
            }
            else
                sprintf( buf, "00:00:00:00:00:00" );
            break;

        default:
            buf[0]=0; //null
            break;
    }
    close(sock);
}



