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
#define IPF_SIP_PROXY

//#define DEBUG_SIP_RPOXY
#ifdef DEBUG_SIP_RPOXY
#define	SIP_DBG(fmt, ...)	diag_printf(fmt, ## __VA_ARGS__)
#else
#define SIP_DBG(fmt, ...)
#endif

static int ippr_sip_in __P((fr_info_t *, ip_t *, ap_session_t *, nat_t *));
static int ippr_sip_init __P((void));
static int ippr_sip_new __P((fr_info_t *, ap_session_t *, nat_t *));
static int ippr_sip_out __P((fr_info_t *, ip_t *, ap_session_t *, nat_t *));

#define SIP_STATE_INIT			0
#define SIP_STATE_REGISTER	1
#define SIP_STATE_INVITE		2

typedef	struct sipentry {
	uint8_t sip_state;
	struct in_addr out_addr;
	struct in_addr in_addr;
	struct in_addr o_addr;
	int in_audio_port; 
	int in_video_port;
} sipentry_t;

static inline unsigned char tolower(unsigned char c)
{
	if (isupper(c))
		c -= 'A'-'a';
	return c;
}

int strnicmp(const char *s1, const char *s2, int len)
{
	unsigned char c1, c2;

	c1 = c2 = 0;
	if (len) {
		do {
			c1 = *s1;
			c2 = *s2;
			s1++;
			s2++;
			if (!c1)
				break;
			if (!c2)
				break;
			if (c1 == c2)
				continue;
			c1 = tolower(c1);
			c2 = tolower(c2);
			if (c1 != c2)
				break;
		} while (--len);
	}
	return (int)c1 - (int)c2;
}

static int string_len(
	const char *dptr,
	const char *limit, 
	int *shift)
{
	int len = 0;

	while (dptr < limit && isalpha(*dptr)) {
		dptr++;
		len++;
	}
	return len;
}

static int digits_len(
	const char *dptr,
	const char *limit, 
	int *shift)
{
	int len = 0;
	while (dptr < limit && isdigit(*dptr)) {
		dptr++;
		len++;
	}
	return len;
}

/* get media type + port length */
static int media_len(
	const char *dptr,
	const char *limit, 
	int *shift)
{
	int len = string_len(dptr, limit, shift);

	dptr += len;
	if (dptr >= limit || *dptr != ' ')
		return 0;
	len++;
	dptr++;

	return len + digits_len(dptr, limit, shift);
}

int sip_get_header (
	const char *dptr, 
	unsigned int dataoff, 
	unsigned int datalen,
	unsigned int *comp_name,
	unsigned int *matchoff)
{
	int comp_len = strlen(comp_name);
	const char *start = dptr, *limit = dptr + datalen;

	for (dptr += dataoff; dptr < limit; dptr++) {
		/* Find beginning of line */
		if (*dptr != '\r' && *dptr != '\n')
			continue;
		if (++dptr >= limit)
			break;
		if (*(dptr - 1) == '\r' && *dptr == '\n') {
			if (++dptr >= limit)
				break;
		}

		if (limit - dptr >= comp_len &&
		    strnicmp(dptr, comp_name, comp_len) == 0) 
		{		
			dptr += comp_len;			
			*matchoff = dptr - start;
			return 1;
		}else
			continue;
	}
	return 0;
}

static int sip_addnat(fr_info_t *fin, nat_t *nat, sipentry_t *pinfo)
{
	char naptcmd[256];

	if (pinfo->in_audio_port != 0)
	{
		sprintf(naptcmd, "rdr %s %s/32 port  %d ->", WAN_NAME, inet_ntoa(pinfo->out_addr), pinfo->in_audio_port);
		sprintf(naptcmd, "%s %s port %d udp\n", naptcmd, inet_ntoa(pinfo->in_addr), pinfo->in_audio_port);
		NAT_setrule(naptcmd);
		sprintf(naptcmd, "rdr %s %s/32 port  %d ->", WAN_NAME, inet_ntoa(pinfo->out_addr), pinfo->in_audio_port + 1);
		sprintf(naptcmd, "%s %s port %d udp\n", naptcmd, inet_ntoa(pinfo->in_addr), pinfo->in_audio_port + 1);
		NAT_setrule(naptcmd);
	}
	if (pinfo->in_video_port != 0)
	{
		sprintf(naptcmd, "rdr %s %s/32 port  %d ->", WAN_NAME, inet_ntoa(pinfo->out_addr), pinfo->in_video_port);
		sprintf(naptcmd, "%s %s port %d udp\n", naptcmd, inet_ntoa(pinfo->in_addr), pinfo->in_video_port);
		NAT_setrule(naptcmd);
		sprintf(naptcmd, "rdr %s %s/32 port  %d ->", WAN_NAME, inet_ntoa(pinfo->out_addr), pinfo->in_video_port + 1);
		sprintf(naptcmd, "%s %s port %d udp\n", naptcmd, inet_ntoa(pinfo->in_addr), pinfo->in_video_port + 1);
		NAT_setrule(naptcmd);
	}
}

