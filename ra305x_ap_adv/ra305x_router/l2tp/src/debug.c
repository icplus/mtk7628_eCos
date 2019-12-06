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

#include <l2tp.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>

#define AVPCASE(x) case AVP_ ## x: return #x
#define MSGCASE(x) case MESSAGE_ ## x: return #x

static unsigned long debug_mask = 0;

/* Big bang is when the universe started... set by first call
   to db */
static struct timeval big_bang = {0, 0};

/**********************************************************************
* %FUNCTION: debug_avp_type_to_str
* %ARGUMENTS:
*  type -- an AVP type
* %RETURNS:
*  A string representation of AVP type
***********************************************************************/
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
char const * ld_Avp2Str(uint16_t type)
{
    static char buf[64];
    switch(type) {
	AVPCASE(MESSAGE_TYPE);
	AVPCASE(RESULT_CODE);
	AVPCASE(PROTOCOL_VERSION);
	AVPCASE(FRAMING_CAPABILITIES);
	AVPCASE(BEARER_CAPABILITIES);
	AVPCASE(TIE_BREAKER);
	AVPCASE(FIRMWARE_REVISION);
	AVPCASE(HOST_NAME);
	AVPCASE(VENDOR_NAME);
	AVPCASE(ASSIGNED_TUNNEL_ID);
	AVPCASE(RECEIVE_WINDOW_SIZE);
	AVPCASE(CHALLENGE);
	AVPCASE(Q931_CAUSE_CODE);
	AVPCASE(CHALLENGE_RESPONSE);
	AVPCASE(ASSIGNED_SESSION_ID);
	AVPCASE(CALL_SERIAL_NUMBER);
	AVPCASE(MINIMUM_BPS);
	AVPCASE(MAXIMUM_BPS);
	AVPCASE(BEARER_TYPE);
	AVPCASE(FRAMING_TYPE);
	AVPCASE(CALLED_NUMBER);
	AVPCASE(CALLING_NUMBER);
	AVPCASE(SUB_ADDRESS);
	AVPCASE(TX_CONNECT_SPEED);
	AVPCASE(PHYSICAL_CHANNEL_ID);
	AVPCASE(INITIAL_RECEIVED_CONFREQ);
	AVPCASE(LAST_SENT_CONFREQ);
	AVPCASE(LAST_RECEIVED_CONFREQ);
	AVPCASE(PROXY_AUTHEN_TYPE);
	AVPCASE(PROXY_AUTHEN_NAME);
	AVPCASE(PROXY_AUTHEN_CHALLENGE);
	AVPCASE(PROXY_AUTHEN_ID);
	AVPCASE(PROXY_AUTHEN_RESPONSE);
	AVPCASE(CALL_ERRORS);
	AVPCASE(ACCM);
	AVPCASE(RANDOM_VECTOR);
	AVPCASE(PRIVATE_GROUP_ID);
	AVPCASE(RX_CONNECT_SPEED);
	AVPCASE(SEQUENCING_REQUIRED);
    }
    /* Unknown */
    sprintf(buf, "Unknown_AVP#%d", (int) type);
    return buf;
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
char const *
ld_MessageType2Str(uint16_t type)
{
    static char buf[64];
    switch(type) {
	MSGCASE(SCCRQ);
	MSGCASE(SCCRP);
	MSGCASE(SCCCN);
	MSGCASE(StopCCN);
	MSGCASE(HELLO);
	MSGCASE(OCRQ);
	MSGCASE(OCRP);
	MSGCASE(OCCN);
	MSGCASE(ICRQ);
	MSGCASE(ICRP);
	MSGCASE(ICCN);
	MSGCASE(CDN);
	MSGCASE(WEN);
	MSGCASE(SLI);
	MSGCASE(ZLB);
    }
    sprintf(buf, "Unknown_Message#%d", (int) type);
    return buf;
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
char const *ld_Tunnel2Str(l2tp_tunnel *tunnel)
{
    static char buf[64];
    sprintf(buf, "%d/%d", (int) tunnel->my_id, (int) tunnel->assigned_id);
    return buf;
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
char const *
ld_Ses2Str(l2tp_session *session)
{
	l2tp_tunnel *tunnel = (l2tp_tunnel *)session->tunnel;
	
    static char buf[128];
    sprintf(buf, "(%d/%d, %d/%d)",
	(int) tunnel->my_id,
	(int) tunnel->assigned_id,
	(int) session->my_id, (int) session->assigned_id);
    return buf;
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
void
ld_Debug(int what, char const *fmt, ...)
{
    va_list ap;
    struct timeval now;
    long sec_diff, usec_diff;
	
    if (!(debug_mask & what)) return;
	
    gettimeofday(&now, NULL);
	
    if (!big_bang.tv_sec) {
		big_bang = now;
    }
	
    /* Compute difference between now and big_bang */
    sec_diff = now.tv_sec - big_bang.tv_sec;
    usec_diff = now.tv_usec - big_bang.tv_usec;
    if (usec_diff < 0) {
		usec_diff += 1000000;
		sec_diff--;
    }
	
    /* Convert to seconds.milliseconds */
    usec_diff /= 1000;
	
    va_start(ap, fmt);
    fprintf(stderr, "%4ld.%03ld ", sec_diff, usec_diff);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
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
void
ld_SetBitMask(unsigned long mask)
{
    debug_mask = mask;
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
char const * ld_DescribeDgram(l2tp_dgram const *dgram)
{
    static char buf[256];
	
    if (dgram->bits & TYPE_BIT) {
		/* Control datagram */
		snprintf(buf, sizeof(buf), "type=%s, tid=%d, sid=%d, Nr=%d, Ns=%d",
		ld_MessageType2Str(dgram->msg_type),
		(int) dgram->tid, (int) dgram->sid,
		(int) dgram->Nr, (int) dgram->Ns);
    } else {
		snprintf(buf, sizeof(buf), "data message tid=%d sid=%d payload_len=%d",
		(int) dgram->tid, (int) dgram->sid,
		(int) dgram->payload_len);
    }
    return buf;
}


