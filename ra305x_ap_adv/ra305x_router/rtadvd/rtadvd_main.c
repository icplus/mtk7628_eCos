/*	$KAME: rtadvd.c,v 1.30 2000/06/22 20:16:12 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */
typedef unsigned int size_t;

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/bsdtypes.h>

#include <net/if.h>
#include <net/route.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet/icmp6.h>

#include <arpa/inet.h>

#include <time.h>
#include <errno.h>
#include <syslog.h>
#include "rtadvd.h"
#include "rrenum.h"
#include "timer.h"
#include "if.h"
#include "config.h"

struct msghdr rcvmhdr;
static u_char *rcvcmsgbuf = NULL;
static size_t rcvcmsgbuflen;
static u_char *sndcmsgbuf = NULL;
static size_t sndcmsgbuflen;
static int rtadvd_running = 0;
static int rtadvd_doing_shutdown = 0;
static cyg_handle_t rtadvd_thread_h;
static cyg_thread rtadvd_thread;
static char rtadvd_stack[4096];
static u_char answer[1500];
struct msghdr sndmhdr;
struct iovec rcviov[2];
struct iovec sndiov[2];
struct sockaddr_in6 from;
struct sockaddr_in6 sin6_allnodes = {sizeof(sin6_allnodes), AF_INET6};
int sock, rtsock;
#ifdef MIP6
int mobileip6 = 0;
#endif
int accept_rr = 0;
int dflag = 0, sflag = 0;

struct rainfo *ralist = NULL;
struct nd_optlist {
	struct nd_optlist *next;
	struct nd_opt_hdr *opt;
};
union nd_opts {
	struct nd_opt_hdr *nd_opt_array[7];
	struct {
		struct nd_opt_hdr *zero;
		struct nd_opt_hdr *src_lladdr;
		struct nd_opt_hdr *tgt_lladdr;
		struct nd_opt_prefix_info *pi;
		struct nd_opt_rd_hdr *rh;
		struct nd_opt_mtu *mtu;
		struct nd_optlist *list;
	} nd_opt_each;
};
#define nd_opts_src_lladdr	nd_opt_each.src_lladdr
#define nd_opts_tgt_lladdr	nd_opt_each.tgt_lladdr
#define nd_opts_pi		nd_opt_each.pi
#define nd_opts_rh		nd_opt_each.rh
#define nd_opts_mtu		nd_opt_each.mtu
#define nd_opts_list		nd_opt_each.list

#define NDOPT_FLAG_SRCLINKADDR 0x1
#define NDOPT_FLAG_TGTLINKADDR 0x2
#define NDOPT_FLAG_PREFIXINFO 0x4
#define NDOPT_FLAG_RDHDR 0x8
#define NDOPT_FLAG_MTU 0x10

u_int32_t ndopt_flags[] = {
	0, NDOPT_FLAG_SRCLINKADDR, NDOPT_FLAG_TGTLINKADDR,
	NDOPT_FLAG_PREFIXINFO, NDOPT_FLAG_RDHDR, NDOPT_FLAG_MTU
};

static int sock_open __P((void));
static void rtsock_open __P((void));
static void rtadvd_input __P((void));
static void rs_input __P((int, struct nd_router_solicit *,
			  struct in6_pktinfo *, struct sockaddr_in6 *));
static void ra_input __P((int, struct nd_router_advert *,
			  struct in6_pktinfo *, struct sockaddr_in6 *));
static int prefix_check __P((struct nd_opt_prefix_info *, struct rainfo *,
			     struct sockaddr_in6 *));
static int rtadvd_nd6_options __P((struct nd_opt_hdr *, int,
			    union nd_opts *, u_int32_t));
static void free_ndopts __P((union nd_opts *));
static struct rainfo *if_indextorainfo __P((int));
static void ra_output __P((struct rainfo *));
static void rtmsg_input __P((void));
struct prefix *find_prefix __P((struct rainfo *, struct in6_addr *, int));


void rtadvd_daemon(cyg_addrword_t data)
{
	int i;
	fd_set fdset;
	int maxfd = 0;
	struct timeval waittime;
	struct timeval *timeout;
	struct ipv6_mreq mreq;
	struct rainfo *ra = ralist;

	FD_ZERO(&fdset);
	FD_SET(sock, &fdset);
	maxfd = sock;
	rtsock_open();
	FD_SET(rtsock, &fdset);
	if (rtsock > sock)
			maxfd = rtsock;

	while (rtadvd_running) {
		struct fd_set select_fd = fdset; /* reinitialize */

		/* timer expiration check and reset the timer */
		timeout = rtadvd_check_timer();

		waittime.tv_sec = 5;
		waittime.tv_usec = 0;
		if (timeout && timeout->tv_sec < 5) {
			waittime.tv_sec = timeout->tv_sec;
			waittime.tv_usec = timeout->tv_usec;
		}

#if 0	
		if (timeout != NULL) {			
			syslog(LOG_DEBUG,
				   "<%s> set timer to %ld:%ld. waiting for "
				   "inputs or timeout",
				   __FUNCTION__,
				   (long int)timeout->tv_sec,
				   (long int)timeout->tv_usec);
		} else {
			syslog(LOG_DEBUG,
				   "<%s> there's no timer. waiting for inputs",
				   __FUNCTION__);
		}
#endif

		if ((i = select(maxfd + 1, &select_fd,
				NULL, NULL, waittime)) < 0) {
			/* EINTR would occur upon SIGUSR1 for status dump */
			if (errno != EINTR)
				syslog(LOG_ERR, "<%s> select: %s",
					   __FUNCTION__, strerror(errno));
			continue;
		}

		if (i == 0) /* timeout */
			continue;
		if (sflag == 0 && FD_ISSET(rtsock, &select_fd))
			rtmsg_input();
		if (FD_ISSET(sock, &select_fd))
			rtadvd_input();
	}

	/*
	 * Leave all routers multicast address on each advertising interface.
	 */
	if (inet_pton(AF_INET6, ALLROUTERS, &mreq.ipv6mr_multiaddr.s6_addr) != 1) {
		syslog(LOG_ERR, "<%s> inet_pton failed(library bug?)",
			   __FUNCTION__);
	}

	ra = ralist;
	while(ra) {
		mreq.ipv6mr_interface = ra->ifindex;
		if (setsockopt(sock, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
				   &mreq,
				   sizeof(mreq)) < 0) {
			syslog(LOG_ERR, "<%s> IPV6_LEAVE_GROUP on %s: %s, %d",
				   __FUNCTION__, ra->ifname, strerror(errno), errno);
		}
		if (ra->timer)
			rtadvd_remove_timer(&ra->timer);
		ra = ra->next;
	}
	
	if (sock)
		close(sock);
	
	if (rtsock)
		close(rtsock);
	
	if (rcvcmsgbuf)
		{

		free(rcvcmsgbuf);
		rcvcmsgbuf = NULL;
		}

	if (sndcmsgbuf)
		{

		free(sndcmsgbuf);
		sndcmsgbuf = NULL;
		}

	ra = ralist;
	while(ra) {
		struct rainfo *temp = ra;
		ra = ra->next;
		free(temp->ra_data);
		free(temp);
	}
	ralist = NULL;

	rtadvd_doing_shutdown = 0;
	return 0;
}


