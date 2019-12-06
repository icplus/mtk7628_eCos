/*
 * ipcp.c - PPP IP Control Protocol.
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

#include <sys_inc.h>
#include <pppd.h>
#include <ppp_api.h>

#include <cfg_def.h>
#include <cfg_net.h>
#include <cfg_dns.h>
#include <eventlog.h>
#include <sys_status.h>
/*
 * Callbacks for fsm code.  (CI = Configuration Information)
 */
static void ipcp_resetci    __ARGS((fsm *));                        /* Reset our CI */
static int  ipcp_cilen      __ARGS((fsm *));                        /* Return length of our CI */
static void ipcp_addci      __ARGS((fsm *, u_char *, int *));       /* Add our CI */
static int  ipcp_ackci      __ARGS((fsm *, u_char *, int));         /* Peer ack'd our CI */
static int  ipcp_nakci      __ARGS((fsm *, u_char *, int));         /* Peer nak'd our CI */
static int  ipcp_rejci      __ARGS((fsm *, u_char *, int));         /* Peer rej'd our CI */
static int  ipcp_reqci      __ARGS((fsm *, u_char *, int *, int));  /* Rcv CI */
static void ipcp_up         __ARGS((fsm *));                        /* We're UP */
void ipcp_down       __ARGS((fsm *));                        /* We're DOWN */

static fsm_callbacks ipcp_callbacks = { /* IPCP callback routines */
    ipcp_resetci,       /* Reset our Configuration Information */
    ipcp_cilen,         /* Length of our Configuration Information */
    ipcp_addci,         /* Add our Configuration Information */
    ipcp_ackci,         /* ACK our Configuration Information */
    ipcp_nakci,         /* NAK our Configuration Information */
    ipcp_rejci,         /* Reject our Configuration Information */
    ipcp_reqci,         /* Request peer's Configuration Information */
    ipcp_up,            /* Called when fsm reaches OPENED state */
    ipcp_down,          /* Called when fsm leaves OPENED state */
    NULL,               /* Called when we want the lower layer up */
    NULL,               /* Called when we want the lower layer down */
    NULL,               /* Called when Protocol-Reject received */
    NULL,               /* Retransmission is necessary */
    NULL,               /* Called to handle protocol-specific codes */
    "IPCP"              /* String name of protocol */
};


#define CODENAME(x)	((x) == CONFACK ? "ACK" : \
             (x) == CONFNAK ? "NAK" : "REJ")


/*
 * Make a string representation of a network IP address.
 */
char *
ip_ntoa(ipaddr)
    u_long ipaddr;
{
    static char b[64];

    ipaddr = ntohl(ipaddr);

    sprintf(b, "%d.%d.%d.%d",
            (u_char)(ipaddr >> 24),
            (u_char)(ipaddr >> 16),
            (u_char)(ipaddr >> 8),
            (u_char)(ipaddr));
    return b;
}


/*
 * ipcp_init - Initialize IPCP.
 */
void
ipcp_init(PPP_IF_VAR_T *pPppIf)
{
	//int val;
	
    fsm *f = &pPppIf->ipcp_fsm;
    ipcp_options *wo = &pPppIf->ipcp_wantoptions;
    ipcp_options *ao = &pPppIf->ipcp_allowoptions;
	
    f->pPppIf = pPppIf;
    f->protocol = IPCP;
    f->callbacks = &ipcp_callbacks;
    fsm_init(&pPppIf->ipcp_fsm);
	
    wo->neg_addr = 1;
    wo->old_addrs = 0;
	
    wo->neg_vj = 1;
    wo->old_vj = 0;
    wo->neg_pdns = 1;                   //Primary DNS           
    //wo->pdnsaddr = 0x01010101;
    wo->neg_pnbns = 0;                  //Primary NBNS          
    wo->neg_sdns = 1;                   //Secondary DNS         
    wo->neg_snbns = 0;                  //Secondary NBNS        
    wo->vj_protocol = IPCP_VJ_COMP;
    wo->maxslotindex = MAX_STATES - 1;  /* really max index */
    wo->cflag = 1;
	
	wo->pdnsaddr = pPppIf->pri_dns;
	wo->sdnsaddr = pPppIf->snd_dns;
	
    /* max slots and slot-id compression are currently hardwired in */
    /* ppp_if.c to 16 and 1, this needs to be changed (among other */
    /* things) gmc */

    ao->neg_addr = 1;
    ao->neg_vj = 1;
    ao->maxslotindex = MAX_STATES - 1;
    ao->cflag = 1;
}


/*
 * ipcp_open - IPCP is allowed to come up.
 */
void
ipcp_open(PPP_IF_VAR_T *pPppIf)
{
    fsm_open(&pPppIf->ipcp_fsm);
}


/*
 * ipcp_close - Take IPCP down.
 */
void
ipcp_close(PPP_IF_VAR_T *pPppIf)
{
    fsm_close(&pPppIf->ipcp_fsm);
}


/*
 * ipcp_lowerup - The lower layer is up.
 */
void
ipcp_lowerup(PPP_IF_VAR_T *pPppIf)
{
    fsm_lowerup(&pPppIf->ipcp_fsm);
}


/*
 * ipcp_lowerdown - The lower layer is down.
 */
void
ipcp_lowerdown(PPP_IF_VAR_T *pPppIf)
{
    fsm_lowerdown(&pPppIf->ipcp_fsm);
}


/*
 * ipcp_input - Input IPCP packet.
 */
void
ipcp_input(PPP_IF_VAR_T *pPppIf, u_char *p, int len)
{
    fsm_input(&pPppIf->ipcp_fsm, p, len);
}


