
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

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <l2tp.h>
#include <l2tp_ppp.h>

#define PULL_UINT16(buf, cursor, val) \
do { \
    val = ((uint16_t) buf[cursor]) * 256 + (uint16_t) buf[cursor+1]; \
    cursor += 2; \
} while(0)

#define PUSH_UINT16(buf, cursor, val) \
do { \
    buf[cursor] = val / 256; \
    buf[cursor+1] = val & 0xFF; \
    cursor += 2; \
} while (0)

#define GET_AVP_LEN(x) ((((x)[0] & 3) * 256) + (x)[1])

static int dgram_add_random_vector_avp(l2tp_dgram *dgram);

static void dgram_do_hide(uint16_t type,
uint16_t len,
unsigned char *value,
unsigned char const *secret,
size_t secret_len,
unsigned char const *random,
size_t random_len,
unsigned char *output);

static unsigned char *dgram_do_unhide(uint16_t type,
										uint16_t *len,
										unsigned char *value,
										unsigned char const *secret,
										size_t secret_len,
										unsigned char const *random,
										size_t random_len,
										unsigned char *output);

/* Description of AVP's indexed by AVP type */
struct avp_descrip {
    char const *name;		/* AVP name */
    int can_hide;		/* Can AVP be hidden? */
    uint16_t min_len;		/* Minimum PAYLOAD length */
    uint16_t max_len;		/* Maximum PAYLOAD length (9999 = no limit) */
};

static struct avp_descrip avp_table[] = {
    /* Name                     can_hide min_len max_len */
    {"Message Type",            0,       2,      2}, /* 0 */
    {"Result Code",             0,       2,   9999}, /* 1 */
    {"Protocol Version",        0,       2,      2}, /* 2 */
    {"Framing Capabilities",    1,       4,      4}, /* 3 */
    {"Bearer Capabilities",     1,       4,      4}, /* 4 */
    {"Tie Breaker",             0,       8,      8}, /* 5 */
    {"Firmware Revision",       1,       2,      2}, /* 6 */
    {"Host Name",               0,       0,   9999}, /* 7 */
    {"Vendor Name",             1,       0,   9999}, /* 8 */
    {"Assigned Tunnel ID",      1,       2,      2}, /* 9 */
    {"Receive Window Size",     0,       2,      2}, /* 10 */
    {"Challenge",               1,       0,   9999}, /* 11 */
    {"Q.931 Cause Code",        0,       3,   9999}, /* 12 */
    {"Challenge Response",      1,      16,     16}, /* 13 */
    {"Assigned Session ID",     1,       2,      2}, /* 14 */
    {"Call Serial Number",      1,       4,      4}, /* 15 */
    {"Minimum BPS",             1,       4,      4}, /* 16 */
    {"Maximum BPS",             1,       4,      4}, /* 17 */
    {"Bearer Type",             1,       4,      4}, /* 18 */
    {"Framing Type",            1,       4,      4}, /* 19 */
    {"Unknown",                 0,       0,   9999}, /* 20 */
    {"Called Number",           1,       0,   9999}, /* 21 */
    {"Calling Number",          1,       0,   9999}, /* 22 */
    {"Sub-Address",             1,       0,   9999}, /* 23 */
    {"TX Connect Speed",        1,       4,      4}, /* 24 */
    {"Physical Channel ID",     1,       4,      4}, /* 25 */
    {"Intial Received Confreq", 1,       0,   9999}, /* 26 */
    {"Last Sent Confreq",       1,       0,   9999}, /* 27 */
    {"Last Received Confreq",   1,       0,   9999}, /* 28 */
    {"Proxy Authen Type",       1,       2,      2}, /* 29 */
    {"Proxy Authen Name",       1,       0,   9999}, /* 30 */
    {"Proxy Authen Challenge",  1,       0,   9999}, /* 31 */
    {"Proxy Authen ID",         1,       2,      2}, /* 32 */
    {"Proxy Authen Response",   1,       0,   9999}, /* 33 */
    {"Call Errors",             1,      26,     26}, /* 34 */
    {"ACCM",                    1,      10,     10}, /* 35 */
    {"Random Vector",           0,       0,   9999}, /* 36 */
    {"Private Group ID",        1,       0,   9999}, /* 37 */
    {"RX Connect Speed",        1,       4,      4}, /* 38 */
    {"Sequencing Required",     0,       0,      0}  /* 39 */
};

