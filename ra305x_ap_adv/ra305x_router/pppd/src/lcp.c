/*
 * lcp.c - PPP Link Control Protocol.
 *
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

//#define DEBUGLCP	1
#include <sys_inc.h>
#include <pppd.h>
#include "eventlog.h"

/*
 * Callbacks for fsm code.  (CI = Configuration Information)
 */
static void lcp_resetci     (fsm *);                        /* Reset our CI */
static int  lcp_cilen       (fsm *);                        /* Return length of our CI */
static void lcp_addci       (fsm *, u_char *, int *);       /* Add our CI to pkt */
static int  lcp_ackci       (fsm *, u_char *, int);         /* Peer ack'd our CI */
static int  lcp_nakci       (fsm *, u_char *, int);         /* Peer nak'd our CI */
static int  lcp_rejci       (fsm *, u_char *, int);         /* Peer rej'd our CI */
static int  lcp_reqci       (fsm *, u_char *, int *, int);  /* Rcv peer CI */
static void lcp_up          (fsm *);                        /* We're UP */
static void lcp_down        (fsm *);                        /* We're DOWN */
static void lcp_starting    (fsm *);                        /* We need lower layer up */
static void lcp_finished    (fsm *);                        /* We need lower layer down */
static int  lcp_extcode     (fsm *, int, int, u_char *, int);
static void lcp_rprotrej    (fsm *, u_char *, int);

/*
 * routines to send LCP echos to peer
 */

static void lcp_echo_lowerup        (PPP_IF_VAR_T *pPppIf);
static void lcp_echo_lowerdown      (PPP_IF_VAR_T *pPppIf);
static void lcp_serv_echo_lowerup	(PPP_IF_VAR_T *pPppIf);
static void lcp_serv_echo_lowerdown (PPP_IF_VAR_T *pPppIf);

static void LcpEchoTimeout          (caddr_t);
static void lcp_received_echo_reply (fsm *, int, u_char *, int);
static void LcpSendEchoRequest      (fsm *);
static void LcpLinkFailure          (fsm *);

static fsm_callbacks lcp_callbacks = 
{  /* LCP callback routines */
    lcp_resetci,        /* Reset our Configuration Information */
    lcp_cilen,          /* Length of our Configuration Information */
    lcp_addci,          /* Add our Configuration Information */
    lcp_ackci,          /* ACK our Configuration Information */
    lcp_nakci,          /* NAK our Configuration Information */
    lcp_rejci,          /* Reject our Configuration Information */
    lcp_reqci,          /* Request peer's Configuration Information */
    lcp_up,             /* Called when fsm reaches OPENED state */
    lcp_down,           /* Called when fsm leaves OPENED state */
    lcp_starting,       /* Called when we want the lower layer up */
    lcp_finished,       /* Called when we want the lower layer down */
    NULL,               /* Called when Protocol-Reject received */
    NULL,               /* Retransmission is necessary */
    lcp_extcode,        /* Called to handle LCP-specific codes */
    "LCP"               /* String name of protocol */
};

int lcp_warnloops = DEFWARNLOOPS; /* Warn about a loopback this often */

#define CODENAME(x)	((x) == CONFACK ? "ACK" : \
            (x) == CONFNAK ? "NAK" : "REJ")

//static u_char nak_buffer[PPP_MRU];      /* where we construct a nak packet */

/*
 * lcp_init - Initialize LCP.
 */
void
lcp_init(PPP_IF_VAR_T *pPppIf)
{
	extern	int config_chapms;
	
    fsm *f = &pPppIf->lcp_fsm;
    lcp_options *wo = &pPppIf->lcp_wantoptions;
    lcp_options *ao = &pPppIf->lcp_allowoptions;

    f->pPppIf = pPppIf;
    f->protocol = LCP;
    f->callbacks = &lcp_callbacks;

    fsm_init(f);

    wo->passive = 0;
    wo->silent = 0;
    wo->restart = 0;                    /* Set to 1 in kernels or multi-line implementations */

    wo->neg_mru = 1;
    wo->mru = pPppIf->mru;

    wo->neg_asyncmap = 0;
    wo->asyncmap = 0;
    wo->neg_chap = 0;                   /* Set to 1 on server */
    wo->neg_upap = 0;                   /* Set to 1 on server */

    wo->use_digest = 1;
#ifdef  CHAPMS
    wo->use_chapms_v2 = 0;
    wo->use_chapms = 0;
    if (wo->use_chapms_v2)
        wo->chap_mdtype = CHAP_MICROSOFT_V2;
    else if (wo->use_chapms)
        wo->chap_mdtype = CHAP_MICROSOFT;
    else
#endif
    	wo->chap_mdtype = CHAP_DIGEST_MD5;
		
    wo->neg_magicnumber = 1;
    wo->neg_pcompression = 1;
    wo->neg_accompression = 1;
    wo->neg_lqr = 0;                    /* no LQR implementation yet */

    ao->neg_mru = 1;
    ao->mru = pPppIf->mru;

    ao->neg_asyncmap = 1;
    ao->asyncmap = 0;
    ao->neg_chap = 1;

    ao->use_digest = 1;
#ifdef  CHAPMS
    ao->use_chapms_v2 = ao->use_chapms = config_chapms;
    if(ao->use_chapms_v2)
	    ao->chap_mdtype = CHAP_MICROSOFT_V2;
    else if(ao->use_chapms)
	    ao->chap_mdtype = CHAP_MICROSOFT;
    else
#endif
    ao->chap_mdtype = CHAP_DIGEST_MD5;
    ao->neg_upap = 1;
    ao->neg_magicnumber = 1;
    ao->neg_pcompression = 1;
    ao->neg_accompression = 1;
    ao->neg_lqr = 0;                    /* no LQR implementation yet */

    bzero((char *)pPppIf->xmit_accm, sizeof(pPppIf->xmit_accm[0]));
    pPppIf->xmit_accm[3] = 0x60000000;
}


/*
 * lcp_open - LCP is allowed to come up.
 */
void
lcp_open(PPP_IF_VAR_T *pPppIf)
{
    fsm *f = &pPppIf->lcp_fsm;
    lcp_options *wo = &pPppIf->lcp_wantoptions;

    f->flags = 0;
    if (wo->passive)
        f->flags |= (OPT_PASSIVE | OPT_RESTART);

    if (wo->silent)
        f->flags |= (OPT_SILENT | OPT_RESTART);

    fsm_open(f);
}


/*
 * lcp_close - Take LCP down.
 */
void
lcp_close(PPP_IF_VAR_T *pPppIf)
{
    fsm *f = &pPppIf->lcp_fsm;

    if (f->state == STOPPED && f->flags & (OPT_PASSIVE|OPT_SILENT))
    {
        /*
         * This action is not strictly according to the FSM in RFC1548,
         * but it does mean that the program terminates if you do a
         * lcp_close(0) in passive/silent mode when a connection hasn't
         * been established.
         */
        f->state = CLOSED;
        lcp_finished(f);
    }
    else
    {
        fsm_close(&pPppIf->lcp_fsm);
    }
}


/*
 * lcp_lowerup - The lower layer is up.
 */
void
lcp_lowerup(PPP_IF_VAR_T *pPppIf)
{
    sifdown(pPppIf);
	
    ppp_set_xaccm(pPppIf, pPppIf->xmit_accm);
    ppp_send_config(pPppIf, MTU, 0xffffffff, 0, 0);
    ppp_recv_config(pPppIf, MTU, 0x00000000, 0, 0);
    pPppIf->mru = pPppIf->peer_mru;
    pPppIf->lcp_allowoptions.asyncmap = pPppIf->xmit_accm[0];
	
    fsm_lowerup(&pPppIf->lcp_fsm);
}


