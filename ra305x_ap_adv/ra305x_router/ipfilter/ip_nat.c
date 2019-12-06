/*
 * Copyright (C) 1995-2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Added redirect stuff and a LOT of bug fixes. (mcn@EnGarde.com)
 */
#if defined(__FreeBSD__) && defined(KERNEL) && !defined(_KERNEL)
#define _KERNEL
#endif

#include <sys/errno.h>
#include <sys/bsdtypes.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/malloc.h>

#include <sys/ioctl.h>
#include <sys/protosw.h>
#include <sys/socket.h>

#include <sys/mbuf.h>

#include <net/if.h>

#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_var.h>

#ifdef RFC1825
# include <vpn/md5.h>
# include <vpn/ipsec.h>
extern struct ifnet vpnif;
#endif

#include <netinet/ip_var.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <ip_compat.h> 
#include <netinet/tcpip.h>

#include <ip_fil.h>
#include <ip_nat.h>
#include <ip_frag.h>
#include <ip_state.h>
#include <ip_proxy.h>
#include <cfg_net.h>
#include <sys_status.h>

// #define DEBUG_NAT
#ifdef DEBUG_NAT
#define	NATDBG(fmt, ...)	diag_printf(fmt, ## __VA_ARGS__)
#else
#define NATDBG(fmt, ...)
#endif	/*  DEBUG_NAT  */


#ifndef	MIN
# define	MIN(a,b)	(((a)<(b))?(a):(b))
#endif
#undef	SOCKADDR_IN
#define	SOCKADDR_IN	struct sockaddr_in

nat_t	**nat_table[2] = { NULL, NULL };
nat_t	*nat_instances = NULL;
/* 27/07/04 Roger ++,  */	
nat_t 	*nat_pool = NULL;
nat_t	*free_nat = NULL; 
/* 27/07/04 Roger --,  */		
ipnat_t	*nat_list = NULL;
u_int	ipf_nattable_sz = NAT_TABLE_SZ;
u_int	ipf_natrules_sz = NAT_SIZE;
u_int	ipf_rdrrules_sz = RDR_SIZE;
u_int	ipf_hostmap_sz = HOSTMAP_SIZE;
u_32_t	nat_masks = 0;
u_32_t	rdr_masks = 0;
ipnat_t	**nat_rules = NULL;
ipnat_t	**rdr_rules = NULL;
hostmap_t	**maptable  = NULL;
ipnat_t	*trig_rules = NULL; /* 11/11/03 Roger,  add */	
ipnat_t *nat_default_rule=NULL; /* 09/12/04 Roger,  add*/	
static u_32_t hm_nat_sess_limit;
static u_32_t nat_sess_himark;

u_long	fr_defnatage = DEF_NAT_AGE,
	fr_defnaticmpage = 6;		/* 3 seconds */
natstat_t nat_stats;
int	fr_nat_lock = 0;
#ifdef CONFIG_SUPERDMZ
extern char *superdmz_mac;
#endif

static	int	nat_flushtable __P((void));
static	void	nat_addnat __P((struct ipnat *));
static	void	nat_addrdr __P((struct ipnat *));
static	void	nat_delete __P((struct nat *));
static	void	nat_delrdr __P((struct ipnat *));
static	void	nat_delnat __P((struct ipnat *));
static	int	fr_natgetent __P((caddr_t));
static	int	fr_natgetsz __P((caddr_t));
static	int	fr_natputent __P((caddr_t));
void	nat_tabmove __P((nat_t *));
static	int	nat_match __P((fr_info_t *, ipnat_t *, ip_t *));
static	hostmap_t *nat_hostmap __P((ipnat_t *, struct in_addr,
				    struct in_addr));
static	void	nat_hostmapdel __P((struct hostmap *));
static	void	nat_mssclamp __P((tcphdr_t *, u_short, fr_info_t *, u_short *));

static int check_icmpquerytype4 __P((int));
static int check_icmperrortype4 __P((int));


extern int nat_set_fcache(fr_info_t *fin, nat_t *nat);
extern void nat_unset_fcache(nat_t *nat);

nat_t * nat_reflect_rule(fr_info_t *fin, nat_t *onat, int flags);
void speedup_natexpire(void);

#ifdef CONFIG_NAT_FORCE_TCPWIN
void nat_adj_tcpwindow(tcphdr_t *, int, u_short *);
#endif

extern	int	nat_table_max;

//**************************
// nat initilaization
//**************************
int nat_init(void)
{
	hm_nat_sess_limit = 3  * nat_table_max / 4;
	nat_sess_himark = 4 * nat_table_max / 5;

	KMALLOCS(nat_table[0], nat_t **, sizeof(nat_t *) * ipf_nattable_sz);
	if (nat_table[0] != NULL)
		bzero((char *)nat_table[0], ipf_nattable_sz * sizeof(nat_t *));
	else
		return -1;

	KMALLOCS(nat_table[1], nat_t **, sizeof(nat_t *) * ipf_nattable_sz);
	if (nat_table[1] != NULL)
		bzero((char *)nat_table[1], ipf_nattable_sz * sizeof(nat_t *));
	else
		return -1;

	KMALLOCS(nat_rules, ipnat_t **, sizeof(ipnat_t *) * ipf_natrules_sz);
	if (nat_rules != NULL)
		bzero((char *)nat_rules, ipf_natrules_sz * sizeof(ipnat_t *));
	else
		return -1;

	KMALLOCS(rdr_rules, ipnat_t **, sizeof(ipnat_t *) * ipf_rdrrules_sz);
	if (rdr_rules != NULL)
		bzero((char *)rdr_rules, ipf_rdrrules_sz * sizeof(ipnat_t *));
	else
		return -1;

	KMALLOCS(maptable, hostmap_t **, sizeof(hostmap_t *) * ipf_hostmap_sz);
	if (maptable != NULL)
		bzero((char *)maptable, sizeof(hostmap_t *) * ipf_hostmap_sz);
	else
		return -1;
		
	if (free_nat == NULL)
	{
		int i;
		nat_t *nat;
#undef malloc		
		if((nat_pool = malloc(sizeof(nat_t ) * nat_table_max)))
		{
			nat = nat_pool;
			bzero((char *)nat, sizeof(nat_t ) * nat_table_max);

			for(i=0; i<nat_table_max; i++)
			{
				nat->nat_next = nat+1;
				nat++;
			}
			nat--;
			nat->nat_next = NULL;
			free_nat = nat_pool;
		}
		else
			diag_printf("%s: No memory for NAT pool\n", __FUNCTION__);
	}

	return 0;
}


static void nat_addrdr(n)
ipnat_t *n;
{
	ipnat_t **np;
	u_32_t j;
	u_int hv;
	int k;
	
	k = countbits(n->in_outmsk);
	if ((k >= 0) && (k != 32))
		rdr_masks |= 1 << k;
	j = (n->in_outip & n->in_outmsk);
	hv = NAT_HASH_FN(j, 0, ipf_rdrrules_sz);
	np = rdr_rules + hv;
	while (*np != NULL)
		np = &(*np)->in_rnext;
	n->in_rnext = NULL;
	n->in_prnext = np;
	*np = n;
	/* 13/11/03 Roger ++,  */
	if(n->in_flags & IPN_TRIG) {	
		n->in_trignext = trig_rules;
		trig_rules = n;
	}
	/* 13/11/03 Roger --,  */	
	/* 15/09/04 Roger ++,  add rdr rule in nat rules */	
	//nat_addnat(n);
	/* 15/09/04 Roger --,  */	
}


static void nat_addnat(n)
ipnat_t *n;
{
	ipnat_t **np;
	u_32_t j;
	u_int hv;
	int k;

	k = countbits(n->in_inmsk);
	if ((k >= 0) && (k != 32))
		nat_masks |= 1 << k;
	j = (n->in_inip & n->in_inmsk);
	hv = NAT_HASH_FN(j, 0, ipf_natrules_sz);
	np = nat_rules + hv;
	while (*np != NULL)
		np = &(*np)->in_mnext;
	n->in_mnext = NULL;
	n->in_pmnext = np;
	*np = n;
}


static void nat_delrdr(n)
ipnat_t *n;
{
	if (n->in_rnext)
		n->in_rnext->in_prnext = n->in_prnext;
	*n->in_prnext = n->in_rnext;
	/* 15/09/04 Roger ++,  */	
	//nat_delnat(n);
	/* 15/09/04 Roger --,  */	
}


static void nat_delnat(n)
ipnat_t *n;
{
	if (n->in_mnext)
		n->in_mnext->in_pmnext = n->in_pmnext;
	*n->in_pmnext = n->in_mnext;
}


/*
 * check if an ip address has already been allocated for a given mapping that
 * is not doing port based translation.
 *
 * Must be called with ipf_nat held as a write lock.
 */
static struct hostmap *nat_hostmap(np, real, map)
ipnat_t *np;
struct in_addr real;
struct in_addr map;
{
	hostmap_t *hm;
	u_int hv;

	hv = real.s_addr % HOSTMAP_SIZE;
	for (hm = maptable[hv]; hm; hm = hm->hm_next)
		if ((hm->hm_realip.s_addr == real.s_addr) &&
		    (np == hm->hm_ipnat)) {
			hm->hm_ref++;
			return hm;
		}

	KMALLOC(hm, hostmap_t *);
	if (hm) {
		hm->hm_next = maptable[hv];
		hm->hm_pnext = maptable + hv;
		if (maptable[hv])
			maptable[hv]->hm_pnext = &hm->hm_next;
		maptable[hv] = hm;
		hm->hm_ipnat = np;
		hm->hm_realip = real;
		hm->hm_mapip = map;
		hm->hm_ref = 1;
	}
	return hm;
}


/*
 * Must be called with ipf_nat held as a write lock.
 */
static void nat_hostmapdel(hm)
struct hostmap *hm;
{
	ATOMIC_DEC32(hm->hm_ref);
	if (hm->hm_ref == 0) {
		if (hm->hm_next)
			hm->hm_next->hm_pnext = hm->hm_pnext;
		*hm->hm_pnext = hm->hm_next;
		KFREE(hm);
	}
}

nat_t *nat_alloc(int dir, int flags)
{
	nat_t *nat;
	
	if (nat_stats.ns_inuse >= nat_sess_himark) 
	{
		speedup_natexpire();
//		return NULL;
	}
	
	if(free_nat == NULL)
		return NULL;
		
	nat = free_nat;
	free_nat = nat->nat_next;
	if(nat)
	{
		bzero((char *)nat, sizeof(*nat));
		nat->nat_flags = flags;
		nat->nat_dir = dir;
	}
	return nat;
}

static void nat_free(nat_t *nat)
{
	nat->nat_next = free_nat;
	free_nat = nat;
}

//void fix_outcksum(fin, sp, n)
//fr_info_t *fin;
void fix_outcksum(dlen, sp, n)
int dlen;
u_short *sp;
u_32_t n;
{
	register u_short sumshort;
	register u_32_t sum1;

	if (!n)
		return;
	else if (n & NAT_HW_CKSUM) {
		n &= 0xffff;
		//n += fin->fin_dlen;
		n += dlen;
		n = (n & 0xffff) + (n >> 16);
		*sp = n & 0xffff;
		return;
	}
	sum1 = (~ntohs(*sp)) & 0xffff;
	sum1 += (n);
	sum1 = (sum1 >> 16) + (sum1 & 0xffff);
	/* Again */
	sum1 = (sum1 >> 16) + (sum1 & 0xffff);
	sumshort = ~(u_short)sum1;
	*(sp) = htons(sumshort);
}


//void fix_incksum(fin, sp, n)
//fr_info_t *fin;
void fix_incksum(dlen, sp, n)
int dlen; 
u_short *sp;
u_32_t n;
{
	register u_short sumshort;
	register u_32_t sum1;

	if (!n)
		return;
	else if (n & NAT_HW_CKSUM) {
		n &= 0xffff;
		//n += fin->fin_dlen;
		n += dlen;
		n = (n & 0xffff) + (n >> 16);
		*sp = n & 0xffff;
		return;
	}

	sum1 = (~ntohs(*sp)) & 0xffff;
	sum1 += ~(n) & 0xffff;
	sum1 = (sum1 >> 16) + (sum1 & 0xffff);
	/* Again */
	sum1 = (sum1 >> 16) + (sum1 & 0xffff);
	sumshort = ~(u_short)sum1;
	*(sp) = htons(sumshort);
}


/*
 * fix_datacksum is used *only* for the adjustments of checksums in the data
 * section of an IP packet.
 *
 * The only situation in which you need to do this is when NAT'ing an 
 * ICMP error message. Such a message, contains in its body the IP header
 * of the original IP packet, that causes the error.
 *
 * You can't use fix_incksum or fix_outcksum in that case, because for the
 * kernel the data section of the ICMP error is just data, and no special 
 * processing like hardware cksum or ntohs processing have been done by the 
 * kernel on the data section.
 */
void fix_datacksum(sp, n)
u_short *sp;
u_32_t n;
{
	register u_short sumshort;
	register u_32_t sum1;

	if (!n)
		return;

	sum1 = (~ntohs(*sp)) & 0xffff;
	sum1 += (n);
	sum1 = (sum1 >> 16) + (sum1 & 0xffff);
	/* Again */
	sum1 = (sum1 >> 16) + (sum1 & 0xffff);
	sumshort = ~(u_short)sum1;
	*(sp) = htons(sumshort);
}

/*
 * How the NAT is organised and works.
 *
 * Inside (interface y) NAT       Outside (interface x)
 * -------------------- -+- -------------------------------------
 * Packet going          |   out, processsed by ip_natout() for x
 * ------------>         |   ------------>
 * src=10.1.1.1          |   src=192.1.1.1
 *                       |
 *                       |   in, processed by ip_natin() for x
 * <------------         |   <------------
 * dst=10.1.1.1          |   dst=192.1.1.1
 * -------------------- -+- -------------------------------------
 * ip_natout() - changes ip_src and if required, sport
 *             - creates a new mapping, if required.
 * ip_natin()  - changes ip_dst and if required, dport
 *
 * In the NAT table, internal source is recorded as "in" and externally
 * seen as "out".
 */

/*
 * Handle ioctls which manipulate the NAT.
 */
int nat_ioctl(data, cmd, mode)
u_long cmd;
caddr_t data;
int mode;
{
	register ipnat_t *nat, *nt, *n = NULL, **np = NULL;
	int error = 0, ret, arg = -1, getlock;
	ipnat_t natd;
	u_32_t i, j;
	extern ipnat_t *nat_default_rule;

	nat = NULL;     /* XXX gcc -Wuninitialized */
	KMALLOC(nt, ipnat_t *);
	getlock = (mode & NAT_LOCKHELD) ? 0 : 1;
	if ((cmd == SIOCADNAT) || (cmd == SIOCRMNAT)) {
		if (mode & NAT_SYSSPACE) {
			bcopy(data, (char *)&natd, sizeof(natd));
			error = 0;
		} else {
			error = IRCOPYPTR(data, (char *)&natd, sizeof(natd));
		}
	} else if (cmd == SIOCIPFFL) {	/* SIOCFLNAT & SIOCCNATL */
		error = IRCOPY(data, (char *)&arg, sizeof(arg));
		if (error)
			error = EFAULT;
	}

	if (error)
		goto done;

	/*
	 * For add/delete, look to see if the NAT entry is already present
	 */
	if (getlock == 1) {
		WRITE_ENTER(&ipf_nat);
	}
	if ((cmd == SIOCADNAT) || (cmd == SIOCRMNAT)) {
		nat = &natd;
		nat->in_flags &= IPN_USERFLAGS;
		if ((nat->in_redir & NAT_MAPBLK) == 0) {
			if ((nat->in_flags & IPN_SPLIT) == 0)
				nat->in_inip &= nat->in_inmsk;
			if ((nat->in_flags & IPN_IPRANGE) == 0)
				nat->in_outip &= nat->in_outmsk;
		}
		for (np = &nat_list; (n = *np); np = &n->in_next)
			if (!bcmp((char *)&nat->in_flags, (char *)&n->in_flags,
					IPN_CMPSIZ)) {
				if (n->in_redir == NAT_REDIRECT &&
				    n->in_pnext != nat->in_pnext)
					continue;
				break;
			}
	}

	switch (cmd)
	{
#ifdef  IPFILTER_LOG
	case SIOCIPFFB :
	{
		int tmp;

		if (!(mode & FWRITE))
			error = EPERM;
		else {
			tmp = ipflog_clear(IPL_LOGNAT);
			IWCOPY((char *)&tmp, (char *)data, sizeof(tmp));
		}
		break;
	}
#endif
	case SIOCADNAT :
		if (!(mode & FWRITE)) {
			error = EPERM;
			break;
		}
		if (n) {
			error = EEXIST;
			break;
		}
		if (nt == NULL) {
			error = ENOMEM;
			break;
		}
		if(nat == NULL) {
			error = ENOENT;
			break;
		}
		n = nt;
		nt = NULL;
		bcopy((char *)nat, (char *)n, sizeof(*n));
		n->in_ifp = (void *)GETUNIT(n->in_ifname, 4);
		//if (!n->in_ifp)
		//	n->in_ifp = (void *)-1;
		if (n->in_plabel[0] != '\0') {
			n->in_apr = appr_lookup(n->in_p, n->in_plabel);
			if (!n->in_apr) {
				error = ENOENT;
				break;
			}
		}
		n->in_next = NULL;
		if (np != NULL)
		*np = n;

		if (n->in_redir & NAT_REDIRECT) {
			n->in_flags &= ~IPN_NOTDST;
			nat_addrdr(n);
		}
		if (n->in_redir & (NAT_MAP|NAT_MAPBLK)) {
			n->in_flags &= ~IPN_NOTSRC;
			nat_addnat(n);
		}
		if(n->in_flags & IPN_DEFAULT_RULE)
			nat_default_rule = n;
			
		n->in_use = 0;
		if (n->in_redir & NAT_MAPBLK)
			n->in_space = USABLE_PORTS * ~ntohl(n->in_outmsk);
		else if (n->in_flags & IPN_AUTOPORTMAP)
			n->in_space = USABLE_PORTS * ~ntohl(n->in_inmsk);
		else if (n->in_flags & IPN_IPRANGE)
			n->in_space = ntohl(n->in_outmsk) - ntohl(n->in_outip);
		else if (n->in_flags & IPN_SPLIT)
			n->in_space = 2;
		else
			n->in_space = ~ntohl(n->in_outmsk);
		/*
		 * Calculate the number of valid IP addresses in the output
		 * mapping range.  In all cases, the range is inclusive of
		 * the start and ending IP addresses.
		 * If to a CIDR address, lose 2: broadcast + network address
		 *			         (so subtract 1)
		 * If to a range, add one.
		 * If to a single IP address, set to 1.
		 */
		if (n->in_space) {
			if ((n->in_flags & IPN_IPRANGE) != 0)
				n->in_space += 1;
			else
				n->in_space -= 1;
		} else
			n->in_space = 1;
		if ((n->in_outmsk != 0xffffffff) && (n->in_outmsk != 0) &&
		    ((n->in_flags & (IPN_IPRANGE|IPN_SPLIT)) == 0))
			n->in_nip = ntohl(n->in_outip) + 1;
		else if ((n->in_flags & IPN_SPLIT) &&
			 (n->in_redir & NAT_REDIRECT))
			n->in_nip = ntohl(n->in_inip);
		else
			n->in_nip = ntohl(n->in_outip);
		if (n->in_redir & NAT_MAP) {
			n->in_pnext = ntohs(n->in_pmin);
			/*
			 * Multiply by the number of ports made available.
			 */
			if (ntohs(n->in_pmax) >= ntohs(n->in_pmin)) {
				n->in_space *= (ntohs(n->in_pmax) -
						ntohs(n->in_pmin) + 1);
				/*
				 * Because two different sources can map to
				 * different destinations but use the same
				 * local IP#/port #.
				 * If the result is smaller than in_space, then
				 * we may have wrapped around 32bits.
				 */
				i = n->in_inmsk;
				if ((i != 0) && (i != 0xffffffff)) {
					j = n->in_space * (~ntohl(i) + 1);
					if (j >= n->in_space)
						n->in_space = j;
					else
						n->in_space = 0xffffffff;
				}
			}
			/*
			 * If no protocol is specified, multiple by 256.
			 */
			//if ((n->in_flags & IPN_TCPUDP) == 0) {/* 10/01/04 Roger,  */	
			if ((n->in_flags & IPN_TCPUDPICMP) == 0) {
					j = n->in_space * 256;
					if (j >= n->in_space)
						n->in_space = j;
					else
						n->in_space = 0xffffffff;
			}
		}
		/* Otherwise, these fields are preset */
		n = NULL;
		nat_stats.ns_rules++;
		break;
	case SIOCRMNAT :
		if (!(mode & FWRITE)) {
			error = EPERM;
			n = NULL;
			break;
		}
		if (!n) {
			error = ESRCH;
			break;
		}
		if (n->in_redir & NAT_REDIRECT)
			nat_delrdr(n);
		if (n->in_redir & (NAT_MAPBLK|NAT_MAP))
			nat_delnat(n);
		if (nat_list == NULL) {
			nat_masks = 0;
			rdr_masks = 0;
		}
		*np = n->in_next;
		if (!n->in_use) {
			if (n->in_apr)
				appr_free(n->in_apr);
			/* 27/01/04 Roger ++,  */	
			if (n->in_mport)
				KFREE(n->in_mport);
			/* 27/01/04 Roger --,  */
			KFREE(n);
			nat_stats.ns_rules--;
		} else {
			n->in_flags |= IPN_DELETE;
			n->in_next = NULL;
		}
		n = NULL;
		break;
	case SIOCGNATS :
		MUTEX_DOWNGRADE(&ipf_nat);
		nat_stats.ns_table[0] = nat_table[0];
		nat_stats.ns_table[1] = nat_table[1];
		nat_stats.ns_list = nat_list;
		nat_stats.ns_maptable = maptable;
		nat_stats.ns_nattab_sz = ipf_nattable_sz;
		nat_stats.ns_rultab_sz = ipf_natrules_sz;
		nat_stats.ns_rdrtab_sz = ipf_rdrrules_sz;
		nat_stats.ns_hostmap_sz = ipf_hostmap_sz;
		nat_stats.ns_instances = nat_instances;
		nat_stats.ns_apslist = ap_sess_list;
		error = IWCOPYPTR((char *)&nat_stats, (char *)data,
				  sizeof(nat_stats));
		break;
	case SIOCGNATL :
	    {
		natlookup_t nl;

		MUTEX_DOWNGRADE(&ipf_nat);
		error = IRCOPYPTR((char *)data, (char *)&nl, sizeof(nl));
		if (error)
			break;

		if (nat_lookupredir(&nl)) {
			error = IWCOPYPTR((char *)&nl, (char *)data,
					  sizeof(nl));
		} else
			error = ESRCH;
		break;
	    }
	case SIOCIPFFL :	/* old SIOCFLNAT & SIOCCNATL */
		if (!(mode & FWRITE)) {
			error = EPERM;
			break;
		}
		error = 0;
		if (arg == 0)
			ret = nat_flushtable();
		else if (arg == 1)
			ret = nat_clearlist();
		else
			error = EINVAL;
		MUTEX_DOWNGRADE(&ipf_nat);
		if (!error) {
			error = IWCOPY((caddr_t)&ret, data, sizeof(ret));
			if (error)
				error = EFAULT;
		}
		break;
	case SIOCSTLCK :
		error = IRCOPY(data, (caddr_t)&arg, sizeof(arg));
		if (!error) {
			error = IWCOPY((caddr_t)&fr_nat_lock, data,
					sizeof(fr_nat_lock));
			if (!error)
				fr_nat_lock = arg;
		} else
			error = EFAULT;
		break;
#if 0 /* 28/07/04 Roger,  */	
	case SIOCSTPUT :
		if (fr_nat_lock)
			error = fr_natputent(data);
		else
			error = EACCES;
		break;
	case SIOCSTGSZ :
		if (fr_nat_lock)
			error = fr_natgetsz(data);
		else
			error = EACCES;
		break;
	case SIOCSTGET :
		if (fr_nat_lock)
			error = fr_natgetent(data);
		else
			error = EACCES;
		break;
#endif // 0
	case FIONREAD :
#ifdef	IPFILTER_LOG
		arg = (int)iplused[IPL_LOGNAT];
		MUTEX_DOWNGRADE(&ipf_nat);
		error = IWCOPY((caddr_t)&arg, (caddr_t)data, sizeof(arg));
		if (error)
			error = EFAULT;
#endif
		break;
	default :
		error = EINVAL;
		break;
	}
	if (getlock == 1) {
		RWLOCK_EXIT(&ipf_nat);			/* READ/WRITE */
	}
done:
	if (nt)
		KFREE(nt);
	return error;
}

