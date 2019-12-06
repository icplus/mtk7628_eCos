/*
 * Copyright (C) 1993-2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */
#ifdef __FreeBSD__
# ifndef __FreeBSD_cc_version
#  include <osreldate.h>
# else
#  if __FreeBSD_cc_version < 430000
#   include <osreldate.h>
#  endif
# endif
#endif

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
//#include <sys/file.h>
#if defined(STATETOP)
# if defined(_BSDI_VERSION)
#  undef STATETOP)
# endif
# if defined(__FreeBSD__) && \
     (!defined(__FreeBSD_version) || (__FreeBSD_version < 430000))
#  undef STATETOP
# endif
# if defined(__NetBSD_Version__)
#  if (__NetBSD_Version__ < 105000000)
#   undef STATETOP
#  else
#   include <poll.h>
#   define USE_POLL
#  endif
# endif
# if defined(sun)
#  if defined(__svr4__) || defined(__SVR4)
#   include <sys/select.h>
#  else
#   undef STATETOP	/* NOT supported on SunOS4 */
#  endif
# endif
#endif
//#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <net/if.h>
#if __FreeBSD_version >= 300000
# include <net/if_var.h>
#endif
#include <netdb.h>
#include <arpa/nameser.h>
#include <netinet/tcp.h>
#if defined(STATETOP) && !defined(linux)
# include <netinet/ip_var.h>
# include <netinet/tcp_fsm.h>
#endif
#include "ip_compat.h"
#include "ip_fil.h"
#include "ipf.h"
#include "ip_nat.h"
#include "ip_frag.h"
#include "ip_state.h"
#include "ip_proxy.h"
#include "ip_auth.h"
#ifdef STATETOP
# include "netinet/ipl.h"
# include <ctype.h>
# if SOLARIS || defined(__NetBSD__) || defined(_BSDI_VERSION) || \
     defined(__sgi)
#  ifdef ERR
#   undef ERR
#  endif
#  include <curses.h>
# else /* SOLARIS */
#  include <ncurses.h>
# endif /* SOLARIS */
#endif /* STATETOP */

#if defined(__NetBSD__) || (__OpenBSD__)
# include <paths.h>
#endif


//extern	char	*optarg;
//extern	int	optind;

#undef printf
#undef 	PRINTF
#undef 	FPRINTF
#define	PRINTF	(void)printf
#define	FPRINTF	(void)fprintf
#define	F_IN	0
#define	F_OUT	1
#define	F_ACIN	2
#define	F_ACOUT	3
static	char	*filters[4] = { "ipfilter(in)", "ipfilter(out)",
				"ipacct(in)", "ipacct(out)" };

int	opts;
int	use_inet6; 	

#ifdef STATETOP
#define	STSTRSIZE 	80
#define	STGROWSIZE	16
#define	HOSTNMLEN	40

#define	STSORT_PR	0
#define	STSORT_PKTS	1
#define	STSORT_BYTES	2
#define	STSORT_TTL	3
#define	STSORT_SRCIP	4
#define	STSORT_DSTIP	5
#define	STSORT_MAX	STSORT_DSTIP
#define	STSORT_DEFAULT	STSORT_BYTES


typedef struct statetop {
	union i6addr	st_src;
	union i6addr	st_dst;
	u_short		st_sport;
	u_short 	st_dport;
	u_char		st_p;
	u_char		st_state[2];
	U_QUAD_T	st_pkts;
	U_QUAD_T	st_bytes;
	u_long		st_age;
} statetop_t;
#endif

extern	int	main __P((int, char *[]));
static	void	showstats __P((friostat_t *, u_32_t));
static	void	showfrstates __P((ipfrstat_t *));
static	void	showlist __P((friostat_t *));
static	void	showipstates __P((ips_stat_t *));
static	void	showauthstates __P((fr_authstat_t *));
static	void	showgroups __P((friostat_t *));
static	void	usage (void);
static	void	printlist __P((frentry_t *));
static	void	parse_ipportstr __P((const char *, struct in_addr *, int *));
static	int	ipfstate_live __P((int, friostat_t **, ips_stat_t **,
				   ipfrstat_t **, fr_authstat_t **, u_32_t *));
#ifdef STATETOP
static	void	topipstates __P((struct in_addr, struct in_addr, int, int, int, int, int));
static	char	*ttl_to_string __P((long));
static	int	sort_p __P((const void *, const void *));
static	int	sort_pkts __P((const void *, const void *));
static	int	sort_bytes __P((const void *, const void *));
static	int	sort_ttl __P((const void *, const void *));
static	int	sort_srcip __P((const void *, const void *));
static	int	sort_dstip __P((const void *, const void *));
#endif



static void usage(void)
{
#ifdef  USE_INET6
	PRINTF("Usage: ipfstat [-6aAfhIinosv] [-d <device>]\n");
#else
	PRINTF("Usage: ipfstat [-aAfhIinosv] [-d <device>]\n");
#endif
	PRINTF("\t\t[-M corefile] [-N symbol-list]\n");
	PRINTF("       ipfstat -t [-S source address] [-D destination address] [-P protocol] [-T refreshtime] [-C] [-d <device>]\n");
}


