/*
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <sys_inc.h>
#include <cfg_net.h>
#include <sys/time.h>
#include <net/netisr.h>
#include <pppd.h>
#include <ppp_api.h>
#include <sys/errno.h>
#include <eventlog.h>
#define PACKETPTR	struct mbuf *
#include <ppp_comp.h>
#include <pppSecret.h>
#include <cfg_def.h>
#include <sys_status.h>
 
#define PPP_COMPRESS
/* external variables */
extern void if_attach(struct ifnet *);
extern void if_dettach(struct ifnet *);
extern void if_down(struct ifnet *);
#define PPP_FASTNAT
#ifdef PPP_FASTNAT
extern int ipflow_fastforward(struct mbuf *m);
#endif

/* global variables */
extern struct	ifqueue ipintrq; // define in ip_input.c

PPP_IF_VAR_T *ppp_if_var = NULL;

/* prototypes */
static void hup             (PPP_IF_VAR_T *pPppIf);
static void intr            (PPP_IF_VAR_T *pPppIf);
static void term            (PPP_IF_VAR_T *pPppIf);
static void expire          (PPP_IF_VAR_T *pPppIf);
static void get_input       (PPP_IF_VAR_T *pPppIf);

static void cleanup         (PPP_IF_VAR_T *pPppIf, int status, caddr_t arg);
static int pppread          (PPP_IF_VAR_T *pPppIf, char *buf, int count);
// static void ppp_ccp( struct ppp_softc *sc, struct mbuf *m, int rcvd);

struct mbuf * ppp_dequeue(PPP_IF_VAR_T *pPppIf, struct mbuf *m);
struct mbuf * ppp_ccp_pktin(PPP_IF_VAR_T *pPppIf, struct mbuf *m);
int ppp_ccp_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data);
void ppp_ccp_closed(PPP_IF_VAR_T *pPppIf);

void ipcp_down(fsm *);


/*
 * PPP Data Link Layer "protocol" table.
 * One entry per supported protocol.
 */
struct protent {
    u_short protocol;
    void (*init)(PPP_IF_VAR_T *);
    void (*input)(PPP_IF_VAR_T *, u_char *, int);
    void (*protrej)(PPP_IF_VAR_T *);
    char *name;
} prottbl[] = {
    { LCP,  lcp_init,  lcp_input,  lcp_protrej, "LCP" },
    { IPCP, ipcp_init, ipcp_input, ipcp_protrej, "IPCP" },
    { UPAP, upap_init, upap_input, upap_protrej, "PAP" },
	{ CHAP, ChapInit,  ChapInput,  ChapProtocolReject, "CHAP" }
#ifdef MPPE    
	,
    { CCP, ccp_init, ccp_input, ccp_protrej, "CCP" }
#endif
};

#define N_PROTO         (sizeof(prottbl) / sizeof(prottbl[0]))
#define PPPBUFSIZE      (PPP_MTU * 2)

void ppp_send_wanupdate_event(void)
{
	mon_snd_cmd( MON_CMD_WAN_UPDATE );
	untimeout((timeout_fun *)ppp_send_wanupdate_event,0);
}


