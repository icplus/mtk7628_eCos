
/*
 * ng_pppoe.c
 *
 * Copyright (c) 1996-1999 Whistle Communications, Inc.
 * All rights reserved.
 * 
 * Subject to the following obligations and disclaimer of warranty, use and
 * redistribution of this software, in source or object code forms, with or
 * without modifications are expressly permitted by Whistle Communications;
 * provided, however, that:
 * 1. Any and all reproductions of the source or object code must include the
 *    copyright notice above and the following disclaimer of warranties; and
 * 2. No rights are granted, in any manner or form, to use Whistle
 *    Communications, Inc. trademarks, including the mark "WHISTLE
 *    COMMUNICATIONS" on advertising, endorsements, or otherwise except as
 *    such appears in the above copyright notice or in the software.
 * 
 * THIS SOFTWARE IS BEING PROVIDED BY WHISTLE COMMUNICATIONS "AS IS", AND
 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, WHISTLE COMMUNICATIONS MAKES NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED, REGARDING THIS SOFTWARE,
 * INCLUDING WITHOUT LIMITATION, ANY AND ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.
 * WHISTLE COMMUNICATIONS DOES NOT WARRANT, GUARANTEE, OR MAKE ANY
 * REPRESENTATIONS REGARDING THE USE OF, OR THE RESULTS OF THE USE OF THIS
 * SOFTWARE IN TERMS OF ITS CORRECTNESS, ACCURACY, RELIABILITY OR OTHERWISE.
 * IN NO EVENT SHALL WHISTLE COMMUNICATIONS BE LIABLE FOR ANY DAMAGES
 * RESULTING FROM OR ARISING OUT OF ANY USE OF THIS SOFTWARE, INCLUDING
 * WITHOUT LIMITATION, ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * PUNITIVE, OR CONSEQUENTIAL DAMAGES, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES, LOSS OF USE, DATA OR PROFITS, HOWEVER CAUSED AND UNDER ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF WHISTLE COMMUNICATIONS IS ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Author: Julian Elischer <julian@freebsd.org>
 *
 * $FreeBSD: src/sys/netgraph/ng_pppoe.c,v 1.23.2.17 2002/07/02 22:17:18 archie Exp $
 * $Whistle: ng_pppoe.c,v 1.10 1999/11/01 09:24:52 julian Exp $
 */
#include <sys/param.h>
//#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <ppp_api.h>
#include <ng_pppoe.h>
#include <eventlog.h>
#include <cfg_def.h>
#include <cfg_net.h>
#include <sys_status.h>
#include <stdio.h>

#define SIGNOFF "session closed"
#define UPPER_DRV 0
#define LOWER_DRV 1
#define NG_SEND_DATA(flag,m,info) flag?(pppoe_Send(m,(PPPOE_INFO *)info)):(ppp_Ether2Ppp(m,(PPP_IF_VAR_T *)info))
#define PPPOE_PPP_NUM_SESSIONS 1	//define PPPOE use PPP number,currently is 1,user can change in demand here. 

/*
 * This section contains the netgraph method declarations for the
 * pppoe node. These methods define the netgraph pppoe 'type'.
 */
static int ng_pppoe_rcvmsg(PPPOE_INFO *pInfo,struct ng_mesg *msg);
static int ng_pppoe_connect(PPP_IF_VAR_T *pPppIf);
static struct mbuf *ng_pppoe_rcvdata(PPPOE_INFO *pInfo, struct ether_header *eh, struct mbuf *m, int len);
static int ng_pppoe_senddata(PPPOE_INFO *pInfo, struct mbuf *m);
static int ng_pppoe_disconnect(PPP_IF_VAR_T *pPppIf);

struct ether_header eh_prototype =
{
	{0xff,0xff,0xff,0xff,0xff,0xff},
	{0x00,0x00,0x00,0x00,0x00,0x00},
	ETHERTYPE_PPPOE_DISC
};

#define	LEAVE(x) do { error = x; goto quit; } while(0)

static	void pppoe_start(PPPOE_INFO *pInfo);
static	int sendpacket(PPPOE_INFO *pInfo);
static	void pppoe_ticker(PPPOE_INFO *pInfo);
static	int pppoe_send_event(PPPOE_INFO *pInfo, enum cmd cmdid);
static	const struct pppoe_tag *scan_tags(sessp sp, const struct pppoe_hdr* ph);
static	void pppoe_timer_connect(PPPOE_INFO *pInfo);
//struct	mbuf *m_copypacket( struct mbuf *m, int how);

static	int pppoe_Ppp2Ether(PPP_IF_VAR_T *pPppIf, struct mbuf *m0);
		int pppoe_Send(struct mbuf *m, PPPOE_INFO *pInfo);

#ifndef __P
# if defined(__STDC__) || defined(__GNUC__)
#  define __P(x) x
# else
#  define __P(x) ()
# endif
#endif

//static int pppoe_save_sessionid;
extern	int send_padt_flag;
extern char chk_link;
extern int (*dialup_checkp) __P((void));
int pppoe_dialup_checkp(void);
void pppoe_send_padt(u_int16_t sid, char *dhost, struct ifnet *ifp);
#ifdef CONFIG_FW
extern int need_dialup_flag;
#endif

//
// PPPoE data
//
static PPPOE_INFO pppoe_softc[PPPOE_PPP_NUM_SESSIONS];
extern pppoe_passalg_t pppoe_pass_algtab[];
extern int pppoe_avail_pass_alg(void);
static u_int32_t pppoe_avail_pass_algmask(void);
char onetime_username[128+1];
char onetime_password[128+1];

/*************************************************************************
 * Some basic utilities  from the Linux version with author's permission.*
 * Author:	Michal Ostrowski <mostrows@styx.uwaterloo.ca>		 *
 ************************************************************************/
/*
 * Return the location where the next tag can be put 
 */
static __inline const struct pppoe_tag*
next_tag(const struct pppoe_hdr* ph)
{
	return (const struct pppoe_tag*)(((const char*)&ph->tag[0]) + ntohs(ph->length));
}


/*
 * Look for a tag of a specific type
 * Don't trust any length the other end says.
 * but assume we already sanity checked ph->length.
 */
static const struct pppoe_tag*
get_tag(const struct pppoe_hdr* ph, u_int16_t idx)
{
	const char *const end = (const char *)next_tag(ph);
	const char *ptn;
	const struct pppoe_tag *pt = &ph->tag[0];
	
	/*
	 * Keep processing tags while a tag header will still fit.
	 */
	while((const char*)(pt + 1) <= end)
	{
		/*
		 * If the tag data would go past the end of the packet, abort.
		 */
		ptn = (((const char *)(pt + 1)) + ntohs(pt->tag_len));
		if (ptn > end)
			return NULL;
		
		if (pt->tag_type == idx)
			return pt;
		
		pt = (const struct pppoe_tag*)ptn;
	}
	
	return NULL;
}


/**************************************************************************
 * inlines to initialise or add tags to a session's tag list,
 **************************************************************************/
/*
 * Initialise the session's tag list
 */
static void
init_tags(sessp sp)
{
	sp->neg.numtags = 0;
}


static void
insert_tag(sessp sp, const struct pppoe_tag *tp)
{
	int	i;
	negp neg = &sp->neg;
	
	if ((i = neg->numtags++) < NUMTAGS)
	{
		neg->tags[i] = tp;
	}
	else
	{
		diag_printf("pppoe: asked to add too many tags to packet\n");
		neg->numtags--;
	}
}


/*
 * Make up a packet, using the tags filled out for the session.
 *
 * Assume that the actual pppoe header and ethernet header 
 * are filled out externally to this routine.
 * Also assume that neg->wh points to the correct 
 * location at the front of the buffer space.
 */
#define ETHER_MAX_LEN		1518 

static void make_packet
(
	PPPOE_INFO *pInfo
)
{
	struct pppoe_full_hdr *wh;
	const struct pppoe_tag **tag;
	char *dp;
	int count;
	int tlen;
	u_int16_t length = 0;
	sessp sp = &pInfo->sp;
	negp neg = &sp->neg;
	
	wh = &neg->pkt->pkt_header;
	
	if (neg->m == NULL)
	{
		diag_printf("pppoe: make_packet called from wrong state\n");
		// Simon, 2004/06/22
		return;
	}
	
	dp = (char *)wh->ph.tag;
	for (count = 0, tag = neg->tags;
		((count < neg->numtags) && (count < NUMTAGS)); 
		tag++, count++)
	{
		tlen = ntohs((*tag)->tag_len) + sizeof(**tag);
		if ((length + tlen) > (ETHER_MAX_LEN - 4 - sizeof(*wh)))
		{
			diag_printf("pppoe: tags too long\n");
			neg->numtags = count;
			break;	/* XXX chop off what's too long */
		}
		
		bcopy(*tag, (char *)dp, tlen);
		length += tlen;
		dp += tlen;
	}
	
	wh->ph.length = htons(length);
	
	neg->m->m_len = length + sizeof(*wh);
	neg->m->m_pkthdr.len = length + sizeof(*wh);
}


