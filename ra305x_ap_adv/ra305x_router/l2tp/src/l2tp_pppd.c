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


#include <sys_inc.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
//Dennis #include <malloc.h>
#include <sys/malloc.h>
#include <network.h>
#include <l2tp.h>
#include <l2tp_ppp.h>

#define HANDLER_NAME "sync-pppd"

#define MAX_FDS 256

/* Options for invoking pppd */
#define MAX_OPTS 64
extern cyg_flag_t L2tpEvents;
extern int L2TP_SEND_QHEAD;
extern int L2TP_SEND_QTAIL;
extern cyg_sem_t L2tp_Queue_Lock;
extern struct l2tp_mbox_struc L2TP_PACKET_QUEUE[L2TP_SEND_QUEUE_SIZE];
extern int l2tp_send_process_stop;

/*--------------------------------------------------------------
* ROUTINE NAME - l2_PppClose         
*---------------------------------------------------------------
* FUNCTION: Kills pppd
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void l2_PppClose(l2tp_session *ses)
{
	l2tp_tunnel *tunnel = (l2tp_tunnel *)ses;
    L2tpSelector *es = tunnel->es;
    struct l2tp_if *ifp;
	
	ifp = (struct l2tp_if *)es->pInfo;
	if (ifp->if_ppp.ifnet.if_reset)
	{
		ifp->if_ppp.hungup = 1;
		die(&ifp->if_ppp, 1);
	}
	
	return;
}


/*--------------------------------------------------------------
* ROUTINE NAME - l2_PppClose         
*---------------------------------------------------------------
* FUNCTION: Forks a pppd process
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int l2_PppEstablish(l2tp_session *ses)
{
	l2tp_tunnel *tunnel = (l2tp_tunnel *)ses;
    L2tpSelector *es = tunnel->es;
    struct l2tp_if *ifp;
	
	
	/* launch pppd */
	ifp = (struct l2tp_if *)es->pInfo;
    PppEth_Start(&ifp->if_ppp);
	return 0;
}

/*--------------------------------------------------------------
* ROUTINE NAME - encaps_l2tp         
*---------------------------------------------------------------
* FUNCTION: Forks a pppd process
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int encaps_l2tp	(
				struct l2tp_if *ifp,
				struct mbuf *m0    
				)
{
	if(l2tp_send_process_stop)
	{
		m_freem(m0);
		return 1;
	}	
	//SEM_LOCK(L2tp_Queue_Lock);
	L2TP_PACKET_QUEUE[L2TP_SEND_QTAIL].ifp = ifp;
	L2TP_PACKET_QUEUE[L2TP_SEND_QTAIL].m0 = m0;
	L2TP_SEND_QTAIL++;
	if(L2TP_SEND_QTAIL >= L2TP_SEND_QUEUE_SIZE)
		L2TP_SEND_QTAIL = 0;
	cyg_flag_setbits(&L2tpEvents, L2TP_EVENT_RX);
	return 0;
	
}	

/*--------------------------------------------------------------
* ROUTINE NAME - encaps_l2tp_usermode         
*---------------------------------------------------------------
* FUNCTION: Forks a pppd process
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int encaps_l2tp_usermode(
						struct l2tp_if *ifp,
						struct mbuf *m0    
						)
{
	struct mbuf *m;
	//unsigned char buf[4096+EXTRA_HEADER_ROOM];
	static unsigned char *buf = 0;
	l2tp_session *ses;
	char *p;
	int len = 0;
	int s;
	
	if(!buf)
		buf = malloc(2048+EXTRA_HEADER_ROOM);
	if(buf == NULL)
	{
		diag_printf("L2tp: cannot allocate memory\n");
		return 1;
	}	
	s = splimp();
	m = m0;
    p = buf+EXTRA_HEADER_ROOM;
    /* copy payload into buffer */
	while (m)
    {
    	len += m->m_len;
        if (m->m_len)
        {
            memcpy(p, mtod(m, char *), m->m_len);
			
            p += m->m_len;
            m = m->m_next;
        }
        else
        {
			diag_printf("L2TP: Wrong m->m_len value\n"); 	
			break;
        }   
    }
    m_freem(m0);
    splx(s);
    ses = ifp->ses;
	lf_SendPppFrame(ses, buf+EXTRA_HEADER_ROOM, len);
    //lf_SendPppFrame(ses, buf+EXTRA_HEADER_ROOM+2, len-2);
	//free(buf);
	return 0;
}

int encaps_l2tp_boostmode(
						struct l2tp_if *ifp,
						struct mbuf *m0    
						)
{
	struct mbuf *m;
	l2tp_session *ses;
	int ret =0;
	m = m0;
	ses = ifp->ses;

   	ret = lf_SendPppFrameDirectly(ses, m);
	if(ret)
	  diag_printf("enter %s, lf_SendPppFrameDirectly return %d, error!", __func__, ret);

    return ret;
}


/*--------------------------------------------------------------
* ROUTINE NAME - l2_HandleFrame         
*---------------------------------------------------------------
* FUNCTION: Forks a pppd process
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void l2_HandleFrame(l2tp_session *ses, unsigned char *buf,size_t len)
{
	l2tp_tunnel *tunnel = (l2tp_tunnel *)ses;
    L2tpSelector *es = tunnel->es;
    struct mbuf *m;
    struct l2tp_if *ifp;
    int s;
	
	ifp = (struct l2tp_if *)es->pInfo;
    /* Add framing bytes */
    //*--buf = 0x03;
    //*--buf = 0xFF;
    //len += 2;
	s = splimp();
    // Send to ppp
    MGETHDR(m, M_DONTWAIT, MT_DATA);
    if (m == NULL)
    {
		splx(s);
		return ;
    }
    MCLGET(m, M_DONTWAIT);
    if ((m->m_flags & M_EXT) == 0) 
    {
		splx(s);
		(void) m_free(m);
		return ;
	}
    // copy data to mbuf
    m->m_len = len;
    bcopy(buf, m->m_data, len);
    m->m_flags |= M_PKTHDR;
    m->m_pkthdr.len = len;
    m->m_pkthdr.rcvif = &ifp->if_ppp.ifnet;
    m->m_next = 0;
    splx(s);
    // Send to ppp
    ppppktin(&ifp->if_ppp, m);
}