/* A free list of L2TP dgram strucures.  Allocation of L2TP dgrams
must be fast... */
static l2tp_dgram *dgram_free_list = NULL;

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static void describe_pulled_avp(uint16_t vendor,
								uint16_t type,
								uint16_t len,
								int hidden,
								int mandatory,
								void *val)
{
    unsigned char *buf = val;
    uint16_t i;
    int ascii = 1;
	
    fprintf(stderr, "Pulled avp: len=%d ", (int) len);
    if (vendor == VENDOR_IETF) {
		fprintf(stderr, "type='%s' ",avp_table[type].name);
    } else {
		fprintf(stderr, "type=%d/%d ", (int) vendor, (int) type);
    }
    fprintf(stderr, "M=%d H=%d ", mandatory ? 1 : 0, hidden ? 1 : 0);
	
    fprintf(stderr, "val='");
    for (i=0; i<len; i++) {
		if (buf[i] < 32 || buf[i] > 126) {
			ascii = 0;
			break;
		}
    }
    for (i=0; i<len; i++) {
		if (ascii) {
			fprintf(stderr, "%c", buf[i]);
		} else {
			fprintf(stderr, "%02X ", (unsigned int) buf[i]);
		}
    }
    fprintf(stderr, "'\n");
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int lf_ValidateAvp(uint16_t vendor,
							uint16_t type,
							uint16_t len,
							int mandatory)
{
    if (vendor != VENDOR_IETF) 
	{
		if (mandatory) 
		{
			l2tp_set_errmsg("Unknown mandatory AVP (vendor %d, type %d)",
			(int) vendor, (int) type);
			return 0;
		}
		/* I suppose... */
		return 1;
    }
	
    if (type > HIGHEST_AVP) 
	{
		if (mandatory) 
		{
			l2tp_set_errmsg("Unknown mandatory AVP of type %d",
			(int) type);
			return 0;
		}
		return 1;
    }
	
    return (len >= avp_table[type].min_len &&
	len <= avp_table[type].max_len);
}
/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
l2tp_dgram *lf_New(size_t len)
{
    l2tp_dgram *dgram;
	
    if (len > MAX_PACKET_LEN) 
	{
		l2tp_set_errmsg("dgram_new: cannot allocate datagram with payload length %d",
		(int) len);
		return NULL;
    }
	
    if (dgram_free_list) 
	{
		dgram = dgram_free_list;
		dgram_free_list = dgram->next;
    } 
	else 
	{
		dgram = malloc(sizeof(l2tp_dgram));
    }
    if (!dgram) 
	{
		l2tp_set_errmsg("dgram_new: Out of memory");
		return NULL;
    }
    dgram->bits = 0;
    dgram->version = 2;
    dgram->length = 0;
    dgram->tid = 0;
    dgram->sid = 0;
    dgram->Ns = 0;
    dgram->Nr = 0;
    dgram->off_size = 0;
    dgram->last_random = 0;
    dgram->payload_len = 0;
    dgram->cursor = 0;
	
    /* We maintain this in case it becomes dynamic later on */
    dgram->alloc_len = MAX_PACKET_LEN;
    dgram->next = NULL;
	
    return dgram;
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
l2tp_dgram *lf_NewCtl(uint16_t msg_type,
									uint16_t tid,
									uint16_t sid)
{
    l2tp_dgram *dgram = lf_New(MAX_PACKET_LEN);
    uint16_t u16;
	
    if (!dgram) return NULL;
	
    dgram->bits = TYPE_BIT | LENGTH_BIT | SEQUENCE_BIT;
    dgram->tid = tid;
    dgram->sid = sid;
    dgram->msg_type = msg_type;
    if (msg_type != MESSAGE_ZLB) {
		u16 = htons(msg_type);
		
		lf_AddAvp(dgram, NULL, MANDATORY,
		sizeof(u16), VENDOR_IETF, AVP_MESSAGE_TYPE, &u16);
    }
    return dgram;
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void lf_Free(l2tp_dgram *dgram)
{
    if (!dgram) return;
		free(dgram);
}
/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
//for saving two memory copy.
l2tp_dgram *lf_TakeFromBSD_BoostMode(struct sockaddr_in *from)
	{
		/* EXTRA_HEADER_ROOM bytes for other headers like PPPoE, etc. */	
		struct mbuf *m = NULL ;
		int s;
	
		socklen_t len = sizeof(struct sockaddr_in);
		int cursor;
		l2tp_dgram *dgram;
		unsigned char *buf;
		unsigned char *payload;
		unsigned char *tidptr;
		uint16_t tid, sid;
		static uint16_t cache_tid;
		static uint16_t cache_sid;
		l2tp_tunnel *tunnel;
		//l2tp_session *ses = NULL;
		static l2tp_session *ses;
		int mandatory, hidden, err;
		uint16_t vendor, type;
		unsigned char *msg;
		int r;
		struct timeval tv = {0, 200000};
		fd_set readfds;
		fd_set setted_readfd;
		
		/* Limit iterations before bailing back to select look.  Otherwise,
		we have a nice DoS possibility */
		int iters = 5;
		
		uint16_t off;
		int framelen;
		static struct l2tp_if *ifp;

		FD_ZERO(&readfds);
		FD_SET(Sock, &readfds);
		setted_readfd = readfds;
		
		while(1) 
		{
			if (--iters <= 0) 
			{
			  return NULL;
			}
			framelen = -1;
			l2tp_ScanCmd();//test this
	
			r = select(Sock+1, &setted_readfd, NULL, NULL, &tv);
			if(r<=0)
			  continue;

			r = l2tp_readFrames(Sock ,&m, (struct sockaddr *) from, &len, MAX_PACKET_LEN-EXTRA_HEADER_ROOM);
			if(r == 0)
			  continue; 		
			else if (r < 0) 
			{	
			  return NULL;
			}
			if (m == NULL)
				{
				diag_printf("%s:%d m = NULL\n",__FUNCTION__,__LINE__);
				return NULL;
				}
			
			/* Check version; drop frame if not L2TP (ver = 2) */
			buf = mtod(m, unsigned char *);
			//if ((buf[1] & VERSION_MASK) != 2) 
			//	continue;
			
			/* Not a data frame -- break out of loop and handle control frame */
			if (buf[0] & TYPE_BIT) break;
			/* If it's a data frame, we need to be FAST, so do not allocate
			a dgram structure, etc; just call into handler */
			payload = buf + 6;
			tidptr = buf + 2;
			if (buf[0] & LENGTH_BIT) 
			{
				framelen = (((uint16_t) buf[2]) << 8) + buf[3];
				payload += 2;
				tidptr += 2;			
			}
			if (buf[0] & SEQUENCE_BIT) 
			{
				payload += 4;
			}
			if (buf[0] & OFFSET_BIT) 
			{
				payload += 2;
				off = (((uint16_t) *(payload-2)) << 8) + *(payload-1);
				payload += off;
			}
			off = (payload - buf);
			if (framelen == -1) 
			{
				framelen = r - off;
			} 
			else 
			{
				if (framelen < off || framelen > r) 
					continue;
				/* Only count payload length */
				framelen -= off;
			}
			
			/* Forget the 0xFF, 0x03 HDLC markers */
			//payload += 2;
			//framelen -= 2;
			
			if (framelen < 0) 
				continue;
			/* Get tunnel, session ID */
			tid = (((uint16_t) tidptr[0]) << 8) + (uint16_t) tidptr[1];
			sid = (((uint16_t) tidptr[2]) << 8) + (uint16_t) tidptr[3];

			/* Only do hash lookup if it's not cached */
			/* TODO: Is this optimization really worthwhile? */
			if (!ses || tid != cache_tid || sid != cache_sid) 
			{
				ses = NULL;
				tunnel = lt_FindByMyId(tid);
				if (!tunnel) 
				{
					l2tp_set_errmsg("Unknown tunnel %d", (int) tid);
					continue;
				}
				if (tunnel->state != TUNNEL_ESTABLISHED) 
				{
					/* Drop frame if tunnel in wrong state */
					continue;
				}
				/* TODO: Verify source address */
				ses = lt_FindSession(tunnel, sid);
				if (!ses) 
				{
					l2tp_set_errmsg("Unknown session %d in tunnel %d",
					(int) sid, (int) tid);
					continue;
				}
				if (ses->state != SESSION_ESTABLISHED) 
				{
					/* Drop frame if session in wrong state */
					continue;
				}
				
				cache_tid = tid;
				cache_sid = sid;
			    ifp = (struct l2tp_if *)(((l2tp_tunnel *)ses)->es->pInfo);
			}
			
			//s = splimp();
			m->m_data = payload;
			m->m_len = framelen;
			m->m_flags |= M_PKTHDR;
			m->m_pkthdr.len = framelen;
			m->m_pkthdr.rcvif = &ifp->if_ppp.ifnet;
			m->m_next = 0;
			//splx(s);
			
			ppppktin(&ifp->if_ppp, m);
		}
		
		   
		/* A bit of slop on dgram size */
		dgram = lf_New(r);
		if (!dgram) 
		{
		  if(m)
		    m_freem(m);
		  return NULL;
		}
		dgram->bits = buf[0];
		dgram->version = buf[1];
		cursor = 2;
		
		if (dgram->bits & LENGTH_BIT) 
		{
			PULL_UINT16(buf, cursor, dgram->length);
			if (dgram->length > r) 
			{
				l2tp_set_errmsg("Invalid length field %d greater than received datagram size %d", (int) dgram->length, r);
				lf_Free(dgram);	
				if(m)
		          m_freem(m);				
				return NULL;
			}
		} else {
			dgram->length = r;
		}
		
		PULL_UINT16(buf, cursor, dgram->tid);
		PULL_UINT16(buf, cursor, dgram->sid);
		
		if (dgram->bits & SEQUENCE_BIT) 
		{
			PULL_UINT16(buf, cursor, dgram->Ns);
			PULL_UINT16(buf, cursor, dgram->Nr);
		} 
		else 
		{
			dgram->Ns = 0;
			dgram->Nr = 0;
		}
		
		if (dgram->bits & OFFSET_BIT) 
		{
			PULL_UINT16(buf, cursor, dgram->off_size);
		} 
		else 
		{
			dgram->off_size = 0;
		}
		if (cursor > dgram->length) 
		{
			l2tp_set_errmsg("Invalid length of datagram %d", (int) dgram->length);
			lf_Free(dgram);
			if(m)
		      m_freem(m);
			return NULL;
		}
		
		dgram->payload_len = dgram->length - cursor;
		memcpy(dgram->data, buf+cursor, dgram->payload_len);
				
		/* Pull off the message type */
		if (dgram->bits & OFFSET_BIT) 
		{
			l2tp_set_errmsg("Invalid control frame: O bit is set");
			lf_Free(dgram);
			if(m)
		      m_freem(m);
			return NULL;
		}
		
		/* Handle ZLB */
		if (dgram->payload_len == 0) 
		{
			dgram->msg_type = MESSAGE_ZLB;
		} 
		else 
		{
			uint16_t avp_len;
			msg = lf_PullAvp(dgram, NULL, &mandatory, &hidden,
			&avp_len, &vendor, &type, &err);
			if (!msg) 
			{	
				lf_Free(dgram);
				if(m)
		          m_freem(m);
				return NULL;
			}
			if (type != AVP_MESSAGE_TYPE || vendor != VENDOR_IETF) 
			{
				l2tp_set_errmsg("Invalid control message: First AVP must be message type");
				lf_Free(dgram);
				if(m)
		          m_freem(m);
				return NULL;
			}
			if (avp_len != 2) 
			{
				l2tp_set_errmsg("Invalid length %d for message-type AVP", (int) avp_len+6);
				lf_Free(dgram);
				if(m)
		          m_freem(m);
				return NULL;
			}
			if (!mandatory) 
			{
				l2tp_set_errmsg("Invalid control message: M bit not set on message-type AVP");
				lf_Free(dgram);
				if(m)
		          m_freem(m);
				return NULL;
			}
			if (hidden) 
			{
				l2tp_set_errmsg("Invalid control message: H bit set on message-type AVP");
				lf_Free(dgram);
				if(m)
		          m_freem(m);
				return NULL;
			}
			
			dgram->msg_type = ((unsigned int) msg[0]) * 256 +
			(unsigned int) msg[1];
		}

		if(m)
		  m_freem(m);
		DBG(ld_Debug(DBG_XMIT_RCV,
					"dgram_take_from_wire() -> %s\n",
					ld_DescribeDgram(dgram)));
		return dgram;
	}

l2tp_dgram *lf_TakeFromBSD(struct sockaddr_in *from)
{
    /* EXTRA_HEADER_ROOM bytes for other headers like PPPoE, etc. */
	
    static unsigned char inbuf[MAX_PACKET_LEN+EXTRA_HEADER_ROOM];
    unsigned char *buf;
    socklen_t len = sizeof(struct sockaddr_in);
    int cursor;
    l2tp_dgram *dgram;
    unsigned char *payload;
    unsigned char *tidptr;
    uint16_t tid, sid;
    uint16_t cache_tid, cache_sid;
    l2tp_tunnel *tunnel;
    l2tp_session *ses = NULL;
    int mandatory, hidden, err;
    uint16_t vendor, type;
    unsigned char *msg;
    int r;
    struct timeval tv = {0, 200000};
    int maxfd;
    fd_set readfds;
	
    /* Limit iterations before bailing back to select look.  Otherwise,
	we have a nice DoS possibility */
    int iters = 5;
	
    uint16_t off;
    int framelen;
	
    /* Active part of buffer */
    buf = inbuf + EXTRA_HEADER_ROOM;
	
	//yfchou add for nonblocking
	//setsockopt(Sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    while(1) 
	{
		if (--iters <= 0) return NULL;
		framelen = -1;
		l2tp_ScanCmd();
		FD_ZERO(&readfds);
		FD_SET(Sock, &readfds);
		maxfd = Sock;
		r = select(maxfd+1, &readfds, NULL, NULL, &tv);
		if(r<=0)
			continue;
		r = recvfrom(Sock, buf, MAX_PACKET_LEN, 0,
		(struct sockaddr *) from, &len);
		if (r <= 0) 
		{
			return NULL;
		}
		/* Check version; drop frame if not L2TP (ver = 2) */
		if ((buf[1] & VERSION_MASK) != 2) continue;
		
		/* Not a data frame -- break out of loop and handle control frame */
		if (buf[0] & TYPE_BIT) break;
		
		/* If it's a data frame, we need to be FAST, so do not allocate
		a dgram structure, etc; just call into handler */
		payload = buf + 6;
		tidptr = buf + 2;
		if (buf[0] & LENGTH_BIT) 
		{
			framelen = (((uint16_t) buf[2]) << 8) + buf[3];
			payload += 2;
			tidptr += 2;
		}
		if (buf[0] & SEQUENCE_BIT) 
		{
			payload += 4;
		}
		if (buf[0] & OFFSET_BIT) 
		{
			payload += 2;
			off = (((uint16_t) *(payload-2)) << 8) + *(payload-1);
			payload += off;
		}
		off = (payload - buf);
		if (framelen == -1) 
		{
			framelen = r - off;
		} 
		else 
		{
			if (framelen < off || framelen > r) 
			{
				continue;
			}
			/* Only count payload length */
			framelen -= off;
		}
		
		/* Forget the 0xFF, 0x03 HDLC markers */
		//payload += 2;
		//framelen -= 2;
		
		if (framelen < 0) 
		{
			continue;
		}
		/* Get tunnel, session ID */
		tid = (((uint16_t) tidptr[0]) << 8) + (uint16_t) tidptr[1];
		sid = (((uint16_t) tidptr[2]) << 8) + (uint16_t) tidptr[3];
		
		/* Only do hash lookup if it's not cached */
		/* TODO: Is this optimization really worthwhile? */
		if (!ses || tid != cache_tid || sid != cache_sid) 
		{
			ses = NULL;
			tunnel = lt_FindByMyId(tid);
			if (!tunnel) 
			{
				l2tp_set_errmsg("Unknown tunnel %d", (int) tid);
				continue;
			}
			if (tunnel->state != TUNNEL_ESTABLISHED) 
			{
				/* Drop frame if tunnel in wrong state */
				continue;
			}
			/* TODO: Verify source address */
			ses = lt_FindSession(tunnel, sid);
			if (!ses) 
			{
				l2tp_set_errmsg("Unknown session %d in tunnel %d",
				(int) sid, (int) tid);
				continue;
			}
			cache_tid = tid;
			cache_sid = sid;
			if (ses->state != SESSION_ESTABLISHED) 
			{
				/* Drop frame if session in wrong state */
				continue;
			}
		}
		l2_HandleFrame(ses, payload, framelen);
    }
	
	
    /* A bit of slop on dgram size */
    dgram = lf_New(r);
    if (!dgram) return NULL;
	
    dgram->bits = buf[0];
    dgram->version = buf[1];
    cursor = 2;
	
    if (dgram->bits & LENGTH_BIT) 
	{
		PULL_UINT16(buf, cursor, dgram->length);
		if (dgram->length > r) 
		{
			l2tp_set_errmsg("Invalid length field %d greater than received datagram size %d", (int) dgram->length, r);
			lf_Free(dgram);
			return NULL;
		}
    } else {
		dgram->length = r;
    }
	
    PULL_UINT16(buf, cursor, dgram->tid);
    PULL_UINT16(buf, cursor, dgram->sid);
    if (dgram->bits & SEQUENCE_BIT) 
	{
		PULL_UINT16(buf, cursor, dgram->Ns);
		PULL_UINT16(buf, cursor, dgram->Nr);
    } 
	else 
	{
		dgram->Ns = 0;
		dgram->Nr = 0;
    }
	
    if (dgram->bits & OFFSET_BIT) 
	{
		PULL_UINT16(buf, cursor, dgram->off_size);
    } 
	else 
	{
		dgram->off_size = 0;
    }
    if (cursor > dgram->length) 
	{
		l2tp_set_errmsg("Invalid length of datagram %d", (int) dgram->length);
		lf_Free(dgram);
		return NULL;
    }
	
    dgram->payload_len = dgram->length - cursor;
    memcpy(dgram->data, buf+cursor, dgram->payload_len);
	
    /* Pull off the message type */
    if (dgram->bits & OFFSET_BIT) 
	{
		l2tp_set_errmsg("Invalid control frame: O bit is set");
		lf_Free(dgram);
		return NULL;
    }
	
    /* Handle ZLB */
    if (dgram->payload_len == 0) 
	{
		dgram->msg_type = MESSAGE_ZLB;
    } 
	else 
	{
		uint16_t avp_len;
		msg = lf_PullAvp(dgram, NULL, &mandatory, &hidden,
		&avp_len, &vendor, &type, &err);
		if (!msg) 
		{
			
			lf_Free(dgram);
			return NULL;
		}
		if (type != AVP_MESSAGE_TYPE || vendor != VENDOR_IETF) 
		{
			l2tp_set_errmsg("Invalid control message: First AVP must be message type");
			lf_Free(dgram);
			return NULL;
		}
		if (avp_len != 2) 
		{
			l2tp_set_errmsg("Invalid length %d for message-type AVP", (int) avp_len+6);
			lf_Free(dgram);
			return NULL;
		}
		if (!mandatory) 
		{
			l2tp_set_errmsg("Invalid control message: M bit not set on message-type AVP");
			lf_Free(dgram);
			return NULL;
		}
		if (hidden) 
		{
			l2tp_set_errmsg("Invalid control message: H bit set on message-type AVP");
			lf_Free(dgram);
			return NULL;
		}
		
		dgram->msg_type = ((unsigned int) msg[0]) * 256 +
	    (unsigned int) msg[1];
    }
    DBG(ld_Debug(DBG_XMIT_RCV,
				"dgram_take_from_wire() -> %s\n",
				ld_DescribeDgram(dgram)));
    return dgram;
}


/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int lf_SendToBSD(l2tp_dgram const *dgram, struct sockaddr_in const *to)
{
    static unsigned char buf[MAX_PACKET_LEN+128];
    socklen_t len = sizeof(struct sockaddr_in);
    int cursor = 2;
    size_t total_len;
    unsigned char *len_ptr = NULL;
    DBG(ld_Debug(DBG_XMIT_RCV,
	"dgram_send_to_wire() -> %s\n",
	ld_DescribeDgram(dgram)));
    buf[0] = dgram->bits;
    buf[1] = dgram->version;
    if (dgram->bits & LENGTH_BIT) 
	{
		len_ptr = buf + cursor;
		PUSH_UINT16(buf, cursor, dgram->length);
    }
    PUSH_UINT16(buf, cursor, dgram->tid);
    PUSH_UINT16(buf, cursor, dgram->sid);
    if (dgram->bits & SEQUENCE_BIT) 
	{
		PUSH_UINT16(buf, cursor, dgram->Ns);
		PUSH_UINT16(buf, cursor, dgram->Nr);
    }
    if (dgram->bits & OFFSET_BIT) 
	{
		PUSH_UINT16(buf, cursor, dgram->off_size);
    }
    total_len = cursor + dgram->payload_len;
    if (dgram->bits & LENGTH_BIT) 
	{
		*len_ptr++ = total_len / 256;
		*len_ptr = total_len & 255;
    }
    memcpy(buf+cursor, dgram->data, dgram->payload_len);
    return sendto(Sock, buf, total_len, 0, (struct sockaddr const *) to, len);
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int lf_AddAvp(l2tp_dgram *dgram,
						l2tp_tunnel *tunnel,
						int mandatory,
						uint16_t len,
						uint16_t vendor,
						uint16_t type,
						void *val)
{
    static unsigned char hidden_buffer[1024];
    unsigned char const *random_data;
    size_t random_len;
    int hidden = 0;
	
    /* max len is 1023 - 6 */
    if (len > 1023 - 6) 
	{
		l2tp_set_errmsg("AVP length of %d too long", (int) len);
		return -1;
    }
	
    /* Do hiding where possible */
    if (tunnel                   &&
		tunnel->peer->hide_avps  &&
	vendor == VENDOR_IETF    &&
	tunnel->peer->secret_len &&
	len <= 1021 - 6          &&
	avp_table[type].can_hide) 
	{
		if (!dgram->last_random) 
		{
			/* Add a random vector */
			if (dgram_add_random_vector_avp(dgram) < 0) return -1;
		}
		/* Get pointer to random data */
		random_data = dgram->data + dgram->last_random + 6;
		random_len  = GET_AVP_LEN(dgram->data + dgram->last_random) - 6;
		
		/* Hide the value into the buffer */
		dgram_do_hide(type, len, val, (unsigned char *) tunnel->peer->secret,
		tunnel->peer->secret_len,
		random_data, random_len, hidden_buffer);
		
		/* Length is increased by 2 */
		len += 2;
		val = hidden_buffer;
		hidden = 1;
    }
	
    /* Adjust from payload len to actual len */
    len += 6;
	
    /* Does datagram have room? */
    if (dgram->cursor + len > dgram->alloc_len) 
	{
		l2tp_set_errmsg("No room for AVP of length %d", (int) len);
		return -1;
    }
	
    dgram->data[dgram->cursor] = 0;
    if (mandatory) {
		dgram->data[dgram->cursor] |= AVP_MANDATORY_BIT;
    }
    if (hidden) 
	{
		dgram->data[dgram->cursor] |= AVP_HIDDEN_BIT;
    }
	
    dgram->data[dgram->cursor] |= (len >> 8);
    dgram->data[dgram->cursor+1] = (len & 0xFF);
    dgram->data[dgram->cursor+2] = (vendor >> 8);
    dgram->data[dgram->cursor+3] = (vendor & 0xFF);
    dgram->data[dgram->cursor+4] = (type >> 8);
    dgram->data[dgram->cursor+5] = (type & 0xFF);
	
    if (len > 6) 
	{
		memcpy(dgram->data + dgram->cursor + 6, val, len-6);
    }
    if (type == AVP_RANDOM_VECTOR) 
	{
		/* Remember location of random vector */
		dgram->last_random = dgram->cursor;
    }
    dgram->cursor += len;
    dgram->payload_len = dgram->cursor;
    return 0;
}


/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
unsigned char *lf_PullAvp(l2tp_dgram *dgram,
									l2tp_tunnel *tunnel,
									int *mandatory,
									int *hidden,
									uint16_t *len,
									uint16_t *vendor,
									uint16_t *type,
									int *err)
{
    static unsigned char val[1024];
    unsigned char *ans;
    unsigned char *random_data;
    size_t random_len;
    int local_hidden;
    int local_mandatory;
	
    *err = 1;
    if (dgram->cursor >= dgram->payload_len) 
	{
		/* EOF */
		*err = 0;
		return NULL;
    }
	
    /* Get bits */
    if (!mandatory) 
	{
		mandatory = &local_mandatory;
    }
    *mandatory = dgram->data[dgram->cursor] & AVP_MANDATORY_BIT;
	
    if (!hidden) 
	{
		hidden = &local_hidden;
    }
    *hidden = dgram->data[dgram->cursor] & AVP_HIDDEN_BIT;
    if (dgram->data[dgram->cursor] & AVP_RESERVED_BITS) 
	{
		l2tp_set_errmsg("AVP with reserved bits set to non-zero");
		return NULL;
    }
	
    /* Get len */
    *len = ((unsigned int) (dgram->data[dgram->cursor] & 3)) * 256 +
	dgram->data[dgram->cursor+1];
    if (*len < 6) 
	{
		l2tp_set_errmsg("Received AVP of length %d (too short)", (int) *len);
		return NULL;
    }
	
    if (dgram->cursor + *len > dgram->payload_len) 
	{
		l2tp_set_errmsg("Received AVP of length %d too long for rest of datagram",
		(int) *len);
		return NULL;
    }
    dgram->cursor += 2;
	
    PULL_UINT16(dgram->data, dgram->cursor, *vendor);
    PULL_UINT16(dgram->data, dgram->cursor, *type);
	
    /* If we see a random vector, remember it */
    if (*vendor == VENDOR_IETF && *type == AVP_RANDOM_VECTOR) 
	{
		if (*hidden) 
		{
			l2tp_set_errmsg("Invalid random vector AVP has H bit set");
			return NULL;
		}
		dgram->last_random = dgram->cursor - 6;
    }
	
    ans = dgram->data + dgram->cursor;
    dgram->cursor += *len - 6;
    if (*hidden) 
	{
		if (!tunnel) 
		{
			l2tp_set_errmsg("AVP cannot be hidden");
			return NULL;
		}
		if (*len < 8) 
		{
			l2tp_set_errmsg("Received hidden AVP of length %d (too short)",
			(int) *len);
			return NULL;
		}
		
		if (!tunnel->peer) 
		{
			l2tp_set_errmsg("No peer??");
			return NULL;
		}
		if (!tunnel->peer->secret_len) 
		{
			l2tp_set_errmsg("No shared secret to unhide AVP");
			return NULL;
		}
		if (!dgram->last_random) 
		{
			l2tp_set_errmsg("Cannot unhide AVP unless Random Vector received first");
			return NULL;
		}
		
		if (*type <= HIGHEST_AVP) 
		{
			if (!avp_table[*type].can_hide) 
			{
				l2tp_set_errmsg("AVP of type %s cannot be hidden, but H bit set",
				avp_table[*type].name);
				return NULL;
			}
		}
		
		/* Get pointer to random data */
		random_data = dgram->data + dgram->last_random + 6;
		random_len  = GET_AVP_LEN(dgram->data + dgram->last_random) - 6;
		
		/* Unhide value */
		ans = dgram_do_unhide(*type, len, ans,
		(unsigned char *) tunnel->peer->secret,
		tunnel->peer->secret_len,
		random_data, random_len, val);
		if (!ans) return NULL;
    }
	
    /* Set len to length of value only */
    *len -= 6;
	
    *err = 0;
    /* DBG(describe_pulled_avp(*vendor, *type, *len, *hidden, *mandatory, ans)); */
    return ans;
	
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static void dgram_do_hide(uint16_t type,
							uint16_t len,
							unsigned char *value,
							unsigned char const *secret,
							size_t secret_len,
							unsigned char const *random_data,
							size_t random_len,
							unsigned char *output)
{
    MD5_CTX ctx;
    unsigned char t[2];
    unsigned char digest[16];
    size_t done;
    size_t todo = len;
	
    /* Put type in network byte order */
    t[0] = type / 256;
    t[1] = type & 255;
	
    /* Compute initial pad */
    MD5Init(&ctx);
    MD5Update(&ctx, t, 2);
    MD5Update(&ctx, secret, secret_len);
    MD5Update(&ctx, random_data, random_len);
    MD5Final(digest, &ctx);
	
    /* Insert length */
    output[0] = digest[0] ^ (len / 256);
    output[1] = digest[1] ^ (len & 255);
    output += 2;
    done = 2;
	
    /* Insert value */
    while(todo) 
	{
		*output = digest[done] ^ *value;
		++output;
		++value;
		--todo;
		++done;
		if (done == 16 && todo) 
		{
			/* Compute new digest */
			done = 0;
			MD5Init(&ctx);
			MD5Update(&ctx, secret, secret_len);
			MD5Update(&ctx, output-16, 16);
			MD5Final(digest, &ctx);
		}
    }
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static unsigned char * dgram_do_unhide(uint16_t type,
										uint16_t *len,
										unsigned char *value,
										unsigned char const *secret,
										size_t secret_len,
										unsigned char const *random_data,
										size_t random_len,
										unsigned char *output)
{
    MD5_CTX ctx;
    unsigned char t[2];
    unsigned char digest[16];
    uint16_t tmplen;
    unsigned char *orig_output = output;
    size_t done, todo;
	
    /* Put type in network byte order */
    t[0] = type / 256;
    t[1] = type & 255;
	
    /* Compute initial pad */
    MD5Init(&ctx);
    MD5Update(&ctx, t, 2);
    MD5Update(&ctx, secret, secret_len);
    MD5Update(&ctx, random_data, random_len);
    MD5Final(digest, &ctx);
	
    /* Get hidden length */
    tmplen =
	((uint16_t) (digest[0] ^ value[0])) * 256 +
	(uint16_t) (digest[1] ^ value[1]);
	
    value += 2;
	
    if (tmplen > *len-8) 
	{
		l2tp_set_errmsg("Hidden length %d too long in AVP of length %d",
		(int) tmplen, (int) *len);
		return NULL;
    }
	
    /* Adjust len.  Add 6 to compensate for pseudo-header */
    *len = tmplen + 6;
	
    /* Decrypt remainder */
    done = 2;
    todo = tmplen;
    while(todo) 
	{
		*output = digest[done] ^ *value;
		++output;
		++value;
		--todo;
		++done;
		if (done == 16 && todo) 
		{
			/* Compute new digest */
			done = 0;
			MD5Init(&ctx);
			MD5Update(&ctx, secret, secret_len);
			MD5Update(&ctx, value-16, 16);
			MD5Final(digest, &ctx);
		}
    }
    return orig_output;
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
unsigned char *lf_SearchAvp(l2tp_dgram *dgram,
										l2tp_tunnel *tunnel,
										int *mandatory,
										int *hidden,
										uint16_t *len,
										uint16_t vendor,
										uint16_t type)
{
    uint16_t svend, stype;
    int err;
    unsigned char *val;
    size_t cursor = dgram->cursor;
    size_t last_random = dgram->last_random;
    while(1) 
	{
		val = lf_PullAvp(dgram, tunnel, mandatory, hidden, len,
		&svend, &stype, &err);
		if (!val) 
		{
			if (!err) 
			{
				l2tp_set_errmsg("AVP of vendor/type (%d, %d) not found",
				(int) vendor, (int) type);
			}
			dgram->cursor = cursor;
			dgram->last_random = last_random;
			return NULL;
		}
		if (vendor == svend && type == stype) break;
    }
    dgram->cursor = cursor;
    dgram->last_random = last_random;
    return val;
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
//save two memory copy
int lf_SendPppFrameDirectly(l2tp_session *ses,
								struct mbuf *m0)
{
    int r;
    fd_set writefds;
    int maxfd;
    unsigned char *real_buf;
    l2tp_tunnel *tunnel = ses->tunnel;
    struct timeval tm = {0, 20000};

	unsigned char *buf = 0;
	struct mbuf *m = m0;
	u_char * cp;
	int s;
	int len = 0;
    /* Drop frame if tunnel and/or session in wrong state */
    if (!tunnel ||
		tunnel->state != TUNNEL_ESTABLISHED ||
	ses->state != SESSION_ESTABLISHED) 
	{
	    diag_printf("*********lf_SendPppFrameDirectly, ses->state != SESSION_ESTABLISHED***************\n");
	    m_freem(m);
		return ENOTCONN;
    }

    M_PREPEND(m,6,M_DONTWAIT);
    if(m == NULL)
    {
      errno = ENOBUFS;
	  m_freem(m);
	  return ENOBUFS;
    }
	cp = mtod(m, u_char *);
	real_buf = cp;
    *cp  = 0; cp ++;
    *cp  = 2; cp ++;
    *cp  = (tunnel->assigned_id >> 8); cp ++;
    *cp  = tunnel->assigned_id & 0xFF; cp ++;
    *cp  = ses->assigned_id >> 8;      cp ++;
    *cp  = ses->assigned_id & 0xFF; 
	
    FD_ZERO(&writefds);
    FD_SET(Sock, &writefds);
    maxfd = Sock;
    r = select(maxfd+1, NULL, &writefds, NULL, &tm);
    if (r < 0) 
	{
		if (errno == EINTR) 
		{
		  m_freem(m0);
		  return r;
		}
	}
	
	r = l2tp_writeFrames(Sock, m, 0, (struct sockaddr_in *) &tunnel->peer_addr, sizeof(struct sockaddr_in));
    return r;
}

int lf_SendPppFrame(l2tp_session *ses,
								unsigned char const *buf,
								int len)
{
    int r;
    fd_set writefds;
    int maxfd;
    //unsigned char *real_buf = (unsigned char *) buf - 8;
    unsigned char *real_buf = (unsigned char *) buf - 6;
    l2tp_tunnel *tunnel = ses->tunnel;
    struct timeval tm = {0, 20000};
	
    /* Drop frame if tunnel and/or session in wrong state */
    if (!tunnel ||
		tunnel->state != TUNNEL_ESTABLISHED ||
	ses->state != SESSION_ESTABLISHED) 
	{
		return 0;
    }
    /* If there is a pending ack on the tunnel, cancel and ack now */
	
    real_buf[0] = 0;
    real_buf[1] = 2;
    real_buf[2] = (tunnel->assigned_id >> 8);
    real_buf[3] = tunnel->assigned_id & 0xFF;
    real_buf[4] = ses->assigned_id >> 8;
    real_buf[5] = ses->assigned_id & 0xFF;
    //real_buf[6] = 0xFF;		/* HDLC address */
    //real_buf[7] = 0x03;		/* HDLC control */
    FD_ZERO(&writefds);
    FD_SET(Sock, &writefds);
    maxfd = Sock;
    r = select(maxfd+1, NULL, &writefds, NULL, &tm);
    if (r < 0) 
	{
		if (errno == EINTR) return;
	}
	r = sendto(Sock, real_buf, len+6, 0,
    //r = sendto(Sock, real_buf, len+8, 0,
	(struct sockaddr const *) &tunnel->peer_addr,
	sizeof(struct sockaddr_in));
    return r;
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static int dgram_add_random_vector_avp(l2tp_dgram *dgram)
{
    unsigned char buf[16];
	
    l2tp_random_fill(buf, sizeof(buf));
	
    return lf_AddAvp(dgram, NULL, MANDATORY, sizeof(buf),
	VENDOR_IETF, AVP_RANDOM_VECTOR, buf);
}