/*
 * lcp_lowerdown - The lower layer is down.
 */
void
lcp_lowerdown(PPP_IF_VAR_T *pPppIf)
{
    fsm_lowerdown(&pPppIf->lcp_fsm);
}


/*
 * lcp_input - Input LCP packet.
 */
void
lcp_input(PPP_IF_VAR_T *pPppIf, u_char *p, int len)
{
    fsm_input(&pPppIf->lcp_fsm, p, len);
}


/*
 * lcp_extcode - Handle a LCP-specific code.
 */
static int
lcp_extcode(f, code, id, inp, len)
    fsm *f;
    int code, id;
    u_char *inp;
    int len;
{
    u_char *magp;

    switch( code )
    {
    case PROTREJ:
        lcp_rprotrej(f, inp, len);
        break;

    case ECHOREQ:
        if (f->state != OPENED)
            break;

        LCPDEBUG("lcp: Echo-Request, Rcvd id %d\n", id);
        magp = inp;
        PUTLONG(f->pPppIf->lcp_gotoptions.magicnumber, magp);

        //Panda, Should be 4 bytes
        if (len < (CILEN_LONG-2))
            len = (CILEN_LONG-2);
		
//#ifdef CONFIG_POE_PASSIVE_LCP_ECHO_REPLY
		/* Reset the number of outstanding echo frames */
    	f->pPppIf->lcp_serv_echos_pending = 0;
//#endif    	
        fsm_sdata(f, ECHOREP, id, inp, len);
        break;
    
    case ECHOREP:
        lcp_received_echo_reply(f, id, inp, len);
        break;

    case DISCREQ:
        break;

    default:
        return 0;
    }
    return 1;
}

    
/*
 * lcp_rprotrej - Receive an Protocol-Reject.
 *
 * Figure out which protocol is rejected and inform it.
 */
static void
lcp_rprotrej(f, inp, len)
    fsm *f;
    u_char *inp;
    int len;
{
    u_short prot;

    LCPDEBUG("lcp_rprotrej.\n");

    if (len < sizeof (u_short))
    {
        LCPDEBUG("lcp_rprotrej: Rcvd short Protocol-Reject packet!\n");
        return;
    }

    GETSHORT(prot, inp);

    LCPDEBUG("lcp_rprotrej: Rcvd Protocol-Reject packet for %x!\n", prot);

    /*
     * Protocol-Reject packets received in any state other than the LCP
     * OPENED state SHOULD be silently discarded.
     */
    if( f->state != OPENED )
    {
        LCPDEBUG("Protocol-Reject discarded: LCP in state %d\n", f->state);
        return;
    }

    DEMUXPROTREJ(f->pPppIf, prot);        /* Inform protocol */
}


/*
 * lcp_protrej - A Protocol-Reject was received.
 */
/*ARGSUSED*/
void
lcp_protrej(PPP_IF_VAR_T *pPppIf)
{
    /*
     * Can't reject LCP!
     */
    LCPDEBUG("lcp_protrej: Received Protocol-Reject for LCP!\n");

    fsm_protreject(&pPppIf->lcp_fsm);
}


/*
 * lcp_sprotrej - Send a Protocol-Reject for some protocol.
 */
void
lcp_sprotrej(PPP_IF_VAR_T *pPppIf, u_char *p, int len)
{
    /*
     * Send back the protocol and the information field of the
     * rejected packet.  We only get here if LCP is in the OPENED state.
     */

    fsm_sdata(&pPppIf->lcp_fsm, PROTREJ, ++pPppIf->lcp_fsm.id,
              p, len);
}


/*
 * lcp_resetci - Reset our CI.
 */
static void
  lcp_resetci(f)
fsm *f;
{
    PPP_IF_VAR_T *pPppIf = f->pPppIf;

    pPppIf->lcp_wantoptions.magicnumber = magic();
    pPppIf->lcp_wantoptions.numloops = 0;
    pPppIf->lcp_gotoptions = pPppIf->lcp_wantoptions;
    pPppIf->mru = pPppIf->peer_mru;
}


/*
 * lcp_cilen - Return length of our CI.
 */
static int
lcp_cilen(f)
    fsm *f;
{
    lcp_options *go = &f->pPppIf->lcp_gotoptions;

#define LENCIVOID(neg)      (neg ? CILEN_VOID : 0)
#define LENCICHAP(neg)      (neg ? CILEN_CHAP : 0)
#define LENCISHORT(neg)     (neg ? CILEN_SHORT : 0)
#define LENCILONG(neg)      (neg ? CILEN_LONG : 0)
#define LENCILQR(neg)       (neg ? CILEN_LQR: 0)

    /*
     * NB: we only ask for one of CHAP and UPAP, even if we will
     * accept either.
     */
#ifdef CHAPMS
    if(go->use_chapms_v2)
        go->chap_mdtype = CHAP_MICROSOFT_V2;
    else if(go->use_chapms)
        go->chap_mdtype = CHAP_MICROSOFT;
    else if(go->use_digest)
        go->chap_mdtype = CHAP_DIGEST_MD5;
    else
        go->neg_chap = 0;
#endif

    return (LENCISHORT(go->neg_mru) +
            LENCILONG(go->neg_asyncmap) +
            LENCICHAP(go->neg_chap) +
            LENCISHORT(!go->neg_chap && go->neg_upap) +
            LENCILQR(go->neg_lqr) +
            LENCILONG(go->neg_magicnumber) +
            LENCIVOID(go->neg_pcompression) +
            LENCIVOID(go->neg_accompression));
}


/*
 * lcp_addci - Add our desired CIs to a packet.
 */
static void
lcp_addci(f, ucp, lenp)
    fsm *f;
    u_char *ucp;
    int *lenp;
{
    lcp_options *go = &f->pPppIf->lcp_gotoptions;
    u_char *start_ucp = ucp;

#define ADDCIVOID(opt, neg) \
    if (neg) { \
        PUTCHAR(opt, ucp); \
        PUTCHAR(CILEN_VOID, ucp); \
    }

#define ADDCISHORT(opt, neg, val) \
    if (neg) { \
        PUTCHAR(opt, ucp); \
        PUTCHAR(CILEN_SHORT, ucp); \
        PUTSHORT(val, ucp); \
    }

#define ADDCICHAP(opt, neg, val, digest) \
    if (neg) { \
        PUTCHAR(opt, ucp); \
        PUTCHAR(CILEN_CHAP, ucp); \
        PUTSHORT(val, ucp); \
        PUTCHAR(digest, ucp); \
    }

#define ADDCILONG(opt, neg, val) \
    if (neg) { \
        PUTCHAR(opt, ucp); \
        PUTCHAR(CILEN_LONG, ucp); \
        PUTLONG(val, ucp); \
    }

#define ADDCILQR(opt, neg, val) \
    if (neg) { \
        PUTCHAR(opt, ucp); \
        PUTCHAR(CILEN_LQR, ucp); \
        PUTSHORT(LQR, ucp); \
        PUTLONG(val, ucp); \
    }

    ADDCISHORT  (CI_MRU, go->neg_mru, go->mru);
    ADDCILONG   (CI_ASYNCMAP, go->neg_asyncmap, go->asyncmap);
    ADDCICHAP   (CI_AUTHTYPE, go->neg_chap, CHAP, go->chap_mdtype);
    ADDCISHORT  (CI_AUTHTYPE, !go->neg_chap && go->neg_upap, UPAP);
    ADDCILQR    (CI_QUALITY, go->neg_lqr, go->lqr_period);
    ADDCILONG   (CI_MAGICNUMBER, go->neg_magicnumber, go->magicnumber);
    ADDCIVOID   (CI_PCOMPRESSION, go->neg_pcompression);
    ADDCIVOID   (CI_ACCOMPRESSION, go->neg_accompression);

    if (ucp - start_ucp != *lenp)
    {
        /* this should never happen, because peer_mtu should be 1500 */
        LCPDEBUG("Bug in lcp_addci: wrong length\n");
    }
}