void
rtadvd_stop()
{
	struct rainfo *ra;
	int i;
	const int retrans = MAX_FINAL_RTR_ADVERTISEMENTS;

	if (rtadvd_running == 0)
		return;

	diag_printf("rtadvd stop\n");
	rtadvd_doing_shutdown = 1;

	for (ra = ralist; ra; ra = ra->next) {
		ra->lifetime = 0;
		rtadvd_make_packet(ra);
	}
	for (i = 0; i < retrans; i++) {
		for (ra = ralist; ra; ra = ra->next) {
			ra_output(ra);
		}
		cyg_thread_delay(MIN_DELAY_BETWEEN_RAS);
	}

	rtadvd_running = 0; 

	while (rtadvd_doing_shutdown != 0)
		cyg_thread_delay(5);
	return;
}


int
rtadvd_start(argc, argv)
	int argc;
	char *argv[];
{
	struct timeval *timeout;
	int i;



	/* skip start /stop */
	argc--;
	*argv++;
	if (argc < 1)
		return 1;

	diag_printf("%s...\n", __FUNCTION__);
	if (!rtadvd_running)
	{

	/* timer initialization */
	rtadvd_timer_init();

	/* random value initialization */
#define srandom(s) srand(s)
	srandom((u_long)time(NULL));
		}
	if (getconfig(argc, argv) != 0) {
		diag_printf("Get config failed\n");
		return 1;
	}		
	if (rtadvd_running)
		return 1;

	if (inet_pton(AF_INET6, ALLNODES, &sin6_allnodes.sin6_addr) != 1) {
		diag_printf("fatal: inet_pton failed\n");
		return 1;
	}
	if (sock_open() != 0) {
		diag_printf("Socket open failed\n");
		return 1;
	}		

	rtadvd_running = 1;
	cyg_thread_create(7, (cyg_thread_entry_t *)rtadvd_daemon, (cyg_addrword_t) NULL, "rtadvd_thread",
				  (void *)&rtadvd_stack[0], sizeof(rtadvd_stack), &rtadvd_thread_h, &rtadvd_thread);
	cyg_thread_resume(rtadvd_thread_h);
	return 0;
}

static void
rtmsg_input()
{
	int n, type, ifindex = 0, plen;
	size_t len;
	char msg[2048], *next, *lim;
	u_char ifname[16];
	struct prefix *prefix;
	struct rainfo *rai;
	struct in6_addr *addr;
	char addrbuf[INET6_ADDRSTRLEN];

	n = read(rtsock, msg, 2048);
	if (dflag > 1) {
		syslog(LOG_DEBUG,
		       "<%s> received a routing message "
		       "(type = %d, len = %d)",
		       __FUNCTION__,
		       rtmsg_type(msg), n);
	}
	if (n > rtmsg_len(msg) && n < sizeof(msg)/sizeof(char)) {
		/*
		 * This usually won't happen for messages received on 
		 * a routing socket.
		 */
		if (dflag > 1)
			syslog(LOG_DEBUG,
			       "<%s> received data length is larger than"
			       "1st routing message len. multiple messages?"
			       " read %d bytes, but 1st msg len = %d",
			       __FUNCTION__, n, rtmsg_len(msg));
#if 0
		/* adjust length */
		n = rtmsg_len(msg);
#endif
		return;
	}

	lim = msg + n;
	for (next = msg; next < lim; next += len) {
		int oldifflags;

		next = get_next_msg(next, lim, 0, &len,
				    RTADV_TYPE2BITMASK(RTM_ADD) |
				    RTADV_TYPE2BITMASK(RTM_DELETE) |
				    RTADV_TYPE2BITMASK(RTM_NEWADDR) |
				    RTADV_TYPE2BITMASK(RTM_DELADDR) |
				    RTADV_TYPE2BITMASK(RTM_IFINFO));
		if (len == 0)
			break;
		type = rtmsg_type(next);
		switch (type) {
		case RTM_ADD:
		case RTM_DELETE:
			ifindex = get_rtm_ifindex(next);
			break;
		case RTM_NEWADDR:
		case RTM_DELADDR:
			ifindex = get_ifam_ifindex(next);
			break;
		case RTM_IFINFO:
			ifindex = get_ifm_ifindex(next);
			break;
		default:
			/* should not reach here */
			if (dflag > 1) {
				syslog(LOG_DEBUG,
				       "<%s:%d> unknown rtmsg %d on %s",
				       __FUNCTION__, __LINE__, type,
				       if_indextoname(ifindex, ifname));
			}
			continue;
		}

		if ((rai = if_indextorainfo(ifindex)) == NULL) {
			if (dflag > 1) {
				syslog(LOG_DEBUG,
				       "<%s> route changed on "
				       "non advertising interface(%s)",
				       __FUNCTION__,
				       if_indextoname(ifindex, ifname));
			}
			continue;
		}

#if 1 //Eddy add
{
	struct ifnet *ifp;
	ifp = ifunit(rai->ifname);
	if (ifp == NULL)
		return;
	
	if ((ifp->if_flags & IFF_UP) == 0) {
		if (rai->timer)
			rtadvd_remove_timer(&rai->timer);
	} else if ((ifp->if_flags & IFF_UP) != 0) {
		if (rai->timer == NULL) {	
			rai->initcounter = 0; /* reset the counter */
			rai->waiting = 0; /* XXX */
			rai->timer = rtadvd_add_timer(ra_timeout,
							  ra_timer_update,
							  rai, rai);
			ra_timer_update((void *)rai, &rai->timer->tm);
			rtadvd_set_timer(&rai->timer->tm, rai->timer);
		}
	}

	return; //Eddy Need TODO
}
#endif
		oldifflags = iflist[ifindex]->ifm_flags;

		switch(type) {
		 case RTM_ADD:
			 /* init ifflags because it may have changed */
			 iflist[ifindex]->ifm_flags =
			 	if_getflags(ifindex,
					    iflist[ifindex]->ifm_flags);

			 if (sflag)
				 break;	/* we aren't interested in prefixes  */

			 addr = get_addr(msg);
			 plen = get_prefixlen(msg);
			 /* sanity check for plen */
			 if (plen < 4 /* as RFC2373, prefixlen is at least 4 */
			     || plen > 127) {
				syslog(LOG_INFO, "<%s> new interface route's"
				       "plen %d is invalid for a prefix",
				       __FUNCTION__, plen);
				break;
			 }
			 prefix = find_prefix(rai, addr, plen);
			 if (prefix) {
				 if (dflag > 1) {
					 syslog(LOG_DEBUG,
						"<%s> new prefix(%s/%d) "
						"added on %s, "
						"but it was already in list",
						__FUNCTION__,
						inet_ntop(AF_INET6,
							  addr, (char *)addrbuf,
							  INET6_ADDRSTRLEN),
						plen,
						rai->ifname);
				 }
				 break;
			 }
			 rtadvd_make_prefix(rai, ifindex, addr, plen);
			 break;
		 case RTM_DELETE:
			 /* init ifflags because it may have changed */
			 iflist[ifindex]->ifm_flags =
			 	if_getflags(ifindex,
					    iflist[ifindex]->ifm_flags);

			 if (sflag)
				 break;

			 addr = get_addr(msg);
			 plen = get_prefixlen(msg);
			 /* sanity check for plen */
			 if (plen < 4 /* as RFC2373, prefixlen is at least 4 */
			     || plen > 127) {
				syslog(LOG_INFO, "<%s> deleted interface"
				       "route's"
				       "plen %d is invalid for a prefix",
				       __FUNCTION__, plen);
				break;
			 }
			 prefix = find_prefix(rai, addr, plen);
			 if (prefix == NULL) {
				 if (dflag > 1) {
					 syslog(LOG_DEBUG,
						"<%s> prefix(%s/%d) was "
						"deleted on %s, "
						"but it was not in list",
						__FUNCTION__,
						inet_ntop(AF_INET6,
							  addr, (char *)addrbuf,
							  INET6_ADDRSTRLEN),
						plen,
						rai->ifname);
				 }
				 break;
			 }
			 rtadvd_delete_prefix(rai, prefix);
			 break;
		case RTM_NEWADDR:
		case RTM_DELADDR:
			 /* init ifflags because it may have changed */
			 iflist[ifindex]->ifm_flags =
			 	if_getflags(ifindex,
					    iflist[ifindex]->ifm_flags);
			 break;
		case RTM_IFINFO:
			 iflist[ifindex]->ifm_flags = get_ifm_flags(next);
			 break;
		default:
			/* should not reach here */
			if (dflag > 1) {
				syslog(LOG_DEBUG,
				       "<%s:%d> unknown rtmsg %d on %s",
				       __FUNCTION__, __LINE__, type,
				       if_indextoname(ifindex, ifname));
			}
			return;
		}

		/* check if an interface flag is changed */
		if ((oldifflags & IFF_UP) != 0 &&	/* UP to DOWN */
		    (iflist[ifindex]->ifm_flags & IFF_UP) == 0) {
			syslog(LOG_INFO,
			       "<%s> interface %s becomes down. stop timer.",
			       __FUNCTION__, rai->ifname);
			rtadvd_remove_timer(&rai->timer);
		}
		else if ((oldifflags & IFF_UP) == 0 &&	/* DOWN to UP */
			 (iflist[ifindex]->ifm_flags & IFF_UP) != 0) {
			syslog(LOG_INFO,
			       "<%s> interface %s becomes up. restart timer.",
			       __FUNCTION__, rai->ifname);

			rai->initcounter = 0; /* reset the counter */
			rai->waiting = 0; /* XXX */
			rai->timer = rtadvd_add_timer(ra_timeout,
						      ra_timer_update,
						      rai, rai);
			ra_timer_update((void *)rai, &rai->timer->tm);
			rtadvd_set_timer(&rai->timer->tm, rai->timer);
		}
	}

	return;
}