#if 0 /* 28/07/04 Roger,  */	
static int fr_natgetsz(data)
caddr_t data;
{
	ap_session_t *aps;
	nat_t *nat, *n;
	int error = 0;
	natget_t ng;

	error = IRCOPY(data, (caddr_t)&ng, sizeof(ng));
	if (error)
		return EFAULT;

	nat = ng.ng_ptr;
	if (!nat) {
		nat = nat_instances;
		ng.ng_sz = 0;
		if (nat == NULL) {
			error = IWCOPY((caddr_t)&ng, data, sizeof(ng));
			if (error)
				error = EFAULT;
			return error;
		}
	} else {
		/*
		 * Make sure the pointer we're copying from exists in the
		 * current list of entries.  Security precaution to prevent
		 * copying of random kernel data.
		 */
		for (n = nat_instances; n; n = n->nat_next)
			if (n == nat)
				break;
		if (!n)
			return ESRCH;
	}

	ng.ng_sz = sizeof(nat_save_t);
	aps = nat->nat_aps;
	if ((aps != NULL) && (aps->aps_data != 0)) {
		ng.ng_sz += sizeof(ap_session_t);
		ng.ng_sz += aps->aps_psiz;
	}

	error = IWCOPY((caddr_t)&ng, data, sizeof(ng));
	if (error)
		error = EFAULT;
	return error;
}


static int fr_natgetent(data)
caddr_t data;
{
	nat_save_t ipn, *ipnp, *ipnn = NULL;
	register nat_t *n, *nat;
	ap_session_t *aps;
	int error;

	error = IRCOPY(data, (caddr_t)&ipnp, sizeof(ipnp));
	if (error)
		return EFAULT;
	error = IRCOPY((caddr_t)ipnp, (caddr_t)&ipn, sizeof(ipn));
	if (error)
		return EFAULT;

	nat = ipn.ipn_next;
	if (!nat) {
		nat = nat_instances;
		if (nat == NULL) {
			if (nat_instances == NULL)
				return ENOENT;
			return 0;
		}
	} else {
		/*
		 * Make sure the pointer we're copying from exists in the
		 * current list of entries.  Security precaution to prevent
		 * copying of random kernel data.
		 */
		for (n = nat_instances; n; n = n->nat_next)
			if (n == nat)
				break;
		if (!n)
			return ESRCH;
	}

	ipn.ipn_next = nat->nat_next;
	ipn.ipn_dsize = 0;
	bcopy((char *)nat, (char *)&ipn.ipn_nat, sizeof(ipn.ipn_nat));
	ipn.ipn_nat.nat_data = NULL;

	if (nat->nat_ptr) {
		bcopy((char *)nat->nat_ptr, (char *)&ipn.ipn_ipnat,
		      sizeof(ipn.ipn_ipnat));
	}

	if (nat->nat_fr)
		bcopy((char *)nat->nat_fr, (char *)&ipn.ipn_rule,
		      sizeof(ipn.ipn_rule));

	if ((aps = nat->nat_aps)) {
		ipn.ipn_dsize = sizeof(*aps);
		if (aps->aps_data)
			ipn.ipn_dsize += aps->aps_psiz;
		KMALLOCS(ipnn, nat_save_t *, sizeof(*ipnn) + ipn.ipn_dsize);
		if (ipnn == NULL)
			return ENOMEM;
		bcopy((char *)&ipn, (char *)ipnn, sizeof(ipn));

		bcopy((char *)aps, (char *)ipnn->ipn_data, sizeof(*aps));
		if (aps->aps_data) {
			bcopy(aps->aps_data, ipnn->ipn_data + sizeof(*aps),
			      aps->aps_psiz);
			ipnn->ipn_dsize += aps->aps_psiz;
		}
		error = IWCOPY((caddr_t)ipnn, ipnp,
			       sizeof(ipn) + ipn.ipn_dsize);
		if (error)
			error = EFAULT;
		KFREES(ipnn, sizeof(*ipnn) + ipn.ipn_dsize);
	} else {
		error = IWCOPY((caddr_t)&ipn, ipnp, sizeof(ipn));
		if (error)
			error = EFAULT;
	}
	return error;
}


static int fr_natputent(data)
caddr_t data;
{
	nat_save_t ipn, *ipnp, *ipnn = NULL;
	register nat_t *n, *nat;
	ap_session_t *aps;
	frentry_t *fr;
	ipnat_t *in;

	int error;

	error = IRCOPY(data, (caddr_t)&ipnp, sizeof(ipnp));
	if (error)
		return EFAULT;
	error = IRCOPY((caddr_t)ipnp, (caddr_t)&ipn, sizeof(ipn));
	if (error)
		return EFAULT;
	nat = NULL;
	if (ipn.ipn_dsize) {
		KMALLOCS(ipnn, nat_save_t *, sizeof(ipn) + ipn.ipn_dsize);
		if (ipnn == NULL)
			return ENOMEM;
		bcopy((char *)&ipn, (char *)ipnn, sizeof(ipn));
		error = IRCOPY((caddr_t)ipnp, (caddr_t)ipn.ipn_data,
			       ipn.ipn_dsize);
		if (error) {
			error = EFAULT;
			goto junkput;
		}
	} else
		ipnn = NULL;

	KMALLOC(nat, nat_t *);
	if (nat == NULL) {
		error = EFAULT;
		goto junkput;
	}

	bcopy((char *)&ipn.ipn_nat, (char *)nat, sizeof(*nat));
	/*
	 * Initialize all these so that nat_delete() doesn't cause a crash.
	 */
	nat->nat_phnext[0] = NULL;
	nat->nat_phnext[1] = NULL;
	fr = nat->nat_fr;
	nat->nat_fr = NULL;
	aps = nat->nat_aps;
	nat->nat_aps = NULL;
	in = nat->nat_ptr;
	nat->nat_ptr = NULL;
	nat->nat_hm = NULL;
	nat->nat_data = NULL;
	//nat->nat_ifp = GETUNIT(nat->nat_ifname, 4);

	/*
	 * Restore the rule associated with this nat session
	 */
	if (in) {
		KMALLOC(in, ipnat_t *);
		if (in == NULL) {
			error = ENOMEM;
			goto junkput;
		}
		nat->nat_ptr = in;
		bcopy((char *)&ipn.ipn_ipnat, (char *)in, sizeof(*in));
		in->in_use = 1;
		in->in_flags |= IPN_DELETE;
		in->in_next = NULL;
		in->in_rnext = NULL;
		in->in_prnext = NULL;
		in->in_mnext = NULL;
		in->in_pmnext = NULL;
		in->in_ifp = GETUNIT(in->in_ifname, 4);
		if (in->in_plabel[0] != '\0') {
			in->in_apr = appr_lookup(in->in_p, in->in_plabel);
		}
	}

	/*
	 * Restore ap_session_t structure.  Include the private data allocated
	 * if it was there.
	 */
	if (aps) {
		KMALLOC(aps, ap_session_t *);
		if (aps == NULL) {
			error = ENOMEM;
			goto junkput;
		}
		nat->nat_aps = aps;
		aps->aps_next = ap_sess_list;
		ap_sess_list = aps;
		bcopy(ipnn->ipn_data, (char *)aps, sizeof(*aps));
		if (in)
			aps->aps_apr = in->in_apr;
		if (aps->aps_psiz) {
			KMALLOCS(aps->aps_data, void *, aps->aps_psiz);
			if (aps->aps_data == NULL) {
				error = ENOMEM;
				goto junkput;
			}
			bcopy(ipnn->ipn_data + sizeof(*aps), aps->aps_data,
			      aps->aps_psiz);
		} else {
			aps->aps_psiz = 0;
			aps->aps_data = NULL;
		}
	}

	/*
	 * If there was a filtering rule associated with this entry then
	 * build up a new one.
	 */
	if (fr != NULL) {
		if (nat->nat_flags & FI_NEWFR) {
			KMALLOC(fr, frentry_t *);
			nat->nat_fr = fr;
			if (fr == NULL) {
				error = ENOMEM;
				goto junkput;
			}
			bcopy((char *)&ipn.ipn_fr, (char *)fr, sizeof(*fr));
			ipn.ipn_nat.nat_fr = fr;
			error = IWCOPY((caddr_t)&ipn, ipnp, sizeof(ipn));
			if (error) {
				error = EFAULT;
				goto junkput;
			}
		} else {
			for (n = nat_instances; n; n = n->nat_next)
				if (n->nat_fr == fr)
					break;
			if (!n) {
				error = ESRCH;
				goto junkput;
			}
		}
	}

	if (ipnn)
		KFREES(ipnn, sizeof(ipn) + ipn.ipn_dsize);
	nat_insert(nat);
	return 0;
junkput:
	if (ipnn)
		KFREES(ipnn, sizeof(ipn) + ipn.ipn_dsize);
	if (nat)
		nat_delete(nat);
	return error;
}
#endif // 0

/*
 * Delete a nat entry from the various lists and table.
 */
static void nat_delete(natd)
struct nat *natd;
{
	struct ipnat *ipn;

	if (natd->nat_flags & FI_WILDP)
		nat_stats.ns_wilds--;
	if (natd->nat_hnext[0])
		natd->nat_hnext[0]->nat_phnext[0] = natd->nat_phnext[0];
	*natd->nat_phnext[0] = natd->nat_hnext[0];
	if (natd->nat_hnext[1])
		natd->nat_hnext[1]->nat_phnext[1] = natd->nat_phnext[1];
	*natd->nat_phnext[1] = natd->nat_hnext[1];
	//if (natd->nat_me != NULL)
	//	*natd->nat_me = NULL;

	if (natd->nat_fr != NULL) {
		ATOMIC_DEC32(natd->nat_fr->fr_ref);
	}

	if (natd->nat_hm != NULL)
		nat_hostmapdel(natd->nat_hm);

	/*
	 * If there is an active reference from the nat entry to its parent
	 * rule, decrement the rule's reference count and free it too if no
	 * longer being used.
	 */
	ipn = natd->nat_ptr;
	if (ipn != NULL) {
		ipn->in_space++;
		ipn->in_use--;
		if (!ipn->in_use && (ipn->in_flags & IPN_DELETE)) {
			if (ipn->in_apr)
				appr_free(ipn->in_apr);
			/* 27/01/04 Roger ++,  */	
			if (ipn->in_mport)
				KFREE(ipn->in_mport);
			/* 27/01/04 Roger --,  */	
			KFREE(ipn);
			nat_stats.ns_rules--;
		}
	}

	MUTEX_DESTROY(&natd->nat_lock);
	/*
	 * If there's a fragment table entry too for this nat entry, then
	 * dereference that as well.
	 */
	ipfr_forget((void *)natd);
	aps_free(natd->nat_aps);
	nat_stats.ns_inuse--;
	if(natd->nat_rt)
		rtfree(natd->nat_rt);
	if(natd->nat_ort)
		rtfree(natd->nat_ort);
	//KFREE(natd);
	nat_free(natd);
}


/*
 * nat_flushtable - clear the NAT table of all mapping entries.
 * (this is for the dynamic mappings)
 */
static int nat_flushtable()
{
	register nat_t *nat, **natp;
	register int j = 0;

	/*
	 * ALL NAT mappings deleted, so lets just make the deletions
	 * quicker.
	 */
	if (nat_table[0] != NULL)
		bzero((char *)nat_table[0],
		      sizeof(nat_table[0]) * ipf_nattable_sz);
	if (nat_table[1] != NULL)
		bzero((char *)nat_table[1],
		      sizeof(nat_table[1]) * ipf_nattable_sz);

	for (natp = &nat_instances; (nat = *natp); ) {
		*natp = nat->nat_next;
#ifdef	IPFILTER_LOG
		nat_log(nat, NL_FLUSH);
#endif
		nat_delete(nat);
		j++;
	}
	nat_stats.ns_inuse = 0;
	return j;
}


int nat_sync_ifdown(struct ifnet *ifp)
{
	register nat_t *nat;
	register int j = 0;
	
	for (nat = nat_instances; nat!= NULL;  nat=nat->nat_next) {
		if (ifp ==nat->nat_ifp) {
			diag_printf("natsync: del (%08x/%d)->(%08x,%d)->(%08x,%d)\n", nat->nat_inip.s_addr, nat->nat_inport, nat->nat_outip.s_addr, nat->nat_outport, nat->nat_oip.s_addr, nat->nat_oport);
			nat->nat_flags |= IPN_DELETE;
			nat->nat_age = 1;
			j++;
		}
	}
	return j;
}

void nat_fwsync_down(void)
{
	register nat_t *nat;
	
	for (nat = nat_instances; nat!= NULL;  nat=nat->nat_next)
	{
		nat->nat_fstate.rule = NULL;
	}
}

/*
 * nat_clearlist - delete all rules in the active NAT mapping list.
 * (this is for NAT/RDR rules)
 */
int nat_clearlist()
{
	register ipnat_t *n, **np = &nat_list;
	int i = 0;

	if (nat_rules != NULL)
		bzero((char *)nat_rules, sizeof(*nat_rules) * ipf_natrules_sz);
	if (rdr_rules != NULL)
		bzero((char *)rdr_rules, sizeof(*rdr_rules) * ipf_rdrrules_sz);

	while ((n = *np)) {
		*np = n->in_next;
		if (!n->in_use) {
			if (n->in_apr)
				appr_free(n->in_apr);
			/* 27/01/04 Roger ++,  */	
			if (n->in_mport)
				KFREE(n->in_mport);
			/* 27/01/04 Roger --,  */
			if(n->in_flags & IPN_DEFAULT_RULE)
				nat_default_rule = NULL;
			KFREE(n);
			nat_stats.ns_rules--;
		} else {
			n->in_flags |= IPN_DELETE;
			n->in_next = NULL;
		}
		i++;
	}
	nat_masks = 0;
	rdr_masks = 0;
	trig_rules = NULL;
	return i;
}

/*
 * Find public port when rdr a packet which response an incoming 
 * packet that is from a virtual server mapping
 */
u_short get_public_port(inb, fin)
struct in_addr inb;
fr_info_t *fin;
{
	register ipnat_t *np;
	u_int hv, msk;
	u_32_t iph;
	struct ifnet *ifp = fin->fin_ifp;
	u_short pubport = 0;
	
	//Search matched rdr rule
	msk = 0xffffffff;
	iph = inb.s_addr & htonl(msk);
	hv = NAT_HASH_FN(iph, 0, ipf_rdrrules_sz);

	for (np = rdr_rules[hv]; np; np = np->in_rnext) 
	{
		if ((np->in_ifp && (np->in_ifp != ifp)) ||
			(np->in_p && (np->in_p != fin->fin_p)))
			continue;

		if(np->in_flags & IPN_INACTIVE_SCHED)
			continue;

		if(((np->in_flags & IPN_TRIG) ^ np->in_triguse))
			continue;

		if (fin->fin_saddr != np->in_inip)
			continue;
		
		if (np->in_pnext == 0)
		{
			// DMZ
			pubport = fin->fin_data[0];
			break;
		}
		else
		{
			// Virtual server and port mappings
			int temp;
			
			temp = fin->fin_data[0] - ntohs(np->in_pnext) + ntohs(np->in_pmin);
			if ((temp >= ntohs(np->in_pmin)) && (temp <= ntohs(np->in_pmax)))
			{
				pubport = temp;
				break;
			}
		} 
	}

	return htons(pubport);
}