/*
 * lcp_ackci - Ack our CIs.
 * This should not modify any state if the Ack is bad.
 *
 * Returns:
 *	0 - Ack was bad.
 *	1 - Ack was good.
 */
static int
lcp_ackci(f, p, len)
    fsm *f;
    u_char *p;
    int len;
{
    lcp_options *go = &f->pPppIf->lcp_gotoptions;
    u_char cilen, citype, cichar;
    u_short cishort;
    u_long cilong;

    /*
     * CIs must be in exactly the same order that we sent.
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */
#define ACKCIVOID(opt, neg) \
    if (neg) { \
        if ((len -= CILEN_VOID) < 0) \
            goto bad; \
        GETCHAR(citype, p); \
        GETCHAR(cilen, p); \
        if (cilen != CILEN_VOID || \
            citype != opt) \
            goto bad; \
    }

#define ACKCISHORT(opt, neg, val) \
    if (neg) { \
        if ((len -= CILEN_SHORT) < 0) \
            goto bad; \
        GETCHAR(citype, p); \
        GETCHAR(cilen, p); \
        if (cilen != CILEN_SHORT || \
            citype != (u_char)opt) \
            goto bad; \
        GETSHORT(cishort, p); \
        if (cishort != (u_short)val) \
            goto bad; \
    }

#define ACKCICHAP(opt, neg, val, digest) \
    if (neg) { \
        if ((len -= CILEN_CHAP) < 0) \
            goto bad; \
        GETCHAR(citype, p); \
        GETCHAR(cilen, p); \
        if (cilen != CILEN_CHAP || \
            citype != (u_char)opt) \
            goto bad; \
        GETSHORT(cishort, p); \
        if (cishort != (u_short)val) \
            goto bad; \
        GETCHAR(cichar, p); \
        if (cichar != (u_char)digest) \
          goto bad; \
    }

#define ACKCILONG(opt, neg, val) \
    if (neg) { \
        if ((len -= CILEN_LONG) < 0) \
            goto bad; \
        GETCHAR(citype, p); \
        GETCHAR(cilen, p); \
        if (cilen != CILEN_LONG || \
            citype != (u_char)opt) \
            goto bad; \
        GETLONG(cilong, p); \
        if (cilong != (u_long)val) \
            goto bad; \
    }

#define ACKCILQR(opt, neg, val) \
    if (neg) { \
        if ((len -= CILEN_LQR) < 0) \
            goto bad; \
        GETCHAR(citype, p); \
        GETCHAR(cilen, p); \
        if (cilen != CILEN_LQR || \
            citype != (u_char)opt) \
            goto bad; \
        GETSHORT(cishort, p); \
        if (cishort != LQR) \
            goto bad; \
        GETLONG(cilong, p); \
        if (cilong != (u_long)val) \
            goto bad; \
    }

    ACKCISHORT  (CI_MRU, go->neg_mru, go->mru);
    ACKCILONG   (CI_ASYNCMAP, go->neg_asyncmap, go->asyncmap);
    ACKCICHAP   (CI_AUTHTYPE, go->neg_chap, CHAP, go->chap_mdtype);
    ACKCISHORT  (CI_AUTHTYPE, !go->neg_chap && go->neg_upap, UPAP);
    ACKCILQR    (CI_QUALITY, go->neg_lqr, go->lqr_period);
    ACKCILONG   (CI_MAGICNUMBER, go->neg_magicnumber, go->magicnumber);
    ACKCIVOID   (CI_PCOMPRESSION, go->neg_pcompression);
    ACKCIVOID   (CI_ACCOMPRESSION, go->neg_accompression);

    /*
     * If there are any remaining CIs, then this packet is bad.
     */
    if (len != 0)
        goto bad;

    return (1);

bad:
    LCPDEBUG("lcp_acki: received bad Ack!\n");
    return (0);
}


/*
 * lcp_nakci - Peer has sent a NAK for some of our CIs.
 * This should not modify any state if the Nak is bad
 * or if LCP is in the OPENED state.
 *
 * Returns:
 *	0 - Nak was bad.
 *	1 - Nak was good.
 */
