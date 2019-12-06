#include "dnsmasq.h"
#include <cfg_def.h> 
#include <cfg_dns.h>
#include <network.h>
#include <ctype.h>

// Interface
extern IFENTRY recv_infaces[MAXRECVINFACE];
extern int recv_infaces_num;

// Hosts
HOSTSENTRY hosts_entry[MAXHOSTSENTRY];
int hosts_entry_num;

// Servers
extern DNSSERVER dns_servers[MAXDNSSERVER];
extern int dns_servers_num;

// Forward list
static FORWARDENTRY *forward_list = NULL;

static char dname[MAXDNAME];
static char local_domain[256];

//
static FORWARDENTRY *forwardEntryfind(unsigned short id, MYSOCKADDR *addr, char *dnsname);
static int serverCreatefd(DNSSERVER *server);
static void forwardListdump(int all);

static int msgParsename(DNSHEADER *dnsheader, unsigned int plen, unsigned char **pp, char *name, int isExtract)
{
	char *cp = name;
	unsigned char *p = *pp, *p1 = NULL;
	unsigned int j, l, hops = 0;
	int retvalue = 1;

	while ((l = *p++))
	{
		if ((l & 0xc0) == 0xc0) /* pointer */
		{ 
			if (p - (unsigned char *)dnsheader + 1u >= plen)
				return 0;

			/* get offset */
			l = (l&0x3f) << 8;
			l |= *p++;
			if (l >= (unsigned int)plen) 
				return 0;

			if (!p1) /* first jump, save location to go back to */
				p1 = p;
			    
			hops++; /* break malicious infinite loops */
			if (hops > 255)
				return 0;

			p = l + (unsigned char *)dnsheader;
		}
		else
		{
			if (cp-name+l+1 >= MAXDNAME)
				return 0;
			if (p - (unsigned char *)dnsheader + l >= plen)
				return 0;
			for(j=0; j<l; j++, p++)
				if (isExtract)
				{
					char c = *p;
					/* check for legal char a-z A-Z 0-9 - */
					if (!isalnum(c) && c != '-' && c != '/')
						return 0;
					*cp++ = c;
				}
				else 
				{
					char c = *cp++;
					if (!c || tolower(c) != tolower(*p))
						retvalue =  2;
				}

			if (isExtract)
				*cp++ = '.';
			else
				if (*cp && *cp++ != '.')
					retvalue = 2;
		}

		if ((unsigned int)(p - (unsigned char *)dnsheader) >= plen)
			return 0;
	}

	if (isExtract)
		*--cp = 0; /* terminate: lose final period */

	if (p1) /* we jumped via compression */
		*pp = p1;
	else
		*pp = p;

	return retvalue;
}

static unsigned short msgParserequest(DNSHEADER *dnsheader, unsigned int qlen, char *name)
{
	unsigned char *p = (unsigned char *)(dnsheader+1);
	int qtype, qclass;

	if (ntohs(dnsheader->qdcount) != 1 || dnsheader->opcode != QUERY)
		return 0; /* must be exactly one query. */

	if (!msgParsename(dnsheader, qlen, &p, name, 1))
		return 0; /* bad packet */

	GETSHORT(qtype, p); 
	GETSHORT(qclass, p);

	if (qclass == C_IN)
	{
		if (qtype == T_A)
			return FLAG_IPV4;
	    if (qtype == T_ANY)
			return  FLAG_IPV4;
	}

	return FLAG_QUERY;
}

static unsigned char *msgFindanswerposition(DNSHEADER *dnsheader, unsigned int plen)
{
	int q, qdcount = ntohs(dnsheader->qdcount);
	unsigned char *ansp = (unsigned char *)(dnsheader+1);

	for (q=0; q<qdcount; q++)
	{
		while (1)
		{
			if ((unsigned int)(ansp - (unsigned char *)dnsheader) >= plen)
				return NULL;
			if (((*ansp) & 0xc0) == 0xc0) /* pointer for name compression */
			{
				ansp += 2;	
				break;
			}
			else if (*ansp) 
			{
				/* another segment */
				ansp += (*ansp) + 1;
			}
			else
			{
				/* end */
				ansp++;
				break;
			}
		}
		ansp += 4; /* class and type */
	}

	if ((unsigned int)(ansp - (unsigned char *)dnsheader) > plen) 
		return NULL;

	return ansp;
}

