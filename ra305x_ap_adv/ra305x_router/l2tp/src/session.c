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
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <network.h>
#include <l2tp.h>
#include <l2tp_ppp.h>

extern struct l2tp_cfg L2tp_config_param;

static uint16_t ls_MakeId(l2tp_tunnel *tunnel);
static void ls_SetState(l2tp_session *session, int state);

static uint32_t call_serial_number = 0;

static char *state_names[] = {
    "idle", "wait-tunnel", "wait-reply", "wait-connect", "established"
};


/*--------------------------------------------------------------
* ROUTINE NAME - ls_Release
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void ls_Release(l2tp_session *ses, char const *reason)
{

    ls_SetState(ses, SESSION_IDLE);
    DBG(ld_Debug(DBG_SESSION, "session_free(%s) %s\n",
	   ld_Ses2Str(ses), reason));
    
	l2_PppClose(ses);
    
    memset(ses, 0, sizeof(l2tp_session));
}

/*--------------------------------------------------------------
* ROUTINE NAME -  ls_CallINS        
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
l2tp_session * ls_CallINS(l2tp_peer *peer,
		      char const *calling_number,
		      L2tpSelector *es,
		      void *private)
{
    l2tp_session *ses;
    l2tp_tunnel *tunnel;
    struct l2tp_if *pInfo;
	

    /* Find a tunnel to the peer */
    tunnel = lt_FindForPeer(peer, es);
    if (!tunnel) return NULL;


	ses = (l2tp_session *)tunnel;
	memset(ses,0,sizeof(l2tp_session));
    //yfchou added
    {
    	struct l2tp_cfg *pParam;
    	pParam = &L2tp_config_param;
    	pInfo = l2tp_IfNameToInfoList(pParam->pppname);
		if(pInfo)
    		pInfo->ses = ses;
		else
			diag_printf("%s pInfo=NULL\n",__FUNCTION__);
	}
    /* Init fields */
    memset(ses, 0, sizeof(l2tp_session));
    ses->tunnel = tunnel;
     ses->my_id = ls_MakeId(tunnel);
    ses->state = SESSION_WAIT_TUNNEL;
    strncpy(ses->calling_number, calling_number, MAX_HOSTNAME);
    ses->calling_number[MAX_HOSTNAME-1] = 0;
    ses->private = private;
    ses->send_accm = 0xFFFFFFFF;
    ses->recv_accm = 0xFFFFFFFF;

    /* Add it to the tunnel */
    lt_AddSession(ses);

    return ses;
}

