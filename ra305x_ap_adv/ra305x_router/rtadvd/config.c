/*	$KAME: config.c,v 1.11 2000/05/16 13:34:13 itojun Exp $	*/

/*
 * Copyright (C) 1998 WIDE Project.
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
	  
#include <sys/ioctl.h>
#include <sys/sysctl.h>
	  
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_var.h>
#include <net/route.h>
#include <net/if_dl.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#include <netinet/icmp6.h>
		   
#include <arpa/inet.h>
	 
#include <ifaddrs.h>
	 
#include <time.h>
#include <errno.h>
#include <syslog.h>

#include "rtadvd.h"
#include "timer.h"
#include "if.h"

static void makeentry __P((char *, int, char *, int));
static void get_prefix __P((struct rainfo *));

//rtadvd start eth1 maxinterval=10 mininterval=5 chlim=0 raflagsM=false raflagsO=false rltime=0 rtime=0 retrans=0 pinfoflagsL=false pinfoflagsA=false vltime=0 pltime=0 mtu=0 addrs=3ffe:501:ffff:100::
extern struct rainfo *ralist;
int
getconfig(argc, argv)
	int argc;
	char *argv[];
{
	char *intface;	
	int pfxs, i;
	struct rainfo *tmp;
	char name[64];
	char value[256];
	char address[64];
	int addrs = 0;
	struct in6_ndireq ndi;
	int s;
	char * found;
	u_int32_t validlifetime = DEF_ADVVALIDLIFETIME;
	u_int32_t preflifetime = DEF_ADVPREFERREDLIFETIME;
	int have_lifetime = 0;
	int pfx_flag = (ND_OPT_PI_FLAG_ONLINK|ND_OPT_PI_FLAG_AUTO);

	intface = argv[0];
	if (strcmp(intface, "eth1")  && strcmp(intface, "eth0"))//add by ziqiang
		{
		syslog(LOG_WARNING,
			   "<%s>  Unknow interface %s. ",
			   __FUNCTION__, intface);
		return 1;
		}
	argc--;
	*argv++;

	tmp = (struct rainfo *)malloc(sizeof(*ralist));
	if (NULL == tmp)
	{
		diag_printf("\n cann't malloc tmp");
		return 1;
	}
	memset(tmp, 0, sizeof(*tmp));
	memset(address, 0, sizeof(address));
	tmp->advlinkopt = 1;
	tmp->maxinterval = DEF_MAXRTRADVINTERVAL;
	tmp->hoplimit = DEF_ADVCURHOPLIMIT;
	tmp->managedflg  = 0;
	tmp->otherflg = 0;
	tmp->reachabletime = DEF_ADVREACHABLETIME;
	tmp->retranstimer = DEF_ADVRETRANSTIMER;
	while (argc--) {
		found = strstr(*argv, "=" );
		if (found != NULL) {
			found[0] = '\0';
			*found++;
			sprintf(value, "%s", found);
		} else
			memset(value, 0, sizeof(value));

		sprintf(name, "%s", *argv++);

		if (!strcmp(name, "nolladdr"))
			tmp->advlinkopt = 0;		
		else if (!strcmp(name, "maxinterval")) {
			tmp->maxinterval = atoi(value);
		} else if (!strcmp(name, "mininterval"))
			tmp->mininterval = atoi(value);
		else if (!strcmp(name, "chlim"))
			tmp->hoplimit = atoi(value);
		else if (!strcmp(name, "raflagsM")) {
			if (!strcmp(value, "false"))
				tmp->managedflg = 0;
			else
				tmp->managedflg |= ND_RA_FLAG_MANAGED;
		}
		else if (!strcmp(name, "raflagsO")) {
			if (!strcmp(value, "false"))
				tmp->otherflg = 0;
			else
				tmp->otherflg |= ND_RA_FLAG_OTHER;
		}
		else if (!strcmp(name, "rltime")) {
			have_lifetime = 1;
			tmp->lifetime  = atoi(value);
		} else if (!strcmp(name, "rtime"))
			tmp->reachabletime = atoi(value);
		else if (!strcmp(name, "retrans"))
			tmp->retranstimer = strtoul(value, NULL, 10);
		else if (!strcmp(name, "mtu")) {
			if (!strcmp(value, "auto"))
				tmp->linkmtu = tmp->phymtu;
			else
				tmp->linkmtu = atoi(value);
		}
		else if (!strcmp(name, "pinfoflagsL")) {
			if (!strcmp(value, "false"))
				pfx_flag  &= ~(ND_OPT_PI_FLAG_ONLINK);
		}
		else if (!strcmp(name, "pinfoflagsA")) {
			if (!strcmp(value, "false"))
				pfx_flag  &=  ~(ND_OPT_PI_FLAG_AUTO);
		}
		else if (!strcmp(name, "vltime"))
			validlifetime = strtoul(value, NULL, 10);
		else if (!strcmp(name, "pltime"))
			preflifetime = strtoul(value, NULL, 10);
		else if (!strcmp(name, "addrs")) {
			addrs++;
			sprintf(address, "%s", value);			
			if (addrs == 1){
			tmp->ifindex = if_nametoindex(intface);
			strncpy(tmp->ifname, intface, sizeof(tmp->ifname));
			if ((tmp->phymtu = if_getmtu(intface)) == 0) {
				tmp->phymtu = IPV6_MMTU;
				syslog(LOG_WARNING,
					   "<%s> can't get interface mtu of %s. Treat as %d",
					   __FUNCTION__, intface, IPV6_MMTU);
			}
			if (tmp->mininterval == 0)
				tmp->mininterval = tmp->maxinterval/3;
			
			if (have_lifetime == 0)
				tmp->lifetime = tmp->maxinterval*3;

			
				tmp->prefix.next = tmp->prefix.prev = &tmp->prefix;
			}

				struct prefix *pfx;
				//tmp->pfxs = 1;
								
				/* allocate memory to store prefix information */
				if ((pfx = malloc(sizeof(struct prefix))) == NULL) {
						syslog(LOG_ERR,
							   "<%s> can't allocate enough memory",
							   __FUNCTION__);
						if(tmp)
							free(tmp);
						return 1 ;
				}
				memset(pfx, 0, sizeof(*pfx));
								
				/* link into chain */
				insque(pfx, &tmp->prefix);
				tmp->pfxs++;
								
				pfx->origin = PREFIX_FROM_CONFIG;
				pfx->prefixlen = 64;
				pfx->validlifetime = validlifetime;
				pfx->preflifetime = preflifetime;
				pfx->onlinkflg = pfx_flag & ND_OPT_PI_FLAG_ONLINK;
				pfx->autoconfflg = pfx_flag & ND_OPT_PI_FLAG_AUTO;		
				if (inet_pton(AF_INET6, address, &pfx->prefix) != 1) {
					syslog(LOG_ERR,
						   "<%s> inet_pton failed for %s",
						   __FUNCTION__, address);
				}
				if (IN6_IS_ADDR_MULTICAST(&pfx->prefix)) {
					syslog(LOG_ERR,
						   "<%s> multicast prefix(%s) must "
						   "not be advertised (IF=%s)",
							   __FUNCTION__, address, intface);
				}
				if (IN6_IS_ADDR_LINKLOCAL(&pfx->prefix))
					syslog(LOG_NOTICE,
						   "<%s> link-local prefix(%s) will be"
						   " advertised on %s",
						   __FUNCTION__, address, intface);
			

		pfx_flag = (ND_OPT_PI_FLAG_ONLINK|ND_OPT_PI_FLAG_AUTO);

		}
			
		
	}


	if (!addrs) 
	{
	
			tmp->ifindex = if_nametoindex(intface);
			strncpy(tmp->ifname, intface, sizeof(tmp->ifname));
			if ((tmp->phymtu = if_getmtu(intface)) == 0) {
				tmp->phymtu = IPV6_MMTU;
				syslog(LOG_WARNING,
					   "<%s> can't get interface mtu of %s. Treat as %d",
					   __FUNCTION__, intface, IPV6_MMTU);
			}
			if (tmp->mininterval == 0)
				tmp->mininterval = tmp->maxinterval/3;
			
			if (have_lifetime == 0)
				tmp->lifetime = tmp->maxinterval*3;

			
				tmp->prefix.next = tmp->prefix.prev = &tmp->prefix;
			
		/* prefix information */
		/* auto configure prefix information */
		get_prefix(tmp);
	}
		
	/* Config the interface */
	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		syslog(LOG_ERR, "<%s> socket: %s", __FUNCTION__,
			strerror(errno));
	}
	memset(&ndi, 0, sizeof(ndi));
	strncpy(ndi.ifname, intface, sizeof(ndi.ifname));
	if (ioctl(s, SIOCGIFINFO_IN6, (caddr_t)&ndi) < 0)
		syslog(LOG_INFO, "<%s> ioctl:SIOCGIFINFO_IN6 at %s: %s",
			__FUNCTION__, intface, strerror(errno));
	
	/* reflect the RA info to the host variables in kernel */
	ndi.ndi.chlim =  tmp->hoplimit;
	ndi.ndi.retrans = tmp->retranstimer;
	ndi.ndi.basereachable = tmp->reachabletime;
	if (ioctl(s, SIOCSIFINFO_IN6, (caddr_t)&ndi) < 0)
		syslog(LOG_INFO, "<%s> ioctl:SIOCSIFINFO_IN6 at %s: %s",
			__FUNCTION__, intface, strerror(errno));

	close(s);
	/* okey */
	tmp->next = ralist;
	ralist = tmp;

	/* construct the sending packet */
	rtadvd_make_packet(tmp);

	/* set timer */
	tmp->timer = rtadvd_add_timer(ra_timeout, ra_timer_update,
				      tmp, tmp);

	ra_timer_update((void *)tmp, &tmp->timer->tm);
	rtadvd_set_timer(&tmp->timer->tm, tmp->timer);

	return 0;
}