/**************************************************************************
 * Routine to find a particular session that matches an incoming packet	  *
 **************************************************************************/
static int pppoe_findsession
(
	PPPOE_INFO *pInfo,
	const struct ether_header *eh,
	const struct pppoe_hdr *ph
)
{
	sessp sp;
	u_int16_t session = ntohs(ph->sid);
	
	/*
	 * find matching peer/session combination.
	 */
	sp = &pInfo->sp;
	if (sp->state == PPPOE_CONNECTED &&
		sp->Session_ID == session &&
		bcmp(pInfo->destEnet, eh->ether_shost, ETHER_ADDR_LEN) == 0)
	{
		return TRUE;
	}
	
	return FALSE;
}


/**************************************************************************
 * start of Netgraph entrypoints					  *
 **************************************************************************/
/*
 * Get a netgraph control message.
 * Check it is one we understand. If needed, send a response.
 * We sometimes save the address for an async action later.
 * Always free the message.
 */
static int ng_pppoe_rcvmsg
(
	PPPOE_INFO *pInfo,
	struct ng_mesg *msg
)
{
	struct ngpppoe_init_data *ourmsg = NULL;
	int error = 0;
	sessp sp = NULL;
	negp neg = NULL;
	int len;
	
	/* Deal with message according to cookie and command */
	switch (msg->header.typecookie)
	{
	case NGM_PPPOE_COOKIE: 
		switch (msg->header.cmd)
		{
		case NGM_PPPOE_CONNECT:
			ourmsg = (struct ngpppoe_init_data *)msg->data;
			
			if (sizeof(*ourmsg) > msg->header.arglen ||
				sizeof(*ourmsg) + ourmsg->data_len > msg->header.arglen)
			{
				diag_printf("pppoe_rcvmsg: bad arg size");
				LEAVE(EMSGSIZE);
			}
			
			if (ourmsg->data_len > PPPOE_SERVICE_NAME_SIZE)
			{
				diag_printf("pppoe: init data too long (%d)\n", ourmsg->data_len);
				LEAVE(EMSGSIZE);
			}
			
			/*
			 * PPPOE_SERVICE advertisments are set up
			 * on sessions that are in PRIMED state.
			 */
			sp = &pInfo->sp;
			if (sp->state |= PPPOE_SNONE)
			{
				diag_printf("pppoe: Session already active\n");
				LEAVE(EISCONN);
			}
			
			/*
			 * set up prototype header
			 */
			neg = &sp->neg;
			
			bzero(neg, sizeof(*neg));
			MGETHDR(neg->m, M_DONTWAIT, MT_DATA);
			if (neg->m == NULL)
			{
				diag_printf("pppoe: Session out of mbufs\n");
				LEAVE(ENOBUFS);
			}
			
			neg->m->m_pkthdr.rcvif = NULL;
			MCLGET(neg->m, M_DONTWAIT);
			if ((neg->m->m_flags & M_EXT) == 0)
			{
				diag_printf("pppoe: Session out of mcls\n");
				if (neg->m)
				{
					m_freem(neg->m);
					neg->m = NULL;
				}
				
				LEAVE(ENOBUFS);
			}
			
			neg->m->m_len = sizeof(struct pppoe_full_hdr);
			neg->pkt = mtod(neg->m, union packet*);
			neg->pkt->pkt_header.eh = eh_prototype;
			neg->pkt->pkt_header.ph.ver = 0x1;
			neg->pkt->pkt_header.ph.type = 0x1;
			neg->pkt->pkt_header.ph.sid = 0x0000;
			neg->timeout = 0;
			
			/*
			 * Check the hook exists and is Uninitialised.
			 * Send a PADI request, and start the timeout logic.
			 * Store the originator of this message so we can send
			 * a success of fail message to them later.
			 * Move the session to SINIT
			 * Set up the session to the correct state and
			 * start it.
			 */
			len = strlen(pInfo->param.service_name);
			
			neg->service.hdr.tag_type = PTT_SRV_NAME;
			neg->service.hdr.tag_len= htons(len);
			if (len && len <= PPPOE_SERVICE_NAME_SIZE)
			{
				bcopy(pInfo->param.service_name, neg->service.data, len);
			}	
			
			pppoe_start(pInfo);
			break;
			
		case NGM_PPPOE_LISTEN: 
		case NGM_PPPOE_OFFER: 
		case NGM_PPPOE_SERVICE: 
		default:
			LEAVE(EINVAL);
		}
		break;
		
	default:
		LEAVE(EINVAL);
	}
	
	/* Free the message and return */
quit:
	return(error);
}


/*--------------------------------------------------------------
* ROUTINE NAME - pppoe_start
*---------------------------------------------------------------
* FUNCTION:	Start a client into the first state. A separate 
*			function because it can be needed if the negotiation
*			times out.
*
* INPUT:    None 
---------------------------------------------------------------*/
static void pppoe_start
(
	PPPOE_INFO *pInfo
)
{
	struct {
		struct pppoe_tag hdr;
		char data[HOST_UNIQ_LEN];
	} __attribute ((packed)) uniqtag;
	
	sessp sp;
	negp neg;
	
	/* 
	 * kick the state machine into starting up
	 */
	SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "%s send PADI", pInfo->param.ppp_name);
	
	if (pInfo->pconnstate != NULL)
		*pInfo->pconnstate = CONNECTING;
	
	/* reset the packet header to broadcast */
	sp = &pInfo->sp;
	sp->state = PPPOE_SINIT;
	
	neg = &sp->neg;
	neg->pkt->pkt_header.eh = eh_prototype;
	neg->pkt->pkt_header.ph.code = PADI_CODE;
	
	uniqtag.hdr.tag_type = PTT_HOST_UNIQ;
	uniqtag.hdr.tag_len = htons(HOST_UNIQ_LEN);
	memcpy(uniqtag.data, pInfo->unique, HOST_UNIQ_LEN);
	
	init_tags(sp);
	insert_tag(sp, &uniqtag.hdr);
	insert_tag(sp, &neg->service.hdr);
	make_packet(pInfo);
	
	sendpacket(pInfo);
}