static int
lcp_nakci(f, p, len)
    fsm *f;
    u_char *p;
    int len;
{
    lcp_options *go = &f->pPppIf->lcp_gotoptions;
    lcp_options *wo = &f->pPppIf->lcp_wantoptions;
    u_char cilen, citype, cichar, *next;
    u_short cishort = 0;
    u_long cilong;
    lcp_options no;         /* options we've seen Naks for */
    lcp_options try;        /* options to request next time */
    int looped_back = 0;

    BZERO((char *)&no, sizeof(no));
    try = *go;

    /*
     * Any Nak'd CIs must be in exactly the same order that we sent.
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */
#define NAKCIVOID(opt, neg, code) \
    if (go->neg && \
        len >= CILEN_VOID && \
        p[1] == CILEN_VOID && \
        p[0] == opt) { \
        len -= CILEN_VOID; \
        INCPTR(CILEN_VOID, p); \
        no.neg = 1; \
        code \
    }

#define NAKCICHAP(opt, neg, code) \
    if (go->neg && \
        len >= CILEN_CHAP && \
        p[1] == CILEN_CHAP && \
        p[0] == opt) { \
        len -= CILEN_CHAP; \
        INCPTR(2, p); \
        GETSHORT(cishort, p); \
        GETCHAR(cichar, p); \
        no.neg = 1; \
        code \
    }

#define NAKCISHORT(opt, neg, code) \
    if (go->neg && \
        len >= CILEN_SHORT && \
        p[1] == CILEN_SHORT && \
        p[0] == opt) { \
        len -= CILEN_SHORT; \
        INCPTR(2, p); \
        GETSHORT(cishort, p); \
        no.neg = 1; \
        code \
    }

#define NAKCILONG(opt, neg, code) \
    if (go->neg && \
        len >= CILEN_LONG && \
        p[1] == CILEN_LONG && \
        p[0] == opt) { \
        len -= CILEN_LONG; \
        INCPTR(2, p); \
        GETLONG(cilong, p); \
        no.neg = 1; \
        code \
    }

#define NAKCILQR(opt, neg, code) \
    if (go->neg && \
        len >= CILEN_LQR && \
        p[1] == CILEN_LQR && \
        p[0] == opt) { \
        len -= CILEN_LQR; \
        INCPTR(2, p); \
        GETSHORT(cishort, p); \
        GETLONG(cilong, p); \
        no.neg = 1; \
        code \
    }

    /*
     * We don't care if they want to send us smaller packets than
     * we want.  Therefore, accept any MRU less than what we asked for,
     * but then ignore the new value when setting the MRU in the kernel.
     * If they send us a bigger MRU than what we asked, accept it, up to
     * the limit of the default MRU we'd get if we didn't negotiate.
     */
    NAKCISHORT(CI_MRU, neg_mru,
               if (cishort <= wo->mru || cishort < DEFMRU)
                   try.mru = cishort;
              );
    /*
     * Add any characters they want to our (receive-side) asyncmap.
     */
    NAKCILONG(CI_ASYNCMAP, neg_asyncmap,
              try.asyncmap = go->asyncmap | cilong;
              );

    /*
     * If they've nak'd our authentication-protocol, check whether
     * they are proposing a different protocol, or a different
     * hash algorithm for CHAP.
     */
    if ((go->neg_chap || go->neg_upap)
	&& len >= CILEN_SHORT
	&& p[0] == CI_AUTHTYPE && p[1] >= CILEN_SHORT && p[1] <= len) 
    {
        cilen = p[1];
        len -= cilen;
        no.neg_chap = go->neg_chap;
        no.neg_upap = go->neg_upap;
        INCPTR(2, p);
            GETSHORT(cishort, p);
        if (cishort == UPAP && cilen == CILEN_SHORT) 
        {
	        /*
	         * If we were asking for CHAP, they obviously don't want to do it.
	         * If we weren't asking for CHAP, then we were asking for PAP,
	         * in which case this Nak is bad.
	         */
	        if (!go->neg_chap)
	        goto bad;
	        try.neg_chap = 0;

        }
        else if (cishort == CHAP && cilen == CILEN_CHAP) 
        {
	        GETCHAR(cichar, p);
	        if (go->neg_chap)
            {
	            /*
	             * We were asking for CHAP/MD5; they must want a different
	             * algorithm.  If they can't do MD5, we can ask for M$-CHAP
	             * if we support it, otherwise we'll have to stop
	             * asking for CHAP.
	             */
 		        if (go->chap_mdtype == CHAP_MICROSOFT_V2)
 		        {
 		            try.use_chapms_v2 = 0;
 		            if (try.use_chapms)
 			            try.chap_mdtype = CHAP_MICROSOFT;
 		            else if (try.use_digest)
 			            try.chap_mdtype = CHAP_DIGEST_MD5;
 		            else
 			            try.neg_chap = 0;
 		        }
 		        else if (go->chap_mdtype == CHAP_MICROSOFT)
 		        {
 		            try.use_chapms = 0;
 		            if (try.use_digest)
 			            try.chap_mdtype = CHAP_DIGEST_MD5;
 		            else
 			            try.neg_chap = 0;
 		        }
 		        else if(go->chap_mdtype == CHAP_DIGEST_MD5)
 		        {
 		            try.use_digest = 0;
 		            try.neg_chap = 0;
 		        }
 		        else
 		            try.neg_chap = 0;

 		        if ((cichar != CHAP_MICROSOFT_V2) &&
 		            (cichar != CHAP_MICROSOFT) &&
 		            (cichar != CHAP_DIGEST_MD5))
			        try.neg_chap = 0;
	        }
            else
            {
	            /*
	             * Stop asking for PAP if we were asking for it.
	             */
	            try.neg_upap = 0;
	        }
        }
        else
        {
	        /*
	         * We don't recognize what they're suggesting.
	         * Stop asking for what we were asking for.
	         */
	        if (go->neg_chap)
	        try.neg_chap = 0;
	        else
	        try.neg_upap = 0;
	        p += cilen - CILEN_SHORT;
        }
    }


    /*
     * If they can't cope with our link quality protocol, we'll have
     * to stop asking for LQR.  We haven't got any other protocol.
     * If they Nak the reporting period, take their value XXX ?
     */
    NAKCILQR(CI_QUALITY, neg_lqr,
            if (cishort != LQR)
                try.neg_lqr = 0;
            else
                try.lqr_period = cilong;
            );
    /*
     * Check for a looped-back line.
     */
    NAKCILONG(CI_MAGICNUMBER, neg_magicnumber,
              try.magicnumber = magic();
              ++try.numloops;
              looped_back = 1;
              );

    NAKCIVOID(CI_PCOMPRESSION, neg_pcompression,
              try.neg_pcompression = 0;
              );

    NAKCIVOID(CI_ACCOMPRESSION, neg_accompression,
              try.neg_accompression = 0;
              );

    /*
     * There may be remaining CIs, if the peer is requesting negotiation
     * on an option that we didn't include in our request packet.
     * If we see an option that we requested, or one we've already seen
     * in this packet, then this packet is bad.
     * If we wanted to respond by starting to negotiate on the requested
     * option(s), we could, but we don't, because except for the
     * authentication type and quality protocol, if we are not negotiating
     * an option, it is because we were told not to.
     * For the authentication type, the Nak from the peer means
     * `let me authenticate myself with you' which is a bit pointless.
     * For the quality protocol, the Nak means `ask me to send you quality
     * reports', but if we didn't ask for them, we don't want them.
     */
    while (len > CILEN_VOID)
    {
        GETCHAR(citype, p);
        GETCHAR(cilen, p);
        if( (len -= cilen) < 0 )
            goto bad;
        next = p + cilen - 2;

        switch (citype)
        {
        case CI_MRU:
            if (go->neg_mru || no.neg_mru || cilen != CILEN_SHORT)
                goto bad;
            break;

        case CI_ASYNCMAP:
            if (go->neg_asyncmap || no.neg_asyncmap || cilen != CILEN_LONG)
                goto bad;
            break;

        case CI_AUTHTYPE:
            if (go->neg_chap || no.neg_chap || go->neg_upap || no.neg_upap)
                goto bad;
            break;

        case CI_MAGICNUMBER:
            if (go->neg_magicnumber || no.neg_magicnumber || cilen != CILEN_LONG)
                goto bad;
            break;

        case CI_PCOMPRESSION:
            if (go->neg_pcompression || no.neg_pcompression || cilen != CILEN_VOID)
                goto bad;
            break;

        case CI_ACCOMPRESSION:
            if (go->neg_accompression || no.neg_accompression || cilen != CILEN_VOID)
                goto bad;
            break;

        case CI_QUALITY:
            if (go->neg_lqr || no.neg_lqr || cilen != CILEN_LQR)
                goto bad;
            break;

        default:
            goto bad;
        }

        p = next;
    }

    /* If there is still anything left, this packet is bad. */
    if (len != 0)
        goto bad;

    /*
     * OK, the Nak is good.  Now we can update state.
     */
    if (f->state != OPENED)
    {
        *go = try;
        if (looped_back && try.numloops % lcp_warnloops == 0)
            LCPDEBUG("The line appears to be looped back.\n");
    }

    return 1;

bad:
    LCPDEBUG("lcp_nakci: received bad Nak!\n");
    return 0;
}


/*
 * lcp_rejci - Peer has Rejected some of our CIs.
 * This should not modify any state if the Reject is bad
 * or if LCP is in the OPENED state.
 *
 * Returns:
 *	0 - Reject was bad.
 *	1 - Reject was good.
 */
