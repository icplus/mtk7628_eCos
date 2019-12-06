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


#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <l2tp.h>
#include <l2tp_ppp.h>

#ifdef CONFIG_L2TP_OVER_PPPOE
#include <cfg_def.h>
#include <cfg_net.h>
#define PRIMARY_WANIP_NUM 2
extern struct ip_set primary_wan_ip_set[PRIMARY_WANIP_NUM];
#endif

extern l2tp_peer l2tp_peercfg;

static uint16_t lt_MakeId(void);
static l2tp_tunnel *lt_Init(L2tpSelector *es);
static void lt_Release(l2tp_tunnel *tunnel);
static int lt_SendSCCRQ(l2tp_tunnel *tunnel);
static void lt_HandleSCCRQ(l2tp_dgram *dgram,
					 L2tpSelector *es,
					 struct sockaddr_in *from);
static void lt_HandleSCCCN(l2tp_tunnel *tunnel,
				l2tp_dgram *dgram);
static void lt_ProcessReceivedDatagram(l2tp_tunnel *tunnel,
					     l2tp_dgram *dgram);
static void lt_ScheduleACK(l2tp_tunnel *tunnel);
static void lt_DoACK(L2tpSelector *es, int fd, unsigned int flags,
			  void *data);
static void lt_FinallyCleanup(L2tpSelector *es, int fd,
				   unsigned int flags, void *data);

static void lt_DoHello(L2tpSelector *es, int fd,
			    unsigned int flags, void *data);

static void lt_DequeueAckedPackets(l2tp_tunnel *tunnel);
static void lt_SendStopCCN(l2tp_tunnel *tunnel,
				int result_code,
				int error_code,
				char const *fmt, ...);
static void lt_HandleSCCRP(l2tp_tunnel *tunnel,
		    l2tp_dgram *dgram);
static void lt_ScheduleDestruction(l2tp_tunnel *tunnel);
static int lt_SetParams(l2tp_tunnel *tunnel, l2tp_dgram *dgram);

static void lt_SetupHello(l2tp_tunnel *tunnel);
static void lt_TellSessionsTunnelOpen(l2tp_tunnel *tunnel);
static l2tp_tunnel *lt_FindByPeer(struct sockaddr_in addr);

#ifdef ENABLE_DEBUG
static char *state_names[] = {
    "idle", "wait-ctl-reply", "wait-ctl-conn", "established",
    "received-stop-ccn", "sent-stop-ccn"
};
#endif

#define VENDOR_STR "Ralink Inc."

/* Comparison of serial numbers according to RFC 1982 */
#define SERIAL_GT(a, b) \
(((a) > (b) && (a) - (b) < 32768) || ((a) < (b) && (b) - (a) > 32768))

#define SERIAL_LT(a, b) \
(((a) < (b) && (b) - (a) < 32768) || ((a) > (b) && (a) - (b) > 32768))


static l2tp_tunnel l2tp_tunnel_array[TUNNEL_NUM];
static int l2tp_tunnel_array_indx = 0;

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
static void lt_SetState(l2tp_tunnel *tunnel, int state)
{
    if (state == tunnel->state) return;
	
    DBG(ld_Debug(DBG_TUNNEL, "tunnel(%s) state %s -> %s\n",
	   ld_Tunnel2Str(tunnel),
	   state_names[tunnel->state],
	   state_names[state]));
    tunnel->state = state;
}

