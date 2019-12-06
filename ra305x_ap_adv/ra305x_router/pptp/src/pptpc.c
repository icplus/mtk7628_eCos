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
    pptpc.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#include <config.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pptp.h>
#include <cfg_def.h>
#include <cfg_net.h>
#include <eventlog.h>
#include <sys_status.h>
#include <stdio.h>

#include <pkgconf/system.h>
#include <pkgconf/net.h>

#include <network.h>
#include <cfg_dns.h>
#include <eventlog.h>

#define PPTP_PPP_NUM_SESSIONS	1

static PPTP_INFO pptp_softc[PPTP_PPP_NUM_SESSIONS];

static int manual_flag = 0;
static int force_on = 0;

/* thread data */
#define PPTP_TASK_PRIORITY		6
#define PPTP_STACK_SIZE			4096

//static char pptp_thread_stack[PPTP_STACK_SIZE];
static char *pptp_thread_stack=NULL;
static cyg_thread pptp_thread_obj;
static cyg_handle_t	pptp_thread_handle;

//static char pptp_tx_thread_stack[PPTP_STACK_SIZE];
static char *pptp_tx_thread_stack=NULL;
static cyg_thread pptp_tx_thread_obj;
static cyg_handle_t	pptp_tx_thread_handle;


struct pptp_buf pptp_tx_queue[PPTP_TXQ_SIZE];
int pptp_txq_head;
int pptp_txq_tail;
int pptp_tx_process_stop;
cyg_flag_t pptp_event_flags;
//cyg_mutex_t pptp_mutex;

cyg_mbox pptp_msgq;

#ifdef	CONFIG_PPTP_PPPOE

typedef struct ppp_ctx_s {
	struct ip_set *pipset;
	cyg_uint16 ipset_flags;
	cyg_uint8 state;
	cyg_uint8 staticip;
	cyg_uint32 conn_mode;
	cyg_uint32 prev_dns[2];
} ppp_ctx_t;

//int pptp_vpn_phase = 0;/*{0:init; 1:connecting; 2:connected; 5:disconnecting; 6 disconnected}*/
ppp_ctx_t vpn_ppp_ctl= {pipset: NULL, state: PPP_CHK_SRV};

struct ip_set ppp_ip_set[CONFIG_PPP_NUM_SESSIONS];
//extern struct ip_set wan_ip_set;
extern void *ppp_ifnet_pptp_vpn;
#endif
/* function prototypes */
int inet_IfIpAddr(UINT32 *pIpAddr, char *pIfName);

extern int (*dialup_checkp) __P((void));
extern int pptp_tcp_FIN_RST;

extern int pptp_ctrlconn_disconnect(PPTP_INFO *pInfo);
extern int pptp_ctrlconn_open(PPTP_INFO *pInfo);
extern int pptp_ctrlconn_stop(PPTP_INFO *pInfo, u_int8_t reason);
extern int pptp_ctrlconn_call_clear(PPTP_INFO *pInfo);
extern int pptp_ctrlconn_call_release(PPTP_INFO *pInfo);
extern int pptp_ctrlconn_process(PPTP_INFO *pInfo);
extern int pptp_ctrlconn_keepalive(PPTP_INFO *pInfo);

extern int pptp_gre_output(PPTP_INFO *pInfo, struct mbuf *m);
extern int pptp_gre_input(PPTP_INFO *pInfo);
extern int pptp_gre_encap(PPTP_INFO *pInfo, struct mbuf *m);

static int pptp_dialup_checkp(void);
static int pptp_ppp_hangup(PPTP_INFO *pInfo);
static int pptp_ppp_reconnect(PPTP_INFO *pInfo);
static int pptp_do_connect(int unit);

void pptp_timer_reset(PPTP_INFO *pInfo);

/* timer functions */
#define PPTP_TIMER_INTERVAL		(60*100)