/*--------------------------------------------------------------
* ROUTINE NAME - ng_pppoe_rcvdata
*---------------------------------------------------------------
* FUNCTION:	Receive data, and do something with it.
* 			The caller will never free m or meta, so if we use
*			up this data or abort we must free BOTH of these.
*
* INPUT:    None 
---------------------------------------------------------------*/
static struct mbuf *ng_pppoe_rcvdata
(
	PPPOE_INFO *pInfo,
	struct ether_header *eh,
	struct mbuf *m,
	int len
)
{
	const struct pppoe_hdr *ph;
	const struct pppoe_tag *utag = NULL;
	const struct pppoe_tag *tag = NULL;
	const struct pppoe_tag *actag = NULL;
	u_int8_t ac_name[64+1];
	
	sessp sp = &pInfo->sp;
	negp neg = &sp->neg;
	int	error = 0;
	
	u_int16_t session;
	u_int16_t length;
	u_int8_t code;
	u_int32_t  status;
	int i;
	
	/*
	 * Incoming data. 
	 * Dig out various fields from the packet.
	 * use them to decide where to send it.
	 */
	if (m->m_len < sizeof(*ph))
	{
		m = m_pullup(m, sizeof(*ph)); /* Checks length */
		if (m == NULL)
			LEAVE(ENOBUFS);
	}
	
	ph = mtod(m, struct pppoe_hdr *);
	
	session = ntohs(ph->sid);
	code = ph->code;
	
	switch (eh->ether_type)
	{
	case ETHERTYPE_PPPOE_STUPID_DISC:
		eh_prototype.ether_type = ETHERTYPE_PPPOE_STUPID_DISC;
		/* fall through */
	case ETHERTYPE_PPPOE_DISC:
		/*
		 * We need to try to make sure that the tag area
		 * is contiguous, or we could wander off the end
		 * of a buffer and make a mess. 
		 * (Linux wouldn't have this problem).
		 */
		 /*XXX fix this mess */
		if (m->m_pkthdr.len <= MHLEN)
		{
			if (m->m_len < m->m_pkthdr.len)
			{
				m = m_pullup(m, m->m_pkthdr.len);
				if (m == NULL)
					LEAVE(ENOBUFS);
			}
		}
		
		if (m->m_len != m->m_pkthdr.len)
		{
			/*
			 * It's not all in one piece.
			 * We need to do extra work.
			 */
			LEAVE(EMSGSIZE);
		}
		
		switch(code)
		{
		case PADI_CODE:
			/*
			 * We are a server:
			 * Look for a hook with the required service
			 * and send the ENTIRE packet up there.
			 * It should come back to a new hook in 
			 * PRIMED state. Look there for further
			 * processing.
			 */
			LEAVE(EPFNOSUPPORT);
			
		case PADO_CODE:
			SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "%s rcv PADO", pInfo->param.ppp_name);
			
			/*
			 * Check the session is in the right state.
			 * It needs to be in PPPOE_SINIT.
			 */
			if (sp->state != PPPOE_SINIT)
			{
				SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "Not PPPOE_SINIT");
				LEAVE(ENETUNREACH);
			}
			
			/*
			 * We are a client:
			 * Use the host_uniq tag to find the 
			 * hook this is in response to.
			 * Received #2, now send #3
			 * For now simply accept the first we receive.
			 */
			for (i=0; i<NUM_AUTH_FAIL_LOG; i++)
			{
				if (pInfo->fail_log[i].state == AUTH_BLOCKED &&
					memcmp(pInfo->fail_log[i].dst, eh->ether_shost, 6) == 0)
				{
					if (++pInfo->fail_log[i].count <= PADO_NEGLECTED_MAX)
					{
						SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, 
								"Server %02x:%02x:%02x:%02x:%02x:%02x is in auth failed list",
								pInfo->fail_log[i].dst[0],
								pInfo->fail_log[i].dst[1],
								pInfo->fail_log[i].dst[2],
								pInfo->fail_log[i].dst[3],
								pInfo->fail_log[i].dst[4],
								pInfo->fail_log[i].dst[5]);
								
						LEAVE(ENETUNREACH);
					}
					
					// Reconsider this one
					SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, 
							"Reconsider pppoe server %02x:%02x:%02x:%02x:%02x:%02x",
							pInfo->fail_log[i].dst[0],
							pInfo->fail_log[i].dst[1],
							pInfo->fail_log[i].dst[2],
							pInfo->fail_log[i].dst[3],
							pInfo->fail_log[i].dst[4],
							pInfo->fail_log[i].dst[5]);
							
					pInfo->fail_log[i].state = AUTH_BLOCK_NONE;
					pInfo->fail_log[i].flag = 0;
					break;
				}
			}
			
			utag = get_tag(ph, PTT_HOST_UNIQ);
			if (utag == NULL ||
				ntohs(utag->tag_len) != HOST_UNIQ_LEN ||
				memcmp(utag->tag_data, pInfo->unique, HOST_UNIQ_LEN) != 0)
			{
				SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "Host unique not matched");
				LEAVE(ENETUNREACH);
			}
			
			ac_name[0]=0;
			actag = get_tag(ph, PTT_AC_NAME);
			if (actag)
			{
				int len = ntohs(actag->tag_len);
				
				memcpy(ac_name, actag->tag_data, len);
				ac_name[len]=0;
			}
			
			// if pInfo->param.ac_name == "", don't check AC_NAME
			if (pInfo->param.ac_name[0] != 0 &&
				strcmp(ac_name, pInfo->param.ac_name) != 0)
			{
				SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "Incoming AC_NAME = %s not matched", ac_name);
				LEAVE(ENETUNREACH);
			}
			
			/*
			 * This is the first time we hear
			 * from the server, so note it's
			 * unicast address, replacing the
			 * broadcast address .
			 */
			bcopy(eh->ether_shost,
				neg->pkt->pkt_header.eh.ether_dhost,
				ETHER_ADDR_LEN);
					
			bcopy(eh->ether_shost,
				pInfo->destEnet,
				ETHER_ADDR_LEN);
					
			neg->timeout = 0;
			neg->pkt->pkt_header.ph.code = PADR_CODE;
			
			init_tags(sp);
			insert_tag(sp, utag);      /* Host Unique */
			if ((tag = get_tag(ph, PTT_AC_COOKIE)))
				insert_tag(sp, tag); /* return cookie */
			if (actag)
				insert_tag(sp, actag); /* return it */
			insert_tag(sp, &neg->service.hdr); /* Service */
			
			scan_tags(sp, ph);
			make_packet(pInfo);
			
			// Send PADR out
			SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE,"Send PADR, Enter PPPOE_SREQ");
			
			// Stop timer
			untimeout((timeout_fun *)pppoe_ticker, (void *)pInfo);
			
			// Enter SREQ state
			sp->state = PPPOE_SREQ;
			sendpacket(pInfo);
			break;
			
		case PADR_CODE:
			LEAVE(EPFNOSUPPORT);
			
		case PADS_CODE:
			SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "%s rcv PADS", pInfo->param.ppp_name);
			
			/*
			 * Check the session is in the right state.
			 * It needs to be in PPPOE_SREQ.
			 */
			if (sp->state != PPPOE_SREQ)
			{
				SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "Not PPPOE_SREQ");
				LEAVE(ENETUNREACH);
			}
			
			/*
			 * We are a client:
			 * Use the host_uniq tag to find the 
			 * hook this is in response to.
			 * take the session ID and store it away.
			 * Also make sure the pre-made header is
			 * correct and set us into Session mode.
			 */
			utag = get_tag(ph, PTT_HOST_UNIQ);
			if (utag == NULL || 
				ntohs(utag->tag_len) != HOST_UNIQ_LEN ||
				memcmp(utag->tag_data, pInfo->unique, HOST_UNIQ_LEN) != 0)
			{
				SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "Host unique not matched");
				LEAVE (ENETUNREACH);
			}
			
			neg->pkt->pkt_header.ph.sid = ph->sid;
			sp->Session_ID = ntohs(ph->sid);
				
			SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE,"Enter PPPOE_CONNECTED, Session_ID=%d",sp->Session_ID);
			
			neg->timeout = 0;
			sp->state = PPPOE_CONNECTED;
			
			/*
			 * Now we have gone to Connected mode, 
			 * Free all resources needed for 
			 * negotiation.
			 * Keep a copy of the header we will be using.
			 */
			sp->pkt_hdr = neg->pkt->pkt_header;
			sp->pkt_hdr.eh.ether_type = ETHERTYPE_PPPOE_SESS;
			sp->pkt_hdr.ph.code = 0;
			if (neg->m)
			{
				m_freem(neg->m);
				neg->m = NULL;
			}
			
			// Stop timer and enter data stage
			untimeout((timeout_fun *)pppoe_ticker,(void *)pInfo);
			
			PppEth_Start(&pInfo->if_ppp);
			break;
			
		case PADT_CODE:
			/*
			 * Send a 'close' message to the controlling
			 * process (the one that set us up);
			 * And then tear everything down.
			 *
			 * Find matching peer/session combination.
			 */
			SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE,"%s rcv PADT(SID:%d)", pInfo->param.ppp_name, ntohs(ph->sid));
			
			status = pppoe_findsession(pInfo, eh, ph);
			if (status == 0)
			{
				SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "Session not found");
				LEAVE(ENETUNREACH);
			}
			
			/* send message to creator */
			/* acknowledge ppp */
			pppSendTerminateEvent(&pInfo->if_ppp);
			break;
			
		default:
			LEAVE(EPFNOSUPPORT);
		}
		break;
		
	case ETHERTYPE_PPPOE_STUPID_SESS:
	case ETHERTYPE_PPPOE_SESS:
		/*
		 * find matching peer/session combination.
		 */
		status = pppoe_findsession(pInfo, eh, ph);
		if (status == 0)
		{
			SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "Rcv unknown ETHERTYPE_PPPOE_SESSS(%d)", ntohs(ph->sid));
			LEAVE (ENETUNREACH);
		}
		
		sp = &pInfo->sp;

		/* strip PPPOE header  */
		length = ntohs(ph->length);
		m_adj(m, sizeof(*ph));

		if (m->m_pkthdr.len < length)
		{
			/* Packet too short, dump it */
			LEAVE(EMSGSIZE);
		}
		
		/* Also need to trim excess at the end */
		if (m->m_pkthdr.len > length)
		{
			m_adj(m, -((int)(m->m_pkthdr.len - length)));
		}

		ppppktin(&pInfo->if_ppp, m);
		//--
		return OK;
		
	default:
		LEAVE(EPFNOSUPPORT);
	}
	
