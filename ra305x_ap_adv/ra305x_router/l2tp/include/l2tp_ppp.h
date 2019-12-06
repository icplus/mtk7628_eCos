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

#ifndef	__L2TP_PPP_H__
#define __L2TP_PPP_H___

#include <pppd.h>
#include <ppp_api.h>

struct l2tp_if
{
    PPP_IF_VAR_T    if_ppp;         /* common ppp structures */

#define t_ifname          if_ppp.ifname       /* ethernet layer interfcae name */
#define t_pppname         if_ppp.pppname      /* PPP layer interfcae name */
#define t_user            if_ppp.user         /* authentication username */ 
#define t_password        if_ppp.passwd       /* password for PAP  */
#define t_idle            if_ppp.idle         /* idle time */
#define t_mtu             if_ppp.mtu          /* MTU that we allow */
#define t_mru             if_ppp.mru          /* MRU that we allow */
    
#define t_defaultroute    if_ppp.defaultroute /* flag to specify this interfce whether is defaultroute */
#define t_pOutputHook     if_ppp.pOutput      /* output packet function pointer */
#define t_pIfStopHook     if_ppp.pIfStopHook       /* functon called by PPP daemon to close pppoe interface */
#define t_pIfReconnectHook if_ppp.pIfReconnectHook /* physical layer reconnection hook routine */
    
    struct l2tp_if     *next;         /* pointer to next PPPOE interface */
    struct l2tp_if     *prev;         /* pointer to previous PPPOE interface */
    
    cyg_handle_t msgq_id;        // Message queue ID
    L2tpSelector es;
    l2tp_session *ses;
};

enum L2TP_MSGQ_CMD_E
{
    L2TP_CMD_STOP = 1,  // Comming from upper layer
    L2TP_CMD_HANGUP,    // Comming from lower layer
    L2TP_CMD_RECONNECT,    // Comming from lower layer
    L2TP_CMD_CALLMGR_RECONNECT,
    L2TP_CMD_TIMER
};

struct l2tp_mbox_struc
{
    struct l2tp_if *ifp;
    struct mbuf *m0;    
};
#define L2TP_EVENT_RX 0x01
#define L2TP_EVENT_QUIT 0x02
#define L2TP_SEND_QUEUE_SIZE 512
#define SEM_LOCK(s)   cyg_semaphore_wait(&s) 
#define SEM_UNLOCK(s)   cyg_semaphore_post(&s) 



int l2tp_ScanCmd();
l2tp_session *lt_FindSession(l2tp_tunnel *tunnel, uint16_t sid);
l2tp_tunnel *lt_FindByMyId(uint16_t id);
l2tp_tunnel *lt_FindForPeer(l2tp_peer *peer, L2tpSelector *es);
void lt_AddSession(l2tp_session *ses);
void lt_DeleteSession(l2tp_session *ses, char const *reason);
void lt_HandleReceivedCtlDatagram(l2tp_dgram *dgram,
						  L2tpSelector *es,
						  struct sockaddr_in *from);
void lt_XmitCtlMessage(l2tp_tunnel *tunnel, l2tp_dgram  *dgram);
void lt_StopTunnel(l2tp_tunnel *tunnel, char const *reason);
void lt_StopAll(char const *reason);

l2tp_session *l2tp_tunnel_next_session(l2tp_tunnel *tunnel, void **cursor);
void lt_SendZLB(l2tp_tunnel *tunnel);

/* Access functions */
int l2tp_num_tunnels(void);
l2tp_tunnel *l2tp_first_tunnel(void **cursor);
l2tp_tunnel *l2tp_next_tunnel(void **cursor);

void lt_HandleTimeout(L2tpSelector *es, int fd,
				  unsigned int flags, void *data);
void lt_FreeAllHandler(void *data);
void lt_Cleanup(void *data);
void lt_ForceFinallyCleanup(void *data);

/* session.c */
void l2tp_session_lcp_snoop(l2tp_session *ses,
			    unsigned char const *buf,
			    int len,
			    int incoming);

void ls_SendCDN(l2tp_session *ses, int result_code, int error_code,
			   char const *fmt, ...);
void ls_Release(l2tp_session *ses, char const *reason);
void ls_NotifyTunnelOpen(l2tp_session *ses);
void l2tp_session_lns_handle_incoming_call(l2tp_tunnel *tunnel,
				      uint16_t assigned_id,
				      l2tp_dgram *dgram,
				      char const *calling_number);
