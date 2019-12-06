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
 */

/*
 *  Copyright (c) 1998 by the University of Southern California.
 *  All rights reserved.
 *
 *  Permission to use, copy, modify, and distribute this software and
 *  its documentation in source and binary forms for lawful
 *  purposes and without fee is hereby granted, provided
 *  that the above copyright notice appear in all copies and that both
 *  the copyright notice and this permission notice appear in supporting
 *  documentation, and that any documentation, advertising materials,
 *  and other materials related to such distribution and use acknowledge
 *  that the software was developed by the University of Southern
 *  California and/or Information Sciences Institute.
 *  The name of the University of Southern California may not
 *  be used to endorse or promote products derived from this software
 *  without specific prior written permission.
 *
 *  THE UNIVERSITY OF SOUTHERN CALIFORNIA DOES NOT MAKE ANY REPRESENTATIONS
 *  ABOUT THE SUITABILITY OF THIS SOFTWARE FOR ANY PURPOSE.  THIS SOFTWARE IS
 *  PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, TITLE, AND 
 *  NON-INFRINGEMENT.
 *
 *  IN NO EVENT SHALL USC, OR ANY OTHER CONTRIBUTOR BE LIABLE FOR ANY
 *  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES, WHETHER IN CONTRACT,
 *  TORT, OR OTHER FORM OF ACTION, ARISING OUT OF OR IN CONNECTION WITH,
 *  THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *  Other copyrights might apply to parts of this software and are so
 *  noted when applicable.
 */
/*
 *  Questions concerning this software should be directed to 
 *  Pavlin Ivanov Radoslavov (pavlin@catarina.usc.edu)
 *
 *  $Id: mld6.c,v 1.14 2000/10/05 22:20:38 itojun Exp $
 */
/*
 * Part of this program has been derived from mrouted.
 * The mrouted program is covered by the license in the accompanying file
 * named "LICENSE.mrouted".
 *
 * The mrouted program is COPYRIGHT 1989 by The Board of Trustees of
 * Leland Stanford Junior University.
 *
 * $FreeBSD$
 */

#include "defs.h"
#include <sys/uio.h>

/*
 * Exported variables.
 */

char *mld6_recv_buf;		/* input packet buffer */
char *mld6_send_buf;		/* output packet buffer */
int mld6_socket;		/* socket for all network I/O */
struct sockaddr_in6 allrouters_group = {sizeof(struct sockaddr_in6), AF_INET6};
struct sockaddr_in6 allnodes_group = {sizeof(struct sockaddr_in6), AF_INET6};

/* Extenals */

#define IN6ADDR_LINKLOCAL_ALLROUTERS_INIT \
	{{{ 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 }}}

struct in6_addr in6addr_linklocal_allnodes_pim = IN6ADDR_LINKLOCAL_ALLROUTERS_INIT;;

/* local variables. */
static struct sockaddr_in6 	dst = {sizeof(dst), AF_INET6};
static struct msghdr 		sndmh,rcvmh;
static struct iovec 		sndiov[2];
static struct iovec 		rcviov[2];
static struct sockaddr_in6 	from;
static u_char   		*rcvcmsgbuf = NULL;
static int			rcvcmsglen;

#ifndef USE_RFC2292BIS
u_int8_t raopt[IP6OPT_RTALERT_LEN];
#endif 
static char *sndcmsgbuf;
static int ctlbuflen = 0;
static u_short rtalert_code;

/* local functions */

static void mld6_read __P((int i, fd_set * fds));
static void accept_mld6 __P((int len));
static void make_mld6_msg __P((int, int, struct sockaddr_in6 *,
	struct sockaddr_in6 *, struct in6_addr *, int, int, int, int));

#ifndef IP6OPT_ROUTER_ALERT	/* XXX to be compatible older systems */
#define IP6OPT_ROUTER_ALERT IP6OPT_RTALERT
#endif