#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
/*
 *  Get the mapped port of an outbound packet for a given source IP/port pair.
 */
u_short nat_get_mappedport(fr_info_t *fin, struct in_addr src)
{
	u_short sport;
	nat_t *nat;
	u_int hv;
	u_32_t srcip;
	void *ifp;
	
	ifp = fin->fin_ifp;
	srcip = src.s_addr;
	sport = htons(fin->fin_data[0]);
	
	hv = NAT_HASH_FN(srcip, sport, ipf_nattable_sz);
// diag_printf("nat: ip=%08x, port=%d, hv=%d\n", htonl(srcip), fin->fin_data[0], hv);
	
	for (nat = nat_table[0][hv]; nat; nat = nat->nat_hnext[0]) {
// diag_printf("nat=%08x, inip=%08x, inport=%d, oip=%08x, oport=%d, pro=%d, mport=%d\n",
//		(u_32_t) nat, ntohl(nat->nat_inip.s_addr), ntohs(nat->nat_inport), ntohl(nat->nat_oip.s_addr), ntohs(nat->nat_oport), nat->nat_p, ntohs(nat->nat_outport));
		if ((!ifp || ifp == nat->nat_ifp) &&
		    nat->nat_inip.s_addr == srcip &&
		     nat->nat_inport == sport && 
		     ((nat->nat_p == IPPROTO_TCP) || (nat->nat_p == IPPROTO_UDP))) {
// diag_printf("nat: mport=%d\n", ntohs(nat->nat_outport));
		
		     return nat->nat_outport;
		}
	}
// diag_printf("nat: mport=unknown\n");

	return 0;
}

#define NAT_PORT_COLLISION_TRYCOUNT		512
#define NAT_PORT_AVOID_RDR_BLOCK_SIZE		4096
/*
 *  check if the mapped ip/port already exist
 */
int nat_check_port_collision(u_32_t sip, u_short sport, u_32_t mip, u_short mport)
{
	nat_t *nat;
	u_int hv;
	ipnat_t *np;
	u_short pmax, pmin, mport_h;	

//	NATDBG("nat: colchk, mip=%08x, mp=%d, sip=%08x, sp=%d\n", ntohl(mip), ntohs(mport), ntohl(sip), ntohs(sport));

	mport_h = htons(mport);

	/*  Check if the port is occupied by the RDR  mapping  */
	hv = NAT_HASH_FN(mip, 0, ipf_rdrrules_sz);
	for (np=rdr_rules[hv]; np; np=np->in_rnext) {
		if (!(np->in_flags & IPN_TCPUDP))
			continue;
		
		if (np->in_flags & IPN_INACTIVE_SCHED)
			continue;
		if (((np->in_flags & IPN_TRIG) ^ np->in_triguse))
			continue;
		if (np->in_pnext == 0)
			continue;
		pmax = ntohs(np->in_pmax);
		pmin = ntohs(np->in_pmin);

		if (mport_h < pmin || mport_h > pmax)
			continue;
		if ((pmax - pmin) > NAT_PORT_AVOID_RDR_BLOCK_SIZE)
			continue;
		if ((sip != np->in_inip) || ((sport-np->in_pnext) != (mport-np->in_pmin))) {
#if 1
			NATDBG("nat: rdr port collision, mip=%08x, mp=%d, sip=%08x, sp=%d\n", ntohl(mip), ntohs(mport), ntohl(sip), ntohs(sport));
			NATDBG("rdr: inip=%08x, outip=%08x, srcip=%08x, \n"
				"pnext=%d, pmin=%d, pmax=%d,flag=%x\n", 
				ntohl(np->in_inip), ntohl(np->in_outip), ntohl(np->in_srcip), 
				ntohs(np->in_pnext), ntohs(np->in_pmin), ntohs(np->in_pmax), np->in_flags);
#endif
			return 1;
		}
	}

	/*  Check if the port is occupied by the other mapping  */
	hv = NAT_HASH_FN(mip, mport, ipf_nattable_sz);
	for (nat = nat_table[1][hv]; nat; nat = nat->nat_hnext[1]) {
		if ((nat->nat_outip.s_addr == mip) && (nat->nat_outport == mport) 
			&& ((nat->nat_p == IPPROTO_TCP) || (nat->nat_p == IPPROTO_UDP))
			&& ((nat->nat_inip.s_addr != sip) || (nat->nat_inport != sport))) {
				NATDBG("nat: port collision, mip=%08x, mp=%d, inip=%08x, inp=%d, sip=%08x, sp=%d\n", 
						ntohl(nat->nat_outip.s_addr), ntohs(nat->nat_outport), 
						ntohl(nat->nat_inip.s_addr), ntohs(nat->nat_inport), 
						ntohl(sip), ntohs(sport));
				return 1;
		}
	}
	
	return 0;
}
#endif	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */



/*
 * Create a new NAT table entry.
 * NOTE: Assumes write lock on ipf_nat has been obtained already.
 *       If you intend on changing this, beware: appr_new() may call nat_new()
 *       recursively!
 */
nat_t *nat_new(fin, ip, np, natsave, flags, direction)
fr_info_t *fin;
ip_t *ip;
ipnat_t *np;
nat_t **natsave;
u_int flags;
int direction;
{
	register u_32_t sum1, sum2, sumd, l;
	u_short port = 0, sport = 0, dport = 0, nport = 0;
	struct in_addr in, inb;
	//u_short nflags, sp, dp; /* 10/01/04 Roger,  */	
	u_short sp, dp;
	u_int nflags;
	icmphdr_t *icmp = NULL; /* 10/01/04 Roger,  add*/	
	tcphdr_t *tcp = NULL;
	hostmap_t *hm = NULL;
	nat_t *nat, *natl;
	ipnat_t *tnp; /* 11/11/03 Roger,  */
	aproxy_t *apr=NULL;
	int append=0;
	
	nflags = flags & np->in_flags;
	if (flags & IPN_TCPUDP) {
		tcp = (tcphdr_t *)fin->fin_dp;
		sport = htons(fin->fin_data[0]);
		dport = htons(fin->fin_data[1]);
	/* 10/01/04 Roger ++,  */	
	} else if (flags & IPN_ICMPQUERY) {
		icmp = (icmphdr_t *)fin->fin_dp;

		/*
		 * In the ICMP query NAT code, we translate the ICMP id fields
		 * to make them unique. This is indepedent of the ICMP type 
		 * (e.g. in the unlikely event that a host sends an echo and 
		 * an tstamp request with the same id, both packets will have 
		 * their ip address/id field changed in the same way).
		 */
		/* The icmp_id field is used by the sender to identify the
		 * process making the icmp request. (the receiver justs 
		 * copies it back in its response). So, it closely matches 
		 * the concept of source port. We overlay sport, so we can 
		 * maximally reuse the existing code.
		 */
		if(direction == NAT_OUTBOUND)
		{
			sport = icmp->icmp_id;
			dport = 0;
		}
		else
		{
			sport = 0;
			dport = icmp->icmp_id;
		}
		/* 10/01/04 Roger --,  */
	}	

	/* Give me a new nat */
	//KMALLOC(nat, nat_t *);
	nat = nat_alloc(direction, flags);
	if (nat == NULL) {
		nat_stats.ns_memfail++;
		//diag_printf("No memory for NAT entry!!\n");
		return NULL;
	}
	//bzero((char *)nat, sizeof(*nat));
	nat->nat_flags = flags|(np->in_flags & IPN_DMZ);
	apr = appr_find(fin->fin_p, fin->fin_data[1]);
	if(apr && apr->apr_enable == 0) 
		apr = NULL;
#ifdef CONFIG_SUPERDMZ
	if(superdmz_mac && (nat->nat_flags & IPN_DMZ))
		apr = NULL;
#endif
	/*
	 * Search the current table for a match.
	 */
	if (direction == NAT_OUTBOUND) {
		/*
		 * Values at which the search for a free resouce starts.
		 */
		u_32_t st_ip;
		u_short st_port;

		/*
		 * If it's an outbound packet which doesn't match any existing
		 * record, then create a new port
		 */
		l = 0;
		st_ip = np->in_nip;
		st_port = np->in_pnext;

		do {
			port = 0;
			in.s_addr = htonl(np->in_nip);
			if (l == 0) {
				/*
				 * Check to see if there is an existing NAT
				 * setup for this IP address pair.
				 */
				hm = nat_hostmap(np, fin->fin_src, in);
				if (hm != NULL)
					in.s_addr = hm->hm_mapip.s_addr;
			} else if ((l == 1) && (hm != NULL)) {
				nat_hostmapdel(hm);
				hm = NULL;
			}
			in.s_addr = ntohl(in.s_addr);

			nat->nat_hm = hm;
			if ((hm!= NULL) && ((hm->hm_ref >= hm_nat_sess_limit) 
				|| (nat_stats.ns_inuse >= nat_sess_himark && hm->hm_ref >= nat_table_max/2))) {
//				diag_printf("nat: too many session for %08x, sess=%d\n", hm->hm_realip.s_addr, hm->hm_ref);
				goto badnat;
			}

			if ((np->in_outmsk == 0xffffffff) &&
			    (np->in_pnext == 0)) {
				if (l > 0)
					goto badnat;
			}

			if (np->in_redir & NAT_MAPBLK) {
				if ((l >= np->in_ppip) || ((l > 0) &&
				     !(flags & IPN_TCPUDP)))
					goto badnat;
				/*
				 * map-block - Calculate destination address.
				 */
				in.s_addr = ntohl(fin->fin_saddr);
				in.s_addr &= ntohl(~np->in_inmsk);
				inb.s_addr = in.s_addr;
				in.s_addr /= np->in_ippip;
				in.s_addr &= ntohl(~np->in_outmsk);
				in.s_addr += ntohl(np->in_outip);
				/*
				 * Calculate destination port.
				 */
				if ((flags & IPN_TCPUDP) &&
				    (np->in_ppip != 0)) {
					port = ntohs(sport) + l;
					port %= np->in_ppip;
					port += np->in_ppip *
						(inb.s_addr % np->in_ippip);
					port += MAPBLK_MINPORT;
					port = htons(port);
				}
			} else if (!np->in_outip &&
				   (np->in_outmsk == 0xffffffff)) {
				/*
				 * 0/32 - use the interface's IP address.
				 */
				//if ((l > 0) ||
				//    fr_ifpaddr(4, fin->fin_ifp, &in) == -1)
				/* 07/04/05 Roger,  specify interface */	
				if ((l > 0) || fr_ifpaddr(4, np->in_ifp, &in) == -1)
					goto badnat;
				in.s_addr = ntohl(in.s_addr);
			} else if (!np->in_outip && !np->in_outmsk) {
				/*
				 * 0/0 - use the original source address/port.
				 */
				if (l > 0)
					goto badnat;
				in.s_addr = ntohl(fin->fin_saddr);
			} else if ((np->in_outmsk != 0xffffffff) &&
				   (np->in_pnext == 0) &&
				   ((l > 0) || (hm == NULL)))
				np->in_nip++;
			natl = NULL;
			
			if ((nflags & IPN_TCPUDP) &&
			    ((np->in_redir & NAT_MAPBLK) == 0) &&
			    (np->in_flags & IPN_AUTOPORTMAP)) {
				if ((l > 0) && (l % np->in_ppip == 0)) {
					if (l > np->in_space) {
						goto badnat;
					} else if ((l > np->in_ppip) &&
						   np->in_outmsk != 0xffffffff)
						np->in_nip++;
				}
				if (np->in_ppip != 0) {
					inb.s_addr = htonl(in.s_addr);
#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
					if ((l == 0) && (((port = get_public_port(inb, fin)) != 0) 
						|| ((port = nat_get_mappedport(fin, fin->fin_src)) != 0)))
#else
					if ((l == 0) && ((port = get_public_port(inb, fin)) != 0)) 
#endif
					{
						/*  First pass: a public port or a mapped port has been found  */
					} else  {

						do {
							port = ntohs(sport)+ntohs(dport);
							port += (l % np->in_ppip);
							//port += np->in_ppip *
							//	((ntohl(fin->fin_saddr)+ntohl(fin->fin_daddr)) %
							//	 np->in_ippip);
							port += (np->in_ippip * (ntohl(fin->fin_saddr)+ntohl(fin->fin_daddr))) % np->in_ppip;
							port %= USABLE_PORTS;
							port += MAPBLK_MINPORT;
							port = htons(port);
						}
#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
						 while (nat_check_port_collision(fin->fin_src.s_addr, sport, inb.s_addr, port) &&
							(l++ <   16*NAT_PORT_COLLISION_TRYCOUNT));
#else	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
						 while (0);
#endif	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
					}
				}
			} else if (((np->in_redir & NAT_MAPBLK) == 0) &&
				   //(nflags & IPN_TCPUDP) && /* 10/01/04 Roger,  */	
				   (nflags & IPN_TCPUDPICMP) &&
				   (np->in_pnext != 0)) {
				/* 15/09/04 Roger ++,  */	
				if(np->in_redir == NAT_REDIRECT)
					port = np->in_pmax;
				else
				{
					/*  Use the virtual server port if the source IP/port is the server  */
					inb.s_addr = htonl(in.s_addr);

#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
					if ((l == 0) && (((port = get_public_port(inb, fin)) != 0) 
						|| ((port = nat_get_mappedport(fin, fin->fin_src)) != 0)))
#else
					if ((l == 0) && ((port = get_public_port(inb, fin)) != 0)) 
#endif
					{
						/*  First pass: a public port or a mapped port has been found  */
					} else {
						do {
						         port = htons(np->in_pnext++);
						}
#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
						 while (nat_check_port_collision(fin->fin_src.s_addr, sport, inb.s_addr, port) &&
								(l++ < 16*NAT_PORT_COLLISION_TRYCOUNT));
#else	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
						 while (0);
#endif	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
					}
					if (np->in_pnext > ntohs(np->in_pmax)) {
						np->in_pnext = ntohs(np->in_pmin);
						if (np->in_outmsk != 0xffffffff)
							np->in_nip++;
					}
				}
				/* 15/09/04 Roger --,  */	
			}

			if (np->in_flags & IPN_IPRANGE) {
				if (np->in_nip > ntohl(np->in_outmsk))
					np->in_nip = ntohl(np->in_outip);
			} else {
				if ((np->in_outmsk != 0xffffffff) &&
				    ((np->in_nip + 1) & ntohl(np->in_outmsk)) >
				    ntohl(np->in_outip))
					np->in_nip = ntohl(np->in_outip) + 1;
			}

			//if (!port && (flags & IPN_TCPUDP)) /* 10/01/04 Roger,  */	
			if ((!port && (flags & IPN_TCPUDPICMP))||(apr && (apr->apr_flags == APR_DONOTCHANGEPORT)))
				port = sport;
			/*
			 * Here we do a lookup of the connection as seen from
			 * the outside.  If an IP# pair already exists, try
			 * again.  So if you have A->B becomes C->B, you can
			 * also have D->E become C->E but not D->B causing
			 * another C->B.  Also take protocol and ports into
			 * account when determining whether a pre-existing
			 * NAT setup will cause an external conflict where
			 * this is appropriate.
			 */
			inb.s_addr = htonl(in.s_addr);
			sp = fin->fin_data[0];
			dp = fin->fin_data[1];
			fin->fin_data[0] = fin->fin_data[1];
			fin->fin_data[1] = htons(port);
			if(apr && (apr->apr_flags & APR_WILDDST))
				natl = nat_inlookup(fin, flags | (FI_WILDP|FI_WILDA), (u_int)fin->fin_p, fin->fin_dst, inb, 0);
			else
				natl = nat_inlookup(fin, flags & ~(FI_WILDP|FI_WILDA), 
									(u_int)fin->fin_p, fin->fin_dst, inb, 0);
			fin->fin_data[0] = sp;
			fin->fin_data[1] = dp;

			/*
			 * Has the search wrapped around and come back to the
			 * start ?
			 */
			if ((natl != NULL) &&
			    (np->in_pnext != 0) && (st_port == np->in_pnext) &&
			    (np->in_nip != 0) && (st_ip == np->in_nip))
				goto badnat;
			l++;
		} while (natl != NULL);

		if (np->in_space > 0)
			np->in_space--;
		/* 11/11/03 Roger,  scan trigger port*/	
		for(tnp = trig_rules ; tnp ; tnp = tnp->in_trignext) {
			if((tnp->in_trigp & nflags) && (ntohs(dport) >= tnp->in_trigport[0]) && (ntohs(dport) <= tnp->in_trigport[1])) {
				tnp->in_triguse = IPN_TRIG;
				tnp->in_inip = fin->fin_saddr;
				tnp->in_inmsk = 0xffffffff;
				tnp->in_trigage = 1200;
			}
		}
		/* Setup the NAT table */
		nat->nat_inip = fin->fin_src;
		nat->nat_outip.s_addr = htonl(in.s_addr);
		nat->nat_oip = fin->fin_dst;
		if (nat->nat_hm == NULL)
			nat->nat_hm = nat_hostmap(np, fin->fin_src,
						  nat->nat_outip);
		/* 10/01/04 Roger ++,  */	
		//sum1 = LONG_SUM(ntohl(fin->fin_saddr)) + ntohs(sport);
		//sum2 = LONG_SUM(in.s_addr) + ntohs(port);
		/* 
		 * The ICMP checksum does not have a pseudo header containing
		 * the IP addresses 
		 */
		
		if (flags & IPN_ICMPQUERY) {
			sum1 = ntohs(sport);
			sum2 = ntohs(port);
		} else {
			//sum1 = LONG_SUM(ntohl(ip->ip_src.s_addr)) + ntohs(sport);
			sum1 = LONG_SUM(ntohl(fin->fin_saddr)) + ntohs(sport);
			sum2 = LONG_SUM(in.s_addr) + ntohs(port);
		};
		/* 10/01/04 Roger --,  */	
		//if (flags & IPN_TCPUDP) {
		if (flags & IPN_TCPUDPICMP) { /* 10/01/04 Roger,  */	
			nat->nat_inport = sport;
			nat->nat_outport = port;	/* sport */
			nat->nat_oport = dport;
		}
	} else {
		/*
		 * Otherwise, it's an inbound packet. Most likely, we don't
		 * want to rewrite source ports and source addresses. Instead,
		 * we want to rewrite to a fixed internal address and fixed
		 * internal port.
		 */
		if (np->in_flags & IPN_SPLIT) {
			in.s_addr = np->in_nip;
			if (np->in_inip == htonl(in.s_addr))
				np->in_nip = ntohl(np->in_inmsk);
			else {
				np->in_nip = ntohl(np->in_inip);
				if (np->in_flags & IPN_ROUNDR) {
					nat_delrdr(np);
					nat_addrdr(np);
				}
			}
		} else {
			in.s_addr = ntohl(np->in_inip);
			if (np->in_flags & IPN_ROUNDR) {
				nat_delrdr(np);
				nat_addrdr(np);
			}
		}
		if ((!np->in_pnext) || ((flags & NAT_NOTRULEPORT) != 0))
			nport = dport;
		else {
			/*
			 * Whilst not optimized for the case where
			 * pmin == pmax, the gain is not significant.
			 */
			if (np->in_pmin != np->in_pmax) {
				nport = ntohs(dport) - ntohs(np->in_pmin) +
					ntohs(np->in_pnext);
				nport = ntohs(nport);
			} else
				nport = np->in_pnext;
		}

		/*
		 * When the redirect-to address is set to 0.0.0.0, just
		 * assume a blank `forwarding' of the packet.
		 */
		if (in.s_addr == 0)
			in.s_addr = ntohl(fin->fin_daddr);

		nat->nat_inip.s_addr = htonl(in.s_addr);
		nat->nat_outip = fin->fin_dst;
		nat->nat_oip = fin->fin_src;
		
		/* 14/05/04 Roger ++,  */	
		/* 
		 * The ICMP checksum does not have a pseudo header containing
		 * the IP addresses 
		 */
		
		if (flags & IPN_ICMPQUERY) {
			sum1 = dport;
			sum2 = nport;
		} else { /* 14/05/04 Roger --,  */	
			sum1 = LONG_SUM(ntohl(fin->fin_daddr)) + ntohs(dport);
			sum2 = LONG_SUM(in.s_addr) + ntohs(nport);
		}

		if (flags & IPN_TCPUDPICMP) {
			nat->nat_inport = nport;
			nat->nat_outport = dport;
			nat->nat_oport = sport;
		}
	}
	
	CALC_SUMD(sum1, sum2, sumd);
	nat->nat_sumd[0] = (sumd & 0xffff) + (sumd >> 16);

		nat->nat_sumd[1] = nat->nat_sumd[0];
#if 0 /* 27/07/04 Roger,  do not need */	
	//if ((flags & IPN_TCPUDP) && ((sport != port) || (dport != nport))) {
	if ((flags & IPN_TCPUDPICMP) && ((sport != port) || (dport != nport))) { /* 10/01/04 Roger,  */	
		if (direction == NAT_OUTBOUND)
			sum1 = LONG_SUM(ntohl(fin->fin_saddr));
		else
			sum1 = LONG_SUM(ntohl(fin->fin_daddr));

		sum2 = LONG_SUM(in.s_addr);

		CALC_SUMD(sum1, sum2, sumd);
		nat->nat_ipsumd = (sumd & 0xffff) + (sumd >> 16);
	} else
		nat->nat_ipsumd = nat->nat_sumd[0];
#endif
	in.s_addr = htonl(in.s_addr);

	//strncpy(nat->nat_ifname, IFNAME(fin->fin_ifp), IFNAMSIZ);
	//nat->nat_ifp = fin->fin_ifp;
	//nat->nat_me = natsave;
	nat->nat_dir = direction;
	nat->nat_ifp = fin->fin_ifp;
	nat->nat_ptr = np;
	nat->nat_p = fin->fin_p;
	//nat->nat_bytes = 0;
	//nat->nat_pkts = 0;
	nat->nat_mssclamp = np->in_mssclamp;
	nat->nat_fstate.rule = np->ref_fr;
	nat->nat_fr = fin->fin_fr;
	if (nat->nat_fr != NULL) {
		ATOMIC_INC32(nat->nat_fr->fr_ref);
	}
	if (direction == NAT_OUTBOUND) {
		if (flags & IPN_TCPUDP)
			tcp->th_sport = port;
		/* 10/01/04 Roger ++,  */	
		if (flags & IPN_ICMPQUERY)
			icmp->icmp_id = port;
		/* 10/01/04 Roger --,  */	
	} else {
		if (flags & IPN_TCPUDP)
			tcp->th_dport = nport;
		/* 10/01/04 Roger ++,  */	
		if (flags & IPN_ICMPQUERY)
			icmp->icmp_id = nport;
		/* 10/01/04 Roger --,  */	
	}
	
	if(nat->nat_p == IPPROTO_ESP)
		append = 1;
		
	nat_insert(nat, append);
	
	//if ((np->in_apr != NULL) && (np->in_dport == 0 ||
	//    (tcp != NULL && dport == np->in_dport)))
	//	(void) appr_new(fin, nat);
	/* check protocol and destination port to find proxy */
	if ((np->in_apr != NULL) && (np->in_dport == 0 ||
	    (tcp != NULL && dport == np->in_dport)))
	    apr = np->in_apr;
	appr_new(fin, nat, apr);
	
	np->in_use++;
#ifdef	IPFILTER_LOG
	nat_log(nat, (u_int)np->in_redir);
#endif
	nat_set_fcache(fin, nat); /* 04/03/04 Roger,  */	
	return nat;
badnat:
	nat_stats.ns_badnat++;
	if ((hm = nat->nat_hm) != NULL)
		nat_hostmapdel(hm);
	aps_free(nat->nat_aps);
	//KFREE(nat);
	nat_free(nat);
	return NULL;
}