static int sip_delnat(fr_info_t *fin, nat_t *nat, sipentry_t *pinfo)
{
	char naptcmd[256];

	if (pinfo->in_audio_port != 0)
	{
		sprintf(naptcmd, "rdr %s %s/32 port  %d ->", WAN_NAME, inet_ntoa(pinfo->out_addr), pinfo->in_audio_port);
		sprintf(naptcmd, "%s %s port %d udp\n", naptcmd, inet_ntoa(pinfo->in_addr), pinfo->in_audio_port);
		NAT_delrule(naptcmd);
		sprintf(naptcmd, "rdr %s %s/32 port  %d ->", WAN_NAME, inet_ntoa(pinfo->out_addr), pinfo->in_audio_port + 1);
		sprintf(naptcmd, "%s %s port %d udp\n", naptcmd, inet_ntoa(pinfo->in_addr), pinfo->in_audio_port + 1);
		NAT_delrule(naptcmd);
		pinfo->in_audio_port = 0;		
	}

	if (pinfo->in_video_port != 0)
	{
		sprintf(naptcmd, "rdr %s %s/32 port  %d ->", WAN_NAME, inet_ntoa(pinfo->out_addr), pinfo->in_video_port);
		sprintf(naptcmd, "%s %s port %d udp\n", naptcmd, inet_ntoa(pinfo->in_addr), pinfo->in_video_port);
		NAT_delrule(naptcmd);
		sprintf(naptcmd, "rdr %s %s/32 port  %d ->", WAN_NAME, inet_ntoa(pinfo->out_addr), pinfo->in_video_port + 1);
		sprintf(naptcmd, "%s %s port %d udp\n", naptcmd, inet_ntoa(pinfo->in_addr), pinfo->in_video_port + 1);
		NAT_delrule(naptcmd);
		pinfo->in_video_port = 0;		
	}
	
}

#if 0
static int translate_client_IP_PORT(fr_info_t *fin, ip_t *ip, sipentry_t *pinfo, nat_t *nat)
{
	udphdr_t *udp;
	int siplen, newlen, difflen, inc;
	uint8_t *psip;
	uint8_t *pmatch;
	int mlen;
	mb_t *m;	
	char *p;
	char tempIP[20];

	m = *((mb_t **)fin->fin_mp);
	udp = (tcphdr_t *)fin->fin_dp;
	siplen = fin->fin_dlen -sizeof(udphdr_t);
	psip = (uint8_t *) udp + sizeof(udphdr_t);

	sprintf(&tempIP[0], "%s", inet_ntoa(pinfo->out_addr));
	newlen = strlen(&tempIP[0]);
	sprintf(&tempIP[0], "%s", inet_ntoa(pinfo->in_addr));
	difflen = newlen - strlen(&tempIP[0]);

	SIP_DBG("Replace %s to ", inet_ntoa(pinfo->in_addr));
	SIP_DBG("%s,  difflen = %d\n",  inet_ntoa(pinfo->out_addr), difflen);

	mlen = mbufchainlen(m);
	p = mtod(m, char *);

	p = p + mlen - siplen;
	while ((pinfo) && (p != NULL))
	{
		if (nat->nat_dir == NAT_OUTBOUND) { 	
			p = strstr(p + 1, inet_ntoa(pinfo->in_addr));
			if (p != 0)
				memcpy(&p[0], inet_ntoa(pinfo->out_addr), newlen);
		} else {
			p = strstr(p + 1, inet_ntoa(pinfo->out_addr));
			if (p != 0)
				memcpy(&p[0], inet_ntoa(pinfo->in_addr), newlen);
		}
	}		
}
#endif

/*
 * Initialize local structures.
 */
static int ippr_sip_init(void)
{
	return 0;
}

static int ippr_sip_new(fin, aps, nat)
fr_info_t *fin;
ap_session_t *aps;
nat_t *nat;
{
	sipentry_t *pinfo;
	if (nat->nat_dir != NAT_OUTBOUND) {
		aps->aps_data = NULL;
		aps->aps_psiz = 0;
		return 0;
	}

	KMALLOC(pinfo, sipentry_t *);
	if (pinfo == NULL) {
		SIP_DBG("%s: failed to alloc sip info\n", __FUNCTION__);
		return -1;
	}
	bzero((char *)pinfo, sizeof(sipentry_t));

	pinfo->sip_state = SIP_STATE_INIT;
	pinfo->in_addr = nat->nat_inip;
	pinfo->out_addr = nat->nat_outip;
	aps->aps_data = pinfo;
	aps->aps_psiz = sizeof(sipentry_t);

	SIP_DBG("%s:%d\n", __FUNCTION__, __LINE__);
	
	return 0;
}