int ipfstat_cmd(argc,argv)
int argc;
char *argv[];
{
	fr_authstat_t	frauthst;
	fr_authstat_t	*frauthstp = &frauthst;
	friostat_t fio;
	friostat_t *fiop = &fio;
	ips_stat_t ipsst;
	ips_stat_t *ipsstp = &ipsst;
	ipfrstat_t ifrst;
	ipfrstat_t *ifrstp = &ifrst;
	int device = IPL_LOGIPF;
	char	*kern = NULL;
	struct protoent *proto;

	int protocol = -1;		/* -1 = wild card for any protocol */
	int refreshtime = 1; 		/* default update time */
	int sport = -1;			/* -1 = wild card for any source port */
	int dport = -1;			/* -1 = wild card for any dest port */
	int topclosed = 0;		/* do not show closed tcp sessions */
	struct in_addr saddr, daddr;
	u_32_t frf;
	int idx = 0;
	char *optstr;

	saddr.s_addr = INADDR_ANY; 	/* default any source addr */ 
	daddr.s_addr = INADDR_ANY; 	/* default any dest addr */

	opts = 0;
	use_inet6 = 0;
	
	//while ((c = getopt(argc, argv, "6aACfghIilnoqstvd:D:M:N:P:S:T:")) != -1)
	if( argc < 1 ){
	   usage();
	   goto exit;
	}
	while( idx < argc ) 
	{
		optstr = argv[idx];
		if( '-' == optstr[0]) {
			switch (optstr[1])
			{
#ifdef	USE_INET6
			case '6' :
				use_inet6 = 1;
				break;
#endif
			case 'a' :
				opts |= OPT_ACCNT|OPT_SHOWLIST;
				break;
			case 'A' :
				device = IPL_LOGAUTH;
				opts |= OPT_AUTHSTATS;
				break;
			case 'C' :
				topclosed = 1;
				break;
			//case 'd' :
			//	device = optarg;
			//	break;
			case 'D' :
				parse_ipportstr(argv[++idx], &daddr, &dport);
				break;
			case 'f' :
				opts |= OPT_FRSTATES;
				break;
			case 'g' :
				opts |= OPT_GROUPS;
				break;
			case 'h' :
				opts |= OPT_HITS;
				break;
			case 'i' :
				opts |= OPT_INQUE|OPT_SHOWLIST;
				break;
			case 'I' :
				opts |= OPT_INACTIVE;
				break;
			case 'l' :
				opts |= OPT_SHOWLIST;
				break;
			case 'M' :
				break;
			case 'N' :
				break;
			case 'n' :
				opts |= OPT_SHOWLINENO;
				break;
			case 'o' :
				opts |= OPT_OUTQUE|OPT_SHOWLIST;
				break;
			case 'P' :
				if ((proto = getprotobyname(argv[++idx])) != NULL) {
					protocol = proto->p_proto;
				} else if (!sscanf(argv[idx], "%ud", &protocol) ||
					   (protocol < 0)) {
					PRINTF("ipfstat : Invalid protocol: %s\n", argv[idx]);
					goto exit;
				}
				break;
			case 'q' :
#if	SOLARIS
				showqiflist(kern);
				goto exit;
				break;
#else
				PRINTF("-q only availble on Solaris\n");
				goto exit;
				break;
#endif
			case 's' :
				opts |= OPT_IPSTATES;
				break;
			case 'S' :
				parse_ipportstr(argv[++idx], &saddr, &sport);
				break;
			case 't' :
#ifdef STATETOP
				opts |= OPT_STATETOP;
				break;
#else
				PRINTF("ipfstat : state top facility not compiled in\n");
				goto exit;
#endif
			case 'T' :
				if (!sscanf(argv[++idx], "%d", &refreshtime) ||
				    (refreshtime <= 0)) {
					PRINTF(	"ipfstat : Invalid refreshtime < 1 : %s\n", argv[idx]);
					goto exit;
				}
				break;
			case 'v' :
				opts |= OPT_VERBOSE;
				break;
			default :
				usage();
				break;
			}
		}else {
			usage();
			goto exit;
		}
		idx++;
	}

	
	bzero((char *)&fio, sizeof(fio));
	bzero((char *)&ipsst, sizeof(ipsst));
	bzero((char *)&ifrst, sizeof(ifrst));

	ipfstate_live(device, &fiop, &ipsstp, &ifrstp, &frauthstp, &frf);
	

	if (opts & OPT_IPSTATES) {
		showipstates(ipsstp);
	} else if (opts & OPT_SHOWLIST) {
		showlist(fiop);
		if ((opts & OPT_OUTQUE) && (opts & OPT_INQUE)){
			opts &= ~OPT_OUTQUE;
			showlist(fiop);
		}
	} else {
		if (opts & OPT_FRSTATES)
			showfrstates(ifrstp);
#ifdef STATETOP
		else if (opts & OPT_STATETOP)
			topipstates(saddr, daddr, sport, dport,
				    protocol, refreshtime, topclosed);
#endif
		else if (opts & OPT_AUTHSTATS)
			showauthstates(frauthstp);
		else if (opts & OPT_GROUPS)
			showgroups(fiop);
		else
			showstats(fiop, frf);
	}
exit:
	return 0;
}


/*
 * Fill in the stats structures from the live kernel, using a combination
 * of ioctl's and copying directly from kernel memory.
 */