void nat_setup(nat_t *nat, unsigned int srcip, unsigned int dstip, unsigned int nip, unsigned char p, unsigned short sport,  unsigned short dport,  unsigned short nport)
{
	register u_32_t sum1, sum2, sumd;
	int append=0;
	
	nat->nat_p = p;
	
	if(nat->nat_dir == NAT_OUTBOUND)
	{
		nat->nat_inip.s_addr = srcip;
		nat->nat_outip.s_addr = nip;
		nat->nat_oip.s_addr = dstip;
		
		if (nat->nat_flags & IPN_ICMPQUERY) {
			sum1 = ntohs(sport);
			sum2 = ntohs(nport);
		} else {
			sum1 = LONG_SUM(ntohl(srcip)) + ntohs(sport);
			sum2 = LONG_SUM(ntohl(nip)) + ntohs(nport);
		};
		if (nat->nat_flags & IPN_TCPUDPICMP) { 
			nat->nat_inport = sport;
			nat->nat_outport = nport;
			nat->nat_oport = dport;
		}
	}	
	else
	{	
		nat->nat_inip.s_addr = nip;
		nat->nat_outip.s_addr = dstip;
		nat->nat_oip.s_addr = srcip;
		
		if (nat->nat_flags & IPN_ICMPQUERY) {
			sum1 = dport;
			sum2 = nport;
		} else { 	
			sum1 = LONG_SUM(ntohl(dstip)) + ntohs(dport);
			sum2 = LONG_SUM(htonl(nip)) + ntohs(nport);
		}

		if (nat->nat_flags & IPN_TCPUDPICMP) {
			nat->nat_inport = nport;
			nat->nat_outport = dport;
			nat->nat_oport = sport;
		}
	}
	
	CALC_SUMD(sum1, sum2, sumd);
	nat->nat_sumd[0] = (sumd & 0xffff) + (sumd >> 16);
	nat->nat_sumd[1] = nat->nat_sumd[0];
	
	if(p == IPPROTO_ESP)
		append = 1;
	nat_insert(nat, append);
}

/*
 * Insert a NAT entry into the hash tables for searching and add it to the
 * list of active NAT entries.  Adjust global counters when complete.
 */
void	nat_insert(nat, append)
nat_t	*nat;
u_32_t append;
{
	u_int hv1, hv2;
	nat_t **natp;

	MUTEX_INIT(&nat->nat_lock, "nat entry lock", NULL);
	
	nat->nat_age = fr_defnatage;
	//nat->nat_ifname[sizeof(nat->nat_ifname) - 1] = '\0';
	//if (nat->nat_ifname[0] !='\0') {
	//	nat->nat_ifp = GETUNIT(nat->nat_ifname, 4);
	//}

	nat->nat_next = nat_instances;
	nat_instances = nat;
	if ((nat->nat_flags & (FI_W_DADDR|FI_W_DPORT))==(FI_W_DADDR|FI_W_DPORT)) {
		hv1 = NAT_HASH_FN(nat->nat_inip.s_addr, nat->nat_inport, ipf_nattable_sz);
		hv2 = NAT_HASH_FN(nat->nat_outip.s_addr, nat->nat_outport, ipf_nattable_sz);
	}
	else if (nat->nat_flags & (FI_W_SPORT|FI_W_DPORT)) {
#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
		hv1 = NAT_HASH_FN(nat->nat_inip.s_addr, nat->nat_inport, ipf_nattable_sz);
		hv2 = NAT_HASH_FN(nat->nat_outip.s_addr, nat->nat_outport, ipf_nattable_sz);
#else	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
		hv1 = NAT_HASH_FN(nat->nat_inip.s_addr, 0, 0xffffffff);
		hv1 = NAT_HASH_FN(nat->nat_oip.s_addr, hv1, ipf_nattable_sz);
		hv2 = NAT_HASH_FN(nat->nat_outip.s_addr, 0, 0xffffffff);
		hv2 = NAT_HASH_FN(nat->nat_oip.s_addr, hv2, ipf_nattable_sz);
#endif	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
	} 
	else {
#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
		hv1 = NAT_HASH_FN(nat->nat_inip.s_addr, nat->nat_inport, ipf_nattable_sz);
		hv2 = NAT_HASH_FN(nat->nat_outip.s_addr, nat->nat_outport, ipf_nattable_sz);
#else	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
		hv1 = NAT_HASH_FN(nat->nat_inip.s_addr, nat->nat_inport, 0xffffffff);
		hv1 = NAT_HASH_FN(nat->nat_oip.s_addr, hv1 + nat->nat_oport, ipf_nattable_sz);
		hv2 = NAT_HASH_FN(nat->nat_outip.s_addr, nat->nat_outport, 0xffffffff);
		hv2 = NAT_HASH_FN(nat->nat_oip.s_addr, hv2 + nat->nat_oport, ipf_nattable_sz);
#endif	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
	} 

	natp = &nat_table[0][hv1];
	if(append)
	{
		if (*natp) {
			for(;(*natp)->nat_hnext[0]; natp=&((*natp)->nat_hnext[0]));
			(*natp)->nat_hnext[0] = nat;
		
		} else {
			*natp = nat;
		}
		nat->nat_phnext[0] = natp;
	}
	else
	{
		if (*natp)
			(*natp)->nat_phnext[0] = &nat->nat_hnext[0];
		nat->nat_phnext[0] = natp;
		nat->nat_hnext[0] = *natp;
		*natp = nat;
	}
	//diag_printf("nat_insert(hv1:%d), natp:%x, nat:%x\n", hv1, natp, nat); /* 10/01/04 Roger,  */	
	natp = &nat_table[1][hv2];
	if(append)
	{
		if (*natp) {
			for(;(*natp)->nat_hnext[1]; natp=&((*natp)->nat_hnext[1]));
			(*natp)->nat_hnext[1] = nat;
		} else {
			*natp = nat;
		}
		nat->nat_phnext[1] = natp;
	}
	else
	{
		if (*natp)
			(*natp)->nat_phnext[1] = &nat->nat_hnext[1];
		nat->nat_phnext[1] = natp;
		nat->nat_hnext[1] = *natp;
		*natp = nat;
	}
	//diag_printf("nat_insert(hv2:%d) natp:%x, nat:%x\n", hv2, natp, nat); /* 10/01/04 Roger,  */	
	if (nat->nat_flags & (FI_WILDP | FI_WILDA))
		nat_stats.ns_wilds++;
	nat_stats.ns_added++;
	
	nat_stats.ns_inuse++;
	if(nat_stats.ns_inuse > nat_stats.ns_maxuse)
		nat_stats.ns_maxuse = nat_stats.ns_inuse;
}

/* 10/01/04 Roger ++,  */	
//nat_t *nat_icmplookup(ip, fin, dir)
/* 
 * Check if the ICMP error message is related to an existing TCP, UDP or
 * ICMP query nat entry
 */
nat_t *nat_icmperrorlookup(ip, fin, dir)
/* 10/01/04 Roger --,  */	
ip_t *ip;
fr_info_t *fin;
int dir;
{
	//icmphdr_t *icmp;
	icmphdr_t *icmp, *orgicmp; /* 10/01/04 Roger,  */	
	tcphdr_t *tcp = NULL;
	ip_t *oip;
	int flags = 0, type, minlen;

	icmp = (icmphdr_t *)fin->fin_dp;
	/*
	 * Does it at least have the return (basic) IP header ?
	 * Only a basic IP header (no options) should be with an ICMP error
	 * header.
	 */
	if ((ip->ip_hl != 5) || (ip->ip_len < ICMPERR_MINPKTLEN))
		return NULL;
	type = icmp->icmp_type;
	/*
	 * If it's not an error type, then return.
	 */
	/* 10/01/04 Roger ++,  */	
	//if ((type != ICMP_UNREACH) && (type != ICMP_SOURCEQUENCH) &&
	//    (type != ICMP_REDIRECT) && (type != ICMP_TIMXCEED) &&
	//    (type != ICMP_PARAMPROB))
	if (!check_icmperrortype4(type))
	/* 10/01/04 Roger --,  */	
		return NULL;

	oip = (ip_t *)((char *)fin->fin_dp + 8);
	minlen = (oip->ip_hl << 2);
	if (minlen < sizeof(ip_t))
		return NULL;
	if (ip->ip_len < ICMPERR_IPICMPHLEN + minlen)
		return NULL;
	/*
	 * Is the buffer big enough for all of it ?  It's the size of the IP
	 * header claimed in the encapsulated part which is of concern.  It
	 * may be too big to be in this buffer but not so big that it's
	 * outside the ICMP packet, leading to TCP deref's causing problems.
	 * This is possible because we don't know how big oip_hl is when we
	 * do the pullup early in fr_check() and thus can't gaurantee it is
	 * all here now.
	 */
	{
	mb_t *m;


	m = *(mb_t **)fin->fin_mp;
	if ((char *)oip + fin->fin_dlen - ICMPERR_ICMPHLEN >
	    (char *)ip + m->m_len)
		return NULL;
	}

	if (oip->ip_p == IPPROTO_TCP)
		flags = IPN_TCP;
	else if (oip->ip_p == IPPROTO_UDP)
		flags = IPN_UDP;
	/* 10/01/04 Roger ++,  */	
	else if (oip->ip_p == IPPROTO_ICMP) {

		orgicmp = (icmphdr_t *)((char *)oip + (oip->ip_hl << 2));

		/* see if this is related to an ICMP query */
		if (check_icmpquerytype4(orgicmp->icmp_type)) {
			nat_t *nat;
			char *data;
			
			flags = IPN_ICMPQUERY;
			data = fin->fin_dp;
			fin->fin_dp = orgicmp;
			/* NOTE : dir refers to the direction of the original
			 *        ip packet. By definition the icmp error
			 *        message flows in the opposite direction.
			 */
			if (dir == NAT_INBOUND)
				nat = nat_inlookup(fin, flags,
					(u_int)oip->ip_p, oip->ip_dst, oip->ip_src, 0);
			else
				nat = nat_outlookup(fin, flags,
					(u_int)oip->ip_p, oip->ip_dst, oip->ip_src, 0);
			fin->fin_dp = data;	
			return nat;
		}
	}
	/* 10/01/04 Roger --,  */			
	if (flags & IPN_TCPUDP) {
		u_short	data[2];
		nat_t *nat;

		minlen += 8;		/* + 64bits of data to get ports */
		if (ip->ip_len < ICMPERR_IPICMPHLEN + minlen)
			return NULL;

		data[0] = fin->fin_data[0];
		data[1] = fin->fin_data[1];
		tcp = (tcphdr_t *)((char *)oip + (oip->ip_hl << 2));
		fin->fin_data[0] = ntohs(tcp->th_dport);
		fin->fin_data[1] = ntohs(tcp->th_sport);

		if (dir == NAT_INBOUND) {
			nat = nat_inlookup(fin, flags, (u_int)oip->ip_p,
					    oip->ip_dst, oip->ip_src, 0);
		} else {
			nat = nat_outlookup(fin, flags, (u_int)oip->ip_p,
					    oip->ip_dst, oip->ip_src, 0);
		}
		fin->fin_data[0] = data[0];
		fin->fin_data[1] = data[1];
		return nat;
	}
	if (dir == NAT_INBOUND)
		return nat_inlookup(fin, 0, (u_int)oip->ip_p,
				    oip->ip_dst, oip->ip_src, 0);
	else
		return nat_outlookup(fin, 0, (u_int)oip->ip_p,
				    oip->ip_dst, oip->ip_src, 0);
}


/*
 * This should *ONLY* be used for incoming packets to make sure a NAT'd ICMP
 * packet gets correctly recognised.
 */