/*******************************************************************************
*
* ppp_daemon - PPP daemon task
*
* This is the entry point for the PPP daemon task.
*
* NOMANUAL
*/
void ppp_daemon(cyg_addrword_t argv)
{
	int i;
	struct ppp_if_var *pPppIf;
	pPppIf = (struct ppp_if_var *)argv;
	u_long event;
	
	
	/* Attach to ifnet firstly */
	if (pPppIf->pAttach(pPppIf) == ERROR)
	{
		die(pPppIf,1);
	}
	
	pPppIf->callout = 0;
	
	setipaddr(pPppIf, pPppIf->my_ip, pPppIf->peer_ip);
	setnetmask(pPppIf, pPppIf->netmask);
	
	pPppIf->phase = PHASE_DEAD;
	pPppIf->ifnet.if_flags |=  IFF_RUNNING | IFF_UP;
	
	/* initialize the authentication secrets table */
	(void) AUTHInitSecurityInfo();
	
	//
	// add the user and password as the secret, and
	// don't care the peer name by using wildcard "*"
	//
	AUTHInsertSecret(pPppIf->user, "*", pPppIf->passwd, 0);
	
	/*
	 * Initialize to the standard option set, then parse, in order,
	 * the system options file, the user's options file, and the command
	 * line arguments.
	 */
	for (i = 0; i < N_PROTO; i++)
		(*prottbl[i].init)(pPppIf);
	
	if (!parse_options(pPppIf))
		die(pPppIf,1);
	
	check_auth_options(pPppIf);
	
	magic_init();
	
	/*
	 * Create evnet group, start opening the connection, and  wait for
	 * incoming event (reply, timeout, etc.).
	 */
	 
	cyg_flag_init(&pPppIf->event_id);
	
	SysLog(LOG_USER|SYS_LOG_NOTICE|LOGM_PPPD,"start lcp stage");
	lcp_lowerup(pPppIf);		/* XXX Well, sort of... */
	lcp_open(pPppIf);		    /* Start protocol */
	
	/*
	 * PPP daemon loop
	 */
	for (pPppIf->phase = PHASE_ESTABLISH, errno = 0;
		pPppIf->phase != PHASE_DEAD;) 
	{
		event = 0;
		
		/* Wait for next signal */
		event = cyg_flag_wait(&pPppIf->event_id,PPP_EVENT_ALL,CYG_FLAG_WAITMODE_OR | CYG_FLAG_WAITMODE_CLR);
			
		if (event & PPP_EVNET_HUNGUP)
		{
			hup(pPppIf);
		}
		if (event & PPP_EVENT_INTERRUPT)
		{
			intr(pPppIf);
		}
		if (event & PPP_EVENT_TERMINATE)
		{
			term(pPppIf);
		}
		if (event & PPP_EVENT_TIMEOUT)
		{
			expire(pPppIf);
		}
		if (event & PPP_EVENT_INPUT)
		{
			get_input (pPppIf);
		}
		
		errno = 0;			/* reset errno */
	}
	
	die(pPppIf, 1);
}

/*
 * die - like quit, clean up state and exit with the specified status..
 */
void die(PPP_IF_VAR_T *pPppIf, int status)
{
	int i;
	fsm *f = &pPppIf->lcp_fsm;
	
	
	//SysLog(LOG_USER|SYS_LOG_NOTICE|LOGM_PPPD, "ppp: die and clean");
	lcp_close(pPppIf);		/* Close connection */
	
#ifdef MPPE
	ppp_ccp_closed(pPppIf);
#endif
	
	if (f->flags & (OPT_PASSIVE | OPT_SILENT))
		return;
	
	if (pPppIf->phase == PHASE_NETWORK)
	{
		ipcp_down(&pPppIf->ipcp_fsm);
    }
	
	MAINDEBUG(" Exiting.");
	
    pPppIf->phase = PHASE_IDLE;
    cleanup(pPppIf, status, NULL);
	
	ppp_IfDelete(pPppIf);
	pPppIf->ifnet.if_reset = 0;
	
	for(i=0;i<16;i++)
	{
		untimeout((timeout_fun *)pppSendTimerEvent, (void *)pPppIf);
	}
	
	if (pPppIf->hungup == 1)
	{
		// stop low level interface device
		pPppIf->pIfStopHook(pPppIf);
	}
	else 
	{
		cyg_thread_delay(200);
		
		// re-connection low level interface device
		pPppIf->pIfReconnectHook(pPppIf);
		
		//mon_snd_cmd( MON_CMD_WAN_UPDATE );
		timeout((timeout_fun *)ppp_send_wanupdate_event, 0, 100);
		if (pPppIf->task_id)
			cyg_thread_delete(pPppIf->task_id);
		
		return;
	}
	

	//wait for rcv terminate pkt
	cyg_thread_delay(50);
	if (pPppIf->task_id)
		cyg_thread_delete(pPppIf->task_id);
}


/*
 * cleanup - restore anything which needs to be restored before we exit
 */