/*
 * This function returns the number of bytes required to hold an option
 * when it is stored as ancillary data, including the cmsghdr structure
 * at the beginning, and any padding at the end (to make its size a
 * multiple of 8 bytes).  The argument is the size of the structure
 * defining the option, which must include any pad bytes at the
 * beginning (the value y in the alignment term "xn + y"), the type
 * byte, the length byte, and the option data.
 */
int
inet6_option_space(int nbytes)
{
	nbytes += 2;	/* we need space for nxt-hdr and length fields */
	return(CMSG_SPACE((nbytes + 7) & ~7));
}

/*
 * This function is called once per ancillary data object that will
 * contain either Hop-by-Hop or Destination options.  It returns 0 on
 * success or -1 on an error.
 */
int
inet6_option_init(void *bp, struct cmsghdr **cmsgp, int type)
{
	struct cmsghdr *ch = (struct cmsghdr *)bp;

	/* argument validation */
	if (type != IPV6_HOPOPTS && type != IPV6_DSTOPTS)
		return(-1);
	
	ch->cmsg_level = IPPROTO_IPV6;
	ch->cmsg_type = type;
	ch->cmsg_len = CMSG_LEN(0);

	*cmsgp = ch;
	return(0);
}

/*
 * This function appends a Hop-by-Hop option or a Destination option
 * into an ancillary data object that has been initialized by
 * inet6_option_init().  This function returns 0 if it succeeds or -1 on
 * an error.
 * multx is the value x in the alignment term "xn + y" described
 * earlier.  It must have a value of 1, 2, 4, or 8.
 * plusy is the value y in the alignment term "xn + y" described
 * earlier.  It must have a value between 0 and 7, inclusive.
 */
int
inet6_option_append(struct cmsghdr *cmsg, const u_int8_t *typep, int multx,
    int plusy)
{
	int padlen, optlen, off;
	u_char *bp = (u_char *)cmsg + cmsg->cmsg_len;
	struct ip6_ext *eh = (struct ip6_ext *)CMSG_DATA(cmsg);

	/* argument validation */
	if (multx != 1 && multx != 2 && multx != 4 && multx != 8)
		return(-1);
	if (plusy < 0 || plusy > 7)
		return(-1);

	/*
	 * If this is the first option, allocate space for the
	 * first 2 bytes(for next header and length fields) of
	 * the option header.
	 */
	if (bp == (u_char *)eh) {
		bp += 2;
		cmsg->cmsg_len += 2;
	}

	/* calculate pad length before the option. */
	off = bp - (u_char *)eh;
	padlen = (((off % multx) + (multx - 1)) & ~(multx - 1)) -
		(off % multx);
	padlen += plusy;
	padlen %= multx;	/* keep the pad as short as possible */
	/* insert padding */
	inet6_insert_padopt(bp, padlen);
	cmsg->cmsg_len += padlen;
	bp += padlen;

	/* copy the option */
	if (typep[0] == IP6OPT_PAD1)
		optlen = 1;
	else
		optlen = typep[1] + 2;
	memcpy(bp, typep, optlen);
	bp += optlen;
	cmsg->cmsg_len += optlen;

	/* calculate pad length after the option and insert the padding */
	off = bp - (u_char *)eh;
	padlen = ((off + 7) & ~7) - off;
	inet6_insert_padopt(bp, padlen);
	bp += padlen;
	cmsg->cmsg_len += padlen;

	/* update the length field of the ip6 option header */
	eh->ip6e_len = ((bp - (u_char *)eh) >> 3) - 1;

	return(0);
}

void
inet6_insert_padopt(u_char *p, int len)
{
	switch(len) {
	 case 0:
		 return;
	 case 1:
		 p[0] = IP6OPT_PAD1;
		 return;
	 default:
		 p[0] = IP6OPT_PADN;
		 p[1] = len - 2; 
		 memset(&p[2], 0, len - 2);
		 return;
	}
}