//nat_t *nat_icmp(ip, fin, nflags, dir)
nat_t *nat_icmperror(ip, fin, nflags, dir) /* 10/01/04 Roger,  */	
ip_t *ip;
fr_info_t *fin;
u_int *nflags;
int dir;
{
	u_32_t sum1, sum2, sumd, sumd2 = 0;
	struct in_addr in;
	int flags, dlen;
	icmphdr_t *icmp;
	udphdr_t *udp;
	tcphdr_t *tcp;
	nat_t *nat;
	ip_t *oip;
		
	if ((fin->fin_fl & FI_SHORT) || (fin->fin_off != 0))
		return NULL;
	/*
	 * nat_icmplookup() will return NULL for `defective' packets.
	 */
	//if ((ip->ip_v != 4) || !(nat = nat_icmplookup(ip, fin, dir)))
	if ((ip->ip_v != 4) || !(nat = nat_icmperrorlookup(ip, fin, dir))) /* 10/01/04 Roger,  */	
		return NULL;
	
	flags = 0;
	*nflags = IPN_ICMPERR;
	icmp = (icmphdr_t *)fin->fin_dp;
	oip = (ip_t *)&icmp->icmp_ip;
	if (oip->ip_p == IPPROTO_TCP)
		flags = IPN_TCP;
	else if (oip->ip_p == IPPROTO_UDP)
		flags = IPN_UDP;
	/* 10/01/04 Roger ++,  */	
	else if (oip->ip_p == IPPROTO_ICMP)
		flags = IPN_ICMPQUERY;
	/* 10/01/04 Roger --,  */	
	udp = (udphdr_t *)((((char *)oip) + (oip->ip_hl << 2)));
	dlen = ip->ip_len - ((char *)udp - (char *)ip);
	/*
	 * XXX - what if this is bogus hl and we go off the end ?
	 * In this case, nat_icmplookup() will have returned NULL.
	 */
	tcp = (tcphdr_t *)udp;

	/*
	 * Need to adjust ICMP header to include the real IP#'s and
	 * port #'s.  Only apply a checksum change relative to the
	 * IP address change as it will be modified again in ip_natout
	 * for both address and port.  Two checksum changes are
	 * necessary for the two header address changes.  Be careful
	 * to only modify the checksum once for the port # and twice
	 * for the IP#.
	 */

	/*
	 * Step 1
	 * Fix the IP addresses in the offending IP packet. You also need
	 * to adjust the IP header checksum of that offending IP packet
	 * and the ICMP checksum of the ICMP error message itself.
	 *
	 * Unfortunately, for UDP and TCP, the IP addresses are also contained
	 * in the pseudo header that is used to compute the UDP resp. TCP
	 * checksum. So, we must compensate that as well. Even worse, the
	 * change in the UDP and TCP checksums require yet another
	 * adjustment of the ICMP checksum of the ICMP error message.
	 *
	 */

	if (oip->ip_dst.s_addr == nat->nat_oip.s_addr) {
		sum1 = LONG_SUM(ntohl(oip->ip_src.s_addr));
		in = nat->nat_inip;
		oip->ip_src = in;
	} else {
		sum1 = LONG_SUM(ntohl(oip->ip_dst.s_addr));
		in = nat->nat_outip;
		oip->ip_dst = in;
	}

	sum2 = LONG_SUM(ntohl(in.s_addr));

	CALC_SUMD(sum1, sum2, sumd);

	if (nat->nat_dir == NAT_OUTBOUND) {
		/*
		 * Fix IP checksum of the offending IP packet to adjust for
		 * the change in the IP address.
		 *
		 * Normally, you would expect that the ICMP checksum of the 
		 * ICMP error message needs to be adjusted as well for the
		 * IP address change in oip.
		 * However, this is a NOP, because the ICMP checksum is 
		 * calculated over the complete ICMP packet, which includes the
		 * changed oip IP addresses and oip->ip_sum. However, these 
		 * two changes cancel each other out (if the delta for
		 * the IP address is x, then the delta for ip_sum is minus x), 
		 * so no change in the icmp_cksum is necessary.
		 *
		 * Be careful that nat_dir refers to the direction of the
		 * offending IP packet (oip), not to its ICMP response (icmp)
		 */
		fix_datacksum(&oip->ip_sum, sumd);

		/*
		 * Fix UDP pseudo header checksum to compensate for the
		 * IP address change.
		 */
		if (oip->ip_p == IPPROTO_UDP && udp->uh_sum) {
			/*
			 * The UDP checksum is optional, only adjust it 
			 * if it has been set.
			 */
			sum1 = ntohs(udp->uh_sum);
			fix_datacksum(&udp->uh_sum, sumd);
			sum2 = ntohs(udp->uh_sum);

			/*
			 * Fix ICMP checksum to compensate the UDP 
			 * checksum adjustment.
			 */
			CALC_SUMD(sum1, sum2, sumd);
			sumd2 = sumd;
		}

		/*
		 * Fix TCP pseudo header checksum to compensate for the 
		 * IP address change. Before we can do the change, we
		 * must make sure that oip is sufficient large to hold
		 * the TCP checksum (normally it does not!).
		 */
		if (oip->ip_p == IPPROTO_TCP && dlen >= 18) {
		
			sum1 = ntohs(tcp->th_sum);
			fix_datacksum(&tcp->th_sum, sumd);
			sum2 = ntohs(tcp->th_sum);

			/*
			 * Fix ICMP checksum to compensate the TCP 
			 * checksum adjustment.
			 */
			CALC_SUMD(sum1, sum2, sumd);
			sumd2 = sumd;
		}
	} else {

		/*
		 * Fix IP checksum of the offending IP packet to adjust for
		 * the change in the IP address.
		 *
		 * Normally, you would expect that the ICMP checksum of the 
		 * ICMP error message needs to be adjusted as well for the
		 * IP address change in oip.
		 * However, this is a NOP, because the ICMP checksum is 
		 * calculated over the complete ICMP packet, which includes the
		 * changed oip IP addresses and oip->ip_sum. However, these 
		 * two changes cancel each other out (if the delta for
		 * the IP address is x, then the delta for ip_sum is minus x), 
		 * so no change in the icmp_cksum is necessary.
		 *
		 * Be careful that nat_dir refers to the direction of the
		 * offending IP packet (oip), not to its ICMP response (icmp)
		 */
		fix_datacksum(&oip->ip_sum, sumd);

/* XXX FV : without having looked at Solaris source code, it seems unlikely
 * that SOLARIS would compensate this in the kernel (a body of an IP packet 
 * in the data section of an ICMP packet). I have the feeling that this should
 * be unconditional, but I'm not in a position to check.
 */
#if !SOLARIS && !defined(__sgi)
		/*
		 * Fix UDP pseudo header checksum to compensate for the
		 * IP address change.
		 */
		if (oip->ip_p == IPPROTO_UDP && udp->uh_sum) {
			/*
			 * The UDP checksum is optional, only adjust it 
			 * if it has been set 
			 */
			sum1 = ntohs(udp->uh_sum);
			fix_datacksum(&udp->uh_sum, sumd);
			sum2 = ntohs(udp->uh_sum);

			/*
			 * Fix ICMP checksum to compensate the UDP 
			 * checksum adjustment.
			 */
			CALC_SUMD(sum1, sum2, sumd);
			sumd2 = sumd;
		}
		
		/* 
		 * Fix TCP pseudo header checksum to compensate for the 
		 * IP address change. Before we can do the change, we
		 * must make sure that oip is sufficient large to hold
		 * the TCP checksum (normally it does not!).
		 */
		if (oip->ip_p == IPPROTO_TCP && dlen >= 18) {
		
			sum1 = ntohs(tcp->th_sum);
			fix_datacksum(&tcp->th_sum, sumd);
			sum2 = ntohs(tcp->th_sum);

			/*
			 * Fix ICMP checksum to compensate the TCP
			 * checksum adjustment.
			 */
			CALC_SUMD(sum1, sum2, sumd);
			sumd2 = sumd;
		}
#endif
	}

	if ((flags & IPN_TCPUDP) != 0) {
		/*
		 * Step 2 :
		 * For offending TCP/UDP IP packets, translate the ports as
		 * well, based on the NAT specification. Of course such
		 * a change must be reflected in the ICMP checksum as well.
		 *
		 * Advance notice : Now it becomes complicated :-)
		 *
		 * Since the port fields are part of the TCP/UDP checksum
		 * of the offending IP packet, you need to adjust that checksum
		 * as well... but, if you change, you must change the icmp
		 * checksum *again*, to reflect that change.
		 *
		 * To further complicate: the TCP checksum is not in the first
		 * 8 bytes of the offending ip packet, so it most likely is not
		 * available. Some OSses like Solaris return enough bytes to
		 * include the TCP checksum. So we have to check if the
		 * ip->ip_len actually holds the TCP checksum of the oip!
		 */

		if (nat->nat_oport == tcp->th_dport) {
			if (tcp->th_sport != nat->nat_inport) {
				/*
				 * Fix ICMP checksum to compensate port
				 * adjustment.
				 */
				sum1 = ntohs(tcp->th_sport);
				sum2 = ntohs(nat->nat_inport);
				CALC_SUMD(sum1, sum2, sumd);
				sumd2 += sumd;
				tcp->th_sport = nat->nat_inport;

				/*
				 * Fix udp checksum to compensate port
				 * adjustment.  NOTE : the offending IP packet
				 * flows the other direction compared to the
				 * ICMP message.
				 *
				 * The UDP checksum is optional, only adjust
				 * it if it has been set.
				 */
				if (oip->ip_p == IPPROTO_UDP && udp->uh_sum) {

					sum1 = ntohs(udp->uh_sum);
					fix_datacksum(&udp->uh_sum, sumd);
					sum2 = ntohs(udp->uh_sum);

					/*
					 * Fix ICMP checksum to 
					 * compensate UDP checksum 
					 * adjustment.
					 */
					CALC_SUMD(sum1, sum2, sumd);
					sumd2 += sumd;
				}

				/*
				 * Fix tcp checksum (if present) to compensate
				 * port adjustment. NOTE : the offending IP
				 * packet flows the other direction compared to
				 * the ICMP message.
				 */
				if (oip->ip_p == IPPROTO_TCP && dlen >= 18) {

					sum1 = ntohs(tcp->th_sum);
					fix_datacksum(&tcp->th_sum, sumd);
					sum2 = ntohs(tcp->th_sum);

					/*
					 * Fix ICMP checksum to 
					 * compensate TCP checksum 
					 * adjustment.
					 */
					CALC_SUMD(sum1, sum2, sumd);
					sumd2 += sumd;
				}
			}
		} else {
			if (tcp->th_dport != nat->nat_outport) {
				/*
				 * Fix ICMP checksum to compensate port
				 * adjustment.
				 */
				sum1 = ntohs(tcp->th_dport);
				sum2 = ntohs(nat->nat_outport);
				CALC_SUMD(sum1, sum2, sumd);
				sumd2 += sumd;
				tcp->th_dport = nat->nat_outport;

				/*
				 * Fix udp checksum to compensate port
				 * adjustment.   NOTE : the offending IP
				 * packet flows the other direction compared
				 * to the ICMP message.
				 *
				 * The UDP checksum is optional, only adjust
				 * it if it has been set.
				 */
				if (oip->ip_p == IPPROTO_UDP && udp->uh_sum) {

					sum1 = ntohs(udp->uh_sum);
					fix_datacksum(&udp->uh_sum, sumd);
					sum2 = ntohs(udp->uh_sum);

					/*
					 * Fix ICMP checksum to compensate
					 * UDP checksum adjustment.
					 */
					CALC_SUMD(sum1, sum2, sumd);
					sumd2 += sumd;
				}

				/*
				 * Fix tcp checksum (if present) to compensate
				 * port adjustment. NOTE : the offending IP
				 * packet flows the other direction compared to
				 * the ICMP message.
				 */
				if (oip->ip_p == IPPROTO_TCP && dlen >= 18) {

					sum1 = ntohs(tcp->th_sum);
					fix_datacksum(&tcp->th_sum, sumd);
					sum2 = ntohs(tcp->th_sum);

					/*
					 * Fix ICMP checksum to compensate
					 * UDP checksum adjustment.
					 */
					CALC_SUMD(sum1, sum2, sumd);
					sumd2 += sumd;
				}
			}
		}
		if (sumd2) {
			sumd2 = (sumd2 & 0xffff) + (sumd2 >> 16);
			sumd2 = (sumd2 & 0xffff) + (sumd2 >> 16);
			if (nat->nat_dir == NAT_OUTBOUND) {
				fix_outcksum(fin->fin_dlen, &icmp->icmp_cksum, sumd2);
			} else {
				fix_incksum(fin->fin_dlen, &icmp->icmp_cksum, sumd2);
			}
		}
	}
	
	/* 10/01/04 Roger ++,  */	
	if ((flags & IPN_ICMPQUERY) != 0) {
		icmphdr_t *orgicmp;

		/*
		 * XXX - what if this is bogus hl and we go off the end ?
		 * In this case, nat_icmperrorlookup() will have returned NULL.
		 */
		orgicmp = (icmphdr_t *)udp;

		if (nat->nat_dir == NAT_OUTBOUND) {
			if (orgicmp->icmp_id != nat->nat_inport) {

				/* 
				 * Fix ICMP checksum (of the offening ICMP
				 * query packet) to compensate the change
				 * in the ICMP id of the offending ICMP
				 * packet.
				 *
				 * Since you modify orgicmp->icmp_id with
				 * a delta (say x) and you compensate that
				 * in origicmp->icmp_cksum with a delta
				 * minus x, you don't have to adjust the
				 * overall icmp->icmp_cksum 
 				 */
				sum1 = ntohs(orgicmp->icmp_id);
				sum2 = ntohs(nat->nat_inport);
				CALC_SUMD(sum1, sum2, sumd);
				orgicmp->icmp_id = nat->nat_inport;
				fix_datacksum(&orgicmp->icmp_cksum, sumd);
			}
		} /* nat_dir == NAT_INBOUND is impossible for icmp queries */
	}
	/* 10/01/04 Roger --,  */	
	
	if (oip->ip_p == IPPROTO_ICMP)
		nat->nat_age = fr_defnaticmpage;
	return nat;
}


/*
 * NB: these lookups don't lock access to the list, it assume it has already
 * been done!
 */
/*
 * Lookup a nat entry based on the mapped destination ip address/port and
 * real source address/port.  We use this lookup when receiving a packet,
 * we're looking for a table entry, based on the destination address.
 * NOTE: THE PACKET BEING CHECKED (IF FOUND) HAS A MAPPING ALREADY.
 */
nat_t *nat_inlookup(fin, flags, p, src, mapdst, write)
fr_info_t *fin;
register u_int flags, p;
struct in_addr src , mapdst;
int write;
{
	register u_short sport, dport;
	register nat_t *nat;
	register int nflags;
	register u_32_t dst;
	void *ifp;
	u_int hv;
	icmphdr_t *icmp; /* 10/01/04 Roger,  */	
	grehdr_t *gre = NULL; /* 14/01/04 Roger,  */	
	u_int sflags;
	
	if (fin != NULL)
		ifp = fin->fin_ifp;
	else
		ifp = NULL;
	dst = mapdst.s_addr;
	
	if ((flags & IPN_TCPUDP) && fin != NULL) {
		sport = htons(fin->fin_data[0]);
		dport = htons(fin->fin_data[1]);
	/* 10/01/04 Roger ++,  */	
	} else if ((flags & IPN_ICMPQUERY) && fin != NULL) {
		icmp = (icmphdr_t *)fin->fin_dp;
		dport = icmp->icmp_id;
		sport = 0;
	/* 10/01/04 Roger --,  */	
	} else {
		if(fin != NULL)
		gre = (p == IPPROTO_GRE) ? fin->fin_dp : NULL;
		sport = 0;
		dport = 0;
	}
	sflags = flags & IPN_TCPUDPICMP; /* 10/01/04 Roger,  */	

#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
	/* Port Restricted NAT */
	hv = NAT_HASH_FN(dst, dport, ipf_nattable_sz);
#else	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
	/*  Symmertic NAT  */
	hv = NAT_HASH_FN(dst, dport, 0xffffffff);
	hv = NAT_HASH_FN(src.s_addr, hv + sport, ipf_nattable_sz);
#endif	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
	nat = nat_table[1][hv];
	
	//diag_printf("nat_inlookup 100(hv:%d), dst:%x, dport:%x, sport:%x src.s_addr:%x, \n", hv, dst, dport, sport, src.s_addr); /* 10/01/04 Roger,  */
	for (; nat; nat = nat->nat_hnext[1]) {
		nflags = nat->nat_flags;
		if (nflags & IPN_DELETE)
			continue;
		if ((!ifp || ifp == nat->nat_ifp) &&
		    nat->nat_oip.s_addr == src.s_addr &&
		    nat->nat_outip.s_addr == dst &&
		    /* 10/01/04 Roger ++,  */	
		    (((p == 0) && (sflags == (nat->nat_flags & IPN_TCPUDPICMP)))||
		    (p == nat->nat_p))){
		    //((p == 0) || (p == nat->nat_p))) {
		    /* 10/01/04 Roger --,  */	
			switch (p)
			{
			case IPPROTO_GRE :
				if (gre != NULL)
					if (gre->gr_call != nat->nat_gre.gs_call)
						continue;
				break;
			case IPPROTO_TCP :
			case IPPROTO_UDP :
			case IPPROTO_ICMP :
				if (nat->nat_oport != sport)
					continue;
				if (nat->nat_outport != dport)
					continue;
				break;
			default :
				break;
			}
			
			//ipn = nat->nat_ptr;
			//if ((ipn != NULL) && (nat->nat_aps != NULL))
			if (nat->nat_aps != NULL)
				if (appr_match(fin, nat) != 0)
					continue;
			return nat;
		}
	}
	
	if (!nat_stats.ns_wilds || !(flags & (FI_WILDP|FI_WILDA)))
		return NULL;
		
	RWLOCK_EXIT(&ipf_nat);
	
#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
	/* Port Restricted NAT */
	hv = NAT_HASH_FN(dst, dport, ipf_nattable_sz);
#else	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
	hv = NAT_HASH_FN(dst, 0, 0xffffffff);
	hv = NAT_HASH_FN(src.s_addr, hv, ipf_nattable_sz);
#endif	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */

	WRITE_ENTER(&ipf_nat);
	
	nat = nat_table[1][hv];
	
	for (; nat; nat = nat->nat_hnext[1]) {
		nflags = nat->nat_flags;
		if (nflags & IPN_DELETE)
			continue;
		if (ifp && ifp != nat->nat_ifp)
			continue;
		if (!(nflags & FI_WILDP))
			continue;
		if (nat->nat_oip.s_addr != src.s_addr ||
		    nat->nat_outip.s_addr != dst)
			continue;
		/* 17/01/04 Roger ++,  */
		//if (((nat->nat_oport == sport) || (nflags & FI_W_DPORT)) &&
		//    ((nat->nat_outport == dport) || (nflags & FI_W_SPORT))) {
		if (((nat->nat_dir == NAT_OUTBOUND) && (((nat->nat_oport == sport) || (nflags & FI_W_DPORT)) &&
		    ((nat->nat_outport == dport) || (nflags & FI_W_SPORT)))) ||
		    ((nat->nat_dir == NAT_INBOUND) && (((nat->nat_oport == sport) || (nflags & FI_W_SPORT)) &&
		    ((nat->nat_outport == dport) || (nflags & FI_W_DPORT))))){
		/* 17/01/04 Roger --,  */
			if(write && !(nat->nat_flags & IPN_NOTABMOVE ))
			{
				nat->nat_oport = sport;
				nat->nat_outport = dport;
				//nat->nat_flags &= ~(FI_W_DPORT|FI_W_SPORT);	
				nat_tabmove(nat);
				//nat_stats.ns_wilds--;
			}
			break;
		}
	}
	
	MUTEX_DOWNGRADE(&ipf_nat);
	
	/* 17/04/04 Roger ++,  FI_W_DADDR|FI_W_DPORT case, for ALG use*/	
	if(nat)
		return nat;
	
	hv = NAT_HASH_FN(dst, dport, ipf_nattable_sz);
	WRITE_ENTER(&ipf_nat);
	nat = nat_table[1][hv];
	//diag_printf("###nat_inlookup 500(hv:%d), nat:%x, dst:%x, dport:%x, sport:%x src.s_addr:%x, \n", hv, nat, dst, dport, sport, src.s_addr); /* 10/01/04 Roger,  */
	for (; nat; nat = nat->nat_hnext[1]) {
		if (nat->nat_flags & IPN_DELETE)
			continue;
		if((nat->nat_flags & (FI_W_DADDR|FI_W_DPORT)) != (FI_W_DADDR|FI_W_DPORT))
			continue;
		if (((ifp && ifp == nat->nat_ifp) || !(nat->nat_ifp)) &&
			(p == nat->nat_p) &&
			(nat->nat_outip.s_addr == dst) &&
			(nat->nat_outport == dport)){
			if(write && !(nat->nat_flags & IPN_NOTABMOVE ))
			{	
				nat->nat_oport = sport;
				nat->nat_oip.s_addr = src.s_addr;
				//nat->nat_flags &= ~(FI_W_DADDR|FI_W_DPORT);	
				nat_tabmove(nat);
				//nat_stats.ns_wilds--;
			}
			break;
		}
			
	}
	
	MUTEX_DOWNGRADE(&ipf_nat);
	return nat;
	/* 17/04/04 Roger --,  */	
}


/*
 * This function is only called for TCP/UDP NAT table entries where the
 * original was placed in the table without hashing on the ports and we now
 * want to include hashing on port numbers.
 */
void nat_tabmove(nat)
nat_t *nat;
{
	u_int hv, nflags;
	nat_t **natp;

	nflags = nat->nat_flags;
	/*
	 * Remove the NAT entry from the old location
	 */
	if (nat->nat_hnext[0])
		nat->nat_hnext[0]->nat_phnext[0] = nat->nat_phnext[0];
	*nat->nat_phnext[0] = nat->nat_hnext[0];

	if (nat->nat_hnext[1])
		nat->nat_hnext[1]->nat_phnext[1] = nat->nat_phnext[1];
	*nat->nat_phnext[1] = nat->nat_hnext[1];

	/*
	 * Add into the NAT table in the new position
	 */
#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
	hv = NAT_HASH_FN(nat->nat_inip.s_addr, nat->nat_inport, ipf_nattable_sz);
#else	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
	hv = NAT_HASH_FN(nat->nat_inip.s_addr, nat->nat_inport, 0xffffffff);
	hv = NAT_HASH_FN(nat->nat_oip.s_addr, hv + nat->nat_oport, ipf_nattable_sz);
#endif	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
	natp = &nat_table[0][hv];
	if (*natp)
		(*natp)->nat_phnext[0] = &nat->nat_hnext[0];
	nat->nat_phnext[0] = natp;
	nat->nat_hnext[0] = *natp;
	*natp = nat;

#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
	hv = NAT_HASH_FN(nat->nat_outip.s_addr, nat->nat_outport, ipf_nattable_sz);
#else	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
	hv = NAT_HASH_FN(nat->nat_outip.s_addr, nat->nat_outport, 0xffffffff);
	hv = NAT_HASH_FN(nat->nat_oip.s_addr, hv + nat->nat_oport, ipf_nattable_sz);
#endif	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
	natp = &nat_table[1][hv];
	if (*natp)
		(*natp)->nat_phnext[1] = &nat->nat_hnext[1];
	nat->nat_phnext[1] = natp;
	nat->nat_hnext[1] = *natp;
	*natp = nat;
}