static void
cleanup(PPP_IF_VAR_T *pPppIf, int status, caddr_t arg)
{
    struct ppp_callout **copp, *freep;

#undef TTY_READY
#ifdef TTY_READY
    if (pPppIf->encapsulate_type == PPP_ENCAP_TTY && pPppIf->restore_term) 
    {
        if (ioctl(pPppIf->fd, FIOSETOPTIONS, pPppIf->ttystate) == ERROR)
            MAINDEBUG("ioctl(FIOSETOPTIONS) error");
    }
    if (pPppIf->encapsulate_type == PPP_ENCAP_TTY)
    {
        close(pPppIf->fd);
        pPppIf->fd = ERROR;
        
        if (pPppIf->s != ERROR)
            close(pPppIf->s);
    }
#endif
	
	pPppIf->ifnet.if_reset((struct ifnet *)pPppIf);
	
	// 
	// Free the time lists before we deleting timer 
	//
	for (copp = &(pPppIf->callout); *copp;)
	{
		freep = *copp;
		*copp = freep->c_next;
		(void) free((char *) freep);
	}
	
	pPppIf->callout = NULL;
	
	untimeout((timeout_fun *)pppSendTimerEvent, (void *)pPppIf);
	
	cyg_flag_destroy(&pPppIf->event_id);
	
	// Clear users of this unit in security table
	if (pPppIf->user[0] || pPppIf->passwd[0])
		AUTHDeleteSecretEntry(pPppIf->user, "*", pPppIf->passwd);
}
  
/*
 * ppp_timeout - Schedule a timeout.
 *
 * Note that this timeout takes the number of seconds, NOT hz (as in
 * the kernel).
 */
void ppp_timeout(void (*func) (), caddr_t arg, int seconds, PPP_IF_VAR_T *pPppIf)
{
	struct ppp_callout *newp, **oldpp;
	
	MAINDEBUG("Timeout %x:%x in %d seconds.", (int) func, (int) arg, seconds);
	
	/*
	 * Allocate timeout.
	 */
	if ((newp = (struct ppp_callout *) malloc(sizeof(struct ppp_callout))) == NULL) 
	{
		diag_printf("Out of memory in timeout()!");
		die(pPppIf, 1);
		return ;
	}
	memset(newp,0,sizeof(struct ppp_callout));
	
	newp->c_arg = arg;
	newp->pPppIf = pPppIf;
	newp->c_func = func;
	
	/*
	 * Find correct place to link it in and decrement its time by the
	 * amount of time used by preceding timeouts.
	 */
	for (oldpp = &(pPppIf->callout);
		 *oldpp && (*oldpp)->c_time <= seconds;
		 oldpp = &(*oldpp)->c_next)
	{  
		seconds -= (*oldpp)->c_time;
	}
	
	newp->c_time = seconds;
	newp->c_next = *oldpp;
	if (*oldpp)
		(*oldpp)->c_time -= seconds;
	
	*oldpp = newp;
	
	/*
	 * If this is now the first callout then we have to set a new
	 * itimer.
	 */
	if (pPppIf->callout == newp) 
	{
		timeout((timeout_fun *)pppSendTimerEvent, (void *)pPppIf, pPppIf->callout->c_time*100);
		pPppIf->schedtime = cyg_current_time();
	}
}
  

/*
 * ppp_untimeout - Unschedule a timeout.
 */
void ppp_untimeout (void (*func) (), caddr_t arg, PPP_IF_VAR_T *pPppIf)
{
	struct ppp_callout **copp, *freep;
	int reschedule = 0;
	
	MAINDEBUG("Untimeout %x:%x.", (int) func, (int) arg);
	
	/*
	 * If the first callout is unscheduled then we have to set a new
	 * itimer.
	 */
	if (pPppIf->callout &&
		pPppIf->callout->c_func == func &&
		pPppIf->callout->c_arg == arg)
	{
		reschedule = 1;
	}
	
	/*
	 * Find first matching timeout.  Add its time to the next timeouts
	 * time.
	 */
	for (copp = &(pPppIf->callout); *copp; copp = &(*copp)->c_next)
	{
		if ((*copp)->c_func == func &&
		   (*copp)->c_arg == arg) 
		{
			freep = *copp;
			*copp = freep->c_next;
			if (*copp)
			{
				(*copp)->c_time += freep->c_time;
			}
	
			(void) free((char *) freep);
			break;
		}
	}
	
	if (reschedule) 
	{
		untimeout((timeout_fun *)pppSendTimerEvent, (void *)pPppIf);
		
		if (pPppIf->callout)
			timeout((timeout_fun *)pppSendTimerEvent, (void *)pPppIf, pPppIf->callout->c_time*100);
		
		pPppIf->schedtime = cyg_current_time();
	}
}


