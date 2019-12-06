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
 ***************************************************************************

    Module Name:
    pptp_ctrlconn.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <pptp.h>
#include <cfg_def.h>

int pptp_tcp_FIN_RST = 0;
static char pptp_read_buf[PPTP_RX_BUFSIZ];

extern void pptp_timer_action(PPTP_INFO *pInfo);
extern void pptp_timer_stop(PPTP_INFO *pInfo);
extern void pptp_timer_reset(PPTP_INFO *pInfo);
extern int pptp_ctrlconn_hangup(PPTP_INFO *pInfo);
extern int pptp_gre_open(PPTP_INFO *pInfo);

int pptp_ctrlconn_process(PPTP_INFO *pInfo);

int pptp_ctrlconn_send(PPTP_INFO *pInfo, void *packet, size_t len)
{
	int n;
	
	if (pptp_tcp_FIN_RST)
		return 0;
	
	n = write(pInfo->ctrl_sock, packet, len);
	if (n < 0)
	{
		if (errno != EAGAIN && errno != EINTR)
		{
			pInfo->ctrlconn_state = PPTP_CTRLCONN_STATE_DESTROY;
		}
		//return 0;
	}
	
	return 0;
}

int pptp_ctrlconn_call(PPTP_INFO *pInfo)
{
	struct pptp_ctrl_conn *ctrl_conn = pInfo->ctrl_conn;
	struct pptp_call *call;
	struct pptpOutCallRequest request;
	
	call = (struct pptp_call *)malloc(sizeof(struct pptp_call));
	if (call == NULL)
	{
		diag_printf("pptp_ctrlconn_call: malloc failed\n");
		return -1;
	}
	
	call->call_id = 0;
	call->call_sn = ctrl_conn->call_sn++;
	pInfo->call_state = PPTP_CALL_STATE_IDLE;
	
	memset(&request, 0, sizeof(request));
	request.header.length = htons(sizeof(struct pptpOutCallRequest));
	request.header.msgType = htons(PPTP_CTRL_MSG_TYPE);
	request.header.magic = htonl(PPTP_MAGIC);
	request.header.type = htons(PPTP_OutCallRequest);
	//request.header.resv0 = 0;
	request.cid = call->call_id;
	request.serno = call->call_sn;
	request.minBps = htonl(PPTP_MINBPS);
	request.maxBps = htonl(PPTP_MAXBPS);
	request.bearType = htonl(PPTP_BEARCAP_ANALOG);
	request.frameType = htonl(PPTP_FRAMECAP_ASYNC);
	request.recvWin = htons(PPTP_RECV_WIN);
	
	//if (pptp_ctrlconn_send(pInfo, &request, sizeof(request)) == 0)
	//{//pptp_ctrlconn_send always return 0
		pptp_ctrlconn_send(pInfo, &request, sizeof(request));
		pInfo->call_state = PPTP_CALL_STATE_WAITREPLY;
		pptp_timer_reset(pInfo);
		ctrl_conn->call = call;
		return 0;
	//}
	/*else
	{
		free(call);
		return -1;
	}*/
}


int pptp_ctrlconn_call_clear(PPTP_INFO *pInfo)
{
	struct pptp_ctrl_conn *ctrl_conn = pInfo->ctrl_conn;
	struct pptp_call *call = ctrl_conn->call;
	struct pptpCallClearRequest request;
	
	if (pInfo->call_state == PPTP_CALL_STATE_IDLE)
		return 0;
	
	request.header.length = htons(sizeof(struct pptpCallClearRequest));
	request.header.msgType = htons(PPTP_CTRL_MSG_TYPE);
	request.header.magic = htonl(PPTP_MAGIC);
	request.header.type = htons(PPTP_CallClearRequest);
	request.header.resv0 = 0;
	request.cid = htons(call->call_id);
	request.resv0 = 0;
	
	pptp_ctrlconn_send(pInfo, &request, sizeof(request));
	pptp_timer_reset(pInfo);
	pInfo->call_state = PPTP_CALL_STATE_WAITDISC;
	return 0;
}