cyg_uint32 hal_lsbit_index1(cyg_uint32 mask)
{
    cyg_uint32 n = mask;

    static const signed char tab[64] =
    { -1, 0, 1, 12, 2, 6, 0, 13, 3, 0, 7, 0, 0, 0, 0, 14, 10,
      4, 0, 0, 8, 0, 0, 25, 0, 0, 0, 0, 0, 21, 27 , 15, 31, 11,
      5, 0, 0, 0, 0, 0, 9, 0, 0, 24, 0, 0 , 20, 26, 30, 0, 0, 0,
      0, 23, 0, 19, 29, 0, 22, 18, 28, 17, 16, 0
    };

    n &= ~(n-1UL);
    n = (n<<16)-n;
    n = (n<<6)+n;
    n = (n<<4)+n;

    return tab[n>>26];
}

#define HAL_LSBIT_INDEX(index, mask) index = hal_lsbit_index1(mask);

static void
get_prefix(struct rainfo *rai)
{
	struct ifaddr *ifa;
	struct ifnet *ifp;
	int addrword = 0;
	int bits = 0, index = 0;
	char name[64], addr[64], netmask[64];
	struct prefix *pp;

	ifp = ifunit(rai->ifname);
	if (ifp == NULL)
		return;

	if_indextoname(ifp->if_index, name, 64);
	TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
		if (ifa->ifa_addr->sa_family == AF_INET6) {
			struct in6_ifaddr * ifa6 = (struct in6_ifaddr *) ifa;
			struct sockaddr_in6 tempip;
			struct sockaddr_in6 *dstip = &tempip;
 			struct sockaddr_in6 *mask6 = (struct sockaddr_in6 *) ifa->ifa_netmask;

			if (ifa6->ia6_flags != 0)
				continue;
			memcpy(dstip, ifa->ifa_addr, sizeof(struct sockaddr_in6));
			if (IN6_IS_ADDR_LINKLOCAL(&dstip->sin6_addr) ||
			    IN6_IS_ADDR_MULTICAST(&dstip->sin6_addr))
				continue;


			dstip->sin6_addr.s6_addr32[0] &= mask6->sin6_addr.s6_addr32[0];
			dstip->sin6_addr.s6_addr32[1] &= mask6->sin6_addr.s6_addr32[1];
			dstip->sin6_addr.s6_addr32[2] &= mask6->sin6_addr.s6_addr32[2];
			dstip->sin6_addr.s6_addr32[3] &= mask6->sin6_addr.s6_addr32[3];

			bits = 0;
			index = 0;
			while (addrword < 4) {
				if (mask6->sin6_addr.s6_addr32[addrword] == 0) {
					break;
				}
				HAL_LSBIT_INDEX(index, mask6->sin6_addr.s6_addr32[addrword++]);
				bits += (32-index);
			}

			/* allocate memory to store prefix info. */
			if ((pp = malloc(sizeof(*pp))) == NULL) {
				syslog(LOG_ERR, "<%s> can't get allocate buffer for prefix", __FUNCTION__);
				return;
			}
			memset(pp, 0, sizeof(*pp));

			/* set prefix and its length */
			memcpy(&pp->prefix, &dstip->sin6_addr, sizeof(struct in6_addr));
			pp->prefixlen = bits;

			/* set other fields with protocol defaults */
			pp->validlifetime = DEF_ADVVALIDLIFETIME;
			pp->preflifetime = DEF_ADVPREFERREDLIFETIME;
			pp->onlinkflg = 1;
			pp->autoconfflg = 1;
			pp->origin = PREFIX_FROM_KERNEL;
				syslog(LOG_DEBUG, "rtadvd Profix= %s,",
					inet6_fmt(&pp->prefix));
	
			/* link into chain */
			insque(pp, &rai->prefix);

			/* counter increment */
			rai->pfxs++;
			//break;
		}

	}	
}