void adjidle(PPP_IF_VAR_T *pPppIf)
{
	u_long	now_ticks = cyg_current_time();
	u_long	delta = (u_long)(now_ticks - pPppIf->last_idle_ticks);
	
    if (pPppIf->idle != 0)
    {
		if (delta >= pPppIf->idle * 100)
		{
			die(pPppIf,1);
		}
		else
		{
			u_long left = ((pPppIf->idle * 100 - delta) + 99)/100;
			PPP_TIMEOUT(adjidle, (caddr_t)pPppIf, left, pPppIf);
		}
    }
}


/*
 * adjtimeout - Decrement the first timeout by the amount of time since
 * it was scheduled.
 */
void adjtimeout(PPP_IF_VAR_T *pPppIf)
{
    int timediff;
    
    if (pPppIf->callout == NULL)
        return;
	
    /*
     * Make sure that the clock hasn't been warped dramatically.
     * Account for recently expired, but blocked timer by adding
     * small fudge factor.
     */

    timediff = (unsigned long)(cyg_current_time() - pPppIf->schedtime)/ 100; /* second */
    
    if (timediff < 0 || timediff >= pPppIf->callout->c_time)
        return;
    
    pPppIf->callout->c_time -= timediff;	/* OK, Adjust time */
}


void ppp_ifstart(struct ifnet *ifp)
{
	PPP_IF_VAR_T *pPppIf = (PPP_IF_VAR_T *)ifp;
	struct mbuf *m0;

	IF_DEQUEUE(&ifp->if_snd, m0);
	if (pPppIf->pOutput != NULL) {
		while(m0 != NULL) {
			ifp->if_opackets++;
			pPppIf->pOutput(pPppIf, m0);
			IF_DEQUEUE(&ifp->if_snd, m0);
		}
	} else {
		while (m0 != NULL) {
			m_freem(m0);
			IF_DEQUEUE(&ifp->if_snd, m0);
		}
	}
}


/*
 * Queue a packet.  Start transmission if not active.
 * Packet is placed in Information field of PPP frame.
 */
int pppoutput(struct ifnet *ifp, struct mbuf *m0, 
                     struct sockaddr *dst, struct rtentry *rt)
{
	PPP_IF_VAR_T *pPppIf = (PPP_IF_VAR_T *)ifp;
	u_char *cp;
	int s, error;
	
	if (((ifp->if_flags & IFF_RUNNING) == 0
		|| (ifp->if_flags & IFF_UP) == 0) && dst->sa_family != AF_UNSPEC) 
	{
		error = ENETDOWN;	/* sort of */
		goto bad;
	}
	
	/*
	* Compute PPP header.
	*/
	switch (dst->sa_family) 
	{
	case AF_INET:
		if (pPppIf->phase != PHASE_NETWORK) 
		{
			error = ENETDOWN;
			goto bad;
		}
		
		// Update the last idel ticks everytime routing traffic found
		#if !defined(CONFIG_PPTP_PPPOE) && !defined(CONFIG_L2TP_OVER_PPPOE)
		if (m0->m_pkthdr.rcvif)
		#endif	
			pPppIf->last_idle_ticks = (u_long)cyg_current_time();

		M_PREPEND(m0, PPPHEADER_LEN, M_DONTWAIT);
		if (m0 == NULL)
		{
			errno = ENOBUFS;
			return ENOBUFS;
		}
			
		// Append PPP header
		cp = mtod(m0, u_char *);
		MAKE_PPPHEADER(cp, PPP_IP);
		break;
		
	case AF_UNSPEC:
		break;
	
	default:
		error = EAFNOSUPPORT;
		goto bad;
	}
#define EXTRA_HDLC_CHECK

#ifdef MPPE
#ifdef EXTRA_HDLC_CHECK
{
		cp = mtod(m0, u_char *);
		if (cp[0]==PPP_ALLSTATIONS && cp[1]==PPP_UI) {
			diag_printf("ppp: HDLC hdr already inserted 1\n");
			goto out;
		}
}
#endif	/*  EXTRA_HDLC_CHECK  */
	if (pPppIf->mppe == 1)
		m0 = ppp_dequeue(pPppIf, m0);
#endif

