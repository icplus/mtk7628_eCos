//==========================================================================
//
//      tests/ping_test.c
//
//      Simple test of PING (ICMP) and networking support
//
//==========================================================================
// ####BSDALTCOPYRIGHTBEGIN####                                             
// -------------------------------------------                              
// Portions of this software may have been derived from FreeBSD, OpenBSD,   
// or other sources, and if so are covered by the appropriate copyright     
// and license included herein.                                             
// -------------------------------------------                              
// ####BSDALTCOPYRIGHTEND####                                               
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    gthomas
// Contributors: gthomas, andrew.lunn@ascom.ch
// Date:         2000-01-10
// Purpose:      
// Description:  
//              
//
//####DESCRIPTIONEND####
//
//==========================================================================

// PING test code

#include <network.h>
#ifdef CYGPKG_NET_INET6
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#endif

#include <pkgconf/system.h>
#include <pkgconf/net.h>

#include <cyg/infra/testcase.h>

#ifdef CYGBLD_DEVS_ETH_DEVICE_H    // Get the device config if it exists
#include CYGBLD_DEVS_ETH_DEVICE_H  // May provide CYGTST_DEVS_ETH_TEST_NET_REALTIME
#endif

#ifdef CYGPKG_NET_TESTS_USE_RT_TEST_HARNESS // do we use the rt test?
# ifdef CYGTST_DEVS_ETH_TEST_NET_REALTIME // Get the test ancilla if it exists
#  include CYGTST_DEVS_ETH_TEST_NET_REALTIME
# endif
#endif

// Fill in the blanks if necessary
#ifndef TNR_OFF
# define TNR_OFF()
#endif
#ifndef TNR_ON
# define TNR_ON()
#endif
#ifndef TNR_INIT
# define TNR_INIT()
#endif
#ifndef TNR_PRINT_ACTIVITY
# define TNR_PRINT_ACTIVITY()
#endif



#ifndef CYGPKG_LIBC_STDIO
#define perror(s) diag_printf(#s ": %s\n", strerror(errno))
#endif

#define STACK_SIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + 0x1000)
static char stack[STACK_SIZE];
static cyg_thread thread_data;
static cyg_handle_t thread_handle;

#define NUM_PINGS 16
#define MAX_PACKET 4096
#define MIN_PACKET   64
#define MAX_SEND   4000

#define PACKET_ADD  ((MAX_SEND - MIN_PACKET)/NUM_PINGS)
#define nPACKET_ADD  1 

static unsigned char pkt1[MAX_PACKET], pkt2[MAX_PACKET];

#define UNIQUEID 0x1234

void
pexit(char *s)
{
    CYG_TEST_FAIL_FINISH(s);
}

// Compute INET checksum
int
inet_cksum(u_short *addr, int len)
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

static int
show_icmp(unsigned char *pkt, int len, 
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
    if (icmp->icmp_id != UNIQUEID) {
        diag_printf("%s: ICMP received for wrong id - sent: %x, recvd: %x\n", 
                    inet_ntoa(from->sin_addr), UNIQUEID, icmp->icmp_id);
    }
    diag_printf("%d bytes from %s: ", len, inet_ntoa(from->sin_addr));
    diag_printf("icmp_seq=%d", icmp->icmp_seq);
    diag_printf(", time=%dms\n", (int)(tv - *tp)*10);
    return (from->sin_addr.s_addr == to->sin_addr.s_addr);
}