void pptp_timer_action(PPTP_INFO *pInfo)
{
	if (pInfo->ctrlconn_state == PPTP_CTRLCONN_STATE_WAITSTOPREPLY)
	{
		pInfo->ctrlconn_state = PPTP_CTRLCONN_STATE_DESTROY;
	}
	else if (pInfo->ctrlconn_state != PPTP_CTRLCONN_STATE_CONNECTED)
		pptp_ctrlconn_stop(pInfo, PPTP_SCCR_REAS_NONE);
	
	if (pInfo->ctrl_conn->keepalive_sent)
		pptp_ctrlconn_stop(pInfo, PPTP_SCCR_REAS_NONE);
	else
		pptp_ctrlconn_keepalive(pInfo);
	
	if (pInfo->call_state == PPTP_CALL_STATE_WAITREPLY)
		pptp_ctrlconn_call_clear(pInfo);
	else if (pInfo->call_state == PPTP_CALL_STATE_WAITDISC)
		pptp_ctrlconn_call_release(pInfo);
}

static void pptp_timer_alarm(PPTP_INFO *pInfo)
{
	static struct pptp_cmd cmd;
	
	if (pInfo->msgq_id == 0)
		return;
	
	cmd.op = PPTP_CMD_TIMER;
	cyg_mbox_tryput(pInfo->msgq_id, &cmd);
	
	// schedule next alarm
	pptp_timer_reset(pInfo);
}


void pptp_timer_stop(PPTP_INFO *pInfo)
{
	untimeout(pptp_timer_alarm, (void *)pInfo);
}


void pptp_timer_reset(PPTP_INFO *pInfo)
{
	untimeout(pptp_timer_alarm, (void *)pInfo);
	timeout(pptp_timer_alarm, (void *)pInfo, PPTP_TIMER_INTERVAL);
}


int pptp_process(PPTP_INFO *pInfo)
{
	struct pptp_cmd *cmd;
	u_long my_ip = 0;
	fd_set fdset;
	int max_fd;
	struct timeval tv = {1, 0};
	int n;
	
	SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPTP, "pptp started");
	
	/* loop until the Ethernet interface gets its IP address */
	for (;;)
	{
		if ((cmd = (struct pptp_cmd *)cyg_mbox_tryget(pInfo->msgq_id)) != 0)
		{
			if (cmd->op == PPTP_CMD_STOP)
				goto pptp_exit;
		}
		
		if (inet_IfIpAddr(&my_ip, pInfo->t_ifname) == OK && my_ip != 0)
		{
			break;
		}
		
		cyg_thread_delay(50);
	}
	
	/* loop until control connection established */
	while (pptp_ctrlconn_open(pInfo) != 0)
	{
		if (pInfo->state == PPTP_STATE_STOP)
			goto pptp_exit;
		
		if ((cmd = (struct pptp_cmd *)cyg_mbox_tryget(pInfo->msgq_id)) != 0)
		{
			if (cmd->op == PPTP_CMD_STOP)
				goto pptp_exit;
		}
		
		cyg_thread_delay(25);
	}
	
	if (pInfo->ctrl_sock == -1 ||
		pInfo->gre_sock == -1)
	{
		diag_printf("ctrl_sock=%d, gre_sock=%d\n",
			pInfo->ctrl_sock,
			pInfo->gre_sock);
	}
	
	if (pInfo->ctrl_sock > pInfo->gre_sock)
		max_fd = pInfo->ctrl_sock + 1;
	else
		max_fd = pInfo->gre_sock + 1;
	
	for (;;)
	{
		if (pInfo->msgq_id == 0) {
			return 0;
		}
		/* handle message queue */
		if ((cmd = (struct pptp_cmd *)cyg_mbox_tryget(pInfo->msgq_id)) != 0)
		{
			switch (cmd->op)
			{
			case PPTP_CMD_STOP:
				/* user stop & no pppd */
				return 0;
			case PPTP_CMD_HANGUP:
				pptp_ctrlconn_disconnect(pInfo);
				goto pptp_exit;
			case PPTP_CMD_TIMER:
				pptp_timer_action(pInfo);
				break;
			default:
				break;
			}
		}
		
		/* handle ctrlconn and pptp_gre sockets */
		FD_ZERO(&fdset);
		FD_SET(pInfo->ctrl_sock, &fdset);
		FD_SET(pInfo->gre_sock, &fdset);
		
		n = select(max_fd, &fdset, 0, 0, &tv);
		if (n <= 0)
			continue;
		
		/* pptp_gre data */
		if (FD_ISSET(pInfo->gre_sock, &fdset))
		{
			pptp_gre_input(pInfo);
			FD_CLR(pInfo->gre_sock, &fdset);
		}
		
		/* pptp_ctrlconn data */
		if (FD_ISSET(pInfo->ctrl_sock, &fdset))
		{
			pptp_ctrlconn_process(pInfo);
			FD_CLR(pInfo->ctrl_sock, &fdset);
		}
	}
	