int ipfstate_live(device, fiopp, ipsstpp, ifrstpp, frauthstpp, frfp)
int device;
friostat_t **fiopp;
ips_stat_t **ipsstpp;
ipfrstat_t **ifrstpp;
fr_authstat_t **frauthstpp;
u_32_t *frfp;
{
	if (!(opts & OPT_AUTHSTATS) && iplioctl(device, SIOCGETFS, fiopp, O_RDONLY, NULL) == -1) {
		perror("iplioctl(ipf:SIOCGETFS)");
		return -1;
	}

	if ((opts & OPT_IPSTATES)) {
		if ((iplioctl(IPL_LOGSTATE, SIOCGETFS, ipsstpp, O_RDONLY, NULL) == -1)) {
			perror("iplioctl(state:SIOCGETFS)");
			return -1;
		}
	}
	if ((opts & OPT_FRSTATES) && (iplioctl(device, SIOCGFRST, ifrstpp, O_RDONLY, NULL) == -1)) {
		perror("iplioctl(SIOCGFRST)");
		return -1;
	}

	if (opts & OPT_VERBOSE)
		PRINTF("opts %#x device %x\n", opts, device);

	if ((opts & OPT_AUTHSTATS) &&
	    (iplioctl(device, SIOCATHST, frauthstpp, O_RDONLY, NULL) == -1)) {
		perror("iplioctl(SIOCATHST)");
		return -1;
	}

	if (iplioctl(device, SIOCGETFF, frfp, O_RDONLY, NULL) == -1)
		perror("iplioctl(SIOCGETFF)");

	return 0;
}



/*
 * Display the kernel stats for packets blocked and passed and other
 * associated running totals which are kept.
 */
static	void	showstats(fp, frf)
struct	friostat	*fp;
u_32_t frf;
{

#if SOLARIS
	PRINTF("dropped packets:\tin %lu\tout %lu\n",
			fp->f_st[0].fr_drop, fp->f_st[1].fr_drop);
	PRINTF("non-data packets:\tin %lu\tout %lu\n",
			fp->f_st[0].fr_notdata, fp->f_st[1].fr_notdata);
	PRINTF("no-data packets:\tin %lu\tout %lu\n",
			fp->f_st[0].fr_nodata, fp->f_st[1].fr_nodata);
	PRINTF("non-ip packets:\t\tin %lu\tout %lu\n",
			fp->f_st[0].fr_notip, fp->f_st[1].fr_notip);
	PRINTF("   bad packets:\t\tin %lu\tout %lu\n",
			fp->f_st[0].fr_bad, fp->f_st[1].fr_bad);
	PRINTF("copied messages:\tin %lu\tout %lu\n",
			fp->f_st[0].fr_copy, fp->f_st[1].fr_copy);
#endif
#ifdef	USE_INET6
	PRINTF(" IPv6 packets:\t\tin %lu out %lu\n",
			fp->f_st[0].fr_ipv6[0], fp->f_st[0].fr_ipv6[1]);
#endif
	PRINTF(" input packets:\t\tblocked %lu passed %lu nomatch %lu",
			fp->f_st[0].fr_block, fp->f_st[0].fr_pass,
			fp->f_st[0].fr_nom);
	PRINTF(" counted %lu short %lu\n", 
			fp->f_st[0].fr_acct, fp->f_st[0].fr_short);
	PRINTF("output packets:\t\tblocked %lu passed %lu nomatch %lu",
			fp->f_st[1].fr_block, fp->f_st[1].fr_pass,
			fp->f_st[1].fr_nom);
	PRINTF(" counted %lu short %lu\n", 
			fp->f_st[1].fr_acct, fp->f_st[1].fr_short);
	PRINTF(" input packets logged:\tblocked %lu passed %lu\n",
			fp->f_st[0].fr_bpkl, fp->f_st[0].fr_ppkl);
	PRINTF("output packets logged:\tblocked %lu passed %lu\n",
			fp->f_st[1].fr_bpkl, fp->f_st[1].fr_ppkl);
	PRINTF(" packets logged:\tinput %lu output %lu\n",
			fp->f_st[0].fr_pkl, fp->f_st[1].fr_pkl);
	PRINTF(" log failures:\t\tinput %lu output %lu\n",
			fp->f_st[0].fr_skip, fp->f_st[1].fr_skip);
	PRINTF("fragment state(in):\tkept %lu\tlost %lu\n",
			fp->f_st[0].fr_nfr, fp->f_st[0].fr_bnfr);
	PRINTF("fragment state(out):\tkept %lu\tlost %lu\n",
			fp->f_st[1].fr_nfr, fp->f_st[1].fr_bnfr);
	PRINTF("packet state(in):\tkept %lu\tlost %lu\n",
			fp->f_st[0].fr_ads, fp->f_st[0].fr_bads);
	PRINTF("packet state(out):\tkept %lu\tlost %lu\n",
			fp->f_st[1].fr_ads, fp->f_st[1].fr_bads);
	PRINTF("ICMP replies:\t%lu\tTCP RSTs sent:\t%lu\n",
			fp->f_st[0].fr_ret, fp->f_st[1].fr_ret);
	PRINTF("Invalid source(in):\t%lu\n", fp->f_st[0].fr_badsrc);
	PRINTF("Result cache hits(in):\t%lu\t(out):\t%lu\n",
			fp->f_st[0].fr_chit, fp->f_st[1].fr_chit);
	PRINTF("IN Pullups succeeded:\t%lu\tfailed:\t%lu\n",
			fp->f_st[0].fr_pull[0], fp->f_st[0].fr_pull[1]);
	PRINTF("OUT Pullups succeeded:\t%lu\tfailed:\t%lu\n",
			fp->f_st[1].fr_pull[0], fp->f_st[1].fr_pull[1]);
	PRINTF("Fastroute successes:\t%lu\tfailures:\t%lu\n",
			fp->f_froute[0], fp->f_froute[1]);
	PRINTF("TCP cksum fails(in):\t%lu\t(out):\t%lu\n",
			fp->f_st[0].fr_tcpbad, fp->f_st[1].fr_tcpbad);

	PRINTF("Packet log flags set: (%#x)\n", frf);
	if (frf & FF_LOGPASS)
		PRINTF("\tpackets passed through filter\n");
	if (frf & FF_LOGBLOCK)
		PRINTF("\tpackets blocked by filter\n");
	if (frf & FF_LOGNOMATCH)
		PRINTF("\tpackets not matched by filter\n");
	if (!frf)
		PRINTF("\tnone\n");
}