/*
 * ipcp_protrej - A Protocol-Reject was received for IPCP.
 *
 * Pretend the lower layer went down, so we shut up.
 */
void
ipcp_protrej(PPP_IF_VAR_T *pPppIf)
{
    fsm_lowerdown(&pPppIf->ipcp_fsm);
}


/*
 * ipcp_resetci - Reset our CI.
 */
static void
ipcp_resetci(f)
    fsm *f;
{
    PPP_IF_VAR_T *pPppIf = f->pPppIf;

    ipcp_options *wo = &pPppIf->ipcp_wantoptions;

    wo->req_addr = wo->neg_addr && pPppIf->ipcp_allowoptions.neg_addr;
    if (wo->ouraddr == 0)
        wo->accept_local = 1;

    if (wo->hisaddr == 0)
        wo->accept_remote = 1;
    
    pPppIf->ipcp_gotoptions = *wo;
    pPppIf->cis_received = 0;
}


/*
 * ipcp_cilen - Return length of our CI.
 */
static int
ipcp_cilen(f)
    fsm *f;
{
    ipcp_options *go = &f->pPppIf->ipcp_gotoptions;

#define LENCIVJ(neg, old)   (neg ? (old? CILEN_COMPRESS : CILEN_VJ) : 0)
#define LENCIADDR(neg, old) (neg ? (old? CILEN_ADDRS : CILEN_ADDR) : 0)
#define LENCIPDNSADDR(neg)  (neg ? CILEN_ADDR : 0)  
#define LENCISDNSADDR(neg)  (neg ? CILEN_ADDR : 0)  
#define LENCIPNBNSADDR(neg) (neg ? CILEN_ADDR : 0)  
#define LENCISNBNSADDR(neg) (neg ? CILEN_ADDR : 0)  

    return (LENCIADDR(go->neg_addr, go->old_addrs) +
            LENCIVJ(go->neg_vj, go->old_vj) + LENCIPDNSADDR(go->neg_pdns) + 
            LENCISDNSADDR(go->neg_sdns) + LENCIPNBNSADDR(go->neg_pnbns) + 
            LENCISNBNSADDR(go->neg_snbns));
}


/*
 * ipcp_addci - Add our desired CIs to a packet.
 */
static void
ipcp_addci(f, ucp, lenp)
    fsm *f;
    u_char *ucp;
    int *lenp;
{
    PPP_IF_VAR_T *pPppIf = f->pPppIf;
    ipcp_options *wo = &pPppIf->ipcp_wantoptions;
    ipcp_options *go = &pPppIf->ipcp_gotoptions;
    ipcp_options *ho = &pPppIf->ipcp_hisoptions;
    int len = *lenp;

#define ADDCIVJ(opt, neg, val, old, maxslotindex, cflag) \
    if (neg) { \
        int vjlen = old? CILEN_COMPRESS : CILEN_VJ; \
        if (len >= vjlen) { \
            PUTCHAR(opt, ucp); \
            PUTCHAR(vjlen, ucp); \
            PUTSHORT(val, ucp); \
            if (!old) { \
                PUTCHAR(maxslotindex, ucp); \
                PUTCHAR(cflag, ucp); \
            } \
            len -= vjlen; \
        } else \
            neg = 0; \
    }

#define ADDCIADDR(opt, neg, old, val1, val2) \
    if (neg) { \
        int addrlen = (old? CILEN_ADDRS: CILEN_ADDR); \
        if (len >= addrlen) { \
            u_long l; \
            PUTCHAR(opt, ucp); \
            PUTCHAR(addrlen, ucp); \
            l = ntohl(val1); \
            PUTLONG(l, ucp); \
            if (old) { \
                l = ntohl(val2); \
                PUTLONG(l, ucp); \
            } \
            len -= addrlen; \
        } else \
            neg = 0; \
    }

#define ADDCIPDNSADDR(neg, val1) \
    if (neg) { \
        int addrlen = CILEN_ADDR; \
        if (len >= addrlen) { \
            u_long l; \
            PUTCHAR(CI_PDNSADDRS, ucp); \
            PUTCHAR(addrlen, ucp); \
            l = ntohl(val1); \
            PUTLONG(l, ucp); \
            len -= addrlen; \
        } else \
            neg = 0; \
    }

#define ADDCISDNSADDR(neg, val1) \
    if (neg) { \
        int addrlen = CILEN_ADDR; \
        if (len >= addrlen) { \
            u_long l; \
            PUTCHAR(CI_SDNSADDRS, ucp); \
            PUTCHAR(addrlen, ucp); \
            l = ntohl(val1); \
            PUTLONG(l, ucp); \
            len -= addrlen; \
        } else \
            neg = 0; \
    }

#define ADDCIPNBNSADDR(neg, val1) \
    if (neg) { \
        int addrlen = CILEN_ADDR; \
        if (len >= addrlen) { \
            u_long l; \
            PUTCHAR(CI_PNBNSADDRS, ucp); \
            PUTCHAR(addrlen, ucp); \
            l = ntohl(val1); \
            PUTLONG(l, ucp); \
            len -= addrlen; \
        } else \
            neg = 0; \
    }

#define ADDCISNBNSADDR(neg, val1) \
    if (neg) { \
        int addrlen = CILEN_ADDR; \
        if (len >= addrlen) { \
            u_long l; \
            PUTCHAR(CI_SNBNSADDRS, ucp); \
            PUTCHAR(addrlen, ucp); \
            l = ntohl(val1); \
            PUTLONG(l, ucp); \
            len -= addrlen; \
        } else \
            neg = 0; \
    }

    /*
     * First see if we want to change our options to the old
     * forms because we have received old forms from the peer.
     */
    if (wo->neg_addr && !go->neg_addr && !go->old_addrs)
    {
        /* use the old style of address negotiation */
        go->neg_addr = 1;
        go->old_addrs = 1;
    }

    if (wo->neg_vj && !go->neg_vj && !go->old_vj)
    {
        /* try an older style of VJ negotiation */
        if (pPppIf->cis_received == 0)
        {
            /* keep trying the new style until we see some CI from the peer */
            go->neg_vj = 1;
        }
        else
        {
            /* use the old style only if the peer did */
            if (ho->neg_vj && ho->old_vj)
            {
                go->neg_vj = 1;
                go->old_vj = 1;
                go->vj_protocol = ho->vj_protocol;
            }
        }
    }

    ADDCIADDR((go->old_addrs? CI_ADDRS: CI_ADDR), go->neg_addr,
              go->old_addrs, go->ouraddr, go->hisaddr);

    ADDCIVJ(CI_COMPRESSTYPE, go->neg_vj, go->vj_protocol, go->old_vj,
            go->maxslotindex, go->cflag);

    ADDCIPDNSADDR(go->neg_pdns, go->pdnsaddr);

    ADDCISDNSADDR(go->neg_sdns, go->sdnsaddr);

    ADDCIPNBNSADDR(go->neg_pnbns, go->pnbnsaddr);

    ADDCISNBNSADDR(go->neg_snbns, go->snbnsaddr);

    *lenp -= len;
}