pptp_exit:
	while (pInfo->task_flag != 0)
		cyg_thread_delay(1);
	
	if (pInfo->gre_sock != -1)
	{
		close(pInfo->gre_sock);
		pInfo->gre_sock = -1;
	}
	
	if (pInfo->msgq_id != 0)
	{
		cyg_mbox_delete(pInfo->msgq_id);
		pInfo->msgq_id = 0;
	}
	
	pInfo->state = PPTP_STATE_CLOSE;
	
    pptp_tx_process_stop++;
    cyg_flag_setbits(&pptp_event_flags, PPTP_EVENT_QUIT);
	
	cyg_thread_delay(50);
	cyg_thread_exit();
	
	return 0;
}


int pptp_tx_process(void)
{
	cyg_flag_value_t events;

	while (1)
	{
		events = cyg_flag_wait(
					&pptp_event_flags,
					PPTP_EVENT_ALL,
					CYG_FLAG_WAITMODE_OR|CYG_FLAG_WAITMODE_CLR);
		if (events & PPTP_EVENT_TX)
		{
			while (pptp_txq_head != pptp_txq_tail)
			{
				pptp_gre_encap(
					pptp_tx_queue[pptp_txq_head].info,
					pptp_tx_queue[pptp_txq_head].m);
				if (++pptp_txq_head >= PPTP_TXQ_SIZE)
					pptp_txq_head = 0;
			}
		}
		
		if (events & PPTP_EVENT_QUIT)
			break;
	}
	
	while (pptp_txq_head != pptp_txq_tail)
	{
		m_freem(pptp_tx_queue[pptp_txq_head].m);
		if (++pptp_txq_head >= PPTP_TXQ_SIZE)
			pptp_txq_head = 0;
	}
	
	cyg_thread_exit();
	
	return 0;
}


int pptp_start(PPTP_INFO *pInfo)
{
	struct pptp_param *param = &pInfo->param;
	
	if (pInfo->activated != 0)
		return 0;
	
	pptp_txq_head = pptp_txq_tail = 0;
	pptp_tx_process_stop = 0;
	
	memset(pInfo, 0, (unsigned long)&((PPTP_INFO *)0)->param);
	
	if ((pInfo->eth_ifp = ifunit(param->eth_name)) == NULL)
	{
		SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPTP, "Cannot find %s", param->eth_name);
		return -1;
	}
	
	pInfo->t_unit = param->unit;
	
	strcpy(pInfo->t_ifname, param->eth_name);
	strcpy(pInfo->t_pppname, param->ppp_name);
	strcpy(pInfo->t_user, param->username);
	strcpy(pInfo->t_password, param->password);
	
	pInfo->t_type = PPP_ENCAP_ETHERNET;
	pInfo->t_idle = param->idle_time;
	pInfo->t_mtu = param->mtu;
	pInfo->t_mru = param->mtu;
#ifdef MPPE
	pInfo->t_mppe = param->mppe;
#endif
	
	pInfo->t_defaultroute = param->dflrt;
	pInfo->t_pOutputHook = pptp_gre_output;
	pInfo->t_pIfStopHook = pptp_ppp_hangup;
	pInfo->t_pIfReconnectHook = pptp_ppp_reconnect;
	pInfo->task_flag = 0;
	pInfo->state = PPTP_STATE_INIT;
	
	pInfo->gre_sock = -1;
	pInfo->ctrl_sock = -1;
	
	//cyg_mutex_init(&pptp_mutex);
	cyg_flag_init(&pptp_event_flags);
	cyg_mbox_create(&pInfo->msgq_id, &pptp_msgq);
	if(pptp_thread_stack)
		free(pptp_thread_stack);
	pptp_thread_stack =(char *)malloc(PPTP_STACK_SIZE);
	cyg_thread_create(
			PPTP_TASK_PRIORITY,
			pptp_process,
			(cyg_addrword_t)pInfo,
			"PPTP Client",
			(void *)pptp_thread_stack,
			PPTP_STACK_SIZE,
			&pptp_thread_handle,
			&pptp_thread_obj);
	if(pptp_tx_thread_stack)
		free(pptp_tx_thread_stack);
	pptp_tx_thread_stack =(char *)malloc(PPTP_STACK_SIZE);	
	
	cyg_thread_create(
			PPTP_TASK_PRIORITY,
			pptp_tx_process,
			(cyg_addrword_t)0,
			"PPTP Tx Process",
			(void *)pptp_tx_thread_stack,
			PPTP_STACK_SIZE,
			&pptp_tx_thread_handle,
			&pptp_tx_thread_obj);
	
	cyg_thread_resume(pptp_thread_handle);
	cyg_thread_resume(pptp_tx_thread_handle);
	
	pInfo->activated = 1;
	
	return 0;
}