/*
 * Lookup a nat entry based on the source 'real' ip address/port and
 * destination address/port.  We use this lookup when sending a packet out,
 * we're looking for a table entry, based on the source address.
 * NOTE: THE PACKET BEING CHECKED (IF FOUND) HAS A MAPPING ALREADY.
 */
nat_t *nat_outlookup(fin, flags, p, src, dst, write)
fr_info_t *fin;
register u_int flags, p;
struct in_addr src , dst;
int write;
{
	register u_short sport, dport;
	register nat_t *nat;
	register int nflags;
	u_32_t srcip;
	void *ifp;
	u_int hv;
	icmphdr_t *icmp; /* 10/01/04 Roger,  */
	grehdr_t *gre = NULL;
	u_int sflags;
	
	ifp = fin->fin_ifp;
	srcip = src.s_addr;
	if (flags & IPN_TCPUDP) {
		sport = ntohs(fin->fin_data[0]);
		dport = ntohs(fin->fin_data[1]);
	/* 10/01/04 Roger ++,  */	
	} else if (flags & IPN_ICMPQUERY) {
		icmp = (icmphdr_t *)fin->fin_dp;
		dport = 0;
		sport = icmp->icmp_id;
	/* 10/01/04 Roger --,  */	
	} else {
		gre = (p == IPPROTO_GRE) ? fin->fin_dp : NULL;
		sport = 0;
		dport = 0;
	}
	sflags = flags & IPN_TCPUDPICMP; /* 10/01/04 Roger,  */
	
#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
	hv = NAT_HASH_FN(srcip, sport, ipf_nattable_sz);
#else	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
	hv = NAT_HASH_FN(srcip, sport, 0xffffffff);
	hv = NAT_HASH_FN(dst.s_addr, hv + dport, ipf_nattable_sz);
#endif	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
	nat = nat_table[0][hv];
	for (; nat; nat = nat->nat_hnext[0]) {
		nflags = nat->nat_flags;
		if (nflags & IPN_DELETE)
			continue;

		if ((!ifp || ifp == nat->nat_ifp) &&
		    nat->nat_inip.s_addr == srcip &&
		    nat->nat_oip.s_addr == dst.s_addr &&
		    /* 10/01/04 Roger ++,  */	
		    (((p == 0) && (sflags == (nflags & IPN_TCPUDPICMP)))
 		     || (p == nat->nat_p))){
 		     //((p == 0) || (p == nat->nat_p))) {
 		     /* 10/01/04 Roger --,  */	
			switch (p)
			{
			case IPPROTO_GRE :
				if(gre != NULL)
					if (gre->gr_call != nat->nat_gre.gs_call)
						continue;
				break;
			case IPPROTO_TCP :
			case IPPROTO_UDP :
			case IPPROTO_ICMP :
				if (nat->nat_oport != dport)
					continue;
				if (nat->nat_inport != sport)
					continue;
				break;
			default :
				break;
			}

			//ipn = nat->nat_ptr;
			//if ((ipn != NULL) && (nat->nat_aps != NULL))
			if (nat->nat_aps != NULL)
				if (appr_match(fin, nat) != 0)
					continue;
			return nat;
		}
	}
	if (!nat_stats.ns_wilds || !(flags & (FI_WILDP|FI_WILDA)))
		return NULL;
	
	RWLOCK_EXIT(&ipf_nat);

#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
	hv = NAT_HASH_FN(srcip, sport, ipf_nattable_sz);
#else	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
    // musterc+, 2006.03.10: Modify the hash method according to nat_insert()
	//hv = NAT_HASH_FN(dst.s_addr, srcip, ipf_nattable_sz);
    hv = NAT_HASH_FN(srcip, 0, 0xffffffff);
    hv = NAT_HASH_FN(dst.s_addr, hv, ipf_nattable_sz);
#endif	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
	
	WRITE_ENTER(&ipf_nat);
	
	nat = nat_table[0][hv];
	for (; nat; nat = nat->nat_hnext[0]) {
		nflags = nat->nat_flags;
		if (nflags & IPN_DELETE)
			continue;
		if (ifp && ifp != nat->nat_ifp)
			continue;
		if (!(nflags & FI_WILDP))
			continue;
		if ((nat->nat_inip.s_addr != srcip) ||
		    (nat->nat_oip.s_addr != dst.s_addr))
			continue;
		/* 17/01/04 Roger ++,  */	
		//if (((nat->nat_inport == sport) || (nflags & FI_W_SPORT)) &&
		//    ((nat->nat_oport == dport) || (nflags & FI_W_DPORT))) {
		if (((nat->nat_dir ==  NAT_OUTBOUND)&&(((nat->nat_inport == sport) || (nflags & FI_W_SPORT)) &&
		    ((nat->nat_oport == dport) || (nflags & FI_W_DPORT)))) ||
		    ((nat->nat_dir == NAT_INBOUND) &&(((nat->nat_inport == sport) || (nflags & FI_W_DPORT)) &&
		    ((nat->nat_oport == dport) || (nflags & FI_W_SPORT))))) {
		/* 17/01/04 Roger --,  */	
			if(write && !(nat->nat_flags & IPN_NOTABMOVE ))
			{
				nat->nat_inport = sport;
				nat->nat_oport = dport;
				if (nat->nat_outport == 0)
					nat->nat_outport = sport;
				//nat->nat_flags &= ~(FI_W_DPORT|FI_W_SPORT);
				nat_tabmove(nat);
				//nat_stats.ns_wilds--;
			}
			break;
		}
	}
	
	MUTEX_DOWNGRADE(&ipf_nat);
	
	/* 25/02/05 Roger,  FI_W_DADDR|FI_W_DPORT case, for ALG use*/	
	if(nat)
		return nat;

	hv = NAT_HASH_FN(srcip, sport, ipf_nattable_sz);
	WRITE_ENTER(&ipf_nat);
	nat = nat_table[0][hv];
	for (; nat; nat = nat->nat_hnext[0]) {
		if (nat->nat_flags & IPN_DELETE)
			continue;
		if((nat->nat_flags & (FI_W_DADDR|FI_W_DPORT)) != (FI_W_DADDR|FI_W_DPORT))
			continue;
		if (((ifp && ifp == nat->nat_ifp) || !(nat->nat_ifp)) &&
			(p == nat->nat_p) &&
			(nat->nat_inip.s_addr == srcip) &&
			(nat->nat_inport == sport)){
				
			if(write && !(nat->nat_flags & IPN_NOTABMOVE ))
			{	
				nat->nat_oport = dport;
				nat->nat_oip.s_addr = srcip;
				//nat->nat_flags &= ~(FI_W_DADDR|FI_W_DPORT);	
				nat_tabmove(nat);
				//nat_stats.ns_wilds--;
			}
			break;
		}
			
	}
	
	MUTEX_DOWNGRADE(&ipf_nat);
	return nat;
	/* 17/04/04 Roger --,  */	
}


/*
 * Lookup the NAT tables to search for a matching redirect
 */
nat_t *nat_lookupredir(np)
register natlookup_t *np;
{
	nat_t *nat;
	fr_info_t fi;

	bzero((char *)&fi, sizeof(fi));
	fi.fin_data[0] = ntohs(np->nl_inport);
	fi.fin_data[1] = ntohs(np->nl_outport);

	/*
	 * If nl_inip is non null, this is a lookup based on the real
	 * ip address. Else, we use the fake.
	 */
	if ((nat = nat_outlookup(&fi, np->nl_flags, 0, np->nl_inip,
				 np->nl_outip, 0))) {
		np->nl_realip = nat->nat_outip;
		np->nl_realport = nat->nat_outport;
	}
	return nat;
}


static int nat_match(fin, np, ip)
fr_info_t *fin;
ipnat_t *np;
ip_t *ip;
{
	frtuc_t *ft;

	if (ip->ip_v != 4)
		return 0;

	if (np->in_p && fin->fin_p != np->in_p)
		return 0;
	if (fin->fin_out) {
		if (!(np->in_redir & (NAT_MAP|NAT_MAPBLK)))
			return 0;
		if (((fin->fin_fi.fi_saddr & np->in_inmsk) != np->in_inip)
		    ^ ((np->in_flags & IPN_NOTSRC) != 0))
			return 0;
		if (((fin->fin_fi.fi_daddr & np->in_srcmsk) != np->in_srcip)
		    ^ ((np->in_flags & IPN_NOTDST) != 0))
			return 0;
	} else {
		if (!(np->in_redir & NAT_REDIRECT))
			return 0;
		if (((fin->fin_fi.fi_saddr & np->in_srcmsk) != np->in_srcip)
		    ^ ((np->in_flags & IPN_NOTSRC) != 0))
			return 0;
		if (((fin->fin_fi.fi_daddr & np->in_outmsk) != np->in_outip)
		    ^ ((np->in_flags & IPN_NOTDST) != 0))
			return 0;
	}

	ft = &np->in_tuc;
	if (!(fin->fin_fl & FI_TCPUDP) ||
	    (fin->fin_fl & FI_SHORT) || (fin->fin_off != 0)) {
		if (ft->ftu_scmp || ft->ftu_dcmp)
			return 0;
		return 1;
	}

	return fr_tcpudpchk(ft, fin);
}

/* 26/01/04 Roger ++,  */	
inline static int multip_match(dport, np)
unsigned short dport;
ipnat_t *np;
{
	struct multip *mtport;
	int i = 0;
	
	if(!np->in_mport)
		return 0;
	mtport = np->in_mport;
	
	while(mtport[i].in_port[0]) {
		if((ntohs(mtport[i].in_port[1]) >= ntohs(dport)) &&
			      (ntohs(dport) >= ntohs(mtport[i].in_port[0])))
			return 1;
		i++;
	}
	
	return 0;
}
/* 26/01/04 Roger --,  */	

/*
 * Packets going out on the external interface go through this.
 * Here, the source address requires alteration, if anything.
 */
#if 0/* 11/03/05 Roger ++,  */	
int ip_natout(ip, fin)
ip_t *ip;
fr_info_t *fin;
{
	register ipnat_t *np = NULL;
	register u_32_t ipa;
	tcphdr_t *tcp = NULL;
	icmphdr_t *icmp = NULL; /* 10/01/04 Roger,  */	
	u_short sport = 0, dport = 0, *csump = NULL;
	int natadd = 1, i, icmpset = 1;
	u_int nflags = 0, hv, msk;
	struct ifnet *ifp;
	frentry_t *fr;
	//void *sifp;
	u_32_t iph;
	nat_t *nat;
	
	if (nat_list == NULL || (fr_nat_lock))
		return 0;
#if 0
	if ((fr = fin->fin_fr) && !(fr->fr_flags & FR_DUP) &&
	    fr->fr_tif.fd_ifp && fr->fr_tif.fd_ifp != (void *)-1) {
		sifp = fin->fin_ifp;
		fin->fin_ifp = fr->fr_tif.fd_ifp;
	} else
		sifp = fin->fin_ifp;
#endif
	ifp = fin->fin_ifp;

	if ((fin->fin_off == 0) && !(fin->fin_fl & FI_SHORT)) {
		if (fin->fin_p == IPPROTO_TCP)
			nflags = IPN_TCP;
		else if (fin->fin_p == IPPROTO_UDP)
			nflags = IPN_UDP;
		/* 10/01/04 Roger ++,  */	
		else if (fin->fin_p == IPPROTO_ICMP) {

			icmp = (icmphdr_t *)fin->fin_dp;
			
			if (check_icmpquerytype4(icmp->icmp_type)) {
				nflags = IPN_ICMPQUERY;
				sport = icmp->icmp_id;
				dport = 0;	
			}
		}
		/* 10/01/04 Roger --,  */	
		if ((nflags & IPN_TCPUDP)) {
			tcp = (tcphdr_t *)fin->fin_dp;
			sport = tcp->th_sport;
			dport = tcp->th_dport;
		}
	}

	ipa = fin->fin_saddr;

	READ_ENTER(&ipf_nat);
	/* 10/01/04 Roger ++,  */	
	//if ((fin->fin_p == IPPROTO_ICMP) &&
	//    (nat = nat_icmp(ip, fin, &nflags, NAT_OUTBOUND)))
	if ((ip->ip_p == IPPROTO_ICMP) && !(nflags & IPN_ICMPQUERY) &&
	    (nat = nat_icmperror(ip, fin, &nflags, NAT_OUTBOUND)))
	/* 10/01/04 Roger --,  */	
		icmpset = 1;
	else if ((fin->fin_fl & FI_FRAG) &&
	    (nat = ipfr_nat_knownfrag(ip, fin)))
		natadd = 0;
	else if ((nat = nat_outlookup(fin, nflags|FI_WILDP|FI_WILDA,
				      (u_int)fin->fin_p, fin->fin_src,
				      fin->fin_dst, 1))) {
		//nflags = nat->nat_flags;
		//if ((nflags & (FI_W_SPORT|FI_W_DPORT)) != 0) {
			//if ((nflags & FI_W_SPORT) &&
			//    (nat->nat_inport != sport))
				//nat->nat_inport = sport;
			//if ((nflags & FI_W_DPORT) &&
			//    (nat->nat_oport != dport))
				//nat->nat_oport = dport;

			//if (nat->nat_outport == 0)
				//nat->nat_outport = sport;
			//nat->nat_flags &= ~(FI_W_DPORT|FI_W_SPORT);
			//nflags = nat->nat_flags;
			//nat_stats.ns_wilds--;
		//}
		if (((nat->nat_flags & IPN_NOTABMOVE) == 0) && ((nflags & (FI_WILDP|FI_WILDA)) != 0)) {
			nat->nat_flags &= ~(FI_WILDP|FI_WILDA);
			nat_stats.ns_wilds--;
		}
	} else {
		RWLOCK_EXIT(&ipf_nat);

		msk = 0xffffffff;
		i = 32;

		WRITE_ENTER(&ipf_nat);
		/*
		 * If there is no current entry in the nat table for this IP#,
		 * create one for it (if there is a matching rule).
		 */
maskloop:
		iph = ipa & htonl(msk);
		hv = NAT_HASH_FN(iph, 0, ipf_natrules_sz);
		for (np = nat_rules[hv]; np; np = np->in_mnext)
		{
			if (np->in_ifp && (np->in_ifp != ifp))
				continue;
			if ((np->in_flags & IPN_RF) &&
			    !(np->in_flags & nflags))
				continue;
			if (np->in_flags & IPN_FILTER) {
				if (!nat_match(fin, np, ip))
					continue;
			} else if ((ipa & np->in_inmsk) != np->in_inip)
				continue;
			if (*np->in_plabel && !appr_ok(ip, tcp, np))
				continue;
			/* 15/09/04 Roger ++,  */	
			if((np->in_redir == NAT_REDIRECT) && (np->in_pnext != sport))
				continue;
			/* 15/09/04 Roger --,  */
			nat = nat_new(fin, ip, np, NULL,
				      (u_int)nflags, NAT_OUTBOUND);
			
			if (nat != NULL) {
				np->in_hits++;
			}
			else
				fin->fin_misc |= FM_BADNAT;
			break;
		}
		if ((np == NULL) && (i > 0)) {
			do {
				i--;
				msk <<= 1;
			} while ((i >= 0) && ((nat_masks & (1 << i)) == 0));
			if (i >= 0)
				goto maskloop;
		}
		MUTEX_DOWNGRADE(&ipf_nat);
	}
	fin->fin_nat = nat;
	return 0;
#if 0
	/*
	 * NOTE: ipf_nat must now only be held as a read lock
	 */
	if (nat) {
		np = nat->nat_ptr;
		if (natadd && (fin->fin_fl & FI_FRAG) && np)
			ipfr_nat_newfrag(ip, fin, nat);
		MUTEX_ENTER(&nat->nat_lock);
		if (fin->fin_p != IPPROTO_TCP) {
			if (np && np->in_age[1])
				nat->nat_age = np->in_age[1];
			else if (!icmpset && (fin->fin_p == IPPROTO_ICMP))
				nat->nat_age = fr_defnaticmpage;
			else
				nat->nat_age = fr_defnatage;
		}
		//nat->nat_bytes += ip->ip_len;
		//nat->nat_pkts++;
		MUTEX_EXIT(&nat->nat_lock);

		/*
		 * Fix up checksums, not by recalculating them, but
		 * simply computing adjustments.
		 */
		if (nflags == IPN_ICMPERR) {
			u_32_t s1, s2, sumd;

			s1 = LONG_SUM(ntohl(fin->fin_saddr));
			s2 = LONG_SUM(ntohl(nat->nat_outip.s_addr));
			CALC_SUMD(s1, s2, sumd);

			if (nat->nat_dir == NAT_OUTBOUND)
				fix_outcksum(fin->fin_dlen, &ip->ip_sum, sumd);
			else
				fix_incksum(fin->fin_dlen, &ip->ip_sum, sumd);
		}

		/*
		 * Only change the packet contents, not what is filtered upon.
		 */
		ip->ip_src = nat->nat_outip;

		if ((fin->fin_off == 0) && !(fin->fin_fl & FI_SHORT)) {
			if ((nat->nat_outport != 0) && (tcp != NULL)) {
				tcp->th_sport = nat->nat_outport;
				fin->fin_data[0] = ntohs(tcp->th_sport);
			}
			/* 10/01/04 Roger ++,  */	
			if ((nat->nat_outport != 0) && (nflags & IPN_ICMPQUERY)) {
				icmp->icmp_id = nat->nat_outport;
			}
			/* 10/01/04 Roger --,  */	
			if (fin->fin_p == IPPROTO_TCP) {
				csump = &tcp->th_sum;
				MUTEX_ENTER(&nat->nat_lock);
				fr_tcp_age(&nat->nat_age,
					   nat->nat_tcpstate, fin, 1, 0);
#if 0
				if (nat->nat_age < fr_defnaticmpage)
					nat->nat_age = fr_defnaticmpage;
#ifdef LARGE_NAT
				else if (nat->nat_age > fr_defnatage)
					nat->nat_age = fr_defnatage;
#endif
#endif
				/*
				 * Increase this because we may have
				 * "keep state" following this too and
				 * packet storms can occur if this is
				 * removed too quickly.
				 */
				//if (nat->nat_age == fr_tcpclosed)
				//	nat->nat_age = fr_tcplastack;

                                /*
                                 * Do a MSS CLAMPING on a SYN packet,
                                 * only deal IPv4 for now.
                                 */
                                if (nat->nat_mssclamp &&
                                    (tcp->th_flags & TH_SYN) != 0)
                                        nat_mssclamp(tcp, nat->nat_mssclamp,
						     fin, csump);

				MUTEX_EXIT(&nat->nat_lock);
			} else if (fin->fin_p == IPPROTO_UDP) {
				udphdr_t *udp = (udphdr_t *)tcp;

				if (udp->uh_sum)
					csump = &udp->uh_sum;
			/* 10/01/04 Roger ++,  */				
			} else if ( (ip->ip_p == IPPROTO_ICMP) && (nflags & IPN_ICMPQUERY) ) {
				if (icmp->icmp_cksum)
					csump = &icmp->icmp_cksum;
			
				nat->nat_age = fr_defnatage;
			/* 10/01/04 Roger --,  */	
			}

			if (csump) {
				if (nat->nat_dir == NAT_OUTBOUND)
					fix_outcksum(fin->fin_dlen, csump,
						     nat->nat_sumd[1]);
				else
					fix_incksum(fin->fin_dlen, csump,
						    nat->nat_sumd[1]);
			}
		}

		//if (np && (np->in_apr != NULL) && ((np->in_dport == 0 ||
		//     (tcp != NULL && dport == np->in_dport))||
		//     ((nat->nat_dir == NAT_INBOUND) && tcp != NULL && sport == np->in_dport))) {
		if (nat->nat_aps) {
			i = appr_check(ip, fin, nat);
			if (i == 0)
				i = 1;
			else if (i == -1)
				//nat->nat_drop[1]++;
				;
		} else
			i = 1;
		nat_check_fcache(nat, tcp);
		
		ATOMIC_INCL(nat_stats.ns_mapped[1]);
		RWLOCK_EXIT(&ipf_nat);	/* READ */
		//fin->fin_ifp = sifp;
		return i;
	}
	RWLOCK_EXIT(&ipf_nat);			/* READ/WRITE */
	//fin->fin_ifp = sifp;
	
	return 0;
#endif
}
#endif /* 11/03/05 Roger --,  0*/	