	if (!(pPppIf->pSc->sc_flags & SC_COMP_AC)) {
#define EXTRA_HDLC_CHECK
#ifdef EXTRA_HDLC_CHECK
{
		cp = mtod(m0, u_char *);
		if (cp[0]==PPP_ALLSTATIONS && cp[1]==PPP_UI) {
			diag_printf("ppp: HDLC hdr already inserted 2\n");
			goto out;
		}
}
#endif	/*  EXTRA_HDLC_CHECK  */
		/*  Prepend HDLC header  */
		M_PREPEND(m0, PPP_HDLC_ACC_LEN, M_DONTWAIT);
		if (m0 == NULL)
		{
			errno = ENOBUFS;
			return ENOBUFS;
		}
			
		// Append HDLC header
		cp = mtod(m0, u_char *);
		MAKE_HDLC_HEADER(cp);
	}
out:

	s = splimp();
	if (IF_QFULL(&ifp->if_snd)) {
		if (ifp->if_start != NULL)
			ifp->if_start(ifp);
		IF_DROP(&ifp->if_snd);
		splx(s);
		error = ENOBUFS;
		goto bad;
	}
	ifp->if_obytes += m0->m_pkthdr.len;

	IF_ENQUEUE(&ifp->if_snd, m0);
	if (ifp->if_start != NULL)
		ifp->if_start(ifp);
	splx(s);

	return (OK);

bad:
	m_freem(m0);
	
	errno = error;
	return (error);
}

/*
 * Line specific (tty) write routine.
 */
int pppwrite(PPP_IF_VAR_T *pPppIf, char *buf, int count)
{
	struct mbuf *m;
	struct sockaddr dst;
	u_char *wp;
	
	if (count > pPppIf->ifnet.if_mtu + PPP_HDRLEN ||
		count < PPP_HDRLEN) 
	{
		errno = EMSGSIZE;
		return (-1);
	}
	
	/*
	 * Get mbuf to send data out
	 */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return 0;
	
	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) 
	{
		/*
		 * we couldn't get a cluster - if memory's this
		 * low, it's time to start dropping packets.
		 */
		(void) m_free(m);
		return 0;
	}

	/*  Reserver some space for lower layers  */
	m->m_data += 32;
#if 0	
	// musterc+, 2006.05.11: for zero padding
	memset(mtod(m, caddr_t), 0, ETHERMIN);
#endif
	wp = mtod(m, caddr_t);
	bcopy(buf, wp, count);
	m->m_pkthdr.len = m->m_len = count;
	m->m_pkthdr.rcvif = &pPppIf->ifnet;
	
	/* This is ppp specific protocol */
	dst.sa_family = AF_UNSPEC;
	
	return pppoutput(&pPppIf->ifnet, m, &dst, (struct rtentry *)pPppIf);
}