void ls_HandleCDN(l2tp_session *ses, l2tp_dgram *dgram);
void ls_HandleICRP(l2tp_session *ses, l2tp_dgram *dgram);
/* Call this when a LAC wants to send an incoming-call-request to an LNS */
l2tp_session *ls_CallINS(l2tp_peer *peer,
				    char const *calling_number,
				    L2tpSelector *es,
				    void *private);

/* dgram.c */
l2tp_dgram *lf_New(size_t len);
l2tp_dgram *lf_NewCtl(uint16_t msg_type, uint16_t tid, uint16_t sid);
void lf_Free(l2tp_dgram *dgram);
l2tp_dgram *lf_TakeFromBSD(struct sockaddr_in *from);
l2tp_dgram *lf_TakeFromBSD_BoostMode(struct sockaddr_in *from);
int lf_SendToBSD(l2tp_dgram const *dgram,
		       struct sockaddr_in const *to);
int lf_SendPppFrame(l2tp_session *ses, unsigned char const *buf,
			 int len);
int lf_SendPppFrameDirectly(l2tp_session *ses, struct mbuf *m0);

unsigned char *lf_SearchAvp(l2tp_dgram *dgram,
				     l2tp_tunnel *tunnel,
				     int *mandatory,
				     int *hidden,
				     uint16_t *len,
				     uint16_t vendor,
				     uint16_t type);

unsigned char *lf_PullAvp(l2tp_dgram *dgram,
				   l2tp_tunnel *tunnel,
				   int *mandatory,
				   int *hidden,
				   uint16_t *len,
				   uint16_t *vendor,
				   uint16_t *type,
				   int *err);

int lf_AddAvp(l2tp_dgram *dgram,
		       l2tp_tunnel *tunnel,
		       int mandatory,
		       uint16_t len,
		       uint16_t vendor,
		       uint16_t type,
		       void *val);

int lf_ValidateAvp(uint16_t vendor, uint16_t type,
			    uint16_t len, int mandatory);

/* utils.c */
typedef void (*l2tp_shutdown_func)(void *);

void l2tp_random_init(void);
void l2tp_random_fill(void *ptr, size_t size);
void l2tp_set_errmsg(char const *fmt, ...);
char const *l2tp_get_errmsg(void);
void l2tp_cleanup(void);
int lu_SetShutdownCtl(l2tp_shutdown_func f, void *data);
void l2tp_TunnelCtl(L2tpSelector *es, int fd);
int encaps_l2tp_usermode(struct l2tp_if *ifp, struct mbuf *m0);    
int encaps_l2tp_boostmode(struct l2tp_if *ifp, struct mbuf *m0);


#define L2TP_RANDOM_FILL(x) l2tp_random_fill(&(x), sizeof(x))

/* network.c */
extern int Sock;
extern char L2tp_Hostname[MAX_HOSTNAME];

int l2tp_NetworkInit(L2tpSelector *es);


/* debug.c */
char const *ld_Avp2Str(uint16_t type);
char const *ld_MessageType2Str(uint16_t type);
char const *ld_Tunnel2Str(l2tp_tunnel *tunnel);
char const *ld_Ses2Str(l2tp_session *session);
char const *ld_DescribeDgram(l2tp_dgram const *dgram);
void ld_Debug(int what, char const *fmt, ...);
void ld_SetBitMask(unsigned long mask);
void l2tp_PendingChanges(L2tpSelector *es);
void l2tp_AuthGenResponse(uint16_t msg_type,
		  char const *secret,
		  unsigned char const *challenge,
		  size_t chal_len,
		  unsigned char buf[16]);
void l2_PppClose(l2tp_session *ses);
int l2_PppEstablish(l2tp_session *ses);


/* Add a handler for a ready file descriptor */
extern L2tpHandler *l2tp_AddHandler(L2tpSelector *es,
				      int fd,
				      unsigned int flags,
				      CallbackFunc fn, void *data);

/* Add a handler for a ready file descriptor with associated timeout*/
extern L2tpHandler *l2tp_AddHandlerWithTimeout(L2tpSelector *es,
						 int fd,
						 unsigned int flags,
						 struct timeval t,
						 CallbackFunc fn,
						 void *data);


/* Add a timer handler */
extern L2tpHandler *l2tp_AddTimerHandler(L2tpSelector *es,
					   struct timeval t,
					   CallbackFunc fn,
					   void *data);

/* Change the timeout of a timer handler */
void l2tp_ChangeTimeout(L2tpHandler *handler, struct timeval t);

/* Delete a handler */
extern int l2tp_DelHandler(L2tpSelector *es,
			    L2tpHandler *eh);
#endif