/*--------------------------------------------------------------
* ROUTINE NAME -   lt_FindByMyId       
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
l2tp_tunnel *lt_FindByMyId(uint16_t id)
{
	int i;
	
	for (i=0; i< TUNNEL_NUM; i++)
	{
		if (l2tp_tunnel_array[i].use && l2tp_tunnel_array[i].my_id == id)
			return &l2tp_tunnel_array[i];
	}
	return NULL;

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
static l2tp_tunnel *lt_FindByPeer(struct sockaddr_in addr)
{
	int i;
	
	for (i=0; i< TUNNEL_NUM; i++)
	{
		if (l2tp_tunnel_array[i].use && 
			l2tp_tunnel_array[i].peer_addr.sin_addr.s_addr == addr.sin_addr.s_addr)
			return &l2tp_tunnel_array[i];
	}
	return NULL;

}
/*--------------------------------------------------------------
* ROUTINE NAME -  lt_MakeId        
*---------------------------------------------------------------
* FUNCTION: The number of outstanding (transmitted, but not ack'd) frames.
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static int lt_NumFrams(l2tp_tunnel *tunnel)
{
    int Ns = (int) tunnel->Ns_on_wire;
    int Nr = (int) tunnel->peer_Nr;
    if (Ns >= Nr) return Ns - Nr;
    return 65536 - Nr + Ns;
}


#ifdef ENABLE_DEBUG
/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: A string representing flow-control info (used for debugging)
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static char const *lt_FlowCtlStats(l2tp_tunnel *tunnel)
{
    static char buf[256];

    snprintf(buf, sizeof(buf), "rws=%d cwnd=%d ssthresh=%d outstanding=%d",
	     (int) tunnel->peer_rws,
	     tunnel->cwnd,
	     tunnel->ssthresh,
	     lt_NumFrams(tunnel));
    return buf;
}
#endif


/*--------------------------------------------------------------
* ROUTINE NAME -  lt_MakeId        
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static uint16_t lt_MakeId(void)
{
    uint16_t id;
    for(;;) 
	{
		L2TP_RANDOM_FILL(id);
		if (!id) continue;
		if (!lt_FindByMyId(id)) return id;
    }
}

/*--------------------------------------------------------------
* ROUTINE NAME -  lt_EnqueuePkt        
*---------------------------------------------------------------
* FUNCTION: Adds datagram to end of transmit queue.
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static void lt_EnqueuePkt(l2tp_tunnel *tunnel,
		     l2tp_dgram  *dgram)
{

    dgram->next = NULL;
    if (tunnel->xmit_queue_tail) 
	{
		tunnel->xmit_queue_tail->next = dgram;
		tunnel->xmit_queue_tail = dgram;
    } else 
	{
		tunnel->xmit_queue_head = dgram;
		tunnel->xmit_queue_tail = dgram;
    }
    if (!tunnel->xmit_new_dgrams) 
	{
		tunnel->xmit_new_dgrams = dgram;
    }
	
    dgram->Ns = tunnel->Ns;
    tunnel->Ns++;
    DBG(ld_Debug(DBG_FLOW, "lt_EnqueuePkt(%s, %s) %s\n",
	ld_Tunnel2Str(tunnel),
	ld_MessageType2Str(dgram->msg_type),
	lt_FlowCtlStats(tunnel)));
	
}

/*--------------------------------------------------------------
* ROUTINE NAME - lt_DequeueHead         
*---------------------------------------------------------------
* FUNCTION: Dequeues datagram from head of transmit queue and frees it
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static void lt_DequeueHead(l2tp_tunnel *tunnel)
{
	
    l2tp_dgram *dgram = tunnel->xmit_queue_head;
    if (dgram) 
	{
		tunnel->xmit_queue_head = dgram->next;
		if (tunnel->xmit_new_dgrams == dgram) 
		{
			tunnel->xmit_new_dgrams = dgram->next;
		}
		if (tunnel->xmit_queue_tail == dgram) 
		{
			tunnel->xmit_queue_tail = NULL;
		}
		lf_Free(dgram);
    }
}


/*--------------------------------------------------------------
* ROUTINE NAME -  lt_XmitQueuedMessages        
*---------------------------------------------------------------
* FUNCTION: Transmits as many control messages as possible from the queue.
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static void lt_XmitQueuedMessages(l2tp_tunnel *tunnel)
{
    l2tp_dgram *dgram;
    struct timeval t;
	
    dgram = tunnel->xmit_new_dgrams;
    if (!dgram) return;
	
    DBG(ld_Debug(DBG_FLOW, "xmit_queued(%s): %s\n",
	ld_Tunnel2Str(tunnel),
	lt_FlowCtlStats(tunnel)));
    while (dgram) 
	{
		/* If window is closed, we can't transmit anything */
		if (lt_NumFrams(tunnel) >= tunnel->cwnd) 
		{
			break;
		}
		
		/* Update Nr */
		dgram->Nr = tunnel->Nr;
		
		/* Tid might have changed if call was initiated before
		tunnel establishment was complete */
		dgram->tid = tunnel->assigned_id;
		
		if (lf_SendToBSD(dgram, &tunnel->peer_addr) < 0)
		{
		  diag_printf("enter %s, Tunnel sended message error! L2TP failed!\n", __func__);
		  break;
		}
		
		/* Cancel any pending ack */
		
		tunnel->xmit_new_dgrams = dgram->next;
		tunnel->Ns_on_wire = dgram->Ns + 1;
		DBG(ld_Debug(DBG_FLOW, "loop in xmit_queued(%s): %s\n",
		ld_Tunnel2Str(tunnel),
		lt_FlowCtlStats(tunnel)));
		dgram = dgram->next;
    }
	
    t.tv_sec = tunnel->timeout;
    t.tv_usec = 0;
    /* Set retransmission timer */
    if (tunnel->timeout_handler) 
	{
		l2tp_ChangeTimeout(tunnel->timeout_handler, t);
    } 
	else 
	{
		tunnel->timeout_handler =
	    l2tp_AddTimerHandler(tunnel->es, t,
		lt_HandleTimeout, tunnel);
    }
}

