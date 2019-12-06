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

//==============================================================================
//                                    MACROS
//==============================================================================
#define	IPF_MMS_PROXY

#define FILE_GUID               "{5D3E02AB-6190-11d3-BBBB-00C04F795683}"

#define IS_DIGIT(c)    (c >= '0' && c <= '9')
#define IS_DIGITDOT(c) ((c >= '0' && c <= '9') || c == '.')
#define IS_ALPHA(c)    ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
#define IS_WSPACE(c)   (c == ' ' || c == '\t' || c == '\r' || c == '\n')
#define SKIP_WSPACE(p,n) while( n > 0 && IS_WSPACE(*p) ) { n--; p++; }

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
static	frentry_t	ipmmsfr;

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================
int ippr_mms_init __P((void));
int ippr_mms_new __P((fr_info_t *, ap_session_t *, nat_t *));
void ippr_mms_del __P((ap_session_t *));
int ippr_mms_out __P((fr_info_t *, ip_t *, ap_session_t *, nat_t *));

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
static int next_line( char** ppbuf, int* pbuflen, char** ppline, int* plinelen )
{
    char* pbuf = *ppbuf;
    int buflen = *pbuflen;
    int linelen = 0;

    do
    {
        while( *pbuf != '\r' && *pbuf != '\n' )
        {
            if( buflen <= 1 )
            {
                return 0;
            }

            pbuf++;
            linelen++;
            buflen--;
        }

        if( buflen > 1 && *pbuf == '\r' && *(pbuf+1) == '\n' )
        {
            pbuf++;
            buflen--;
        }

        pbuf++;
        buflen--;
    }
    while( buflen > 0 && (*pbuf == ' ' || *pbuf == '\t') );

    *ppline = *ppbuf;
    *plinelen = linelen;
    *ppbuf = pbuf;
    *pbuflen = buflen;

    return linelen;
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
int ippr_mms_init(void)
{
	bzero((char *)&ipmmsfr, sizeof(ipmmsfr));
	ipmmsfr.fr_ref = 1;
	ipmmsfr.fr_flags = FR_OUTQUE|FR_PASS|FR_QUICK|FR_KEEPSTATE;
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
int replace_mbufdata(fr_info_t *fin, int off, int orglen, char* newdata, int newlen)
{
	int diff, remain_len;
	mb_t **mp = fin->fin_mp;
	mb_t *m = *mp;
	struct ip *ip = mtod(m, struct ip *);
	char *cp;
	
	diff = newlen-orglen;
	
	if ((diff + ip->ip_len) > 65535) {
			diag_printf("replace_mbufdata:diff(%d) + ip->ip_len > 65535\n", diff);
			return 0;
	}

	/* if addition length excess one mbuf, we have to copy all to anther big mbuf(cluster) */
	if(!(m->m_flags & M_EXT) && (m->m_data + m->m_len + diff >&m->m_dat[MLEN]))
	{
		struct mbuf *n;
		diag_printf("replace_mbufdata: change to big mbuf\n");
		if ((n = m_get(M_DONTWAIT, MT_DATA)) == 0)
            return 0;
        n->m_len = diff;
        m_cat(m, n);
        m->m_pkthdr.len += diff;
        if ((m = m_pullup2(m, m->m_pkthdr.len)) == 0)
            return 0;
		ip = mtod(m, struct ip *);
	}
	else
	{
		m->m_len += diff;
		m->m_pkthdr.len += diff;
	}

    cp = (char *)((char *)ip + (ip->ip_hl << 2));
    fin->fin_dp = cp;
    cp = (char *)cp + off;
    remain_len = ip->ip_len - ((ip->ip_hl << 2)+ off + orglen);
    // Move Rest data
    if (diff)
    {
        memmove(cp+newlen, cp+orglen, remain_len);
        ip->ip_len += diff;
        fin->fin_dlen += diff;
		fin->fin_plen += diff;
    }
    // Copy new content
    bcopy(newdata, cp, newlen);
    *mp = m;
   
    return diff;
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
int ippr_mms_out(fin, ip, aps, nat)
fr_info_t *fin;
ip_t *ip;
ap_session_t *aps;
nat_t *nat;
{
	int ipaddr, datlen, linelen;
	unsigned short port;
	char *data ,*line;
	tcphdr_t *tcp;
	int isinvite, inc;
	int accepted=0, addroff=0, portoff=0;
	int addrlen, portlen;
	fr_info_t fi;
	nat_t *ipn=NULL;
	
	inc = 0;
	isinvite = 0;
	
	tcp = (tcphdr_t *)fin->fin_dp;
	
	data = (unsigned char *)tcp + (tcp->th_off << 2);
	datlen = fin->fin_dlen - (tcp->th_off << 2);
	
	if( !next_line( &data, &datlen, &line, &linelen ) )
    {
        return 0;
    }
    //for(i=0;i<linelen;i++)
    //	diag_printf("%c", line[i]);
    
	if( strncmp( line, "MSG ", 4 ) != 0 ) 
		return 0;
	line += 4;
    if( ! IS_DIGIT(*line) )
    	return 0;
    while( IS_DIGIT(*line) ) 
    	line++;
    if( *line != ' ' ) 
    	return 0;
    line++;
    if( ! IS_ALPHA(*line) ) 
    	return 0;
    line++;
    if( *line != ' ' ) 
    	return 0;
    line++;
    if( ! IS_DIGIT(*line) ) 
    	return 0;
    /* Parse mime headers */
    while( next_line( &data, &datlen, &line, &linelen ) )
    {
        if( linelen == 0 )
        {
            break;
        }
        if( memcmp( line, "Content-Type:", 13 ) == 0 )
        {
            line += 13;
            while( *line == ' ' ) line++;
            if( memcmp( line, "text/x-msmsgsinvite;", 20 ) == 0 )
                isinvite = 1;
        }
    }
    if( !isinvite ) 
    	return 0;
   
    /* Parse entity for accept, ipaddr, port */
    while( next_line( &data, &datlen, &line, &linelen ) )
    {
        if( memcmp( line, "Invitation-Command:", 19 ) == 0 )
        {
            line += 19;
            while( *line == ' ' ) line++;
            if( memcmp( line, "ACCEPT", 6 ) == 0 )
                accepted = 1;
        }
        if( memcmp( line, "IP-Address:", 11 ) == 0 )
        {
        	char addrstr[]="255.255.255.255";
            line += 11;
            while( *line == ' ' ) line++;
            addroff = line - (char *)tcp;
            addrlen = 0;
            while( IS_DIGITDOT(line[addrlen]) ) addrlen++;
            memcpy(addrstr, line, addrlen);
            addrstr[addrlen]=0;
            ipaddr = inet_addr(addrstr);
        }
        if( memcmp( line, "Port:", 5 ) == 0 )
        {
        	char portstr[]="65535";
            line += 5;
            while( *line == ' ' ) line++;
            portoff = line - (char *)tcp;
            portlen = 0;
            while( IS_DIGIT(line[portlen]) ) portlen++;
            memcpy(portstr, line, portlen);
            portstr[portlen]=0;
            //diag_printf("++++++++++portstr:%s, atoi(portstr):%d, atoi(portstr):%x\n", portstr, atoi(portstr), atoi(portstr));
            port = atoi(portstr);
        }
    }
    
    if( !accepted || !addroff || !portoff ) return 0;
    /* maybe messenger has use correct IP by UPnP*/
    //diag_printf("ipaddr:%x,ip->ip_src.s_addr:%x, port:%x\n", ipaddr, ip->ip_src.s_addr, port);
    if(ipaddr == ip->ip_src.s_addr)
    	return 0;
    
    memcpy(&fi, fin, sizeof(fr_info_t));
    
    if (ipn == NULL) {
		struct ip newip;
		tcphdr_t newtcp;
		bcopy(ip, &newip, sizeof(newip));
		newip.ip_len = fin->fin_hlen + sizeof(newtcp);
		newip.ip_p = IPPROTO_TCP;
		newip.ip_src = nat->nat_inip;
		newip.ip_dst.s_addr = 0;
			
		bzero(&newtcp, sizeof(newtcp));
		newtcp.th_sport = port;
			
		fi.fin_fi.fi_p = IPPROTO_TCP;
		fi.fin_data[0] = port;
		fi.fin_data[1] = 0;
		fi.fin_dp = (char *)&newtcp;
		
		ipn = nat_new(&fi, &newip, nat->nat_ptr, NULL, IPN_TCP|FI_W_DADDR|FI_W_DPORT, NAT_OUTBOUND);
			
	}
	
	if (ipn != NULL) {	
		int nat_ip ;
		int nlen;
		char newbuf[]="255.255.255.255";
		
    	nat_ip = ntohl(ipn->nat_outip.s_addr);
		nlen = sprintf(newbuf, "%d.%d.%d.%d", 	(nat_ip >> 24) & 0xff,
												(nat_ip >> 16) & 0xff, 
												(nat_ip >> 8) & 0xff,
												nat_ip & 0xff);
		
		//diag_printf("addroff:%d, addrlen:%d, newbuf:%s, nlen:%d\n", addroff, addrlen, newbuf, nlen);
		inc = replace_mbufdata(fin, addroff, addrlen, newbuf, nlen);
		nlen = sprintf(newbuf, "%d", ntohs(ipn->nat_outport));
		//diag_printf("new port:%s\n", newbuf);
		inc += replace_mbufdata(fin, portoff+inc, portlen, newbuf, nlen);
		ipn->nat_fstate.rule = nat->nat_fstate.rule;
    }
    
	return inc;
}