int pptp_stop(PPTP_INFO *pInfo)
{
	static struct pptp_cmd cmd;

	if (pInfo->msgq_id == 0)
		return 0;
	
	pptp_timer_stop(pInfo);
	
	cmd.op = PPTP_CMD_STOP;
	cyg_mbox_tryput(pInfo->msgq_id, &cmd);
	
#if 1
	cyg_thread_delay(101);
	
	if (pInfo->gre_sock != -1)
	{
		close(pInfo->gre_sock);
		pInfo->gre_sock = -1;
	}
	
	if (pInfo->msgq_id)
	{
		cyg_mbox_delete(pInfo->msgq_id);
		pInfo->msgq_id = 0;
	}
	
	pInfo->state = PPTP_STATE_CLOSE;

    pptp_tx_process_stop++;
    cyg_flag_setbits(&pptp_event_flags, PPTP_EVENT_QUIT);

    cyg_thread_delay(101);
	
	cyg_thread_delete(pptp_thread_handle);
	if(pptp_thread_stack)
	{
		free(pptp_thread_stack);
		pptp_thread_stack=NULL;
	}
#endif
	
	return 0;
}


static int pptp_ppp_hangup(PPTP_INFO *pInfo)
{
	static struct pptp_cmd cmd;
	
	if (pInfo->msgq_id == 0)
		return -1;
	
	SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPTP, "PPTP stop");
	
	pptp_timer_stop(pInfo);
	
	cmd.op = PPTP_CMD_HANGUP;
	cyg_mbox_tryput(pInfo->msgq_id, &cmd);
	pInfo->task_flag &= ~PPTP_TASKFLAG_PPP;
	
	return 0;
}

static int pptp_ppp_reconnect(PPTP_INFO *pInfo)
{
	static struct pptp_cmd cmd;
	
	if (pInfo->msgq_id == 0 || pInfo->activated == 0)
		return -1;
	
	SysLog(LOG_USER|LOG_INFO|LOGM_PPTP, "PPTP reconnect");
	
	pptp_timer_stop(pInfo);
	
	cmd.op = PPTP_CMD_HANGUP;
	cyg_mbox_tryput(pInfo->msgq_id, &cmd);
	
	//cyg_mutex_lock(&pptp_mutex);
	//pptp_ctrlconn_disconnect(pInfo);
	//cyg_mutex_unlock(&pptp_mutex);
	if (pInfo->gre_sock != -1)
	{
		close(pInfo->gre_sock);
		pInfo->gre_sock = -1;
	}
	
	return 0;
}


int pptp_ctrlconn_hangup(PPTP_INFO *pInfo)
{
	static struct pptp_cmd cmd;
	
	if (pInfo->msgq_id == 0)
		return -1;
	
	cmd.op = PPTP_CMD_HANGUP;
	cyg_mbox_tryput(pInfo->msgq_id, &cmd);
	
	return 0;
}