/*--------------------------------------------------------------
* ROUTINE NAME - ls_MakeId
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static uint16_t ls_MakeId(l2tp_tunnel *tunnel)
{
    uint16_t sid;

	while(1) {
		L2TP_RANDOM_FILL(sid);
		if (!sid) continue;
		if (!lt_FindSession(tunnel, sid)) return sid;
    }
}

/*--------------------------------------------------------------
* ROUTINE NAME -  ls_NotifyTunnelOpen
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void ls_NotifyTunnelOpen(l2tp_session *ses)
{
    uint16_t u16;
    uint32_t u32;
    l2tp_dgram *dgram;
    l2tp_tunnel *tunnel = ses->tunnel;
	
	
    if (ses->state != SESSION_WAIT_TUNNEL) return;
	
    /* Send ICRQ */
    DBG(ld_Debug(DBG_SESSION, "Session %s tunnel open\n",
	ld_Ses2Str(ses)));
	
    dgram = lf_NewCtl(MESSAGE_ICRQ, tunnel->assigned_id, 0);
    if (!dgram) 
	{
		l2tp_set_errmsg("Could not establish session - out of memory");
		lt_DeleteSession(ses, "Out of memory");
		return;
    }
	
    /* assigned session ID */
    u16 = htons(ses->my_id);
    lf_AddAvp(dgram, tunnel, MANDATORY,
	sizeof(u16), VENDOR_IETF, AVP_ASSIGNED_SESSION_ID, &u16);
	
    /* Call serial number */
    u32 = htonl(call_serial_number);
    call_serial_number++;
    lf_AddAvp(dgram, tunnel, MANDATORY,
	sizeof(u32), VENDOR_IETF, AVP_CALL_SERIAL_NUMBER, &u32);
	
    /* Ship it out */
    lt_XmitCtlMessage(tunnel, dgram);
	
    ls_SetState(ses, SESSION_WAIT_REPLY);
}
/*--------------------------------------------------------------
* ROUTINE NAME -  ls_SetState
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static void ls_SetState(l2tp_session *session, int state)
{

	
    if (state == session->state) return;
    DBG(ld_Debug(DBG_SESSION, "session(%s) state %s -> %s\n",
	   ld_Ses2Str(session),
	   state_names[session->state],
	   state_names[state]));
    session->state = state;
}


/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: Sends CDN with specified result code and message.
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void ls_SendCDN(l2tp_session *ses,
		      int result_code,
		      int error_code,
		      char const *fmt, ...)
{
    char buf[256];
    va_list ap;
    l2tp_tunnel *tunnel = ses->tunnel;
    uint16_t len;
    l2tp_dgram *dgram;
    uint16_t u16;

    /* Build the buffer for the result-code AVP */
    buf[0] = result_code / 256;
    buf[1] = result_code & 255;
    buf[2] = error_code / 256;
    buf[3] = error_code & 255;

    va_start(ap, fmt);
    vsnprintf(buf+4, 256-4, fmt, ap);
    buf[255] = 0;
    va_end(ap);

    DBG(ld_Debug(DBG_SESSION, "session_send_CDN(%s): %s\n",
	   ld_Ses2Str(ses), buf+4));

    len = 4 + strlen(buf+4);
    /* Build the datagram */
    dgram = lf_NewCtl(MESSAGE_CDN, tunnel->assigned_id,
			      ses->assigned_id);
    if (!dgram) return;

    /* Add assigned session ID */
    u16 = htons(ses->my_id);
    lf_AddAvp(dgram, tunnel, MANDATORY,
		  sizeof(u16), VENDOR_IETF, AVP_ASSIGNED_SESSION_ID, &u16);

    /* Add result code */
    lf_AddAvp(dgram, tunnel, MANDATORY,
		  len, VENDOR_IETF, AVP_RESULT_CODE, buf);

    /* TODO: Clean up */
    ls_SetState(ses, SESSION_IDLE);

    /* Ship it out */
    lt_XmitCtlMessage(tunnel, dgram);

    /* Free session */
    lt_DeleteSession(ses, buf+4);
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: Handles a CDN by destroying session
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void ls_HandleCDN(l2tp_session *ses,
			l2tp_dgram *dgram)
{
    char buf[1024];
    unsigned char *val;
    uint16_t len;

    val = lf_SearchAvp(dgram, ses->tunnel, NULL, NULL, &len,
	VENDOR_IETF, AVP_RESULT_CODE);
    if (!val || len < 4) 
	{
		lt_DeleteSession(ses, "Received CDN");
    } 
	else 
	{
		uint16_t result_code, error_code;
		char *msg;
		result_code = ((uint16_t) val[0]) * 256 + (uint16_t) val[1];
		error_code = ((uint16_t) val[2]) * 256 + (uint16_t) val[3];
		if (len > 4) {
			msg = (char *) &val[4];
		} else {
			msg = "";
		}
		snprintf(buf, sizeof(buf), "Received CDN: result-code = %d, error-code = %d, message = '%.*s'", result_code, error_code, (int) len-4, msg);
		buf[1023] = 0;
		lt_DeleteSession(ses, buf);
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
void ls_HandleICRP(l2tp_session *ses,
			 l2tp_dgram *dgram)
{
    uint16_t u16;
    unsigned char *val;
    uint16_t len;
    uint32_t u32;

    int mandatory, hidden;
    l2tp_tunnel *tunnel = ses->tunnel;

	
    /* Get assigned session ID */
    val = lf_SearchAvp(dgram, tunnel, &mandatory, &hidden, &len,
	VENDOR_IETF, AVP_ASSIGNED_SESSION_ID);
    if (!val) 
	{
		l2tp_set_errmsg("No assigned session-ID in ICRP");
		return;
    }
    if (!lf_ValidateAvp(VENDOR_IETF, AVP_ASSIGNED_SESSION_ID,
	len, mandatory)) 
	{
		l2tp_set_errmsg("Invalid assigned session-ID in ICRP");
		return;
    }
	
    /* Set assigned session ID */
    u16 = ((uint16_t) val[0]) * 256 + (uint16_t) val[1];
	
    if (!u16) 
	{
		l2tp_set_errmsg("Invalid assigned session-ID in ICRP");
		return;
    }
	
    ses->assigned_id = u16;
	
    /* If state is not WAIT_REPLY, fubar */
    if (ses->state != SESSION_WAIT_REPLY) 
	{
		ls_SendCDN(ses, RESULT_FSM_ERROR, 0, "Received ICRP for session in state %s", state_names[ses->state]);
		return;
    }
	
    /* Send ICCN */
    dgram = lf_NewCtl(MESSAGE_ICCN, tunnel->assigned_id,
	ses->assigned_id);
    if (!dgram) 
	{
		/* Ugh... not much chance of this working... */
		ls_SendCDN(ses, RESULT_GENERAL_ERROR, ERROR_OUT_OF_RESOURCES,
		"Out of memory");
		return;
    }
	
    /* TODO: Speed, etc. are faked for now. */
	
    /* Connect speed */
    u32 = htonl(57600);
    lf_AddAvp(dgram, tunnel, MANDATORY,
	sizeof(u32), VENDOR_IETF, AVP_TX_CONNECT_SPEED, &u32);
	
    /* Framing Type */
    u32 = htonl(1);
    lf_AddAvp(dgram, tunnel, MANDATORY,
	sizeof(u32), VENDOR_IETF, AVP_FRAMING_TYPE, &u32);
	
    /* Ship it out */
    lt_XmitCtlMessage(tunnel, dgram);
	
    /* Set session state */
    ls_SetState(ses, SESSION_ESTABLISHED);
    
    l2_PppEstablish(ses);

}