char *
packet_kind(proto, type, code)
    u_int proto, type, code;
{
    static char unknown[60];

    switch (proto) {
     case IPPROTO_ICMPV6:
	switch (type) {
	 case MLD6_LISTENER_QUERY:	return "Multicast Listener Query    ";
	 case MLD6_LISTENER_REPORT:	return "Multicast Listener Report   ";
	 case MLD6_LISTENER_DONE:	return "Multicast Listener Done     ";
	 default:
	    sprintf(unknown,
		    "UNKNOWN ICMPv6 message: type = 0x%02x, code = 0x%02x ",
		    type, code);
	    return unknown;
	}

     case IPPROTO_PIM:    /* PIM v2 */
	switch (type) {
	case PIM_V2_HELLO:	       return "PIM v2 Hello             ";
	case PIM_V2_REGISTER:          return "PIM v2 Register          ";
	case PIM_V2_REGISTER_STOP:     return "PIM v2 Register_Stop     ";
	case PIM_V2_JOIN_PRUNE:        return "PIM v2 Join/Prune        ";
	case PIM_V2_BOOTSTRAP:         return "PIM v2 Bootstrap         ";
	case PIM_V2_ASSERT:            return "PIM v2 Assert            ";
	case PIM_V2_GRAFT:             return "PIM-DM v2 Graft          ";
	case PIM_V2_GRAFT_ACK:         return "PIM-DM v2 Graft_Ack      ";
	case PIM_V2_CAND_RP_ADV:       return "PIM v2 Cand. RP Adv.     ";
	default:
	    sprintf(unknown,      "UNKNOWN PIM v2 message type =%3d ", type);
	    return unknown;
	}
    default:
	sprintf(unknown,          "UNKNOWN proto =%3d               ", proto);
	return unknown;
    }
}

/*
 * Open and initialize the MLD socket.
 */
