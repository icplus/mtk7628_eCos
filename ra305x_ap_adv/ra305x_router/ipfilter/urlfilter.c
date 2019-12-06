/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <network.h>
#include <stdio.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/if_ether.h>

#include <sys/socket.h>
#include <sys/mbuf.h>

#include "ip_compat.h"
#include "ip_fil.h"
#include <urlf.h>
#include <cfg_net.h>
#include <sys_status.h>
#include <lib_packet.h>
#include <eventlog.h>
#include <cfg_def.h>

//==============================================================================
//                                    MACROS
//==============================================================================
#define SUPPORT_REFERER
#define URL_BLOCK_RESPONSE_HEADER 		"HTTP/1.1 200 OK\r\nCache-Control: no-cache\r\nConnection: close\r\n\r\n"
#define URL_BLOCK_MSG					"<html><head></head><body><H1>Page Blocked!!</body></html>"
#define MAX_URL_BLOCKMSG_SZ				1200

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================


//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
static int urlfil_list = NULL;
static int urlfil_dafault_action = URLF_PASS;
static int urlfil_default_mode = 0;
static char daymap[7]={4,5,6,0,1,2,3};
static char *urlfil_block_response=NULL;
static int urlfil_block_res_len = 0;

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
inline char* get_line(char *data, int *len)
{
    register int i;
    register char c;

    if ((data == NULL) && (*len > 0))
    {
	*len = 0;
	goto unNormal;
    }

    for ( i = 0; i < *len; ++i )
    {
		c = data[i];
		if ( c == '\n' || c == '\r' )
		{
	    	++i;
	    	if ( (c == '\r') && (i < *len) && (data[i] == '\n') )
	    	{
				++i;
			}
			*len -= i ;
	   		return &(data[i]);
	    }
	}

unNormal:
    return (char*) 0;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void sendblock(struct ifnet *ifp, char *head, struct mbuf *m)
{
	ip_t *ip;
	struct tcphdr *tcp;
	struct mbuf *nm;
	struct sockaddr nsa;
	struct ether_header *neh, *oeh;
	struct ip *niph;
	struct tcphdr *ntcph;
	int pk_len;
	int ndata_len=0;
	struct ippseudo *ipseudo;
	char *pndata;

	ip = mtod(m, struct ip *);
	tcp =  (struct tcphdr *) ((char *)ip + (ip->ip_hl << 2));
	pk_len = m->m_pkthdr.len - (tcp->th_off << 2) - (ip->ip_hl << 2);

	MGETHDR(nm, M_DONTWAIT, MT_DATA);
	if(nm==NULL)
		goto close_tcp;

	MCLGET(nm, M_DONTWAIT);
	if(!(nm->m_flags & M_EXT))
	{
		m_freem(nm);
		goto close_tcp;
	}

	/**************************
	Date:20080423
	Perpare new package for sending BLOCK message on webpage.
	***************************/
	nm->m_data += 32;
	nsa.sa_family = AF_UNSPEC;
	nsa.sa_len = sizeof(nsa);

	niph = mtod(nm, struct ip *);
	ntcph = (struct tcphdr *)((char *)niph + sizeof(struct ip));
	pndata = (char *)ntcph + sizeof(struct tcphdr);

	if (urlfil_block_response != NULL) {
		memcpy(pndata, urlfil_block_response, urlfil_block_res_len);
		ndata_len = urlfil_block_res_len;
	} else 
		ndata_len = 0 ;

	nm->m_pkthdr.len = nm->m_len = (sizeof(struct ip)+ sizeof(struct tcphdr) + ndata_len);
		

	/* partial IP */
	memset(niph, 0, sizeof(struct ip));
	niph->ip_src.s_addr = ip->ip_dst.s_addr;
	niph->ip_dst.s_addr = ip->ip_src.s_addr;
	niph->ip_p = ip->ip_p;
	niph->ip_len = htons(sizeof(struct tcphdr) + ndata_len);

	/* General TCP */
	ntcph->th_off = sizeof(struct tcphdr) >> 2;
	ntcph->th_sport = tcp->th_dport;
	ntcph->th_dport = tcp->th_sport;
	ntcph->th_flags = TH_ACK | TH_FIN | TH_PUSH ;
	ntcph->th_seq = tcp->th_ack;
	ntcph->th_ack = htonl(ntohl(tcp->th_seq) + pk_len);
	ntcph->th_win = htons(65535);
	ntcph->th_sum = 0;
	ntcph->th_sum = in_cksum(nm, nm->m_pkthdr.len);

	/* General Internet Protocol */
	niph->ip_hl = sizeof(struct ip) >> 2;
	niph->ip_off = IP_DF;
	niph->ip_v = IPVERSION;
	niph->ip_tos = ip->ip_tos;
	niph->ip_ttl = 7;
	niph->ip_len = htons(nm->m_pkthdr.len);
	niph->ip_sum = 0;
	niph->ip_sum = in_cksum(nm, sizeof(struct ip));		

	/* General Ether Header */
	neh = (struct ether_header *)nsa.sa_data;
	oeh = (struct ether_header *)head;
	bcopy(oeh->ether_shost, neh->ether_dhost, sizeof(neh->ether_dhost));
	neh->ether_type = htons(ETHERTYPE_IP);

	(* ifp->if_output)(ifp, nm, &nsa, (struct rtentry *)NULL);
				

close_tcp:
	/**************************
	Date:20080423
	Modify original m and send it to extranet for Closing connection.
	***************************/
	tcp->th_flags = TH_ACK | TH_RST;
	tcp->th_off = sizeof(struct tcphdr) >> 2;

	ipseudo = (struct ippseudo*)((char *)tcp + sizeof(struct tcphdr));
	ipseudo->ippseudo_src.s_addr = ip->ip_dst.s_addr;
	ipseudo->ippseudo_dst.s_addr = ip->ip_src.s_addr;
	ipseudo->ippseudo_p = ip->ip_p;
	ipseudo->ippseudo_len = htons(sizeof(struct tcphdr));
	ipseudo->ippseudo_pad = 0;

	m->m_data += (ip->ip_hl << 2);
	tcp->th_sum = 0;
	tcp->th_sum = in_cksum(m, sizeof(struct tcphdr)+sizeof(struct ippseudo));
	m->m_data -= (ip->ip_hl << 2);

	m->m_pkthdr.len = m->m_len = (ip->ip_hl << 2) + sizeof(struct tcphdr);
	ip->ip_len = htons(m->m_pkthdr.len);
	ip->ip_sum = 0;
	ip->ip_sum = in_cksum(m, (ip->ip_hl << 2));		
}


static char cmpstr[2000];
int urlfilter_match(struct ifnet *ifp, char *head, struct mbuf *m)
{
	ip_t *ip;
	struct urlfilter *url_en;
	struct tcphdr *tcp;
	char *data, *url=0, *path;
	int pk_len, url_len=0, uri_len=0, i, sip;
	char *pcmphead, *pcmptail;
#ifdef SUPPORT_REFERER
	char *prefhead, *preftail;
	int ref_len=0;
#endif
	
	if(!urlfil_list) //no entry
		return URLF_PASS;
	if(m->m_pkthdr.len < 40)
		return URLF_PASS;
	
	ip = mtod(m, struct ip *);

	/* we do not check packet that touch my managed web */
	if (ip->ip_dst.s_addr == SYS_lan_ip)
		return URLF_PASS;
    
	if(ip->ip_p != IPPROTO_TCP) // Not TCP
		return URLF_PASS;
	
	/*  Skip TCP SYN */
	tcp =  (struct tcphdr *) ((char *)ip + (ip->ip_hl << 2));
	if(tcp->th_flags & TH_SYN)
		return URLF_PASS;
	
	/*  Skip frames too short*/
	data = (char *)tcp + (tcp->th_off << 2);
	pk_len = m->m_pkthdr.len - (tcp->th_off << 2) - (ip->ip_hl << 2);
	if(pk_len < 10)
		return URLF_PASS;

	/*  Looking for HTTP request */
    path = NULL;
	switch(data[0])
	{
		case 'G':
			if(!memcmp(data, "GET ", 4))
			{
				path = data+4;
			}
			break;
		case 'P':
			if(!memcmp(data, "POST ", 5))
			{
				path = data+5;
			}
			break;
		case 'H':
			if(!memcmp(data, "HEAD ", 5))
			{
				path = data+5;
			}
			break;
		default:
			/*  Allow non HTTP to pass  */
			return URLF_PASS;
	}

    if (path == NULL)
        return URLF_PASS;
	
	// set pointers to the buf head
	pcmphead = pcmptail = cmpstr;
#ifdef SUPPORT_REFERER
	prefhead = preftail = cmpstr;
#endif
	
	/*  Looking for HOST line in HTTP header and composing the URL  */
	while(pk_len > 0) 
	{
		url = get_line(data, &pk_len);
		if(url == 0)
		{
			break;
		}
		/* Http header end */
		if((*url == '\r') || (*url == '\n'))
		{
			url = 0;
			break;
		}
        if (uri_len == 0)
            uri_len = url - path;
			
		if(url[0] == 'H' && !memcmp(url, "Host:", 5)) 
		{
			url += 6;
#ifdef SUPPORT_REFERER
			// check if referer comes first
			if (prefhead != preftail)
			{
				pcmphead = pcmptail = preftail + 1;
			}
#endif
			for (i=0; i<pk_len; i++)
			{
				if (url[i] == '\r' || url[i] == '\n')
				{
					url_len = i;
					*pcmptail ='\0';
					for (i=0; i< uri_len; i++)
					{
						if (path[i]== ' ')
						{
							url_len += i;
							*(pcmphead+url_len) = '\0';
							break;
						}

						*(pcmphead+url_len+i) = path[i];
					}
					break;
				}
				*pcmptail = url[i];
				pcmptail++;
			}
			break;
		}
#ifdef SUPPORT_REFERER
		if(url[0] == 'R' && !memcmp(url, "Referer:", 8))
		{
			url += 16;
			// check if host comes first
			if (pcmphead != pcmptail)
			{
				prefhead = preftail = pcmptail + 1;
			}
			for(i=0;i<pk_len;i++)
			{
				if(url[i] == '\r' || url[i] == '\n')
				{
					ref_len = i;
					*preftail = '\0';
					break;
				}
				else
				{
					*preftail = url[i];
					preftail++;
				}
			}
		}
#endif
		data = url;
		url =0;
	}
		
	if( url == 0 || pk_len == 0 )
		return URLF_PASS;
		
	sip = ntohl(ip->ip_src.s_addr);
	for (url_en = urlfil_list; url_en ; url_en = url_en->next) 
	{
		int len, j;

		/*  Check the next rule if the current rule is not active  */
		if (!(url_en->flags & URLF_FG_ACTIVE))
			continue;

		if(url_en->sip > sip || url_en->eip < sip )
			continue;
        
		len = strlen(url_en->url);
		switch(urlfil_default_mode)
		{
		case URLF_KEYWORD_ONLY_DENY: 
		case URLF_KEYWORD_ONLY_ALLOW:
			for(j=0; j<url_len; j++) 
			{
				if((*(pcmphead+j) == *(url_en->url)) && !memcmp(url_en->url, pcmphead+j, len))
				{
					goto match;
				}
			}
#ifdef SUPPORT_REFERER
			for(j=0; j<ref_len; j++)
			{
				if((*(prefhead+j) == *(url_en->url)) && !memcmp(url_en->url, prefhead+j, len))
				{
					goto match;
				}
			}
#endif
			break;
		case URLF_EXACTLY_ONLY_DENY:
		case URLF_EXACTLY_ONLY_ALLOW:
			if(!memcmp(url_en->url, pcmphead, len))
			{
				//diag_printf("GO A\n");
				goto match;
			}
#ifdef SUPPORT_REFERER
			if(!memcmp(url_en->url, prefhead, len))
			{
				//diag_printf("GO B\n");
				goto match;
			}
#endif
			break;
		default:
			break;
		}
	}
  	
	if(urlfil_dafault_action == URLF_DENY)
	{
		sendblock(ifp, head, m);
	}
	return URLF_PASS;
	
match:
	//diag_printf("URL filter match!! str:[%s]\n", url_en->url);
#ifdef CONFIG_URLFLT_LOG
	{
		char srcip[15];
		sip = ntohl(sip);
		strcpy(srcip, inet_ntoa(*(struct in_addr *)&sip));
		if((urlfil_dafault_action^1) == 1)
			SysLog(LOG_DROPPK|SYS_LOG_NOTICE|LOGM_FIREWALL, "**%s** [%s] match, %s->*", STR_FILTER, url_en->url, srcip);
	}
#endif

	if((urlfil_dafault_action^1) == URLF_DENY){
		sendblock(ifp, head, m);		
	}

	return URLF_PASS;
}


//------------------------------------------------------------------------------
// FUNCTION
//		urlfilter_check_acttime
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void urlfilter_check_acttime(void)
{
	struct urlfilter *pfilter;
	unsigned int curtime, secs, weekday;
		
	curtime = GMTtime(0);
	weekday = curtime % 604800;
	secs = weekday % 86400;
	weekday /= 86400;

	for (pfilter=urlfil_list; pfilter; pfilter=pfilter->next) {
		if (pfilter->flags & URLF_FG_VALID) {
			switch(pfilter->urlf_mode) {
			case URLF_MODE_PERIOD:
				if (!(pfilter->urlf_tday & (1<<daymap[weekday]))) {
					if (pfilter->flags & URLF_FG_ACTIVE)
						pfilter->flags &= ~URLF_FG_ACTIVE;
					continue;
				}
				/*  Fall to URLF_MODE_ONE  */
			case URLF_MODE_ONE:
				if ((secs >= pfilter->urlf_tstart) && 
					(secs <= (pfilter->urlf_tend ? pfilter->urlf_tend : 86400))) {
					if (!(pfilter->flags & URLF_FG_ACTIVE))
						pfilter->flags |= URLF_FG_ACTIVE;
				} else {
					if (pfilter->flags & URLF_FG_ACTIVE)
						pfilter->flags &= ~URLF_FG_ACTIVE;
				}
				break;
			case URLF_MODE_ALWAYS:
			default:
				/*  Make the rule active if it is not.  */
				if (!(pfilter->flags & URLF_FG_ACTIVE))
					pfilter->flags |= URLF_FG_ACTIVE;
				break;
			}
		}
	}
}

extern int get_file_handle(char *filename);
extern int query_webfile_len(int hdl);
extern int copy_webfile(int hdl, char *dst, int len);
extern int close_file(void *file);
extern char *_url_blockpage_name;
//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void urlfilter_act(void)
{
	struct ifnet *ifp;
	int hdr_sz, filehdl, file_len;
	
	if (urlfil_block_response == NULL) {
		hdr_sz = strlen(URL_BLOCK_RESPONSE_HEADER);
#ifndef USE_DEFAULT_BLOCK_PAGE_ONLY
		if ((_url_blockpage_name != NULL) && 
				(filehdl = get_file_handle(_url_blockpage_name)) != 0) {
			file_len = query_webfile_len(filehdl);
			if (file_len <=0 || file_len > MAX_URL_BLOCKMSG_SZ) {
				close_file(filehdl);
				diag_printf("url: block page size too large or too small!\n");
				goto url_def_blockpage;
			}
			if ((urlfil_block_response = malloc(hdr_sz+file_len))  != NULL) {
				strcpy(urlfil_block_response, URL_BLOCK_RESPONSE_HEADER);
				copy_webfile(filehdl, urlfil_block_response+hdr_sz, file_len);
			}
			close_file(filehdl);
		} else 
#endif
			{
url_def_blockpage:
			file_len = strlen(URL_BLOCK_MSG);
			if ((urlfil_block_response = malloc(hdr_sz+file_len+16)) != NULL) {
				strcpy(urlfil_block_response, URL_BLOCK_RESPONSE_HEADER);
				strcat(urlfil_block_response, URL_BLOCK_MSG);
			}
		}
		urlfil_block_res_len = hdr_sz + file_len;
	}
	ifp = ifunit(LAN_NAME);
	LIB_ether_input_register(ifp, urlfilter_match, FORWARDQ);
}


//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void urlfilter_inact(void)
{
	struct ifnet *ifp;
	ifp = ifunit(LAN_NAME);
	LIB_ether_input_unregister(ifp, urlfilter_match, FORWARDQ);
	if (urlfil_block_response != NULL) {
		free(urlfil_block_response);
		urlfil_block_response = NULL;
		urlfil_block_res_len = 0;
	}
}


//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int urlfilter_del(struct urlfilter *entry)
{
	return 0;
	
}


//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int urlfilter_add(struct urlfilter *entry)
{
	struct urlfilter *new_en;
	int urllen;

	urllen = strlen(entry->url) + 1;

	if(!(new_en = malloc(sizeof(struct urlfilter)+urllen)))
		return -1;
		
	memcpy(new_en, entry, sizeof(struct urlfilter));
	new_en->url = (char *) (new_en + 1);
	
	/*  Trim leading http://  */
	if (strncasecmp(entry->url, "http://", 7) == 0)
		strcpy(new_en->url, entry->url+7);
	else
		strcpy(new_en->url, entry->url);

	new_en->flags = URLF_FG_VALID;
	if (entry->urlf_mode == URLF_MODE_ALWAYS)
		new_en->flags |= URLF_FG_ACTIVE;

	/*  Add new entry to list  */
	new_en->next = urlfil_list;
	urlfil_list = new_en;
		
	return 0;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void urlfilter_flush(void)
{
	struct urlfilter *entry, *next_en;
	
	diag_printf("urlfilter_flush!\n");
	entry = urlfil_list;
	urlfil_list = NULL;
	while(entry) {
		next_en = entry->next;
		free(entry);
		entry = next_en;
	}
}


//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void urlfilter_setmode(int mode)
{
	urlfil_default_mode  = mode;
	urlfil_dafault_action = (mode%2==0) ? URLF_ALLOW_MODE : URLF_DENY_MODE;
	
	diag_printf("url: act = %d\n", urlfil_dafault_action);
}