static void
makeentry(buf, id, string, add)
    char *buf, *string;
    int id, add;
{
	strcpy(buf, string);
	if (add) {
		char *cp;

		cp = (char *)index(buf, '\0');
		cp += sprintf(cp, "%d", id);
		*cp = '\0';
	}
}

/*
 * Add a prefix to the list of specified interface and reconstruct
 * the outgoing packet.
 * The prefix must not be in the list.
 * XXX: other parameter of the prefix(e.g. lifetime) shoule be
 * able to be specified.
 */
static void
add_prefix(struct rainfo *rai, struct in6_prefixreq *ipr)
{
	struct prefix *prefix;
	u_char ntopbuf[INET6_ADDRSTRLEN];

	if ((prefix = malloc(sizeof(*prefix))) == NULL) {
		syslog(LOG_ERR, "<%s> memory allocation failed",
		       __FUNCTION__);
		return;		/* XXX: error or exit? */
	}
	prefix->prefix = ipr->ipr_prefix.sin6_addr;
	prefix->prefixlen = ipr->ipr_plen;
	prefix->validlifetime = ipr->ipr_vltime;
	prefix->preflifetime = ipr->ipr_pltime;
	prefix->onlinkflg = ipr->ipr_raf_onlink;
	prefix->autoconfflg = ipr->ipr_raf_auto;
	prefix->origin = PREFIX_FROM_DYNAMIC;

	insque(prefix, &rai->prefix);

	syslog(LOG_DEBUG, "<%s> new prefix %s/%d was added on %s",
	       __FUNCTION__, inet_ntop(AF_INET6, &ipr->ipr_prefix.sin6_addr,
				       ntopbuf, INET6_ADDRSTRLEN),
	       ipr->ipr_plen, rai->ifname);

	/* free the previous packet */
	if (NULL != rai->ra_data)
	{
		free(rai->ra_data);
		rai->ra_data = 0;
	}

	/* reconstruct the packet */
	rai->pfxs++;
	rtadvd_make_packet(rai);

	/*
	 * reset the timer so that the new prefix will be advertised quickly.
	 */
	rai->initcounter = 0;
	ra_timer_update((void *)rai, &rai->timer->tm);
	rtadvd_set_timer(&rai->timer->tm, rai->timer);
}