/*
 * Print out a list of rules from the kernel, starting at the one passed.
 */
static void printlist(fp)
frentry_t *fp;
{
	struct	frentry	fb;
	int	n;

	for (n = 1; fp; n++) {
		memcpy((char *)&fb, (u_long)fp, sizeof(fb));
		fp = &fb;
		if (opts & OPT_OUTQUE)
			fp->fr_flags |= FR_OUTQUE;
		if (opts & (OPT_HITS|OPT_VERBOSE))
#ifdef	USE_QUAD_T
			PRINTF("%qu ", (unsigned long long) fp->fr_hits);
#else
			PRINTF("%lu ", fp->fr_hits);
#endif
		if (opts & (OPT_ACCNT|OPT_VERBOSE))
#ifdef	USE_QUAD_T
			PRINTF("%qu ", (unsigned long long) fp->fr_bytes);
#else
			PRINTF("%lu ", fp->fr_bytes);
#endif
		if (opts & OPT_SHOWLINENO)
			PRINTF("@%d ", n);
		printfr(fp);
		if (opts & OPT_VERBOSE)
			binprint(fp);
		if (fp->fr_grp)
			printlist(fp->fr_grp);
		fp = fp->fr_next;
	}
}

/*
 * print out all of the asked for rule sets, using the stats struct as
 * the base from which to get the pointers.
 */
static	void	showlist(fiop)
struct	friostat	*fiop;
{
	struct	frentry	*fp = NULL;
	int	i, set;

	set = fiop->f_active;
	if (opts & OPT_INACTIVE)
		set = 1 - set;
	if (opts & OPT_ACCNT) {
#ifdef USE_INET6
		if ((use_inet6) && (opts & OPT_OUTQUE)) {
			i = F_ACOUT;
			fp = (struct frentry *)fiop->f_acctout6[set];
		} else if ((use_inet6) && (opts & OPT_INQUE)) {
			i = F_ACIN;
			fp = (struct frentry *)fiop->f_acctin6[set];
		} else
#endif
		if (opts & OPT_OUTQUE) {
			i = F_ACOUT;
			fp = (struct frentry *)fiop->f_acctout[set];
		} else if (opts & OPT_INQUE) {
			i = F_ACIN;
			fp = (struct frentry *)fiop->f_acctin[set];
		} else {
			PRINTF("No -i or -o given with -a\n");
			return;
		}
	} else {
#ifdef	USE_INET6
		if ((use_inet6) && (opts & OPT_OUTQUE)) {
			i = F_OUT;
			fp = (struct frentry *)fiop->f_fout6[set];
		} else if ((use_inet6) && (opts & OPT_INQUE)) {
			i = F_IN;
			fp = (struct frentry *)fiop->f_fin6[set];
		} else
#endif
		if (opts & OPT_OUTQUE) {
			i = F_OUT;
			fp = (struct frentry *)fiop->f_fout[set];
		} else if (opts & OPT_INQUE) {
			i = F_IN;
			fp = (struct frentry *)fiop->f_fin[set];
		} else
			return;
	}
	if (opts & OPT_VERBOSE)
		PRINTF("showlist:opts %#x i %d\n", opts, i);

	if (opts & OPT_VERBOSE)
		PRINTF("fp %p set %d\n", fp, set);
	if (fp == NULL) {
		PRINTF("empty list for %s%s\n",
			(opts & OPT_INACTIVE) ? "inactive " : "", filters[i]);
		return;
	}
	printlist(fp);
}


/*
 * Display ipfilter stateful filtering information
 */