static int ippr_sip_in(fin, ip, aps, nat)
fr_info_t *fin;
ip_t *ip;
ap_session_t *aps;
nat_t *nat;
{
	sipentry_t *pinfo;
	udphdr_t *udp;
	int siplen;
	uint8_t *psip;
	
	udp = (tcphdr_t *)fin->fin_dp;
	siplen = fin->fin_dlen -sizeof(udphdr_t);
	psip = (uint8_t *) udp + sizeof(udphdr_t);

	if ((pinfo = (sipentry_t *) aps->aps_data) == NULL)
		return 0;

	if (siplen < strlen("SIP/2.0 200"))
		return 0;

	/* sip_response */
	if (strnicmp(psip, "SIP/2.0 ", strlen("SIP/2.0 ")) == 0)
	{
		unsigned int matchoff;	
		int status;			
		status = strtoul(psip + strlen("SIP/2.0 "), NULL, 10);
		if (status != 200)			
			return 0;

		if (sip_get_header(psip, 0, siplen, "CSeq: ", &matchoff))
		{
			uint8_t *start = psip + matchoff, *limit = psip + siplen;

			while (start >= limit || *start != ' ')
				start++;

			matchoff = start - psip + 1;
			if  (strnicmp(psip + matchoff, "INVITE", strlen("INVITE")) == 0)
				sip_addnat(fin, nat, pinfo); /*  Add NAT mapping  */
		}
	}

	if ((siplen > strlen("BYE")) && (strnicmp(psip, "BYE ", strlen("BYE ")) == 0))
	{
		sip_delnat(fin, nat, pinfo);
		SIP_DBG("%s(%d): BYE\n", __FUNCTION__, __LINE__);
	}

	return 0;
}


static int ippr_sip_out(fin, ip, aps, nat)
fr_info_t *fin;
ip_t *ip;
ap_session_t *aps;
nat_t *nat;
{
	sipentry_t *pinfo;
	udphdr_t *udp;
	int siplen;
	uint8_t *psip;

	udp = (tcphdr_t *)fin->fin_dp;
	siplen = fin->fin_dlen -sizeof(udphdr_t);
	psip = (uint8_t *) udp + sizeof(udphdr_t);

	if ((pinfo = (sipentry_t *) aps->aps_data) == NULL)
		return 0;

	if (siplen < strlen("SIP/2.0 200"))
		return 0;

	 /* sip_response */
	if (strnicmp(psip, "SIP/2.0 ", strlen("SIP/2.0 ")) == 0)
	{
		unsigned int matchoff;	
		int status; 		

		status = strtoul(psip + strlen("SIP/2.0 "), NULL, 10);
		if (status != 200)			
			return 0;

		if (sip_get_header(psip, 0, siplen, "CSeq: ", &matchoff))
		{
			uint8_t *start = psip + matchoff, *limit = psip + siplen;
	
			while (start >= limit || *start != ' ')
				start++;
		
			matchoff = start - psip + 1;
			if	(strnicmp(psip + matchoff, "INVITE", strlen("INVITE")) == 0)
			{
				if (sip_get_header(psip, 0, siplen, "m=audio ", &matchoff))
					pinfo->in_audio_port = strtoul(psip + matchoff, NULL, 10);

				if (sip_get_header(psip, 0, siplen, "m=video ", &matchoff))
					pinfo->in_video_port = strtoul(psip + matchoff, NULL, 10);

				sip_addnat(fin, nat, pinfo); /*  Add NAT mapping  */

				SIP_DBG("%s(%d): SIP/2.0 200 OK audio_port=%d, video_port=%d\n", __FUNCTION__, __LINE__, pinfo->in_audio_port, pinfo->in_video_port);
			}
		}
	}

	/* sip_request */
	if ((siplen > strlen("REGISTER")) && (strnicmp(psip, "REGISTER ", strlen("REGISTER ")) == 0))
	{
		unsigned int matchoff;	
		int value = 0;
		if (sip_get_header(psip, 0, siplen, "Expires: ", &matchoff)) {
			value = strtoul(psip + matchoff, NULL, 10);
			if (value == 0) {
				sip_delnat(fin, nat, pinfo);
				SIP_DBG("%s(%d): REGISTER Expires 0\n", __FUNCTION__, __LINE__);
				return 0;
			}
		}
		
			if (pinfo->sip_state == SIP_STATE_INIT)
				pinfo->sip_state = SIP_STATE_REGISTER;
	}

	if ((siplen > strlen("INVITE")) && (strnicmp(psip, "INVITE ", strlen("INVITE ")) == 0))
	{
		unsigned int matchoff;	
		uint8_t *start, *limit = psip + siplen;
		char ipstr[20];

		if (sip_get_header(psip, 0, siplen, "m=audio ", &matchoff))
			pinfo->in_audio_port = strtoul(psip + matchoff, NULL, 10);

		if (sip_get_header(psip, 0, siplen, "m=video ", &matchoff))
			pinfo->in_video_port = strtoul(psip + matchoff, NULL, 10);
		
		SIP_DBG("%s(%d): INVITE audio_port=%d, video_port=%d\n", __FUNCTION__, __LINE__, pinfo->in_audio_port, pinfo->in_video_port);
	}

	if ((siplen > strlen("BYE")) && (strnicmp(psip, "BYE ", strlen("BYE ")) == 0))
	{
		sip_delnat(fin, nat, pinfo);
		SIP_DBG("%s(%d): BYE\n", __FUNCTION__, __LINE__);
	}
	
	return 0;
}



