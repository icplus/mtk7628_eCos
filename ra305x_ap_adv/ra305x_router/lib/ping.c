/*
	ping.c
*/

#include <cyg/kernel/kapi.h>
#include <network.h>
#include <sys/sockio.h>
#ifdef CYGPKG_NET_INET6
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#endif

#define DEFDATALEN    	(64 - 8)    /* default data length */
#define MAXIPLEN    	60
#define MAXICMPLEN    	76
#define	TIMEOUT			4
int ping_id=0;

char SavePingResult[60];

typedef struct
{
    int  s; 
    int  datalen;
    struct sockaddr_in whereto;
    unsigned short seq;
    unsigned short id;
    char outpack[256];
} pinger_t;

static pinger(pinger_t *);
static int show_icmp(unsigned char *pkt, int len, 
    struct sockaddr_in *from, struct sockaddr_in *to);

// Compute INET checksum
#define checksum inet_cksum

int inet_cksum(u_short *addr, int len)
{
    register int nleft = len;
    register u_short *w = addr;
    register u_short answer;
    register u_int sum = 0;
    u_short odd_byte = 0;

    /*
     *  Our algorithm is simple, using a 32 bit accumulator (sum),
     *  we add sequential 16 bit words to it, and at the end, fold
     *  back all the carry bits from the top 16 bits into the lower
     *  16 bits.
     */
    while( nleft > 1 )  {
        sum += *w++;
        nleft -= 2;
    }

    /* mop up an odd byte, if necessary */
    if( nleft == 1 ) {
        *(u_char *)(&odd_byte) = *(u_char *)w;
        sum += odd_byte;
    }

    /*
     * add back carry outs from top 16 bits to low 16 bits
     */
    sum = (sum >> 16) + (sum & 0x0000ffff); /* add hi 16 to low 16 */
    sum += (sum >> 16);                     /* add carry */
    answer = ~sum;                          /* truncate to 16 bits */
    return (answer);
}

