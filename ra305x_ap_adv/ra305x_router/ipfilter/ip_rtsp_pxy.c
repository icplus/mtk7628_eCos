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
#define IPF_RTSP_PROXY

// #define DEBUG_RTSP_RPOXY
#ifdef DEBUG_RTSP_RPOXY
#define	RTSP_DBG(fmt, ...)	diag_printf(fmt, ## __VA_ARGS__)
#else
#define RTSP_DBG(fmt, ...)
#endif


#define IS_WSP(c)			(c==' ' || c=='\t')
#define TRIM_LEADING_WSP(p,len) 	while(len>0 && IS_WSP(*p)) {len--; p++;}

#define HDR_RTSP_TRANSPORT		"Transport:"
#define HDR_RTSP_TRANSPORT_LEN		10
#define HDR_RTSP_SESSION		"Session:"
#define HDR_RTSP_SESSION_LEN		8
#define HDR_RTSP_CSEQ			"CSeq:"
#define HDR_RTSP_CSEQ_LEN		5

static int ippr_rtsp_in __P((fr_info_t *, ip_t *, ap_session_t *, nat_t *));
static int ippr_rtsp_init __P((void));
static int ippr_rtsp_new __P((fr_info_t *, ap_session_t *, nat_t *));
static int ippr_rtsp_out __P((fr_info_t *, ip_t *, ap_session_t *, nat_t *));

typedef struct rtpport_s {
	uint16_t cport_l;
	uint16_t cport_h;
	uint16_t sport_l;
	uint16_t sport_h;
	uint16_t mport_l;
} rtpport_t;


#define RTSP_STATE_INIT			0
#define RTSP_STATE_SETUP		1
#define RTSP_STATE_PLAY			2
#define RTSP_STATE_PAUSE		3
#define RTSP_STATE_TEARDOWN		4


#define MAX_RTSP_SESSION_ID_LEN		32
#define MAX_RTP_SESSION_NUM		4
typedef struct rtspinfo_s {
	uint8_t rtsp_state;
	uint8_t sessid_len;
	uint16_t num_rtp;
	rtpport_t ports[MAX_RTP_SESSION_NUM];
	uint8_t sessid[MAX_RTSP_SESSION_ID_LEN];
} rtspinfo_t;


typedef enum {
	RTSPMSG_REPLY = 0,
	RTSPMSG_DESCRIBE,
	RTSPMSG_ANNOUNCE,
	RTSPMSG_GET_PARAM,
	RTSPMSG_OPTIONS,
	RTSPMSG_PAUSE,
	RTSPMSG_PLAY,
	RTSPMSG_RECORD,
	RTSPMSG_REDIRECT,
	RTSPMSG_SETUP,
	RTSPMSG_SET_PARAM,
	RTSPMSG_TEARDOWN,
	RTSPMSG_UNKNOWN
} rtsptype_t;

typedef struct rtspmsg_s {
	rtsptype_t msgtype;
	uint8_t *method;
	uint8_t *cseq;
	uint8_t *transport;
	uint8_t *sessid;
	uint16_t method_len;
	uint16_t cseq_len;
	uint16_t transport_len;
	uint16_t sessid_len;
} rtspmsg_t;

#define RTSPMSG_DESCRIBE_MAGIC		0x44455343	/*  DESC  */
#define RTSPMSG_ANNOUNCE_MAGIC		0x414e4e4f	/*  ANNO  */
#define RTSPMSG_GET_PARAM_MAGIC		0x4745545f	/*  GET_*/
#define RTSPMSG_OPTIONS_MAGIC		0x4f505449	/*  OPTI  */
#define RTSPMSG_PAUSE_MAGIC		0x50415553	/*  PAUS  */
#define RTSPMSG_PLAY_MAGIC		0x504c4159	/*  PLAY  */
#define RTSPMSG_RECORD_MAGIC		0x5245434f	/*  RECO  */
#define RTSPMSG_REDIRECT_MAGIC		0x52454449	/*  REDI  */
#define RTSPMSG_SETUP_MAGIC		0x53455455	/*  SETU  */
#define RTSPMSG_SET_PARAM_MAGIC		0x5345545f	/*  SET_ */
#define RTSPMSG_TEARDOWN_MAGIC		0x54454152	/*  TEAR  */