/*******************************************************************************
* ppppktin - pass the packet to the appropriate stack LCP or IP
* 
* This routine determines packet type and queues the mbuf to the appropriate
* queue. This routine returns a value of 1 if the packet type is other than
* PPP_IP.
*
* NOMANUAL
*
* RETURNS: 0/1
*/
int ppppktin(PPP_IF_VAR_T *pPppIf, struct mbuf *m)
{
	struct ifqueue *inq;
	int s, ilen, proto;
	int rv = 0;
	u_char *cp;
	struct mbuf *mp;
	cyg_uint64 c_time;
		
	pPppIf->ifnet.if_ipackets++;
	pPppIf->ifnet.if_ibytes += m->m_pkthdr.len;
	
	c_time = cyg_current_time();
	pPppIf->ifnet.if_lastchange.tv_sec = (u_long)(c_time/100);
	pPppIf->ifnet.if_lastchange.tv_usec = (((u_long)ctime)%100) * 10000;

	if (m->m_len < PPP_HDRLEN && (m = m_pullup(m, PPP_HDRLEN)) == 0)
		return 0;
	
	/*  Strip HDLC header  */
	cp = mtod(m, u_char *);
	if ((cp[0] == PPP_ALLSTATIONS) && (cp[1] == PPP_UI)) {
		/*  Ignore HDLC address/control field  */
		m_adj(m, PPP_HDLC_ACC_LEN);
		cp = mtod(m, u_char *);
	}
	
#ifdef MPPE
	if (pPppIf->mppe == 1)
	{
		m = ppp_ccp_pktin(pPppIf, m);
		if (m == NULL)
			return 0;
		// Pull up again 
		if (m->m_len < PPPHEADER_LEN && (m = m_pullup(m, PPPHEADER_LEN)) == 0)
			return 0;
		cp = mtod(m, u_char *);
	}
#endif
	
	/*  Mark the line is active  */
//	if (pPppIf->lcp_echo_number > 0)
		pPppIf->lcp_line_active = 1;
	
	proto = PPP_PROTOCOL(cp);
	ilen = 0;
	for (mp = m; mp != NULL; mp = mp->m_next)
	{
		ilen += mp->m_len;
	}
    
	/*
	* If the packet will fit in a header mbuf, don't waste a
	* whole cluster on it.
	*/
	m->m_pkthdr.len = ilen;
	m->m_pkthdr.rcvif = &pPppIf->ifnet;
	
	switch (proto) 
	{
#ifdef INET
	case PPP_IP:
		/*
		 * IP packet - take off the ppp header and pass it up to IP.
		 */
		if ((pPppIf->ifnet.if_flags & IFF_UP) == 0 || pPppIf->phase != PHASE_NETWORK) 
		{
			/* interface is down - drop the packet. */
			m_freem(m);
			return 0;
		}
		
		/*
		 * Trim the PPP header from head
		 */
		m_adj(m, PPPHEADER_LEN);
#ifdef PPP_FASTNAT
		if (ipflow_fastforward(m))
			return 0;
#endif
		inq = &ipintrq;
		break;
#endif
	
	default:
	   /*
		* Some other protocol - place on input queue for read().
		*/
		if (pPppIf->phase == PHASE_IDLE)
		{
			m_freem(m);
			return 0;    
		}
		
		inq = &pPppIf->inq;
		rv = 1;
		break;
	}
    
	/*
	* Put the packet on the appropriate input queue.
	*/
	s = splimp();
	if (IF_QFULL(inq))
	{   
		// input queue has fulled, send a INPUT event to push 
		// PPP task to read packet from queue. 
		pppSendInputEvent(pPppIf);
		
		IF_DROP(inq);
		
		pPppIf->ifnet.if_ierrors++;
		pPppIf->ifnet.if_iqdrops++;
		m_freem(m);
		rv = 0;
	}
	else
	{
		IF_ENQUEUE(inq, m);
		if (proto == PPP_IP)
		{
			//ipintr();
			schednetisr(NETISR_IP);
		}
		else 
		{
			pppSendInputEvent(pPppIf); 	/* notify PPP daemon task to process this packet */
		}
	}
	
	splx(s);
	return rv;
}
 
/*
 * Line specific (tty) read routine.
 */
static int pppread(PPP_IF_VAR_T *pPppIf, char *buf, int count)
{
    struct mbuf *m, *mfree;
    int s;

    s = splimp();
    IF_DEQUEUE(&pPppIf->inq, m);
    splx(s);

    if (m == NULL)
	    return 0;
    
    mfree = m;
	
    // Copies dat from mbuf chain to user supplied buffer area
    count = 0;
    do 
    {
        int mlen = m->m_len;
        
        bcopy(mtod(m, char *), buf, mlen);
		
        buf += mlen;
        count += mlen;
	}
	while ((m = m->m_next) != NULL);
    
    // free mbuf chain and return device driver 
    m_freem(mfree);
    return (count);
}

/*
 * Process an ioctl request to interface.
 */