quit:
	if (error == ENETUNREACH)
		return m;
	
	// No need for the next interface to process
	if (m)
		m_freem(m);
	
	return NULL;
}


static int ng_pppoe_senddata
(
	PPPOE_INFO *pInfo,
	struct mbuf *m
) 
{
	static const u_char addrctrl[] = { 0xff, 0x03 };
	struct pppoe_full_hdr *wh;
	int	error = 0;
	sessp sp = &pInfo->sp;
	
	/*
	 * 	Not ethernet or debug hook..
	 *
	 * The packet has come in on a normal hook.
	 * We need to find out what kind of hook,
	 * So we can decide how to handle it.
	 * Check the hook's state.
	 */
	switch (sp->state)
	{
	case PPPOE_CONNECTED:
	{
		/*
		 * Remove PPP address and control fields, if any.
		 * For example, ng_ppp(4) always sends LCP packets
		 * with address and control fields as required by
		 * generic PPP. PPPoE is an exception to the rule.
		 */
		if (m->m_pkthdr.len >= 2)
		{
			if (m->m_len < 2 && !(m = m_pullup(m, 2)))
				LEAVE(ENOBUFS);
			
			if (bcmp(mtod(m, u_char *), addrctrl, 2) == 0)
				m_adj(m, 2);
		}
		
		/*
		 * Bang in a pre-made header, and set the length up
		 * to be correct. Then send it to the ethernet driver.
		 * But first correct the length.
		 */
		if (m->m_next != 0)
		{
			short total_len;
			struct mbuf *m0;
			
			m0 = m; 
			total_len = m->m_len;
			while (m->m_next)
			{
				total_len+=m->m_next->m_len;	
				m=m->m_next;
			}
			
			m = m0;	
			sp->pkt_hdr.ph.length = htons((short)(total_len));
		}   
		else
		{
			sp->pkt_hdr.ph.length = htons((short)(m->m_len));
		}
		
		M_PREPEND(m, sizeof(*wh), M_DONTWAIT);
		if (m == NULL)
		{
			LEAVE(ENOBUFS);
		}
		
		wh = mtod(m, struct pppoe_full_hdr *);
		bcopy(&sp->pkt_hdr, wh, sizeof(*wh));
		
		// Don't free this one, the upper layer will do it.
		NG_SEND_DATA(LOWER_DRV,m,pInfo);
		return OK;
	}
		
	/*
	 * Packets coming from the hook make no sense
	 * to sessions in these states. Throw them away.
	 */
	case PPPOE_NEWCONNECTED:
	case PPPOE_PRIMED:
	case PPPOE_SINIT:
	case PPPOE_SREQ:
	case PPPOE_SOFFER:
	case PPPOE_SNONE:
	case PPPOE_LISTENING:
	case PPPOE_DEAD:
	default:
		LEAVE(ENETUNREACH);
	}
	
quit:
	if (m)
		m_freem(m);
	
	return error;
}


/*
 * This is called once we've already connected a new hook to the other node.
 * It gives us a chance to balk at the last minute.
 */
static int
ng_pppoe_connect(PPP_IF_VAR_T *pPppIf)
{
	PPPOE_INFO  *pInfo = (PPPOE_INFO *)pPppIf;
	sessp sp = &pInfo->sp;
	
	if (pPppIf->authfail == 1)
	{
		int i;
		negp neg = &pInfo->sp.neg;
		int empty_slot = -1;
		
		SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "authentication failed log");
		
		for (i=0; i<NUM_AUTH_FAIL_LOG; i++)
		{
			switch (pInfo->fail_log[i].state)
			{
			case AUTH_BLOCK_NONE:
				if (empty_slot == -1)
					empty_slot = i;
				break;
				
			case AUTH_BLOCK_CANDIDATE:
				if (memcmp(pInfo->fail_log[i].dst, pInfo->destEnet, 6) == 0)
					goto check_slot;
				break;
			
			case AUTH_BLOCKED:
			default:
				continue;
			}
		}
		
check_slot:
		if (i != NUM_AUTH_FAIL_LOG || empty_slot != -1)
		{
			if (i == NUM_AUTH_FAIL_LOG)
			{
				i = empty_slot;
				
				memcpy(pInfo->fail_log[i].dst, pInfo->destEnet, 6);
				pInfo->fail_log[i].state = AUTH_BLOCK_CANDIDATE;
			}
			
			// Set flag
			pInfo->fail_log[i].flag |= (0x1 << pInfo->passalg_idx);
			if (pInfo->param.passwd_algorithm != PPPOE_PASSWDALG_AUTO ||
				pInfo->fail_log[i].flag == pppoe_avail_pass_algmask())
			{
				pInfo->fail_log[i].state = AUTH_BLOCKED;
				pInfo->fail_log[i].count = 0;
				SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, 
						"Server %02x:%02x:%02x:%02x:%02x:%02x auth fail, log it",
						pInfo->fail_log[i].dst[0],
						pInfo->fail_log[i].dst[1],
						pInfo->fail_log[i].dst[2],
						pInfo->fail_log[i].dst[3],
						pInfo->fail_log[i].dst[4],
						pInfo->fail_log[i].dst[5]);
			}
		}
		
		untimeout((timeout_fun *)pppoe_ticker, (void *)pInfo);
		
		neg->timeout = PPPOE_INITIAL_TIMEOUT;
		timeout((timeout_fun *)pppoe_ticker,(void *)pInfo, neg->timeout * hz);
		
		// Fixed for auto-connect mode won't clear everything
		pPppIf->authfail = 0;
		pInfo->last_passalg_idx = -1;
	}
	
	if (sp->state == PPPOE_CONNECTED)
	{
		struct pppoe_full_hdr *wh;
		
		/* revert the stored header to DISC/PADT mode */
	 	wh = &sp->pkt_hdr;
		pppoe_send_padt(wh->ph.sid, wh->eh.ether_dhost, pInfo->eth_ifp);
	}
	
	return 0;
}


/*
 * Hook disconnection
 *
 * Clean up all dangling links and information about the session/hook.
 * For this type, removal of the last link destroys the node
 */
static int ng_pppoe_disconnect
(
	PPP_IF_VAR_T *pPppIf
)
{
	PPPOE_INFO *pInfo = (PPPOE_INFO *)pPppIf;
	sessp sp = &pInfo->sp;
	negp neg = &sp->neg;
	
	//untimeout((timeout_fun *)pppoe_timer_connect, pInfo);
	
	/*
	 * According to the spec, if we are connected,
	 * we should send a DISC packet if we are shutting down
	 * a session.
	 */
	if (sp->state == PPPOE_CONNECTED)
	{
		struct pppoe_full_hdr *wh;
		
		/* revert the stored header to DISC/PADT mode */
	 	wh = &sp->pkt_hdr;
		pppoe_send_padt(wh->ph.sid, wh->eh.ether_dhost, pInfo->eth_ifp);
	}
	
	/*
	 * As long as we have somewhere to store the timeout handle,
	 * we may have a timeout pending.. get rid of it.
	 */
	untimeout((timeout_fun *)pppoe_ticker, (void *)pInfo);
	if (neg->m)
	{
		m_freem(neg->m);
		neg->m = NULL;
	}
	
	pInfo->activated = 0;
	sp->state = PPPOE_SNONE;
	
#ifdef CONFIG_POE_STOP_DIAL_IF_AUTH_FAIL     
    if(pPppIf->authfail == 1)
    	mon_snd_cmd( MON_CMD_WAN_STOP );
#endif
    
	return (0);
}


/*
 * timeouts come here.
 */
static void pppoe_ticker
(
	PPPOE_INFO *pInfo
)
{
	int s = splnet();
	
	sessp sp = &pInfo->sp;
	negp neg = &sp->neg;
	
	struct mbuf *m0 = NULL;
	
	switch (sp->state)
	{
	/*
	 * resend the last packet, using an exponential backoff.
	 * After a period of time, stop growing the backoff,
	 * and either leave it, or revert to the start.
	 */
	case PPPOE_SINIT:
	case PPPOE_SREQ:
		/* timeouts on these produce resends */
		untimeout((timeout_fun *)pppoe_ticker, (void *)pInfo);
		m0 = m_copypacket(neg->m, M_DONTWAIT);
		if (!m0)
		{
			SysLog(LOG_USER|SYS_LOG_DEBUG|LOGM_PPPOE, "%s: m_copypacket return null", __FUNCTION__);
		    break;
		}		
		NG_SEND_DATA( LOWER_DRV, m0, pInfo);
		timeout((timeout_fun *)pppoe_ticker,(void *)pInfo, neg->timeout * hz);
					
		if ((neg->timeout <<= 1) > PPPOE_TIMEOUT_LIMIT) 
		{
			if (sp->state == PPPOE_SREQ)
			{
				/* revert to SINIT mode */
				pppoe_start(pInfo);
			}
			else
			{
				//neg->timeout = PPPOE_TIMEOUT_LIMIT;
				neg->timeout = PPPOE_INITIAL_TIMEOUT;
			}
		}
		break;
		
	case PPPOE_PRIMED:
	case PPPOE_SOFFER:
		break;
		
	default:
		/* timeouts have no meaning in other states */
		diag_printf("pppoe: unexpected timeout\n");
	}
	splx(s);
}