static int msgBuildreply(DNSHEADER *dnsheader, unsigned int qlen,
		MYADDR *addrp, unsigned short flags)
{
	unsigned long ttl = 0;
	unsigned char *p = msgFindanswerposition(dnsheader, qlen);

	dnsheader->qr = 1; /* response */
	dnsheader->aa = 0; /* authoritive */
	dnsheader->ra = 1; /* recursion if available */
	dnsheader->tc = 0; /* not truncated */
	dnsheader->nscount = htons(0);
	dnsheader->arcount = htons(0);
	dnsheader->ancount = htons(0); /* no answers unless changed below*/
	if (flags == FLAG_NEG)
		dnsheader->rcode = SERVFAIL; /* couldn't get memory */
	else if (flags == FLAG_NOERR)
		dnsheader->rcode = NOERROR; /* empty domain */
	else if (p && flags == FLAG_IPV4)
	{
		/* we know the address */
		dnsheader->rcode = NOERROR;
		dnsheader->ancount = htons(1);
		dnsheader->aa = 1;
		PUTSHORT (sizeof(DNSHEADER) | 0xc000, p);
		PUTSHORT(T_A, p);
		PUTSHORT(C_IN, p);
		PUTLONG(ttl, p);
		PUTSHORT(INADDRSZ, p);
		memcpy(p, addrp, INADDRSZ);
		p += INADDRSZ;
	}
	else /* nowhere to forward to */
		dnsheader->rcode = REFUSED;
 
  return p - (unsigned char *)dnsheader;
}

static void get_localdomain(char *domain)
{
	char name[256],dn[256];

	CFG_get_str(CFG_SYS_NAME,name) ; 
	CFG_get_str(CFG_SYS_DOMAIN,dn) ;
	local_domain[0]=0;
	if ((strlen(name)+strlen(dn))<256)
		sprintf(local_domain,"%s.%s",name,dn);
}

static int sa_len(MYSOCKADDR *addr)
{
#ifdef HAVE_SOCKADDR_SA_LEN
	return addr->sa.sa_len;
#else
	return sizeof(addr->in); 
#endif
}

static unsigned short rand16(void)
{
	static int been_seeded = 0;
	
	if (! been_seeded) 
	{
		unsigned int seed = 0, badseed;
		struct timeval now;
		
		/* get the bad seed as a backup */
		/* (but we'd rather have something more random) */
		gettimeofday(&now, NULL);

#ifndef	__ECOS
		badseed = now.tv_sec ^ now.tv_usec ^ (getpid() << 16);
#else
		badseed = now.tv_sec ^ now.tv_usec ^ (((int)cyg_thread_self()) << 16);
#endif

		srand(seed);
		been_seeded = 1;
	}
	/* Some rand() implementations have less randomness in low bits
	 * than in high bits, so we only pay attention to the high ones.
	 * But most implementations don't touch the high bit, so we 
	 * ignore that one.
	 */
	return( (unsigned short) (rand() >> 15) );
}

static unsigned short getnid(void)
{
	unsigned short nid = 0;

	while(1)
	{
		nid = rand16();

		if(nid && !forwardEntryfind(nid, NULL, NULL))
			break;
	}

	return nid;
}

static int hostnamecmp(char *a, char *b)
{
	int c1, c2;

	do
	{
		c1 = *a++;
		c2 = *b++;
		
		if (c1 >= 'A' && c1 <= 'Z')
			c1 += 'a' - 'A';
		if (c2 >= 'A' && c2 <= 'Z')
			c2 += 'a' - 'A';

		if (c1 != c2)
			return 0;
	} while (c1);

	return 1;
}

static int sockaddrcmp(MYSOCKADDR *s1, MYSOCKADDR *s2)
{
	if (s1->sa.sa_family == s2->sa.sa_family)
	{
		if (s1->sa.sa_family == AF_INET &&
		   /* s1->in.sin_port == s2->in.sin_port && */ //yfchou mark;
		   memcmp(&s1->in.sin_addr, &s2->in.sin_addr, sizeof(struct in_addr)) == 0)
			return 1;
	}
	return 0;
}