void
init_mld6()
{
    struct icmp6_filter filt;
    int             on;

    rtalert_code = htons(IP6OPT_RTALERT_MLD);
    if (!mld6_recv_buf && (mld6_recv_buf = malloc(RECV_BUF_SIZE)) == NULL)
	    syslog(LOG_ERR, "malloc failed");
    if (!mld6_send_buf && (mld6_send_buf = malloc(RECV_BUF_SIZE)) == NULL)
	    syslog(LOG_ERR, "malloc failed");

    rcvcmsglen = CMSG_SPACE(sizeof(struct in6_pktinfo)) +
	    CMSG_SPACE(sizeof(int));
    if (rcvcmsgbuf == NULL && (rcvcmsgbuf = malloc(rcvcmsglen)) == NULL)
	    syslog(LOG_ERR,"malloc failed");
    
        syslog(LOG_DEBUG,"%d octets allocated for the emit/recept buffer mld6",RECV_BUF_SIZE);

    if ((mld6_socket = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) < 0)
		syslog(LOG_ERR, "MLD6 socket");

    k_set_rcvbuf(mld6_socket, SO_RECV_BUF_SIZE_MAX,
		 SO_RECV_BUF_SIZE_MIN);	/* lots of input buffering */
    k_set_hlim(mld6_socket, MINHLIM);	/* restrict multicasts to one hop */
    k_set_loop(mld6_socket, FALSE);	/* disable multicast loopback     */

    /* address initialization */
    allnodes_group.sin6_addr = in6addr_linklocal_allnodes_pim;
    if (inet_pton(AF_INET6, "ff02::2",
		  (void *) &allrouters_group.sin6_addr) != 1)
	syslog(LOG_ERR, "inet_pton failed for ff02::2");

    /* filter all non-MLD ICMP messages */
    ICMP6_FILTER_SETBLOCKALL(&filt);
    ICMP6_FILTER_SETPASS(ICMP6_MEMBERSHIP_QUERY, &filt);
    ICMP6_FILTER_SETPASS(ICMP6_MEMBERSHIP_REPORT, &filt);
    ICMP6_FILTER_SETPASS(ICMP6_MEMBERSHIP_REDUCTION, &filt);
    ICMP6_FILTER_SETPASS(MLD6_MTRACE_RESP, &filt);
    ICMP6_FILTER_SETPASS(MLD6_MTRACE, &filt);
    if (setsockopt(mld6_socket, IPPROTO_ICMPV6, ICMP6_FILTER, &filt,
		   sizeof(filt)) < 0)
	syslog(LOG_ERR, "setsockopt(ICMP6_FILTER)");


    /* specify to tell receiving interface */
    on = 1;
    if (setsockopt(mld6_socket, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on,
		   sizeof(on)) < 0)
	syslog(LOG_ERR, "setsockopt(IPV6_RECVPKTINFO)");

    on = 1;
    /* specify to tell value of hoplimit field of received IP6 hdr */
    if (setsockopt(mld6_socket, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on,
		   sizeof(on)) < 0)
	syslog(LOG_ERR, "setsockopt(IPV6_RECVHOPLIMIT)");

    /* initialize msghdr for receiving packets */
    rcviov[0].iov_base = (caddr_t) mld6_recv_buf;
    rcviov[0].iov_len = RECV_BUF_SIZE;
    rcvmh.msg_name = (caddr_t) & from;
    rcvmh.msg_namelen = sizeof(from);
    rcvmh.msg_iov = rcviov;
    rcvmh.msg_iovlen = 1;
    rcvmh.msg_control = (caddr_t) rcvcmsgbuf;
    rcvmh.msg_controllen = rcvcmsglen;

    /* initialize msghdr for sending packets */
    sndiov[0].iov_base = (caddr_t)mld6_send_buf;
    sndmh.msg_namelen = sizeof(struct sockaddr_in6);
    sndmh.msg_iov = sndiov;
    sndmh.msg_iovlen = 1;
    /* specifiy to insert router alert option in a hop-by-hop opt hdr. */
#ifndef USE_RFC2292BIS
    raopt[0] = IP6OPT_ROUTER_ALERT;
    raopt[1] = IP6OPT_RTALERT_LEN - 2;
    memcpy(&raopt[2], (caddr_t) & rtalert_code, sizeof(u_short));
#endif 

    /* register MLD message handler */
    if (register_input_handler(mld6_socket, mld6_read) < 0)
	syslog(LOG_ERR, "Couldn't register mld6_read as an input handler");
}


/* Read an MLD message */
static void
mld6_read(i, rfd)
    int             i;
    fd_set         *rfd;
{
    register int    mld6_recvlen;

    rcviov[0].iov_base = (caddr_t) mld6_recv_buf;
    rcviov[0].iov_len = RECV_BUF_SIZE;
    rcvmh.msg_controllen = rcvcmsglen;
    mld6_recvlen = recvmsg(mld6_socket, &rcvmh, 0);

    if (mld6_recvlen < 0)
    {
	if (errno != EINTR)
	    syslog(LOG_ERR, "MLD6 recvmsg");
	return;
    }

    /* TODO: make it as a thread in the future releases */
    accept_mld6(mld6_recvlen);
}

/*
 * Process a newly received MLD6 packet that is sitting in the input packet
 * buffer.
 */