static void showipstates(ipsp)
ips_stat_t *ipsp;
{
	ipstate_t *istab[IPSTATE_SIZE];

	/*
	 * If a list of states hasn't been asked for, only print out stats
	 */
	if (!(opts & OPT_SHOWLIST)) {
		PRINTF("IP states added:\n\t%lu TCP\n\t%lu UDP\n\t%lu ICMP\n",
			ipsp->iss_tcp, ipsp->iss_udp, ipsp->iss_icmp);
		PRINTF("\t%lu hits\n\t%lu misses\n", ipsp->iss_hits,
			ipsp->iss_miss);
		PRINTF("\t%lu maximum\n\t%lu no memory\n\t%lu bkts in use\n",
			ipsp->iss_max, ipsp->iss_nomem, ipsp->iss_inuse);
		PRINTF("\t%lu active\n\t%lu expired\n\t%lu closed\n",
			ipsp->iss_active, ipsp->iss_expire, ipsp->iss_fin);
		return;
	}

	memcpy((char *)istab, (u_long)ipsp->iss_table, sizeof(istab));
		

	/*
	 * Print out all the state information currently held in the kernel.
	 */
	while (ipsp->iss_list != NULL) {
		ipsp->iss_list = printstate(ipsp->iss_list, opts);
	}
}


#if SOLARIS
/*
 * Displays the list of interfaces of which IPFilter has taken control in
 * Solaris.
 */
void showqiflist(kern)
char *kern;
{
	struct nlist qifnlist[2] = {
		{ "qif_head" },
		{ NULL }
	};
	qif_t qif, *qf;
	ill_t ill;

	if (kern == NULL)
		kern = "/dev/ksyms";

	if (nlist(kern, qifnlist) == -1) {
		fprintf(stderr, "nlist error\n");
		return;
	}

	printf("List of interfaces bound by IPFilter:\n");
	if (kmemcpy((char *)&qf, (u_long)qifnlist[0].n_value, sizeof(qf)))
		return;
	while (qf) {
		if (kmemcpy((char *)&qif, (u_long)qf, sizeof(qif)))
			break;
		if (kmemcpy((char *)&ill, (u_long)qif.qf_ill, sizeof(ill)))
			ill.ill_ppa = -1;
		printf("Name: %-8s Header Length: %2d SAP: %s (%04x) PPA %d",
			qif.qf_name, qif.qf_hl,
#ifdef	IP6_DL_SAP
			(qif.qf_sap == IP6_DL_SAP) ? "IPv6" : "IPv4"
#else
			"IPv4"
#endif
			, qif.qf_sap, ill.ill_ppa);
		printf(" %ld %ld", qif.qf_incnt, qif.qf_outcnt);
		qf = qif.qf_next;
		putchar('\n');
	}
}
#endif


#ifdef STATETOP
static void topipstates(saddr, daddr, sport, dport, protocol,
		        refreshtime, topclosed)