void
rtadvd_input()
{
	int i;
	int *hlimp = NULL;
	struct icmp6_hdr *icp;
	int ifindex = 0;
	struct cmsghdr *cm;
	struct in6_pktinfo *pi = NULL;
	u_char ntopbuf[INET6_ADDRSTRLEN], ifnamebuf[IFNAMSIZ];
	struct in6_addr dst = in6addr_any;
	struct rainfo *ra = NULL;

	/*
	 * Get message. We reset msg_controllen since the field could
	 * be modified if we had received a message before setting
	 * receive options.
	 */	 
	rcviov[0].iov_base = (caddr_t)answer;
	rcviov[0].iov_len = sizeof(answer);	
	rcvmhdr.msg_controllen = rcvcmsgbuflen;
	if ((i = recvmsg(sock, &rcvmhdr, 0)) < 0)
		return;

	/* extract optional information via Advanced API */
	for (cm = (struct cmsghdr *)CMSG_FIRSTHDR(&rcvmhdr);
	     cm;
	     cm = (struct cmsghdr *)CMSG_NXTHDR(&rcvmhdr, cm)) {
		if (cm->cmsg_level == IPPROTO_IPV6 &&
		    cm->cmsg_type == IPV6_PKTINFO &&
		    cm->cmsg_len == CMSG_LEN(sizeof(struct in6_pktinfo))) {
			pi = (struct in6_pktinfo *)(CMSG_DATA(cm));
			ifindex = pi->ipi6_ifindex;
			dst = pi->ipi6_addr;			
		}
		if (cm->cmsg_level == IPPROTO_IPV6 &&
		    cm->cmsg_type == IPV6_HOPLIMIT &&
		    cm->cmsg_len == CMSG_LEN(sizeof(int)))
			hlimp = (int *)CMSG_DATA(cm);
	}
	if (ifindex == 0) {
		syslog(LOG_ERR,
		       "<%s> failed to get receiving interface",
		       __FUNCTION__);
		return;
	}
	if (hlimp == NULL) {
		syslog(LOG_ERR,
		       "<%s> failed to get receiving hop limit",
		       __FUNCTION__);
		return;
	}

	{
		if (!ralist)
			return;
		struct ifnet *ifp;
	ra = ralist;
	while (ra != NULL) {
		if (pi->ipi6_ifindex == ra->ifindex)
			break;
		ra = ra->next;
	}
	if (ra == NULL)
		return ;
		ifp = ifunit(ra ->ifname);
		if (ifp == NULL)
			return;

		if (ifp && ((ifp->if_flags & IFF_UP) == 0))
			return;
	}

#if 0
	/*
	 * If we happen to receive data on an interface which is now down,
	 * just discard the data.
	 */
	if ((iflist[pi->ipi6_ifindex]->ifm_flags & IFF_UP) == 0) {
		syslog(LOG_INFO,
		       "<%s> received data on a disabled interface (%s)",
		       __FUNCTION__,
		       if_indextoname(pi->ipi6_ifindex, ifnamebuf));
		return;
	}
#endif

	if (i < sizeof(struct icmp6_hdr)) {
		syslog(LOG_ERR,
		       "<%s> packet size(%d) is too short",
		       __FUNCTION__, i);
		return;
	}

	icp = (struct icmp6_hdr *) answer;
	switch(icp->icmp6_type) {
	 case ND_ROUTER_SOLICIT:
		 /*
		  * Message verification - RFC-2461 6.1.1
		  * XXX: these checks must be done in the kernel as well,
		  *      but we can't completely rely on them.
		  */
		 if (*hlimp != 255) {
			 syslog(LOG_NOTICE,
				"<%s> RS with invalid hop limit(%d) "
				"received from %s on %s",
				__FUNCTION__, *hlimp,
				inet_ntop(AF_INET6, &from.sin6_addr, ntopbuf,
					  INET6_ADDRSTRLEN),
				if_indextoname(pi->ipi6_ifindex, ifnamebuf));
			 return;
		 }
		 if (icp->icmp6_code) {
			 syslog(LOG_NOTICE,
				"<%s> RS with invalid ICMP6 code(%d) "
				"received from %s on %s",
				__FUNCTION__, icp->icmp6_code,
				inet_ntop(AF_INET6, &from.sin6_addr, ntopbuf,
					  INET6_ADDRSTRLEN),
				if_indextoname(pi->ipi6_ifindex, ifnamebuf));
			 return;
		 }
		 if (i < sizeof(struct nd_router_solicit)) {
			 syslog(LOG_NOTICE,
				"<%s> RS from %s on %s does not have enough "
				"length (len = %d)",
				__FUNCTION__,
				inet_ntop(AF_INET6, &from.sin6_addr, ntopbuf,
					  INET6_ADDRSTRLEN),
				if_indextoname(pi->ipi6_ifindex, ifnamebuf), i);
			 return;
		 }
		 rs_input(i, (struct nd_router_solicit *)icp, pi, &from);
		 break;
	 case ND_ROUTER_ADVERT:
		 /*
		  * Message verification - RFC-2461 6.1.2
		  * XXX: there's a same dilemma as above... 
		  */
		 if (*hlimp != 255) {
			 syslog(LOG_NOTICE,
				"<%s> RA with invalid hop limit(%d) "
				"received from %s on %s",
				__FUNCTION__, *hlimp,
				inet_ntop(AF_INET6, &from.sin6_addr, ntopbuf,
					  INET6_ADDRSTRLEN),
				if_indextoname(pi->ipi6_ifindex, ifnamebuf));
			 return;
		 }
		 if (icp->icmp6_code) {
			 syslog(LOG_NOTICE,
				"<%s> RA with invalid ICMP6 code(%d) "
				"received from %s on %s",
				__FUNCTION__, icp->icmp6_code,
				inet_ntop(AF_INET6, &from.sin6_addr, ntopbuf,
					  INET6_ADDRSTRLEN),
				if_indextoname(pi->ipi6_ifindex, ifnamebuf));
			 return;
		 }
		 if (i < sizeof(struct nd_router_advert)) {
			 syslog(LOG_NOTICE,
				"<%s> RA from %s on %s does not have enough "
				"length (len = %d)",
				__FUNCTION__,
				inet_ntop(AF_INET6, &from.sin6_addr, ntopbuf,
					  INET6_ADDRSTRLEN),
				if_indextoname(pi->ipi6_ifindex, ifnamebuf), i);
			 return;
		 }
		 ra_input(i, (struct nd_router_advert *)icp, pi, &from);
		 break;
	 case ICMP6_ROUTER_RENUMBERING:
		 if (accept_rr == 0) {
			 syslog(LOG_ERR,
				"<%s> received a router renumbering "
				"message, but not allowed to be accepted",
				__FUNCTION__);
			 break;
		 }
		 rr_input(i, (struct icmp6_router_renum *)icp, pi, &from,
			  &dst);
		 break;
	 default:
		 /*
		  * Note that this case is POSSIBLE, especially just
		  * after invocation of the daemon. This is because we
		  * could receive message after opening the socket and
		  * before setting ICMP6 type filter(see sock_open()).
		  */
		 syslog(LOG_ERR,
			"<%s> invalid icmp type(%d)",
			__FUNCTION__, icp->icmp6_type);
		 return;
	}

	return;
}