static int sendpacket
(
	PPPOE_INFO *pInfo
)
{
	int	error = 0;
	struct mbuf *m0 = NULL;
	sessp sp = &pInfo->sp;
	negp neg = &sp->neg;
	
	
	switch(sp->state)
	{
	case PPPOE_SINIT:
	case PPPOE_SREQ:
		m0 = m_copypacket(neg->m, M_DONTWAIT);
		if (!m0)
		{
			SysLog(LOG_USER|SYS_LOG_DEBUG|LOGM_PPPOE, "%s: m_copypacket return null", __FUNCTION__);
		    break;
		}		
		NG_SEND_DATA( LOWER_DRV, m0, pInfo);
		
		timeout((timeout_fun *)pppoe_ticker, (void *)pInfo, (hz * PPPOE_INITIAL_TIMEOUT));			
		neg->timeout = PPPOE_INITIAL_TIMEOUT * 2;
		break;
	
	case PPPOE_LISTENING:
	case PPPOE_DEAD:
	case PPPOE_SNONE:
	case PPPOE_CONNECTED:
	case PPPOE_NEWCONNECTED:
	case PPPOE_PRIMED:
	case PPPOE_SOFFER:
	default:
		error = EINVAL;
		diag_printf("pppoe: timeout: bad state\n");
	}
	
	return error;
}


/*
 * Parse an incoming packet to see if any tags should be copied to the
 * output packet. Don't do any tags that have been handled in the main
 * state machine.
 */
static const struct pppoe_tag* scan_tags
(
	sessp sp,
	const struct pppoe_hdr* ph
)
{
	const char *const end = (const char *)next_tag(ph);
	const char *ptn;
	const struct pppoe_tag *pt = &ph->tag[0];
	
	/*
	 * Keep processing tags while a tag header will still fit.
	 */
	while((const char*)(pt + 1) <= end)
	{
		/*
		 * If the tag data would go past the end of the packet, abort.
		 */
		ptn = (((const char *)(pt + 1)) + ntohs(pt->tag_len));
		if(ptn > end)
			return NULL;

		switch (pt->tag_type)
		{
		case PTT_RELAY_SID:
			insert_tag(sp, pt);
			break;
			
		case PTT_EOL:
			return NULL;
			
		case PTT_SRV_NAME:
		case PTT_AC_NAME:
		case PTT_HOST_UNIQ:
		case PTT_AC_COOKIE:
		case PTT_VENDOR:
		case PTT_SRV_ERR:
		case PTT_SYS_ERR:
		case PTT_GEN_ERR:
			break;
		}
		pt = (const struct pppoe_tag*)ptn;
	}
	return NULL;
}


static int pppoe_send_event
(
	PPPOE_INFO *pInfo,
	enum cmd cmdid
)
{
	char data[sizeof(struct ng_mesg) + sizeof(struct ngpppoe_init_data)];
	struct ng_mesg *msg = (struct ng_mesg *)data;
	
	NG_MKMESSAGE(msg, NGM_PPPOE_COOKIE, cmdid, sizeof(struct ngpppoe_init_data), M_NOWAIT);
	if(msg)
		return ng_pppoe_rcvmsg(pInfo, msg);
}


/*--------------------------------------------------------------
* ROUTINE NAME - pppoe_InputRtn
*---------------------------------------------------------------
* FUNCTION:   
*
* INPUT:    None 
---------------------------------------------------------------*/
int pppoe_InputRtn
(
	struct arpcom *pArpcom,
	struct ether_header *eh,
	struct mbuf *m,
	int len
)
{
	int i;
	PPPOE_INFO *pInfo;
	
	for (i=0; i<PPPOE_PPP_NUM_SESSIONS; i++)
	{
		pInfo = &pppoe_softc[i];
		if (pInfo->eth_ifp == (struct ifnet *)pArpcom)
		{
			m = ng_pppoe_rcvdata(pInfo, eh, m, len);
			if (m == NULL)
				return TRUE;
		}
	}
	
	// Send PADT if a data phase packet accept
	// Maybe we should think about pppoe pass through method
	if (eh->ether_type == ETHERTYPE_PPPOE_SESS &&
		bcmp(eh->ether_dhost, pArpcom->ac_enaddr, ETHER_ADDR_LEN) == 0)
	{
		if (send_padt_flag	== 1)
		{
			const struct pppoe_hdr *ph = mtod(m, struct pppoe_hdr *);
			
			SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE,"Recv wrong session ID %d(with my mac)", ph->sid);
			
			for (i=0; i<PPPOE_PPP_NUM_SESSIONS; i++)
			{
				sessp sp = &pInfo->sp;
				negp neg = &sp->neg;
				
				pInfo = &pppoe_softc[i];
				sp = &pInfo->sp;
				neg = &sp->neg;
				
				// Simon, 2006.03.02
				if (pInfo->param.enabled == 0)	// Skip not enabled ones
					continue;
				
				if (pInfo->activated && bcmp(eh->ether_shost, pInfo->destEnet, 6) == 0)
				{
					if (sp->state == PPPOE_SREQ)
					{
						// Skip the case find at HC
						goto drop_it;
					}
					else
					{
						break;
					}
				}
				
				// Simon, only skip the case receive LCP before we received PADR
				//if (pInfo->activated == 0 || pInfo->sp.state != PPPOE_CONNECTED)
				//	goto drop_it;
			}
			
			// Now we are sure this session is not belonged to us,
			// and we are all in PPPOE_CONNECTED stat, send PADT
			pppoe_send_padt(ph->sid, eh->ether_shost, (struct ifnet *)pArpcom);
		}
	}
	
drop_it:
	// No one adopted, free it.
	m_freem(m);
	return TRUE;
}


int pppoe_Send
(
	struct mbuf *m,
	PPPOE_INFO *pInfo
)
{
    //int status;
    struct sockaddr dstAddr;
    struct ether_header *pEthHdr;
	
	sessp sp = &pInfo->sp;
	negp neg = &sp->neg;
    int s;
	
	 if (m == NULL)
		return OK;
	
	memset(&dstAddr, 0, sizeof(dstAddr));
	
	// ether_output will prepend the ethernet header
	dstAddr.sa_len = 16;
	dstAddr.sa_family = AF_UNSPEC;
	
    // fill in ethernet header
    pEthHdr = (struct ether_header *) dstAddr.sa_data;
    
    if (sp->state != PPPOE_CONNECTED)
    {
		pEthHdr->ether_type = eh_prototype.ether_type;
		if (neg->pkt->pkt_header.ph.code == PADI_CODE)
		{
			bcopy (eh_prototype.ether_dhost, pEthHdr->ether_dhost, sizeof(pEthHdr->ether_dhost));
		}
		else
		{
			bcopy (m->m_data, pEthHdr->ether_dhost, sizeof(pEthHdr->ether_dhost));
		}
	}
	else
	{
		pEthHdr->ether_type = sp->pkt_hdr.eh.ether_type;
		bcopy (pInfo->destEnet, pEthHdr->ether_dhost, sizeof(pEthHdr->ether_dhost));
	}
	
    m->m_len -= 14;
    m->m_data += 14;
    m->m_pkthdr.len -= 14;
	
    s = splimp();
    ether_output(pInfo->eth_ifp, m, &dstAddr, (struct rtentry *)NULL);
    splx(s);
	
	//if (status != OK) 
    //{
		//diag_printf("pppoe::got you!\n");
	    //m_free(m);
	    //return ERROR;
    //}

    return OK;
}