struct in_addr saddr;
struct in_addr daddr;
int sport;
int dport;
int protocol;
int refreshtime;
int topclosed;
{
	char str1[STSTRSIZE], str2[STSTRSIZE], str3[STSTRSIZE], str4[STSTRSIZE];
	int maxtsentries = 0, reverse = 0, sorting = STSORT_DEFAULT;
	int i, j, sfd, winx, tsentry, maxx, maxy, redraw = 0;
	ipstate_t *istab[IPSTATE_SIZE], ips;
	ips_stat_t ipsst, *ipsstp = &ipsst;
	statetop_t *tstable = NULL, *tp;
	char hostnm[HOSTNMLEN];
	struct protoent *proto;
	int c = 0;
	time_t t;
#ifdef USE_POLL
	struct pollfd set[1];
#else
	struct timeval selecttimeout; 
	fd_set readfd;
#endif

	/* open state device */
	if ((sfd = open(IPL_STATE, O_RDONLY)) == -1) {
		perror("open");
		exit(-1);
	}

	/* init ncurses stuff */
  	initscr();
  	cbreak();
  	noecho();

	/* init hostname */
	gethostname(hostnm, sizeof(hostnm) - 1);
	hostnm[sizeof(hostnm) - 1] = '\0';

	/* repeat until user aborts */
	while ( 1 ) {

		/* get state table */
		bzero((char *)&ipsst, sizeof(&ipsst));
		if ((ioctl(sfd, SIOCGETFS, &ipsstp) == -1)) {
			perror("ioctl(SIOCGETFS)");
			exit(-1);
		}
		if (kmemcpy((char *)istab, (u_long)ipsstp->iss_table,
			    sizeof(ips)))
			return;

		/* clear the history */
		tsentry = -1;

		/* read the state table and store in tstable */
		while (ipsstp->iss_list) {
			if (kmemcpy((char *)&ips, (u_long)ipsstp->iss_list,
				    sizeof(ips)))
				break;
			ipsstp->iss_list = ips.is_next;

			if (((saddr.s_addr == INADDR_ANY) ||
			     (saddr.s_addr == ips.is_saddr)) &&
			    ((daddr.s_addr == INADDR_ANY) ||
			     (daddr.s_addr == ips.is_daddr)) &&
			    ((protocol < 0) || (protocol == ips.is_p)) &&
			    (((ips.is_p != IPPROTO_TCP) &&
			     (ips.is_p != IPPROTO_UDP)) || 
			     (((sport < 0) ||
			       (htons(sport) == ips.is_sport)) &&
			      ((dport < 0) ||
			       (htons(dport) == ips.is_dport)))) &&
			     (topclosed || (ips.is_p != IPPROTO_TCP) ||
			     (ips.is_state[0] < TCPS_LAST_ACK) ||
			     (ips.is_state[1] < TCPS_LAST_ACK))) { 
				/*
				 * if necessary make room for this state
				 * entry
				 */
				tsentry++;
				if (!maxtsentries ||
				    (tsentry == maxtsentries)) {

					maxtsentries += STGROWSIZE;
					tstable = realloc(tstable, maxtsentries * sizeof(statetop_t));
					if (!tstable) {
						perror("malloc");
						exit(-1);
					}
				}

				/* fill structure */
				tp = tstable + tsentry;
				tp->st_src = ips.is_src;
				tp->st_dst = ips.is_dst;
				tp->st_p = ips.is_p;
				tp->st_state[0] = ips.is_state[0];
				tp->st_state[1] = ips.is_state[1];
				tp->st_pkts = ips.is_pkts;
				tp->st_bytes = ips.is_bytes;
				tp->st_age = ips.is_age;
				if ((ips.is_p == IPPROTO_TCP) ||
				    (ips.is_p == IPPROTO_UDP)) {
					tp->st_sport = ips.is_sport;
					tp->st_dport = ips.is_dport;
				}

			}
		}


		/* sort the array */
		if (tsentry != -1)
			switch (sorting)
			{
			case STSORT_PR:
				qsort(tstable, tsentry + 1,
				      sizeof(statetop_t), sort_p);
				break;
			case STSORT_PKTS:
				qsort(tstable, tsentry + 1,
				      sizeof(statetop_t), sort_pkts);
				break;
			case STSORT_BYTES:
				qsort(tstable, tsentry + 1,
				      sizeof(statetop_t), sort_bytes);
				break;
			case STSORT_TTL:
				qsort(tstable, tsentry + 1,
				      sizeof(statetop_t), sort_ttl);
				break;
			case STSORT_SRCIP:
				qsort(tstable, tsentry + 1,
				      sizeof(statetop_t), sort_srcip);
				break;
			case STSORT_DSTIP:
				qsort(tstable, tsentry + 1,
				      sizeof(statetop_t), sort_dstip);
				break;
			default:
				break;
			}

		/* print title */
		erase();
		getmaxyx(stdscr, maxy, maxx);
		attron(A_BOLD);
		winx = 0;
		move(winx,0);
		sprintf(str1, "%s - %s - state top", hostnm, IPL_VERSION);
		for (j = 0 ; j < (maxx - 8 - strlen(str1)) / 2; j++)
			printw(" ");
		printw("%s", str1);
		attroff(A_BOLD);

		/* just for fun add a clock */
		move(winx, maxx - 8);
		t = time(NULL);
		strftime(str1, 80, "%T", localtime(&t));
		printw("%s\n", str1);

		/*
		 * print the display filters, this is placed in the loop, 
		 * because someday I might add code for changing these
		 * while the programming is running :-)
		 */
		if (sport >= 0)
			sprintf(str1, "%s,%d", inet_ntoa(saddr), sport);
		else
			sprintf(str1, "%s", inet_ntoa(saddr));

		if (dport >= 0)
			sprintf(str2, "%s,%d", inet_ntoa(daddr), dport);
		else
			sprintf(str2, "%s", inet_ntoa(daddr));

		if (protocol < 0)
			strcpy(str3, "any");
		else if ((proto = getprotobynumber(protocol)) != NULL)
			sprintf(str3, "%s", proto->p_name); 
		else
			sprintf(str3, "%d", protocol);

		switch (sorting)
		{
		case STSORT_PR:
			sprintf(str4, "proto");
			break;
		case STSORT_PKTS:
			sprintf(str4, "# pkts");
			break;
		case STSORT_BYTES:
			sprintf(str4, "# bytes");
			break;
		case STSORT_TTL:
			sprintf(str4, "ttl");
			break;
		case STSORT_SRCIP:
			sprintf(str4, "srcip");
			break;
		case STSORT_DSTIP:
			sprintf(str4, "dstip");
			break;
		default:
			sprintf(str4, "unknown");
			break;
		}

		if (reverse)
			strcat(str4, " (reverse)");

		winx += 2;
		move(winx,0);
		printw("Src = %s  Dest = %s  Proto = %s  Sorted by = %s\n\n",
		       str1, str2, str3, str4);

		/* print column description */
		winx += 2;
		move(winx,0);
		attron(A_BOLD);
		printw("%-21s %-21s %3s %4s %7s %9s %9s\n", "Source IP",
		       "Destination IP", "ST", "PR", "#pkts", "#bytes", "ttl");
		attroff(A_BOLD);

		/* print all the entries */
		tp = tstable;
		if (reverse)
			tp += tsentry;

		if (tsentry > maxy - 6)
			tsentry = maxy - 6;
		for (i = 0; i <= tsentry; i++) {
			/* print src/dest and port */
			if ((tp->st_p == IPPROTO_TCP) ||
			    (tp->st_p == IPPROTO_UDP)) {
				sprintf(str1, "%s,%hu",
					inet_ntoa(tp->st_src.in4),
					ntohs(tp->st_sport));
				sprintf(str2, "%s,%hu",
					inet_ntoa(tp->st_dst.in4),
					ntohs(tp->st_dport));
			} else {
				sprintf(str1, "%s", inet_ntoa(tp->st_src.in4));
				sprintf(str2, "%s", inet_ntoa(tp->st_dst.in4));
			}
			winx++;
			move(winx, 0);
			printw("%-21s %-21s", str1, str2);

			/* print state */
			sprintf(str1, "%X/%X", tp->st_state[0],
				tp->st_state[1]);
			printw(" %3s", str1);

			/* print proto */
			proto = getprotobynumber(tp->st_p);
			if (proto) {
				strncpy(str1, proto->p_name, 4);
				str1[4] = '\0';
			} else {
				sprintf(str1, "%d", tp->st_p);
			}
			printw(" %4s", str1);
				/* print #pkt/#bytes */
#ifdef	USE_QUAD_T
			printw(" %7qu %9qu", (unsigned long long) tp->st_pkts,
				(unsigned long long) tp->st_bytes);
#else
			printw(" %7lu %9lu", tp->st_pkts, tp->st_bytes);
#endif
			printw(" %9s", ttl_to_string(tp->st_age));

			if (reverse)
				tp--;
			else
				tp++;
		}

		/* screen data structure is filled, now update the screen */
		if (redraw)
			clearok(stdscr,1);

		refresh();
		if (redraw) {
			clearok(stdscr,0);
			redraw = 0;
		}

		/* wait for key press or a 1 second time out period */
#ifdef USE_POLL
		set[0].fd = 0;
		set[0].events = POLLIN;
		poll(set, 1, refreshtime * 1000);

		/* if key pressed, read all waiting keys */
		if (set[0].revents & POLLIN)
#else
		selecttimeout.tv_sec = refreshtime;
		selecttimeout.tv_usec = 0;
		FD_ZERO(&readfd);
		FD_SET(0, &readfd);
		select(1, &readfd, NULL, NULL, &selecttimeout);

		/* if key pressed, read all waiting keys */
		if (FD_ISSET(0, &readfd))
#endif

		{
			c = wgetch(stdscr);
			if (c == ERR)
				continue;

			if (isalpha(c) && isupper(c))
				c = tolower(c);
			if (c == 'l') {
				redraw = 1;
			} else if (c == 'q') {
				break;	/* exits while() loop */
			} else if (c == 'r') {
				reverse = !reverse;
			} else if (c == 's') {
				sorting++;
				if (sorting > STSORT_MAX)
					sorting = 0;
			}
		}
	} /* while */

	close(sfd);

	printw("\n");
	nocbreak();
	endwin();
}
#endif