static int
lcp_rejci(f, p, len)
    fsm *f;
    u_char *p;
    int len;
{
    lcp_options *go = &f->pPppIf->lcp_gotoptions;
    u_char cichar = 0;
    u_short cishort;
    u_long cilong;

    u_char *start = p;
    int plen = len;

    lcp_options try;        /* options to request next time */

    try = *go;

    /*
     * Any Rejected CIs must be in exactly the same order that we sent.
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */

#define REJCIVOID(opt, neg) \
    if (go->neg && \
        len >= CILEN_VOID && \
        p[1] == CILEN_VOID && \
        p[0] == opt) { \
        len -= CILEN_VOID; \
        INCPTR(CILEN_VOID, p); \
        try.neg = 0; \
        LCPDEBUG("lcp_rejci rejected void opt %d\n", opt); \
    }

#define REJCISHORT(opt, neg, val) \
    if (go->neg && \
        len >= CILEN_SHORT && \
        p[1] == CILEN_SHORT && \
        p[0] == opt) { \
        len -= CILEN_SHORT; \
        INCPTR(2, p); \
        GETSHORT(cishort, p); \
    /* Check rejected value. */ \
    if (cishort != (u_short)val) \
        goto bad; \
        try.neg = 0; \
        LCPDEBUG("lcp_rejci rejected short opt %d\n", opt); \
    }

#define REJCICHAP(opt, neg, val, digest) \
    if (go->neg && \
        len >= CILEN_CHAP && \
        p[1] == CILEN_CHAP && \
        p[0] == opt) { \
        len -= CILEN_CHAP; \
        INCPTR(2, p); \
        GETSHORT(cishort, p); \
        GETCHAR(cichar, p); \
        /* Check rejected value. */ \
        if (cishort != (u_short)val || cichar != (u_char)digest) \
            goto bad; \
        switch (digest) \
	    { \
	        case CHAP_MICROSOFT_V2: \
		        try.use_chapms_v2 = 0; \
		        break; \
	        case CHAP_MICROSOFT: \
		        try.use_chapms = 0; \
		        break; \
	        case CHAP_DIGEST_MD5: \
		        try.use_digest = 0; \
	    } \
	    if (!try.use_chapms_v2 && !try.use_chapms && !try.use_digest) \
	    { \
	        try.neg = 0; \
	        try.neg_upap = 0; \
	    } \
        /* -- try.neg = 0; */ \
        LCPDEBUG("lcp_rejci rejected chap opt %d\n", opt); \
    }

#define REJCILONG(opt, neg, val) \
    if (go->neg && \
        len >= CILEN_LONG && \
        p[1] == CILEN_LONG && \
        p[0] == opt) { \
        len -= CILEN_LONG; \
        INCPTR(2, p); \
        GETLONG(cilong, p); \
        /* Check rejected value. */ \
        if (cilong != (u_long)val) \
            goto bad; \
        try.neg = 0; \
        LCPDEBUG("lcp_rejci rejected long opt %d\n", opt); \
    }

#define REJCILQR(opt, neg, val) \
    if (go->neg && \
        len >= CILEN_LQR && \
        p[1] == CILEN_LQR && \
        p[0] == opt) { \
        len -= CILEN_LQR; \
        INCPTR(2, p); \
        GETSHORT(cishort, p); \
        GETLONG(cilong, p); \
        /* Check rejected value. */ \
        if (cishort != LQR || cilong != val) \
            goto bad; \
        try.neg = 0; \
        LCPDEBUG("lcp_rejci rejected LQR opt %d\n", opt); \
    }

    REJCISHORT  (CI_MRU, neg_mru, go->mru);
    REJCILONG   (CI_ASYNCMAP, neg_asyncmap, go->asyncmap);
    REJCICHAP   (CI_AUTHTYPE, neg_chap, CHAP, go->chap_mdtype);

    if (!go->neg_chap)
    {
        REJCISHORT(CI_AUTHTYPE, neg_upap, UPAP);
    }
    REJCILQR(CI_QUALITY, neg_lqr, go->lqr_period);
    REJCILONG(CI_MAGICNUMBER, neg_magicnumber, go->magicnumber);
    REJCIVOID(CI_PCOMPRESSION, neg_pcompression);
    REJCIVOID(CI_ACCOMPRESSION, neg_accompression);

    /*
     * If there are any remaining CIs, then this packet is bad.
     */
    if (len != 0)
        goto bad;
    /*
     * Now we can update state.
     */
    if (f->state != OPENED)
        *go = try;

    return 1;

bad:
    LCPDEBUG("lcp_rejci: received bad Reject!\n");
    LCPDEBUG("lcp_rejci: plen %d len %d off %d\n", plen, len, p - start);
    return 0;
}


/*
 * lcp_reqci - Check the peer's requested CIs and send appropriate response.
 *
 * Returns: CONFACK, CONFNAK or CONFREJ and input packet modified
 * appropriately.  If reject_if_disagree is non-zero, doesn't return
 * CONFNAK; returns CONFREJ if it can't return CONFACK.
 */