/*--------------------------------------------------------------
* ROUTINE NAME - pppoe_DaemonStart
*---------------------------------------------------------------
* FUNCTION:   
*
* INPUT:    None 
---------------------------------------------------------------*/
int pppoe_DaemonStart
(
	PPPOE_INFO *pInfo
)
{
	PPPOE_PARAM_STRUCT *param = &pInfo->param;
	u_char *srcAddr = NULL;
	sessp sp;
	
	SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "%s start...", param->ppp_name);
	
	// Check if this interface is working.
	if (ifunit(param->ppp_name) != NULL)
	{
		SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "%s had been started", param->ppp_name);
		return OK;
	}
    
	//
	// Set ppp information
	//
	memset(pInfo, 0, (unsigned long)&((PPPOE_INFO *)0)->param);
	
	if ((pInfo->eth_ifp = ifunit(param->eth_name)) != NULL)
	{
		struct arpcom * ac = (struct arpcom *)pInfo->eth_ifp;
		srcAddr = ac->ac_enaddr;
	}
	else
	{
		SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "Cannot find %s", param->eth_name);
		return ERROR;
	}
	
	pInfo->e_unit = param->unit;
	strcpy(pInfo->e_ifname, param->eth_name);
	strcpy(pInfo->e_pppname, param->ppp_name);
	
	strcpy(pInfo->e_user, param->username);
	strcpy(pInfo->e_password, param->password);
	
	pInfo->e_type = PPP_ENCAP_ETHERNET;
	pInfo->e_idle = param->idle_time;
	pInfo->e_defaultroute = param->dflrt; 
	pInfo->e_pOutputHook = pppoe_Ppp2Ether;
	pInfo->e_pIfStopHook = ng_pppoe_disconnect;
	pInfo->e_pIfReconnectHook = ng_pppoe_connect;
	
	pInfo->e_mru = param->mtu;
	pInfo->e_mtu = param->mtu;
	pInfo->e_netmask = param->netmask;
	pInfo->e_myip = param->my_ip;
	pInfo->e_peerip = param->peer_ip;
	pInfo->e_pridns = param->pri_dns;
	pInfo->e_snddns = param->snd_dns;
	
	//
	// Setup pppoe related information
	//
	memcpy(pInfo->srcEnet, srcAddr, 6);
	
	memcpy(pInfo->unique, srcAddr, 6);
	pInfo->unique[6] = 0;
	pInfo->unique[7] = param->unit;
	
	memcpy(pInfo->srcEnet, srcAddr, 6);
	memcpy(pInfo->destEnet, eh_prototype.ether_dhost, ENET_LEN);
	
	pInfo->activated = TRUE;
	
	//
	// Init pppoe state and start pppoe
	//
	sp = &pInfo->sp;
	sp->state = PPPOE_SNONE;
	
	pppoe_send_event(pInfo,NGM_PPPOE_CONNECT);
	return OK;
}


/*--------------------------------------------------------------
* ROUTINE NAME - pppoe_init_connect_mode
*---------------------------------------------------------------
* FUNCTION:   
*
* INPUT:    None 
---------------------------------------------------------------*/
static int pppoe_init_connect_mode
(
	PPPOE_INFO *pInfo
)
{
	extern int manual_connect_with_idle;
	
	SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "%s connect mode=%d", pInfo->param.ppp_name, pInfo->param.connect_mode);
	
	untimeout((timeout_fun *)pppoe_timer_connect, pInfo);
	
	switch (pInfo->param.connect_mode)
	{
	case 0:			// Keep alive
		pInfo->param.idle_time = 0;
		break;
	case 1:
		dialup_checkp = pppoe_dialup_checkp;
		break;
	case 2:
		if (manual_connect_with_idle == 0)
			pInfo->param.idle_time = 0;
		break;
	case 3:
		pInfo->param.idle_time = 0;
		timeout((timeout_fun *)pppoe_timer_connect, pInfo, 100);
		break;
	case 4:
		pInfo->param.idle_time = 0;
		break;
	default:
		diag_printf("PPPoE: wrong connect mode\n");	
    }  
    return 0;
}


/*--------------------------------------------------------------
* ROUTINE NAME - pppoe_init_param
*---------------------------------------------------------------
* FUNCTION:   
*
* INPUT:    None 
---------------------------------------------------------------*/
int pppoe_init_param
(
	int unit,
	PPPOE_INFO *pInfo
)
{
	int i = unit, j;
	int val;
	PPPOE_PARAM_STRUCT *param = &pInfo->param;
	
	char username[PPPOE_MAX_USRNAME_LEN+33];
	char password[PPPOE_MAX_PASSWD_LEN+33];
	int num_passalg;
	pppoe_passalg_t *passalg;
	int passalg_sel;
	
	// Get username and password to decide
	// Whether we should clear auth fail log or not
	CFG_get(CFG_POE_USER+i, username);
	CFG_get(CFG_POE_PASS+i, password);

	num_passalg = pppoe_avail_pass_alg();
	if ((CFG_get(CFG_POE_PASSWDALG+i, &passalg_sel)) < 0) {
		passalg_sel = 0;
	}

	if (strcmp(username, pInfo->param.org_usrname) != 0 ||
		strcmp(password, pInfo->param.opass) != 0 ||
		param->passwd_algorithm != passalg_sel)
	{
		SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "clear ppp%d auth fail log", unit);
		memset(pInfo->fail_log, 0, sizeof(pInfo->fail_log));
		
		pInfo->passalg_idx = -1;
	}

	// Reset fill all the configuration
	memset(param, 0, sizeof(*param));
	strcpy(pInfo->param.org_usrname, username);
	param->passwd_algorithm = passalg_sel;

	if (passalg_sel != PPPOE_PASSWDALG_AUTO) {
		for (j=0; j<num_passalg; j++) {
			if (pppoe_pass_algtab[j].algid == passalg_sel)
				break;
		}
		if (j< num_passalg)
			pInfo->passalg_idx = j;
		else
			passalg_sel = PPPOE_PASSWDALG_AUTO;
	}

	if (passalg_sel == PPPOE_PASSWDALG_AUTO) {
		/*  Try all username/password algorithm automatically  */
		if (pInfo->last_passalg_idx >= 0) {
			pInfo->passalg_idx = pInfo->last_passalg_idx;
		}
		else {
			if (++ pInfo->passalg_idx >= num_passalg)
				pInfo->passalg_idx = 0;
			pInfo->last_passalg_idx = pInfo->passalg_idx;
		}
	}

	if (pInfo->passalg_idx<0 || pInfo->passalg_idx>=num_passalg)
		pInfo->passalg_idx = 0;
	passalg = &pppoe_pass_algtab[pInfo->passalg_idx];
	if (passalg->pppoe_passalg_fun != NULL) {
		(*passalg->pppoe_passalg_fun)(username, password);
	}
	
	// Check configuration
	param->unit = i;
	CFG_get(CFG_POE_EN+i, &param->enabled);
	if (param->enabled == 0)
	{
		SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "ppp%d was not enabled", unit);
		return -1;
	}
	
	strncpy(param->username, username, PPPOE_MAX_USRNAME_LEN);
	strncpy(param->password, password, PPPOE_MAX_PASSWD_LEN);
	param->username[PPPOE_MAX_USRNAME_LEN] = 0;
	param->password[PPPOE_MAX_PASSWD_LEN] = 0;

	CFG_get(CFG_POE_PASS+i, param->opass);

	CFG_get(CFG_POE_SVC+i, &param->service_name);
	CFG_get(CFG_POE_AC+i, &param->ac_name);
	CFG_get(CFG_POE_IDLET+i, &param->idle_time);
	CFG_get(CFG_POE_AUTO+i, &param->connect_mode);
	CFG_get(CFG_POE_MTU+i, &param->mtu);
	
	CFG_get(CFG_POE_ST+i, &param->start_time);
	CFG_get(CFG_POE_ET+i, &param->end_time);
	
	val = 0;
	CFG_get(CFG_POE_SIPE+i, &val);
	if (val)
	{
		CFG_get(CFG_POE_SIP, &param->my_ip);
		
		// I don't know why this flag was qualified
		CFG_get(CFG_DNS_SVR+1, &param->pri_dns);
		CFG_get(CFG_DNS_SVR+2, &param->snd_dns);
	}
	
	param->netmask = 0xffffff00;
	param->peer_ip = 0;
	
	// Get enabled and default route flag
	CFG_get(CFG_POE_DFLRT+i, &param->dflrt);
	CFG_get(CFG_POE_EN+i, &param->enabled);
	
	sprintf(param->ppp_name, "ppp%d", i);
	strcpy(param->eth_name, WAN_NAME);
	return 0;
}