/*--------------------------------------------------------------
* ROUTINE NAME - lt_XmitCtlMessage
*---------------------------------------------------------------
* FUNCTION: Transmits a control message.  If there is no room in the transmit
*  window, queues the message.
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void lt_XmitCtlMessage(l2tp_tunnel *tunnel,
				 l2tp_dgram  *dgram)
{
   /* Queue the datagram in case we need to retransmit it */
    lt_EnqueuePkt(tunnel, dgram);

    /* Run the queue */
    lt_XmitQueuedMessages(tunnel);
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
void lt_Cleanup(void *data)
{
    L2tpSelector *es = (L2tpSelector *) data;

    /* Stop all tunnels */
    lt_StopAll("Shutting down");

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
static l2tp_tunnel *lt_Init(L2tpSelector *es)
{
	l2tp_tunnel *tunnel;
	
	tunnel = &l2tp_tunnel_array[(l2tp_tunnel_array_indx++)%4];
	if(l2tp_tunnel_array_indx>=4)
		l2tp_tunnel_array_indx = 0;

    memset(tunnel, 0, sizeof(l2tp_tunnel));

	tunnel->use = 1;
    tunnel->rws = 4;
    tunnel->peer_rws = 1;
    tunnel->es = es;
    tunnel->timeout = 1;
    tunnel->my_id = lt_MakeId();
    tunnel->ssthresh = 1;
    tunnel->cwnd = 1;

    return tunnel;
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
static void lt_Release(l2tp_tunnel *tunnel)
{
    l2tp_session *ses = &tunnel->session;
    L2tpSelector *es = tunnel->es;

	
    DBG(ld_Debug(DBG_TUNNEL, "lt_Release(%s)\n", ld_Tunnel2Str(tunnel)));
	
	
	ls_Release(ses, "Tunnel closing down");
    if (tunnel->hello_handler) l2tp_DelHandler(es, tunnel->hello_handler);
    if (tunnel->timeout_handler) l2tp_DelHandler(es, tunnel->timeout_handler);
    //if (tunnel->ack_handler) l2tp_DelHandler(es, tunnel->ack_handler);
	
    while(tunnel->xmit_queue_head) 
	{
		lt_DequeueHead(tunnel);
    }
    memset(tunnel, 0, sizeof(l2tp_tunnel));
    l2tp_tunnel_array_indx--;
	
}

/*--------------------------------------------------------------
* ROUTINE NAME -  lt_Establish
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static l2tp_tunnel *lt_Establish(l2tp_peer *peer, L2tpSelector *es)
{
    l2tp_tunnel *tunnel;

    tunnel = lt_Init(es);
    if (!tunnel) return NULL;

    tunnel->peer = peer;
    tunnel->peer_addr = peer->addr;

    lt_SendSCCRQ(tunnel);
    lt_SetState(tunnel, TUNNEL_WAIT_CTL_REPLY);
    return tunnel;
}

/*--------------------------------------------------------------
* ROUTINE NAME - lt_SendSCCRQ
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static int lt_SendSCCRQ(l2tp_tunnel *tunnel)
{
    uint16_t u16;
    uint32_t u32;
    unsigned char tie_breaker[8];
    unsigned char challenge[16];
    int old_hide;

    l2tp_dgram *dgram = lf_NewCtl(MESSAGE_SCCRQ, 0, 0);
    if (!dgram) return -1;


    /* Add the AVP's */
    /* HACK!  Cisco equipment cannot handle hidden AVP's in SCCRQ.
       So we temporarily disable AVP hiding */
    old_hide = tunnel->peer->hide_avps;
    tunnel->peer->hide_avps = 0;

    /* Protocol version */
    u16 = htons(0x0100);
    lf_AddAvp(dgram, tunnel, MANDATORY,
		  sizeof(u16), VENDOR_IETF, AVP_PROTOCOL_VERSION, &u16);

    /* Framing capabilities -- hard-coded as sync and async */
    u32 = htonl(3);
    lf_AddAvp(dgram, tunnel, MANDATORY,
		  sizeof(u32), VENDOR_IETF, AVP_FRAMING_CAPABILITIES, &u32);

    /* Host name */
    lf_AddAvp(dgram, tunnel, MANDATORY,
		  strlen(L2tp_Hostname), VENDOR_IETF, AVP_HOST_NAME,
		  L2tp_Hostname);

    /* Assigned ID */
    u16 = htons(tunnel->my_id);
    lf_AddAvp(dgram, tunnel, MANDATORY,
		  sizeof(u16), VENDOR_IETF, AVP_ASSIGNED_TUNNEL_ID, &u16);

	u16 = htons(0);
    lf_AddAvp(dgram, tunnel, MANDATORY,
		  sizeof(u32), VENDOR_IETF, AVP_BEARER_CAPABILITIES, &u32);

    /* Receive window size */
    u16 = htons(tunnel->rws);
    lf_AddAvp(dgram, tunnel, MANDATORY,
		  sizeof(u16), VENDOR_IETF, AVP_RECEIVE_WINDOW_SIZE, &u16);

    /* Tie-breaker */
    l2tp_random_fill(tie_breaker, sizeof(tie_breaker));
    lf_AddAvp(dgram, tunnel, NOT_MANDATORY,
		  sizeof(tie_breaker), VENDOR_IETF, AVP_TIE_BREAKER,
		  tie_breaker);

    /* Vendor name */
    lf_AddAvp(dgram, tunnel, NOT_MANDATORY,
		  strlen(VENDOR_STR), VENDOR_IETF,
		  AVP_VENDOR_NAME, VENDOR_STR);

    /* Challenge */
    if (tunnel->peer->secret_len) 
	{
		l2tp_random_fill(challenge, sizeof(challenge));
		lf_AddAvp(dgram, tunnel, MANDATORY,
				   sizeof(challenge), VENDOR_IETF,
				   AVP_CHALLENGE, challenge);
	
		/* Compute and save expected response */
		l2tp_AuthGenResponse(MESSAGE_SCCRP, tunnel->peer->secret,
					   challenge, sizeof(challenge), tunnel->expected_response);
    }

    tunnel->peer->hide_avps = old_hide;
    /* And ship it out */
    lt_XmitCtlMessage(tunnel, dgram);
    return 1;
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
void lt_HandleReceivedCtlDatagram(l2tp_dgram *dgram,
					     L2tpSelector *es,
					     struct sockaddr_in *from)
{
    l2tp_tunnel *tunnel;
	
	
    /* If it's SCCRQ, then handle it */
    if (dgram->msg_type == MESSAGE_SCCRQ) 
	{
		lt_HandleSCCRQ(dgram, es, from);
		return;
    }
	
    /* Find the tunnel to which it refers */
    if (dgram->tid == 0) 
	{
		l2tp_set_errmsg("Invalid control message - tunnel ID = 0");
		return;
    }
	
    tunnel = lt_FindByMyId(dgram->tid);
    if (!tunnel) 
	{
		/* TODO: Send error message back? */
		l2tp_set_errmsg("Invalid control message - unknown tunnel ID %d",
		(int) dgram->tid);
		return;
    }
	
    /* Verify that source address is the tunnel's peer */
    if (tunnel->peer->validate_peer_ip) 
	{
		if (from->sin_addr.s_addr != tunnel->peer_addr.sin_addr.s_addr) 
		{
			l2tp_set_errmsg("Invalid control message for tunnel %s - not sent from peer",
			ld_Tunnel2Str(tunnel));
			return;
		}
    }
	
    /* Set port for replies */
    tunnel->peer_addr.sin_port = from->sin_port;
	
    /* Schedule an ACK for 100ms from now, but do not ack ZLB's */
    if (dgram->msg_type != MESSAGE_ZLB) 
	{
		lt_ScheduleACK(tunnel);
    }
	
    /* If it's an old datagram, ignore it */
    if (dgram->Ns != tunnel->Nr) 
	{
		if (SERIAL_LT(dgram->Ns, tunnel->Nr)) 
		{
			/* Old packet: Drop it */
			/* Throw away ack'd packets in our xmit queue */
			lt_DequeueAckedPackets(tunnel);
			return;
		}
		/* Out-of-order packet or intermediate dropped packets.
		TODO: Look into handling this better */
		return;
    }
	
    /* Do not increment if we got ZLB */
    if (dgram->msg_type != MESSAGE_ZLB) 
	{
		tunnel->Nr++;
    }
	
    /* Update peer_Nr */
    if (SERIAL_GT(dgram->Nr, tunnel->peer_Nr)) 
	{
		tunnel->peer_Nr = dgram->Nr;
    }
	
    /* Reschedule HELLO handler for 60 seconds in future */
    if (tunnel->state != TUNNEL_RECEIVED_STOP_CCN &&
		tunnel->state != TUNNEL_SENT_STOP_CCN &&
	    tunnel->hello_handler != NULL) 
	{
		struct timeval t;
		t.tv_sec = 60;
		t.tv_usec = 0;
		l2tp_ChangeTimeout(tunnel->hello_handler, t);
    }
	
    /* Reset retransmission stuff */
    tunnel->retransmissions = 0;
    tunnel->timeout = 1;
	
    /* Throw away ack'd packets in our xmit queue */
    lt_DequeueAckedPackets(tunnel);
	
    /* Let the specific tunnel handle it */
    lt_ProcessReceivedDatagram(tunnel, dgram);
	
    /* Run the xmit queue -- window may have opened */
    lt_XmitQueuedMessages(tunnel);
	
    /* Destroy tunnel if required and if xmit queue empty */
    if (!tunnel->xmit_queue_head) 
	{
		if (tunnel->timeout_handler) 
		{
			l2tp_DelHandler(tunnel->es, tunnel->timeout_handler);
			tunnel->timeout_handler = NULL;
		}
		if (tunnel->state == TUNNEL_RECEIVED_STOP_CCN) 
		{
			lt_ScheduleDestruction(tunnel);
		} 
		else if (tunnel->state == TUNNEL_SENT_STOP_CCN) 
		{
			/* Our stop-CCN has been ack'd, destroy NOW */
			lt_Release(tunnel);
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
static void lt_HandleSCCRQ(l2tp_dgram *dgram,
		    L2tpSelector *es,
		    struct sockaddr_in *from)
{
    l2tp_tunnel *tunnel = NULL;
	
    uint16_t u16;
    uint32_t u32;
    unsigned char challenge[16];
	
    /* TODO: Check if this is a retransmitted SCCRQ */
    /* Allocate a tunnel */
    tunnel = lt_Init(es);
    if (!tunnel) 
	{
		l2tp_set_errmsg("Unable to allocate new tunnel");
		return;
    }
	
    tunnel->peer_addr = *from;
	
	//
    /* We've received our first control datagram (SCCRQ) */
    tunnel->Nr = 1;
	
    if (lt_SetParams(tunnel, dgram) < 0) return;
	
    /* Hunky-dory.  Send SCCRP */
    dgram = lf_NewCtl(MESSAGE_SCCRP, tunnel->assigned_id, 0);
    if (!dgram) 
	{
		/* Doh! Out of resources.  Not much chance of StopCCN working... */
		lt_SendStopCCN(tunnel,
		RESULT_GENERAL_ERROR, ERROR_OUT_OF_RESOURCES,
		"Out of memory");
		return;
    }
	
    /* Protocol version */
    u16 = htons(0x0100);
    lf_AddAvp(dgram, tunnel, MANDATORY,
	sizeof(u16), VENDOR_IETF, AVP_PROTOCOL_VERSION, &u16);
	
    /* Framing capabilities -- hard-coded as sync and async */
    u32 = htonl(3);
    lf_AddAvp(dgram, tunnel, MANDATORY,
	sizeof(u32), VENDOR_IETF, AVP_FRAMING_CAPABILITIES, &u32);
	
    /* Host name */
    lf_AddAvp(dgram, tunnel, MANDATORY,
	strlen(L2tp_Hostname), VENDOR_IETF, AVP_HOST_NAME,
	L2tp_Hostname);
	
    /* Assigned ID */
    u16 = htons(tunnel->my_id);
    lf_AddAvp(dgram, tunnel, MANDATORY,
	sizeof(u16), VENDOR_IETF, AVP_ASSIGNED_TUNNEL_ID, &u16);
	
    /* Receive window size */
    u16 = htons(tunnel->rws);
    lf_AddAvp(dgram, tunnel, MANDATORY,
	sizeof(u16), VENDOR_IETF, AVP_RECEIVE_WINDOW_SIZE, &u16);
	
    /* Vendor name */
    lf_AddAvp(dgram, tunnel, NOT_MANDATORY,
	strlen(VENDOR_STR), VENDOR_IETF,
	AVP_VENDOR_NAME, VENDOR_STR);
	
    if (tunnel->peer->secret_len) 
	{
		/* Response */
		lf_AddAvp(dgram, tunnel, MANDATORY,
		MD5LEN, VENDOR_IETF, AVP_CHALLENGE_RESPONSE,
		tunnel->response);
		
		/* Challenge */
		l2tp_random_fill(challenge, sizeof(challenge));
		lf_AddAvp(dgram, tunnel, MANDATORY,
		sizeof(challenge), VENDOR_IETF,
		AVP_CHALLENGE, challenge);
		
		/* Compute and save expected response */
		l2tp_AuthGenResponse(MESSAGE_SCCCN, tunnel->peer->secret,
		challenge, sizeof(challenge), tunnel->expected_response);
    }
	
    lt_SetState(tunnel, TUNNEL_WAIT_CTL_CONN);
	
    /* And ship it out */
    lt_XmitCtlMessage(tunnel, dgram);
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
static void lt_ScheduleACK(l2tp_tunnel *tunnel)
{
    struct timeval t;

	
    DBG(ld_Debug(DBG_FLOW, "lt_ScheduleACK(%s)\n",
	   ld_Tunnel2Str(tunnel)));

    t.tv_sec = 0;
    t.tv_usec = 100000;
   lt_DoACK(tunnel->es, 0, 0, tunnel);
}


/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: If there is a frame on our queue, update it's Nr and run queue; if not,
*  send a ZLB immediately.
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static void lt_DoACK(L2tpSelector *es,
	      int fd,
	      unsigned int flags,
	      void *data)
{
    l2tp_tunnel *tunnel = (l2tp_tunnel *) data;


    DBG(ld_Debug(DBG_FLOW, "lt_DoACK(%s)\n",
	   ld_Tunnel2Str(tunnel)));

    /* If nothing is on the queue, add a ZLB */
    /* Or, if we cannot transmit because of flow-control, send ZLB */
    if (!tunnel->xmit_new_dgrams ||
		lt_NumFrams(tunnel) >= tunnel->cwnd) 
	{
		lt_SendZLB(tunnel);
		return;
    }

    /* Run the queue */
    lt_XmitQueuedMessages(tunnel);
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
static void lt_DequeueAckedPackets(l2tp_tunnel *tunnel)
{
    l2tp_dgram *dgram = tunnel->xmit_queue_head;
    DBG(ld_Debug(DBG_FLOW, "lt_DequeueAckedPackets(%s) %s\n",
	ld_Tunnel2Str(tunnel), lt_FlowCtlStats(tunnel)));
	
	while (dgram) 
	{
		if (SERIAL_LT(dgram->Ns, tunnel->peer_Nr)) {
			lt_DequeueHead(tunnel);
			if (tunnel->cwnd < tunnel->ssthresh) {
				/* Slow start: increment CWND for each packet dequeued */
				tunnel->cwnd++;
				if (tunnel->cwnd > tunnel->peer_rws) {
					tunnel->cwnd = tunnel->peer_rws;
				}
			} else {
				/* Congestion avoidance: increment CWND for each CWND
				packets dequeued */
				tunnel->cwnd_counter++;
				if (tunnel->cwnd_counter >= tunnel->cwnd) {
					tunnel->cwnd_counter = 0;
					tunnel->cwnd++;
					if (tunnel->cwnd > tunnel->peer_rws) {
						tunnel->cwnd = tunnel->peer_rws;
					}
				}
			}
		} else {
			break;
		}
		dgram = tunnel->xmit_queue_head;
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
void lt_HandleTimeout(L2tpSelector *es,
		      int fd,
		      unsigned int flags,
		      void *data)
{
    l2tp_tunnel *tunnel = (l2tp_tunnel *) data;
	
    cyg_thread_delay(10);
    /* Timeout handler has fired */
    tunnel->timeout_handler = NULL;
	
    /* Reset xmit_new_dgrams */
    tunnel->xmit_new_dgrams = tunnel->xmit_queue_head;
	
    /* Set Ns on wire to Ns of first frame in queue */
    if (tunnel->xmit_queue_head) 
	{
		tunnel->Ns_on_wire = tunnel->xmit_queue_head->Ns;
    }
	
    /* Go back to slow-start phase */
    tunnel->ssthresh = tunnel->cwnd / 2;
    if (!tunnel->ssthresh) tunnel->ssthresh = 1;
    tunnel->cwnd = 1;
    tunnel->cwnd_counter = 0;
	
    tunnel->retransmissions++;
    if (tunnel->retransmissions >= MAX_RETRANSMISSIONS) 
	{
		l2tp_set_errmsg("Too many retransmissions on tunnel (%s); closing down",
		ld_Tunnel2Str(tunnel));
		/* Close tunnel... */
		lt_Release(tunnel);
		return;
    }
	
    /* Double timeout, capping at 8 seconds */
    if (tunnel->timeout < 8) 
	{
		tunnel->timeout *= 2;
    }
	
    /* Run the queue */
    lt_XmitQueuedMessages(tunnel);
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
static void lt_SendStopCCN(l2tp_tunnel *tunnel,
		    int result_code,
		    int error_code,
		    char const *fmt, ...)
{
    char buf[256];
    va_list ap;
    uint16_t u16;
    uint16_t len;
    l2tp_dgram *dgram;

    /* Build the buffer for the result-code AVP */
    buf[0] = result_code / 256;
    buf[1] = result_code & 255;
    buf[2] = error_code / 256;
    buf[3] = error_code & 255;

    va_start(ap, fmt);
    vsnprintf(buf+4, 256-4, fmt, ap);
    buf[255] = 0;
    va_end(ap);

    DBG(ld_Debug(DBG_TUNNEL, "lt_SendStopCCN(%s, %d, %d, %s)\n",
	   ld_Tunnel2Str(tunnel), result_code, error_code, buf+4));

    len = 4 + strlen(buf+4);
    /* Build the datagram */
    dgram = lf_NewCtl(MESSAGE_StopCCN, tunnel->assigned_id, 0);
    if (!dgram) return;

    /* Add assigned tunnel ID */
    u16 = htons(tunnel->my_id);
    lf_AddAvp(dgram, tunnel, MANDATORY,
		  sizeof(u16), VENDOR_IETF, AVP_ASSIGNED_TUNNEL_ID, &u16);

    /* Add result code */
    lf_AddAvp(dgram, tunnel, MANDATORY,
		  len, VENDOR_IETF, AVP_RESULT_CODE, buf);

    /* TODO: Clean up */
    lt_SetState(tunnel, TUNNEL_SENT_STOP_CCN);

    /* Ship it out */
    lt_XmitCtlMessage(tunnel, dgram);
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
static void lt_HandleStopCCN(l2tp_tunnel *tunnel,
		       l2tp_dgram *dgram)
{
    unsigned char *val;
    int mandatory, hidden;
    uint16_t len, vendor, type;
    int err;
    l2tp_session *ses = &tunnel->session;
	
	ls_Release(ses, "Tunnel closing down");
	
    lt_SetState(tunnel, TUNNEL_RECEIVED_STOP_CCN);
	
    /* If there are any queued datagrams waiting for transmission,
	nuke them and adjust tunnel's Ns to whatever peer has ack'd */
    /* TODO: Is this correct? */
    if (tunnel->xmit_queue_head) 
	{
		tunnel->Ns = tunnel->peer_Nr;
		while(tunnel->xmit_queue_head) {
			lt_DequeueHead(tunnel);
		}
    }
	
    /* Parse the AVP's */
    while(1) 
	{
		val = lf_PullAvp(dgram, tunnel, &mandatory, &hidden,
		&len, &vendor, &type, &err);
		if (!val) 
		{
			break;
		}
		
		/* Only care about assigned tunnel ID.  TODO: Fix this! */
		if (vendor != VENDOR_IETF || type != AVP_ASSIGNED_TUNNEL_ID) 
		{
			continue;
		}
		
		if (len == 2) 
		{
			tunnel->assigned_id = ((unsigned int) val[0]) * 256 +
			(unsigned int) val[1];
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
static void lt_ProcessReceivedDatagram(l2tp_tunnel *tunnel,
				 l2tp_dgram *dgram)
{
    l2tp_session *ses = NULL;
	#ifdef CONFIG_L2TP_OVER_PPPOE
	int l2tp_wanif;
	#endif
	
    /* If message is associated with existing session, find session */
	
    DBG(ld_Debug(DBG_TUNNEL, "lt_ProcessReceivedDatagram(%s, %s)\n",
	ld_Tunnel2Str(tunnel),
	ld_MessageType2Str(dgram->msg_type)));
    switch (dgram->msg_type) 
	{
		case MESSAGE_OCRP:
		case MESSAGE_OCCN:
		case MESSAGE_ICRP:
		case MESSAGE_ICCN:
		case MESSAGE_CDN:
		ses = lt_FindSession(tunnel, dgram->sid);
		if (!ses) {
			l2tp_set_errmsg("Session-related message for unknown session %d",
			(int) dgram->sid);
			return;
		}
    }
	
    switch (dgram->msg_type) 
	{
		case MESSAGE_StopCCN:
		lt_HandleStopCCN(tunnel, dgram);
		return;
		case MESSAGE_SCCRP:
		lt_HandleSCCRP(tunnel, dgram);
		return;
		case MESSAGE_SCCCN:
		lt_HandleSCCCN(tunnel, dgram);
		return;
		case MESSAGE_CDN:
		ls_HandleCDN(ses, dgram);
		#ifdef CONFIG_L2TP_OVER_PPPOE
	    CFG_get(CFG_L2T_WANIF, &l2tp_wanif);
	    if((l2tp_wanif == 2) && (primary_wan_ip_set[0].up)){
		  cyg_thread_delay(100);
	      mon_snd_cmd(MON_CMD_WAN_UPDATE);// stop and init l2tp over pppoe
	    }
		#endif
		return;
		case MESSAGE_ICRP:
		ls_HandleICRP(ses, dgram);
		return;
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
static void lt_FinallyCleanup(L2tpSelector *es,
		       int fd,
		       unsigned int flags,
		       void *data)
{
    l2tp_tunnel *tunnel = (l2tp_tunnel *) data;

    /* Hello handler has fired */
    tunnel->hello_handler = NULL;
    lt_Release(tunnel);
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: Schedules tunnel for destruction 31 seconds hence.
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static void lt_ScheduleDestruction(l2tp_tunnel *tunnel)
{
    struct timeval t;
    l2tp_session *ses;
	
    t.tv_sec = 0;
    t.tv_usec = 200000;
	
    DBG(ld_Debug(DBG_TUNNEL, "lt_ScheduleDestruction(%s)\n",
	ld_Tunnel2Str(tunnel)));
    /* Shut down the tunnel in 31 seconds - (ab)use the hello handler */
    if (tunnel->hello_handler) {
		l2tp_DelHandler(tunnel->es, tunnel->hello_handler);
    }
    tunnel->hello_handler =
	l2tp_AddTimerHandler(tunnel->es, t,
	lt_FinallyCleanup, tunnel);
	
	ses = &tunnel->session;
	ls_Release(ses, "Tunnel closing down");
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
void lt_SendZLB(l2tp_tunnel *tunnel)
{
    l2tp_dgram *dgram =
	lf_NewCtl(MESSAGE_ZLB, tunnel->assigned_id, 0);

    if (!dgram) {
	l2tp_set_errmsg("lt_SendZLB: Out of memory");
	return;
    }
    dgram->Nr = tunnel->Nr;
    dgram->Ns = tunnel->Ns;
    lf_SendToBSD(dgram, &tunnel->peer_addr);
    lf_Free(dgram);
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
static void lt_HandleSCCRP(l2tp_tunnel *tunnel,
		    l2tp_dgram *dgram)
{
	
    /* TODO: If we don't get challenge response at all, barf */
    /* Are we expecing SCCRP? */
    if (tunnel->state != TUNNEL_WAIT_CTL_REPLY) 
	{
		lt_SendStopCCN(tunnel, RESULT_FSM_ERROR, 0, "Not expecting SCCRP");
		return;
    }
	
    /* Extract tunnel params */
    if (lt_SetParams(tunnel, dgram) < 0) return;
	
    lt_SetState(tunnel, TUNNEL_ESTABLISHED);
    lt_SetupHello(tunnel);
	
    /* Hunky-dory.  Send SCCCN */
    dgram = lf_NewCtl(MESSAGE_SCCCN, tunnel->assigned_id, 0);
    if (!dgram) 
	{
		/* Doh! Out of resources.  Not much chance of StopCCN working... */
		lt_SendStopCCN(tunnel,
					RESULT_GENERAL_ERROR, ERROR_OUT_OF_RESOURCES,
		"Out of memory");
		return;
    }
	
    /* Add response */
    if (tunnel->peer->secret_len) 
	{
		lf_AddAvp(dgram, tunnel, MANDATORY,
		MD5LEN, VENDOR_IETF, AVP_CHALLENGE_RESPONSE,
		tunnel->response);
    }
	
    /* Ship it out */
    lt_XmitCtlMessage(tunnel, dgram);
	
    /* Tell sessions tunnel has been established */
    lt_TellSessionsTunnelOpen(tunnel);
}

/*--------------------------------------------------------------
* ROUTINE NAME - lt_SetParams
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static int lt_SetParams(l2tp_tunnel *tunnel,
		  l2tp_dgram *dgram)
{
    unsigned char *val;
    int mandatory, hidden;
    uint16_t len, vendor, type;
    int err = 0;
    int found_response = 0;
	
    uint16_t u16;
	
    /* Find peer */
    tunnel->peer = &l2tp_peercfg;
	
    /* Get assigned tunnel ID */
    val = lf_SearchAvp(dgram, tunnel, &mandatory, &hidden, &len,
	VENDOR_IETF, AVP_ASSIGNED_TUNNEL_ID);
    if (!val) 
	{
		l2tp_set_errmsg("No assigned tunnel ID AVP in SCCRQ");
		lt_Release(tunnel);
		return -1;
    }
	
    if (!lf_ValidateAvp(VENDOR_IETF, AVP_ASSIGNED_TUNNEL_ID,
	len, mandatory)) 
	{
		lt_Release(tunnel);
		return -1;
    }
	
    /* Set tid */
    tunnel->assigned_id = ((unsigned int) val[0]) * 256 + (unsigned int) val[1];
    if (!tunnel->assigned_id) 
	{
		l2tp_set_errmsg("Invalid assigned-tunnel-ID of zero");
		lt_Release(tunnel);
		return -1;
    }
	
    /* Validate peer */
    if (!tunnel->peer) 
	{
		l2tp_set_errmsg("Peer %s is not authorized to create a tunnel",
		inet_ntoa(tunnel->peer_addr.sin_addr));
		lt_SendStopCCN(tunnel, RESULT_NOAUTH, ERROR_OK, "%s", l2tp_get_errmsg());
		return -1;
    }
	
    /* Pull out and examine AVP's */
    while(1) 
	{
		val = lf_PullAvp(dgram, tunnel, &mandatory, &hidden,
		&len, &vendor, &type, &err);
		if (!val) 
		{
			if (err) 
			{
				lt_SendStopCCN(tunnel, RESULT_GENERAL_ERROR,
				ERROR_BAD_VALUE, "%s", l2tp_get_errmsg());
				return -1;
			}
			break;
		}
		
		/* Unknown vendor?  Ignore, unless mandatory */
		if (vendor != VENDOR_IETF) 
		{
			if (!mandatory) continue;
			lt_SendStopCCN(tunnel, RESULT_GENERAL_ERROR,
			ERROR_UNKNOWN_AVP_WITH_M_BIT,
			"Unknown mandatory attribute (vendor=%d, type=%d) in %s",
			(int) vendor, (int) type,
			ld_Avp2Str(dgram->msg_type));
			return -1;
		}
		
		/* Validate AVP, ignore invalid AVP's without M bit set */
		if (!lf_ValidateAvp(vendor, type, len, mandatory)) 
		{
			if (!mandatory) continue;
			lt_SendStopCCN(tunnel, RESULT_GENERAL_ERROR,
			ERROR_BAD_LENGTH, "%s", l2tp_get_errmsg());
			return -1;
		}
		
		switch(type) 
		{
			case AVP_PROTOCOL_VERSION:
			u16 = ((uint16_t) val[0]) * 256 + val[1];
			if (u16 != 0x100) 
			{
				lt_SendStopCCN(tunnel, RESULT_UNSUPPORTED_VERSION,
				0x100, "Unsupported protocol version");
				return -1;
			}
			break;
			
			case AVP_HOST_NAME:
			if (len >= MAX_HOSTNAME) len = MAX_HOSTNAME-1;
			memcpy(tunnel->peer_hostname, val, len);
			tunnel->peer_hostname[len] = 0;
			DBG(ld_Debug(DBG_TUNNEL, "%s: Peer host name is '%s'\n",
			ld_Tunnel2Str(tunnel), tunnel->peer_hostname));
			break;
			
			case AVP_FRAMING_CAPABILITIES:
			/* TODO: Do we care about framing capabilities? */
			break;
			
			case AVP_ASSIGNED_TUNNEL_ID:
			/* Already been handled */
			break;
			
			case AVP_BEARER_CAPABILITIES:
			/* TODO: Do we care? */
			break;
			
			case AVP_RECEIVE_WINDOW_SIZE:
			u16 = ((uint16_t) val[0]) * 256 + val[1];
			/* Silently correct bogus rwin */
			if (u16 < 1) u16 = 1;
			tunnel->peer_rws = u16;
			tunnel->ssthresh = u16;
			break;
			
			case AVP_CHALLENGE:
			if (tunnel->peer->secret_len) 
			{
				l2tp_AuthGenResponse(
				((dgram->msg_type == MESSAGE_SCCRQ) ? MESSAGE_SCCRP : MESSAGE_SCCCN),
				tunnel->peer->secret,
				val, len, tunnel->response);
			}
			break;
			
			case AVP_CHALLENGE_RESPONSE:
			/* Length has been validated by lf_ValidateAvp */
			if (tunnel->peer->secret_len) 
			{
				if (memcmp(val, tunnel->expected_response, MD5LEN)) 
				{
					lt_SendStopCCN(tunnel, RESULT_NOAUTH, ERROR_BAD_VALUE,
					"Incorrect challenge response");
					return -1;
				}
			}
			found_response = 1;
			break;
			
			case AVP_TIE_BREAKER:
			/* TODO: Handle tie-breaker */
			break;
			
			case AVP_FIRMWARE_REVISION:
			/* TODO: Do we care? */
			break;
			
			case AVP_VENDOR_NAME:
			/* TODO: Do we care? */
			break;
			
			default:
			/* TODO: Maybe print an error? */
			break;
		}
    }
	
    if (tunnel->peer->secret_len &&
		!found_response &&
	dgram->msg_type != MESSAGE_SCCRQ) 
	{
		lt_SendStopCCN(tunnel, RESULT_NOAUTH, 0,
		"Missing challenge-response");
		return -1;
    }
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
static void lt_SetupHello(l2tp_tunnel *tunnel)
{
    struct timeval t;

    t.tv_sec = 60;
    t.tv_usec = 0;

    if (tunnel->hello_handler) 
	{
		l2tp_DelHandler(tunnel->es, tunnel->hello_handler);
    }
    tunnel->hello_handler = l2tp_AddTimerHandler(tunnel->es, t,
						  lt_DoHello, tunnel);
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
static void lt_DoHello(L2tpSelector *es,
		int fd,
		unsigned int flags,
		void *data)
{
    l2tp_tunnel *tunnel = (l2tp_tunnel *) data;
    l2tp_dgram *dgram;

    /* Hello handler has fired */
    tunnel->hello_handler = NULL;

    /* Reschedule HELLO timer */
    lt_SetupHello(tunnel);

    /* Send a HELLO message */
    dgram = lf_NewCtl(MESSAGE_HELLO, tunnel->assigned_id, 0);
    if (dgram) lt_XmitCtlMessage(tunnel, dgram);
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
static void lt_HandleSCCCN(l2tp_tunnel *tunnel,
		    l2tp_dgram *dgram)
{
    unsigned char *val;
    uint16_t len;
    int hidden, mandatory;
	
    /* Are we expecing SCCCN? */
    if (tunnel->state != TUNNEL_WAIT_CTL_CONN) 
	{
		lt_SendStopCCN(tunnel, RESULT_FSM_ERROR, 0, "Not expecting SCCCN");
		return;
    }
	
    /* Check challenge response */
    if (tunnel->peer->secret_len) 
	{
		val = lf_SearchAvp(dgram, tunnel, &mandatory, &hidden, &len,
		VENDOR_IETF, AVP_CHALLENGE_RESPONSE);
		if (!val) 
		{
			lt_SendStopCCN(tunnel, RESULT_NOAUTH, 0,
			"Missing challenge-response");
			return;
		}
		
		if (!lf_ValidateAvp(VENDOR_IETF, AVP_CHALLENGE_RESPONSE,
		len, mandatory)) 
		{
			lt_SendStopCCN(tunnel, RESULT_GENERAL_ERROR, ERROR_BAD_LENGTH,
			"Invalid challenge-response");
			return;
		}
		if (memcmp(val, tunnel->expected_response, MD5LEN)) 
		{
			lt_SendStopCCN(tunnel, RESULT_NOAUTH, ERROR_BAD_VALUE,
			"Incorrect challenge response");
			return;
		}
    }
	
    lt_SetState(tunnel, TUNNEL_ESTABLISHED);
    lt_SetupHello(tunnel);
	
    /* Tell sessions tunnel has been established */
    lt_TellSessionsTunnelOpen(tunnel);
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
l2tp_tunnel *lt_FindForPeer(l2tp_peer *peer,
		     L2tpSelector *es)
{
    l2tp_tunnel *tunnel = lt_FindByPeer(peer->addr);

	if (tunnel) {
	if (tunnel->state == TUNNEL_WAIT_CTL_REPLY ||
	    tunnel->state == TUNNEL_WAIT_CTL_CONN ||
	    tunnel->state == TUNNEL_ESTABLISHED) {
	    return tunnel;
	}
    }

    /* No tunnel, or tunnel in wrong state */
    return lt_Establish(peer, es);
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
l2tp_session *lt_FindSession(l2tp_tunnel *tunnel,
		    uint16_t sid)
{
	if (tunnel->session.my_id == sid)
		return (l2tp_session *)tunnel;
	else
		return NULL;
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
static void lt_TellSessionsTunnelOpen(l2tp_tunnel *tunnel)
{
	ls_NotifyTunnelOpen(&tunnel->session);
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
void lt_AddSession(l2tp_session *ses)
{
    l2tp_tunnel *tunnel = (l2tp_tunnel *)ses;


    if (tunnel->state == TUNNEL_ESTABLISHED) 
	{
		ls_NotifyTunnelOpen(ses);
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
void lt_DeleteSession(l2tp_session *ses, char const *reason)
{
    l2tp_tunnel *tunnel = (l2tp_tunnel *)ses;


    ls_Release(ses, reason);

    /* Tear down tunnel if so indicated */
   
	if (!tunnel->peer->retain_tunnel) 
	{
	    lt_SendStopCCN(tunnel,
				RESULT_GENERAL_REQUEST, 0,
				"Last session has closed");
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
void lt_StopAll(char const *reason)
{
	int i;

    /* Send StopCCN on all tunnels except those which are scheduled for
       destruction */
	for (i=0; i<TUNNEL_NUM; i++)
	{
		if(l2tp_tunnel_array[i].use)
		{
			lt_StopTunnel(&l2tp_tunnel_array[i], reason);
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
void lt_StopTunnel(l2tp_tunnel *tunnel,
			char const *reason)
{
 
	/* Do not send StopCCN if we've received one already */
    if (tunnel->state != TUNNEL_RECEIVED_STOP_CCN &&
	tunnel->state != TUNNEL_SENT_STOP_CCN) 
	{
		lt_SendStopCCN(tunnel, RESULT_SHUTTING_DOWN, 0, reason);
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
void lt_ForceFinallyCleanup(void *data)
{
	lt_FinallyCleanup(NULL, 0, 0, data);
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
void lt_FreeAllHandler(void *data)
{
	L2tpSelector *es;
	l2tp_tunnel *tunnel;
	
	tunnel= (l2tp_tunnel *)data;
	es = tunnel->es;
	
	if (tunnel->hello_handler) l2tp_DelHandler(es, tunnel->hello_handler);
    if (tunnel->timeout_handler) l2tp_DelHandler(es, tunnel->timeout_handler);
}	