static int pptp_init_param(int unit, PPTP_INFO *pInfo)
{
	struct pptp_param *param = &pInfo->param;
	
	//memset(param, 0, sizeof(*param));
	#ifdef	CONFIG_PPTP_PPPOE
	//diag_printf("sht:location 1\n");
	int pptp_wanif;
	CFG_get( CFG_PTP_WANIF, &pptp_wanif );
	if(pptp_wanif ==2)
	{	//diag_printf("sht:location 2\n");
		param->unit = unit+1;
		sprintf(param->ppp_name, "ppp%d", unit+1);
		strcpy(param->eth_name, primary_wan_ip_set[0].ifname);
		param->my_ip = primary_wan_ip_set[0].ip;
		param->netmask = primary_wan_ip_set[0].netmask;
	}
	else
	#endif	
	{
	param->unit = unit;
	sprintf(param->ppp_name, "ppp%d", unit);
	strcpy(param->eth_name, WAN_NAME);

	CFG_get(CFG_PTP_IP, &param->my_ip);
	CFG_get(CFG_PTP_MSK, &param->netmask);
		if (unit == 0)
		param->dflrt = 1;
	}

	CFG_get(CFG_PTP_SVR, &param->server_ip);
	CFG_get(CFG_PTP_USER, param->username);
	CFG_get(CFG_PTP_PASS, param->password);
	CFG_get(CFG_PTP_SVRD_SEL, &param->fqdn);
	if (param->fqdn)
	{
		CFG_get(CFG_PTP_SVRDM, param->server_name);
	}
	CFG_get(CFG_SYS_NAME, param->_hostname);
	
	CFG_get(CFG_PTP_IDLET, &param->idle_time);
	CFG_get(CFG_PTP_AUTO, &param->connect_mode);
	param->mtu = 1460;
	CFG_get(CFG_PTP_MTU, &param->mtu);
#ifdef MPPE
	CFG_get(CFG_PTP_MPPE_EN, &param->mppe);
#endif

	return 0;
}

static int pptp_init_connect_mode(PPTP_INFO *pInfo)
{
	if (manual_flag)
		pInfo->param.connect_mode = PPTP_CONNECT_MANUAL;
	
	switch (pInfo->param.connect_mode)
	{
	case PPTP_CONNECT_KEEPALAIVE:
		pInfo->param.idle_time = 0;
		break;
	case PPTP_CONNECT_AUTO:
		dialup_checkp = pptp_dialup_checkp;
		break;
	case PPTP_CONNECT_MANUAL:
		pInfo->param.idle_time = 0;
		break;
	default:
		diag_printf("PPTP: wrong connect mode\n");
		break;
	}
	
	return 0;
}

static int pptp_do_connect(int unit)
{
	PPTP_INFO *pInfo = &pptp_softc[unit];
	
	//if (pptp_init_param(unit, pInfo) != 0)
		//return -1;
	pptp_init_param(unit, pInfo);
	
	pptp_init_connect_mode(pInfo);
	
	if ((pInfo->param.idle_time == 0 && pInfo->param.connect_mode == 0) || force_on)
	{
		pptp_start(pInfo);
	}
	
	manual_flag = 0;
	force_on = 0;
	
	pptp_tcp_FIN_RST = 0;
	
	return 0;
}

int PPTP_doConnect(void)
{
	int i;
	
	dialup_checkp = NULL;
	
	for (i=0; i<PPTP_PPP_NUM_SESSIONS; i++)
	{
		pptp_do_connect(i);
	}
	
	return 0;
}

void PPTP_down(void)
{
	PPTP_INFO *pInfo;
	PPP_IF_VAR_T *pPppIf;
	int i;
	
	dialup_checkp = NULL;
	
	for (i=0; i<PPTP_PPP_NUM_SESSIONS; i++)
	{
		pInfo = &pptp_softc[i];
		if (!pInfo->activated)
			continue;
		
		pPppIf = &pInfo->if_ppp;
		if (pPppIf->ifnet.if_reset)
		{
			diag_printf("pptp:PPTP_down\n");
			pPppIf->hungup = 1;
			die(pPppIf, 1);
		}
		else
		{
			pptp_stop(pInfo);
		}
		pInfo->activated = 0;
	}
	pptp_tx_process_stop++;
	cyg_flag_setbits(&pptp_event_flags, PPTP_EVENT_QUIT);
}

static int pptp_dialup_checkp(void)
{
	PPTP_INFO *pInfo;
	int i;
	int rc = 1;
	
	for (i=0; i<PPTP_PPP_NUM_SESSIONS; i++)
	{
		pInfo = &pptp_softc[i];
		if (pInfo->activated)
			continue;
		
		if (pInfo->param.connect_mode != PPTP_CONNECT_AUTO)
			continue;
		
		pptp_start(pInfo);
		rc = 0;
	}
	
	return (rc);
}

/*
static void usage(void)
{
	diag_printf("usage: pptp [up|down]\n");
}*/