static void
rs_input(int len, struct nd_router_solicit *rs,
	 struct in6_pktinfo *pi, struct sockaddr_in6 *from)
{
	u_char ntopbuf[INET6_ADDRSTRLEN], ifnamebuf[IFNAMSIZ];
	union nd_opts ndopts;
	struct rainfo *ra;

	syslog(LOG_DEBUG,
	       "<%s> RS received from %s on %s",
	       __FUNCTION__,
	       inet_ntop(AF_INET6, &from->sin6_addr,
			 ntopbuf, INET6_ADDRSTRLEN),
	       if_indextoname(pi->ipi6_ifindex, ifnamebuf));

	/* ND option check */
	memset(&ndopts, 0, sizeof(ndopts));
	if (rtadvd_nd6_options((struct nd_opt_hdr *)(rs + 1),
			len - sizeof(struct nd_router_solicit),
			 &ndopts, NDOPT_FLAG_SRCLINKADDR)) {
		syslog(LOG_DEBUG,
		       "<%s> ND option check failed for an RS from %s on %s",
		       __FUNCTION__,
		       inet_ntop(AF_INET6, &from->sin6_addr,
				 ntopbuf, INET6_ADDRSTRLEN),
		       if_indextoname(pi->ipi6_ifindex, ifnamebuf));
		return;
	}

	/*
	 * If the IP source address is the unspecified address, there
	 * must be no source link-layer address option in the message.
	 * (RFC-2461 6.1.1)
	 */
	if (IN6_IS_ADDR_UNSPECIFIED(&from->sin6_addr) &&
	    ndopts.nd_opts_src_lladdr) {
		syslog(LOG_ERR,
		       "<%s> RS from unspecified src on %s has a link-layer"
		       " address option",
		       __FUNCTION__,
		       if_indextoname(pi->ipi6_ifindex, ifnamebuf));
		goto done;
	}

	ra = ralist;
	while (ra != NULL) {
		if (pi->ipi6_ifindex == ra->ifindex)
			break;
		ra = ra->next;
	}
	if (ra == NULL) {
		syslog(LOG_INFO,
		       "<%s> RS received on non advertising interface(%s)",
		       __FUNCTION__,
		       if_indextoname(pi->ipi6_ifindex, ifnamebuf));
		goto done;
	}

	ra->rsinput++;		/* increment statistics */