static void
accept_mld6(recvlen)
int recvlen;
{
	struct in6_addr *group, *dst = NULL;
	struct mld6_hdr *mldh;
	struct cmsghdr *cm;
	struct in6_pktinfo *pi = NULL;
	int *hlimp = NULL;
	int ifindex = 0;
	struct sockaddr_in6 *src = (struct sockaddr_in6 *) rcvmh.msg_name;

	if (recvlen < sizeof(struct mld6_hdr))
	{
		syslog(LOG_WARNING, "received packet too short (%u bytes) for MLD header", recvlen);
		return;
	}
	mldh =  (struct mld6_hdr *) mld6_recv_buf;
//	mldh = (struct mld6_hdr *) rcvmh.msg_iov[0].iov_base;

	/*
	 * Packets sent up from kernel to daemon have ICMPv6 type = 0.
	 * Note that we set filters on the mld6_socket, so we should never
	 * see a "normal" ICMPv6 packet with type 0 of ICMPv6 type.
	 */
	if (mldh->mld6_type == 0) {
		/* XXX: msg_controllen must be reset in this case. */
		rcvmh.msg_controllen = rcvcmsglen;

		process_kernel_call();
		return;
	}

	group = &mldh->mld6_addr;

	/* extract optional information via Advanced API */
	for (cm = (struct cmsghdr *) CMSG_FIRSTHDR(&rcvmh);
	     cm;
	     cm = (struct cmsghdr *) CMSG_NXTHDR(&rcvmh, cm))
	{
		if (cm->cmsg_level == IPPROTO_IPV6 &&
		    cm->cmsg_type == IPV6_PKTINFO &&
		    cm->cmsg_len == CMSG_LEN(sizeof(struct in6_pktinfo)))
		{
			pi = (struct in6_pktinfo *) (CMSG_DATA(cm));
			ifindex = pi->ipi6_ifindex;
			dst = &pi->ipi6_addr;
		}
		if (cm->cmsg_level == IPPROTO_IPV6 &&
		    cm->cmsg_type == IPV6_HOPLIMIT &&
		    cm->cmsg_len == CMSG_LEN(sizeof(int)))
			hlimp = (int *) CMSG_DATA(cm);
	}
	if (hlimp == NULL)
	{
		syslog(LOG_WARNING, "failed to get receiving hop limit");
		return;
	}

	/* TODO: too noisy. Remove it? */
#ifdef NOSUCHDEF
	IF_DEBUG(DEBUG_PKT | debug_kind(IPPROTO_ICMPV6, mldh->mld6_type,
					mldh->mld6_code))
		syslog(LOG_DEBUG, "RECV %s from %s to %s",
		    packet_kind(IPPROTO_ICMPV6,
				mldh->mld6_type, mldh->mld6_code),
		    inet6_fmt(&src->sin6_addr), inet6_fmt(dst));
#endif				/* NOSUCHDEF */

	/* for an mtrace message, we don't need strict checks */
	if (mldh->mld6_type == MLD6_MTRACE) {
		accept_mtrace(src, dst, group, ifindex, (char *)(mldh + 1),
			      mldh->mld6_code, recvlen - sizeof(struct mld6_hdr));
		return;
	}

	/* hop limit check */
	if (*hlimp != 1)
	{
		syslog(LOG_WARNING, "received an MLD6 message with illegal hop limit(%d) from %s",
		    *hlimp, inet6_fmt(&src->sin6_addr));
		/* but accept the packet */
	}
	if (ifindex == 0)
	{
		syslog(LOG_WARNING, "failed to get receiving interface");
		return;
	}

	/* scope check */
	if (IN6_IS_ADDR_MC_NODELOCAL(&mldh->mld6_addr))
	{
		syslog(LOG_INFO,
		    "RECV %s with an invalid scope: %s from %s",
		    packet_kind(IPPROTO_ICMPV6, mldh->mld6_type,
				mldh->mld6_code),
		    inet6_fmt(&mldh->mld6_addr),
		    inet6_fmt(&src->sin6_addr));
		return;			/* discard */
	}

	/* source address check */
	if (!IN6_IS_ADDR_LINKLOCAL(&src->sin6_addr))
	{
		syslog(LOG_INFO,
		    "RECV %s from a non link local address: %s",
		    packet_kind(IPPROTO_ICMPV6, mldh->mld6_type,
				mldh->mld6_code),
		    inet6_fmt(&src->sin6_addr));
		return;
	}

	switch (mldh->mld6_type)
	{
	case MLD6_LISTENER_QUERY:
		accept_listener_query(src, dst, group,
				      ntohs(mldh->mld6_maxdelay));
		return;

	case MLD6_LISTENER_REPORT:
		accept_listener_report(src, dst, group);
		return;

	case MLD6_LISTENER_DONE:
		accept_listener_done(src, dst, group);
		return;

	default:
		/* This must be impossible since we set a type filter */
		syslog(LOG_INFO,
		    "ignoring unknown ICMPV6 message type %x from %s to %s",
		    mldh->mld6_type, inet6_fmt(&src->sin6_addr),
		    inet6_fmt(dst));
		return;
	}
}