/*
 * ipcp_ackci - Ack our CIs.
 *
 * Returns:
 *  0 - Ack was bad.
 *  1 - Ack was good.
 */
static int
ipcp_ackci(f, p, len)
    fsm *f;
    u_char *p;
    int len;
{
    ipcp_options *go = &f->pPppIf->ipcp_gotoptions;
    u_short cilen, citype, cishort;
    u_long cilong;
    u_char cimaxslotindex, cicflag;

    /*
     * CIs must be in exactly the same order that we sent...
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */

#define ACKCIVJ(opt, neg, val, old, maxslotindex, cflag) \
    if (neg) { \
        int vjlen = old? CILEN_COMPRESS : CILEN_VJ; \
        if ((len -= vjlen) < 0) \
            goto bad; \
        GETCHAR(citype, p); \
        GETCHAR(cilen, p); \
        if (cilen != vjlen || \
            citype != opt)  \
            goto bad; \
        GETSHORT(cishort, p); \
        if (cishort != val) \
            goto bad; \
        if (!old) { \
            GETCHAR(cimaxslotindex, p); \
            if (cimaxslotindex != maxslotindex) \
                goto bad; \
            GETCHAR(cicflag, p); \
            if (cicflag != cflag) \
                goto bad; \
        } \
    }

#define ACKCIADDR(opt, neg, old, val1, val2) \
    if (neg) { \
        int addrlen = (old? CILEN_ADDRS: CILEN_ADDR); \
        u_long l; \
        if ((len -= addrlen) < 0) \
            goto bad; \
        GETCHAR(citype, p); \
        GETCHAR(cilen, p); \
        if (cilen != addrlen || \
            citype != opt) \
            goto bad; \
        GETLONG(l, p); \
        cilong = htonl(l); \
        if (val1 != cilong) \
            goto bad; \
        if (old) { \
            GETLONG(l, p); \
            cilong = htonl(l); \
            if (val2 != cilong) \
            goto bad; \
        } \
    }

#define ACKCIPDNSADDR(opt, neg, val1) \
    if (neg) { \
        int addrlen = CILEN_ADDR; \
        u_long l; \
        if ((len -= addrlen) < 0) \
            goto bad; \
        GETCHAR(citype, p); \
        GETCHAR(cilen, p); \
        if (cilen != addrlen || \
            citype != opt) \
            goto bad; \
        GETLONG(l, p); \
        cilong = htonl(l); \
        if (val1 != cilong) \
            goto bad; \
    }

#define ACKCISDNSADDR(opt, neg, val1) \
    if (neg) { \
        int addrlen = CILEN_ADDR; \
        u_long l; \
        if ((len -= addrlen) < 0) \
            goto bad; \
        GETCHAR(citype, p); \
        GETCHAR(cilen, p); \
        if (cilen != addrlen || \
            citype != opt) \
            goto bad; \
        GETLONG(l, p); \
        cilong = htonl(l); \
        if (val1 != cilong) \
            goto bad; \
    }

#define ACKCIPNBNSADDR(opt, neg, val1) \
    if (neg) { \
        int addrlen = CILEN_ADDR; \
        u_long l; \
        if ((len -= addrlen) < 0) \
            goto bad; \
        GETCHAR(citype, p); \
        GETCHAR(cilen, p); \
        if (cilen != addrlen || \
            citype != opt) \
            goto bad; \
        GETLONG(l, p); \
        cilong = htonl(l); \
        if (val1 != cilong) \
            goto bad; \
    }

#define ACKCISNBNSADDR(opt, neg, val1) \
    if (neg) { \
        int addrlen = CILEN_ADDR; \
        u_long l; \
        if ((len -= addrlen) < 0) \
            goto bad; \
        GETCHAR(citype, p); \
        GETCHAR(cilen, p); \
        if (cilen != addrlen || \
            citype != opt) \
            goto bad; \
        GETLONG(l, p); \
        cilong = htonl(l); \
        if (val1 != cilong) \
            goto bad; \
    }

    ACKCIADDR((go->old_addrs? CI_ADDRS: CI_ADDR), go->neg_addr,
              go->old_addrs, go->ouraddr, go->hisaddr);

    ACKCIVJ(CI_COMPRESSTYPE, go->neg_vj, go->vj_protocol, go->old_vj,
            go->maxslotindex, go->cflag);

    ACKCIPDNSADDR(CI_PDNSADDRS, go->neg_pdns, go->pdnsaddr);        //panda+

    ACKCISDNSADDR(CI_SDNSADDRS, go->neg_sdns, go->sdnsaddr);        //panda+

    ACKCIPNBNSADDR(CI_PNBNSADDRS, go->neg_pnbns, go->pnbnsaddr);    //panda+

    ACKCISNBNSADDR(CI_SNBNSADDRS, go->neg_snbns, go->snbnsaddr);    //panda+

    /*
     * If there are any remaining CIs, then this packet is bad.
     */
    if (len != 0)
        goto bad;
    return (1);

bad:
    IPCPDEBUG("ipcp_ackci: received bad Ack!");
    return (0);
}