	/*
	 * Decide whether to send RA according to the rate-limit
	 * consideration.
	 */
	{
		long delay;	/* must not be greater than 1000000 */
		struct timeval interval, now, min_delay, tm_tmp, *rest;
		struct soliciter *sol;

		/*
		 * record sockaddr waiting for RA, if possible
		 */
		sol = (struct soliciter *)malloc(sizeof(*sol));
		if (sol) {
			sol->addr = *from;
			/*XXX RFC2553 need clarification on flowinfo */
			sol->addr.sin6_flowinfo = 0;	
			sol->next = ra->soliciter;
			ra->soliciter = sol;
		}

		/*
		 * If there is already a waiting RS packet, don't
		 * update the timer.
		 */
		if (ra->waiting++)
			goto done;

		/*
		 * Compute a random delay. If the computed value
		 * corresponds to a time later than the time the next
		 * multicast RA is scheduled to be sent, ignore the random
		 * delay and send the advertisement at the
		 * already-scheduled time. RFC-2461 6.2.6
		 */
		delay = random() % MAX_RA_DELAY_TIME;
		interval.tv_sec = 0;
		interval.tv_usec = delay;
		rest = rtadvd_timer_rest(ra->timer);
		if (TIMEVAL_LT(*rest, interval)) {
			syslog(LOG_DEBUG,
			       "<%s> random delay is larger than "
			       "the rest of normal timer",
			       __FUNCTION__);
			interval = *rest;
		}

		/*
		 * If we sent a multicast Router Advertisement within
		 * the last MIN_DELAY_BETWEEN_RAS seconds, schedule
		 * the advertisement to be sent at a time corresponding to
		 * MIN_DELAY_BETWEEN_RAS plus the random value after the
		 * previous advertisement was sent.
		 */
		gettimeofday(&now, NULL);
		TIMEVAL_SUB(&now, &ra->lastsent, &tm_tmp);
		min_delay.tv_sec = MIN_DELAY_BETWEEN_RAS;
		min_delay.tv_usec = 0;
		if (TIMEVAL_LT(tm_tmp, min_delay)) {
			TIMEVAL_SUB(&min_delay, &tm_tmp, &min_delay);
			TIMEVAL_ADD(&min_delay, &interval, &interval);
			if ( interval.tv_sec<MIN_DELAY_BETWEEN_RAS)//nd  item 132 may fail,for the time in ecos is not precise
			  interval.tv_sec +=MIN_DELAY_BETWEEN_RAS;
		}

		rtadvd_set_timer(&interval, ra->timer);
		goto done;
	}

  done:
	free_ndopts(&ndopts);
	return;
}

static void
ra_input(int len, struct nd_router_advert *ra,
	 struct in6_pktinfo *pi, struct sockaddr_in6 *from)
{
	struct rainfo *rai;
	u_char ntopbuf[INET6_ADDRSTRLEN], ifnamebuf[IFNAMSIZ];
	union nd_opts ndopts;
	char *on_off[] = {"OFF", "ON"};
	u_int32_t reachabletime, retranstimer, mtu;
	int inconsistent = 0;

#if 0
	syslog(LOG_DEBUG,
	       "<%s> RA received from %s on %s",
	       __FUNCTION__,
	       inet_ntop(AF_INET6, &from->sin6_addr,
			 ntopbuf, INET6_ADDRSTRLEN),
	       if_indextoname(pi->ipi6_ifindex, ifnamebuf));
#endif
	
	/* ND option check */
	memset(&ndopts, 0, sizeof(ndopts));
	if (rtadvd_nd6_options((struct nd_opt_hdr *)(ra + 1),
			len - sizeof(struct nd_router_advert),
			 &ndopts,
			NDOPT_FLAG_PREFIXINFO | NDOPT_FLAG_MTU)) { 
		syslog(LOG_ERR,
		       "<%s> ND option check failed for an RA from %s on %s",
		       __FUNCTION__,
		       inet_ntop(AF_INET6, &from->sin6_addr,
				 ntopbuf, INET6_ADDRSTRLEN),
		       if_indextoname(pi->ipi6_ifindex, ifnamebuf));
		return;
	}


	/*
	 * RA consistency check according to RFC-2461 6.2.7
	 */
	if ((rai = if_indextorainfo(pi->ipi6_ifindex)) == 0) {
		syslog(LOG_INFO,
		       "<%s> received RA from %s on non-advertising"
		       " interface(%s)",
		       __FUNCTION__,
		       inet_ntop(AF_INET6, &from->sin6_addr,
				 ntopbuf, INET6_ADDRSTRLEN),
		       if_indextoname(pi->ipi6_ifindex, ifnamebuf));
		goto done;
	}
	rai->rainput++;		/* increment statistics */
	