static int
lcp_reqci(f, inp, lenp, reject_if_disagree)
    fsm *f;
    u_char *inp;                /* Requested CIs */
    int *lenp;                  /* Length of requested CIs */
    int reject_if_disagree;
{
    lcp_options *go = &f->pPppIf->lcp_gotoptions;
    lcp_options *ho = &f->pPppIf->lcp_hisoptions;
    lcp_options *ao = &f->pPppIf->lcp_allowoptions;
    u_char *cip, *next;                 /* Pointer to current and next CIs */
    u_char cilen, citype = 0, cichar;   /* Parsed len, type, char value */
    u_short cishort;                    /* Parsed short value */
    u_long cilong;                      /* Parse long value */
    int rc = CONFACK;                   /* Final packet return code */
    int orc;                            /* Individual option return code */
    u_char *p;                          /* Pointer to next char to parse */
    u_char *rejp;                       /* Pointer to next char in reject frame */
    u_char *nakp;                       /* Pointer to next char in Nak frame */
    int l = *lenp;                      /* Length left */
	u_char *nak_buffer;						/* Pointer to next char in Nak frame */

    /*
     * Reset all his options.
     */
    BZERO((char *)ho, sizeof(*ho));

	nak_buffer =malloc(200);//old value:1500
	if (nak_buffer== NULL)
	{
		diag_printf("\nno memory in lcp_reqci\n");
		return (CONFNAK);
	}

    /*
     * Process all his options.
     */
    next = inp;
    nakp = nak_buffer;
    rejp = inp;
    while (l) 
    {
        orc = CONFACK;              /* Assume success */
        cip = p = next;             /* Remember begining of CI */
        if (l < 2 ||                /* Not enough data for CI header or */
            p[1] < 2 ||             /*  CI length too small or */
            p[1] > l)               /*  CI length too big? */
        {
            LCPDEBUG("lcp_reqci: bad CI length!\n");
            orc = CONFREJ;          /* Reject bad CI */
            cilen = l;              /* Reject till end of packet */
            l = 0;                  /* Don't loop again */
            goto endswitch;
        }

        GETCHAR(citype, p);         /* Parse CI type */
        GETCHAR(cilen, p);          /* Parse CI length */
        l -= cilen;                 /* Adjust remaining length */
        next += cilen;              /* Step to next CI */

        switch (citype)             /* Check CI type */
        {
        case CI_MRU:
            LCPDEBUG("lcp_reqci: rcvd MRU\n");
            if (!ao->neg_mru ||         /* Allow option? */
                cilen != CILEN_SHORT)   /* Check CI length */
            {
                orc = CONFREJ;          /* Reject CI */
                break;
            }

            GETSHORT(cishort, p);       /* Parse MRU */
            LCPDEBUG("(%d)\n", cishort);

            /*
            * He must be able to receive at least our minimum.
            * No need to check a maximum.  If he sends a large number,
            * we'll just ignore it.
            */
            if (cishort < MINMRU)
            {
                orc = CONFNAK;                  /* Nak CI */
                if( !reject_if_disagree )
                {
                    DECPTR(sizeof (short), p);  /* Backup */
                    PUTSHORT(MINMRU, p);        /* Give him a hint */
                }
                break;
            }
            else if (ao->neg_mru && cishort > ao->mru)
            {
                orc = CONFNAK;                      /* Nak CI */
                if( !reject_if_disagree )
                {
                    //DECPTR(sizeof (short), p);    /* Backup */
                    //PUTSHORT(ao->mru, p);         /* Give him a hint */
                    PUTCHAR(CI_MRU, nakp);
                    PUTCHAR(CILEN_SHORT, nakp);
                    PUTSHORT(ao->mru, nakp);
                }
                break;
            }
            
            ho->neg_mru = 1;                        /* Remember he sent MRU */
            ho->mru = cishort;                      /* And remember value */
            break;

        case CI_ASYNCMAP:
            LCPDEBUG("lcp_reqci: rcvd ASYNCMAP\n");
            if (!ao->neg_asyncmap ||
                cilen != CILEN_LONG)
            {
                orc = CONFREJ;
                break;
            }
            GETLONG(cilong, p);
            LCPDEBUG("(%lx)\n", cilong);
            
            /*
            * Asyncmap must have set at least the bits
            * which are set in lcp_allowoptions[unit].asyncmap.
            */
            if ((ao->asyncmap & ~cilong) != 0)
            {
                orc = CONFNAK;
                if( !reject_if_disagree )
                {
                    DECPTR(sizeof (long), p);
                    PUTLONG(ao->asyncmap | cilong, p);
                }
                break;
            }
            ho->neg_asyncmap = 1;
            ho->asyncmap = cilong;
            break;

        case CI_AUTHTYPE:
            LCPDEBUG("lcp_reqci: rcvd AUTHTYPE\n");
            if (cilen < CILEN_SHORT ||
                !(ao->neg_upap || ao->neg_chap))
            {
                orc = CONFREJ;
                if (!ao->neg_upap && !ao->neg_chap)
                    LCPDEBUG(" we're not willing to authenticate\n");
                else
                    LCPDEBUG(" cilen is too short!\n");
                break;
            }
            GETSHORT(cishort, p);
            LCPDEBUG("(%x)\n", cishort);
            
            /*
            * Authtype must be UPAP or CHAP.
            *
            * Note: if both ao->neg_upap and ao->neg_chap are set,
            * and the peer sends a Configure-Request with two
            * authenticate-protocol requests, one for CHAP and one
            * for UPAP, then we will reject the second request.
            * Whether we end up doing CHAP or UPAP depends then on
            * the ordering of the CIs in the peer's Configure-Request.
            */
            
            if (cishort == UPAP)
            {
                if (ho->neg_chap ||         /* we've already accepted CHAP */
                    cilen != CILEN_SHORT)
                {
                    LCPDEBUG("lcp_reqci: rcvd AUTHTYPE PAP, rejecting...\n");
                    orc = CONFREJ;
                    break;
                }

                if (!ao->neg_upap)          /* we don't want to do PAP */
                {
                    orc = CONFNAK;          /* NAK it and suggest CHAP */
                    PUTCHAR(CI_AUTHTYPE, nakp);
                    PUTCHAR(CILEN_CHAP, nakp);
                    PUTSHORT(CHAP, nakp);
                    PUTCHAR(ao->chap_mdtype, nakp);
		    /* XXX if we can do CHAP_MICROSOFT as well, we should
		       probably put in another option saying so */
                    break;
                }
                ho->neg_upap = 1;
                break;
            }

            if (cishort == CHAP)
            {
                if (ho->neg_upap ||         /* we've already accepted PAP */
                    cilen != CILEN_CHAP)
                {
                    LCPDEBUG("lcp_reqci: rcvd AUTHTYPE CHAP, rejecting...\n");
                    orc = CONFREJ;
                    break;
                }

                if (!ao->neg_chap)          /* we don't want to do CHAP */
                {
                    orc = CONFNAK;          /* NAK it and suggest PAP */
                    PUTCHAR(CI_AUTHTYPE, nakp);
                    PUTCHAR(CILEN_SHORT, nakp);
                    PUTSHORT(UPAP, nakp);
                    break;
                }
                GETCHAR(cichar, p);         /* get digest type*/
                
		        if (cichar != CHAP_DIGEST_MD5
#ifdef CHAPMS
		            && cichar != CHAP_MICROSOFT
		            && cichar != CHAP_MICROSOFT_V2
#endif
                    )
                {
                    orc = CONFNAK;
                    PUTCHAR(CI_AUTHTYPE, nakp);
                    PUTCHAR(CILEN_CHAP, nakp);
                    PUTSHORT(CHAP, nakp);
                    PUTCHAR(ao->chap_mdtype, nakp);
                    break;
                }
                ho->chap_mdtype = cichar; /* save md type */
                ho->neg_chap = 1;
                break;
            }
            
            /*
            * We don't recognize the protocol they're asking for.
            * Nak it with something we're willing to do.
            * (At this point we know ao->neg_upap || ao->neg_chap.)
            */
            orc = CONFNAK;
            PUTCHAR(CI_AUTHTYPE, nakp);
            if (ao->neg_chap)
            {
                PUTCHAR(CILEN_CHAP, nakp);
                PUTSHORT(CHAP, nakp);
                PUTCHAR(ao->chap_mdtype, nakp);
            }
            else
            {
                PUTCHAR(CILEN_SHORT, nakp);
                PUTSHORT(UPAP, nakp);
            }
            break;
            
        case CI_QUALITY:
            LCPDEBUG("lcp_reqci: rcvd QUALITY\n");
            if (!ao->neg_lqr ||
                cilen != CILEN_LQR)
            {
                orc = CONFREJ;
                break;
            }
        
            GETSHORT(cishort, p);
            GETLONG(cilong, p);
            LCPDEBUG("(%x %lx)\n", cishort, cilong);
        
            /*
            * Check the protocol and the reporting period.
            * XXX When should we Nak this, and what with?
            */
            if (cishort != LQR)
            {
                orc = CONFNAK;
                PUTCHAR(CI_QUALITY, nakp);
                PUTCHAR(CILEN_LQR, nakp);
                PUTSHORT(LQR, nakp);
                PUTLONG(ao->lqr_period, nakp);
                break;
            }
            break;
        
        case CI_MAGICNUMBER:
            LCPDEBUG("lcp_reqci: rcvd MAGICNUMBER\n");
            if (!(ao->neg_magicnumber || go->neg_magicnumber) ||
                cilen != CILEN_LONG)
            {
                orc = CONFREJ;
                break;
            }
            GETLONG(cilong, p);
            LCPDEBUG("(%lx)\n", cilong);
        
            /*
            * He must have a different magic number.
            */
            if (go->neg_magicnumber &&
                cilong == go->magicnumber)
            {
                orc = CONFNAK;
                cilong = magic();       /* Don't put magic() inside macro! */
                orc = CONFNAK;
                PUTCHAR(CI_MAGICNUMBER, nakp);
                PUTCHAR(CILEN_LONG, nakp);
                PUTLONG(cilong, nakp);
                break;
            }
            ho->neg_magicnumber = 1;
            ho->magicnumber = cilong;
            break;

        case CI_PCOMPRESSION:
            LCPDEBUG("lcp_reqci: rcvd PCOMPRESSION\n");
            if (!ao->neg_pcompression ||
                cilen != CILEN_VOID)
            {
                orc = CONFREJ;
                break;
            }
            ho->neg_pcompression = 1;
            break;

        case CI_ACCOMPRESSION:
            LCPDEBUG("lcp_reqci: rcvd ACCOMPRESSION\n");
            if (!ao->neg_accompression ||
                cilen != CILEN_VOID)
            {
                orc = CONFREJ;
                break;
            }
            ho->neg_accompression = 1;
            break;

        default:
            LCPDEBUG("lcp_reqci: rcvd unknown option %d\n", citype);
            orc = CONFREJ;
            break;
        }

endswitch:
        LCPDEBUG(" (%s)\n", CODENAME(orc));
        if (orc == CONFACK &&           /* Good CI */
            rc != CONFACK)              /*  but prior CI wasnt? */
            continue;                   /* Don't send this one */

        if (orc == CONFNAK)             /* Nak this CI? */
        {
            if (reject_if_disagree      /* Getting fed up with sending NAKs? */
                && citype != CI_MAGICNUMBER)
            {
                orc = CONFREJ;          /* Get tough if so */
            }
            else
            {
                if (rc == CONFREJ)      /* Rejecting prior CI? */
                    continue;           /* Don't send this one */

                rc = CONFNAK;
            }
        }

        if (orc == CONFREJ)             /* Reject this CI */
        {
            rc = CONFREJ;
            if (cip != rejp)            /* Need to move rejected CI? */
                BCOPY(cip, rejp, cilen);/* Move it */

            INCPTR(cilen, rejp);        /* Update output pointer */
        }
    }
    
    /*
    * If we wanted to send additional NAKs (for unsent CIs), the
    * code would go here.  The extra NAKs would go at *nakp.
    * At present there are no cases where we want to ask the
    * peer to negotiate an option.
    */

    switch (rc)
    {
    case CONFACK:
        *lenp = next - inp;
        break;

    case CONFNAK:
        /*
        * Copy the Nak'd options from the nak_buffer to the caller's buffer.
        */
        *lenp = nakp - nak_buffer;
		if(*lenp >= 0 && *lenp <= 200)
        BCOPY(nak_buffer, inp, *lenp);
        break;

    case CONFREJ:
        *lenp = rejp - inp;
        break;
    }

    LCPDEBUG("lcp_reqci: returning CONF%s.\n", CODENAME(rc));
	free(nak_buffer);
    return (rc);            /* Return final code */
}