/*
 * Delete a prefix to the list of specified interface and reconstruct
 * the outgoing packet.
 * The prefix must be in the list.
 */
void
rtadvd_delete_prefix(struct rainfo *rai, struct prefix *prefix)
{
	u_char ntopbuf[INET6_ADDRSTRLEN];

	remque(prefix);
	syslog(LOG_DEBUG, "<%s> prefix %s/%d was deleted on %s",
	       __FUNCTION__, inet_ntop(AF_INET6, &prefix->prefix,
				       ntopbuf, INET6_ADDRSTRLEN),
	       prefix->prefixlen, rai->ifname);
	free(prefix);
	rai->pfxs--;
	rtadvd_make_packet(rai);
}

/*
 * Try to get an in6_prefixreq contents for a prefix which matches
 * ipr->ipr_prefix and ipr->ipr_plen and belongs to
 * the interface whose name is ipr->ipr_name[].
 */
static int
init_prefix(struct in6_prefixreq *ipr)
{
	int s;

	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		syslog(LOG_ERR, "<%s> socket: %s", __FUNCTION__,
		       strerror(errno));
		exit(1);
	}

	if (ioctl(s, SIOCGIFPREFIX_IN6, (caddr_t)ipr) < 0) {
		syslog(LOG_INFO, "<%s> ioctl:SIOCGIFPREFIX %s", __FUNCTION__,
		       strerror(errno));

		ipr->ipr_vltime = DEF_ADVVALIDLIFETIME;
		ipr->ipr_pltime = DEF_ADVPREFERREDLIFETIME;
		ipr->ipr_raf_onlink = 1;
		ipr->ipr_raf_auto = 1;
		/* omit other field initialization */
	}
	else if (ipr->ipr_origin < PR_ORIG_RR) {
		u_char ntopbuf[INET6_ADDRSTRLEN];

		syslog(LOG_WARNING, "<%s> Added prefix(%s)'s origin %d is"
		       "lower than PR_ORIG_RR(router renumbering)."
		       "This should not happen if I am router", __FUNCTION__,
		       inet_ntop(AF_INET6, &ipr->ipr_prefix.sin6_addr, ntopbuf,
				 sizeof(ntopbuf)), ipr->ipr_origin);
		close(s);
		return 1;
	}

	close(s);
	return 0;
}