	/* Cur Hop Limit value */
	if (ra->nd_ra_curhoplimit && rai->hoplimit &&
	    ra->nd_ra_curhoplimit != rai->hoplimit) {
		syslog(LOG_WARNING,
		       "<%s> CurHopLimit inconsistent on %s:"
		       " %d from %s, %d from us",
		       __FUNCTION__,
		       rai->ifname,
		       ra->nd_ra_curhoplimit,
		       inet_ntop(AF_INET6, &from->sin6_addr,
				 ntopbuf, INET6_ADDRSTRLEN),
		       rai->hoplimit);
		inconsistent++;
	}
	/* M flag */
	if ((ra->nd_ra_flags_reserved & ND_RA_FLAG_MANAGED) !=
	    rai->managedflg) {
		syslog(LOG_WARNING,
		       "<%s> M flag inconsistent on %s:"
		       " %s from %s, %s from us",
		       __FUNCTION__,
		       rai->ifname,
		       on_off[!rai->managedflg],
		       inet_ntop(AF_INET6, &from->sin6_addr,
				 ntopbuf, INET6_ADDRSTRLEN),
		       on_off[rai->managedflg]);
		inconsistent++;
	}
	/* O flag */
	if ((ra->nd_ra_flags_reserved & ND_RA_FLAG_OTHER) !=
	    rai->otherflg) {
		syslog(LOG_WARNING,
		       "<%s> O flag inconsistent on %s:"
		       " %s from %s, %s from us",
		       __FUNCTION__,
		       rai->ifname,
		       on_off[!rai->otherflg],
		       inet_ntop(AF_INET6, &from->sin6_addr,
				 ntopbuf, INET6_ADDRSTRLEN),
		       on_off[rai->otherflg]);
		inconsistent++;
	}
	/* Reachable Time */
	reachabletime = ntohl(ra->nd_ra_reachable);
	if (reachabletime && rai->reachabletime &&
	    reachabletime != rai->reachabletime) {
		syslog(LOG_WARNING,
		       "<%s> ReachableTime inconsistent on %s:"
		       " %d from %s, %d from us",
		       __FUNCTION__,
		       rai->ifname,
		       reachabletime,
		       inet_ntop(AF_INET6, &from->sin6_addr,
				 ntopbuf, INET6_ADDRSTRLEN),
		       rai->reachabletime);
		inconsistent++;
	}
	/* Retrans Timer */
	retranstimer = ntohl(ra->nd_ra_retransmit);
	if (retranstimer && rai->retranstimer &&
	    retranstimer != rai->retranstimer) {
		syslog(LOG_WARNING,
		       "<%s> RetranceTimer inconsistent on %s:"
		       " %d from %s, %d from us",
		       __FUNCTION__,
		       rai->ifname,
		       retranstimer,
		       inet_ntop(AF_INET6, &from->sin6_addr,
				 ntopbuf, INET6_ADDRSTRLEN),
		       rai->retranstimer);
		inconsistent++;
	}
	/* Values in the MTU options */
	if (ndopts.nd_opts_mtu) {
		mtu = ntohl(ndopts.nd_opts_mtu->nd_opt_mtu_mtu);
		if (mtu && rai->linkmtu && mtu != rai->linkmtu) {
			syslog(LOG_WARNING,
			       "<%s> MTU option value inconsistent on %s:"
			       " %d from %s, %d from us",
			       __FUNCTION__,
			       rai->ifname, mtu,
			       inet_ntop(AF_INET6, &from->sin6_addr,
					 ntopbuf, INET6_ADDRSTRLEN),
			       rai->linkmtu);
			inconsistent++;
		}
	}
	/* Preferred and Valid Lifetimes for prefixes */
	{
		struct nd_optlist *optp = ndopts.nd_opts_list;
		
		if (ndopts.nd_opts_pi) {
			if (prefix_check(ndopts.nd_opts_pi, rai, from))
				inconsistent++;
		}
		while (optp) {
			if (prefix_check((struct nd_opt_prefix_info *)optp->opt,
					 rai, from))
				inconsistent++;
			optp = optp->next;
		}
	}

	if (inconsistent) {
		printf("RA input %d inconsistents\n", inconsistent);
		rai->rainconsistent++;
	}
	
  done:
	free_ndopts(&ndopts);
	return;
}

/* return a non-zero value if the received prefix is inconsitent with ours */
static int
prefix_check(struct nd_opt_prefix_info *pinfo,
	     struct rainfo *rai, struct sockaddr_in6 *from)
{
	u_int32_t preferred_time, valid_time;
	struct prefix *pp;
	int inconsistent = 0;
	u_char ntopbuf[INET6_ADDRSTRLEN], prefixbuf[INET6_ADDRSTRLEN];

	/*
	 * log if the adveritsed prefix has link-local scope(sanity check?)
	 */
	if (IN6_IS_ADDR_LINKLOCAL(&pinfo->nd_opt_pi_prefix)) {
		syslog(LOG_INFO,
		       "<%s> link-local prefix %s/%d is advertised "
		       "from %s on %s",
		       __FUNCTION__,
		       inet_ntop(AF_INET6, &pinfo->nd_opt_pi_prefix,
				 prefixbuf, INET6_ADDRSTRLEN),
		       pinfo->nd_opt_pi_prefix_len,
		       inet_ntop(AF_INET6, &from->sin6_addr,
				 ntopbuf, INET6_ADDRSTRLEN),
		       rai->ifname);
	}

	if ((pp = find_prefix(rai, &pinfo->nd_opt_pi_prefix,
			      pinfo->nd_opt_pi_prefix_len)) == NULL) {
		syslog(LOG_INFO,
		       "<%s> prefix %s/%d from %s on %s is not in our list",
		       __FUNCTION__,
		       inet_ntop(AF_INET6, &pinfo->nd_opt_pi_prefix,
				 prefixbuf, INET6_ADDRSTRLEN),
		       pinfo->nd_opt_pi_prefix_len,
		       inet_ntop(AF_INET6, &from->sin6_addr,
				 ntopbuf, INET6_ADDRSTRLEN),
		       rai->ifname);
		return(0);
	}

	preferred_time = ntohl(pinfo->nd_opt_pi_preferred_time);
	if (preferred_time != pp->preflifetime) {
		syslog(LOG_WARNING,
		       "<%s> prefeerred lifetime for %s/%d"
		       " inconsistent on %s:"
		       " %d from %s, %d from us",
		       __FUNCTION__,
		       inet_ntop(AF_INET6, &pinfo->nd_opt_pi_prefix,
				 prefixbuf, INET6_ADDRSTRLEN),
		       pinfo->nd_opt_pi_prefix_len,
		       rai->ifname, preferred_time,
		       inet_ntop(AF_INET6, &from->sin6_addr,
				 ntopbuf, INET6_ADDRSTRLEN),
		       pp->preflifetime);
		inconsistent++;
	}

	valid_time = ntohl(pinfo->nd_opt_pi_valid_time);
	if (valid_time != pp->validlifetime) {
		syslog(LOG_WARNING,
		       "<%s> valid lifetime for %s/%d"
		       " inconsistent on %s:"
		       " %d from %s, %d from us",
		       __FUNCTION__,
		       inet_ntop(AF_INET6, &pinfo->nd_opt_pi_prefix,
				 prefixbuf, INET6_ADDRSTRLEN),
		       pinfo->nd_opt_pi_prefix_len,
		       rai->ifname, valid_time,
		       inet_ntop(AF_INET6, &from->sin6_addr,
				 ntopbuf, INET6_ADDRSTRLEN),
		       pp->validlifetime);
		inconsistent++;
	}

	return(inconsistent);
}

struct prefix *
find_prefix(struct rainfo *rai, struct in6_addr *prefix, int plen)
{
	struct prefix *pp;
	int bytelen, bitlen;

	for (pp = rai->prefix.next; pp != &rai->prefix; pp = pp->next) {
		if (plen != pp->prefixlen)
			continue;
		bytelen = plen / 8;
		bitlen = plen % 8;
		if (memcmp((void *)prefix, (void *)&pp->prefix, bytelen))
			continue;
		if (prefix->s6_addr[bytelen] >> (8 - bitlen) ==
		    pp->prefix.s6_addr[bytelen] >> (8 - bitlen))
			return(pp);
	}

	return(NULL);
}

static int
rtadvd_nd6_options(struct nd_opt_hdr *hdr, int limit,
	    union nd_opts *ndopts, u_int32_t optflags)
{
	int optlen = 0;

