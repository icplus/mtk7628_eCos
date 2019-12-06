/*
 * Copyright (C) 1993-2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Added redirect stuff and a variety of bug fixes. (mcn@EnGarde.com)
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

#include <sys/time.h>
#include <sys/param.h>
//#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <net/if.h>
#if __FreeBSD_version >= 300000
# include <net/if_var.h>
#endif
#include <net/netdb.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
//#include <resolv.h>
#include <ctype.h>
#include <ip_compat.h>
#include <ip_fil.h>
#include <ip_nat.h>
#include <ip_state.h>
#include <ip_proxy.h>
#include <ipf.h>


#define	STRERROR(x)	strerror(x)


#ifdef	USE_INET6
extern	int	use_inet6;
#endif

//extern	char	thishost[MAXHOSTNAMELEN];

extern	int	countbits __P((u_32_t));

void	printnat __P((ipnat_t *, int));
char	*getnattype __P((ipnat_t *));
void	printactivenat __P((nat_t *, int));
void	printhostmap __P((hostmap_t *, u_int));
char	*getsumd __P((u_32_t));

static void	printaps __P((ap_session_t *, int));

static void printaps(aps, opts)
ap_session_t *aps;
int opts;
{
	ipsec_pxy_t ipsec;
	ap_session_t ap;
	ftpinfo_t ftp;
	aproxy_t apr;
	raudio_t ra;
	struct esp_pxy espxy;
	
	memcpy((char *)&ap, (long)aps, sizeof(ap));
	
	memcpy((char *)&apr, (long)ap.aps_apr, sizeof(apr));
	
	printf("\tproxy %s/%d use %d flags %x\n", apr.apr_label,
		apr.apr_p, apr.apr_ref, apr.apr_flags);
	printf("\t\tproto %d flags %#x bytes ", ap.aps_p, ap.aps_flags);
#ifdef	USE_QUAD_T
	printf("%qu pkts %qu", (unsigned long long)ap.aps_bytes,
		(unsigned long long)ap.aps_pkts);
#else
	printf("%lu pkts %lu", ap.aps_bytes, ap.aps_pkts);
#endif
	printf(" data %s size %d\n", ap.aps_data ? "YES" : "NO", ap.aps_psiz);
	if ((ap.aps_p == IPPROTO_TCP) && (opts & OPT_VERBOSE)) {
		printf("\t\tstate[%u,%u], sel[%d,%d]\n",
			ap.aps_state[0], ap.aps_state[1],
			ap.aps_sel[0], ap.aps_sel[1]);
#if (defined(NetBSD) && (NetBSD >= 199905) && (NetBSD < 1991011)) || \
    (__FreeBSD_version >= 300000) || defined(OpenBSD)
		printf("\t\tseq: off %hd/%hd min %x/%x\n",
			ap.aps_seqoff[0], ap.aps_seqoff[1],
			ap.aps_seqmin[0], ap.aps_seqmin[1]);
		printf("\t\tack: off %hd/%hd min %x/%x\n",
			ap.aps_ackoff[0], ap.aps_ackoff[1],
			ap.aps_ackmin[0], ap.aps_ackmin[1]);
#else
		printf("\t\tseq: off %hd/%hd min %lx/%lx\n",
			ap.aps_seqoff[0], ap.aps_seqoff[1],
			ap.aps_seqmin[0], ap.aps_seqmin[1]);
		printf("\t\tack: off %hd/%hd min %lx/%lx\n",
			ap.aps_ackoff[0], ap.aps_ackoff[1],
			ap.aps_ackmin[0], ap.aps_ackmin[1]);
#endif
	}

	if (!strcmp(apr.apr_label, "raudio") && ap.aps_psiz == sizeof(ra) && ap.aps_data) {
		memcpy((char *)&ra, (long)ap.aps_data, sizeof(ra));
		printf("\tReal Audio Proxy:\n");
		printf("\t\tSeen PNA: %d\tVersion: %d\tEOS: %d\n",
			ra.rap_seenpna, ra.rap_version, ra.rap_eos);
		printf("\t\tMode: %#x\tSBF: %#x\n", ra.rap_mode, ra.rap_sbf);
		printf("\t\tPorts:pl %hu, pr %hu, sr %hu\n",
			ra.rap_plport, ra.rap_prport, ra.rap_srport);
	} else if (!strcmp(apr.apr_label, "ftp") &&
		   (ap.aps_psiz == sizeof(ftp)) && ap.aps_data) {
		memcpy((char *)&ftp, (long)ap.aps_data, sizeof(ftp));
		printf("\tFTP Proxy:\n");
		printf("\t\tpassok: %d\n", ftp.ftp_passok);
		ftp.ftp_side[0].ftps_buf[FTP_BUFSZ - 1] = '\0';
		ftp.ftp_side[1].ftps_buf[FTP_BUFSZ - 1] = '\0';
		printf("\tClient:\n");
		printf("\t\tseq %08x%08x len %d junk %d cmds %d\n",
			ftp.ftp_side[0].ftps_seq[1],
			ftp.ftp_side[0].ftps_seq[0],
			ftp.ftp_side[0].ftps_len,
			ftp.ftp_side[0].ftps_junk, ftp.ftp_side[0].ftps_cmds);
		printf("\t\tbuf [");
		printbuf(ftp.ftp_side[0].ftps_buf, FTP_BUFSZ, 1);
		printf("]\n\tServer:\n");
		printf("\t\tseq %08x%08x len %d junk %d cmds %d\n",
			ftp.ftp_side[1].ftps_seq[1],
			ftp.ftp_side[1].ftps_seq[0],
			ftp.ftp_side[1].ftps_len,
			ftp.ftp_side[1].ftps_junk, ftp.ftp_side[1].ftps_cmds);
		printf("\t\tbuf [");
		printbuf(ftp.ftp_side[1].ftps_buf, FTP_BUFSZ, 1);
		printf("]\n");
	} else if (!strcmp(apr.apr_label, "ipsec") &&
		   (ap.aps_psiz == sizeof(ipsec)) && ap.aps_data) {
		memcpy((char *)&ipsec, (long)ap.aps_data, sizeof(ipsec));
		printf("\tIPSec Proxy:\n");
		printf("\t\tICookie %08x%08x RCookie %08x%08x %s\n",
			(u_int)ntohl(ipsec.ipsc_icookie[0]),
			(u_int)ntohl(ipsec.ipsc_icookie[1]),
			(u_int)ntohl(ipsec.ipsc_rcookie[0]),
			(u_int)ntohl(ipsec.ipsc_rcookie[1]),
			ipsec.ipsc_rckset ? "(Set)" : "(Not set)");
	}
	else if (!strcmp(apr.apr_label, "esp") &&
		   (ap.aps_psiz == sizeof(struct esp_pxy)) && ap.aps_data) {
		memcpy((char *)&espxy, (long)ap.aps_data, sizeof(struct esp_pxy));
		printf("\tESP Proxy:\n");
		printf("\t\tSPI[0] %08x SPI[1] %08x\n",
			(u_int)ntohl(espxy.spi[0]),
			(u_int)ntohl(espxy.spi[1]));
	}
}


/*
 * Get a nat filter type given its kernel address.
 */