/*
 * Show fragment cache information that's held in the kernel.
 */
static void showfrstates(ifsp)
ipfrstat_t *ifsp;
{
	struct ipfr *ipfrtab[IPFT_SIZE], ifr;
	frentry_t fr;
	int i;

	/*
	 * print out the numeric statistics
	 */
	PRINTF("IP fragment states:\n\t%lu new\n\t%lu expired\n\t%lu hits\n",
		ifsp->ifs_new, ifsp->ifs_expire, ifsp->ifs_hits);
	PRINTF("\t%lu no memory\n\t%lu already exist\n",
		ifsp->ifs_nomem, ifsp->ifs_exists);
	PRINTF("\t%lu inuse\n", ifsp->ifs_inuse);
	memcpy((char *)ipfrtab, (u_long)ifsp->ifs_table, sizeof(ipfrtab));

	/*
	 * Print out the contents (if any) of the fragment cache table.
	 */
	for (i = 0; i < IPFT_SIZE; i++)
		while (ipfrtab[i]) {
			memcpy((char *)&ifr, (u_long)ipfrtab[i], sizeof(ifr));
				
			PRINTF("%s -> ", iphostname(4, &ifr.ipfr_src));
			memcpy((char *)&fr, (u_long)ifr.ipfr_rule, sizeof(fr));
				
			PRINTF("%s %d %d %d %#02x = %#x\n",
				iphostname(4, &ifr.ipfr_dst), ifr.ipfr_id,
				ifr.ipfr_ttl, ifr.ipfr_p, ifr.ipfr_tos,
				fr.fr_flags);
			ipfrtab[i] = ifr.ipfr_next;
		}
	memcpy((char *)ipfrtab, (u_long)ifsp->ifs_nattab,sizeof(ipfrtab));
		
	for (i = 0; i < IPFT_SIZE; i++)
		while (ipfrtab[i]) {
			memcpy((char *)&ifr, (u_long)ipfrtab[i], sizeof(ifr));
				
			PRINTF("NAT: %s -> ", iphostname(4, &ifr.ipfr_src));
			memcpy((char *)&fr, (u_long)ifr.ipfr_rule, sizeof(fr)); 
				
			PRINTF("%s %d %d %d %#02x = %#x\n",
				iphostname(4, &ifr.ipfr_dst), ifr.ipfr_id,
				ifr.ipfr_ttl, ifr.ipfr_p, ifr.ipfr_tos,
				fr.fr_flags);
			ipfrtab[i] = ifr.ipfr_next;
		}
}


/*
 * Show stats on how auth within IPFilter has been used
 */