	for (; limit > 0; limit -= optlen) {
		hdr = (struct nd_opt_hdr *)((caddr_t)hdr + optlen);
		optlen = hdr->nd_opt_len << 3;
		if (hdr->nd_opt_len == 0) {
			syslog(LOG_ERR,
			       "<%s> bad ND option length(0) (type = %d)",
			       __FUNCTION__, hdr->nd_opt_type);
			goto bad;
		}

		if (hdr->nd_opt_type > ND_OPT_MTU) {
			syslog(LOG_INFO,
			       "<%s> unknown ND option(type %d)",
			       __FUNCTION__,
			       hdr->nd_opt_type);
			continue;
		}

		if ((ndopt_flags[hdr->nd_opt_type] & optflags) == 0) {
#if 0
			syslog(LOG_INFO,
			       "<%s> unexpected ND option(type %d)",
			       __FUNCTION__,
			       hdr->nd_opt_type);
#endif
			continue;
		}

		switch(hdr->nd_opt_type) {
		 case ND_OPT_SOURCE_LINKADDR:
		 case ND_OPT_TARGET_LINKADDR:
		 case ND_OPT_REDIRECTED_HEADER:
		 case ND_OPT_MTU:
			 if (ndopts->nd_opt_array[hdr->nd_opt_type]) {
				 syslog(LOG_INFO,
					"<%s> duplicated ND option"
					" (type = %d)",
					__FUNCTION__,
					hdr->nd_opt_type);
			 }
			 ndopts->nd_opt_array[hdr->nd_opt_type] = hdr;
			 break;
		 case ND_OPT_PREFIX_INFORMATION:
		 {
			 struct nd_optlist *pfxlist;

			 if (ndopts->nd_opts_pi == 0) {
				 ndopts->nd_opts_pi =
					 (struct nd_opt_prefix_info *)hdr;
				 continue;
			 }
			 if ((pfxlist = malloc(sizeof(*pfxlist))) == NULL) {
				 syslog(LOG_ERR,
					"<%s> can't allocate memory",
					__FUNCTION__);
				 goto bad;
			 }
			 pfxlist->next = ndopts->nd_opts_list;
			 pfxlist->opt = hdr;
			 ndopts->nd_opts_list = pfxlist;

			 break;
		 }
		 default:	/* impossible */
			 break;
		}
	}

	return(0);

  bad:
	free_ndopts(ndopts);

	return(-1);
}

static void
free_ndopts(union nd_opts *ndopts)
{
	struct nd_optlist *opt = ndopts->nd_opts_list, *next;

	while(opt) {
		next = opt->next;
		free(opt);
		opt = next;
	}
}

int
sock_open()
{
	struct icmp6_filter filt;
	struct ipv6_mreq mreq;
	struct rainfo *ra = ralist;
	int on;
	/* XXX: should be max MTU attached to the node */

	rcvcmsgbuflen = CMSG_SPACE(sizeof(struct in6_pktinfo)) +
				CMSG_SPACE(sizeof(int));
	rcvcmsgbuf = (u_char *)malloc(rcvcmsgbuflen);
	if (rcvcmsgbuf == NULL) {
		syslog(LOG_ERR, "<%s> not enough core", __FUNCTION__);
		goto error;
	}

	sndcmsgbuflen = CMSG_SPACE(sizeof(struct in6_pktinfo)) + 
				CMSG_SPACE(sizeof(int));
	sndcmsgbuf = (u_char *)malloc(sndcmsgbuflen);
	if (sndcmsgbuf == NULL) {
		syslog(LOG_ERR, "<%s> not enough core", __FUNCTION__);
		goto error;
	}

	if ((sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) < 0) {
		syslog(LOG_ERR, "<%s> socket: %s", __FUNCTION__,
		       strerror(errno));
		goto error;
	}

	/* specify to tell receiving interface */
	on = 1;
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on,
		       sizeof(on)) < 0) {
		syslog(LOG_ERR, "<%s> IPV6_RECVPKTINFO: %s",
		       __FUNCTION__, strerror(errno));
		goto error;
	}

	on = 1;
	/* specify to tell value of hoplimit field of received IP6 hdr */
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on,
		       sizeof(on)) < 0) {
		syslog(LOG_ERR, "<%s> IPV6_RECVHOPLIMIT: %s",
		       __FUNCTION__, strerror(errno));
		goto error;
	}

	ICMP6_FILTER_SETBLOCKALL(&filt);
	ICMP6_FILTER_SETPASS(ND_ROUTER_SOLICIT, &filt);
	ICMP6_FILTER_SETPASS(ND_ROUTER_ADVERT, &filt);
	if (accept_rr)
		ICMP6_FILTER_SETPASS(ICMP6_ROUTER_RENUMBERING, &filt);
	if (setsockopt(sock, IPPROTO_ICMPV6, ICMP6_FILTER, &filt,
		       sizeof(filt)) < 0) {
		syslog(LOG_ERR, "<%s> IICMP6_FILTER: %s",
		       __FUNCTION__, strerror(errno));
		goto error;
	}

	/*
	 * join all routers multicast address on each advertising interface.
	 */
	if (inet_pton(AF_INET6, ALLROUTERS, &mreq.ipv6mr_multiaddr.s6_addr)
	    != 1) {
		syslog(LOG_ERR, "<%s> inet_pton failed(library bug?)",
		       __FUNCTION__);
		goto error;
	}
	while(ra) {
		mreq.ipv6mr_interface = ra->ifindex;
		if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP,
			       &mreq,
			       sizeof(mreq)) < 0) {
			syslog(LOG_ERR, "<%s> IPV6_JOIN_GROUP on %s: %s, %d",
			       __FUNCTION__, ra->ifname, strerror(errno), errno);
			goto error;
		}
		if (ra->ifindex == if_nametoindex("eth0"))//by ziqiang
			mreq.ipv6mr_interface =  if_nametoindex("eth1");
		if (ra->ifindex == if_nametoindex("eth1"))
			mreq.ipv6mr_interface = if_nametoindex("eth0");	
		if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP,
			       &mreq,
			       sizeof(mreq)) < 0) {
			syslog(LOG_ERR, "<%s> IPV6_JOIN_GROUP on %s: %s, %d",
			       __FUNCTION__, ra->ifname, strerror(errno), errno);
			goto error;
		}
		ra = ra->next;
	}

	
	/* initialize msghdr for receiving packets */
	rcviov[0].iov_base = (caddr_t)answer;
	rcviov[0].iov_len = sizeof(answer);
	rcvmhdr.msg_name = (caddr_t)&from;
	rcvmhdr.msg_namelen = sizeof(from);
	rcvmhdr.msg_iov = rcviov;
	rcvmhdr.msg_iovlen = 1;
	rcvmhdr.msg_control = (caddr_t) rcvcmsgbuf;
	rcvmhdr.msg_controllen = rcvcmsgbuflen;

	/* initialize msghdr for sending packets */
	sndmhdr.msg_namelen = sizeof(struct sockaddr_in6);
	sndmhdr.msg_iov = sndiov;
	sndmhdr.msg_iovlen = 1;
	sndmhdr.msg_control = (caddr_t)sndcmsgbuf;
	sndmhdr.msg_controllen = sndcmsgbuflen;
	
	return 0;