#ifndef	EINVAL
#define EINVAL -1
#endif
int pppioctl(ifp, cmd, data)
    struct ifnet *ifp;
    u_long cmd;
    caddr_t data;
{
	struct ifaddr *ifa = (struct ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *)data;
	
	int s = splimp(), error = 0;
	
	switch (cmd)
	{
	case SIOCGIFFLAGS:
		*(short *)data = ifp->if_flags;
		 break;
		 
	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_RUNNING) == 0)
			ifp->if_flags &= ~IFF_UP;
		break;
		
	case SIOCSIFADDR:
		if (ifa->ifa_addr->sa_family != AF_INET)
			error = EAFNOSUPPORT;
		break;
	
	case SIOCSIFDSTADDR:
		if (ifa->ifa_dstaddr->sa_family != AF_INET)
			error = EAFNOSUPPORT;
		break;
		
	case SIOCSIFMTU:
		ifp->if_mtu = ifr->ifr_mtu;
		break;
		
	case SIOCGIFMTU:
		ifr->ifr_mtu = ifp->if_mtu;
		break;
		
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		if (ifr == 0) 
		{
			error = EAFNOSUPPORT;
			break;
		}
		switch(ifr->ifr_addr.sa_family) 
		{
#ifdef INET
		case AF_INET:
			break;
#endif
		default:
			error = EAFNOSUPPORT;
			break;
		}
		break;
		
	default:
#ifdef MPPE
		error = ppp_ccp_ioctl(ifp, cmd, data);
#else
		error = EINVAL;
#endif
		break;
	}
	
	splx(s);
	return (error);
}


/*
 * hup - process PPP_EVENT_HUNGUP event.
 *
 * Indicates that the physical layer has been disconnected.
 */
static void
hup(PPP_IF_VAR_T *pPppIf)
{
    MAINDEBUG("Hungup (SIGHUP)");
    pPppIf->hungup = 1;		/* they hung up on us! */
    adjtimeout(pPppIf);   	/* Adjust timeouts */
    die(pPppIf, 1);			/* Close connection */
}

/*
 * term - process PPP_EVENT_TERMINATE event.
 *
 * Indicates that we should initiate a graceful disconnect and exit.
 */
static void
term(PPP_IF_VAR_T *pPppIf)
{
    fsm *f = &pPppIf->lcp_fsm;

    f->flags = 0;   
    MAINDEBUG("Terminating link.");
    adjtimeout(pPppIf);    	/* Adjust timeouts */
    die(pPppIf, 1);			/* Close connection */
}


/*
 * intr - PPP_EVENT_INTERRUPT
 *
 * Indicates that we should initiate a graceful disconnect and exit.
 */
static void
intr(PPP_IF_VAR_T *pPppIf)
{
    MAINDEBUG("Interrupt received: terminating link");
    adjtimeout(pPppIf);	    /* Adjust timeouts */
    die(pPppIf, 1);			/* Close connection */
}

/*
 * expire - timer has expired
 *
 * Indicates a timeout.
 */
static void
expire(PPP_IF_VAR_T *pPppIf)
{
	struct ppp_callout *freep, *list, *last;
	
    MAINDEBUG("expire");
    
    if (pPppIf->callout == NULL)
    {
        return;
    }
    /*
     * Get the first scheduled timeout and any that were scheduled
     * for the same time as a list, and remove them all from callout
     * list.
    */
    list = last = pPppIf->callout;
	
    while (last->c_next != NULL && last->c_next->c_time <= 0)
    {
        last = last->c_next;
    }
	
    pPppIf->callout = last->c_next;
    last->c_next = NULL;
	
    /*
    * Set a new itimer if there are more timeouts scheduled.
    */
    if (pPppIf->callout) 
    {
        timeout((timeout_fun *)pppSendTimerEvent, (void *)pPppIf, pPppIf->callout->c_time*100);
		
        pPppIf->schedtime = cyg_current_time();
		
    }
    
    /*
    * Now call all the timeout routines scheduled for this time.
    */
    while (list) 
    {
        (*list->c_func)(list->c_arg, list->pPppIf);
        freep = list;
        list = list->c_next;
        (void) free((char *) freep);
    }
}

/*
 * get_input - Proces PPP_EVENT_INPUT event.
 *
 * Indicates that incoming data is available.
 */