/*
 * ipcp_nakci - Peer has sent a NAK for some of our CIs.
 * This should not modify any state if the Nak is bad
 * or if IPCP is in the OPENED state.
 *
 * Returns:
 *  0 - Nak was bad.
 *  1 - Nak was good.
 */
static int
ipcp_nakci(f, p, len)
    fsm *f;
    u_char *p;
    int len;
{
    ipcp_options *go = &f->pPppIf->ipcp_gotoptions;
    u_char cimaxslotindex = 0, cicflag = 0;
    u_char citype, cilen, *next;
    u_short cishort;
    u_long ciaddr1, ciaddr2, l;
    ipcp_options no;                /* options we've seen Naks for */
    ipcp_options try;               /* options to request next time */

    BZERO((char *)&no, sizeof(no));
    try = *go;

    /*
     * Any Nak'd CIs must be in exactly the same order that we sent.
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */
#define NAKCIADDR(opt, neg, old, code) \
    if (go->neg && \
        len >= (cilen = (old? CILEN_ADDRS: CILEN_ADDR)) && \
        p[1] == cilen && \
        p[0] == opt) \
    { \
        len -= cilen; \
        INCPTR(2, p); \
        GETLONG(l, p); \
        ciaddr1 = htonl(l); \
        if (old) { \
            GETLONG(l, p); \
            ciaddr2 = htonl(l); \
            no.old_addrs = 1; \
        } else \
            ciaddr2 = 0; \
        no.neg = 1; \
        code \
    }

#define NAKCIVJ(opt, neg, code) \
    if (go->neg && \
        ((cilen = p[1]) == CILEN_COMPRESS || cilen == CILEN_VJ) && \
        len >= cilen && \
        p[0] == opt) \
    { \
        len -= cilen; \
        INCPTR(2, p); \
        GETSHORT(cishort, p); \
        no.neg = 1; \
        code \
    }

#define NAKCIPDNSADDR(opt, neg, code) \
    if (go->neg && \
        len >= (cilen = CILEN_ADDR) && \
        p[1] == cilen && \
        p[0] == opt) \
    { \
        len -= cilen; \
        INCPTR(2, p); \
        GETLONG(l, p); \
        ciaddr1 = htonl(l); \
        no.neg = 1; \
        code \
    }

#define NAKCISDNSADDR(opt, neg, code) \
    if (go->neg && \
        len >= (cilen = CILEN_ADDR) && \
        p[1] == cilen && \
        p[0] == opt) \
    { \
        len -= cilen; \
        INCPTR(2, p); \
        GETLONG(l, p); \
        ciaddr1 = htonl(l); \
        no.neg = 1; \
        code \
    }

#define NAKCIPNBNSADDR(opt, neg, code) \
    if (go->neg && \
        len >= (cilen = CILEN_ADDR) && \
        p[1] == cilen && \
        p[0] == opt) \
    { \
        len -= cilen; \
        INCPTR(2, p); \
        GETLONG(l, p); \
        ciaddr1 = htonl(l); \
        no.neg = 1; \
        code \
    }

#define NAKCISNBNSADDR(opt, neg, code) \
    if (go->neg && \
        len >= (cilen = CILEN_ADDR) && \
        p[1] == cilen && \
        p[0] == opt) \
    { \
        len -= cilen; \
        INCPTR(2, p); \
        GETLONG(l, p); \
        ciaddr1 = htonl(l); \
        no.neg = 1; \
        code \
    }


    /*
     * Accept the peer's idea of {our,his} address, if different
     * from our idea, only if the accept_{local,remote} flag is set.
     */
    NAKCIADDR(CI_ADDR, neg_addr, go->old_addrs,
        if (go->accept_local && ciaddr1) /* Do we know our address? */
        {
          try.ouraddr = ciaddr1;
          IPCPDEBUG("local IP address %s", ip_ntoa(ciaddr1));
        }
        if (go->accept_remote && ciaddr2) { /* Does he know his? */
          try.hisaddr = ciaddr2;
          IPCPDEBUG("remote IP address %s", ip_ntoa(ciaddr2));
        }
    );

    /*
     * Accept the peer's value of maxslotindex provided that it
     * is less than what we asked for.  Turn off slot-ID compression
     * if the peer wants.  Send old-style compress-type option if
     * the peer wants.
     */
    NAKCIVJ(CI_COMPRESSTYPE, neg_vj,
        if (cilen == CILEN_VJ)
        {
            GETCHAR(cimaxslotindex, p);
            GETCHAR(cicflag, p);
            if (cishort == IPCP_VJ_COMP)
            {
                try.old_vj = 0;
                if (cimaxslotindex < go->maxslotindex)
                    try.maxslotindex = cimaxslotindex;
                if (!cicflag)
                    try.cflag = 0;
            }
            else
            {
                try.neg_vj = 0;
            }
        }
        else
        {
            if (cishort == IPCP_VJ_COMP || cishort == IPCP_VJ_COMP_OLD)
            {
                try.old_vj = 1;
                try.vj_protocol = cishort;
            }
            else
            {
                try.neg_vj = 0;
            }
        }
    );

    NAKCIPDNSADDR(CI_PDNSADDRS, neg_pdns,
        if (go->accept_local && ciaddr1)    /* Do we know our Primary DNS address? */
        {
            try.pdnsaddr = ciaddr1;
            IPCPDEBUG("local Primary DNS address %s", ip_ntoa(ciaddr1));
        }
    );

    NAKCIPDNSADDR(CI_SDNSADDRS, neg_sdns,
        if (go->accept_local && ciaddr1)    /* Do we know our Secondary DNS address? */
        {
            try.sdnsaddr = ciaddr1;
            IPCPDEBUG("local Secondary DNS address %s", ip_ntoa(ciaddr1));
        }
    );

    NAKCIPDNSADDR(CI_PNBNSADDRS, neg_pnbns,
        if (go->accept_local && ciaddr1)    /* Do we know our Primary NBNS address? */
        {
            try.pnbnsaddr = ciaddr1;
            IPCPDEBUG("local Primary NBNS address %s", ip_ntoa(ciaddr1));
        }
    );

    NAKCIPDNSADDR(CI_SNBNSADDRS, neg_snbns,
        if (go->accept_local && ciaddr1)    /* Do we know our Secondary NBNS address? */
        {
            try.snbnsaddr = ciaddr1;
            IPCPDEBUG("local Secondary NBNS address %s", ip_ntoa(ciaddr1));
        }
    );

    /*
     * There may be remaining CIs, if the peer is requesting negotiation
     * on an option that we didn't include in our request packet.
     * If they want to negotiate about IP addresses, we comply.
     * If they want us to ask for compression, we refuse.
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
        case CI_COMPRESSTYPE:
            if (go->neg_vj || no.neg_vj || (cilen != CILEN_VJ && cilen != CILEN_COMPRESS))
                goto bad;
            no.neg_vj = 1;
            break;

        case CI_ADDRS:
            if ((go->neg_addr && go->old_addrs) || no.old_addrs || cilen != CILEN_ADDRS)
                goto bad;
            try.neg_addr = 1;
            try.old_addrs = 1;
            GETLONG(l, p);
            ciaddr1 = htonl(l);
            if (ciaddr1 && go->accept_local)
                try.ouraddr = ciaddr1;
            GETLONG(l, p);
            ciaddr2 = htonl(l);
            if (ciaddr2 && go->accept_remote)
                try.hisaddr = ciaddr2;
            no.old_addrs = 1;
            break;

        case CI_ADDR:
            if (go->neg_addr || no.neg_addr || cilen != CILEN_ADDR)
                goto bad;
            try.neg_addr = 1;
            try.old_addrs = 0;
            GETLONG(l, p);
            ciaddr1 = htonl(l);
            if (ciaddr1 && go->accept_local)
                try.ouraddr = ciaddr1;
            no.neg_addr = 1;
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
        *go = try;

    return 1;

bad:
    IPCPDEBUG("ipcp_nakci: received bad Nak!");
    return 0;
}


/*
 * ipcp_rejci - Reject some of our CIs.
 */
static int
ipcp_rejci(f, p, len)
    fsm *f;
    u_char *p;
    int len;
{
    ipcp_options *go = &f->pPppIf->ipcp_gotoptions;
    u_char cimaxslotindex, ciflag, cilen;
    u_short cishort;
    u_long cilong;
    ipcp_options try;       /* options to request next time */

    try = *go;

    /*
     * Bring down link if peer rejects the IPCP address configuration option,
     * since if the IP address can't be negotiated then the link is useless.
     */
    /*
     * Any Rejected CIs must be in exactly the same order that we sent.
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */
#define REJCIADDR(opt, neg, old, val1, val2) \
    if (go->neg && \
        len >= (cilen = old? CILEN_ADDRS: CILEN_ADDR) && \
        p[1] == cilen && \
        p[0] == opt) \
    { \
        u_long l; \
        die(f->pPppIf, 1); /* not to protocol spec. see note in ipcp_close() */ \
        len -= cilen; \
        INCPTR(2, p); \
        GETLONG(l, p); \
        cilong = htonl(l); \
        /* Check rejected value. */ \
        if (cilong != val1) \
            goto bad; \
        if (old) \
        { \
            GETLONG(l, p); \
            cilong = htonl(l); \
            /* Check rejected value. */ \
            if (cilong != val2) \
            goto bad; \
        } \
        try.neg = 0; \
    }

#define REJCIVJ(opt, neg, val, old, maxslot, cflag) \
    if (go->neg && \
        p[1] == (old? CILEN_COMPRESS : CILEN_VJ) && \
        len >= p[1] && \
        p[0] == opt) \
    { \
        len -= p[1]; \
        INCPTR(2, p); \
        GETSHORT(cishort, p); \
        /* Check rejected value. */  \
        if (cishort != val) \
            goto bad; \
        if (!old) \
        { \
            GETCHAR(cimaxslotindex, p); \
            if (cimaxslotindex != maxslot) \
                goto bad; \
            GETCHAR(ciflag, p); \
            if (ciflag != cflag) \
                goto bad; \
        } \
        try.neg = 0; \
    }

#define REJCIPDNSADDR(opt, neg, val1) \
    if (go->neg && \
        len >= (cilen = CILEN_ADDR) && \
        p[1] == cilen && \
        p[0] == opt) \
    { \
        u_long l; \
        len -= cilen; \
        INCPTR(2, p); \
        GETLONG(l, p); \
        cilong = htonl(l); \
        if (cilong != val1) \
            goto bad; \
        try.neg = 0; \
    }

#define REJCISDNSADDR(opt, neg, val1) \
    if (go->neg && \
        len >= (cilen = CILEN_ADDR) && \
        p[1] == cilen && \
        p[0] == opt) \
    { \
        u_long l; \
        len -= cilen; \
        INCPTR(2, p); \
        GETLONG(l, p); \
        cilong = htonl(l); \
        if (cilong != val1) \
            goto bad; \
        try.neg = 0; \
    }

#define REJCIPNBNSADDR(opt, neg, val1) \
    if (go->neg && \
        len >= (cilen = CILEN_ADDR) && \
        p[1] == cilen && \
        p[0] == opt) \
    { \
        u_long l; \
        len -= cilen; \
        INCPTR(2, p); \
        GETLONG(l, p); \
        cilong = htonl(l); \
        if (cilong != val1) \
            goto bad; \
        try.neg = 0; \
    }

#define REJCISNBNSADDR(opt, neg, val1) \
    if (go->neg && \
        len >= (cilen = CILEN_ADDR) && \
        p[1] == cilen && \
        p[0] == opt) \
    { \
        u_long l; \
        len -= cilen; \
        INCPTR(2, p); \
        GETLONG(l, p); \
        cilong = htonl(l); \
        if (cilong != val1) \
            goto bad; \
        try.neg = 0; \
    }

    REJCIADDR((go->old_addrs? CI_ADDRS: CI_ADDR), neg_addr,
              go->old_addrs, go->ouraddr, go->hisaddr);

    REJCIVJ(CI_COMPRESSTYPE, neg_vj, go->vj_protocol, go->old_vj,
            go->maxslotindex, go->cflag);

    REJCIPDNSADDR(CI_PDNSADDRS, neg_pdns, go->pdnsaddr); 

    REJCISDNSADDR(CI_SDNSADDRS, neg_sdns, go->sdnsaddr); 

    REJCIPNBNSADDR(CI_PNBNSADDRS, neg_pnbns, go->pnbnsaddr);

    REJCISNBNSADDR(CI_SNBNSADDRS, neg_snbns, go->snbnsaddr); 

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
    IPCPDEBUG("ipcp_rejci: received bad Reject!");
    return 0;
}

/*
 * ipcp_reqci - Check the peer's requested CIs and send appropriate response.
 *
 * Returns: CONFACK, CONFNAK or CONFREJ and input packet modified
 * appropriately.  If reject_if_disagree is non-zero, doesn't return
 * CONFNAK; returns CONFREJ if it can't return CONFACK.
 */
static int
ipcp_reqci(f, inp, len, reject_if_disagree)
    fsm *f;
    u_char *inp;                /* Requested CIs */
    int *len;                   /* Length of requested CIs */
    int reject_if_disagree;
{
    ipcp_options *wo = &f->pPppIf->ipcp_wantoptions;
    ipcp_options *ho = &f->pPppIf->ipcp_hisoptions;
    ipcp_options *ao = &f->pPppIf->ipcp_allowoptions;
    ipcp_options *go = &f->pPppIf->ipcp_gotoptions;
    u_char *cip, *next;             /* Pointer to current and next CIs */
    u_short cilen, citype;          /* Parsed len, type */
    u_short cishort;                /* Parsed short value */
    u_long tl, ciaddr1, ciaddr2;    /* Parsed address values */
    int rc = CONFACK;               /* Final packet return code */
    int orc;                        /* Individual option return code */
    u_char *p;                      /* Pointer to next char to parse */
    u_char *ucp = inp;              /* Pointer to current output char */
    int l = *len;                   /* Length left */
    u_char maxslotindex, cflag;

    /*
     * Reset all his options.
     */
    BZERO((char *)ho, sizeof(*ho));
    
    /*
     * Process all his options.
     */
    next = inp;
    while (l)
    {
        orc = CONFACK;          /* Assume success */
        cip = p = next;         /* Remember begining of CI */
        if (l < 2 ||            /* Not enough data for CI header or */
            p[1] < 2 ||         /*  CI length too small or */
            p[1] > l)           /*  CI length too big? */
        {
            IPCPDEBUG("ipcp_reqci: bad CI length!");
            orc = CONFREJ;      /* Reject bad CI */
            cilen = l;          /* Reject till end of packet */
            l = 0;              /* Don't loop again */
            goto endswitch;
        }

        GETCHAR(citype, p);     /* Parse CI type */
        GETCHAR(cilen, p);      /* Parse CI length */
        l -= cilen;             /* Adjust remaining length */
        next += cilen;          /* Step to next CI */

        switch (citype)                 /* Check CI type */
        {
        case CI_ADDRS:
            IPCPDEBUG("ipcp: received ADDRS ");
            if (!ao->neg_addr ||
                cilen != CILEN_ADDRS)   /* Check CI length */
            {
                orc = CONFREJ;          /* Reject CI */
                break;
            }
            
            /*
            * If he has no address, or if we both have his address but
            * disagree about it, then NAK it with our idea.
            * In particular, if we don't know his address, but he does,
            * then accept it.
            */
            GETLONG(tl, p);             /* Parse source address (his) */
            ciaddr1 = htonl(tl);
            IPCPDEBUG("(%s:", ip_ntoa(ciaddr1));
            if (ciaddr1 != wo->hisaddr
                && (ciaddr1 == 0 || !wo->accept_remote))
            {
                orc = CONFNAK;
                if (!reject_if_disagree)
                {
                    DECPTR(sizeof (long), p);
                    tl = ntohl(wo->hisaddr);
                    PUTLONG(tl, p);
                }
            }

            /*
            * If he doesn't know our address, or if we both have our address
            * but disagree about it, then NAK it with our idea.
            */
            GETLONG(tl, p);         /* Parse desination address (ours) */
            ciaddr2 = htonl(tl);
            IPCPDEBUG("(%s)", ip_ntoa(ciaddr2));
            if (ciaddr2 != wo->ouraddr)
            {
                if (ciaddr2 == 0 || !wo->accept_local)
                {
                    orc = CONFNAK;
                    if (!reject_if_disagree)
                    {
                        DECPTR(sizeof (long), p);
                        tl = ntohl(wo->ouraddr);
                        PUTLONG(tl, p);
                    }
                }
                else
                {
                    go->ouraddr = ciaddr2;      /* accept peer's idea */
                }
            }

            ho->neg_addr = 1;
            ho->old_addrs = 1;
            ho->hisaddr = ciaddr1;
            ho->ouraddr = ciaddr2;
            break;

        case CI_ADDR:
            IPCPDEBUG("ipcp: received ADDR ");

            if (!ao->neg_addr ||
                cilen != CILEN_ADDR)    /* Check CI length */
            {
                orc = CONFREJ;          /* Reject CI */
                break;
            }
            
            /*
            * If he has no address, or if we both have his address but
            * disagree about it, then NAK it with our idea.
            * In particular, if we don't know his address, but he does,
            * then accept it.
            */
            GETLONG(tl, p);	/* Parse source address (his) */
            ciaddr1 = htonl(tl);
            IPCPDEBUG("(%s)", ip_ntoa(ciaddr1));
            if (ciaddr1 != wo->hisaddr
                && (ciaddr1 == 0 || !wo->accept_remote))
            {
                orc = CONFNAK;
                if (!reject_if_disagree)
                {
                    DECPTR(sizeof (long), p);
                    tl = ntohl(wo->hisaddr);
                    PUTLONG(tl, p);
                }
            }
            
            ho->neg_addr = 1;
            ho->hisaddr = ciaddr1;
            break;
            
        case CI_COMPRESSTYPE:
            IPCPDEBUG("ipcp: received COMPRESSTYPE ");
            if (!ao->neg_vj ||
                (cilen != CILEN_VJ && cilen != CILEN_COMPRESS))
            {
                orc = CONFREJ;
                break;
            }
            GETSHORT(cishort, p);
            IPCPDEBUG("(%d)", cishort);
            
            if (!(cishort == IPCP_VJ_COMP ||
                (cishort == IPCP_VJ_COMP_OLD && cilen == CILEN_COMPRESS)))
            {
                orc = CONFREJ;
                break;
            }
            
            ho->neg_vj = 1;
            ho->vj_protocol = cishort;
            if (cilen == CILEN_VJ)
            {
                GETCHAR(maxslotindex, p);
                if (maxslotindex > ao->maxslotindex)
                {
                    orc = CONFNAK;
                    if (!reject_if_disagree)
                    {
                        DECPTR(1, p);
                        PUTCHAR(ao->maxslotindex, p);
                    }
                }
                GETCHAR(cflag, p);
                if (cflag && !ao->cflag)
                {
                    orc = CONFNAK;
                    if (!reject_if_disagree)
                    {
                        DECPTR(1, p);
                        PUTCHAR(wo->cflag, p);
                    }
                }
                ho->maxslotindex = maxslotindex;
                ho->cflag = wo->cflag;
            }
            else
            {
                ho->old_vj = 1;
                ho->maxslotindex = MAX_STATES - 1;
                ho->cflag = 1;
            }
            break;
            
        default:
            orc = CONFREJ;
            break;
        }

endswitch:
        IPCPDEBUG(" (%s)", CODENAME(orc));

        if (orc == CONFACK &&       /* Good CI */
            rc != CONFACK)          /*  but prior CI wasnt? */
            continue;               /* Don't send this one */

        if (orc == CONFNAK)         /* Nak this CI? */
        {
            if (reject_if_disagree) /* Getting fed up with sending NAKs? */
                orc = CONFREJ;      /* Get tough if so */
            else
            {
                if (rc == CONFREJ)  /* Rejecting prior CI? */
                    continue;       /* Don't send this one */
                if (rc == CONFACK)  /* Ack'd all prior CIs? */
                {
                    rc = CONFNAK;   /* Not anymore... */
                    ucp = inp;      /* Backup */
                }
            }
        }

        if (orc == CONFREJ &&       /* Reject this CI */
            rc != CONFREJ)          /*  but no prior ones? */
        {
            rc = CONFREJ;
            ucp = inp;              /* Backup */
        }

        /* Need to move CI? */
        if (ucp != cip)
            BCOPY((char *)cip, (char *)ucp, cilen);	/* Move it */

        /* Update output pointer */
        INCPTR(cilen, ucp);
    }
    
    /*
    * If we aren't rejecting this packet, and we want to negotiate
    * their address, and they didn't send their address, then we
    * send a NAK with a CI_ADDR option appended.  We assume the
    * input buffer is long enough that we can append the extra
    * option safely.
    */
    if (rc != CONFREJ && !ho->neg_addr &&
        wo->req_addr && !reject_if_disagree)
    {
        if (rc == CONFACK)
        {
            rc = CONFNAK;
            ucp = inp;              /* reset pointer */
            wo->req_addr = 0;       /* don't ask again */
        }

        PUTCHAR(CI_ADDR, ucp);
        PUTCHAR(CILEN_ADDR, ucp);
        tl = ntohl(wo->hisaddr);
        PUTLONG(tl, ucp);
    }

    *len = ucp - inp;           /* Compute output length */
    IPCPDEBUG("ipcp: returning Configure-%s", CODENAME(rc));
    return (rc);                /* Return final code */
}


/*
 * ipcp_up - IPCP has come UP.
 *
 * Configure the IP network interface appropriately and bring it up.
 */
static void
ipcp_up(f)
    fsm *f;
{
	PPP_IF_VAR_T *pPppIf = f->pPppIf;
	ipcp_options *ho = &pPppIf->ipcp_hisoptions;
	ipcp_options *go = &pPppIf->ipcp_gotoptions;
	
	
	IPCPDEBUG("ipcp: up");
	SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPD,"ipcp up");
	
	go->default_route = 0;
	go->proxy_arp = 0;
	
	/*
	 * We must have a non-zero IP address for both ends of the link.
	 */
	if (!ho->neg_addr)
		ho->hisaddr = pPppIf->ipcp_wantoptions.hisaddr;
	
	if (ho->hisaddr == 0)
	{
		IPCPDEBUG("Could not determine remote IP address");
		ipcp_close(pPppIf);
		return;
	}
	if (go->ouraddr == 0)
	{
		IPCPDEBUG("Could not determine local IP address");
		ipcp_close(pPppIf);
		return;
	}
	
	/*
	 * Check that the peer is allowed to use the IP address it wants.
	 */
#if 0
	if (!auth_ip_addr(pPppIf, ho->hisaddr))
	{
		IPCPDEBUG("Peer is not authorized to use remote address %s",
			   ip_ntoa(ho->hisaddr));
		ipcp_close(pPppIf);
		return;
	}
#endif
	
	SysLog(LOG_USER|SYS_LOG_NOTICE|LOGM_PPPD,"Got gateway IP %s",ip_ntoa(ho->hisaddr));    
	SysLog(LOG_USER|SYS_LOG_NOTICE|LOGM_PPPD,"Got local IP %s",ip_ntoa(go->ouraddr));
	
	/* set tcp compression */
	sifvjcomp(pPppIf, ho->neg_vj, ho->cflag, ho->maxslotindex);
	
	/* bring the interface up for IP */
	if (!sifup(pPppIf))
	{
		IPCPDEBUG("sifup failed");
		ipcp_close(pPppIf);
		return;
	}
	
	/*
	 * Set DNS
	 */
	sifdns(pPppIf);
	
	/*
	 * Netbios Name Server
	 */
	if(go->neg_pnbns)
		IPCPDEBUG ("Primary NBNS address %s", ip_ntoa(go->pnbnsaddr));
	if(go->neg_snbns)
		IPCPDEBUG ("Secondary NBNS address %s", ip_ntoa(go->snbnsaddr));
	
	/*
	 * Set IP addresses and (if specified) netmask.
	 */
	sifaddr(pPppIf);
	
	/* Start idle timer */
	pPppIf->last_idle_ticks = (u_long)cyg_current_time();
	if (pPppIf->idle)
		PPP_TIMEOUT(adjidle, (caddr_t)pPppIf, pPppIf->idle, pPppIf);
	
	return;
}