int ip_natout_match(ip, fin)
ip_t *ip;
fr_info_t *fin;
{
	icmphdr_t *icmp = NULL;
	int natadd = 1, icmpset = 1;
	u_int nflags = 0;
	nat_t *nat;
	
	if (nat_list == NULL || (fr_nat_lock))
		return 0;

	if ((fin->fin_off == 0) && !(fin->fin_fl & FI_SHORT)) {
		if (fin->fin_p == IPPROTO_TCP)
			nflags = IPN_TCP;
		else if (fin->fin_p == IPPROTO_UDP)
			nflags = IPN_UDP;
		else if (fin->fin_p == IPPROTO_ICMP) {

			icmp = (icmphdr_t *)fin->fin_dp;
			
			if (check_icmpquerytype4(icmp->icmp_type)) {
				nflags = IPN_ICMPQUERY;
			}
		}
	}

	READ_ENTER(&ipf_nat);
	//if ((fin->fin_p == IPPROTO_ICMP) &&
	//    (nat = nat_icmp(ip, fin, &nflags, NAT_OUTBOUND)))
	if ((ip->ip_p == IPPROTO_ICMP) && !(nflags & IPN_ICMPQUERY) &&
	    (nat = nat_icmperror(ip, fin, &nflags, NAT_OUTBOUND)))
		icmpset = 1;
	else if ((fin->fin_fl & FI_FRAG) && (nat = ipfr_nat_knownfrag(ip, fin)))
		natadd = 0;
	else if ((nat = nat_outlookup(fin, nflags|FI_WILDP|FI_WILDA,
				      (u_int)fin->fin_p, fin->fin_src,
				      fin->fin_dst, 1))) {
		//nflags = nat->nat_flags;
		//if ((nflags & (FI_W_SPORT|FI_W_DPORT)) != 0) {
			//if ((nflags & FI_W_SPORT) &&
			//    (nat->nat_inport != sport))
				//nat->nat_inport = sport;
			//if ((nflags & FI_W_DPORT) &&
			//    (nat->nat_oport != dport))
				//nat->nat_oport = dport;

			//if (nat->nat_outport == 0)
				//nat->nat_outport = sport;
			//nat->nat_flags &= ~(FI_W_DPORT|FI_W_SPORT);
			//nflags = nat->nat_flags;
			//nat_stats.ns_wilds--;
		//}
		if (((nat->nat_flags & IPN_NOTABMOVE) == 0) && ((nat->nat_flags & (FI_WILDP|FI_WILDA)) != 0)) {
			nat->nat_flags &= ~(FI_WILDP|FI_WILDA);
			nat_stats.ns_wilds--;
		}
	}
	RWLOCK_EXIT(&ipf_nat); 
		
	fin->fin_nat = nat;
	return 0;
}

int ip_natout_change(ip, fin)
ip_t *ip;
fr_info_t *fin;
{
	register ipnat_t *np = NULL;
	register u_32_t ipa;
	tcphdr_t *tcp = NULL;
	icmphdr_t *icmp = NULL; /* 10/01/04 Roger,  */	
	u_short sport = 0, dport = 0, *csump = NULL;
	int i, icmpset = 0;
	u_int nflags = 0, hv, msk;
	struct ifnet *ifp;
	u_32_t iph;
	nat_t *nat;
	
	if (nat_list == NULL || (fr_nat_lock))
		return 0;
		
	nat = fin->fin_nat;
	
	ifp = fin->fin_ifp;

	if ((fin->fin_off == 0) && !(fin->fin_fl & FI_SHORT)) {
		if (fin->fin_p == IPPROTO_TCP)
			nflags = IPN_TCP;
		else if (fin->fin_p == IPPROTO_UDP)
			nflags = IPN_UDP;
		else if (fin->fin_p == IPPROTO_ICMP) {

			icmp = (icmphdr_t *)fin->fin_dp;
			
			if (check_icmpquerytype4(icmp->icmp_type)) {
				nflags = IPN_ICMPQUERY;
				sport = icmp->icmp_id;
				dport = 0;	
			}
			else if(nat)
				icmpset = 1;
		}
		if ((nflags & IPN_TCPUDP)) {
			tcp = (tcphdr_t *)fin->fin_dp;
			sport = tcp->th_sport;
			dport = tcp->th_dport;
		}
	}
	ipa = fin->fin_saddr;
	
	if(!nat)
	{
		msk = 0xffffffff;
		i = 32;

		WRITE_ENTER(&ipf_nat);
		/*
		 * If there is no current entry in the nat table for this IP#,
		 * create one for it (if there is a matching rule).
		 */
maskloop:
		iph = ipa & htonl(msk);
		hv = NAT_HASH_FN(iph, 0, ipf_natrules_sz);
		for (np = nat_rules[hv]; np; np = np->in_mnext)
		{
			if (np->in_ifp && (np->in_ifp != ifp))
				continue;
			/* 30/03/05 Roger ++,  check interface packet come from */	

			if(np->from_ifp)
			{
				mb_t *m;
				
				m = *((mb_t **)fin->fin_mp);
				if(m->m_pkthdr.rcvif != np->from_ifp)
					continue;
			}
			/* 30/03/05 Roger --,  */	
			if ((np->in_flags & IPN_RF) &&
			    !(np->in_flags & nflags))
				continue;
			if (np->in_flags & IPN_FILTER) {
				if (!nat_match(fin, np, ip))
					continue;
			} else if ((ipa & np->in_inmsk) != np->in_inip)
				continue;

			if (*np->in_plabel && !appr_ok(ip, tcp, np))
				continue;
			/* 15/09/04 Roger ++,  */	
			if((np->in_redir == NAT_REDIRECT) && (np->in_pnext != sport))
				continue;
			/* 15/09/04 Roger --,  */
			nat = nat_new(fin, ip, np, NULL,
				      (u_int)nflags, NAT_OUTBOUND);
			
			if (nat != NULL) {
				np->in_hits++;
			}
			else
				fin->fin_misc |= FM_BADNAT;
			break;
		}

		if ((np == NULL) && (i > 0)) {
			do {
				i--;
				msk <<= 1;
			} while ((i >= 0) && ((nat_masks & (1 << i)) == 0));
			if (i >= 0)
				goto maskloop;
		}
		MUTEX_DOWNGRADE(&ipf_nat);
	}
	/*
	 * NOTE: ipf_nat must now only be held as a read lock
	 */
	if (nat) {
		np = nat->nat_ptr;
#ifdef CONFIG_DROP_ILLEGAL_TCPRST
		if ((fin->fin_fl & FI_FRAG) && np)
		{
			ipfr_t *ipf;
			nat_t *frag_nat = ipfr_nat_knownfrag(ip, fin);
			if (frag_nat == NULL)
			{
				if (ipfr_nat_newfrag(ip, fin, nat) == 0)
				{
					ipf = nat->nat_data;
					ipf->ipfr_new_ipid = htons(ip_id++);
					ip->ip_id = ipf->ipfr_new_ipid;
				}
			}
			else
			{
				ipf = frag_nat->nat_data;
				ip->ip_id = ipf->ipfr_new_ipid;
			}
		}
		else
			ip->ip_id = htons(ip_id++);
#else
		if ((fin->fin_fl & FI_FRAG) && (ipfr_nat_knownfrag(ip, fin)==NULL) && np)
		{
			ipfr_nat_newfrag(ip, fin, nat);
		}
#endif
		MUTEX_ENTER(&nat->nat_lock);
		if (fin->fin_p != IPPROTO_TCP) {
			if (np && np->in_age[1])
				nat->nat_age = np->in_age[1];
			else if (!icmpset && (fin->fin_p == IPPROTO_ICMP))
				nat->nat_age = fr_defnaticmpage;
			else
				nat->nat_age = fr_defnatage;
		}
		//nat->nat_bytes += ip->ip_len;
		//nat->nat_pkts++;
		MUTEX_EXIT(&nat->nat_lock);

		/*
		 * Fix up checksums, not by recalculating them, but
		 * simply computing adjustments.
		 */
		if (nat->nat_flags == IPN_ICMPERR) {
			u_32_t s1, s2, sumd;

			s1 = LONG_SUM(ntohl(fin->fin_saddr));
			s2 = LONG_SUM(ntohl(nat->nat_outip.s_addr));
			CALC_SUMD(s1, s2, sumd);

			if (nat->nat_dir == NAT_OUTBOUND)
				fix_outcksum(fin->fin_dlen, &ip->ip_sum, sumd);
			else
				fix_incksum(fin->fin_dlen, &ip->ip_sum, sumd);
		}

		/*
		 * Only change the packet contents, not what is filtered upon.
		 */
		ip->ip_src = nat->nat_outip;

		if ((fin->fin_off == 0) && !(fin->fin_fl & FI_SHORT)) {
			if ((nat->nat_outport != 0) && (tcp != NULL)) {
				tcp->th_sport = nat->nat_outport;
				fin->fin_data[0] = ntohs(tcp->th_sport);
			}
			/* 10/01/04 Roger ++,  */	
			if ((nat->nat_outport != 0) && (nat->nat_flags & IPN_ICMPQUERY)) {
				if(icmp != NULL)
				icmp->icmp_id = nat->nat_outport;
			}
			/* 10/01/04 Roger --,  */	
			if (fin->fin_p == IPPROTO_TCP) {
				csump = &tcp->th_sum;
				MUTEX_ENTER(&nat->nat_lock);
				fr_tcp_age(&nat->nat_age,
					   nat->nat_tcpstate, fin, 1, 0);
#if 0
				if (nat->nat_age < fr_defnaticmpage)
					nat->nat_age = fr_defnaticmpage;
#ifdef LARGE_NAT
				else if (nat->nat_age > fr_defnatage)
					nat->nat_age = fr_defnatage;
#endif
#endif
				/*
				 * Increase this because we may have
				 * "keep state" following this too and
				 * packet storms can occur if this is
				 * removed too quickly.
				 */
				//if (nat->nat_age == fr_tcpclosed)
				//	nat->nat_age = fr_tcplastack;

                                /*
                                 * Do a MSS CLAMPING on a SYN packet,
                                 * only deal IPv4 for now.
                                 */
								if(tcp != NULL) {
								if (nat->nat_mssclamp &&
                                    (tcp->th_flags & TH_SYN) != 0)
                                        nat_mssclamp(tcp, nat->nat_mssclamp,
						     fin, csump);
								}

				MUTEX_EXIT(&nat->nat_lock);
#ifdef CONFIG_NAT_FORCE_TCPWIN
				nat_adj_tcpwindow(tcp, fin->fin_dlen, csump);
#endif
			} else if (fin->fin_p == IPPROTO_UDP) {
				if(tcp == NULL) return -1;
				udphdr_t *udp = (udphdr_t *)tcp;

				if (udp->uh_sum)
					csump = &udp->uh_sum;
			/* 10/01/04 Roger ++,  */				
			} else if ( (ip->ip_p == IPPROTO_ICMP) && (nat->nat_flags & IPN_ICMPQUERY) ) {
				if(icmp != NULL){
				if (icmp->icmp_cksum)
					csump = &icmp->icmp_cksum;}
			
				nat->nat_age = fr_defnatage;
			/* 10/01/04 Roger --,  */	
			} else if (fin->fin_p == IPPROTO_GRE) 
				nat->nat_age = DEFAULT_NAT_LONG_AGE;

			if (csump) {
				if (nat->nat_dir == NAT_OUTBOUND)
					fix_outcksum(fin->fin_dlen, csump,
						     nat->nat_sumd[1]);
				else
					fix_incksum(fin->fin_dlen, csump,
						    nat->nat_sumd[1]);
			}
		}

		//if (np && (np->in_apr != NULL) && ((np->in_dport == 0 ||
		//     (tcp != NULL && dport == np->in_dport))||
		//     ((nat->nat_dir == NAT_INBOUND) && tcp != NULL && sport == np->in_dport))) {
		if (nat->nat_aps) {
			i = appr_check(ip, fin, nat);
			if (i == 0)
				i = 1;
			else if (i == -1)
				//nat->nat_drop[1]++;
				;
		} else
			i = 1;
		nat_check_fcache(nat, tcp);
		
		ATOMIC_INCL(nat_stats.ns_mapped[1]);
		RWLOCK_EXIT(&ipf_nat);	/* READ */
		//fin->fin_ifp = sifp;
		return i;
	}
	RWLOCK_EXIT(&ipf_nat);			/* READ/WRITE */
	//fin->fin_ifp = sifp;
	
	return 0;
}

/*
 * Packets coming in from the external interface go through this.
 * Here, the destination address requires alteration, if anything.
 */
int ip_natin(ip, fin)
ip_t *ip;
fr_info_t *fin;
{
	register struct in_addr src;
	register struct in_addr in;
	register ipnat_t *np;
	u_short sport = 0, dport = 0, *csump = NULL;
	u_int nflags = 0, natadd = 1, hv, msk;
	struct ifnet *ifp = fin->fin_ifp;
	tcphdr_t *tcp = NULL;
	icmphdr_t *icmp = NULL;
	int i, icmpset = 0;
	nat_t *nat;
	u_32_t iph;
	int natnew=0;
	
	if ((nat_list == NULL) || (ip->ip_v != 4) || (fr_nat_lock))
		return 0;
	if ((fin->fin_off == 0) && !(fin->fin_fl & FI_SHORT)) {
		if (fin->fin_p == IPPROTO_TCP)
			nflags = IPN_TCP;
		else if (fin->fin_p == IPPROTO_UDP)
			nflags = IPN_UDP;
		else if (fin->fin_p == IPPROTO_ICMP) {
		
			icmp = (icmphdr_t *)fin->fin_dp;

			/* This is an incoming packet, so the destination is
			 * the icmp_id and the source port equals 0
			 */
			if (check_icmpquerytype4(icmp->icmp_type)) {
				nflags = IPN_ICMPQUERY;
				dport = icmp->icmp_id;	
				sport = 0;
			}
		}
		if ((nflags & IPN_TCPUDP)) {
			tcp = (tcphdr_t *)fin->fin_dp;
			sport = tcp->th_sport;
			dport = tcp->th_dport;
		}
	}

	in = fin->fin_dst;
	/* make sure the source address is to be redirected */
	src = fin->fin_src;
	
	READ_ENTER(&ipf_nat);
	/* 10/01/04 Roger ++,  */	
	//if ((fin->fin_p == IPPROTO_ICMP) &&
	//    (nat = nat_icmp(ip, fin, &nflags, NAT_INBOUND)))
	if ((ip->ip_p == IPPROTO_ICMP) && !(nflags & IPN_ICMPQUERY) &&
	    (nat = nat_icmperror(ip, fin, &nflags, NAT_INBOUND))){
	/* 10/01/04 Roger --,  */	
		icmpset = 1;
	}
	else if ((fin->fin_fl & FI_FRAG) &&
		 (nat = ipfr_nat_knownfrag(ip, fin)))
		natadd = 0;
	else if ((nat = nat_inlookup(fin, nflags|FI_WILDP|FI_WILDA,
				     (u_int)fin->fin_p, fin->fin_src, in, 1))) {
		//nflags = nat->nat_flags;
		//if ((nflags & (FI_W_SPORT|FI_W_DPORT)) != 0) {
			//if ((nat->nat_oport != sport) && (nflags & FI_W_DPORT))
				//nat->nat_oport = sport;
			//if ((nat->nat_outport != dport) &&
			//	 (nflags & FI_W_SPORT))
				//nat->nat_outport = dport;
			//nat->nat_flags &= ~(FI_W_SPORT|FI_W_DPORT);
			//nflags = nat->nat_flags;
			//nat_stats.ns_wilds--;
		//}
		/* 28/04/05 Roger ++,  TCP syn packet appear twice!! */	
		if(((nat->nat_flags & IPN_NOTABMOVE) == 0) && 
			(fin->fin_p == IPPROTO_TCP) && 
			((fin->fin_tcpf & TH_OPENING) == TH_SYN) && 
			((nat->nat_tcpstate)[0] != 2) &&
			!(((nat->nat_tcpstate)[0] == 0) && ((nat->nat_tcpstate)[1] == 0)))
		{
			RWLOCK_EXIT(&ipf_nat);			/* READ/WRITE */
			return 0;
		}
		if (((nat->nat_flags & IPN_NOTABMOVE) == 0) && ((nat->nat_flags & (FI_WILDP|FI_WILDA)) != 0)) {
			nat->nat_flags &= ~(FI_WILDP|FI_WILDA);
			nat_stats.ns_wilds--;
			natnew = 1;
		}
	} else {
	//} else if (!(nflags & IPN_ICMPQUERY)) { /* icmp query INBOUND can never create a new NAT entry */
		RWLOCK_EXIT(&ipf_nat);

		msk = 0xffffffff;
		i = 32;
		
		WRITE_ENTER(&ipf_nat);
		/*
		 * If there is no current entry in the nat table for this IP#,
		 * create one for it (if there is a matching rule).
		 */
		 
		// 2009-01-16 markt test only
		if(check_localport2(fin->fin_saddr, sport, fin->fin_daddr, dport, fin->fin_p) != 0)
		{
			fin->fin_misc |= FM_OURSPORT;
			goto test_exit;
		}
maskloop:
		iph = in.s_addr & htonl(msk);
		hv = NAT_HASH_FN(iph, 0, ipf_rdrrules_sz);
		//diag_printf("ip_natin: hv=%d, dport=%x np:%x in.s_addr=%x\n", hv, dport, rdr_rules[hv], in.s_addr);
		for (np = rdr_rules[hv]; np; np = np->in_rnext) {
			if ((np->in_ifp && (np->in_ifp != ifp)) ||
			    (np->in_p && (np->in_p != fin->fin_p)) ||
			    ((np->in_flags&IPN_RF) && !(nflags & (np->in_flags&IPN_RF))))
				continue;
			if(np->in_flags & IPN_INACTIVE_SCHED)
				continue;
			if((nflags & IPN_DMZ) != (np->in_flags & IPN_DMZ))
				continue;
			if(((np->in_flags & IPN_TRIG) ^ np->in_triguse))
				continue;
			if (np->in_flags & IPN_FILTER) {
				if (!nat_match(fin, np, ip))
					continue;
			} else if ((in.s_addr & np->in_outmsk) != np->in_outip)
				continue;			
			if ((!np->in_pmin || (np->in_flags & IPN_FILTER) ||
			     ((ntohs(np->in_pmax) >= ntohs(dport)) &&
			      (ntohs(dport) >= ntohs(np->in_pmin))) ||
			      ((np->in_flags & IPN_MULTIP)&& multip_match(dport, np))))
				{
					if ((nat = nat_new(fin, ip, np, NULL, nflags,
							    NAT_INBOUND))) {
						np->in_hits++;
						natnew = 1;
						break;
					}
					else
					{
						fin->fin_misc |= FM_BADNAT;
						break;
					}
				}
		}

		if ((np == NULL) && (i > 0)) {
			do {
				i--;
				msk <<= 1;
			} while ((i >= 0) && ((rdr_masks & (1 << i)) == 0));
			if (i >= 0)
				goto maskloop;
		}

		/* 11/05/04 Roger ++,  */
		if(nat == NULL)
		{
			if(fin->fin_p == IPPROTO_ICMP)
			/* we consider ICMP packets belong ours */
			;
			else if(check_localport(fin->fin_daddr, dport, fin->fin_p) != 0)
			{
				fin->fin_misc |= FM_OURSPORT;
			}
			else if((nflags & IPN_DMZ) == 0)
			{
				nflags |= IPN_DMZ;
				msk = 0xffffffff;
				
				goto maskloop;
			}
		}
test_exit:
	/* 11/05/04 Roger --,  */	
		MUTEX_DOWNGRADE(&ipf_nat);
	}

	/*
	 * NOTE: ipf_nat must now only be held as a read lock
	 */
	if (nat) {
		/* 31/08/04 Roger ++,  when reflect traffic, we do not reponse ICMP redirect message */	
		if(fin->fin_ifp == GETUNIT(SYS_lan_if, 4))
		{
			if(natnew)
			{
				nat_t *ref_nat;
				ref_nat = nat_reflect_rule(fin, nat, nflags);
				if(ref_nat && nat->nat_aps)
				{
					appr_new(fin, ref_nat, appr_find(fin->fin_p, fin->fin_data[1]));
				}
			}
			mb_t *m;
			m = *((mb_t **)fin->fin_mp);
			m->m_flags |= M_FORCEFORDWARD;
			//m->m_pkthdr.rcvif = 0;
		}
		/* 31/08/04 Roger --,  */
		
		np = nat->nat_ptr;
		fin->fin_fr = nat->nat_fr;
		if (natadd && (fin->fin_fl & FI_FRAG) && np)
			ipfr_nat_newfrag(ip, fin, nat);
		MUTEX_ENTER(&nat->nat_lock);
		
		if (fin->fin_p != IPPROTO_TCP) {
			if (np && np->in_age[0])
				nat->nat_age = np->in_age[0];
			else if (!icmpset && (fin->fin_p == IPPROTO_ICMP))
				nat->nat_age = fr_defnaticmpage;
			else
				nat->nat_age = fr_defnatage;
		}
		/* 07/05/04 Roger ++,  */	
		//if (np && (np->in_apr != NULL) && ((np->in_dport == 0 ||
		//     (tcp != NULL && sport == np->in_dport) 
		//     || ((nat->nat_dir == NAT_INBOUND) && tcp != NULL && dport == np->in_dport)))) {
		if (nat->nat_aps) {
			i = appr_check(ip, fin, nat);
			if (i == -1) {
				//nat->nat_drop[0]++;
				RWLOCK_EXIT(&ipf_nat);
				return i;
			}
		}
		/* 07/05/04 Roger --,  */	
		//nat->nat_bytes += ip->ip_len;
		//nat->nat_pkts++;
		MUTEX_EXIT(&nat->nat_lock);
		ip->ip_dst = nat->nat_inip;
		fin->fin_fi.fi_daddr = nat->nat_inip.s_addr;

		/*
		 * Fix up checksums, not by recalculating them, but
		 * simply computing adjustments.
		 */
		if ((fin->fin_off == 0) && !(fin->fin_fl & FI_SHORT)) {

			//if ((nat->nat_inport != 0) && (tcp != NULL)) {
			if (tcp != NULL) {
				tcp->th_dport = nat->nat_inport;
				fin->fin_data[1] = ntohs(tcp->th_dport);
			}
			/* 10/01/04 Roger ++,  */	
			//if ((nat->nat_inport != 0) && (nflags & IPN_ICMPQUERY)) {
			else if (nflags & IPN_ICMPQUERY) {
				if(icmp != NULL)
				icmp->icmp_id = nat->nat_inport;
			}
			/* 10/01/04 Roger --,  */	
			if (fin->fin_p == IPPROTO_TCP) {
				csump = &tcp->th_sum;
				MUTEX_ENTER(&nat->nat_lock);
				fr_tcp_age(&nat->nat_age, nat->nat_tcpstate, fin, 0, 0);
#if 0
				if (nat->nat_age < fr_defnaticmpage)
					nat->nat_age = fr_defnaticmpage;
#ifdef LARGE_NAT
				else if (nat->nat_age > fr_defnatage)
					nat->nat_age = fr_defnatage;
#endif
#endif
				/*
				 * Increase this because we may have
				 * "keep state" following this too and
				 * packet storms can occur if this is
				 * removed too quickly.
				 */
				//if (nat->nat_age == fr_tcpclosed)
				//	nat->nat_age = fr_tcplastack;
                                /*
                                 * Do a MSS CLAMPING on a SYN packet,
                                 * only deal IPv4 for now.
                                 */
								if(tcp != NULL){
								if (nat->nat_mssclamp &&
                                    (tcp->th_flags & TH_SYN) != 0)
                                        nat_mssclamp(tcp, nat->nat_mssclamp,
						     fin, csump);
									}

				MUTEX_EXIT(&nat->nat_lock);
			} else if (fin->fin_p == IPPROTO_UDP) {
				if(tcp == NULL) return -1;
				udphdr_t *udp = (udphdr_t *)tcp;

				if (udp->uh_sum)
					csump = &udp->uh_sum;
			/* 10/01/04 Roger ++,  */	
			} else if ( (ip->ip_p == IPPROTO_ICMP) && (nflags & IPN_ICMPQUERY) ) {
				if(icmp != NULL){
				if (icmp->icmp_cksum)
					csump = &icmp->icmp_cksum;
					}
			/* 10/01/04 Roger --,  */	
			}else if (fin->fin_p == IPPROTO_GRE) 
				nat->nat_age = DEFAULT_NAT_LONG_AGE;

			if (csump) {
				if (nat->nat_dir == NAT_OUTBOUND)
					fix_incksum(fin->fin_dlen, csump,
						    nat->nat_sumd[0]);
				else
					fix_outcksum(fin->fin_dlen, csump,
						    nat->nat_sumd[0]);
			}
		}
		nat_check_fcache(nat, tcp);
		
		ATOMIC_INCL(nat_stats.ns_mapped[0]);
		RWLOCK_EXIT(&ipf_nat);			/* READ */
		fin->fin_nat = nat;
		return 1;
	}
	RWLOCK_EXIT(&ipf_nat);			/* READ/WRITE */
	return 0;
}