int icmp_ping (unsigned int dst, int seq, int timeout)
{ 
    fd_set          fdmask;
    int             packetlen, hold, cc, fromlen;
    pinger_t      	pc;
    struct 			sockaddr_in from, *to;
    struct 			timeval select_timeout; 
    struct ip       *ip;
    struct icmp     *icp;
    char            *packet;

    if (timeout < 10 || timeout > 1000*30)
    {
        timeout = 1000*TIMEOUT; // 4 seconds
    }
    memset((char *)&pc, 0, sizeof(pc));
    pc.datalen = DEFDATALEN;
 
    to = (struct sockaddr_in *)&pc.whereto;
    to->sin_family = AF_INET;
    to->sin_addr.s_addr = htonl(dst);

    packetlen = pc.datalen + MAXIPLEN + MAXICMPLEN;

    if (!(packet = (char *)malloc(packetlen))) 
    {
        printf("\n\rping: out of memory\n");
        return -1;
    }

    if ((pc.s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) 
    {
        printf("\n\rping: socket error\n");
        free(packet);
        return -1;
    }

    hold = 4096;
    
    setsockopt(pc.s, SOL_SOCKET, SO_RCVBUF,
              (char *)&hold, sizeof(hold));


    pc.seq = seq & 0xffff ;
    pc.id  =  cyg_thread_self() & 0xffff ;

    /* send the ping message */
    if (pinger(&pc) < 0)
    {
        free(packet);
        close(pc.s);
        return -1;
    }

    FD_ZERO(&fdmask);
    FD_SET(pc.s, &fdmask);
    select_timeout.tv_sec = 0;
    select_timeout.tv_usec = timeout * 1000;

    if (select(pc.s + 1, (fd_set *)&fdmask, (fd_set *)NULL,
        (fd_set *)NULL, &select_timeout) >= 1)
    {
        fromlen = sizeof(from);
        if ((cc = recvfrom(pc.s, (char *)packet, packetlen, 0,
            (struct sockaddr_in *)&from, &fromlen)) <= 0) 
        {
            free(packet);
            close(pc.s);
            return -1;
        } 
        else 
        {
            ip = (struct ip *) packet;
            icp = (struct icmp *) (((packet[0] & 0xf) << 2) + packet);
            
            if ((icp->icmp_seq == pc.seq) && (icp->icmp_id == pc.id))
            {
                show_icmp(packet, packetlen, &from, dst);            
                free(packet);
                close(pc.s);
                return 0;
            }

        }// else recvfrom
    }  // end of if select

   free(packet);
   close(pc.s);
   return -1;
}

static pinger(pinger_t *pc)
{
    register struct icmp *icp;
    cyg_tick_count_t *tp;    
    register int cc; 

    icp = (struct icmp *)pc->outpack;
    icp->icmp_type = ICMP_ECHO;
    icp->icmp_code = 0;
    icp->icmp_cksum = 0;
    icp->icmp_seq = pc->seq;
    icp->icmp_id = pc->id;            /* ID */
    tp = (cyg_tick_count_t *)&icp->icmp_data;
    *tp = cyg_current_time();

    cc = pc->datalen + 8;            /* skips ICMP portion */

    /* compute ICMP checksum here */
    icp->icmp_cksum = checksum((unsigned short *)icp, cc);
    ping_id = pc->id;
    if (sendto(pc->s, (char *)pc->outpack,
               cc, 0, (struct sockaddr_in *)&pc->whereto,
               sizeof(struct sockaddr_in)) < 0)
    {
        printf("\n\rping: sendto error\n");
        return -1;
    } 
    return(0);
}

static int show_icmp(unsigned char *pkt, int len, 
          struct sockaddr_in *from, struct sockaddr_in *to)
{
    cyg_tick_count_t *tp, tv;
    struct ip *ip;
    struct icmp *icmp;
    tv = cyg_current_time();
    ip = (struct ip *)pkt;
    if ((len < sizeof(*ip)) || ip->ip_v != IPVERSION) {
        diag_printf("%s: Short packet or not IP! - Len: %d, Version: %d\n", 
                    inet_ntoa(from->sin_addr), len, ip->ip_v);
        return 0;
    }
    icmp = (struct icmp *)(pkt + sizeof(*ip));
    len -= (sizeof(*ip) + 8);
    tp = (cyg_tick_count_t *)&icmp->icmp_data;
    if (icmp->icmp_type != ICMP_ECHOREPLY) {
        diag_printf("%s: Invalid ICMP - type: %d\n", 
                    inet_ntoa(from->sin_addr), icmp->icmp_type);
        return 0;
    }
   memset(SavePingResult,0,sizeof(SavePingResult));
    diag_printf("%d bytes from %s: ", len, inet_ntoa(from->sin_addr));
    diag_printf("icmp_seq=%d", icmp->icmp_seq);
    diag_printf(", time=%dms\n", (int)(tv - *tp)*10);
    sprintf(SavePingResult,"%d bytes from %s: icmp_seq=%d time=%dms", len, inet_ntoa(from->sin_addr), icmp->icmp_seq, (int)(tv - *tp)*10);
    return (from->sin_addr.s_addr == to->sin_addr.s_addr);
}

#ifdef CYGPKG_NET_INET6
#define NUM_PINGS 16
#define MAX_PACKET 4096
#define MIN_PACKET   64
#define MAX_SEND   4000

#define UNIQUEID 0x1234
static unsigned char pkt1[MAX_PACKET], pkt2[MAX_PACKET];
static int
show6_icmp(unsigned char *pkt, int len, 
          const struct sockaddr_in6 *from, const struct sockaddr_in6 *to)
{
    cyg_tick_count_t *tp, tv;
    struct icmp6_hdr *icmp;
    char fromnamebuf[128];
    char tonamebuf[128];
    int error;
#if 0
    error = getnameinfo((struct sockaddr *)from,sizeof(*from), 
			fromnamebuf, sizeof(fromnamebuf), 
			NULL, 0,
			NI_NUMERICHOST);
    if (error) {
      perror ("getnameinfo(from)");
      return 0;
    }

    error = getnameinfo((struct sockaddr *)to,sizeof(*to), 
			tonamebuf, sizeof(tonamebuf), 
			NULL, 0,
			NI_NUMERICHOST);
    if (error) {
      perror ("getnameinfo(to)");
      return 0;
    }
#endif
    tv = cyg_current_time();
    icmp = (struct icmp6_hdr *)pkt;
    tp = (cyg_tick_count_t *)&icmp->icmp6_data8[4];
   
    if (icmp->icmp6_type != ICMP6_ECHO_REPLY) {
        return 0;
    }
   inet_ntop(AF_INET6,(void *)&from->sin6_addr,fromnamebuf,64);	
    if (icmp->icmp6_id != UNIQUEID) {
        diag_printf("%s: ICMP received for wrong id - sent: %x, recvd: %x\n", 
                    fromnamebuf, UNIQUEID, icmp->icmp6_id);
    }
    diag_printf("%d bytes from %s: ", len, fromnamebuf);
    diag_printf("icmp_seq=%d", icmp->icmp6_seq);
    diag_printf(", time=%dms\n", (int)(tv - *tp)*10);
    return (!memcmp(&from->sin6_addr, &to->sin6_addr,sizeof(from->sin6_addr))||IN6_IS_ADDR_MULTICAST(&to->sin6_addr));
}

static void
ping6_host(int s, const struct sockaddr_in6 *dst, int count, char * name, int datasize)
{
    struct icmp6_hdr *icmp = (struct icmp6_hdr *)pkt1;
    int icmp_len = datasize;
    int seq, ok_recv, bogus_recv;
    cyg_tick_count_t *tp;
    long *dp;
    struct sockaddr_in6 from;
    int i, len, fromlen;
    char namebuf[128];
    int error = 0;
    int echo_responce;
    struct sockaddr_in6  dst_scope = *dst;
    int scope = 0,ifd;
    struct ifnet *if_interface ;

   
    ifd = if_nametoindex(name);
   // diag_printf("\n[%s]:ifd = %d  %s",__FUNCTION__,ifd,name);
    if_interface = ifindex2ifnet[ifd];
    scope=in6_addr2zoneid(if_interface,&dst_scope.sin6_addr);
  // diag_printf("\n[%s]  scope = %d",__FUNCTION__,scope);
   dst_scope.sin6_scope_id = scope;
   in6_embedscope(&dst_scope.sin6_addr,&dst_scope);
   
    ok_recv = 0;
    bogus_recv = 0;
#if 0	
    error = getnameinfo((struct sockaddr *)host,sizeof(*host), 
			namebuf, sizeof(namebuf), 
			NULL, 0,
			NI_NUMERICHOST);
    if (error) {
      perror ("getnameinfo");
    } else {
      diag_printf("PING6 server %s\n", namebuf);
    }
#endif
    for (seq = 0;  seq < count;  seq++) {
        // Build ICMP packet
        icmp->icmp6_type = ICMP6_ECHO_REQUEST;
        icmp->icmp6_code = 0;
        icmp->icmp6_cksum = 0;
        icmp->icmp6_seq = seq;
        icmp->icmp6_id = UNIQUEID;

        // Set up ping data
        tp = (cyg_tick_count_t *)&icmp->icmp6_data8[4];
        *tp++ = cyg_current_time();
        dp = (long *)tp;
        for (i = sizeof(*tp);  i < icmp_len;  i += sizeof(*dp)) {
            *dp++ = i;
        }
        // Add checksum
        icmp->icmp6_cksum = inet_cksum( (u_short *)icmp, icmp_len+8);
        // Send it off
        if (sendto(s, icmp, icmp_len+8, 0, (struct sockaddr *)&dst_scope, 
		   sizeof(dst_scope)) < 0) {
            perror("sendto");
            continue;
        }
        // Wait for a response. We get our own ECHO_REQUEST and the responce
	echo_responce = 0;
	while (!echo_responce) {
	  fromlen = sizeof(from);
	  len = recvfrom(s, pkt2, sizeof(pkt2), 0, (struct sockaddr *)&from, 
			 &fromlen);
	  if (len < 0) {
	  	if ((!IN6_IS_ADDR_MULTICAST(&dst->sin6_addr) )||
			(IN6_IS_ADDR_MULTICAST(&dst->sin6_addr) && ok_recv==0 ))
	  		{
	  		bogus_recv++;
	  		diag_printf("recvfrom: timeout\n");
	  		}
	    echo_responce=1;
		
	  } else {
            if (show6_icmp(pkt2, len, &from, dst)) {
	      ok_recv++;
	 if (!IN6_IS_ADDR_MULTICAST(&dst->sin6_addr) )
	      echo_responce=1;
	    }
	  }
        }
    }
    diag_printf("Sent %d packets, received %d OK, %d bad\n", count, 
		ok_recv, bogus_recv);
}


int icmp_ping6(struct in6_addr *dst_v6, int count, char * name, int datasize)
{
	struct timeval tv;
	struct sockaddr_in6 addr;

	int s;

	s = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	if (s < 0) {
		perror("socket");
		return;
	}
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	bzero(&addr, sizeof(addr));
	addr.sin6_len = sizeof(struct sockaddr_in6);
	addr.sin6_family = AF_INET6;
	memcpy(&addr.sin6_addr, dst_v6, sizeof(struct in6_addr));
	ping6_host(s, &addr, count, name, datasize);

	close(s);
	return 0;
}
#endif