int pptp_cmd(int argc, char *argv[])
{
	if (argc < 1)
	{
		//usage();
		return -1;
	}
	
	if (strcmp(argv[0], "up") == 0)
	{
		if (SYS_wan_up != CONNECTED)
		{
			force_on = 1;
			mon_snd_cmd(MON_CMD_WAN_UPDATE);
		}
	}
	else if (strcmp(argv[0], "down") == 0)
	{
#ifdef CONFIG_POE_KL_FORCE_DISCON
		int i;
#endif
		
		mon_snd_cmd(MON_CMD_WAN_STOP);
		cyg_thread_delay(10);
		
		SysLog(LOG_USER|SYS_LOG_NOTICE|LOGM_PPTP, "Shut down");
		
#ifdef CONFIG_POE_KL_FORCE_DISCON
		for (i=0; i<PPTP_PPP_NUM_SESSIONS; i++)
		{
			PPTP_INFO *pInfo = &pptp_softc[i];
			if (pInfo->param.connect_mode != PPTP_CONNECT_AUTO)
			{
				manual_flag = 1;
				pInfo->param.connect_mode = PPTP_CONNECT_MANUAL;
			}
		}
#endif
		mon_snd_cmd(MON_CMD_WAN_UPDATE);
	}
	else
	{
		//usage();
		return -1;
	}
	
	return 0;
}

#ifdef	CONFIG_PPTP_PPPOE

#if (MODULE_NAT == 1)
extern void NAT_init(void);
#endif
#if (MODULE_FW == 1)
extern void FW_init(void);
#endif

void config_ppps(void) 
{
	struct ip_set setp;

	#if (MODULE_NAT == 1)
	diag_printf("config_ppps\n");
	memcpy(&setp,&primary_wan_ip_set[0],sizeof(struct ip_set));	
	memcpy(&primary_wan_ip_set[0],&ppp_ip_set[0],sizeof(struct ip_set));
	NAT_init();
	//FW_init();
	memcpy(&primary_wan_ip_set[0],&setp,sizeof(struct ip_set));
	//NAT_init();
	#endif	
}

int pptp_vpn_mon(void)
{
	int retval=-1;
	ppp_ctx_t *pctrl = &vpn_ppp_ctl;
	struct ip_set *pipset = vpn_ppp_ctl.pipset;

	//
	diag_printf("pptp_vpn_mon UP\n");
	force_on = 1;
	//if (vpn_ppp_ctl.pipset == NULL) 
	if(vpn_ppp_ctl.state == PPP_CHK_SRV)
	{
	vpn_ppp_ctl.pipset = &ppp_ip_set[0];//0:ppp1 for pptp vpn use,1:ppp0 for pppoe use first
	vpn_ppp_ctl.ipset_flags = IPSET_FG_DEFROUTE|IPSET_FG_NATIF|IPSET_FG_FWIF;
	vpn_ppp_ctl.prev_dns[0] = vpn_ppp_ctl.prev_dns[1] = 0;
	DNS_CFG_get_autodns(pctrl->prev_dns, 2);
	strcpy(vpn_ppp_ctl.pipset->ifname, "ppp1");//here fix name ppp1,for pppoe will create ppp0 first
	
	//
	pipset = vpn_ppp_ctl.pipset;

	pipset->mode = PPTPMODE;
	pipset->up = DISCONNECTED;
	pipset->flags = 0;
	pipset->data = &ppp_ip_set[1];

	memcpy(&ppp_ip_set[1],&primary_wan_ip_set[0],sizeof(struct ip_set));
	}
	switch(pctrl->state) {
	case PPP_CHK_SRV:
			{
			struct in_addr dst, mask, gw, hostmask;
			int i, fqdn=0;
			char sever_name[256];
			struct hostent *hostinfo;

			hostmask.s_addr = 	0xffffffff;
			mask.s_addr = 		primary_wan_ip_set[0].netmask;
			gw.s_addr = 		primary_wan_ip_set[0].gw_ip;
			dst.s_addr = 		primary_wan_ip_set[0].server_ip;

			/*   Set route to the DNS server  */
			for (i=0; i<2; i++) {
				if ((dst.s_addr = pctrl->prev_dns[i]) == 0)
					continue;

				if ((dst.s_addr&mask.s_addr) != (gw.s_addr & mask.s_addr)) 
					RT_add_del(NULL, &dst, &hostmask, &gw, 1, 0, ROUTE_ADD);
			}
			CFG_get( CFG_PTP_SVRD_SEL, &fqdn );
			if (fqdn) {
				CFG_get( CFG_PTP_SVRDM, &sever_name );
				hostinfo = gethostbyname(sever_name);
				if (hostinfo && (hostinfo->h_length == 4)) {
					memcpy(&(dst.s_addr), hostinfo->h_addr_list[0], 4);
					pipset->tunnel_gw = dst.s_addr;
				} else{
					timeout(mon_snd_cmd, MON_CMD_WAN_UPDATE, 200);
					return 0;
				}
			} else {
				CFG_get(CFG_PTP_SVR,&dst.s_addr);
				pipset->tunnel_gw = dst.s_addr;	
			}
			/*  PPTP tunnel server might be not in the WAN domain.  */
			if((gw.s_addr &mask.s_addr)!= (dst.s_addr & mask.s_addr)) {
				if(dst.s_addr)
					RT_add_del(NULL, &dst, &hostmask, &gw, 1, 0, ROUTE_ADD);
    				}
			}
		/*  Fall to PPP_GET_ETH  */
	case PPP_GET_ETH:
		pctrl->state = PPP_IN_BEGIN;
		//pipset->up = CONNECTING;
		//RT_add_default(primary_wan_ip_set[1].ifname, primary_wan_ip_set[1].gw_ip);
		PPTP_doConnect();
//		cyg_thread_delay(PPTP_START_DELAY);  //wait for pptp to startup

		break;

	case PPP_IN_BEGIN:
	case PPP_IN_ACTIVE:
		memcpy(pipset, &ppp_ip_set[0], sizeof(struct ip_set));
		pipset->tunnel_mode = 1;
		pipset->data = &ppp_ip_set[1];
		pipset->up = CONNECTED;
		pipset->flags = pctrl->ipset_flags;
		pctrl->state = PPP_IN_ACTIVE;

		//RT_del_default(primary_wan_ip_set[1].ifname, primary_wan_ip_set[1].gw_ip);
		/*  Configure wan interface ip and routes  */
		config_ppps();
		break;

	default:
		diag_printf("PPTP VPN: Unknown wan state: %d\n", pctrl->state);
		break;
	}
	
	return retval;
}