static void
make_mld6_msg(type, code, src, dst, group, ifindex, delay, datalen, alert)
    int type, code, ifindex, delay, datalen, alert;
    struct sockaddr_in6 *src, *dst;
    struct in6_addr *group;
{
    static struct sockaddr_in6 dst_sa = {sizeof(dst_sa), AF_INET6};
    struct mld6_hdr *mhp = (struct mld6_hdr *)mld6_send_buf;
    int ctllen, hbhlen = 0;

    switch(type) {
    case MLD6_MTRACE:
    case MLD6_MTRACE_RESP:
	sndmh.msg_name = (caddr_t)dst;
	break;
    default:
	if (IN6_IS_ADDR_UNSPECIFIED(group))
	    dst_sa.sin6_addr = allnodes_group.sin6_addr;
	else
	    dst_sa.sin6_addr = *group;
	sndmh.msg_name = (caddr_t)&dst_sa;
	datalen = sizeof(struct mld6_hdr);
	break;
    }
   
    bzero(mhp, sizeof(*mhp));
    mhp->mld6_type = type;
    mhp->mld6_code = code;
    mhp->mld6_maxdelay = htons(delay);
    mhp->mld6_addr = *group;

    sndiov[0].iov_len = datalen;

    /* estimate total ancillary data length */
    ctllen = 0;
    if (ifindex != -1 || src)
	    ctllen += CMSG_SPACE(sizeof(struct in6_pktinfo));
    if (alert) {
#ifdef USE_RFC2292BIS
	if ((hbhlen = inet6_opt_init(NULL, 0)) == -1)
		syslog(LOG_ERR, "inet6_opt_init(0) failed");
	if ((hbhlen = inet6_opt_append(NULL, 0, hbhlen, IP6OPT_ROUTER_ALERT, 2,
				       2, NULL)) == -1)
		syslog(LOG_ERR, "inet6_opt_append(0) failed");
	if ((hbhlen = inet6_opt_finish(NULL, 0, hbhlen)) == -1)
		syslog(LOG_ERR, "inet6_opt_finish(0) failed");
	ctllen += CMSG_SPACE(hbhlen);
#else  /* old advanced API */
	hbhlen = inet6_option_space(sizeof(raopt));
	ctllen += hbhlen;
#endif
	
    }
    /* extend ancillary data space (if necessary) */
    if (ctlbuflen < ctllen) {
	    if (sndcmsgbuf)
		    free(sndcmsgbuf);
	    if ((sndcmsgbuf = malloc(ctllen)) == NULL)
		    syslog(LOG_ERR, "make_mld6_msg: malloc failed"); /* assert */
	    ctlbuflen = ctllen;
    }
    /* store ancillary data */
    if ((sndmh.msg_controllen = ctllen) > 0) {
	    struct cmsghdr *cmsgp;

	    sndmh.msg_control = sndcmsgbuf;
	    cmsgp = CMSG_FIRSTHDR(&sndmh);
		if(cmsgp)
	    if (ifindex != -1 || src) {
		    struct in6_pktinfo *pktinfo;

		    cmsgp->cmsg_len = CMSG_SPACE(sizeof(struct in6_pktinfo));
		    cmsgp->cmsg_level = IPPROTO_IPV6;
		    cmsgp->cmsg_type = IPV6_PKTINFO;
		    pktinfo = (struct in6_pktinfo *)CMSG_DATA(cmsgp);
		    memset((caddr_t)pktinfo, 0, sizeof(*pktinfo));
		    if (ifindex != -1)
			    pktinfo->ipi6_ifindex = ifindex;
		    if (src)
			    pktinfo->ipi6_addr = src->sin6_addr;
		    cmsgp = CMSG_NXTHDR(&sndmh, cmsgp);
	    }
		if(cmsgp)
	    if (alert) {
#ifdef USE_RFC2292BIS
		    int currentlen;
		    void *hbhbuf, *optp = NULL;

		    cmsgp->cmsg_len = CMSG_SPACE(hbhlen);
		    cmsgp->cmsg_level = IPPROTO_IPV6;
		    cmsgp->cmsg_type = IPV6_HOPOPTS;
		    hbhbuf = CMSG_DATA(cmsgp);

		    if ((currentlen = inet6_opt_init(hbhbuf, hbhlen)) == -1)
			    syslog(LOG_ERR, "inet6_opt_init(len = %d) failed",
				hbhlen);
		    if ((currentlen = inet6_opt_append(hbhbuf, hbhlen,
						       currentlen,
						       IP6OPT_ROUTER_ALERT, 2,
						       2, &optp)) == -1)
			    syslog(LOG_ERR,
				"inet6_opt_append(len = %d) failed",
				currentlen, hbhlen);
		    (void)inet6_opt_set_val(optp, 0, &rtalert_code,
					    sizeof(rtalert_code));
		    if (inet6_opt_finish(hbhbuf, hbhlen, currentlen) == -1)
			    syslog(LOG_ERR, "inet6_opt_finish(buf) failed");
#else  /* old advanced API */
		    if (inet6_option_init((void *)cmsgp, &cmsgp, IPV6_HOPOPTS))
			    syslog(LOG_ERR, /* assert */
				"make_mld6_msg: inet6_option_init failed");
		    if (inet6_option_append(cmsgp, raopt, 4, 0))
			    syslog(LOG_ERR, /* assert */
				"make_mld6_msg: inet6_option_append failed");
#endif 
		    cmsgp = CMSG_NXTHDR(&sndmh, cmsgp);
	    }
    }
    else
	    sndmh.msg_control = NULL; /* clear for safety */
}