void
rtadvd_make_prefix(struct rainfo *rai, int ifindex, struct in6_addr *addr, int plen)
{
	struct in6_prefixreq ipr;

	memset(&ipr, 0, sizeof(ipr));
	if (if_indextoname(ifindex, ipr.ipr_name) == NULL) {
		syslog(LOG_ERR, "<%s> Prefix added interface No.%d doesn't"
		       "exist. This should not happen! %s", __FUNCTION__,
		       ifindex, strerror(errno));
		exit(1);
	}
	ipr.ipr_prefix.sin6_len = sizeof(ipr.ipr_prefix);
	ipr.ipr_prefix.sin6_family = AF_INET6;
	ipr.ipr_prefix.sin6_addr = *addr;
	ipr.ipr_plen = plen;

	if (init_prefix(&ipr))
		return; /* init failed by some error */
	add_prefix(rai, &ipr);
}

#define ROUNDUP8(a) (1 + (((a) - 1) | 7))

void
rtadvd_make_packet(struct rainfo *rainfo)
{
	size_t packlen, lladdroptlen = 0;
	char *buf;
	struct nd_router_advert *ra;
	struct nd_opt_prefix_info *ndopt_pi;
	struct nd_opt_mtu *ndopt_mtu;
#ifdef MIP6
	struct nd_opt_advint *ndopt_advint;
	struct nd_opt_hai *ndopt_hai;
#endif
	struct prefix *pfx;

	/* calculate total length */
	packlen = sizeof(struct nd_router_advert);
	if (rainfo->advlinkopt) {
//		if ((lladdroptlen = lladdropt_length(rainfo->sdl)) == 0) {		
		lladdroptlen = (ROUNDUP8(ETHER_ADDR_LEN + 2));
		if (lladdroptlen  == 0) {
			
			syslog(LOG_INFO,
			       "<%s> link-layer address option has"
			       " null length on %s."
			       " Treat as not included.",
			       __FUNCTION__, rainfo->ifname);
			rainfo->advlinkopt = 0;
		}
		packlen += lladdroptlen;
	}
	if (rainfo->pfxs)
		packlen += sizeof(struct nd_opt_prefix_info) * rainfo->pfxs;
	if (rainfo->linkmtu)
		packlen += sizeof(struct nd_opt_mtu);
#ifdef MIP6
	if (mobileip6 && rainfo->maxinterval)
		packlen += sizeof(struct nd_opt_advint);
	if (mobileip6 && rainfo->hatime)
		packlen += sizeof(struct nd_opt_hai);
#endif

	/* allocate memory for the packet */
	if ((buf = malloc(packlen)) == NULL) {
		syslog(LOG_ERR,
		       "<%s> can't get enough memory for an RA packet",
		       __FUNCTION__);
		exit(1);
	}
	if (NULL  != rainfo->ra_data)
	{
		free(rainfo->ra_data); //free early buf
		rainfo->ra_data =NULL;
	}
	rainfo->ra_data = buf;
	/* XXX: what if packlen > 576? */
	rainfo->ra_datalen = packlen;

	/*
	 * construct the packet
	 */
	ra = (struct nd_router_advert *)buf;
	ra->nd_ra_type = ND_ROUTER_ADVERT;
	ra->nd_ra_code = 0;
	ra->nd_ra_cksum = 0;
	ra->nd_ra_curhoplimit = (u_int8_t)(0xff & rainfo->hoplimit);
	ra->nd_ra_flags_reserved = 0;
	ra->nd_ra_flags_reserved |=
		rainfo->managedflg ? ND_RA_FLAG_MANAGED : 0;
	ra->nd_ra_flags_reserved |=
		rainfo->otherflg ? ND_RA_FLAG_OTHER : 0;
#ifdef MIP6
	ra->nd_ra_flags_reserved |=
		rainfo->haflg ? ND_RA_FLAG_HA : 0;
#endif
	ra->nd_ra_router_lifetime = htons(rainfo->lifetime);
	ra->nd_ra_reachable = htonl(rainfo->reachabletime);
	ra->nd_ra_retransmit = htonl(rainfo->retranstimer);
	buf += sizeof(*ra);

	if (rainfo->advlinkopt) {
#if 1
{
	struct nd_opt_hdr *ndopt = (struct nd_opt_hdr *)buf;
	char addr[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	if (!strcmp(rainfo->ifname, "eth0")) {	
		CFG_get_mac(0, addr);
	} else if (!strcmp(rainfo->ifname, "eth1")) {	
		CFG_get_mac(1, addr);
	}
	 ndopt->nd_opt_type = ND_OPT_SOURCE_LINKADDR; /* fixed */
	 ndopt->nd_opt_len = (ROUNDUP8(ETHER_ADDR_LEN + 2)) >> 3;
	 memcpy((char *)(ndopt + 1), addr, ETHER_ADDR_LEN);
}

#else
		lladdropt_fill(rainfo->sdl, (struct nd_opt_hdr *)buf);
#endif
		buf += lladdroptlen;
	}

	if (rainfo->linkmtu) {
		ndopt_mtu = (struct nd_opt_mtu *)buf;
		ndopt_mtu->nd_opt_mtu_type = ND_OPT_MTU;
		ndopt_mtu->nd_opt_mtu_len = 1;
		ndopt_mtu->nd_opt_mtu_reserved = 0;
		ndopt_mtu->nd_opt_mtu_mtu = ntohl(rainfo->linkmtu);
		buf += sizeof(struct nd_opt_mtu);
	}

#ifdef MIP6
	if (mobileip6 && rainfo->maxinterval) {
		ndopt_advint = (struct nd_opt_advint *)buf;
		ndopt_advint->nd_opt_int_type = ND_OPT_ADV_INTERVAL;
		ndopt_advint->nd_opt_int_len = 1;
		ndopt_advint->nd_opt_int_reserved = 0;
		ndopt_advint->nd_opt_int_interval = ntohl(rainfo->maxinterval *
							  1000);
		buf += sizeof(struct nd_opt_advint);
	}
#endif
	
#ifdef MIP6
	if (rainfo->hatime) {
		ndopt_hai = (struct nd_opt_hai *)buf;
		ndopt_hai->nd_opt_hai_type = ND_OPT_HA_INFORMATION;
		ndopt_hai->nd_opt_hai_len = 1;
		ndopt_hai->nd_opt_hai_reserved = 0;
		ndopt_hai->nd_opt_hai_pref = ntohs(rainfo->hapref);
		ndopt_hai->nd_opt_hai_lifetime = ntohs(rainfo->hatime);
		buf += sizeof(struct nd_opt_hai);
	}
#endif
	
	for (pfx = rainfo->prefix.next;
	    ( pfx != &rainfo->prefix) && (pfx != NULL); pfx = pfx->next) {
		ndopt_pi = (struct nd_opt_prefix_info *)buf;
		ndopt_pi->nd_opt_pi_type = ND_OPT_PREFIX_INFORMATION;
		ndopt_pi->nd_opt_pi_len = 4;
		ndopt_pi->nd_opt_pi_prefix_len = pfx->prefixlen;
		ndopt_pi->nd_opt_pi_flags_reserved = 0;
		if (pfx->onlinkflg)
			ndopt_pi->nd_opt_pi_flags_reserved |=
				ND_OPT_PI_FLAG_ONLINK;
		if (pfx->autoconfflg)
			ndopt_pi->nd_opt_pi_flags_reserved |=
				ND_OPT_PI_FLAG_AUTO;
#ifdef MIP6
		if (pfx->routeraddr)
			ndopt_pi->nd_opt_pi_flags_reserved |=
				ND_OPT_PI_FLAG_RTADDR;
#endif
		ndopt_pi->nd_opt_pi_valid_time = ntohl(pfx->validlifetime);
		ndopt_pi->nd_opt_pi_preferred_time =
			ntohl(pfx->preflifetime);
		ndopt_pi->nd_opt_pi_reserved2 = 0;
		ndopt_pi->nd_opt_pi_prefix = pfx->prefix;

		buf += sizeof(struct nd_opt_prefix_info);
	}

	return;
}