int pptp_vpn_mon_down()
{
	ppp_ctx_t *pctrl = &vpn_ppp_ctl;
	struct ip_set *pipset = vpn_ppp_ctl.pipset;

	if (vpn_ppp_ctl.pipset != NULL)
	{
		diag_printf("pptp_vpn_cmd Down\n");
		
		/*  shutdown pptp  */
		PPTP_down();    		
		cyg_thread_delay(30);  //wait for pptp clean up
		pctrl->state = PPP_CHK_SRV;

		/*  Remove all routes related with pptp interface */
		RT_ifp_flush(pipset->ifname);
		RT_ifa_flush(pipset->ip);
		/*  Remove pptp IP address  */
		if (!netif_del(pipset->ifname, NULL))
			diag_printf("pptp vpn:del pptp ip failed\n");
			
		pipset->up = DISCONNECTED;
		pipset->conn_time = 0;

		/*  Remove all NAT records related with this pptp vpn interface */
		#if (MODULE_NAT == 1)
		struct ip_set setp;
		memcpy(&setp,&primary_wan_ip_set[0],sizeof(struct ip_set)); 
		memcpy(&primary_wan_ip_set[0],&ppp_ip_set[0],sizeof(struct ip_set));
		NAT_flush(3);//flush pptp vpn ppp1 related NAT table;
		//FW_init();
		memcpy(&primary_wan_ip_set[0],&setp,sizeof(struct ip_set));
		primary_wan_ip_set[0].flags &= ~IPSET_FG_NATCONFIG;
		NAT_init();//rollback primary_wan_ip ppp0 related NAT table;
		#endif

		/*	Remove PPP interface  */
		if (ppp_ifnet_pptp_vpn)
			if_detach(ppp_ifnet_pptp_vpn);
		ppp_ifnet_pptp_vpn = NULL;
		
		memset(&ppp_ip_set[0], 0, sizeof(struct ip_set));
		vpn_ppp_ctl.pipset = NULL;
	}
	return 0;
}
#endif//#ifdef	CONFIG_PPTP_PPPOE