/*--------------------------------------------------------------
* ROUTINE NAME - ng_pppoe_doConnect
*---------------------------------------------------------------
* FUNCTION:   
*
* INPUT:    None 
---------------------------------------------------------------*/
int	pppoe_do_connect
(
	int unit,
	int force_on
)
{
	PPPOE_INFO *pInfo = &pppoe_softc[unit];
	
	SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE, "ppp%d do connect", unit);
	
	//
	// Do parameter initialization
	//
	if (pppoe_init_param(unit, pInfo) != 0)
		return -1;
	
	pppoe_init_connect_mode(pInfo);

	// Start pppoe if necessary
	if ((pInfo->param.idle_time == 0 && pInfo->param.connect_mode == 0) || force_on == TRUE)
	{
		pppoe_DaemonStart(pInfo);
	}

	return 0;
}


int ng_pppoe_doConnect(u_char *pconnstate)
{
	int i, val = 0;
	
	dialup_checkp = NULL;

	pppoe_softc[0].pconnstate = pconnstate;

	CFG_get(CFG_POE_AUTO, &val);

	// Do per interface bringup
	for (i=0; i<PPPOE_PPP_NUM_SESSIONS/*CONFIG_PPP_NUM_SESSIONS*/; i++)
		pppoe_do_connect(i, FALSE);

	return 0;
}


static int pppoe_Ppp2Ether
(
	PPP_IF_VAR_T *pPppIf,
	struct mbuf *m0
)
{
	ng_pppoe_senddata((PPPOE_INFO *)pPppIf, m0);
	return OK;
}

#if 0
struct mbuf *
m_copypacket(m, how)
	struct mbuf *m;
	int how;
{
	struct mbuf *top, *n, *o, *tmp_m;

	if(!m)
		return 0;
    tmp_m = m;
	MGET(n, how, m->m_type);
	top = n;
	if (!n)
		return 0;

	M_COPY_PKTHDR(n, m);
	n->m_len = m->m_len;
	if (m->m_flags & M_EXT) {
		n->m_data = m->m_data;
		if(!m->m_ext.ext_ref)
			mclrefcnt[mtocl(m->m_ext.ext_buf)]++;
		else
			(*(m->m_ext.ext_ref))(m->m_ext.ext_buf,
						m->m_ext.ext_size);
		n->m_ext = m->m_ext;
		n->m_flags |= M_EXT;
	} else {
		bcopy(mtod(m, char *), mtod(n, char *), n->m_len);
	}

	m = m->m_next;
	while (m) {
		MGET(o, how, m->m_type);
		if (!o)
			goto nospace;

		n->m_next = o;
		n = n->m_next;

		n->m_len = m->m_len;
		if (m->m_flags & M_EXT) {
			n->m_data = m->m_data;
			if(!m->m_ext.ext_ref)
				mclrefcnt[mtocl(m->m_ext.ext_buf)]++;
			else
				(*(m->m_ext.ext_ref))(m->m_ext.ext_buf, m->m_ext.ext_size);	
			n->m_ext = m->m_ext;
			n->m_flags |= M_EXT;
		} else {
			bcopy(mtod(m, char *), mtod(n, char *), n->m_len);
		}

		m = m->m_next;
	}
	m = tmp_m;
	return top;
nospace:
	if(top)
		m_freem(top);
	//MCFail++;
	m = tmp_m;
	return 0;
}
#endif


#include <network.h>
#if 0
//extern struct bootp lan_bootp_data;
int IfLib_IpDetach(char *pIfName)
{
    int     sockfd = 0;
    int     status;
    struct  ifreq ifr;
    struct in_addr ip_addr,net_mask;
	
	
    if ( (sockfd = socket (AF_INET, SOCK_RAW, 0)) < 0) 
        return 0;
	
    /*
    * Deleting the interface address is required to correctly scrub the
    * routing table based on the current netmask.
    */
    bzero ( (char *)&ifr, sizeof (struct ifreq));

    strcpy(ifr.ifr_name, pIfName);
    ifr.ifr_addr.sa_len = sizeof (struct sockaddr_in);
    
    ( (struct sockaddr_in *) &ifr.ifr_addr)->sin_family = AF_INET;

    while (ioctl(sockfd, SIOCGIFADDR, (int) &ifr) == OK)
    {
        status = ioctl (sockfd, SIOCDIFADDR, (int)&ifr);
        if (status < 0) 
        {
            // Sometimes no address has been set, so ignore that error.
            if (errno != EADDRNOTAVAIL)
            {
                close (sockfd);
                return ERROR;
            }
        }
    }
    close (sockfd);
    
    memset((char *)&ip_addr,0,sizeof(struct in_addr));
    memset((char *)&net_mask,0,sizeof(struct in_addr));
	
    //delete the default gateway
    //route_del(pIfName,&ip_addr,&net_mask,&lan_bootp_data.bp_giaddr,0);
#if 0 /// need to implement
    // Flush all route entry associate with this interface
    rtlib_Purge(AF_INET, pIfName);

    // flush all ARP entry
    ArpLib_Flush(pIfName);
#endif    
    return OK;
}
#endif


/*--------------------------------------------------------------
* ROUTINE NAME - PPPOE_down
*---------------------------------------------------------------
* FUNCTION:
*
* INPUT:   None
---------------------------------------------------------------*/
void PPPOE_down(int clear_authlog)
{
	PPPOE_INFO *pInfo;
	PPP_IF_VAR_T *pPppIf;
	sessp sp;
	negp neg;
	int i, j;
	int ipmode;

	diag_printf("PPPOE_down!!\n");
	dialup_checkp = NULL;
#ifdef CONFIG_FW	
	need_dialup_flag = 0;
#endif
	for (i=0; i<PPPOE_PPP_NUM_SESSIONS; i++)
	{
		pInfo = &pppoe_softc[i];
		
		if (pInfo->param.enabled == 0)
			continue;
		
		if (pInfo->activated == 0)
			return;
		
		if (clear_authlog) {
			memset(pInfo->fail_log, 0, sizeof(pInfo->fail_log));
			pInfo->passalg_idx = -1;			
		}
			
		// Simon 2006/07/02, patched here, will be back to solve it
		CFG_get(CFG_WAN_IP_MODE, &ipmode);
		#ifdef	CONFIG_PPTP_PPPOE
		//Added by haitao,for pppoe-->pptp vpn case.
		int pptp_wanif=0;
		CFG_get(CFG_PTP_WANIF, &pptp_wanif);
		if(ipmode == PPTPMODE && pptp_wanif == 2)ipmode=PPPOEMODE;	
		#endif
		#ifdef	CONFIG_L2TP_OVER_PPPOE
		//l2tp over pppoe case.
		int l2tp_wanif=0;
		CFG_get(CFG_L2T_WANIF, &l2tp_wanif);
		if((ipmode == L2TPMODE) && (l2tp_wanif == 2))
		  ipmode = PPPOEMODE;	
		#endif
		if (ipmode != PPPOEMODE)
			untimeout((timeout_fun *)pppoe_timer_connect, pInfo);
		
		// Clear pppoe_ticker
		sp = &pInfo->sp;
		neg = &sp->neg;
		if (neg->m)
		{
			m_freem(neg->m);
			neg->m = NULL;
		}
		
		pPppIf = &pInfo->if_ppp;
		if (pPppIf->ifnet.if_reset)
		{
			pPppIf->hungup = 1;
			die(pPppIf, 1);
		}
		else
		{
			pInfo->activated = 0;
			sp->state = PPPOE_SNONE; //yfchou added
		}
		
		//clear the timer
		for(j=0; j<16; j++)
		{
			untimeout((timeout_fun *)pppoe_ticker, (void *)pInfo);
		}
	}
}


/*--------------------------------------------------------------
* ROUTINE NAME - pppoe_cmd
*---------------------------------------------------------------
* FUNCTION:
*
* INPUT:   None
---------------------------------------------------------------*/
static void usage(void)
{
	diag_printf("usage: pppoe [up|down]\n");
}


