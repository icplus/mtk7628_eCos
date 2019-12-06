/*
 * Copyright (C) 1993-2002 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Added redirect stuff and a variety of bug fixes. (mcn@EnGarde.com)
 */

#include <stdio.h>
#include <string.h>
//#include <fcntl.h>
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

#include <netdb.h>
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

# define	STRERROR(x)	strerror(x)

#undef PRINTF
#undef FPRINTF
#define	PRINTF	(void)printf
#define	FPRINTF	(void)fprintf

extern	char	*optarg;
extern	ipnat_t	*natparse __P((char *, int));
extern	void	natparsefile __P((int, char *, int));
extern	void	printnat __P((ipnat_t *, int));
extern	void	printactivenat __P((nat_t *, int));
extern	void	printhostmap __P((hostmap_t *, u_int));
extern	char	*getsumd __P((u_32_t));

void	dostats __P((natstat_t *, int)), flushtable __P((int, int));
int	countbits __P((u_32_t));
char	*getnattype __P((ipnat_t *));
int	main __P((int, char*[]));
void	printaps __P((ap_session_t *, int));


static void usage(void)
{
	PRINTF("ipnat: [-CFhlnrstv]\n");
}

int ipnat_cmd(int argc, char *argv[])
{
	natstat_t ns, *nsp = &ns;
	char	*file, *core, *kernel;
	int	fd, opts, mode, idx;
	char *optstr;

	fd = -1;
	opts = 0;
	file = NULL;
	core = NULL;
	kernel = NULL;
	mode = O_RDWR;
	
	idx = 0;
	
	if( argc < 1 )
	   usage();
	while( idx < argc ) {
		optstr = argv[idx];
		if( '-' == optstr[0]) {
			switch (optstr[1])
			{
			case 'C' :
				opts |= OPT_CLEAR;
				break;
			case 'd' :
				opts |= OPT_DEBUG;
				break;
			case 'F' :
				opts |= OPT_FLUSH;
				break;
			case 'h' :
				opts |=OPT_HITS;
				break;
			case 'l' :
				opts |= OPT_LIST;
				mode = O_RDONLY;
				break;
			case 'n' :
				opts |= OPT_NODO;
				mode = O_RDONLY;
				break;
			case 'r' :
				opts |= OPT_REMOVE;
				break;
			case 's' :
				opts |= OPT_STAT;
				mode = O_RDONLY;
				break;
			case 't':
				opts |= OPT_SESSION;
				break;
			case 'v' :
				opts |= OPT_VERBOSE;
			break;
			default :
				usage();
				return -1;
			}
		}
		else {
			usage();
			goto exit;
		}
		idx++;
	}

	bzero((char *)&ns, sizeof(ns));

	if( nat_ioctl(&nsp, SIOCGNATS, mode) == -1) {
		perror("ioctl(SIOCGNATS)");
		return -1;
	}

	if (opts & (OPT_FLUSH|OPT_CLEAR))
		flushtable(mode, opts);

	if (opts & (OPT_LIST|OPT_STAT|OPT_SESSION))
		dostats(nsp, opts);
exit:
	return 0;
}


/*
 * Display NAT statistics.
 */
void dostats(nsp, opts)
natstat_t *nsp;
int opts;
{
	nat_t *np, nat, *shownat;
	ipnat_t	ipn;
	int s, idx=0, j;
	extern int	nat_table_max;
	
	/*
	 * Show statistics ?
	 */
	if (opts & OPT_STAT) {
		printf("mapped\tin\t%lu\tout\t%lu\n",
			nsp->ns_mapped[0], nsp->ns_mapped[1]);
		printf("added\t%lu\texpired\t%lu\n",
			nsp->ns_added, nsp->ns_expire);
		printf("no memory\t%lu\tbad nat\t%lu\n",
			nsp->ns_memfail, nsp->ns_badnat);
		printf("inuse\t%lu\nrules\t%lu\n",
			nsp->ns_inuse, nsp->ns_rules);
		printf("wilds\t%u\n", nsp->ns_wilds);
		printf("maxuse\t%u\n", nsp->ns_maxuse);
		if (opts & OPT_VERBOSE)
			printf("table %p list %p\n",
				nsp->ns_table, nsp->ns_list);
	}

	/*
	 * Show list of NAT rules and NAT sessions ?
	 */
	if ((opts & OPT_LIST) || (opts & OPT_SESSION)) {
		if (opts & OPT_LIST)
			printf("List of active MAP/Redirect filters:\n");
		while (nsp->ns_list) {
			memcpy((char *)&ipn, (long)nsp->ns_list, sizeof(ipn));
			
			if (opts & OPT_LIST)
			{
				if (opts & OPT_HITS)
					printf("%d ", ipn.in_hits);
				printnat(&ipn, opts & (OPT_DEBUG|OPT_VERBOSE));
			}
			nsp->ns_list = ipn.in_next;
		}

		if (opts & OPT_LIST)
			printf("\nList of active sessions:\n");
		if((shownat = malloc(sizeof(nat_t)*nat_table_max)) == 0)
			return;
		s = splnet();
		for (np = nsp->ns_instances; np; np = np->nat_next) 
		{
			memcpy((char *)&(shownat[idx++]), (long)np, sizeof(nat));
		}
		splx(s);
		if (opts & OPT_LIST)
		{
			for(j=0; j<idx; j++)
				printactivenat(&(shownat[j]), opts);
		}
		free(shownat);
		printf("Total %d active sessions\n", idx);
	}
}