static void showauthstates(asp)
fr_authstat_t *asp;
{
	frauthent_t *frap, fra;

#ifdef	USE_QUAD_T
	printf("Authorisation hits: %qu\tmisses %qu\n",
		(unsigned long long) asp->fas_hits,
		(unsigned long long) asp->fas_miss);
#else
	printf("Authorisation hits: %ld\tmisses %ld\n", asp->fas_hits,
		asp->fas_miss);
#endif
	printf("nospace %ld\nadded %ld\nsendfail %ld\nsendok %ld\n",
		asp->fas_nospace, asp->fas_added, asp->fas_sendfail,
		asp->fas_sendok);
	printf("queok %ld\nquefail %ld\nexpire %ld\n",
		asp->fas_queok, asp->fas_quefail, asp->fas_expire);

	frap = asp->fas_faelist;
	while (frap) {
		memcpy((char *)&fra, (u_long)frap, sizeof(fra));
			
		printf("age %ld\t", fra.fae_age);
		printfr(&fra.fae_fr);
		frap = fra.fae_next;
	}
}


/*
 * Display groups used for each of filter rules, accounting rules and
 * authentication, separately.
 */
static void showgroups(fiop)
struct friostat	*fiop;
{
	static char *gnames[3] = { "Filter", "Accounting", "Authentication" };
	frgroup_t *fp, grp;
	int on, off, i;

	on = fiop->f_active;
	off = 1 - on;

	for (i = 0; i < 3; i++) {
		printf("%s groups (active):\n", gnames[i]);
		for (fp = fiop->f_groups[i][on]; fp; fp = grp.fg_next){
			memcpy((char *)&grp, (u_long)fp, sizeof(grp));
			printf("%hu\n", grp.fg_num);
		}
		printf("%s groups (inactive):\n", gnames[i]);
		for (fp = fiop->f_groups[i][off]; fp; fp = grp.fg_next){
			memcpy((char *)&grp, (u_long)fp, sizeof(grp));
			printf("%hu\n", grp.fg_num);
		}
	}
}

static void parse_ipportstr(argument, ip, port)
const char *argument;
struct in_addr *ip;
int *port;
{

	char *s, *comma;

	/* make working copy of argument, Theoretically you must be able
	 * to write to optarg, but that seems very ugly to me....
	 */
	if ((s = malloc(strlen(argument) + 1)) == NULL)
		perror("malloc");
	strcpy(s, argument);

	/* get port */
	if ((comma = strchr(s, ',')) != NULL) {
		if (!strcasecmp(s, "any")) {
			*port = -1;
		} else if (!sscanf(comma + 1, "%d", port) ||
			   (*port < 0) || (*port > 65535)) {
			printf("Invalid port specfication in %s\n",
				argument);
			goto out;
		}
		*comma = '\0';
	}


	/* get ip address */
	if (!strcasecmp(s, "any")) {
		ip->s_addr = INADDR_ANY;
	} else	if (!inet_aton(s, ip)) {
		printf( "Invalid IP address: %s\n", s);
		goto out;
	}
out:
	/* free allocated memory */
	free(s);
}


#ifdef STATETOP
static char ttlbuf[STSTRSIZE];

static char *ttl_to_string(ttl)
long int ttl;
{

	int hours, minutes, seconds;

	/* ttl is in half seconds */
	ttl /= 2;

	hours = ttl / 3600;
	ttl = ttl % 3600;
	minutes = ttl / 60;
	seconds = ttl % 60;

	if (hours > 0 )
		sprintf(ttlbuf, "%2d:%02d:%02d", hours, minutes, seconds);
	else
		sprintf(ttlbuf, "%2d:%02d", minutes, seconds);
	return ttlbuf;
}


static int sort_pkts(a, b)
const void *a;
const void *b;
{

	register const statetop_t *ap = a;
	register const statetop_t *bp = b;

	if (ap->st_pkts == bp->st_pkts)
		return 0;
	else if (ap->st_pkts < bp->st_pkts)
		return 1;
	return -1;
}


static int sort_bytes(a, b)
const void *a;
const void *b;
{
	register const statetop_t *ap = a;
	register const statetop_t *bp = b;

	if (ap->st_bytes == bp->st_bytes)
		return 0;
	else if (ap->st_bytes < bp->st_bytes)
		return 1;
	return -1;
}


static int sort_p(a, b)
const void *a;
const void *b;
{
	register const statetop_t *ap = a;
	register const statetop_t *bp = b;

	if (ap->st_p == bp->st_p)
		return 0;
	else if (ap->st_p < bp->st_p)
		return 1;
	return -1;
}


static int sort_ttl(a, b)
const void *a;
const void *b;
{
	register const statetop_t *ap = a;
	register const statetop_t *bp = b;

	if (ap->st_age == bp->st_age)
		return 0;
	else if (ap->st_age < bp->st_age)
		return 1;
	return -1;
}

static int sort_srcip(a, b)
const void *a;
const void *b;
{
	register const statetop_t *ap = a;
	register const statetop_t *bp = b;

	if (ntohl(ap->st_src.in4.s_addr) == ntohl(bp->st_src.in4.s_addr))
		return 0;
	else if (ntohl(ap->st_src.in4.s_addr) > ntohl(bp->st_src.in4.s_addr))
		return 1;
	return -1;
}

static int sort_dstip(a, b)
const void *a;
const void *b;
{
	register const statetop_t *ap = a;
	register const statetop_t *bp = b;

	if (ntohl(ap->st_dst.in4.s_addr) == ntohl(bp->st_dst.in4.s_addr))
		return 0;
	else if (ntohl(ap->st_dst.in4.s_addr) > ntohl(bp->st_dst.in4.s_addr))
		return 1;
	return -1;
}
#endif