int pppoe_cmd
(
	int argc,
	char *argv[]
)
{
	int i;
	
	if (argc < 1)
	{
		usage();
		return 0;
	}
	
	if (strcmp(argv[0], "up") == 0)
	{
		if (chk_link)
		{
			netif_up(WAN_NAME);
			
			// Do per interface bringup
			for (i=0; i<PPPOE_PPP_NUM_SESSIONS/*CONFIG_PPP_NUM_SESSIONS*/; i++)
			{
				PPPOE_INFO *pInfo = &pppoe_softc[i];
				
				pInfo->force_flag = TIME_CONNECT_FORCE_ON;
				pppoe_do_connect(i, TRUE);
			}
		}
	}
	else if (strcmp(argv[0], "down") == 0)
	{
		int mode = MON_CMD_WAN_STOP;
		
		SysLog(LOG_USER|SYS_LOG_NOTICE|LOGM_PPPOE,"Shut down");
		
		for (i=0; i<PPPOE_PPP_NUM_SESSIONS/*CONFIG_PPP_NUM_SESSIONS*/; i++)
		{
			PPPOE_INFO *pInfo = &pppoe_softc[i];
			
			if (pInfo->param.enabled == 0)
				continue;
			
			pInfo->force_flag = TIME_CONNECT_FORCE_OFF;
			
#ifdef CONFIG_POE_KL_FORCE_DISCON
			if (pInfo->param.connect_mode == 1) //auto-connection
#else		
//			if (pInfo->param.connect_mode == 1 || pInfo->param.connect_mode == 0) //auto-connection and keep alive
#endif		
				mode = MON_CMD_WAN_UPDATE;
		}
		
		mon_snd_cmd(mode);
	}    	
	else
	{
		usage();
	}
	
	return 0;
}


int pppoe_dialup_checkp(void)
{
	int	i;
	PPPOE_INFO  *pInfo;
	int rc = 1;
	
	for (i=0; i<PPPOE_PPP_NUM_SESSIONS/*CONFIG_PPP_NUM_SESSIONS*/; i++)
	{
		pInfo = &pppoe_softc[i];
		if (pInfo->param.enabled == 0)
			continue;
		
		if (pInfo->activated == 0)
			pppoe_DaemonStart(pInfo);

#ifdef CONFIG_FW		
		need_dialup_flag++;
#endif
		// this interface is started
		rc = 0;
	}
	
	return rc;
}


static void pppoe_timer_connect
(
	PPPOE_INFO *pInfo
)
{
	//int i;
	//int flags;
	
	int now_time;
	struct tm local;
	
	untimeout((timeout_fun *)pppoe_timer_connect, pInfo);
	
	//i = pInfo->param.unit;
	
	if (pInfo->param.enabled == 1)
	{
		// Check bringup
		if (Is_ntp_ever_sync() == 0)
		{
			if (pInfo->activated == 0)
			{
				pppoe_DaemonStart(pInfo);
			}
			
			// Check every 3 seconds if the timer synchronized.
			timeout((timeout_fun *)pppoe_timer_connect, pInfo, 3*100);
		}
		else
		{
			// Check normal time
			now_time = ntp_getlocaltime(0);
			localtime_r(&now_time, &local);
			now_time = local.tm_hour*3600+local.tm_min*60+local.tm_sec;
			if ((now_time >= pInfo->param.start_time) && (now_time < pInfo->param.end_time))
			{
				if (pInfo->activated == 0)
				{
					// If force flag is not set, raise it
					if (pInfo->force_flag != TIME_CONNECT_FORCE_OFF)
					{
						//diag_printf("pppoe_timer_connect::Connect time on, force_flag = %d\n", pInfo->force_flag);
						pppoe_DaemonStart(pInfo);
					}
					else
					{
						//diag_printf("pppoe_timer_connect::Connect time on, buf forced off.\n");
					}
				}
				else
				{
					// Clear manual flag anyway
					if (pInfo->force_flag == TIME_CONNECT_FORCE_ON)
					{
						//diag_printf("pppoe_timer_connect::Connect time on, clear force flag.\n");
						pInfo->force_flag = TIME_CONNECT_FORCE_NONE;
					}
				}
			}
			else
			{
				// Should changed to down
				if (pInfo->if_ppp.ifnet.if_flags & IFF_UP)
				{
					// If force flag is not set, shut it down
					if (pInfo->force_flag != TIME_CONNECT_FORCE_ON)
					{
						//diag_printf("pppoe_timer_connect::Connect time off, force_flag = %d\n", pInfo->force_flag);
						mon_snd_cmd( MON_CMD_WAN_UPDATE );
					}
					else
					{
						//diag_printf("pppoe_timer_connect::Connect time off, buf forced on.\n");
					}
				}
				else
				{
					// Clear manual flag anyway
					if (pInfo->force_flag == TIME_CONNECT_FORCE_OFF)
					{
						//diag_printf("pppoe_timer_connect::Connect time off, clear force flag.\n");
						pInfo->force_flag = TIME_CONNECT_FORCE_NONE;
					}
				}
			}
			
			timeout((timeout_fun *)pppoe_timer_connect, pInfo, 30*100);
		}
	}
}


void pppoe_send_padt(u_int16_t sid, char *dhost, struct ifnet *ifp)
{
	struct pppoe_hdr *ph;
	struct pppoe_tag *tag;
	int	msglen = strlen(SIGNOFF);
	struct mbuf *m;
	
	struct sockaddr dst;
	struct ether_header *eh;
	//int status;
	
	SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPOE,"send PADT(%d)",ntohs(sid));
	
	/* generate a packet of that type */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
	{
		diag_printf("pppoe: Session out of mbufs\n");
	}
	else
	{
		m->m_data += sizeof(*eh);		// Save space for ethernet header
		m->m_len = sizeof(*ph);
		
		/*
		 * Initialize pppoe header
		 */
		ph = mtod(m, struct pppoe_hdr *);
		memset(ph, 0, sizeof(*ph));
		
		ph->ver = 0x1;
		ph->type = 0x1;
		ph->code = PADT_CODE;
		ph->sid = sid;
		ph->length = htons(sizeof(*tag) + msglen);
		
		/*
		 * Add a General error message and adjust
		 * sizes
		 */
		tag = ph->tag;
		tag->tag_type = PTT_GEN_ERR;
		tag->tag_len = htons((u_int16_t)msglen);
		strncpy(tag->tag_data, SIGNOFF, msglen);
		
		//
		// Prepare mbuf header
		//
		m->m_len += sizeof(*tag) + msglen;
		
		m->m_pkthdr.len = m->m_len;
		m->m_pkthdr.rcvif = NULL;
		
		// Build socket for ether output
		memset(&dst, 0, sizeof(dst));
		
		dst.sa_len = 16;
		dst.sa_family = AF_UNSPEC;
		
		eh = (struct ether_header *)dst.sa_data;
		memcpy(eh->ether_dhost, dhost, sizeof(eh->ether_dhost));
		eh->ether_type = ETHERTYPE_PPPOE_DISC;
		
		ether_output(ifp, m, &dst, (struct rtentry *)NULL);
		//if (status != OK) 
		//{
		//	m_free(m);
		//}
	}
}


void pppoe_cleanup(void)
{
	int i;
	
	for (i=0; i<PPPOE_PPP_NUM_SESSIONS/*CONFIG_PPP_NUM_SESSIONS*/; i++)
	{
		memset(&pppoe_softc[i], 0, sizeof(PPPOE_INFO));
	}
}


void pppoe_def_password(char *username, char *password)
{
//diag_printf("Original username=%s\nOriginal password=%s\n", username, password);
	if (onetime_username[0] != 0)
	{
		strcpy(username, onetime_username);
//		diag_printf("username has been changed = [%s]\n", username);
		onetime_username[0] = 0;
	}
	if (onetime_password[0] != 0)
	{
		strcpy(password, onetime_password);
//		diag_printf("password has been changed = [%s]\n", password);
		onetime_password[0] = 0;
	}
}


static u_int32_t pppoe_avail_pass_algmask(void)
{
	static unsigned long algmask = 0;
	int i, numalg;
	
	if (algmask == 0) {
		numalg = pppoe_avail_pass_alg();
		if (numalg > 32)
			numalg = 32;
		for(i=0; i<numalg; i++) {
			algmask |= (1 << i);
		}		
	}
	return algmask;
}


int set_pppoe_onetime_uname(char *source, int len)
{
	PPPOE_INFO *pInfo;

	/*  This only works when there's only one ppp connection in the system.  */
	if (source!=NULL)
	{
		memcpy(onetime_username, source, len);
		onetime_username[len] = 0;
		diag_printf("set onetime uname=%s\n", onetime_username);
		
		/*  Clear failed log  */
		pInfo = &pppoe_softc[0];
		memset(pInfo->fail_log, 0, sizeof(pInfo->fail_log));
	}
	return 0;
}


int set_pppoe_onetime_passwd(char *source, int len)
{
	PPPOE_INFO *pInfo;

	/*  This only works when there's only one ppp connection in the system.  */
	if (source!=NULL)
	{
		memcpy(onetime_password, source, len);
		onetime_password[len] = 0;
		diag_printf("set onetime password=%s\n", onetime_password);
		
		/*  Clear failed log  */
		pInfo = &pppoe_softc[0];
		memset(pInfo->fail_log, 0, sizeof(pInfo->fail_log));
	}

	return 0;
}


