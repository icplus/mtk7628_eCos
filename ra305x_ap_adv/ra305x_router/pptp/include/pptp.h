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
    pptp.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/
#ifndef __PPTP_H__
#define __PPTP_H__

#include <sys_inc.h>
#include <pppd.h>
#include <ppp_api.h>
#include <pptp_protocol.h>

//**********************
// message queue
//**********************
enum
{
	PPTP_CMD_STOP = 1,  // stop command from upper layer
	PPTP_CMD_HANGUP,    // stop command from lower layer
	PPTP_CMD_TIMER
};

struct pptp_cmd
{
	int op;
};

enum
{
	PPTP_CONNECT_KEEPALAIVE = 0,
	PPTP_CONNECT_AUTO = 1,
	PPTP_CONNECT_MANUAL = 2,
};

#ifndef IFNAMSIZ
#define IFNAMSIZ	16
#endif

struct pptp_param
{
	int unit;
    char ppp_name[IFNAMSIZ];
    char eth_name[IFNAMSIZ];
    char username[256];
    char password[256];
    u_long my_ip;
    u_long server_ip;
    u_long netmask;
    int idle_time;
    int connect_mode;
    int mtu;
	int	mppe;
	int	dflrt;
	int fqdn;
	char server_name[256];
	char _hostname[256];
};


struct pptp_call
{
	u_int16_t	call_id;
	u_int16_t	peer_call_id;
	u_int16_t	call_sn;
	u_int32_t	speed;
};

#define PPTP_RX_BUFSIZ	1600//(2*1024)

struct pptp_ctrl_conn
{
	u_int32_t keepalive_sent;
	u_int32_t keepalive_id;
	
    u_int16_t vers;
    u_int16_t firmware;
    u_int8_t  host[PPTP_HOSTNAME_LEN];
	u_int8_t  vendor[PPTP_VENDOR_LEN];
	
	struct pptp_call *call;
	u_int16_t call_sn;
	
	size_t read_len;
};


enum
{
	PPTP_STATE_INIT = 0,
	PPTP_STATE_CONNECT,
	PPTP_STATE_PPP,
	PPTP_STATE_GRE,
	PPTP_STATE_CLOSE,
	PPTP_STATE_STOP
};

enum
{
	PPTP_CTRLCONN_STATE_IDLE,
	PPTP_CTRLCONN_STATE_WAITSTARTREPLY,
	PPTP_CTRLCONN_STATE_WAITSTOPREPLY,
	PPTP_CTRLCONN_STATE_CONNECTED,
	PPTP_CTRLCONN_STATE_DESTROY,
};

enum
{
	PPTP_CALL_STATE_IDLE,
	PPTP_CALL_STATE_WAITREPLY,
	PPTP_CALL_STATE_CONNECTED,
	PPTP_CALL_STATE_WAITDISC,
};

#define PPTP_TASKFLAG_PPP	0x01

typedef struct PPTP_INFO_S
{
	PPP_IF_VAR_T if_ppp;			/* common ppp structures */
#define t_unit				if_ppp.unit		/* PPP unit number */
#define t_ifname			if_ppp.ifname	/* ethernet layer interfcae name */
#define t_pppname			if_ppp.pppname	/* PPP layer interfcae name */
#define t_type				if_ppp.encapsulate_type	/* type of interface device */
#define t_user				if_ppp.user		/* authentication username */
#define t_password			if_ppp.passwd	/* password for PAP  */
#define t_idle				if_ppp.idle		/* idle time */
#define t_mtu				if_ppp.mtu		/* MTU that we allow */
#define t_mru				if_ppp.mru		/* MRU that we allow */
#define t_mppe				if_ppp.mppe		/* MPPE option */

#define t_defaultroute		if_ppp.defaultroute	/* flag to specify whether this interfce is defaultroute */
#define t_pOutputHook		if_ppp.pOutput		/* output packet function pointer */
#define t_pIfStopHook		if_ppp.pIfStopHook	/* functon called by PPP daemon to close pppoe interface */
#define t_pIfReconnectHook	if_ppp.pIfReconnectHook	/* physical layer reconnection hook routine */
	struct ifnet *eth_ifp;
	struct pptp_ctrl_conn *ctrl_conn;
	
	int activated;
	int state;
	int ctrlconn_state;
	int call_state;
	int task_flag;
	cyg_handle_t msgq_id;
	struct pptp_param param;
	int gre_sock;
	int ctrl_sock;
	
	/* PPTP_GRE */
	u_int16_t call_id;
	u_int16_t peer_call_id;
	u_int32_t ack_sent;
	u_int32_t ack_recv;
	u_int32_t seq_sent;
	u_int32_t seq_recv;
	
}
PPTP_INFO;


struct pptp_buf
{
	PPTP_INFO *info;
	struct mbuf *m;
};
#define PPTP_TXQ_SIZE 512


#define PPTP_EVENT_TX		0x01
#define PPTP_EVENT_QUIT		0x02
#define PPTP_EVENT_ALL		(PPTP_EVENT_TX|PPTP_EVENT_QUIT)

#endif	/* __PPTP_H__ */