int pptp_ctrlconn_call_release(PPTP_INFO *pInfo)
{
	struct pptp_ctrl_conn *ctrl_conn = pInfo->ctrl_conn;
	
	free(ctrl_conn->call);
	ctrl_conn->call = 0;
	return 0;
}


int pptp_ctrlconn_keepalive(PPTP_INFO *pInfo)
{
	struct pptpEchoRequest request;
	
	request.header.length = htons(sizeof(struct pptpEchoRequest));
	request.header.msgType = htons(PPTP_CTRL_MSG_TYPE);
	request.header.magic = htonl(PPTP_MAGIC);
	request.header.type = htons(PPTP_EchoRequest);
	request.header.resv0 = 0;
	request.id = pInfo->ctrl_conn->keepalive_id;
	
	//if (pptp_ctrlconn_send(pInfo, &request, sizeof(request)) == 0)
	//pptp_ctrlconn_send always return 0
	pptp_ctrlconn_send(pInfo, &request, sizeof(request));
		pInfo->ctrl_conn->keepalive_sent = 1;
	
	return 0;
}

int pptp_ctrlconn_connect(PPTP_INFO *pInfo)
{
	struct pptp_param *param = &pInfo->param;
	struct pptp_ctrl_conn *ctrl_conn;
	struct hostent *hostinfo;
	struct sockaddr_in peer;
	int error;
	struct pptpStartCtrlConnRequest request;
	
	/* connect to peer */
	if (param->fqdn)
	{
		hostinfo = gethostbyname(param->server_name);
		if (hostinfo && (hostinfo->h_length == 4))
		{
			memcpy(&param->server_ip, hostinfo->h_addr_list[0], 4);
		}
		else
		{
			diag_printf("PPTP: error looking up %s\n", param->server_name);
			memset(&param->server_ip, 0, 4);
			return -1;
		}
	}
	
	if ((pInfo->ctrl_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		diag_printf("PPTP_CTRLCONN: socket failed (%s)\n", strerror(errno));
		return -1;
	}
	
	peer.sin_family = AF_INET;
	peer.sin_port = htons(PPTP_PORT);
//	peer.sin_addr.s_addr = htonl(param->server_ip);
	peer.sin_addr.s_addr = param->server_ip;

	if ((error = connect(pInfo->ctrl_sock, (struct sockaddr *)&peer, sizeof(peer))) < 0)
	{
		if (error != EWOULDBLOCK)
		{
			diag_printf("PPTP_CTRLCONN: connect failed (%s)\n", strerror(errno));
			goto err_out;
		}
	}
	
	if ((ctrl_conn = (struct pptp_ctrl_conn *)malloc(sizeof(struct pptp_ctrl_conn))) == NULL)
	{
		goto err_out;
	}
	
	memset(ctrl_conn, 0, sizeof(*ctrl_conn));
	ctrl_conn->keepalive_id = 1;
	if (strlen(param->_hostname) >= 64)
		strncpy(ctrl_conn->host, param->_hostname, sizeof(ctrl_conn->host)-1);
	else
		strcpy(ctrl_conn->host, param->_hostname);
	memset(&request, 0, sizeof(request));
	request.header.length = htons(sizeof(struct pptpStartCtrlConnRequest));
	request.header.msgType = htons(PPTP_CTRL_MSG_TYPE);
	request.header.magic = htonl(PPTP_MAGIC);
	request.header.type = htons(PPTP_StartControlConnRequest);
	//request.header.resv0 = 0;
	request.vers = htons(PPTP_PROTO_VERS);
	//request.resv0 = 0;
	request.frameCap = PPTP_FRAMECAP_ASYNC;
	request.bearCap = PPTP_BEARCAP_ANALOG;
	//request.maxChan = 0;
	//request.firmware = 0;
	//memset(request.host, 0, sizeof(request.host));
	memcpy(request.host, ctrl_conn->host, sizeof(ctrl_conn->host));
	//memset(request.vendor, 0, sizeof(request.vendor));
	
	pInfo->ctrl_conn = ctrl_conn;
	pInfo->ctrlconn_state = PPTP_CTRLCONN_STATE_IDLE;
	
	//if (pptp_ctrlconn_send(pInfo, &request, sizeof(request)) == 0)
	//{
	//pptp_ctrlconn_send always return 0
		pptp_ctrlconn_send(pInfo, &request, sizeof(request));
		pInfo->ctrlconn_state = PPTP_CTRLCONN_STATE_WAITSTARTREPLY;
		pptp_timer_reset(pInfo);
	//}
	/*else
	{
		free(ctrl_conn);
		pInfo->ctrl_conn = NULL;
		goto err_out;
	}*/
	
	pInfo->state = PPTP_STATE_CONNECT;
	
	return 0;
	
err_out:
	close(pInfo->ctrl_sock);
	pInfo->ctrl_sock = -1;
	return -1;
}

int pptp_ctrlconn_stop(PPTP_INFO *pInfo, u_int8_t reason)
{
	struct pptp_ctrl_conn *ctrl_conn = pInfo->ctrl_conn;
	struct pptpStopCtrlConnRequest request;
	
	request.header.length = htons(sizeof(struct pptpStopCtrlConnRequest));
	request.header.msgType = htons(PPTP_CTRL_MSG_TYPE);
	request.header.magic = htonl(PPTP_MAGIC);
	request.header.type = htons(PPTP_StopControlConnRequest);
	request.header.resv0 = 0;
	
	request.reason = reason;
	request.resv0 = 0;
	request.resv1 = 0;
	
	if (pInfo->ctrlconn_state == PPTP_CTRLCONN_STATE_IDLE ||
		pInfo->ctrlconn_state == PPTP_CTRLCONN_STATE_WAITSTOPREPLY)
	{
		return 0;
	}
	
	if (ctrl_conn->call)
		pptp_ctrlconn_call_clear(pInfo);
	
	pptp_ctrlconn_send(pInfo, &request, sizeof(request));
	
	pptp_timer_reset(pInfo);
	pInfo->ctrlconn_state = PPTP_CTRLCONN_STATE_WAITSTOPREPLY;
	
	return 0;
}

int pptp_ctrlconn_disconnect(PPTP_INFO *pInfo)
{
	struct pptp_ctrl_conn *ctrl_conn = pInfo->ctrl_conn;
	int s;

	if (!pInfo->ctrl_conn)
		return 0;

	s = splimp();
	pptp_ctrlconn_stop(pInfo, PPTP_SCCR_REAS_LOCAL);
	
	if (ctrl_conn->call)
		pptp_ctrlconn_call_release(pInfo);

	if (ctrl_conn)	{
		free(ctrl_conn);
		pInfo->ctrl_conn = NULL;
	}

	if (pInfo->ctrl_sock != -1) {		
		close(pInfo->ctrl_sock);
		pInfo->ctrl_sock = -1;
	}
	
	pptp_timer_stop(pInfo);

	splx(s);
	return 0;
}

int pptp_ctrlconn_connected(PPTP_INFO *pInfo)
{
	struct pptp_ctrl_conn *ctrl_conn = pInfo->ctrl_conn;
	
	if (!ctrl_conn->call)
		return 0;
	
	return (pInfo->call_state == PPTP_CALL_STATE_CONNECTED);
}


int pptp_ctrlconn_read(PPTP_INFO *pInfo)
{
	struct pptp_ctrl_conn *ctrl_conn = pInfo->ctrl_conn;
	int n;
	
	if (pptp_tcp_FIN_RST)
		return 0;
	
	if (ctrl_conn->read_len == PPTP_RX_BUFSIZ)
	{
		diag_printf("PPTP_CTRLCONN: buffer full\n");
		return 0;
	}
	
	n = read(pInfo->ctrl_sock, pptp_read_buf+ctrl_conn->read_len, PPTP_RX_BUFSIZ-ctrl_conn->read_len);
	if (n <= 0)
	{
		/* == 0, may caused by FIN, < 0 may caused by RST */
		diag_printf("PPTP_CTRLCONN: TCP session reset or closed\n");
		mon_snd_cmd(MON_CMD_WAN_UPDATE);
		pptp_tcp_FIN_RST = 1;
		mon_snd_cmd(MON_CMD_WAN_UPDATE);
		return 0;
	}
	
	ctrl_conn->read_len += n;
	
	return n;
}


//use mapping table to replace swtich case code to reduce code size.
//when any pptp control message structure changed,must be adjust related element in pptp_ctrlmsg_len_table[] 
static char pptp_ctrlmsg_len_table[16]={0,156,156,16,16,16,20,168,32,220,\
	24,28,16,148,40,24};
static unsigned char pptp_ctrlmsg_len(u_int16_t type)
{
	unsigned char  msg_len = 0;
	
	if(type<16)
		msg_len = pptp_ctrlmsg_len_table[type];

	return msg_len;
}

/*
static int pptp_ctrlmsg_len(u_int16_t type)
{
	int msg_len = 0;

		//for diag_pritnf msg_len to generate table:pptp_ctrlmsg_len_table[].
		msg_len = sizeof(struct pptpStartCtrlConnRequest);
		diag_printf("\n[1:%d]\n",msg_len);

		msg_len = sizeof(struct pptpStartCtrlConnReply);
		diag_printf("\n[2:%d]\n",msg_len);

		msg_len = sizeof(struct pptpStopCtrlConnRequest);
		diag_printf("\n[3:%d]\n",msg_len);

		msg_len = sizeof(struct pptpStopCtrlConnReply);
		diag_printf("\n[4:%d]\n",msg_len);

		msg_len = sizeof(struct pptpEchoRequest);
		diag_printf("\n[5:%d]\n",msg_len);

		msg_len = sizeof(struct pptpEchoReply);
		diag_printf("\n[6:%d]\n",msg_len);

		msg_len = sizeof(struct pptpOutCallRequest);
		diag_printf("\n[7:%d]\n",msg_len);

		msg_len = sizeof(struct pptpOutCallReply);
		diag_printf("\n[8:%d]\n",msg_len);

		msg_len = sizeof(struct pptpInCallRequest);
		diag_printf("\n[9:%d]\n",msg_len);

		msg_len = sizeof(struct pptpInCallReply);
		diag_printf("\n[10:%d]\n",msg_len);

		msg_len = sizeof(struct pptpInCallConn);
		diag_printf("\n[11:%d]\n",msg_len);

		msg_len = sizeof(struct pptpCallClearRequest);
		diag_printf("\n[12:%d]\n",msg_len);

		msg_len = sizeof(struct pptpCallDiscNotify);
		diag_printf("\n[13:%d]\n",msg_len);

		msg_len = sizeof(struct pptpWanErrorNotify);
		diag_printf("\n[14:%d]\n",msg_len);

		msg_len = sizeof(struct pptpSetLinkInfo);
		diag_printf("\n[15:%d]\n",msg_len);	
	
	switch (type)
	{
	case PPTP_StartControlConnRequest:
		msg_len = sizeof(struct pptpStartCtrlConnRequest);
		break;
	case PPTP_StartControlConnReply:
		msg_len = sizeof(struct pptpStartCtrlConnReply);
		break;
	case PPTP_StopControlConnRequest:
		msg_len = sizeof(struct pptpStopCtrlConnRequest);
		break;
	case PPTP_StopControlConnReply:
		msg_len = sizeof(struct pptpStopCtrlConnReply);
		break;
	case PPTP_EchoRequest:
		msg_len = sizeof(struct pptpEchoRequest);
		break;
	case PPTP_EchoReply:
		msg_len = sizeof(struct pptpEchoReply);
		break;
	case PPTP_OutCallRequest:
		msg_len = sizeof(struct pptpOutCallRequest);
		break;
	case PPTP_OutCallReply:
		msg_len = sizeof(struct pptpOutCallReply);
		break;
	case PPTP_InCallRequest:
		msg_len = sizeof(struct pptpInCallRequest);
		break;
	case PPTP_InCallReply:
		msg_len = sizeof(struct pptpInCallReply);
		break;
	case PPTP_InCallConn:
		msg_len = sizeof(struct pptpInCallConn);
		break;
	case PPTP_CallClearRequest:
		msg_len = sizeof(struct pptpCallClearRequest);
		break;
	case PPTP_CallDiscNotify:
		msg_len = sizeof(struct pptpCallDiscNotify);
		break;
	case PPTP_WanErrorNotify:
		msg_len = sizeof(struct pptpWanErrorNotify);
		break;
	case PPTP_SetLinkInfo:
		msg_len = sizeof(struct pptpSetLinkInfo);
		break;
	default:
		break;
	}
	
	return msg_len;
}
*/
int pptp_ctrlconn_parse(PPTP_INFO *pInfo, char **buf)
{
	struct pptp_ctrl_conn *ctrl_conn = pInfo->ctrl_conn;
	struct pptpMsgHead	*header;
	int len;
	int skip_bytes = 0;
	
	while ((ctrl_conn->read_len - skip_bytes) >= sizeof(struct pptpMsgHead))
	{
		header = (struct pptpMsgHead *)(pptp_read_buf+skip_bytes);
		if (ntohl(header->magic) != PPTP_MAGIC)
		{
			skip_bytes++;
			continue;
		}
		if (ntohs(header->resv0) != 0)
		{
			skip_bytes++;
			continue;
		}
		
		if (ntohs(header->length) < sizeof(struct pptpMsgHead))
		{
			skip_bytes++;
			continue;
		}
		if (ntohs(header->length) > (ctrl_conn->read_len - skip_bytes))
		{
			break;
		}
		if (ntohs(header->msgType) == PPTP_CTRL_MSG_TYPE)
		{
			int msg_len = pptp_ctrlmsg_len(ntohs(header->type));
			
			if (ntohs(header->length) != msg_len)
			{
				skip_bytes++;
				continue;
			}
		}
		
		len = ntohs(header->length);
		*buf = (char *)malloc(len);
		if (*buf == NULL)
		{
			diag_printf("pptp_ctrlconn_parse: malloc failed\n");
			return 0;
		}
		
		memcpy(*buf, pptp_read_buf+skip_bytes, len);
		ctrl_conn->read_len -= (skip_bytes+len);
		memmove(pptp_read_buf, pptp_read_buf+skip_bytes+len, ctrl_conn->read_len);
		return len;
	}
	
	ctrl_conn->read_len -= skip_bytes;
	memmove(pptp_read_buf, pptp_read_buf+skip_bytes, ctrl_conn->read_len);
	
	return 0;
}

static u_int8_t pptp_handle_StartControlConnReply(PPTP_INFO *pInfo, struct pptpStartCtrlConnReply *reply)
{
	struct pptp_ctrl_conn *ctrl_conn = pInfo->ctrl_conn;
	
	if (pInfo->ctrlconn_state != PPTP_CTRLCONN_STATE_WAITSTARTREPLY)
		return 0;
	
	if (ntohs(reply->vers) != PPTP_PROTO_VERS)
		return PPTP_SCCR_REAS_PROTO;
	
	if (reply->result != 1)
		return PPTP_SCCR_REAS_PROTO;
	
	pInfo->ctrlconn_state = PPTP_CTRLCONN_STATE_CONNECTED;
	
	ctrl_conn->vers = ntohs(reply->vers);
	ctrl_conn->firmware = ntohs(reply->firmware);
	memcpy(ctrl_conn->host, reply->host, sizeof(ctrl_conn->host));
	memcpy(ctrl_conn->vendor, reply->vendor, sizeof(ctrl_conn->vendor));
	
	pptp_timer_reset(pInfo);
	
	return 0;
}

int pptp_handle_StopControlConnRequest(PPTP_INFO *pInfo, struct pptpStopCtrlConnRequest * request)
{
	struct pptpStopCtrlConnReply reply;
	
	if (pInfo->ctrlconn_state == PPTP_CTRLCONN_STATE_IDLE)
		return 0;
	
	reply.header.length = htons(sizeof(struct pptpStopCtrlConnReply));
	reply.header.msgType = htons(PPTP_CTRL_MSG_TYPE);
	reply.header.magic = htonl(PPTP_MAGIC);
	reply.header.type = htons(PPTP_StopControlConnReply);
	reply.header.resv0 = 0;
	reply.result = 1;
	reply.err = 0;
	reply.resv0 = 0;
	//if (pptp_ctrlconn_send(pInfo, &reply, sizeof(reply)) == 0)
		////pptp_ctrlconn_send always return 0
		pptp_ctrlconn_send(pInfo, &reply, sizeof(reply)); 
		pInfo->ctrlconn_state = PPTP_CTRLCONN_STATE_DESTROY;
	
	return 0;
}

int pptp_handle_StopControlConnReply(PPTP_INFO *pInfo, struct pptpStopCtrlConnReply *reply)
{
	if (pInfo->ctrlconn_state == PPTP_CTRLCONN_STATE_IDLE)
		return 0;
	
	pInfo->ctrlconn_state = PPTP_CTRLCONN_STATE_DESTROY;
	
	return 0;
}

int pptp_handle_EchoRequest(PPTP_INFO *pInfo, struct pptpEchoRequest *request)
{
	struct pptpEchoReply reply;
	
	memset(&reply,0,sizeof(reply));
	reply.header.length = htons(sizeof(struct pptpEchoReply));
	reply.header.msgType = htons(PPTP_CTRL_MSG_TYPE);
	reply.header.magic = htonl(PPTP_MAGIC);
	reply.header.type = htons(PPTP_EchoReply);
	//reply.header.resv0 = 0;
	reply.id = request->id;
	reply.result = 1;
	//reply.err = 0;
	//reply.resv0 = 0;
	
	pptp_ctrlconn_send(pInfo, &reply, sizeof(reply));
	
	pptp_timer_reset(pInfo);
	return 0;
}

int pptp_handle_EchoReply(PPTP_INFO *pInfo, struct pptpEchoReply *reply)
{
	struct pptp_ctrl_conn *ctrl_conn = pInfo->ctrl_conn;
	
	if (ctrl_conn->keepalive_sent &&
		(reply->id == ctrl_conn->keepalive_id))
	{
		ctrl_conn->keepalive_id++;
		ctrl_conn->keepalive_sent = 0;
		pptp_timer_reset(pInfo);
	}
	
	return 0;
}

int pptp_handle_OutCallReply(PPTP_INFO *pInfo, struct pptpOutCallReply *reply)
{
	struct pptp_ctrl_conn *ctrl_conn = pInfo->ctrl_conn;
	struct pptp_call *call = ctrl_conn->call;
	struct pptpSetLinkInfo link_info;
	
	if (ntohs(reply->peerCid) != call->call_id)
		return 0;
	
	if (pInfo->call_state != PPTP_CALL_STATE_WAITREPLY)
		return 0;
	
	if (reply->result != 1)
	{
		pptp_ctrlconn_call_release(pInfo);
		pInfo->call_state = PPTP_CALL_STATE_IDLE;
	}
	else
	{
		link_info.header.length = htons(sizeof(struct pptpSetLinkInfo));
		link_info.header.msgType = htons(PPTP_CTRL_MSG_TYPE);
		link_info.header.magic = htonl(PPTP_MAGIC);
		link_info.header.type = htons(PPTP_SetLinkInfo);
		link_info.header.resv0 = 0;
		link_info.cid = reply->cid;
		link_info.resv0 = 0;
		link_info.sendAccm = 0xffffffff;
		link_info.recvAccm = 0xffffffff;
		
		pptp_ctrlconn_send(pInfo, &link_info, sizeof(link_info));
		
		pInfo->call_state = PPTP_CALL_STATE_CONNECTED;
		//call->peer_call_id = ntohs(reply->cid);
		call->call_id = ntohs(reply->cid);	// XXX
		call->speed = ntohl(reply->speed);
		pptp_timer_reset(pInfo);
	}
	return 0;
}

int pptp_handle_CallClearRequest(PPTP_INFO *pInfo, struct pptpCallClearRequest *request)
{
	struct pptp_ctrl_conn *ctrl_conn = pInfo->ctrl_conn;
	struct pptp_call *call = ctrl_conn->call;
	struct pptpCallDiscNotify notify;
	
	if (call->call_id == ntohs(request->cid))
	{
		if (pptp_ctrlconn_hangup(pInfo) != 0)
			return 0;
		
		memset(&notify, 0, sizeof(notify));
		notify.header.length = htons(sizeof(struct pptpCallDiscNotify));
		notify.header.msgType = htons(PPTP_CTRL_MSG_TYPE);
		notify.header.magic = htonl(PPTP_MAGIC);
		notify.header.type = htons(PPTP_CallDiscNotify);
		//notify.header.resv0 = 0;
		notify.cid = request->cid;
		notify.result = 1;
		
		pptp_ctrlconn_send(pInfo, &notify, sizeof(notify));
		pptp_ctrlconn_call_release(pInfo);
	}
	return 0;
}

int pptp_handle_CallDiscNotify(PPTP_INFO *pInfo, struct pptpCallDiscNotify *notify)
{
	struct pptp_ctrl_conn *ctrl_conn = pInfo->ctrl_conn;
	struct pptp_call *call = ctrl_conn->call;
	
	if (call->call_id == ntohs(notify->cid))
	{
		if (pInfo->call_state == PPTP_CALL_STATE_WAITDISC)
			pptp_ctrlconn_call_release(pInfo);
		else
			mon_snd_cmd(MON_CMD_WAN_UPDATE);
	}
	return 0;
}

int pptp_handle_SetLinkInfo(PPTP_INFO *pInfo, struct pptpSetLinkInfo *packet)
{
	struct pptp_ctrl_conn *ctrl_conn = pInfo->ctrl_conn;
	struct pptp_call *call = ctrl_conn->call;
	struct pptpSetLinkInfo linfo;
	
	if (ntohs(packet->cid) != call->peer_call_id)
		return 0;
	
	if (ntohl(packet->sendAccm) == 0 && ntohl(packet->recvAccm) == 0)
		return 0;

	linfo.header.length = htons(sizeof(struct pptpSetLinkInfo));
	linfo.header.msgType = htons(PPTP_CTRL_MSG_TYPE);
	linfo.header.magic = htonl(PPTP_MAGIC);
	linfo.header.type = htons(PPTP_SetLinkInfo);
	linfo.header.resv0 = 0;
	linfo.cid = htons(call->call_id);
	linfo.resv0 = 0;
	linfo.sendAccm = 0xffffffff;
	linfo.recvAccm = 0xffffffff;

	pptp_ctrlconn_send(pInfo, &linfo, sizeof(linfo));
	return 0;
}

////use mapping table to replace swtich case code to reduce code size.
static int (*pptp_ctrlconn_handler_table[16])(PPTP_INFO *,void*)=
	{
		NULL,// 0
		NULL, // 1
		pptp_handle_StartControlConnReply,// 2
		pptp_handle_StopControlConnRequest,// 3
		pptp_handle_StopControlConnReply,// 4
		pptp_handle_EchoRequest,// 5
		pptp_handle_EchoReply,// 6
		NULL,// 7
		pptp_handle_OutCallReply,// 8
		NULL,// 9
		NULL,// 10
		NULL,// 11
		pptp_handle_CallClearRequest,// 12
		pptp_handle_CallDiscNotify,// 13
		NULL,// 14
		pptp_handle_SetLinkInfo// 15
	};
int pptp_ctrlconn_handler(PPTP_INFO *pInfo, char *buf, int len)
{
	struct pptpMsgHead *header = (struct pptpMsgHead *)buf;
	u_int8_t stop_reason = 0;
	u_int8_t index=0;
	
	if (ntohs(header->msgType) != PPTP_CTRL_MSG_TYPE)
	{
		diag_printf("msgType=%d\n", ntohs(header->msgType));
		return -1;
	}
	
	index = ntohs(header->type);
	if(pptp_ctrlconn_handler_table[index]!=NULL)
		stop_reason = (*pptp_ctrlconn_handler_table[index])(pInfo,buf);
	/*
	switch (ntohs(header->type))
	{
	case PPTP_StartControlConnReply:
		stop_reason = pptp_handle_StartControlConnReply(pInfo, (struct pptpStartCtrlConnReply *)buf);
		break;
	case PPTP_StopControlConnRequest:
		stop_reason = pptp_handle_StopControlConnRequest(pInfo, (struct pptpStopCtrlConnRequest *)buf);
		break;
	case PPTP_StopControlConnReply:
		stop_reason = pptp_handle_StopControlConnReply(pInfo, (struct pptpStopCtrlConnReply *)buf);
		break;
	case PPTP_EchoRequest:
		stop_reason = pptp_handle_EchoRequest(pInfo, (struct pptpEchoRequest *)buf);
		break;
	case PPTP_EchoReply:
		stop_reason = pptp_handle_EchoReply(pInfo, (struct pptpEchoReply *)buf);
		break;
	case PPTP_OutCallReply:
		stop_reason = pptp_handle_OutCallReply(pInfo, (struct pptpOutCallReply *)buf);
		break;
	case PPTP_CallClearRequest:
		stop_reason = pptp_handle_CallClearRequest(pInfo, (struct pptpCallClearRequest *)buf);
		break;
	case PPTP_CallDiscNotify:
		stop_reason = pptp_handle_CallDiscNotify(pInfo, (struct pptpCallDiscNotify *)buf);
		break;
	case PPTP_SetLinkInfo:
		stop_reason = pptp_handle_SetLinkInfo(pInfo, (struct pptpSetLinkInfo *)buf);
		break;
	default:
		break;
	}
	*/
	if (stop_reason)
	{
		pptp_ctrlconn_stop(pInfo, stop_reason);
		return -1;
	}
	
	return 0;
}

int pptp_ctrlconn_process(PPTP_INFO *pInfo)
{
	char *buf = NULL;
	int len;
		
	pptp_ctrlconn_read(pInfo);
	
	while ((len = pptp_ctrlconn_parse(pInfo, &buf)) != 0)
	{
		pptp_ctrlconn_handler(pInfo, buf, len);
		free(buf);
		buf = NULL;
	}
	if(buf)
		free(buf);
	return 0;
}

int pptp_ctrlconn_open(PPTP_INFO *pInfo)
{
	struct pptp_cmd *cmd;
	fd_set fdset;
	int max_fd;
	struct timeval tv = {1, 0};
	int n;
	int first = 1;
	
	if (pptp_ctrlconn_connect(pInfo) != 0)
		return -1;
	
	while (!pptp_ctrlconn_connected(pInfo))
	{
		if ((cmd = (struct pptp_cmd *)cyg_mbox_tryget(pInfo->msgq_id)) != 0)
		{
			switch (cmd->op)
			{
			case PPTP_CMD_STOP:
				pptp_ctrlconn_disconnect(pInfo);
				pInfo->state = PPTP_STATE_STOP;
				return -1;
			case PPTP_CMD_HANGUP:
				pptp_ctrlconn_disconnect(pInfo);
				if (pptp_ctrlconn_connect(pInfo) != 0)
					return -1;
				break;
			case PPTP_CMD_TIMER:
				pptp_timer_action(pInfo);
				break;
			default:
				break;
			}
		}
		
		FD_ZERO(&fdset);
		FD_SET(pInfo->ctrl_sock, &fdset);
		max_fd = pInfo->ctrl_sock + 1;
		
		n = select(max_fd, &fdset, 0, 0, &tv);
		if (n < 0)
		{
			pptp_ctrlconn_disconnect(pInfo);
			pInfo->state = PPTP_STATE_STOP;
			return -1;
		}
		if (n == 0)
			continue;
		
		if (FD_ISSET(pInfo->ctrl_sock, &fdset))
		{
			pptp_ctrlconn_process(pInfo);
			FD_CLR(pInfo->ctrl_sock, &fdset);
		}
		
		if (first && pInfo->ctrlconn_state == PPTP_CTRLCONN_STATE_CONNECTED)
		{
			if (pptp_ctrlconn_call(pInfo) == 0)
				first = 0;
		}
	}
	
	pInfo->call_id = pInfo->ctrl_conn->call->call_id;
	pInfo->peer_call_id = pInfo->ctrl_conn->call->peer_call_id;
	
	if (pptp_gre_open(pInfo) != 0)
	{
		pptp_ctrlconn_disconnect(pInfo);
		return -1;
	}
	pInfo->state = PPTP_STATE_GRE;
	
	if (PppEth_Start(&pInfo->if_ppp) < 0)
		return -1;
	pInfo->task_flag |= PPTP_TASKFLAG_PPP;
	pInfo->state = PPTP_STATE_PPP;
	
	return 0;
}