/*
 * lcp_up - LCP has come UP.
 *
 * Start UPAP, IPCP, etc.
 */
static void
lcp_up(f)
    fsm *f;
{
    lcp_options *wo = &f->pPppIf->lcp_wantoptions;
    lcp_options *ho = &f->pPppIf->lcp_hisoptions;
    lcp_options *go = &f->pPppIf->lcp_gotoptions;
    lcp_options *ao = &f->pPppIf->lcp_allowoptions;

    if (!go->neg_magicnumber)
        go->magicnumber = 0;
    if (!ho->neg_magicnumber)
        ho->magicnumber = 0;

    /*
     * Set our MTU to the smaller of the MTU we wanted and
     * the MRU our peer wanted.  If we negotiated an MRU,
     * set our MRU to the larger of value we wanted and
     * the value we got in the negotiation.
     */
    ppp_send_config(f->pPppIf,
                    MIN(ao->mru? ao->mru: MTU, (ho->neg_mru? ho->mru: MTU)),
                    (ho->neg_asyncmap? ho->asyncmap: 0xffffffff),
                    ho->neg_pcompression, ho->neg_accompression);
    /*
     * If the asyncmap hasn't been negotiated, we really should
     * set the receive asyncmap to ffffffff, but we set it to 0
     * for backwards contemptibility.
     */
    ppp_recv_config(f->pPppIf, (go->neg_mru? MAX(wo->mru, go->mru): MTU),
                    (go->neg_asyncmap? go->asyncmap: 0x00000000),
                    go->neg_pcompression, go->neg_accompression);
					
    if (ho->neg_mru)
        f->pPppIf->mru = ho->mru;
	
    ChapLowerUp(f->pPppIf);           /* Enable CHAP */
    upap_lowerup(f->pPppIf);          /* Enable UPAP */
    ipcp_lowerup(f->pPppIf);          /* Enable IPCP */
	
#ifdef MPPE
	if (f->pPppIf->mppe)
		ccp_lowerup(f->pPppIf);			/* Enable CCP */
#endif

	if (f->pPppIf->lcp_passive_echo_reply)
		lcp_serv_echo_lowerup(f->pPppIf);
	else
		lcp_echo_lowerup(f->pPppIf);      /* Enable echo messages */

    link_established(f);
}


/*
 * lcp_down - LCP has gone DOWN.
 *
 * Alert other protocols.
 */
static void
lcp_down(f)
    fsm *f;
{
	lcp_serv_echo_lowerdown(f->pPppIf);
    lcp_echo_lowerdown(f->pPppIf);
    ipcp_lowerdown(f->pPppIf);
    ChapLowerDown(f->pPppIf);
    upap_lowerdown(f->pPppIf);

    sifdown(f->pPppIf);
    ppp_send_config(f->pPppIf, MTU, 0xffffffff, 0, 0);
    ppp_recv_config(f->pPppIf, MTU, 0x00000000, 0, 0);
    f->pPppIf->mru = f->pPppIf->peer_mru;

    link_down(f);
}


/*
 * lcp_starting - LCP needs the lower layer up.
 */
static void
lcp_starting(f)
    fsm *f;
{
    link_required(f);
}


/*
 * lcp_finished - LCP has finished with the lower layer.
 */
static void
lcp_finished(f)
    fsm *f;
{
    /* if passive or silent flags set and if lcp state is STOPPED or CLOSED 
     * then reopen the link for the next connection
     */
    if (((f->state == STOPPED) || (f->state == CLOSED)) && 
        (f->flags & OPT_RESTART))
        fsm_open (f);
    else 
        link_terminated(f);
}

/*
 * Time to shut down the link because there is nothing out there.
 */

static
void LcpLinkFailure (f)
    fsm *f;
{
    if (f->state == OPENED)
    {
        LCPDEBUG ("Excessive lack of response to LCP echo frames.\n");
        lcp_lowerdown(f->pPppIf);         /* Reset connection */
        f->pPppIf->phase = PHASE_TERMINATE;
    }
}

/*
 * Timer expired for the LCP echo requests from this process.
 */

static void
LcpEchoCheck (f)
    fsm *f;
{
    u_long             delta;
    PPP_IF_VAR_T *pPppIf = f->pPppIf;
//#ifdef __linux__
#if 0
    struct ppp_ddinfo  ddinfo;
    u_long             latest;

   /*
    * Read the time since the last packet was received.
    */
    if (ioctl (fd, PPPIOCGTIME, &ddinfo) < 0)
    {
        LCPDEBUG ("ioctl(PPPIOCGTIME): %m\n");
        
        die (f->pPppIf, 1);
    }

    /*
     * Choose the most recient frame received. It may be an IP or NON-IP frame.
     */
    latest = ddinfo.nip_rjiffies < ddinfo.ip_rjiffies ? ddinfo.nip_rjiffies
                                                      : ddinfo.ip_rjiffies;
    /*
     * Compute the time since the last packet was received. If the timer
     *  has expired then send the echo request and reset the timer to maximum.
     */
    delta = (lcp_echo_interval * HZ) - latest;
    if (delta < HZ || latest < 0L)
    {
        LcpSendEchoRequest (f);
        delta = lcp_echo_interval * HZ;
    }
    delta /= HZ;

#else /* Other implementations do not have ability to find delta */
    if (pPppIf->lcp_line_active != 0) {
	    /*  Do not send echo request if the line is still active  */
	    /*  Clear the line active flag  */
	    pPppIf->lcp_line_active = 0;
	    pPppIf->lcp_echos_pending = 0;
	    delta = pPppIf->lcp_echo_interval;
    } else {
        LcpSendEchoRequest (f);
        if (pPppIf->lcp_echos_pending*4 <= (pPppIf->lcp_echo_fails+3))
	        delta = pPppIf->lcp_echo_interval;
        else if (pPppIf->lcp_echos_pending*2 <= (pPppIf->lcp_echo_fails+1))
	        delta = (pPppIf->lcp_echo_interval + 1)/2;
        else
	        delta = (pPppIf->lcp_echo_interval + 3)/4;
    }
#endif

/*
 * Start the timer for the next interval.
 */
    if (f->pPppIf->lcp_echo_timer_running != 0)
    {
        LCPDEBUG ("lcp_echo_timer_running != 0\n");
        die(f->pPppIf, 1);
    }
    PPP_TIMEOUT (LcpEchoTimeout, (caddr_t) f, delta, f->pPppIf);
    f->pPppIf->lcp_echo_timer_running = 1;
}