static void
ping_host(int s, struct sockaddr_in *host)
{
    struct icmp *icmp = (struct icmp *)pkt1;
    int icmp_len = MIN_PACKET;
    int seq, ok_recv, bogus_recv;
    cyg_tick_count_t *tp;
    long *dp;
    struct sockaddr_in from;
    int i, len;
    socklen_t fromlen;

    ok_recv = 0;
    bogus_recv = 0;
    diag_printf("PING server %s\n", inet_ntoa(host->sin_addr));
    for (seq = 0;  seq < NUM_PINGS;  seq++, icmp_len += PACKET_ADD ) {
        TNR_ON();
        // Build ICMP packet
        icmp->icmp_type = ICMP_ECHO;
        icmp->icmp_code = 0;
        icmp->icmp_cksum = 0;
        icmp->icmp_seq = seq;
        icmp->icmp_id = 0x1234;
        // Set up ping data
        tp = (cyg_tick_count_t *)&icmp->icmp_data;
        *tp++ = cyg_current_time();
        dp = (long *)tp;
        for (i = sizeof(*tp);  i < icmp_len;  i += sizeof(*dp)) {
            *dp++ = i;
        }
        // Add checksum
        icmp->icmp_cksum = inet_cksum( (u_short *)icmp, icmp_len+8);
        // Send it off
        if (sendto(s, icmp, icmp_len+8, 0, (struct sockaddr *)host, sizeof(*host)) < 0) {
            TNR_OFF();
            perror("sendto");
            continue;
        }
        // Wait for a response
        fromlen = sizeof(from);
        len = recvfrom(s, pkt2, sizeof(pkt2), 0, (struct sockaddr *)&from, &fromlen);
        TNR_OFF();
        if (len < 0) {
            perror("recvfrom");
            icmp_len = MIN_PACKET - PACKET_ADD; // just in case - long routes
        } else {
            if (show_icmp(pkt2, len, &from, host)) {
                ok_recv++;
            } else {
                bogus_recv++;
            }
        }
    }
    TNR_OFF();
    diag_printf("Sent %d packets, received %d OK, %d bad\n", NUM_PINGS, ok_recv, bogus_recv);
}

#ifdef CYGPKG_NET_INET6
static int
show6_icmp(unsigned char *pkt, int len, 
          const struct sockaddr_in6 *from, const struct sockaddr_in6 *to)
{
    cyg_tick_count_t *tp, tv;
    struct icmp6_hdr *icmp;
    char fromnamebuf[128];
    char tonamebuf[128];
    int error;

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

    tv = cyg_current_time();
    icmp = (struct icmp6_hdr *)pkt;
    tp = (cyg_tick_count_t *)&icmp->icmp6_data8[4];
   
    if (icmp->icmp6_type != ICMP6_ECHO_REPLY) {
        return 0;
    }
    if (icmp->icmp6_id != UNIQUEID) {
        diag_printf("%s: ICMP received for wrong id - sent: %x, recvd: %x\n", 
                    fromnamebuf, UNIQUEID, icmp->icmp6_id);
    }
    diag_printf("%d bytes from %s: ", len, fromnamebuf);
    diag_printf("icmp_seq=%d", icmp->icmp6_seq);
    diag_printf(", time=%dms\n", (int)(tv - *tp)*10);
    return (!memcmp(&from->sin6_addr, &to->sin6_addr,sizeof(from->sin6_addr)));
}

static void
ping6_host(int s, const struct sockaddr_in6 *host)
{
    struct icmp6_hdr *icmp = (struct icmp6_hdr *)pkt1;
    int icmp_len = 64;
    int seq, ok_recv, bogus_recv;
    cyg_tick_count_t *tp;
    long *dp;
    struct sockaddr_in6 from;
    int i, len, fromlen;
    char namebuf[128];
    int error;
    int echo_responce;

    ok_recv = 0;
    bogus_recv = 0;
    error = getnameinfo((struct sockaddr *)host,sizeof(*host), 
			namebuf, sizeof(namebuf), 
			NULL, 0,
			NI_NUMERICHOST);
    if (error) {
      perror ("getnameinfo");
    } else {
      diag_printf("PING6 server %s\n", namebuf);
    }
    for (seq = 0;  seq < NUM_PINGS;  seq++) {
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
        if (sendto(s, icmp, icmp_len+8, 0, (struct sockaddr *)host, 
		   sizeof(*host)) < 0) {
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
            perror("recvfrom");
	    echo_responce=1;
	  } else {
            if (show6_icmp(pkt2, len, &from, host)) {
	      ok_recv++;
	      echo_responce=1;
	    }
	  }
        }
    }
    diag_printf("Sent %d packets, received %d OK, %d bad\n", NUM_PINGS, 
		ok_recv, bogus_recv);
}