static void
get_input(PPP_IF_VAR_T *pPppIf)
{
    int len, i;
    u_char *p;
    u_short protocol;
    
    MAINDEBUG("IO signal received");
    adjtimeout(pPppIf);   /* Adjust timeouts */
    
    /* Yup, this is for real */
    for (;;) 
    {                    /* Read all available packets */
        p = pPppIf->inpacket_buf;/* point to beggining of packet buffer */
         
        /* get a ppp packet from interface device */
        len = pppread (pPppIf, pPppIf->inpacket_buf, MTU + DLLHEADERLEN);

        if (len <= 0)
            break;
        
        if (len < PPPHEADER_LEN) 
        {
            MAINDEBUG("get_input(): Received short packet.");
            break;
        }
	
        /*  Skip PPP header  */
        len -= PPPHEADER_LEN;

        GETSHORT(protocol, p);
        
        /*
        * Toss all non-LCP packets unless LCP is OPEN.
        */
        if (protocol != LCP && pPppIf->lcp_fsm.state != OPENED) 
        {
            MAINDEBUG("get_input(): Received non-LCP packet and LCP not open.");
            break;
        }
        
        /*
        * Upcall the proper protocol input routine.
        */
        for (i = 0; i < sizeof (prottbl) / sizeof (struct protent); i++)
        {
            if (prottbl[i].protocol == protocol) 
            {
#ifdef MPPE
				if (protocol == CCP && pPppIf->mppe == 0)
					continue;
#endif
                (*prottbl[i].input)(pPppIf, p, len);
                break;
            }
        }  
        
	if (i == sizeof(prottbl) / sizeof(struct protent)) 
	{
	    pPppIf->unknownProto++;
	    MAINDEBUG("input: Unknown protocol (%x) received!", protocol);
			
            lcp_sprotrej(pPppIf, p - PPPHEADER_LEN, len + PPPHEADER_LEN);
        }
    }
}

/*
 * demuxprotrej - Demultiplex a Protocol-Reject.
 */
void
demuxprotrej (PPP_IF_VAR_T *pPppIf, u_short protocol)
{
    int i;
    
    /*
    * Upcall the proper Protocol-Reject routine.
    */
    for (i = 0; i < sizeof (prottbl) / sizeof (struct protent); i++)
    {
        if (prottbl[i].protocol == protocol) 
        {
            (*prottbl[i].protrej)(pPppIf);
            return;
        }
    }
    MAINDEBUG("demuxprotrej: Unrecognized Protocol-Reject for protocol %d!",  protocol);
}

/*
 * pppSendHungupEvent - send interface disconnect event to PPP daemon task.
 */
void pppSendHungupEvent(PPP_IF_VAR_T *pPppIf)
{
    cyg_flag_setbits(&pPppIf->event_id,PPP_EVNET_HUNGUP);
}

/*
 * pppSendInterruptEvent - send interrupt event to PPP daemon task.
 */
void pppSendInterruptEvent(PPP_IF_VAR_T *pPppIf)
{
    cyg_flag_setbits(&pPppIf->event_id,PPP_EVENT_INTERRUPT);
}

/*
 * pppSendTerminateEvent - send terminate event to PPP daemon task.
 */
void pppSendTerminateEvent(PPP_IF_VAR_T *pPppIf)
{
    cyg_flag_setbits(&pPppIf->event_id,PPP_EVENT_TERMINATE);
}

/*
 * pppSendTimerEvent - send timer expiration event to PPP daemon task.
 */
void pppSendTimerEvent(PPP_IF_VAR_T *pPppIf)
{
    PPP_IF_VAR_T  *temp;

    if (ppp_if_var == NULL)
        return ;

    for (temp = ppp_if_var; temp; temp = temp->ppp_next)
    {
        if (temp == pPppIf)
        {
            untimeout(pppSendTimerEvent, (void *)pPppIf);
            cyg_flag_setbits(&pPppIf->event_id,PPP_EVENT_TIMEOUT);
        }        
    }
    
    return;
}

/*
 * pppSendInputEvent - send packet input event to PPP daemon task.
 */
void pppSendInputEvent(PPP_IF_VAR_T *pPppIf)
{
    cyg_flag_setbits(&pPppIf->event_id,PPP_EVENT_INPUT);
}