/*
 * Issue an ioctl to flush either the NAT rules table or the active mapping
 * table or both.
 */
void flushtable(mode, opts)
int mode, opts;
{
	int n = 0;
	if (opts & OPT_FLUSH) {
		n = 0;
		if( !(opts & OPT_NODO) && nat_ioctl(&n, SIOCIPFFL, mode) == -1)
			perror("ioctl(SIOCFLNAT)");
		else
			diag_printf("%d entries flushed from NAT table\n", n);
	}
	
	if (opts & OPT_CLEAR) {
		n = 1;
		if (!(opts & OPT_NODO) && nat_ioctl(&n, SIOCIPFFL, mode) == -1)
			perror("ioctl(SIOCCNATL)");
		else
			diag_printf("%d entries flushed from NAT list\n", n);
	}
}

typedef struct nat_sessinfo_s {
	uint32_t sip;
	uint32_t dip;
	uint16_t sport;
	uint16_t dport;
	uint16_t prot;
	uint8_t s_tcpstate;
	uint8_t d_tcpstate;
	uint32_t age;
} nat_sessinfo_t;


int ipnat_session_list(void *buf, int buflen)
{
	natstat_t ns, *nsp;
	int i, s, mode, sess_num;
	nat_t *pnat, *np;
	nat_sessinfo_t *pinfo;
	int retval;
	extern int	nat_table_max;

	mode = O_RDWR;
	nsp = &ns;
	bzero(nsp, sizeof(ns));

	if (nat_ioctl(&nsp, SIOCGNATS, mode) < 0) {
		perror("ioctl(SIOCGNATS)");
		return 0;
	}
	
	if ((pnat = malloc(sizeof(nat_t)*nat_table_max)) == NULL)
		return 0;
	/*  Get a copy of NAT table  */
	s = splnet();
	for (sess_num=0, np=nsp->ns_instances; np; np=np->nat_next) 
		memcpy((char *)&pnat[sess_num++], (char *)np, sizeof(nat_t));
	splx(s);

	retval = sess_num * sizeof(nat_sessinfo_t);
	if (buf != NULL && buflen >= retval) {
		for (i=0, pinfo = (nat_sessinfo_t *)buf; i<sess_num; i++, pinfo++) {
			/*  Client IP  */
			pinfo->sip = ntohl(pnat[i].nat_inip.s_addr);
			/*  Dst IP  */
			pinfo->dip = ntohl(pnat[i].nat_oip.s_addr);
			if (pnat[i].nat_flags & IPN_TCPUDP) {
				/*  Client port  */
				pinfo->sport = ntohs(pnat[i].nat_inport);
				/*  Dst port  */
				pinfo->dport = ntohs(pnat[i].nat_oport);
			}
			else
				pinfo->sport = pinfo->dport = 0;
			pinfo->prot = pnat[i].nat_p;
			if (pinfo->prot == 6) {
				/*  TCP  */
				pinfo->s_tcpstate = pnat[i].nat_tcpstate[0];
				pinfo->d_tcpstate = pnat[i].nat_tcpstate[1];
			}
			else
				pinfo->s_tcpstate = pinfo->d_tcpstate = 0;

			/*  Convert to age in seconds  */
			pinfo->age = (pnat[i].nat_age+1)/2 ;
		}
	}
	else
		retval = -retval;
	
	free(pnat);
	return retval;	
}