error:
	if (rcvcmsgbuf)
		{
		free(rcvcmsgbuf);
		rcvcmsgbuf =NULL;
		}
	if (sndcmsgbuf)
		{
		free(sndcmsgbuf);
		sndcmsgbuf=NULL;
		}
	if (sock)
		close(sock);
	return 1;
}

/* open a routing socket to watch the routing table */
static void
rtsock_open()
{
	if ((rtsock = socket(PF_ROUTE, SOCK_RAW, 0)) < 0) {
		syslog(LOG_ERR,
		       "<%s> socket: %s", __FUNCTION__, strerror(errno));
		exit(1);
	}
}

static struct rainfo *
if_indextorainfo(int index)
{
	struct rainfo *rai = ralist;

	for (rai = ralist; rai; rai = rai->next) {
		if (rai->ifindex == index)
			return(rai);
	}

	return(NULL);		/* search failed */
}

static void
ra_output(rainfo)
struct rainfo *rainfo;
{
	int i;
	struct cmsghdr *cm;
	struct in6_pktinfo *pi;
	struct soliciter *sol, *nextsol;

	{
		struct ifnet *ifp;
		ifp = ifunit(rainfo->ifname);
		if (ifp == NULL)
			return;

		if (ifp && ((ifp->if_flags & IFF_UP) == 0))
			return;
	}

#if 0
	if ((iflist[rainfo->ifindex]->ifm_flags & IFF_UP) == 0) {
		syslog(LOG_DEBUG, "<%s> %s is not up, skip sending RA",
		       __FUNCTION__, rainfo->ifname);
		return;
	}
#endif

	sndmhdr.msg_name = (caddr_t)&sin6_allnodes;
	sndmhdr.msg_iov[0].iov_base = (caddr_t)rainfo->ra_data;
	sndmhdr.msg_iov[0].iov_len = rainfo->ra_datalen;

	cm = CMSG_FIRSTHDR(&sndmhdr);
	if(!cm)panic("ra_output cm=NULL\n");
	/* specify the outgoing interface */
	cm->cmsg_level = IPPROTO_IPV6;
	cm->cmsg_type = IPV6_PKTINFO;
	cm->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));
	pi = (struct in6_pktinfo *)CMSG_DATA(cm);
	memset(&pi->ipi6_addr, 0, sizeof(pi->ipi6_addr));	/*XXX*/
	pi->ipi6_ifindex = rainfo->ifindex;

	/* specify the hop limit of the packet */
	{
		int hoplimit = 255;

		cm = CMSG_NXTHDR(&sndmhdr, cm);
		if(!cm)panic("ra_output cm=NULL\n");
		cm->cmsg_level = IPPROTO_IPV6;
		cm->cmsg_type = IPV6_HOPLIMIT;
		cm->cmsg_len = CMSG_LEN(sizeof(int));
		memcpy(CMSG_DATA(cm), &hoplimit, sizeof(int));
	}

	syslog(LOG_DEBUG,
	       "<%s> send RA on %s, # of waitings = %d",
	       __FUNCTION__, rainfo->ifname, rainfo->waiting); 

	i = sendmsg(sock, &sndmhdr, 0);

	if (i < 0 || i != rainfo->ra_datalen)  {
		if (i < 0) {
			syslog(LOG_ERR, "<%s> sendmsg on %s: %s",
			       __FUNCTION__, rainfo->ifname,
			       strerror(errno));
		}
	}

	/*
	 * unicast advertisements
	 * XXX commented out.  reason: though spec does not forbit it, unicast
	 * advert does not really help
	 */
	for (sol = rainfo->soliciter; sol; sol = nextsol) {
		nextsol = sol->next;
		sndmhdr.msg_name = (caddr_t)&sol->addr;
#if 0
		i = sendmsg(sock, &sndmhdr, 0);
		if (i < 0 || i != rainfo->ra_datalen)  {
			if (i < 0) {
				syslog(LOG_ERR,
				    "<%s> unicast sendmsg on %s: %s",
				    __FUNCTION__, rainfo->ifname,
				    strerror(errno));
			}
		}
#endif		
		sol->next = NULL;
		free(sol);
	}
	rainfo->soliciter = NULL;

	/* update counter */
	if (rainfo->initcounter < MAX_INITIAL_RTR_ADVERTISEMENTS)
		rainfo->initcounter++;
	rainfo->raoutput++;

	/* update timestamp */
	gettimeofday(&rainfo->lastsent, NULL);

	/* reset waiting conter */
	rainfo->waiting = 0;
}

/* process RA timer */
void
ra_timeout(void *data)
{
	struct rainfo *rai = (struct rainfo *)data;

#ifdef notyet
	/* if necessary, reconstruct the packet. */
#endif

	syslog(LOG_DEBUG,
	       "<%s> RA timer on %s is expired",
	       __FUNCTION__, rai->ifname);

	ra_output(rai);
}

/* update RA timer */
void
ra_timer_update(void *data, struct timeval *tm)
{
	struct rainfo *rai = (struct rainfo *)data;
	long interval;

	/*
	 * Whenever a multicast advertisement is sent from an interface,
	 * the timer is reset to a uniformly-distributed random value
	 * between the interface's configured MinRtrAdvInterval and
	 * MaxRtrAdvInterval(discovery-v2-02 6.2.4).
	 */
	interval = rai->mininterval; 
	interval += random() % (rai->maxinterval - rai->mininterval);

	/*
	 * For the first few advertisements (up to
	 * MAX_INITIAL_RTR_ADVERTISEMENTS), if the randomly chosen interval
	 * is greater than MAX_INITIAL_RTR_ADVERT_INTERVAL, the timer
	 * SHOULD be set to MAX_INITIAL_RTR_ADVERT_INTERVAL instead.
	 * (RFC-2461 6.2.4)
	 */
	if (rai->initcounter < MAX_INITIAL_RTR_ADVERTISEMENTS &&
	    interval > MAX_INITIAL_RTR_ADVERT_INTERVAL)
		interval = MAX_INITIAL_RTR_ADVERT_INTERVAL;

	tm->tv_sec = interval;
	tm->tv_usec = 0;

	syslog(LOG_DEBUG,
	       "<%s> RA timer on %s is set to %ld:%ld",
	       __FUNCTION__, rai->ifname,
	       (long int)tm->tv_sec, (long int)tm->tv_usec);

	return;
}