int ip_natoff(ip_t *ip, fr_info_t *fin)
{
	tcphdr_t *tcp = NULL;
	u_short *csump=NULL;
	u_short max_mss;
	struct ifnet *ifp;
	
	if (nat_list != NULL)
		return 0;
	
	if (fin->fin_p == IPPROTO_TCP)
	{
		tcp = (tcphdr_t *)fin->fin_dp;
		
		// Do a MSS CLAMPING on a SYN packet
		if ((tcp->th_flags & TH_SYN) != 0)
		{
			ifp = fin->fin_ifp;
			csump = &tcp->th_sum;
			max_mss = ifp->if_mtu - 40;
			nat_mssclamp(tcp, max_mss, fin, csump);
		}
	}
	return 0;
}

nat_t * nat_reflect_rule(fr_info_t *fin, nat_t *onat, int flags)
{
	nat_t *nat, *natl;
	int trynum=0;
	u_short sp, dp, port;
	
	nat = nat_alloc(NAT_OUTBOUND, flags);
	
	if (nat == NULL) {
		nat_stats.ns_memfail++;
		return NULL;
	}
	
	nat->nat_ifp = fin->fin_ifp;

	sp = fin->fin_data[0];
	dp = fin->fin_data[1];
	fin->fin_data[0] = ntohs(onat->nat_inport);
//	port = ntohs(onat->nat_oport);
	NATDBG("natref: finsrc=(%08x/%d), findst=(%08x/%d), in=(%08x/%d), map=(%08x/%d),dst=(%08x/%d)\n",
		ntohl(fin->fin_src.s_addr), sp, ntohl(fin->fin_dst.s_addr), dp, 
		ntohl(onat->nat_inip.s_addr), ntohs(onat->nat_inport), ntohl(onat->nat_outip.s_addr),
		ntohs(onat->nat_outport), ntohl(onat->nat_oip.s_addr), ntohs(onat->nat_oport));
	
	do{
#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
		if ((trynum == 0) && (port=nat_get_mappedport(fin,  fin->fin_src)) != 0) {
		} else 
#endif	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
		
		{
			port = ntohs(onat->nat_oport);
			port += (trynum*(cyg_current_time() & 0xffff));
			port %= USABLE_PORTS; 
			port += MAPBLK_MINPORT;
			port = htons(port);
		}
		
		fin->fin_data[1] = ntohs(port);
#ifdef CONFIG_NAT_PORT_RESTRICTED_CONE
		natl = (nat_t *) nat_check_port_collision(fin->fin_src.s_addr, sp, htonl(onat->nat_outip.s_addr), port);
#else
		natl = nat_inlookup(fin, flags & ~(FI_WILDP|FI_WILDA), (u_int)onat->nat_p, onat->nat_inip, onat->nat_outip, 0);
#endif	/*  CONFIG_NAT_PORT_RESTRICTED_CONE  */
		trynum++;
	} while(natl && trynum<10);
	fin->fin_data[0] = sp;
	fin->fin_data[1] = dp;
				
	nat_setup(nat, onat->nat_oip.s_addr, onat->nat_inip.s_addr, onat->nat_outip.s_addr, onat->nat_p, onat->nat_oport, onat->nat_inport, ntohs(port));
	return nat;
}

/*
 * Free all memory used by NAT structures allocated at runtime.
 */
void ip_natunload()
{
	WRITE_ENTER(&ipf_nat);
	(void) nat_clearlist();
	(void) nat_flushtable();
	RWLOCK_EXIT(&ipf_nat);

	if (nat_table[0] != NULL) {
		KFREES(nat_table[0], sizeof(nat_t *) * ipf_nattable_sz);
		nat_table[0] = NULL;
	}
	if (nat_table[1] != NULL) {
		KFREES(nat_table[1], sizeof(nat_t *) * ipf_nattable_sz);
		nat_table[1] = NULL;
	}
	if (nat_rules != NULL) {
		KFREES(nat_rules, sizeof(ipnat_t *) * ipf_natrules_sz);
		nat_rules = NULL;
	}
	if (rdr_rules != NULL) {
		KFREES(rdr_rules, sizeof(ipnat_t *) * ipf_rdrrules_sz);
		rdr_rules = NULL;
	}
	if (maptable != NULL) {
		KFREES(maptable, sizeof(hostmap_t *) * ipf_hostmap_sz);
		maptable = NULL;
	}
	if(nat_pool != NULL) {
#undef free
		free(nat_pool);
		nat_pool = NULL;
		free_nat = NULL;
	}
}


/*
 * Slowly expire held state for NAT entries.  Timeouts are set in
 * expectation of this being called twice per second.
 */
void ip_natexpire()
{
	register struct nat *nat, *natpre=0, **natp;
	int s;

	SPL_NET(s);
	WRITE_ENTER(&ipf_nat);
	for (natp = &nat_instances; (nat = *natp); ) {
		nat->nat_age--;
		if (nat->nat_age && !(nat->nat_flags & IPN_DELETE)) {
			natp = &nat->nat_next;
			natpre = nat;
			continue;
		}
		*natp = nat->nat_next;
		if(natpre)
			natpre->nat_next = *natp;
		else
			nat_instances = *natp;
		
#ifdef	IPFILTER_LOG
		nat_log(nat, NL_EXPIRE);
#endif
		nat_delete(nat);
		nat_stats.ns_expire++;
	}
	RWLOCK_EXIT(&ipf_nat);
	SPL_X(s);
}

void speedup_natexpire(void)
{
	register struct nat *nat, **natp;
	int s, num;

	SPL_NET(s);
	WRITE_ENTER(&ipf_nat);
	for (natp = &nat_instances, num=0; (nat = *natp); num++) {
		if(num > nat_table_max/2)
		{
			switch(nat->nat_p)
			{
				case IPPROTO_UDP:
					if(nat->nat_age < fr_defnatage/2)
						nat->nat_age = 1;
					break;
				case IPPROTO_TCP:
					if((nat->nat_age > fr_tcptimeout)&&(nat->nat_age < fr_tcpidletimeout*3/4))
						nat->nat_age = 1;
					break;
				case IPPROTO_ICMP:
					nat->nat_age = 1;
					break;
				default:
					break;
			}
		}
		natp = &nat->nat_next;
	}
	RWLOCK_EXIT(&ipf_nat);
	SPL_X(s);
}

#if 0
/*
 */
void ip_natsync(ifp)
void *ifp;
{
	register ipnat_t *n;
	register nat_t *nat;
	register u_32_t sum1, sum2, sumd;
	struct in_addr in;
	ipnat_t *np;
	void *ifp2;
	int s;

	/*
	 * Change IP addresses for NAT sessions for any protocol except TCP
	 * since it will break the TCP connection anyway.
	 */
	SPL_NET(s);
	WRITE_ENTER(&ipf_nat);
	for (nat = nat_instances; nat; nat = nat->nat_next)
		if (((ifp == NULL) || (ifp == nat->nat_ifp)) &&
		    !(nat->nat_flags & IPN_TCP) && (np = nat->nat_ptr) &&
		    (np->in_outmsk == 0xffffffff) && !np->in_nip) {
			ifp2 = nat->nat_ifp;
			/*
			 * Change the map-to address to be the same as the
			 * new one.
			 */
			sum1 = nat->nat_outip.s_addr;
			if (fr_ifpaddr(4, ifp2, &in) != -1)
				nat->nat_outip = in;
			sum2 = nat->nat_outip.s_addr;

			if (sum1 == sum2)
				continue;
			/*
			 * Readjust the checksum adjustment to take into
			 * account the new IP#.
			 */
			CALC_SUMD(sum1, sum2, sumd);
			/* XXX - dont change for TCP when solaris does
			 * hardware checksumming.
			 */
			sumd += nat->nat_sumd[0];
			nat->nat_sumd[0] = (sumd & 0xffff) + (sumd >> 16);
			nat->nat_sumd[1] = nat->nat_sumd[0];
		}

	for (n = nat_list; (n != NULL); n = n->in_next)
		if (n->in_ifp == ifp) {
			n->in_ifp = (void *)GETUNIT(n->in_ifname, 4);
			if (!n->in_ifp)
				n->in_ifp = (void *)-1;
		}
	RWLOCK_EXIT(&ipf_nat);
	SPL_X(s);
}
#endif

#ifdef	IPFILTER_LOG
void nat_log(nat, type)
struct nat *nat;
u_int type;
{
	struct ipnat *np;
	struct natlog natl;
	void *items[1];
	size_t sizes[1];
	int rulen, types[1];

	natl.nl_inip = nat->nat_inip;
	natl.nl_outip = nat->nat_outip;
	natl.nl_origip = nat->nat_oip;
	//natl.nl_bytes = nat->nat_bytes;
	//natl.nl_pkts = nat->nat_pkts;
	natl.nl_origport = nat->nat_oport;
	natl.nl_inport = nat->nat_inport;
	natl.nl_outport = nat->nat_outport;
	natl.nl_p = nat->nat_p;
	natl.nl_type = type;
	natl.nl_rule = -1;
#ifndef LARGE_NAT
	if (nat->nat_ptr != NULL) {
		for (rulen = 0, np = nat_list; np; np = np->in_next, rulen++)
			if (np == nat->nat_ptr) {
				natl.nl_rule = rulen;
				break;
			}
	}
#endif	/*  LARGE_NAT  */
	items[0] = &natl;
	sizes[0] = sizeof(natl);
	types[0] = 0;

	(void) ipllog(IPL_LOGNAT, NULL, items, sizes, types, 1);
}
#endif	/*  IPFILTER_LOG  */

/* 10/01/04 Roger ++,  */	
static int	check_icmpquerytype4(icmptype)
int icmptype;
{

	/*
	 * For the ICMP query NAT code, it is essential that both the query
	 * and the reply match on the NAT rule. Because the NAT structure
	 * does not keep track of the icmptype, and a single NAT structure
	 * is used for all icmp types with the same src, dest and id, we
	 * simply define the replies as queries as well. The funny thing is,
	 * altough it seems silly to call a reply a queyr, this is exactly
	 * as it is defined in the IPv4 specification
	 */
	 
	switch (icmptype) { 
	
		case ICMP_ECHOREPLY:
		case ICMP_ECHO:
		/* route aedvertisement/solliciation is currently unsupported:
		 * it would require rewriting the ICMP data section */
		case ICMP_TSTAMP:
		case ICMP_TSTAMPREPLY:
		case ICMP_IREQ:
		case ICMP_IREQREPLY:
		case ICMP_MASKREQ:
		case ICMP_MASKREPLY:
			return 1;
		default: 
			return 0;
	}
}

static int	check_icmperrortype4(icmptype)
int icmptype;
{

	switch (icmptype) { 

		case ICMP_UNREACH:
		case ICMP_SOURCEQUENCH:
		//only NAT router need to know redirect to new GW
		//case ICMP_REDIRECT:
		case ICMP_TIMXCEED:
		case ICMP_PARAMPROB:
			return 1;
		default: 
			return 0;
	}
}
/* 10/01/04 Roger --,  */	
#if 0
#if defined(__OpenBSD__)
void nat_ifdetach(ifp)
void *ifp;
{
	frsync();
	return;
}
#endif
#endif

/*
 * Check for MSS option and clamp it if necessary.
 */
static void nat_mssclamp(tcp, maxmss, fin, csump)
tcphdr_t *tcp;
u_short maxmss;
fr_info_t *fin;
u_short *csump;
{
	u_char *cp, *ep, opt;
	int hlen, advance;
	u_32_t sumd;
	u_short v, mss;

	hlen = tcp->th_off << 2;
	if (hlen > sizeof(*tcp)) {
		cp = (u_char *)tcp + sizeof(*tcp);
		ep = (u_char *)tcp + hlen;

		while (cp < ep) {
			opt = cp[0];
			if (opt == TCPOPT_EOL)
				break;
			else if (opt == TCPOPT_NOP) {
				cp++;
				continue;
			}
 
			if (cp + 1 >= ep)
				break;
			if((advance = cp[1]) == 0)
				break;
			if (cp + advance > ep)
				break;
			switch (opt) {
			case TCPOPT_MAXSEG:
				if (advance != 4)
					break;
				bcopy(&cp[2], &v, sizeof(v));
				mss = ntohs(v);
				if (mss > maxmss) {
					v = htons(maxmss);
					bcopy(&v, &cp[2], sizeof(v));
					CALC_SUMD(mss, maxmss, sumd);
					fix_outcksum(fin->fin_dlen, csump, sumd);
				}
				goto out;
			default:
				/* ignore unknown options */
				break;
			}
		    
			cp += advance;  
		}       
out:     
	return ;
	}	
}		       


/*
 * Roger add ip_triggerexpire()
 * check trigger age to diable trigger rule
 */	
void ip_triggerexpire(void)
{
	ipnat_t *np;
	int s;

	SPL_NET(s);
	for(np = trig_rules ; np ; np = np->in_trignext) {
		if(np->in_triguse) {
			if( (-- np->in_trigage) == 0 )
				np->in_triguse &= ~IPN_TRIG;
		}
	}
	SPL_X(s);
}

int nat_addstate(ip, fin)
ip_t *ip;
fr_info_t *fin;
{
	nat_t *nat;
	
	if((nat = fin->fin_nat) == NULL)
		return 0;
	nat->nat_fstate.rule = fin->fin_fr;
	return 1;
}

frentry_t *nat_checkstate(ip, fin)
ip_t *ip;
fr_info_t *fin;
{
	nat_t *nat;
	
	if((nat = fin->fin_nat) == NULL)
		return 0;
	return (nat->nat_fstate.rule);
}