void
send_mld6(type, code, src, dst, group, index, delay, datalen, alert)
    int type;
    int code;		/* for trace packets only */
    struct sockaddr_in6 *src;
    struct sockaddr_in6 *dst; /* may be NULL */
    struct in6_addr *group;
    int index, delay, alert;
    int datalen;		/* for trace packets only */
{
    int setloop = 0;
    struct sockaddr_in6 *dstp;
	
    make_mld6_msg(type, code, src, dst, group, index, delay, datalen, alert);
    dstp = (struct sockaddr_in6 *)sndmh.msg_name;
    if (IN6_ARE_ADDR_EQUAL(&dstp->sin6_addr, &allnodes_group.sin6_addr)) {
	setloop = 1;
	k_set_loop(mld6_socket, TRUE);
    }
    if (sendmsg(mld6_socket, &sndmh, 0) < 0) {
	if (errno == ENETDOWN)
	    check_vif_state();
	else
	    syslog(LOG_DEBUG,
		"sendmsg to %s with src %s on %s",
		inet6_fmt(&dstp->sin6_addr),
		src ? inet6_fmt(&src->sin6_addr) : "(unspec)",
		ifindex2str(index));

	if (setloop)
	    k_set_loop(mld6_socket, FALSE);
	return;
    }
    
	syslog(LOG_DEBUG, "SENT %s from %-15s to %s",
	    packet_kind(IPPROTO_ICMPV6, type, 0),
	    src ? inet6_fmt(&src->sin6_addr) : "unspec",
	    inet6_fmt(&dstp->sin6_addr));
}