/*
 * ipcp_down - IPCP has gone DOWN.
 *
 * Take the IP network interface down, clear its addresses
 * and delete routes through it.
 */
void
ipcp_down(f)
    fsm *f;
{
	PPP_IF_VAR_T *pPppIf = f->pPppIf;
	u_long ouraddr, hisaddr;
	
	IPCPDEBUG("ipcp: down");
	SysLog(LOG_USER|SYS_LOG_INFO|LOGM_PPPD,"ipcp down");
	
	ouraddr = pPppIf->ipcp_gotoptions.ouraddr;
	hisaddr = pPppIf->ipcp_hisoptions.hisaddr;
	
	if (pPppIf->ipcp_gotoptions.default_route) 
		cifdefaultroute(hisaddr);
	
	sifdown(pPppIf);
	cifaddr(pPppIf, ouraddr, hisaddr);
}

#ifdef  notyet
/*
 * ipcp_script - Execute a script with arguments
 * interface-name tty-name speed local-IP remote-IP.
 */
static void
ipcp_script(f, script)
    fsm *f;
    char *script;
{
    PPP_IF_VAR_T *pPppIf = f->pPppIf;
    char strspeed[32], strlocal[32], strremote[32];
    char *argv[8];

    sprintf(strspeed, "%d", pPppIf->baud_rate);
    strcpy(strlocal, ip_ntoa(pPppIf->ipcp_gotoptions.ouraddr));
    strcpy(strremote, ip_ntoa(pPppIf->ipcp_hisoptions.hisaddr));

    argv[0] = script;
    argv[1] = pPppIf->ifname;
    argv[2] = pPppIf->devname;
    argv[3] = strspeed;
    argv[4] = strlocal;
    argv[5] = strremote;
    argv[6] = NULL;
    run_program(script, argv, 0);
}
#endif	/* notyet */