char *getnattype(ipnat)
ipnat_t *ipnat;
{
	static char unknownbuf[20];
	ipnat_t ipnatbuff;
	char *which;
	
	#if 0 /* 30/10/03 Roger ++,  */	
	if (!ipnat || (ipnat && memcpy((char *)&ipnatbuff, (long)ipnat,
					sizeof(ipnatbuff))))
	#else
	if(ipnat)
		memcpy((char *)&ipnatbuff, (long)ipnat, sizeof(ipnatbuff));
	else
		{
		panic("getnattype:ipnat=NULL");
		return NULL;
		}
	if (!ipnat)
	#endif /* 30/10/03 Roger --,  */	
		return "???";
	memset(&ipnatbuff,0,sizeof(ipnatbuff));
	switch (ipnatbuff.in_redir)
	{
	case NAT_MAP :
		which = "MAP";
		break;
	case NAT_MAPBLK :
		which = "MAP-BLOCK";
		break;
	case NAT_REDIRECT :
		which = "RDR";
		break;
	case NAT_BIMAP :
		which = "BIMAP";
		break;
	default :
		sprintf(unknownbuf, "unknown(%04x)",
			ipnatbuff.in_redir & 0xffffffff);
		which = unknownbuf;
		break;
	}
	return which;
}


void printactivenat(nat, opts)
nat_t *nat;
int opts;
{
	u_int hv1, hv2;
	char proto[16];
	struct protoent *p;
	
	p = getprotobynumber(nat->nat_p);
	if (p) 
		strncpy(proto, p->p_name, sizeof(proto)-1);
	else
		snprintf(proto, sizeof(proto)-1, "%d", nat->nat_p);

	printf("%s %-15s", getnattype(nat->nat_ptr), inet_ntoa(nat->nat_inip));

	if ((nat->nat_flags & IPN_TCPUDP) != 0)
		printf(" %-5u", ntohs(nat->nat_inport));
	else
		printf("      ");

	printf(" <- -> %-15s",inet_ntoa(nat->nat_outip));

	if ((nat->nat_flags & IPN_TCPUDP) != 0)
		printf(" %-5u", ntohs(nat->nat_outport));
	else
		printf("      ");

	printf(" [%s", inet_ntoa(nat->nat_oip));
	if ((nat->nat_flags & IPN_TCPUDP) != 0)
		printf(" %u", ntohs(nat->nat_oport));
	printf("]");
	
	printf(" %s", proto);

	if (opts & OPT_VERBOSE) {
		printf("\n\tage %lu sumd %s/",
			nat->nat_age, getsumd(nat->nat_sumd[0]));
#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
		hv1 = NAT_HASH_FN(nat->nat_inip.s_addr, nat->nat_inport,
				  NAT_TABLE_SZ),
		hv2 = NAT_HASH_FN(nat->nat_outip.s_addr, nat->nat_outport,
				  NAT_TABLE_SZ),
#else
		hv1 = NAT_HASH_FN(nat->nat_inip.s_addr, nat->nat_inport,
				  0xffffffff),
		hv1 = NAT_HASH_FN(nat->nat_oip.s_addr, hv1 + nat->nat_oport,
				  NAT_TABLE_SZ),
		hv2 = NAT_HASH_FN(nat->nat_outip.s_addr, nat->nat_outport,
				  0xffffffff),
		hv2 = NAT_HASH_FN(nat->nat_oip.s_addr, hv2 + nat->nat_oport,
				  NAT_TABLE_SZ),
#endif
		printf("%s pr %u bkt %d/%d flags %x\n",
			getsumd(nat->nat_sumd[1]), nat->nat_p,
			hv1, hv2, nat->nat_flags);
		printf("\tifp %s ", (char *) getifname(nat->nat_ifp));

		if(nat->nat_p == 6)
			printf(" tcpstat [%d/%d]", (nat->nat_tcpstate)[0], (nat->nat_tcpstate)[1]);

		if(nat->nat_gre.gs_call != 0)
			printf(" greid=%x", nat->nat_gre.gs_call);
		/* 10/02/04 Roger --,  */	
	}
	printf("\n");
	if (nat->nat_aps)
		printf("nat->nat_aps:%x\n", nat->nat_aps);
}