//
// Interface
// interfaceGet
//
static int interfaceGet(int port)
{
	int len = 10 * sizeof(struct ifreq);
	int i, fd, on, lastlen = 0;
	char *buf, *ptr;
	struct ifconf ifc;
	static struct ifreq *ifr;
	struct sockaddr *sa;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd < 0)
	{
		diag_printf("%s(): Line%d Can not create socket!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	// clear recv_infaces data
	for (i=0; i<MAXRECVINFACE; i++)
	{
		recv_infaces[i].valid = 0;
		if (recv_infaces[i].fd > 0)
		{
			close(recv_infaces[i].fd);
			recv_infaces[i].fd = -1;
		}
	}
	recv_infaces_num = 0;

	while (1)
	{
		if (!(buf = malloc(len)))
		{
			diag_printf("%s(): Line%d Can not allocate buffer!\n", __FUNCTION__, __LINE__);
			close(fd);
			return -1;
		}

		memset( buf, 0, len );	
		ifc.ifc_len = len;
		ifc.ifc_buf = buf;

		if (ioctl(fd, SIOCGIFCONF, &ifc) < 0)
		{
			if (errno != EINVAL || lastlen != 0)
			{
				diag_printf("%s(): Line%d ioctl error!\n", __FUNCTION__, __LINE__);
				close(fd);
				free(buf);
				return -1;
			}
		}
		else
		{
			if (ifc.ifc_len == lastlen)
				break; // * got a all ifnet data */
			lastlen = ifc.ifc_len;
		}
		len += 10*sizeof(struct ifreq);
		free(buf);
	}
	close(fd);

	ifr = ifc.ifc_req;
	for (ptr = buf; ptr < buf + ifc.ifc_len; )
 	{
		MYSOCKADDR addr;

		//copy address since getting flags overwrites */
		if (ifr->ifr_addr.sa_family == AF_INET)
		{
			char ipbuf[32];

			addr.in = *((struct sockaddr_in *) &ifr->ifr_addr);
			addr.in.sin_port = htons(port);

			// add new interface.
			fd = socket(addr.sa.sa_family, SOCK_DGRAM, 0);
			if (fd == -1)
			{
				diag_printf("%s(): Line%d create socket error!\n", __FUNCTION__, __LINE__);
				free(buf);
				return -1;
			}

			on = 1;
			setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

			if (bind(fd, &addr.sa, sa_len(&addr)))
			{
				close(fd);
				strcpy(ipbuf , inet_ntoa (addr.in.sin_addr));
				diag_printf("%s(): Line%d bind socket error on %s:%d!\n", __FUNCTION__, __LINE__, ipbuf, ntohs(addr.in.sin_port));
				free(buf);
				return -1;
			}

			recv_infaces[recv_infaces_num].fd = fd;
			recv_infaces[recv_infaces_num].addr = addr;
			recv_infaces[recv_infaces_num].valid = 1;
			recv_infaces_num++;
                        strcpy(ipbuf , inet_ntoa (addr.in.sin_addr));
                        diag_printf("%s(): bind socket successful on %s:%d\n", __FUNCTION__, ipbuf, ntohs(addr.in.sin_port));
		}

		do
		{
                        sa = &ifr->ifr_addr;
                        if (sa->sa_len <= sizeof(*sa))
                        {
                                ifr++;
                        }
			else
			{
				ifr=(struct ifreq *)(sa->sa_len + (char *)sa);
				ifc.ifc_len -= sa->sa_len - sizeof(*sa);
			}
			ifc.ifc_len -= sizeof(*ifr);
		}while (!ifr->ifr_name[0] && ifc.ifc_len);
	}

	if (buf)
                free(buf);

	return 0;
}

//
// Host
// hostsInit
//
static int hostsInit(void)
{
	int i;
	char linebuf[256];
	int *arglists[MAX_DNS_ARGS];

	// clear all
	for (i=0; i<MAXHOSTSENTRY; i++)
		hosts_entry[i].name[0] = '\0';

	hosts_entry_num = 0;

	// read hosts setting to hosts_entry
	for(i=0; i< MAXHOSTSENTRY; i++)
	{
		if (CFG_get(CFG_DNS_HOSTS+1+i, linebuf) == -1)
			continue;
		if((str2arglist(linebuf, arglists, ';', 3) == 3) && (atoi(arglists[2]) == 1))
		{
			strcpy(hosts_entry[hosts_entry_num].name, arglists[0]);
			inet_aton(arglists[1], &hosts_entry[hosts_entry_num].addr.addr);

                        strcpy(linebuf , inet_ntoa (hosts_entry[hosts_entry_num].addr.addr.addr4));
                        diag_printf("%s(): [%d] %s %s\n", __FUNCTION__, hosts_entry_num,
                                hosts_entry[hosts_entry_num].name, linebuf);

			hosts_entry_num++;
		}
	}

	return 0;
}

static HOSTSENTRY * hostsEntryfind(char *dnsname)
{
	int i;

	for (i=0; i<hosts_entry_num; i++)
	{
		if ( hostnamecmp(hosts_entry[i].name, dnsname))
			return &hosts_entry[i];
	}

	return NULL;
}

//
// Server
// serverInit
//
static int serverInit(int port)
{
	char ipa[20];
        char ipbuf[32];
	int i;

	// clear dns_servers table
	for(i=0; i<MAXDNSSERVER; i++)
	{
		if (dns_servers[i].fd > 0)
		{
			close (dns_servers[i].fd);
			dns_servers[i].fd = -1;
		}
	}
	dns_servers_num = 0;

	// add new servers.
	for(i=0; i<MAXDNSSERVER; i++)
	{
                if (DNS_group_get(i) != 0)
                {   			
			dns_servers[i].flags = 0;
			dns_servers[i].fd = -1;
			dns_servers[i].domain = NULL;
			dns_servers[i].addr.in.sin_addr.s_addr = DNS_group_get(i);
			dns_servers[i].addr.in.sin_port = htons(port);
			dns_servers[i].addr.sa.sa_family = dns_servers[i].source_addr.sa.sa_family = AF_INET;
			dns_servers[i].source_addr.in.sin_len = dns_servers[i].addr.in.sin_len = sizeof(struct sockaddr_in);
			dns_servers[i].source_addr.in.sin_addr.s_addr = INADDR_ANY;
			serverCreatefd(&dns_servers[i]);
                        strcpy(ipbuf , inet_ntoa (dns_servers[i].addr.in.sin_addr));
                        diag_printf("%s(): DNS Server List %d. %s:%d\n", __FUNCTION__, i, ipbuf, ntohs(dns_servers[i].addr.in.sin_port));
			dns_servers_num++;                        
                }			
	}

	//build local host
	dns_servers[dns_servers_num].flags = SERV_NO_ADDR|SERV_LITERAL_ADDRESS;
	dns_servers[dns_servers_num].fd = -1;
	dns_servers[dns_servers_num].domain = NULL;
	CFG_get_str(CFG_LAN_IP, ipa);
	dns_servers[dns_servers_num].addr.in.sin_addr.s_addr = inet_addr(ipa);
	dns_servers[dns_servers_num].addr.in.sin_port = htons(port);
	dns_servers[dns_servers_num].addr.sa.sa_family = dns_servers[dns_servers_num].source_addr.sa.sa_family = AF_INET;
	dns_servers[dns_servers_num].source_addr.in.sin_len = dns_servers[dns_servers_num].addr.in.sin_len = sizeof(struct sockaddr_in);
	dns_servers[dns_servers_num].source_addr.in.sin_addr.s_addr = INADDR_ANY;
	get_localdomain(local_domain);
	dns_servers[dns_servers_num].domain = local_domain;
        strcpy(ipbuf , inet_ntoa (dns_servers[dns_servers_num].addr.in.sin_addr));
        diag_printf("%s(): DNS Server List %d. %s:%d\n", __FUNCTION__, dns_servers_num, ipbuf, ntohs(dns_servers[dns_servers_num].addr.in.sin_port));
	dns_servers_num++;

	return 0;
}	

//
// Server
// serverCreatefd
//
static int serverCreatefd(DNSSERVER *server)
{
	char serverString[ADDRSTRLEN];
	int i, sfd, port = 0;

	// check if server ip address is same as our interface ip address.
	if (!(server->flags & (SERV_LITERAL_ADDRESS | SERV_NO_ADDR)))
	{
		strcpy(serverString, inet_ntoa(server->addr.in.sin_addr));
		port = ntohs(server->addr.in.sin_port); 

		for (i=0; i<MAXRECVINFACE; i++)
		{
			if ( recv_infaces[i].valid &&
			   sockaddrcmp(&server->addr, &recv_infaces[i].addr) )
				break;
		}

		if (i != MAXRECVINFACE)
		{
			diag_printf("%s(): Line%d ignoring nameserver %s - local interface\n", __FUNCTION__, __LINE__, serverString);
			return -1;
	    }

		// create fd
		if ((sfd = socket(server->source_addr.sa.sa_family, SOCK_DGRAM, 0)) == -1)
		{
			diag_printf("%s(): Line%d create socket error\n", __FUNCTION__, __LINE__);
			return -1;
		}
  
		if (bind(sfd, (struct sockaddr *)&server->source_addr, sa_len(&server->source_addr)) == -1)
		{
			close(sfd);
			diag_printf("%s(): Line%d bind socket error\n", __FUNCTION__, __LINE__);
			return -1;
		}

		server->fd = sfd;
		return 0;
	}
	return -1;
}

//
// Server
// dnsMasqservercreatefd
//
static DNSSERVER *serverFindnextforwardto(DNSSERVER *current)
{
	int i;
        if (current == NULL)
                return &dns_servers[0];
	
	// find current index first.
	for(i=0; i<dns_servers_num; i++)
	{
		if (&dns_servers[i] == current)
                        break;			
	}

	if (i == dns_servers_num || i == dns_servers_num-1)
		return &dns_servers[0];
        else
                return &dns_servers[i+1];
}

//
// Forward
// forwardListreset
//
static void forwardListdump(int all)
{
	FORWARDENTRY	*fentry;
	DNSSERVER		*server;
	char			serverstring[ADDRSTRLEN];
	char			queriststring[ADDRSTRLEN];
	char			timestring[32];
	int				count = 0;
	time_t			now;

	DNS_DBGPRINT(DNS_DEBUG_OFF, "\nQuerist          HostName                         DnsServer        When\n");		

	now = time(0);

	for(fentry = forward_list; fentry != NULL; fentry = fentry->next)
	{
		if (all || fentry->nid != 0)
		{
			server = fentry->forwardto;
			if (server->addr.sa.sa_family == AF_INET) 
				inet_ntop(AF_INET, &server->addr.in.sin_addr, serverstring, ADDRSTRLEN);

			if (fentry->queryaddr.sa.sa_family == AF_INET) 
				inet_ntop(AF_INET, &fentry->queryaddr.in.sin_addr, queriststring, ADDRSTRLEN);

				DNS_DBGPRINT(DNS_DEBUG_OFF, "%-16.16s %-32.32s %-16.16s %s\n",queriststring, fentry->dnsname, serverstring, ctime(&(fentry->time)));
		}
	}
	ctime_r(&now, timestring);
	DNS_DBGPRINT(DNS_DEBUG_OFF, "Total : %d, Now : %s\n", count, timestring);
}

static void forwardListreset(void)
{
	FORWARDENTRY *fentry;

	for (fentry = forward_list; fentry; fentry = fentry->next)
		fentry->nid = 0;
}

//
// Forward
// forwardListfree
//
static void forwardListfree(void)
{
	FORWARDENTRY *curr,*prev;

	curr = forward_list;

	while(curr)
	{
		prev = forward_list;
		curr = forward_list->next;
		forward_list = curr;
		free(prev);
	}
}

//
// Forward
// forwardEntryfind
//
static FORWARDENTRY *forwardEntryfind(unsigned short id, MYSOCKADDR *addr, char *dnsname)
{
	FORWARDENTRY *fentry;

	if (addr == NULL && dnsname == NULL)
	{
		for(fentry = forward_list; fentry != NULL; fentry = fentry->next)
		{
			if (fentry->nid == id)
				return fentry;
		}
	}
	else
	{
		for(fentry = forward_list; fentry != NULL; fentry = fentry->next)
		{
			if (fentry->nid &&
			  hostnamecmp(fentry->dnsname, dnsname) && 
			  sockaddrcmp(&fentry->queryaddr, addr))
			{
				fentry->oid = id;
				return fentry;
			}
		}
	}

	return NULL;
}

//
// Forward
// forwardEntrynew
//
static FORWARDENTRY *forwardEntrynew(time_t now)
{
  FORWARDENTRY *fentry = forward_list, *oldest = NULL;
  time_t oldtime = now;
  int count = 0;
  static time_t warntime = 0;

	while(fentry)
	{
		if (fentry->nid == 0) // not used
		{
			fentry->time = now;
			return fentry;
		}

		// find oldest one.
		if (fentry->time <= oldtime)
		{
			oldtime = fentry->time;
			oldest = fentry;
		}

		count++;
		fentry = fentry->next;
	}

	/* can't find empty one, use oldest if there is one
	   and it's older than timeout */
	if (oldest && now - oldtime  > TIMEOUT)
	{ 
		oldest->time = now;
		return oldest;
	}

	if (count > FTABSIZ)
	{
		/* limit logging rate so syslog isn't DOSed either */
		if (!warntime || now - warntime > LOGRATE)
		{
			warntime = now;
			//diag_printf("forwarding table overflow: check for server loops.");
		}
		return NULL;
	}

	if ((fentry = (FORWARDENTRY *)malloc(sizeof(FORWARDENTRY))))
	{
		fentry->next = forward_list;
		fentry->time = now;
		forward_list = fentry;
	}
	return fentry; /* OK if malloc fails and this is NULL */
}

//
// Export
// dnsMasqprocessquery
//
DNSSERVER *dnsMasqprocessreply(int fd, char *packet, int plen, time_t now)
{
	FORWARDENTRY	*fentry;
	DNSSERVER		*active_srv = NULL;
	DNSHEADER		*dnsheader;

        DNS_DBGPRINT(DNS_DEBUG_ERROR, "===> %s\n", __FUNCTION__);

	dnsheader = (DNSHEADER *)packet;
	if (dnsheader->qr && plen >= (int)sizeof(DNSHEADER))
	{
		if ((fentry = forwardEntryfind(ntohs(dnsheader->id), NULL, NULL)))
		{
			if (dnsheader->rcode == NOERROR || dnsheader->rcode == NXDOMAIN)
			{
				if (!fentry->forwardto->domain)
					active_srv = fentry->forwardto;
			}

			// replace to original id
			dnsheader->id = htons(fentry->oid);

                        DNS_DBGPRINT(DNS_DEBUG_ERROR, "%s: oid=%d, response to clinet(%s)\n", __FUNCTION__, fentry->oid, inet_ntoa(fentry->queryaddr.in.sin_addr));

			// send to the requester
			sendto(fentry->fd, packet, plen, 0, &fentry->queryaddr.sa, sa_len(&fentry->queryaddr));
			fentry->nid = 0; // mark this fentry as clear
		}
	}

        DNS_DBGPRINT(DNS_DEBUG_ERROR, "<=== %s\n", __FUNCTION__);
	return active_srv;
}

//
// Export
// dnsMasqprocessquery
//
DNSSERVER *dnsMasqprocessquery(int queryfd, MYSOCKADDR *queryaddr, DNSHEADER *dnsheader, 
			     int plen, DNSSERVER *active_srv, time_t now)
{
	FORWARDENTRY	*fentry;
	HOSTSENTRY      *hentry;
	char            *domain = NULL;
	MYADDR          *addrp = NULL;
	unsigned short  flags = FLAG_NOERR;
	unsigned short  goodreq;
	int i;

        DNS_DBGPRINT(DNS_DEBUG_ERROR, "===> %s\n", __FUNCTION__);

	goodreq = msgParserequest(dnsheader, (unsigned int)plen, dname);

	// check hosts first.
	if (goodreq && (hentry = hostsEntryfind(dname)))
	{
		// query found in hosts case
		flags = FLAG_IPV4;
		addrp = &hentry->addr;		
	}
	else if (dns_servers_num == 0) // no any available dns server
	{
		// no available dns server case
		flags = FLAG_NOERR;
		fentry = NULL;
	}
	else if ((fentry = forwardEntryfind(ntohs(dnsheader->id), queryaddr, dname)))
	{
		// query retry case
		domain = fentry->forwardto->domain;
		fentry->forwardto = serverFindnextforwardto(fentry->forwardto);
		dnsheader->id = htons(fentry->nid);
		fentry->queryaddr = *queryaddr;
	}
	else 
	{
		// first query case, make new forward entry
		if (goodreq)
		{
			/* If the query ends in the domain in one of our servers, set
			   domain to point to that name. We find the largest match to allow both
			   domain.org and sub.domain.org to exist. */
			unsigned int namelen = strlen(dname);
			unsigned int matchlen = 0;

			for(i=0; i<dns_servers_num; i++)
			{
				if (dns_servers[i].domain)
				{
					unsigned int domainlen;

					domainlen = strlen(dns_servers[i].domain);
					if (namelen >= domainlen &&
						hostnamecmp(dname + namelen - domainlen, dns_servers[i].domain) &&
						domainlen > matchlen)
					{
						if (dns_servers[i].flags & SERV_LITERAL_ADDRESS)
						{
							/* flags gets set if server is in fact an answer */
							unsigned short sflag;

							sflag = FLAG_IPV4;

							if (sflag & goodreq) /* only OK if addrfamily == query */
							{
								flags = sflag;
								domain = dns_servers[i].domain;
								matchlen = domainlen; 
								if (dns_servers[i].addr.sa.sa_family == AF_INET) 
									addrp = (MYADDR *)&dns_servers[i].addr.in.sin_addr;
							}
						}
						else
						{
							flags = FLAG_NOERR; /* may be better match from previous literal */
							domain = dns_servers[i].domain;
							matchlen = domainlen;
						}
					} 
				}
			}
		}

		if (!flags) /* flags set here means a literal found */
		{
			fentry = forwardEntrynew(now);
			if (fentry == NULL)
				flags = FLAG_NEG; // forward_list is full
		}

		// setup fentry data
		if (fentry)
		{
			fentry->forwardto = active_srv;
			fentry->queryaddr = *queryaddr;
			fentry->fd = queryfd;
			fentry->nid = getnid();
			fentry->oid = ntohs(dnsheader->id);
			fentry->dnsname[0]=0;
			strcpy(fentry->dnsname, dname);
			dnsheader->id = htons(fentry->nid);
		}
	}

	/* check for send errors here (no route to host) 
	   if we fail to send to all nameservers, send back an error
	   packet straight away (helps modem users when offline)  */

	if (!flags && fentry)
	{
		DNSSERVER *first = fentry->forwardto;
		      
		while (1)
		{		
		        addrp = NULL;
		        if (fentry->forwardto->fd > 0)
                        {
                                if (fentry->forwardto->addr.sa.sa_family == AF_INET)
                                        addrp = (MYADDR *)&fentry->forwardto->addr.in.sin_addr;

        			/* only send to servers dealing with our domain.
        			   domain may be NULL, in which case server->domain 
        			   must be NULL also. */
        			if ((!domain && !fentry->forwardto->domain) ||
        				(domain && fentry->forwardto->domain && hostnamecmp(domain, fentry->forwardto->domain)))
        			{
        				if (fentry->forwardto->flags & SERV_NO_ADDR)
        					flags = FLAG_NOERR; /* NULL servers are OK. */
        				else if (!(fentry->forwardto->flags & SERV_LITERAL_ADDRESS))
        				{	
       						if(sendto(fentry->forwardto->fd, (char *)dnsheader, plen, 0, &fentry->forwardto->addr.sa,
       								 sa_len(&fentry->forwardto->addr)) != -1)
       						{
                                                        /* for no-domain, don't update last_server */
                                                        DNS_DBGPRINT(DNS_DEBUG_ERROR, "%s: oid=%d, send to %s query: %s, success\n", __FUNCTION__, fentry->oid, inet_ntoa(fentry->forwardto->addr.in.sin_addr), dname);
                                                        return domain ? active_srv : serverFindnextforwardto(fentry->forwardto);
                                                }
        				}
        			}
                        }

			fentry->forwardto = serverFindnextforwardto(fentry->forwardto);

			/* check if we tried all without success */
			if (fentry->forwardto == first)
				break;
		}

		/* could not send on, prepare to return */ 
		dnsheader->id = htons(fentry->oid);
		fentry->nid = 0;
	}	  

	// send back querist in case of could not send on, return empty answer or address if known for whole domain
	if (addrp == NULL)
		{
#ifndef CONFIG_SNIFFER_DAEMON		
		//diag_printf("%s %d:addrp = NULL,return NULL here\n",__FUNCTION__,__LINE__);
#endif
		return NULL;
		}
	plen = msgBuildreply(dnsheader, (unsigned int)plen, addrp, flags);
	sendto(queryfd, (char *)dnsheader, plen, 0, &queryaddr->sa, sa_len(queryaddr));

        if (DNSDebugLevel > DNS_DEBUG_OFF)
        {
        	if (flags == FLAG_NEG) {
                        DNS_DBGPRINT(DNS_DEBUG_ERROR, "%s: quesy %s, couldn't get memory\n", __FUNCTION__, dname);
        	} else if (flags == FLAG_NOERR) {
                        DNS_DBGPRINT(DNS_DEBUG_ERROR, "%s: quesy %s, no domain\n", __FUNCTION__, dname);
        	} else if (flags == FLAG_IPV4) {
                        DNS_DBGPRINT(DNS_DEBUG_ERROR, "%s: quesy %s, we know the address\n", __FUNCTION__, dname);
                } else
                        DNS_DBGPRINT(DNS_DEBUG_ERROR, "%s: quesy %s, other\n", __FUNCTION__, dname);
        }
	return active_srv;
}

//
// Export
// dnsMasqinit
//
int dnsMasqstart(int port)
{
	int i, ret;

        DNS_DBGPRINT(DNS_DEBUG_OFF, "%s\n", __FUNCTION__);

	ret = interfaceGet(port);
	if (ret < 0)
	{
		DNS_DBGPRINT(DNS_DEBUG_OFF, "%s(): Line%d interfaceGet ERROR!\n", __FUNCTION__, __LINE__);
		if (recv_infaces_num == 0)
			goto exit_out;
	}

	forwardListreset();

	hostsInit();

	ret = serverInit(port);
	if (ret < 0)
	{
		DNS_DBGPRINT(DNS_DEBUG_OFF, "%s(): Line%d serverInit ERROR!\n", __FUNCTION__, __LINE__);
		if (dns_servers_num == 0)
			goto exit_out1;
	}

	return 0;

exit_out:
	return ret;

exit_out1:
	// clear recv_infaces data
	for (i=0; i<MAXRECVINFACE; i++)
	{
		recv_infaces[i].valid = 0;
		if (recv_infaces[i].fd > 0)
		{
			close(recv_infaces[i].fd);
			recv_infaces[i].fd = -1;
		}
	}
	recv_infaces_num = 0;
	return ret;
}

//
// Export
// dnsMasqstop
//
int dnsMasqstop(void)
{
	int i;

        DNS_DBGPRINT(DNS_DEBUG_OFF, "%s\n", __FUNCTION__);

	// clear recv_infaces data
	for (i=0; i<MAXRECVINFACE; i++)
	{
		recv_infaces[i].valid = 0;
		if (recv_infaces[i].fd > 0)
		{
			close(recv_infaces[i].fd);
			recv_infaces[i].fd = -1;
		}
	}
	recv_infaces_num = 0;

	// free forward list
	forwardListfree();

    return 0;
}

void dnsMasqforwarddump(void)
{
	forwardListdump(1);	
}