static void
ping6_test( struct sockaddr_in6 *host)
{
    struct protoent *p;
    struct timeval tv;
    struct sockaddr_in6 addr;
    int s;

    if ((p = getprotobyname("ipv6-icmp")) == (struct protoent *)0) {
        perror("getprotobyname");
        return;
    }
    s = socket(AF_INET6, SOCK_RAW, p->p_proto);
    if (s < 0) {
        perror("socket");
        return;
    }
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    ping6_host(s, host);

    // Now try a bogus host
    memcpy(&addr,host,sizeof(addr));
    addr.sin6_addr.s6_addr[15] = addr.sin6_addr.s6_addr[15] + 32;
    ping6_host(s, &addr);
}
#endif

#ifdef CYGPKG_PROFILE_GPROF
#include <cyg/profile/profile.h>

extern char _stext, _etext;  // Defined by the linker

static void
start_profile(void)
{
    // This starts up the system-wide profiling, gathering
    // profile information on all of the code, with a 16 byte
    // "bucket" size, at a rate of 100us/profile hit.
    // Note: a bucket size of 16 will give pretty good function
    //       resolution.  Much smaller and the buffer becomes
    //       much too large for very little gain.
    // Note: a timer period of 100us is also a reasonable
    //       compromise.  Any smaller and the overhead of 
    //       handling the timter (profile) interrupt could
    //       swamp the system.  A fast processor might get
    //       by with a smaller value, but a slow one could
    //       even be swamped by this value.  If the value is
    //       too large, the usefulness of the profile is reduced.
    profile_on(&_stext, &_etext, 16, 100);
}
#endif

static void
ping_test(struct bootp *bp)
{
    struct protoent *p;
    struct timeval tv;
    struct sockaddr_in host;
    int s;

    if ((p = getprotobyname("icmp")) == (struct protoent *)0) {
        pexit("getprotobyname");
        return;
    }
    s = socket(AF_INET, SOCK_RAW, p->p_proto);
    if (s < 0) {
        pexit("socket");
        return;
    }
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    // Set up host address
    host.sin_family = AF_INET;
    host.sin_len = sizeof(host);
    host.sin_addr = bp->bp_siaddr;
    host.sin_port = 0;
    ping_host(s, &host);
    // Now try a bogus host
    host.sin_addr.s_addr = htonl(ntohl(host.sin_addr.s_addr) + 32);
    ping_host(s, &host);
}

void
net_test(cyg_addrword_t p)
{
#ifdef CYGPKG_NET_INET6
    struct sockaddr_in6 ipv6router;
#endif

    diag_printf("Start PING test\n");
    TNR_INIT();
    init_all_network_interfaces();
#ifdef CYGPKG_PROFILE_GPROF
    start_profile();
#endif
#ifdef CYGHWR_NET_DRIVER_ETH0
    if (eth0_up) {
        ping_test(&eth0_bootp_data);
    }
#endif
#ifdef CYGHWR_NET_DRIVER_ETH1
    if (eth1_up) {
        ping_test(&eth1_bootp_data);
    }
#endif
#ifdef CYGPKG_NET_INET6
    if (cyg_net_get_ipv6_advrouter(&ipv6router)) {
      ping6_test(&ipv6router);
    } else {
      CYG_TEST_FAIL("No router advertisement recieved");
    }
#endif
    TNR_PRINT_ACTIVITY();
    CYG_TEST_PASS_FINISH("Ping test OK");
}

void
cyg_start(void)
{
    // Create a main thread, so we can run the scheduler and have time 'pass'
    cyg_thread_create(10,                // Priority - just a number
                      net_test,          // entry
                      0,                 // entry parameter
                      "Network test",    // Name
                      &stack[0],         // Stack
                      STACK_SIZE,        // Size
                      &thread_handle,    // Handle
                      &thread_data       // Thread data structure
            );
    cyg_thread_resume(thread_handle);  // Start it
    cyg_scheduler_start();
}