void printhostmap(hmp, hv)
hostmap_t *hmp;
u_int hv;
{
	printf("%s -> ", inet_ntoa(hmp->hm_realip));
	printf("%s ", inet_ntoa(hmp->hm_mapip));
	printf("(use = %d hv = %u)\n", hmp->hm_ref, hv);
}


char *getsumd(sum)
u_32_t sum;
{
	static char sumdbuf[17];

	if (sum & NAT_HW_CKSUM)
		sprintf(sumdbuf, "hw(%#0x)", sum & 0xffff);
	else
		sprintf(sumdbuf, "%#0x", sum);
	return sumdbuf;
}


/*
 * Print out a NAT rule
 */
void printnat(np, opts)
ipnat_t *np;
int opts;
{
	struct	protoent	*pr;
	struct	servent	*sv;
	int	bits;

	pr = getprotobynumber(np->in_p);

	switch (np->in_redir)
	{
	case NAT_REDIRECT :
		printf("rdr");
		break;
	case NAT_MAP :
		printf("map");
		break;
	case NAT_MAPBLK :
		printf("map-block");
		break;
	case NAT_BIMAP :
		printf("bimap");
		break;
	default :
		fprintf(stderr, "unknown value for in_redir: %#x\n",
			np->in_redir);
		break;
	}

	printf(" %s ", np->in_ifname);

	if (np->in_flags & IPN_FILTER) {
		if (np->in_flags & IPN_NOTSRC)
			printf("! ");
		printf("from ");
		if (np->in_redir == NAT_REDIRECT) {
			printhostmask(4, (u_32_t *)&np->in_srcip,
				      (u_32_t *)&np->in_srcmsk);
		} else {
			printhostmask(4, (u_32_t *)&np->in_inip,
				      (u_32_t *)&np->in_inmsk);
		}
		if (np->in_scmp)
			printportcmp(np->in_p, &np->in_tuc.ftu_src);

		if (np->in_flags & IPN_NOTDST)
			printf(" !");
		printf(" to ");
		if (np->in_redir == NAT_REDIRECT) {
			printhostmask(4, (u_32_t *)&np->in_outip,
				      (u_32_t *)&np->in_outmsk);
		} else {
			printhostmask(4, (u_32_t *)&np->in_srcip,
				      (u_32_t *)&np->in_srcmsk);
		}
		if (np->in_dcmp)
			printportcmp(np->in_p, &np->in_tuc.ftu_dst);
	}

	if (np->in_redir == NAT_REDIRECT) {
		if (!(np->in_flags & IPN_FILTER)) {
			printf("%s", inet_ntoa(np->in_out[0]));
			bits = countbits(np->in_out[1].s_addr);
			if (bits != -1)
				printf("/%d ", bits);
			else
				printf("/%s ", inet_ntoa(np->in_out[1]));
			printf("port %d", ntohs(np->in_pmin));
			if (np->in_pmax != np->in_pmin)
				printf("- %d", ntohs(np->in_pmax));
		}
		/* 14/11/03 Roger ++,  */	
		if(((np->in_flags & IPN_TRIG) ^ np->in_triguse)) {
			if ((np->in_flags & IPN_TCPUDP) == IPN_TCPUDP)
				printf(" trigger tcp/udp");
			else if ((np->in_flags & IPN_TCP) == IPN_TCP)
				printf(" trigger tcp");
			else if ((np->in_flags & IPN_UDP) == IPN_UDP)
				printf(" trigger udp");
			else if (np->in_p == 0)
				printf(" trigger ip");
		
			printf(" port %d-%d", np->in_trigpmin, np->in_trigpmax);
			if ((np->in_trigp & IPN_TCPUDP) == IPN_TCPUDP)
				printf(" tcp/udp");
			else if ((np->in_trigp & IPN_TCP) == IPN_TCP)
				printf(" tcp");
			else if ((np->in_trigp & IPN_UDP) == IPN_UDP)
				printf(" udp");
			printf("\n");
			return;
		}
		/* 14/11/03 Roger --,  */	
		printf(" -> %s", inet_ntoa(np->in_in[0]));
		if(np->in_flags & IPN_TRIG) /* 14/11/03 Roger,  */	
			printf(" (Trigger age %d)", np->in_trigage); 
		if (np->in_flags & IPN_SPLIT)
			printf(",%s", inet_ntoa(np->in_in[1]));
		printf(" port %d", ntohs(np->in_pnext));
		if ((np->in_flags & IPN_TCPUDP) == IPN_TCPUDP)
			printf(" tcp/udp");
		else if ((np->in_flags & IPN_TCP) == IPN_TCP)
			printf(" tcp");
		else if ((np->in_flags & IPN_UDP) == IPN_UDP)
			printf(" udp");
		else if (np->in_p == 0)
			printf(" ip");
		else if (np->in_p != 0) {
			if (pr != NULL)
				printf(" %s", pr->p_name);
			else
				printf(" %d", np->in_p);
		}
		if (np->in_flags & IPN_ROUNDR)
			printf(" round-robin");
		if (np->in_flags & IPN_FRAG)
			printf(" frag");
		if (np->in_age[0])
			printf(" age %d/%d", np->in_age[0], np->in_age[1]);
		if (np->in_mssclamp)
			printf(" mssclamp %u", np->in_mssclamp);
		printf("\n");
		if (opts & OPT_DEBUG)
			printf("\tspc %lu flg %#x max %u use %d\n",
			       np->in_space, np->in_flags,
			       np->in_pmax, np->in_use);
	} else {
		np->in_nextip.s_addr = htonl(np->in_nextip.s_addr);
		if (!(np->in_flags & IPN_FILTER)) {
			printf("%s/", inet_ntoa(np->in_in[0]));
			bits = countbits(np->in_in[1].s_addr);
			if (bits != -1)
				printf("%d", bits);
			else
				printf("%s", inet_ntoa(np->in_in[1]));
		}
		printf(" -> ");
		if (np->in_flags & IPN_IPRANGE) {
			printf("range %s-", inet_ntoa(np->in_out[0]));
			printf("%s", inet_ntoa(np->in_out[1]));
		} else {
			printf("%s/", inet_ntoa(np->in_out[0]));
			bits = countbits(np->in_out[1].s_addr);
			if (bits != -1)
				printf("%d", bits);
			else
				printf("%s", inet_ntoa(np->in_out[1]));
		}
		if (*np->in_plabel) {
			printf(" proxy port");
			if (np->in_dport != 0) {
				if (pr != NULL)
					sv = getservbyport(np->in_dport,
							   pr->p_name);
				else
					sv = getservbyport(np->in_dport, NULL);
				if (sv != NULL)
					printf(" %s", sv->s_name);
				else
					printf(" %hu", ntohs(np->in_dport));
			}
			printf(" %.*s/", (int)sizeof(np->in_plabel),
				np->in_plabel);
			if (pr != NULL)
				printf("%s", pr->p_name);
				//fputs(pr->p_name, stdout);
			else
				printf("%d", np->in_p);
		} else if (np->in_redir == NAT_MAPBLK) {
			if ((np->in_pmin == 0) &&
			    (np->in_flags & IPN_AUTOPORTMAP))
				printf(" ports auto");
			else
				printf(" ports %d", np->in_pmin);
			if (opts & OPT_DEBUG)
				printf("\n\tip modulous %d", np->in_pmax);
		} else if (np->in_pmin || np->in_pmax) {
			/* 10/01/04 Roger ++,  */	
			//printf(" portmap");
			if (np->in_flags & IPN_ICMPQUERY)
				printf(" icmpidmap");
			else
				printf(" portmap");
			/* 10/01/04 Roger --,  */	
			if ((np->in_flags & IPN_TCPUDP) == IPN_TCPUDP)
				printf(" tcp/udp");
			else if (np->in_flags & IPN_TCP)
				printf(" tcp");
			else if (np->in_flags & IPN_UDP)
				printf(" udp");
			/* 10/01/04 Roger ++,  */	
			else if (np->in_flags & IPN_ICMPQUERY)
					printf(" icmp");
			/* 10/01/04 Roger --,  */	
			if (np->in_flags & IPN_AUTOPORTMAP) {
				printf(" auto");
				if (opts & OPT_DEBUG)
					printf(" [%d:%d %d %d]",
					       ntohs(np->in_pmin),
					       ntohs(np->in_pmax),
					       np->in_ippip, np->in_ppip);
			} else {
				printf(" %d:%d", ntohs(np->in_pmin),
				       ntohs(np->in_pmax));
			}
		}
		if (np->in_flags & IPN_FRAG)
			printf(" frag");
		if (np->in_age[0])
			printf(" age %d/%d", np->in_age[0], np->in_age[1]);
		if (np->in_mssclamp)
			printf(" mssclamp %u", np->in_mssclamp);
		printf("\n");
		if (opts & OPT_DEBUG) {
			printf("\tspace %lu nextip %s pnext %d", np->in_space,
			       inet_ntoa(np->in_nextip), np->in_pnext);
			printf(" flags %x use %u\n",
			       np->in_flags, np->in_use);
		}
	}
}