/*
 * LcpEchoTimeout - Timer expired on the LCP echo
 */
static void
LcpEchoTimeout (arg)
    caddr_t arg;
{
    fsm *f = (fsm *)arg;
    PPP_IF_VAR_T *pPppIf = f->pPppIf;

    if (f->pPppIf->lcp_echo_timer_running != 0)
    {
        f->pPppIf->lcp_echo_timer_running = 0;
        LcpEchoCheck ((fsm *) arg);
    }
}


/*
 * LcpEchoReply - LCP has received a reply to the echo
 */

static void
lcp_received_echo_reply (f, id, inp, len)
    fsm *f;
    int id; u_char *inp; int len;
{
    u_long magic;

    /* Check the magic number - don't count replies from ourselves. */
#ifdef  notyet
    if (len < CILEN_LONG)
        return;
#endif  /* notyet */

    GETLONG(magic, inp);
    if (f->pPppIf->lcp_gotoptions.neg_magicnumber
        && magic == f->pPppIf->lcp_gotoptions.magicnumber)
        return;

    /* Reset the number of outstanding echo frames */
	f->pPppIf->lcp_echos_pending = 0;
//#ifdef CONFIG_POE_PASSIVE_LCP_ECHO_REPLY
	if (f->pPppIf->lcp_passive_echo_reply)
	{
		lcp_echo_lowerdown(f->pPppIf);
		lcp_serv_echo_lowerup(f->pPppIf);
	}
//#endif    
}

/*
 * LcpSendEchoRequest - Send an echo request frame to the peer
 */

static void
LcpSendEchoRequest (f)
    fsm *f;
{
    u_long lcp_magic;
    u_char pkt[4], *pktp;
    PPP_IF_VAR_T *pPppIf = f->pPppIf;
/*
 * Detect the failure of the peer at this point.
 */
    if (pPppIf->lcp_echo_fails != 0)
    {
        if (pPppIf->lcp_echos_pending++ >= pPppIf->lcp_echo_fails)
        {
			// Simon test here...
            //LcpLinkFailure(f);
            pPppIf->lcp_echos_pending = 0;
			//pPppIf->hungup = 1;
			
			//?????????????????????????
			// Simon test
            //die(pPppIf, 1);
			lcp_close(pPppIf);
        }
    }

/*
 * Make and send the echo request frame.
 */
    if (f->state == OPENED)
    {
        lcp_magic = pPppIf->lcp_gotoptions.neg_magicnumber
                    ? pPppIf->lcp_gotoptions.magicnumber
                    : 0L;
        pktp = pkt;
        PUTLONG(lcp_magic, pktp);

        fsm_sdata(f,
                  ECHOREQ,
                  pPppIf->lcp_echo_number++ & 0xFF,
                  pkt, pktp - pkt);
    }
}

/*
 * lcp_echo_lowerup - Start the timer for the LCP frame
 */

static void
lcp_echo_lowerup (PPP_IF_VAR_T *pPppIf)
{
    fsm *f = &pPppIf->lcp_fsm;

    /* Clear the parameters for generating echo frames */
    pPppIf->lcp_echos_pending      = 0;
    pPppIf->lcp_echo_number        = 0;
    pPppIf->lcp_echo_timer_running = 0;
    pPppIf->lcp_line_active = 0;

    /* If a timeout interval is specified then start the timer */
    if (pPppIf->lcp_echo_interval != 0)
        LcpEchoCheck (f);
}

/*
 * lcp_echo_lowerdown - Stop the timer for the LCP frame
 */

static void
lcp_echo_lowerdown (PPP_IF_VAR_T *pPppIf)
{
    fsm *f = &pPppIf->lcp_fsm;

    if (pPppIf->lcp_echo_timer_running != 0)
    {
        PPP_UNTIMEOUT (LcpEchoTimeout, (caddr_t) f, pPppIf);
        pPppIf->lcp_echo_timer_running = 0;
    }
}
//#ifdef CONFIG_POE_PASSIVE_LCP_ECHO_REPLY
////// PASIVE ECHO REPLY ///////
static void
LcpPasiveEchoTimeout (arg)
    caddr_t arg;
{
	u_long   delta;
    fsm *f = (fsm *)arg;
    PPP_IF_VAR_T *pPppIf = f->pPppIf;

    if (pPppIf->lcp_serv_echo_timer_running != 0)
    {
        if (pPppIf->lcp_serv_echo_fails != 0)
    	{
		if (pPppIf->lcp_line_active != 0) {
			/*  Clear the line active flag  */
			pPppIf->lcp_line_active = 0;
			pPppIf->lcp_serv_echos_pending = 0;
		} else if (pPppIf->lcp_serv_echos_pending++ > pPppIf->lcp_serv_echo_fails ) {
			pPppIf->lcp_serv_echos_pending = 0;
			lcp_serv_echo_lowerdown(pPppIf);
			lcp_echo_lowerup(pPppIf);
            		return;
        	}
        	delta = pPppIf->lcp_serv_echo_interval;
        	PPP_TIMEOUT (LcpPasiveEchoTimeout, (caddr_t) f, delta, pPppIf);
    	}	
    }
}

static void
lcp_serv_echo_lowerup (PPP_IF_VAR_T *pPppIf)
{
	u_long             delta;
    fsm *f = &pPppIf->lcp_fsm;

    /* Clear the parameters for generating echo frames */
    pPppIf->lcp_serv_echos_pending      = 0;
    pPppIf->lcp_serv_echo_timer_running = 0;
	delta = pPppIf->lcp_serv_echo_interval;
	//delta = pPppIf->lcp_serv_echo_interval = pPppIf->lcp_echo_interval;
	//pPppIf->lcp_serv_echo_fails = pPppIf->lcp_echo_fails;
    /* If a timeout interval is specified then start the timer */
    if (pPppIf->lcp_serv_echo_interval != 0)
    {
    	if (f->pPppIf->lcp_serv_echo_timer_running != 0)
    	{
        	LCPDEBUG ("lcp_serv_echo_timer_running = !0\n");
        	die(f->pPppIf, 1);
    	}
    	PPP_TIMEOUT (LcpPasiveEchoTimeout, (caddr_t) f, delta, f->pPppIf);
    	f->pPppIf->lcp_serv_echo_timer_running = 1;
	}	
}


static void
lcp_serv_echo_lowerdown (PPP_IF_VAR_T *pPppIf)
{
    fsm *f = &pPppIf->lcp_fsm;

    if (pPppIf->lcp_serv_echo_timer_running != 0)
    {
        PPP_UNTIMEOUT (LcpPasiveEchoTimeout, (caddr_t) f, pPppIf);
        pPppIf->lcp_serv_echo_timer_running = 0;
    }
}