#define RTSPMSG_REPLY_MAGIC		0x52545350	/*  RTSP  */


static	frentry_t	rtsppxyfr;


#define MIN_RTSPMSG_LEN		24


extern uint8_t *get_line(uint8_t *data, int *len);


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
static int check_rtsp_msg(rtspmsg_t *pmsg, uint8_t *prtsp, int len)
{
	int remain_len, line_len;
	int retval = -2;
	uint32_t *magic;
	uint8_t *pnext;
	
	magic = (uint32_t *)prtsp;
// RTSP_DBG("rtsppxy: magic=%08x\n", *magic);
	switch(ntohl(*magic)) {
#ifdef IDENTIFY_ALL_RTSP_COMMAND
	case RTSPMSG_ANNOUNCE_MAGIC:
		pmsg->msgtype = RTSPMSG_ANNOUNCE;
		RTSP_DBG("rtspchk: ANNOUNCE msg\n");
		break;
		
	case RTSPMSG_DESCRIBE_MAGIC:
		pmsg->msgtype = RTSPMSG_DESCRIBE;
		RTSP_DBG("rtspchk: DESCRIBE msg\n");
		break;
		
	case RTSPMSG_GET_PARAM_MAGIC:
		pmsg->msgtype = RTSPMSG_GET_PARAM;
		RTSP_DBG("rtspchk: GET_PARAM msg\n");
		break;
		
	case RTSPMSG_OPTIONS_MAGIC:
		pmsg->msgtype = RTSPMSG_OPTIONS;
		RTSP_DBG("rtspchk: OPTIONS msg\n");
		break;
		
	case RTSPMSG_PAUSE_MAGIC:
		pmsg->msgtype = RTSPMSG_PAUSE;
		RTSP_DBG("rtspchk: PAUSE msg\n");
		break;
		
	case RTSPMSG_PLAY_MAGIC:
		pmsg->msgtype = RTSPMSG_PLAY;
		RTSP_DBG("rtspchk: PLAY msg\n");
		break;
		
	case RTSPMSG_RECORD_MAGIC:
		pmsg->msgtype = RTSPMSG_RECORD;
		RTSP_DBG("rtspchk: RECORD msg\n");
		break;
		
	case RTSPMSG_REDIRECT_MAGIC:
		pmsg->msgtype = RTSPMSG_REDIRECT;
		RTSP_DBG("rtspchk: REDIRECT msg\n");
		break;

	case RTSPMSG_SET_PARAM_MAGIC:
		pmsg->msgtype = RTSPMSG_SET_PARAM;
		RTSP_DBG("rtspchk: SET_PARAM msg\n");
		break;
#endif	/*  IDENTIFY_ALL_RTSP_COMMAND  */

	case RTSPMSG_REPLY_MAGIC:
		pmsg->msgtype = RTSPMSG_REPLY;
		RTSP_DBG("rtspchk: REPLY msg\n");
		break;
		
	case RTSPMSG_SETUP_MAGIC:
		pmsg->msgtype = RTSPMSG_SETUP;
		RTSP_DBG("rtspchk: SETUP msg\n");
		break;
		
	case RTSPMSG_TEARDOWN_MAGIC:
		pmsg->msgtype = RTSPMSG_TEARDOWN;
		RTSP_DBG("rtspchk: TEARDOWN msg\n");
		break;

	default:
		pmsg->msgtype = RTSPMSG_UNKNOWN;
		RTSP_DBG("rtspchk: unidentified msg\n");
		break;
	}
	
	/*  process request line or response line  */
	pmsg->method = prtsp;
	remain_len = len;
	pnext = get_line(prtsp, &remain_len);
	pmsg->method_len = len - remain_len;
	
	while (remain_len >0) {
		len = remain_len;
		prtsp = pnext;
		pnext = get_line(prtsp, &remain_len);
		line_len = len - remain_len;
		/*  trim leading space  */
		if (!prtsp)
			return retval;
		TRIM_LEADING_WSP(prtsp, line_len);
			
		switch(prtsp[0]) {
		case 'C':
			if (strncasecmp(HDR_RTSP_CSEQ, prtsp, HDR_RTSP_CSEQ_LEN) == 0) {
				pmsg->cseq = prtsp;
				pmsg->cseq_len = line_len;
//				RTSP_DBG("rtspchk: Cseq found\n");
				retval ++;
			}
			break;
		case 'S':
			if (strncmp(HDR_RTSP_SESSION, prtsp, HDR_RTSP_SESSION_LEN) == 0)
			{
				prtsp += HDR_RTSP_SESSION_LEN;
				line_len -= HDR_RTSP_SESSION_LEN;
				TRIM_LEADING_WSP(prtsp, line_len);
				pmsg->sessid = prtsp ;
				pmsg->sessid_len = line_len;
				
//				RTSP_DBG("rtspchk: Session found\n");
			}
			break;
		case 'T':
			if (strncmp(HDR_RTSP_TRANSPORT, prtsp, HDR_RTSP_TRANSPORT_LEN) == 0) {
				pmsg->transport = prtsp;
				pmsg->transport_len = line_len;
//				RTSP_DBG("rtspchk: Transport found\n");
			}
			break;
		case '\r':
			if (line_len==2 && prtsp[1] == '\n') {
				/*  RTSP header end  */
				retval++;
				return retval;
			}
			break;
		}
	}

	
	return retval;
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
static int extract_sessid(rtspmsg_t *pmsg, uint8_t *buf, int buflen)
{
	uint8_t *p;
	int len;

	for (p=pmsg->sessid; *p != ';' && *p!='\r' && *p!='\n'; p++);
	len = p - pmsg->sessid;
	if (len >= buflen)
		len = buflen - 1;
	memcpy(buf, pmsg->sessid, len);
	buf[len] = 0;
	return len;
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
static int check_sessid(rtspinfo_t *pinfo, rtspmsg_t *pmsg)
{
	if (pinfo->sessid[0] == 0) {
		/*  Not yet  */
		return 0;
	}
	if (pmsg->sessid != NULL) {
		if (pinfo->sessid_len >= pmsg->sessid_len)
			return -1;
		if (strncmp(pmsg->sessid, pinfo->sessid, pinfo->sessid_len) != 0)
			return -1;
	}
	
	/*  Session ID check OK  */
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
/* 
	What is concerned here is unicast/UDP only
	Transport: RTP/AVP;client_port=x-x
 */
#define MIN_TRANSPORT_LEN	34
static int parse_transport_line(rtpport_t *pport, rtspmsg_t *pmsg)
{
	int len;
	uint8_t *pline, *p;
	int lo, hi, cnt;
	
	if (pmsg->transport_len < MIN_TRANSPORT_LEN)
		return -1;
	bzero(pport, sizeof(rtpport_t));
	/*  Skip "Transport:" */
	pline = pmsg->transport + HDR_RTSP_TRANSPORT_LEN;
	len = pmsg->transport_len - HDR_RTSP_TRANSPORT_LEN;
	if ((p = strnstr(pline, "client_port", len)) == NULL) {
		/* Maybe TCP or multicast  */
		RTSP_DBG("rtsptp: client_port not found\n");
		return -1;
	}
	
	/*  Skip "client_port="  */
	len -= (p-pline) - 12;
	p += 12;

	cnt = sscanf(p, "%d-%d", &lo, &hi);
	if (cnt == 2) {
		RTSP_DBG("rtsptp: cport=%d-%d\n", lo, hi);
		pport->cport_l = lo;
		pport->cport_h = hi;
		
	} else if (cnt == 1) {
		RTSP_DBG("rtsptp: cport=%d\n", lo);
		pport->cport_l = lo;
		pport->cport_h = lo+1;
	
	} else {
		RTSP_DBG("rtsptp: unable to get client port\n");
		return -1;
	}
#if 0	
	if (sscanf(p, "%d-%d", &lo, &hi) != 2) {
		RTSP_DBG("rtsptp: unable to get client port\n");
		return -1;
	}
	RTSP_DBG("rtsptp: cport=%d-%d\n", lo, hi);
	pport->cport_l = lo;
	pport->cport_h = hi;
#endif

	if (pmsg->msgtype != RTSPMSG_REPLY)
		return 0;
	
	/*  Looking for server_port  */
	pline = p;
	if ((p = strnstr(pline, "server_port", len)) == NULL) {
		/* Maybe TCP or multicast  */
		RTSP_DBG("rtsptp: server_port not found\n");
		return 0;
	}
	/*  Skip "server_port="  */
	len -= (p-pline) - 12;
	p += 12;
	cnt = sscanf(p, "%d-%d", &lo, &hi);
	if (cnt == 2) {
		RTSP_DBG("rtsptp: sport=%d-%d\n\n", lo, hi);
		pport->sport_l = lo;
		pport->sport_h = hi;
		
	} else if (cnt == 1) {
		RTSP_DBG("rtsptp: sport=%d\n", lo);
		pport->sport_l = lo;
		pport->sport_h = lo+1;
	} else {
		RTSP_DBG("rtsptp: unable to get server port\n");
	}
#if 0
	if (sscanf(p, "%d-%d", &lo, &hi) != 2) {
		RTSP_DBG("rtsptp: unable to get server port\n");
		return 0;
	}
	RTSP_DBG("rtsptp: sport=%d-%d\n", lo, hi);
	pport->sport_l = lo;
	pport->sport_h = hi;
#endif
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
#define RTSP_LOCAL_BUF_LEN		768
static int translate_client_port(fr_info_t *fin, ip_t *ip, rtspmsg_t *pmsg, rtpport_t *pport)
{
	tcphdr_t *tcp;
	mb_t *m;
	int linelen, newlen, cplen, inc;
	uint8_t *pcport, *pcportend;
	int off;
	uint8_t lbuf[RTSP_LOCAL_BUF_LEN+32];
//	uint8_t *p;
	
	/*  Locate "client_port"  */
	pcport = strnstr(pmsg->transport, "client_port", pmsg->transport_len);
	pcport += 11;
	linelen = pmsg->transport_len - (pcport - pmsg->transport) - 11;
	if ((pcportend = strnstr(pcport, ";", linelen)) == NULL) {
		for (pcportend=pcport; *pcportend!='\r'&&*pcportend!='\n'; pcportend ++); 
	}
	if (pmsg->msgtype != RTSPMSG_REPLY) {
		sprintf(lbuf, "=%d-%d", pport->mport_l, pport->mport_l+(pport->cport_h-pport->cport_l));
		RTSP_DBG("rtsptc: translate (%d-%d) to (%s), starting at %08x, end at %08x\n", 
			pport->cport_l, pport->cport_h, lbuf+1, (uint32_t)pcport, (uint32_t)pcportend);
	}
	else {
		sprintf(lbuf, "=%d-%d", pport->cport_l, pport->cport_h);
		RTSP_DBG("rtsptc: translate (%d-%d) to (%s), starting at %08x, end at %08x\n", 
			pport->mport_l, pport->mport_l+(pport->cport_h-pport->cport_l), lbuf+1, 
			(uint32_t)pcport, (uint32_t)pcportend);
	}
	newlen = strlen(lbuf);
		
	/*  RTSP message length difference*/
	inc = newlen - (pcportend - pcport);
	if (inc == 0) {
		/*  Dirty hack  */
		memcpy(pcport, lbuf, newlen);
		return 0;
	}

	m = *((mb_t **)fin->fin_mp);
	tcp = (tcphdr_t *)fin->fin_dp;
	off = fin->fin_hlen + (tcp->th_off << 2);
	
	/*  Copy the data behind client_port to local buffer  */
	cplen = mbufchainlen(m) - off - (pcportend - pmsg->method);
	if (cplen > RTSP_LOCAL_BUF_LEN) {
		/*  Should do something useful  */
		RTSP_DBG("rtsptc: too much data\n");
		return 0;
	}
	m_copydata(m, off+(pcportend - pmsg->method), cplen, lbuf+newlen);
	
	if (inc < 0)
		m_adj(m, inc);
	
	/*  Copy translated client_port and anything behind to mbuf chain  */
	m_copyback(m, off+(pcport - pmsg->method), cplen+newlen, lbuf);
	
	/*  Fix ip length and some firewall data  */
	ip->ip_len = htons(ntohs(ip->ip_len) + inc);
	fin->fin_dlen += inc;
	fin->fin_plen += inc;

	return inc;
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
#define MAX_RTPPORT_NUM		4
static int rtsp_addnat(fr_info_t *fin, nat_t *nat, rtpport_t *pport)
{
	int mport, nflags, numport;
	nat_t *ipn, *pnat;
	fr_info_t fi;
	int sp, mp, i;
	
	memcpy(&fi, fin, sizeof(fr_info_t));
	fi.fin_p = IPPROTO_UDP;
	mport=pport->cport_l;
	nflags = IPN_UDP | FI_W_SPORT;
	fi.fin_data[0] = 0;
	
	numport = pport->cport_h - pport->cport_l + 1;
	if (numport > MAX_RTPPORT_NUM)
		numport = MAX_RTPPORT_NUM;
	RTSP_DBG("rtspnat: %d RTP session to be added\n", numport);

// #define FAKE_MAPPING_PORT
#ifndef FAKE_MAPPING_PORT
	/*  Lookup if the desired port is occupied  */
mport_again:
	for (i=0; i<numport; i++) {
		fi.fin_data[1] = htons(mport + i);
		pnat = nat_inlookup(&fi, nflags, IPPROTO_UDP, nat->nat_oip, nat->nat_outip, 0);
		if (pnat != NULL) {
			mport += 20;
			if (mport >= 0xffff)
				mport = 4000;
			goto mport_again;
		}
	}
#else
	mport += 10000;
#endif
	
	/*  Add the nat session with best effort */
	for (i=0, sp=pport->cport_l, mp = mport; i<numport; i++, sp++, mp++) {
		ipn = nat_alloc(NAT_OUTBOUND, IPN_UDP|FI_W_DPORT);
		if (ipn == NULL) {
			RTSP_DBG("rtspnat: natalloc ipn[%d] failed\n", i);
			nat_stats.ns_memfail++;
			return mport;
		}
	
		ipn->nat_ifp = fin->fin_ifp;
		ipn->nat_ptr = nat->nat_ptr;
		nat_setup(ipn, nat->nat_inip, nat->nat_oip, nat->nat_outip, fi.fin_p, htons(sp), 0, htons(mp));
		ipn->nat_fstate.rule = nat->nat_fstate.rule;
		RTSP_DBG("rtspnat: map%d %08x:%d <- -> %08x:%d [%08x]\n", 
			i, nat->nat_inip, sp, nat->nat_oip, mp, nat->nat_outip);
	}
	return mport;
	
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
/*
 * Initialize local structures.
 */
static int ippr_rtsp_init(void)
{
	bzero((char *)&rtsppxyfr, sizeof(rtsppxyfr));
	rtsppxyfr.fr_ref = 1;
	rtsppxyfr.fr_flags = FR_INQUE|FR_PASS|FR_QUICK|FR_KEEPSTATE;
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
static int ippr_rtsp_new(fin, aps, nat)
fr_info_t *fin;
ap_session_t *aps;
nat_t *nat;
{
	rtspinfo_t *pinfo;

	/*  Only handle outbound RTSP request for the moment  */
	if (nat->nat_dir != NAT_OUTBOUND) {
		RTSP_DBG("rtspnew: ignore inbound session\n");

		aps->aps_data = NULL;
		aps->aps_psiz = 0;
		return 0;
	}
		
	KMALLOC(pinfo, rtspinfo_t *);
	if (pinfo == NULL) {
		RTSP_DBG("rtspnew: failed to alloc rtspinfo\n");
		return -1;
	}
	
	aps->aps_data = pinfo;
	aps->aps_psiz = sizeof(rtspinfo_t);

	bzero((char *)pinfo, sizeof(rtspinfo_t));
	pinfo->rtsp_state = RTSP_STATE_INIT;
	RTSP_DBG("rtspnew: rtsp session %08x added\n", (uint32_t)pinfo);
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
static int ippr_rtsp_in(fin, ip, aps, nat)
fr_info_t *fin;
ip_t *ip;
ap_session_t *aps;
nat_t *nat;
{
	rtspinfo_t *pinfo;
	tcphdr_t *tcp;
	int rtsplen, i, inc;
	uint8_t *prtsp;
	rtspmsg_t msg;
	rtpport_t port;
	
	if ((pinfo = (rtspinfo_t *) aps->aps_data) == NULL)
		return 0;

	tcp = (tcphdr_t *)fin->fin_dp;
	rtsplen = fin->fin_dlen - (tcp->th_off << 2);
	prtsp = (uint8_t *) tcp + (tcp->th_off << 2);;
	if (rtsplen <= MIN_RTSPMSG_LEN) {
		if (rtsplen > 0)
			RTSP_DBG("rtspin: msg too short\n");
		return 0;
	}
	bzero(&msg, sizeof(msg));
	if (check_rtsp_msg(&msg, prtsp, rtsplen) < 0) {
		RTSP_DBG("rtspin: invalid rtspmsg found\n");
		return 0;
	}

	if (check_sessid(pinfo, &msg) < 0) {
		RTSP_DBG("rtspin: incorrect sessid\n");
		return 0;
	}

	switch (msg.msgtype) {
	case RTSPMSG_REPLY:
		if ((msg.transport == NULL) || (parse_transport_line(&port, &msg) < 0))
			return 0;
		if (pinfo->sessid[0] == 0 && msg.sessid != NULL) {
			pinfo->sessid_len = extract_sessid(&msg, pinfo->sessid, MAX_RTSP_SESSION_ID_LEN);
			RTSP_DBG("rtspin: sessid=%s\n", pinfo->sessid);
		}
		
		for (i=0; i<pinfo->num_rtp; i++) {
			if (port.cport_l == pinfo->ports[i].mport_l)
				goto in_transcport;
		}
		
in_transcport:
		if ((port.sport_l != 0) && pinfo->ports[i].sport_l == 0) {
			pinfo->ports[i].sport_l = port.sport_l;
			pinfo->ports[i].sport_h = port.sport_h;
		}
		if (pinfo->ports[i].cport_l != pinfo->ports[i].mport_l) {
			/*  Translate mport to cport*/
			inc = translate_client_port(fin, ip, &msg, &pinfo->ports[i]);
			return APR_INC(inc);		
		}
		
		break;
	default:
		break;
	}
	/*  Nothing to do  */
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
static int ippr_rtsp_out(fin, ip, aps, nat)
fr_info_t *fin;
ip_t *ip;
ap_session_t *aps;
nat_t *nat;
{
	rtspinfo_t *pinfo;
	tcphdr_t *tcp;
	int rtsplen, i, inc;
	uint8_t *prtsp;
	rtspmsg_t msg;
	rtpport_t port;
	
	//diag_printf("ippr_rtsp_out()\n");
	if ((pinfo = (rtspinfo_t *) aps->aps_data) == NULL)
		return 0;
	
	tcp = (tcphdr_t *)fin->fin_dp;
	rtsplen = fin->fin_dlen - (tcp->th_off << 2);
	prtsp = (uint8_t *) tcp + (tcp->th_off << 2);;
	if (rtsplen <= MIN_RTSPMSG_LEN) {
		if (rtsplen > 0)
			RTSP_DBG("rtspout: msg too short\n");
		return 0;
	}
	
	bzero(&msg, sizeof(msg));
	if (check_rtsp_msg(&msg, prtsp, rtsplen) < 0) {
		RTSP_DBG("rtspout: invalid rtspmsg found\n");
		return 0;
	}
	
	if (check_sessid(pinfo, &msg) < 0) {
		RTSP_DBG("rtspout: incorrect sessid\n");
		return 0;
	}
	
	switch (msg.msgtype) {
	case RTSPMSG_SETUP:
		if (parse_transport_line(&port, &msg) < 0)
			return 0;
		for (i=0; i<pinfo->num_rtp; i++) {
			if (port.cport_l == pinfo->ports[i].cport_l)
				goto out_transcport;
		}
		if (i >= MAX_RTP_SESSION_NUM) {
			RTSP_DBG("rtspout: can't add new rtp session\n");
			return 0;
		}
		/*  Add new RTP session  */
		pinfo->ports[i] = port;
		pinfo->num_rtp ++;
		RTSP_DBG("rtspout: add RTP session%d, port=%d\n", i, port.cport_l);
		/*  Add NAT mapping  */
		pinfo->ports[i].mport_l = rtsp_addnat(fin, nat, &pinfo->ports[i]);

out_transcport:
		if (pinfo->ports[i].cport_l != pinfo->ports[i].mport_l) {
			/*  Translate CPORT to mport*/
			inc = translate_client_port(fin, ip, &msg, &pinfo->ports[i]);
			return APR_INC(inc);
		}

		break;
		
	case RTSPMSG_TEARDOWN:
		pinfo->sessid[0] = 0;
		pinfo->num_rtp = 0;
		pinfo->sessid_len = 0;
		break;

	default:
		break;
	}
	
	return 0;
}



