/*	$OpenBSD: ccp.c,v 1.12 2003/04/04 20:25:07 deraadt Exp $	*/

/*
 * ccp.c - PPP Compression Control Protocol.
 *
 * Copyright (c) 1989-2002 Paul Mackerras. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name(s) of the authors of this software must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission.
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Paul Mackerras
 *     <paulus@samba.org>".
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef lint
//#if 0
//static char rcsid[] = "Id: ccp.c,v 1.22 1998/03/25 01:25:02 paulus Exp $";
//#else
//static char rcsid[] = "$OpenBSD: ccp.c,v 1.12 2003/04/04 20:25:07 deraadt Exp $";
//#endif
#endif

#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "pppd.h"
#include <syslog.h>
#include "fsm.h"
//#include "ccp.h"
#ifdef MPPE
#include "mppe.h"
#endif
#include "ppp_comp.h"
#include <cfg_net.h>
#include <cfg_def.h>
/*
 * Protocol entry points from main code.
 */
void ccp_flags_set(int *val, int isopen, int isup);

/*
 * Callbacks for fsm code.
 */
static void ccp_resetci(fsm *);
static int  ccp_cilen(fsm *);
static void ccp_addci(fsm *, u_char *, int *);
static int  ccp_ackci(fsm *, u_char *, int);
static int  ccp_nakci(fsm *, u_char *, int);
static int  ccp_rejci(fsm *, u_char *, int);
static int  ccp_reqci(fsm *, u_char *, int *, int);
static void ccp_up(fsm *);
static void ccp_down(fsm *);
static int  ccp_extcode(fsm *, int, int, u_char *, int);
static void ccp_rack_timeout(void *);
static char *method_name(ccp_options *, ccp_options *);

static fsm_callbacks ccp_callbacks = {
    ccp_resetci,
    ccp_cilen,
    ccp_addci,
    ccp_ackci,
    ccp_nakci,
    ccp_rejci,
    ccp_reqci,
    ccp_up,
    ccp_down,
    NULL,
    NULL,
    NULL,
    NULL,
    ccp_extcode,
    "CCP"
};

/*
 * Do we want / did we get any compression?
 */
#define ANY_COMPRESS(opt)	((opt).deflate || (opt).bsd_compress \
				 || (opt).predictor_1 || (opt).predictor_2 \
				 || (opt).mppe )

/*
 * Local state (mainly for handling reset-reqs and reset-acks).
 */
//static int ccp_localstate[NUM_PPP];
static int ccp_localstate;
#define RACK_PENDING	1	/* waiting for reset-ack */
#define RREQ_REPEAT	2	/* send another reset-req if no reset-ack */

#define RACKTIMEOUT	1	/* second */

//static int all_rejected[NUM_PPP];	/* we rejected all peer's options */
static int all_rejected;
int mppe_bitkeylen = 0;
/*
 * ccp_init - initialize CCP.
 */
void
ccp_init(PPP_IF_VAR_T *pPppIf)
{
    fsm *f = &pPppIf->ccp_fsm;
    ccp_options *wo = &pPppIf->ccp_wantoptions;
	ccp_options *ao = &pPppIf->ccp_allowoptions;

    f->pPppIf = pPppIf;
    f->protocol = PPP_CCP;
    f->callbacks = &ccp_callbacks;
    fsm_init(f);

	mppe_bitkeylen = 0;
    memset(&pPppIf->ccp_wantoptions,  0, sizeof(ccp_options));
    memset(&pPppIf->ccp_gotoptions,   0, sizeof(ccp_options));
    memset(&pPppIf->ccp_allowoptions, 0, sizeof(ccp_options));
    memset(&pPppIf->ccp_hisoptions,   0, sizeof(ccp_options));
   

    //wo->deflate = 1;
    wo->deflate = 0;
    wo->deflate_size = DEFLATE_MAX_SIZE;
    wo->deflate_correct = 1;
    //wo->deflate_draft = 1;
    wo->deflate_draft = 0;
    ao->deflate = 1;
    ao->deflate_size = DEFLATE_MAX_SIZE;
    ao->deflate_correct = 1;
    ao->deflate_draft = 1;

    //wo->bsd_compress = 1;
    wo->bsd_compress = 0;
    wo->bsd_bits = BSD_MAX_BITS;
    ao->bsd_compress = 1;
    ao->bsd_bits = BSD_MAX_BITS;

    ao->predictor_1 = 1;
#ifdef MPPE
	if (pPppIf->mppe == 1)
	{
		pPppIf->ccp_wantoptions.mppe = 1;
		pPppIf->ccp_wantoptions.mppc = 1;
		pPppIf->ccp_wantoptions.mppe_stateless = 1;
		pPppIf->ccp_wantoptions.mppe_40 = 1;
		pPppIf->ccp_wantoptions.mppe_56 = 1;
		pPppIf->ccp_wantoptions.mppe_128 = 1;
		pPppIf->ccp_allowoptions.mppe_stateless = 1;
		pPppIf->ccp_allowoptions.mppe = 1;
		pPppIf->ccp_allowoptions.mppe_40 = 1;
		pPppIf->ccp_allowoptions.mppe_56 = 1;
		pPppIf->ccp_allowoptions.mppe_128 = 1;
	}
#endif /* MPPE*/

}

/*
 * ccp_open - CCP is allowed to come up.
 */
void
ccp_open(PPP_IF_VAR_T *pPppIf)
{
	diag_printf("\n<ccp_open>\n");
    fsm *f = &pPppIf->ccp_fsm;

    if (f->state != OPENED)
		ccp_flags_set(&pPppIf->pSc->sc_flags,1, 0);

    /*
     * Find out which compressors the kernel supports before
     * deciding whether to open in silent mode.
     */
    ccp_resetci(f);
    if (!ANY_COMPRESS(f->pPppIf->ccp_gotoptions))
	f->flags |= OPT_SILENT;

	{ //Added by Eddy
		int ipmode = 0;
		CFG_get(CFG_WAN_IP_MODE, &ipmode);
		if (ipmode == PPPOEMODE)
			f->flags |= OPT_SILENT;
	#ifdef	CONFIG_PPTP_PPPOE	
	  //Added by Haitao
	  	int pptp_wanif = 0;
	    CFG_get(CFG_PTP_WANIF, &pptp_wanif);
	    if(ipmode == PPTPMODE && pptp_wanif == 2 && !strcmp(pPppIf->pppname,"ppp0"))//for pptp over pppoe use ppp
			f->flags |= OPT_SILENT;
	#endif	
	#ifdef	CONFIG_L2TP_OVER_PPPOE	
	  int l2tp_wanif = 0;
	  CFG_get(CFG_L2T_WANIF, &l2tp_wanif);
	  if((ipmode == L2TPMODE) && (l2tp_wanif == 2) && (!strcmp(pPppIf->pppname,"ppp0")))//for l2tp over pppoe use ppp
	    f->flags |= OPT_SILENT;
	#endif	
	}

    fsm_open(f);
}

/*
 * ccp_close - Terminate CCP.
 */
void 
ccp_close(PPP_IF_VAR_T *pPppIf)
{
	fsm *f = &pPppIf->ccp_fsm;
	
    ccp_flags_set(&pPppIf->pSc->sc_flags,0, 0);
    //fsm_close(&ccp_fsm[unit], reason);
    fsm_close(f);
}

/*
 * ccp_lowerup - we may now transmit CCP packets.
 */
void
ccp_lowerup(PPP_IF_VAR_T *pPppIf)
{
    fsm_lowerup(&pPppIf->ccp_fsm);
}

/*
 * ccp_lowerdown - we may not transmit CCP packets.
 */
void
ccp_lowerdown(PPP_IF_VAR_T *pPppIf)
{
    fsm_lowerdown(&pPppIf->ccp_fsm);
#ifdef MPPE
    if (pPppIf->ccp_wantoptions.mppe) {
 	diag_printf("MPPE required but peer negotiation failed\n");
 	lcp_close(pPppIf);
     }
#endif
}

/*
 * ccp_input - process a received CCP packet.
 */
void
ccp_input(PPP_IF_VAR_T *pPppIf, u_char *p, int len)
{
    fsm *f = &pPppIf->ccp_fsm;
    int oldstate;

    /*
     * Check for a terminate-request so we can print a message.
     */
    oldstate = f->state;
    fsm_input(f, p, len);
    if (oldstate == OPENED && p[0] == TERMREQ && f->state != OPENED){
	syslog(SYS_LOG_NOTICE, "Compression disabled by peer.");
	diag_printf("Compression disabled by peer.\n");
#ifdef MPPE
	if (pPppIf->ccp_wantoptions.mppe) {
 	    diag_printf("MPPE disabled, closing LCP\n");
 	    lcp_close(pPppIf);
 	}
#endif /* MPPE */
	}
    /*
     * If we get a terminate-ack and we're not asking for compression,
     * close CCP.
     */
    if (oldstate == REQSENT && p[0] == TERMACK
	&& !ANY_COMPRESS(f->pPppIf->ccp_gotoptions))
	ccp_close(pPppIf);
}

/*
 * Handle a CCP-specific code.
 */
static int
ccp_extcode(f, code, id, p, len)
    fsm *f;
    int code, id;
    u_char *p;
    int len;
{
    switch (code) {
    case CCP_RESETREQ:
	if (f->state != OPENED)
	    break;
	/* send a reset-ack, which the transmitter will see and
	   reset its compression state. */
	fsm_sdata(f, CCP_RESETACK, id, NULL, 0);
	break;

    case CCP_RESETACK:
	if (ccp_localstate & RACK_PENDING && id == f->reqid) {
	    ccp_localstate &= ~(RACK_PENDING | RREQ_REPEAT);
	    //UNTIMEOUT(ccp_rack_timeout, f);
	    PPP_UNTIMEOUT (ccp_rack_timeout, (caddr_t) f, f->pPppIf);
	}
	break;

    default:
	return 0;
    }

    return 1;
}

/*
 * ccp_protrej - peer doesn't talk CCP.
 */
void
ccp_protrej(PPP_IF_VAR_T *pPppIf)
{
   if (pPppIf->ccp_wantoptions.mppe) {
   /* we require encryption, but peer doesn't support it
    so we close connection */
    //lcp_close(pPppIf);
    pPppIf->mppe =0;
    pPppIf->ccp_wantoptions.mppe =0;
    }
    ccp_flags_set(&pPppIf->pSc->sc_flags,0, 0);
    fsm_lowerdown(&pPppIf->ccp_fsm);
}

/*
 * ccp_resetci - initialize at start of negotiation.
 */
static void
ccp_resetci(f)
    fsm *f;
{
    ccp_options *go = &f->pPppIf->ccp_gotoptions;
    u_char opt_buf[256];

    *go = f->pPppIf->ccp_wantoptions;
    all_rejected = 0;

    /*
     * Check whether the kernel knows about the various
     * compression methods we might request.
     */
    if (go->bsd_compress) {
	opt_buf[0] = CI_BSD_COMPRESS;
	opt_buf[1] = CILEN_BSD_COMPRESS;
	opt_buf[2] = BSD_MAKE_OPT(BSD_CURRENT_VERSION, BSD_MIN_BITS);
	if (ccp_test(f->pPppIf, opt_buf, CILEN_BSD_COMPRESS, 0) <= 0)
	    go->bsd_compress = 0;
    }
    if (go->deflate) {
	if (go->deflate_correct) {
	    opt_buf[0] = CI_DEFLATE;
	    opt_buf[1] = CILEN_DEFLATE;
	    opt_buf[2] = DEFLATE_MAKE_OPT(DEFLATE_MIN_SIZE);
	    opt_buf[3] = DEFLATE_CHK_SEQUENCE;
	    if (ccp_test(f->pPppIf, opt_buf, CILEN_DEFLATE, 0) <= 0)
		go->deflate_correct = 0;
	}
	if (go->deflate_draft) {
	    opt_buf[0] = CI_DEFLATE_DRAFT;
	    opt_buf[1] = CILEN_DEFLATE;
	    opt_buf[2] = DEFLATE_MAKE_OPT(DEFLATE_MIN_SIZE);
	    opt_buf[3] = DEFLATE_CHK_SEQUENCE;
	    if (ccp_test(f->pPppIf, opt_buf, CILEN_DEFLATE, 0) <= 0)
		go->deflate_draft = 0;
	}
	if (!go->deflate_correct && !go->deflate_draft)
	    go->deflate = 0;
    }
#ifdef MPPE
    if (go->mppe) {
	if (go->mppe_40) {
	    opt_buf[0] = CI_MPPE;
	    opt_buf[1] = CILEN_MPPE;
	    opt_buf[2] = MPPE_STATELESS;
	    opt_buf[3] = 0;
	    opt_buf[4] = 0;
	    opt_buf[5] = MPPE_40BIT;
	    if (ccp_test(f->pPppIf, opt_buf, CILEN_MPPE+(2*8), 0) <= 0)
		go->mppe_40 = 0;
	}
	if (go->mppe_56) {
	    opt_buf[0] = CI_MPPE;
	    opt_buf[1] = CILEN_MPPE;
	    opt_buf[2] = MPPE_STATELESS;
	    opt_buf[3] = 0;
	    opt_buf[4] = 0;
	    opt_buf[5] = MPPE_56BIT;
	    if (ccp_test(f->pPppIf, opt_buf, CILEN_MPPE+(2*8), 0) <= 0)
		go->mppe_56 = 0;
	}
	if (go->mppe_128) {
	    opt_buf[0] = CI_MPPE;
	    opt_buf[1] = CILEN_MPPE;
	    opt_buf[2] = MPPE_STATELESS;
	    opt_buf[3] = 0;
	    opt_buf[4] = 0;
	    opt_buf[5] = MPPE_128BIT;
	    if (ccp_test(f->pPppIf, opt_buf, CILEN_MPPE+(2*16), 0) <= 0)
		go->mppe_128 = 0;
	}
	if (!go->mppe_40 && !go->mppe_56 && !go->mppe_128)
	    go->mppe = go->mppe_stateless = 0;
    }
#endif /* MPPE */    
    if (go->predictor_1) {
	opt_buf[0] = CI_PREDICTOR_1;
	opt_buf[1] = CILEN_PREDICTOR_1;
	if (ccp_test(f->pPppIf, opt_buf, CILEN_PREDICTOR_1, 0) <= 0)
	    go->predictor_1 = 0;
    }
    if (go->predictor_2) {
	opt_buf[0] = CI_PREDICTOR_2;
	opt_buf[1] = CILEN_PREDICTOR_2;
	if (ccp_test(f->pPppIf, opt_buf, CILEN_PREDICTOR_2, 0) <= 0)
	    go->predictor_2 = 0;
    }
}

/*
 * ccp_cilen - Return total length of our configuration info.
 */
static int
ccp_cilen(f)
    fsm *f;
{
    ccp_options *go = &f->pPppIf->ccp_gotoptions;

    return (go->bsd_compress? CILEN_BSD_COMPRESS: 0)
	+ (go->deflate? CILEN_DEFLATE: 0)
#ifdef MPPE
	+((go->mppe)? CILEN_MPPE: 0)
#endif /* MPPE */
	+ (go->predictor_1? CILEN_PREDICTOR_1: 0)
	+ (go->predictor_2? CILEN_PREDICTOR_2: 0);
}

/*
 * ccp_addci - put our requests in a packet.
 */
static void
ccp_addci(f, p, lenp)
    fsm *f;
    u_char *p;
    int *lenp;
{
    int res;
    ccp_options *go = &f->pPppIf->ccp_gotoptions;
    //ccp_options *ao = &f->pPppIf->ccp_allowoptions;
    //ccp_options *wo = &f->pPppIf->ccp_wantoptions;
    u_char *p0 = p;

    /*
     * Add the compression types that we can receive, in decreasing
     * preference order.  Get the kernel to allocate the first one
     * in case it gets Acked.
     */
    if (go->deflate) {
	p[0] = go->deflate_correct? CI_DEFLATE: CI_DEFLATE_DRAFT;
	p[1] = CILEN_DEFLATE;
	p[2] = DEFLATE_MAKE_OPT(go->deflate_size);
	p[3] = DEFLATE_CHK_SEQUENCE;
	for (;;) {
	    res = ccp_test(f->pPppIf, p, CILEN_DEFLATE, 0);
	    if (res > 0) {
		p += CILEN_DEFLATE;
		break;
	    }
	    if (res < 0 || go->deflate_size <= DEFLATE_MIN_SIZE) {
		go->deflate = 0;
		break;
	    }
	    --go->deflate_size;
	    p[2] = DEFLATE_MAKE_OPT(go->deflate_size);
	}
	if (p != p0 && go->deflate_correct && go->deflate_draft) {
	    p[0] = CI_DEFLATE_DRAFT;
	    p[1] = CILEN_DEFLATE;
	    p[2] = p[2 - CILEN_DEFLATE];
	    p[3] = DEFLATE_CHK_SEQUENCE;
	    p += CILEN_DEFLATE;
	}
    }
#ifdef MPPE
    if (go->mppe) {
        u_char opt_buf[64];
        u_int keysize = 0;

        if(!mppe_allowed)
            go->mppe_stateless = go->mppe_40 = go->mppe_56 = go->mppe_128 = 0;
        p[0]=CI_MPPE;
        p[1]=CILEN_MPPE;
        p[2]=(go->mppe_stateless ? MPPE_STATELESS : 0);
        p[3]=0;
        p[4]=0;
        p[5]=(go->mppe_40 ? MPPE_40BIT : 0) | (go->mppe_56 ? MPPE_56BIT : 0)| 
        	(go->mppe_128 ? MPPE_128BIT : 0);
        if((p[5] & MPPE_40BIT) || (p[5] & MPPE_56BIT)) {
            keysize = 8;
            //diag_printf(">>>>>>>>>>>>MPPE_40BIT || MPPE_56BIT>>>>>>\n");
            if(p[5] & MPPE_40BIT)
            	mppe_bitkeylen = 40;
            if(p[5] & MPPE_56BIT)
            	mppe_bitkeylen = 56;
            BCOPY(mppe_master_send_key_40, opt_buf+3, keysize);
            BCOPY(mppe_master_recv_key_40, opt_buf+11, keysize);
        } else if(p[5] & MPPE_128BIT) {
        	//diag_printf(">>>>>>>>>>>>MPPE_128BIT>>>>>>\n");
            keysize = 16;
            BCOPY(mppe_master_send_key_128, opt_buf+3, keysize);
            BCOPY(mppe_master_recv_key_128, opt_buf+19, keysize);
        }
        if(p[5] != 0) {
            opt_buf[0]=CI_MPPE;
            opt_buf[1]=CILEN_MPPE;
            opt_buf[2] = (go->mppe_stateless) ? 1 : 0;
            if(keysize)
            {
            	res = ccp_test(f->pPppIf, opt_buf, (2*keysize)+3, 0);
            }	
            else
            	res = 1;	
        } else {
            res = -1;
        }
        if (res > 0) {
                p += CILEN_MPPE;
        }
    }
#endif /* MPPE*/
    if (go->bsd_compress) {
	p[0] = CI_BSD_COMPRESS;
	p[1] = CILEN_BSD_COMPRESS;
	p[2] = BSD_MAKE_OPT(BSD_CURRENT_VERSION, go->bsd_bits);
	if (p != p0) {
	    p += CILEN_BSD_COMPRESS;	/* not the first option */
	} else {
	    for (;;) {
		res = ccp_test(f->pPppIf, p, CILEN_BSD_COMPRESS, 0);
		if (res > 0) {
		    p += CILEN_BSD_COMPRESS;
		    break;
		}
		if (res < 0 || go->bsd_bits <= BSD_MIN_BITS) {
		    go->bsd_compress = 0;
		    break;
		}
		--go->bsd_bits;
		p[2] = BSD_MAKE_OPT(BSD_CURRENT_VERSION, go->bsd_bits);
	    }
	}
    }
    /* XXX Should Predictor 2 be preferable to Predictor 1? */
    if (go->predictor_1) {
	p[0] = CI_PREDICTOR_1;
	p[1] = CILEN_PREDICTOR_1;
	if (p == p0 && ccp_test(f->pPppIf, p, CILEN_PREDICTOR_1, 0) <= 0) {
	    go->predictor_1 = 0;
	} else {
	    p += CILEN_PREDICTOR_1;
	}
    }
    if (go->predictor_2) {
	p[0] = CI_PREDICTOR_2;
	p[1] = CILEN_PREDICTOR_2;
	if (p == p0 && ccp_test(f->pPppIf, p, CILEN_PREDICTOR_2, 0) <= 0) {
	    go->predictor_2 = 0;
	} else {
	    p += CILEN_PREDICTOR_2;
	}
    }

    go->method = (p > p0)? p0[0]: -1;

    *lenp = p - p0;
}

/*
 * ccp_ackci - process a received configure-ack, and return
 * 1 iff the packet was OK.
 */
static int
ccp_ackci(f, p, len)
    fsm *f;
    u_char *p;
    int len;
{
    ccp_options *go = &f->pPppIf->ccp_gotoptions;
    ccp_options *ao = &f->pPppIf->ccp_allowoptions;
    ccp_options *wo = &f->pPppIf->ccp_wantoptions;
    u_char *p0 = p;

    if (go->deflate) {
	if (len < CILEN_DEFLATE
	    || p[0] != (go->deflate_correct? CI_DEFLATE: CI_DEFLATE_DRAFT)
	    || p[1] != CILEN_DEFLATE
	    || p[2] != DEFLATE_MAKE_OPT(go->deflate_size)
	    || p[3] != DEFLATE_CHK_SEQUENCE)
	    return 0;
	p += CILEN_DEFLATE;
	len -= CILEN_DEFLATE;
	/* XXX Cope with first/fast ack */
	if (len == 0)
	    return 1;
	if (go->deflate_correct && go->deflate_draft) {
	    if (len < CILEN_DEFLATE
		|| p[0] != CI_DEFLATE_DRAFT
		|| p[1] != CILEN_DEFLATE
		|| p[2] != DEFLATE_MAKE_OPT(go->deflate_size)
		|| p[3] != DEFLATE_CHK_SEQUENCE)
		return 0;
	    p += CILEN_DEFLATE;
	    len -= CILEN_DEFLATE;
	}
    }
#ifdef MPPE
    if (go->mppe || (!wo->mppe && ao->mppe)) {
        if ( len < CILEN_MPPE
		|| p[1] != CILEN_MPPE || p[0] != CI_MPPE
		|| p[2] != (go->mppe_stateless ? MPPE_STATELESS : 0)
		|| p[3] != 0
		|| p[4] != 0
		|| (p[5] != (go->mppe_40 ? MPPE_40BIT : 0)
		&& p[5] != (go->mppe_56 ? MPPE_56BIT : 0)
		&& p[5] != (go->mppe_128 ? MPPE_128BIT : 0)))
           return 0;
	if (go->mppe_40 || go->mppe_56 || go->mppe_128)
	    go->mppe = 1;           
        p += CILEN_MPPE;
        len -= CILEN_MPPE;
        /* Cope with first/fast ack */
	if (p == p0 && len == 0)
            return 1;
    }
#endif /* MPPE */
    if (go->bsd_compress) {
	if (len < CILEN_BSD_COMPRESS
	    || p[0] != CI_BSD_COMPRESS || p[1] != CILEN_BSD_COMPRESS
	    || p[2] != BSD_MAKE_OPT(BSD_CURRENT_VERSION, go->bsd_bits))
	    return 0;
	p += CILEN_BSD_COMPRESS;
	len -= CILEN_BSD_COMPRESS;
	/* XXX Cope with first/fast ack */
	if (p == p0 && len == 0)
	    return 1;
    }
    if (go->predictor_1) {
	if (len < CILEN_PREDICTOR_1
	    || p[0] != CI_PREDICTOR_1 || p[1] != CILEN_PREDICTOR_1)
	    return 0;
	p += CILEN_PREDICTOR_1;
	len -= CILEN_PREDICTOR_1;
	/* XXX Cope with first/fast ack */
	if (p == p0 && len == 0)
	    return 1;
    }
    if (go->predictor_2) {
	if (len < CILEN_PREDICTOR_2
	    || p[0] != CI_PREDICTOR_2 || p[1] != CILEN_PREDICTOR_2)
	    return 0;
	p += CILEN_PREDICTOR_2;
	len -= CILEN_PREDICTOR_2;
	/* XXX Cope with first/fast ack */
	if (p == p0 && len == 0)
	    return 1;
    }

    if (len != 0)
	return 0;
    return 1;
}

/*
 * ccp_nakci - process received configure-nak.
 * Returns 1 if the nak was OK.
 */
static int
ccp_nakci(f, p, len)
    fsm *f;
    u_char *p;
    int len;
{
    ccp_options *go = &f->pPppIf->ccp_gotoptions;
    ccp_options *ao = &f->pPppIf->ccp_allowoptions;
    ccp_options *wo = &f->pPppIf->ccp_wantoptions;
    ccp_options no;		/* options we've seen already */
    ccp_options try;		/* options to ask for next time */

    memset(&no, 0, sizeof(no));
    try = *go;

    if (go->deflate && len >= CILEN_DEFLATE
	&& p[0] == (go->deflate_correct? CI_DEFLATE: CI_DEFLATE_DRAFT)
	&& p[1] == CILEN_DEFLATE) {
	no.deflate = 1;
	/*
	 * Peer wants us to use a different code size or something.
	 * Stop asking for Deflate if we don't understand his suggestion.
	 */
	if (DEFLATE_METHOD(p[2]) != DEFLATE_METHOD_VAL
	    || DEFLATE_SIZE(p[2]) < DEFLATE_MIN_SIZE
	    || p[3] != DEFLATE_CHK_SEQUENCE)
	    try.deflate = 0;
	else if (DEFLATE_SIZE(p[2]) < go->deflate_size)
	    try.deflate_size = DEFLATE_SIZE(p[2]);
	p += CILEN_DEFLATE;
	len -= CILEN_DEFLATE;
	if (go->deflate_correct && go->deflate_draft
	    && len >= CILEN_DEFLATE && p[0] == CI_DEFLATE_DRAFT
	    && p[1] == CILEN_DEFLATE) {
	    p += CILEN_DEFLATE;
	    len -= CILEN_DEFLATE;
	}
    }

#ifdef MPPE
	if ((go->mppe || (!wo->mppe && ao->mppe)) &&
	len >= CILEN_MPPE && p[0] == CI_MPPE && p[1] == CILEN_MPPE) {
	if (go->mppc) {
	    no.mppc = 1;
	    if (!(p[5] & MPPE_MPPC))
		try.mppc = 0;
	}
    if (go->mppe)
	no.mppe = 1;
	if (go->mppe_40)
	    no.mppe_40 = 1;
	if (go->mppe_56)
	    no.mppe_56 = 1;
	if (go->mppe_128)
	    no.mppe_128 = 1;

	if ((p[5] & MPPE_40BIT) && ao->mppe_40)
	    try.mppe_40 = 1;
	else
	    try.mppe_40 = 0;
	if ((p[5] & MPPE_56BIT) && ao->mppe_56)
	    try.mppe_56 = 1;
	else
	    try.mppe_56 = 0;
	if ((p[5] & MPPE_128BIT) && ao->mppe_128)
	    try.mppe_128 = 1;
	else
	    try.mppe_128 = 0;
	    
	if ((p[2] & MPPE_STATELESS) && ao->mppe_stateless)
	    try.mppe_stateless = 1;
	else
            try.mppe_stateless = 0;

	if (!try.mppe_56 && !try.mppe_40 && !try.mppe_128) {
	    try.mppe = try.mppe_stateless = 0;
	    if (wo->mppe) {
		/* we require encryption, but peer doesn't support it
		   so we close connection */
		wo->mppc = wo->mppe = wo->mppe_stateless = 0;
		wo->mppe_40 = wo->mppe_56 = wo->mppe_128 = 0;
		lcp_close(f->pPppIf);
	    }
        }
	if (wo->mppe && (wo->mppe_40 != try.mppe_40) &&
	    (wo->mppe_56 != try.mppe_56) && (wo->mppe_128 != try.mppe_128)) {
	    /* cannot negotiate key length */
	    wo->mppc = wo->mppe = wo->mppe_stateless = 0;
	    wo->mppe_40 = wo->mppe_56 = wo->mppe_128 = 0;
	    lcp_close(f->pPppIf);
	}
	if (try.mppe_40 && try.mppe_56 && try.mppe_128)
	    try.mppe_40 = try.mppe_56 = 0;
	else
	    if (try.mppe_56 && try.mppe_128)
		try.mppe_56 = 0;
	    else
		if (try.mppe_40 && try.mppe_128)
		    try.mppe_40 = 0;
		else
		    if (try.mppe_40 && try.mppe_56)
			try.mppe_40 = 0;

	p += CILEN_MPPE;
	len -= CILEN_MPPE;
    }
#endif /* MPPE */

    if (go->bsd_compress && len >= CILEN_BSD_COMPRESS
	&& p[0] == CI_BSD_COMPRESS && p[1] == CILEN_BSD_COMPRESS) {
	no.bsd_compress = 1;
	/*
	 * Peer wants us to use a different number of bits
	 * or a different version.
	 */
	if (BSD_VERSION(p[2]) != BSD_CURRENT_VERSION)
	    try.bsd_compress = 0;
	else if (BSD_NBITS(p[2]) < go->bsd_bits)
	    try.bsd_bits = BSD_NBITS(p[2]);
	p += CILEN_BSD_COMPRESS;
	len -= CILEN_BSD_COMPRESS;
    }

    /*
     * Predictor-1 and 2 have no options, so they can't be Naked.
     *
     * XXX What should we do with any remaining options?
     */

    if (len != 0)
	return 0;

    if (f->state != OPENED)
	*go = try;
    return 1;
}

/*
 * ccp_rejci - reject some of our suggested compression methods.
 */
static int
ccp_rejci(f, p, len)
    fsm *f;
    u_char *p;
    int len;
{
    ccp_options *wo = &f->pPppIf->ccp_wantoptions;
    ccp_options *go = &f->pPppIf->ccp_gotoptions;
    ccp_options try;		/* options to request next time */

    try = *go;

    /*
     * Cope with empty configure-rejects by ceasing to send
     * configure-requests.
     */
    if (len == 0 && all_rejected)
	return -1;

    if (go->deflate && len >= CILEN_DEFLATE
	&& p[0] == (go->deflate_correct? CI_DEFLATE: CI_DEFLATE_DRAFT)
	&& p[1] == CILEN_DEFLATE) {
	if (p[2] != DEFLATE_MAKE_OPT(go->deflate_size)
	    || p[3] != DEFLATE_CHK_SEQUENCE)
	    return 0;		/* Rej is bad */
	if (go->deflate_correct)
	    try.deflate_correct = 0;
	else
	    try.deflate_draft = 0;
	p += CILEN_DEFLATE;
	len -= CILEN_DEFLATE;
	if (go->deflate_correct && go->deflate_draft
	    && len >= CILEN_DEFLATE && p[0] == CI_DEFLATE_DRAFT
	    && p[1] == CILEN_DEFLATE) {
	    if (p[2] != DEFLATE_MAKE_OPT(go->deflate_size)
		|| p[3] != DEFLATE_CHK_SEQUENCE)
		return 0;		/* Rej is bad */
	    try.deflate_draft = 0;
	    p += CILEN_DEFLATE;
	    len -= CILEN_DEFLATE;
	}
	if (!try.deflate_correct && !try.deflate_draft)
	    try.deflate = 0;
    }
#ifdef MPPE
    if ((go->mppe) && len >= CILEN_MPPE
	&& p[0] == CI_MPPE && p[1] == CILEN_MPPE) {
	if (p[2] != (go->mppe_stateless ? MPPE_STATELESS : 0) ||
	    p[3] != 0 ||
	    p[4] != 0 ||
	    p[5] != ((go->mppe_40 ? MPPE_40BIT : 0) |
		     (go->mppe_56 ? MPPE_56BIT : 0) |
		     (go->mppe_128 ? MPPE_128BIT : 0) |
		     (go->mppc ? MPPE_MPPC : 0)))
	    return 0;
	if (go->mppc)
	    try.mppc = 0;
	if (go->mppe) {
	try.mppe = 0;
	    if (go->mppe_40)
		try.mppe_40 = 0;
	    if (go->mppe_56)
		try.mppe_56 = 0;
	    if (go->mppe_128)
		try.mppe_128 = 0;
	    if (go->mppe_stateless)
		try.mppe_stateless = 0;
	    if (!try.mppe_56 && !try.mppe_40 && !try.mppe_128)
		try.mppe = try.mppe_stateless = 0;
	}
	if (wo->mppe) { /* we want MPPE but cannot negotiate key length */
	    wo->mppc = wo->mppe = wo->mppe_stateless = 0;
	    wo->mppe_40 = wo->mppe_56 = wo->mppe_128 = 0;
	    lcp_close(f->pPppIf);
	    diag_printf("Cannot negotiate MPPE key length\n");
	}
	p += CILEN_MPPE;
	len -= CILEN_MPPE;
    }
#endif /*MPPE*/
    if (go->bsd_compress && len >= CILEN_BSD_COMPRESS
	&& p[0] == CI_BSD_COMPRESS && p[1] == CILEN_BSD_COMPRESS) {
	if (p[2] != BSD_MAKE_OPT(BSD_CURRENT_VERSION, go->bsd_bits))
	    return 0;
	try.bsd_compress = 0;
	p += CILEN_BSD_COMPRESS;
	len -= CILEN_BSD_COMPRESS;
    }
    if (go->predictor_1 && len >= CILEN_PREDICTOR_1
	&& p[0] == CI_PREDICTOR_1 && p[1] == CILEN_PREDICTOR_1) {
	try.predictor_1 = 0;
	p += CILEN_PREDICTOR_1;
	len -= CILEN_PREDICTOR_1;
    }
    if (go->predictor_2 && len >= CILEN_PREDICTOR_2
	&& p[0] == CI_PREDICTOR_2 && p[1] == CILEN_PREDICTOR_2) {
	try.predictor_2 = 0;
	p += CILEN_PREDICTOR_2;
	len -= CILEN_PREDICTOR_2;
    }

    if (len != 0)
	return 0;

    if (f->state != OPENED)
	*go = try;

    return 1;
}

/*
 * ccp_reqci - processed a received configure-request.
 * Returns CONFACK, CONFNAK or CONFREJ and the packet modified
 * appropriately.
 */
static int
ccp_reqci(f, p, lenp, dont_nak)
    fsm *f;
    u_char *p;
    int *lenp;
    int dont_nak;
{
    int ret, newret, res;
    u_char *p0, *retp, p2, p5;
    int len, clen, type, nb;
    ccp_options *ho = &f->pPppIf->ccp_hisoptions;
    ccp_options *ao = &f->pPppIf->ccp_allowoptions;
    ccp_options *wo = &f->pPppIf->ccp_wantoptions;

    ret = CONFACK;
    retp = p0 = p;
    len = *lenp;

    memset(ho, 0, sizeof(ccp_options));
    ho->method = (len > 0)? p[0]: -1;

    while (len > 0) {
	newret = CONFACK;
	if (len < 2 || p[1] < 2 || p[1] > len) {
	    /* length is bad */
	    clen = len;
	    newret = CONFREJ;

	} else {
	    type = p[0];
	    clen = p[1];

	    switch (type) {
	    case CI_DEFLATE:
	    case CI_DEFLATE_DRAFT:
		if (!ao->deflate || clen != CILEN_DEFLATE
		    || (!ao->deflate_correct && type == CI_DEFLATE)
		    || (!ao->deflate_draft && type == CI_DEFLATE_DRAFT)) {
		    newret = CONFREJ;
		    break;
		}

		ho->deflate = 1;
		ho->deflate_size = nb = DEFLATE_SIZE(p[2]);
		if (DEFLATE_METHOD(p[2]) != DEFLATE_METHOD_VAL
		    || p[3] != DEFLATE_CHK_SEQUENCE
		    || nb > ao->deflate_size || nb < DEFLATE_MIN_SIZE) {
		    newret = CONFNAK;
		    if (!dont_nak) {
			p[2] = DEFLATE_MAKE_OPT(ao->deflate_size);
			p[3] = DEFLATE_CHK_SEQUENCE;
			/* fall through to test this #bits below */
		    } else
			break;
		}

		/*
		 * Check whether we can do Deflate with the window
		 * size they want.  If the window is too big, reduce
		 * it until the kernel can cope and nak with that.
		 * We only check this for the first option.
		 */
		if (p == p0) {
		    for (;;) {
			res = ccp_test(f->pPppIf, p, CILEN_DEFLATE, 1);
			if (res > 0)
			    break;		/* it's OK now */
			if (res < 0 || nb == DEFLATE_MIN_SIZE || dont_nak) {
			    newret = CONFREJ;
			    p[2] = DEFLATE_MAKE_OPT(ho->deflate_size);
			    break;
			}
			newret = CONFNAK;
			--nb;
			p[2] = DEFLATE_MAKE_OPT(nb);
		    }
		}
		break;

#ifdef MPPE
	    case CI_MPPE:
	    
		if (!ao->mppe || clen != CILEN_MPPE) {
		    newret = CONFREJ;
		    break;
		}
		if(!mppe_allowed)
		{
		    newret = CONFREJ;
		    break;
		}
		
		p2 = p[2];
		p5 = p[5];
		
		if (((p[2] & ~MPPE_STATELESS) != 0 || p[3] != 0 || p[4] != 0 ||
		     (p[5] & ~(MPPE_40BIT | MPPE_56BIT | MPPE_128BIT |
			       MPPE_MPPC)) != 0 || p[5] == 0) ||
		    (p[2] == 0 && p[3] == 0 && p[4] == 0 &&  p[5] == 0)) 		   
		{
    		    ccp_options *wo = &f->pPppIf->ccp_wantoptions;
		    /* not sure what they want, tell 'em what we got */
		    p[2] &= MPPE_STATELESS;
		    p[3] &= 0;
		    p[4] &= 0;
		    p[5] &= MPPE_40BIT | MPPE_128BIT;
		    if(wo->mppe_40)
		        p[5] |= MPPE_40BIT;
            if(wo->mppe_56)
		        p[5] |= MPPE_56BIT;
		    if(wo->mppe_128)
		        p[5] |= MPPE_128BIT;
		    newret = CONFNAK;
		    break;
		}
		
		if ((p[5] & MPPE_MPPC)) {
			newret = CONFREJ;
			if (wo->mppe || ao->mppe) {
			    p[5] &= ~MPPE_MPPC;
			    newret = CONFNAK;
			}
		}
		
		if (ao->mppe)
		ho->mppe = 1;
		
		if ((p[2] & MPPE_STATELESS)) {
		    if (ao->mppe_stateless)
			ho->mppe_stateless = 1;
		    else {
			newret = CONFNAK;
			if (!dont_nak)
			    p[2] &= ~MPPE_STATELESS;
		    }
		} else {
		    if (wo->mppe_stateless && !dont_nak) {
			wo->mppe_stateless = 0;
			newret = CONFNAK;
			p[2] |= MPPE_STATELESS;
		    }
		}	
			
		if((p[5]&~MPPE_MPPC) == (MPPE_40BIT|MPPE_56BIT|MPPE_128BIT))
		{
			newret = CONFNAK;
		    /* if all are available, select the stronger */
		    if (ao->mppe_128) {
 			ho->mppe_128 = 1;
		    p[5] &= ~(MPPE_40BIT|MPPE_56BIT);
		    goto check_mppe;
			}
		    p[5] &= ~MPPE_128BIT;
		    goto check_mppe_56_40;
		}
		if ((p[5]&~MPPE_MPPC) == (MPPE_56BIT|MPPE_128BIT)) {
		    newret = CONFNAK;
		    /* if both are available, select the stronger */
		    if (ao->mppe_128) {
			ho->mppe_128 = 1;
			p[5] &= ~MPPE_56BIT;
			goto check_mppe;
        	}
        	p[5] &= ~MPPE_128BIT;
		    goto check_mppe_56;
        }	
        if ((p[5] & ~MPPE_MPPC) == (MPPE_40BIT|MPPE_128BIT)) {
		    newret = CONFNAK;
		    if (ao->mppe_128) {
			ho->mppe_128 = 1;
		    p[5] &= ~MPPE_40BIT;
		    goto check_mppe;
		    }
		    p[5] &= ~MPPE_128BIT;
		    goto check_mppe_40;
		}	
		if ((p[5] & ~MPPE_MPPC) == MPPE_128BIT) {
		    if (ao->mppe_128) {
			ho->mppe_128 = 1;
			goto check_mppe;
		    }
		    p[5] &= ~MPPE_128BIT;
		    newret = CONFNAK;
		    goto check_mppe;
		}
		
		check_mppe_56_40:
		if ((p[5] & ~MPPE_MPPC) == (MPPE_40BIT|MPPE_56BIT)) {
		    newret = CONFNAK;
		    if (ao->mppe_56) {
			ho->mppe_56 = 1;
			p[5] &= ~MPPE_40BIT;
			goto check_mppe;
		    }
		    p[5] &= ~MPPE_56BIT;
		    goto check_mppe_40;
		}
	    check_mppe_56:
		if ((p[5] & ~MPPE_MPPC) == MPPE_56BIT) {
		    if (ao->mppe_56) {
			ho->mppe_56 = 1;
			goto check_mppe;
		    }
		    p[5] &= ~MPPE_56BIT;
		    newret = CONFNAK;
		    goto check_mppe;
		}
	    check_mppe_40:
		if ((p[5] & ~MPPE_MPPC) == MPPE_40BIT) {
		    if (ao->mppe_40) {
			ho->mppe_40 = 1;
			goto check_mppe;
		    }
		    p[5] &= ~MPPE_40BIT;
		}

	    check_mppe:
		if (!ho->mppe_40 && !ho->mppe_56 && !ho->mppe_128) {
		    if (wo->mppe_40 || wo->mppe_56 || wo->mppe_128) {
			newret = CONFNAK;
			p[2] |= (wo->mppe_stateless ? MPPE_STATELESS : 0);
			p[5] |= (wo->mppe_40 ? MPPE_40BIT : 0) |
			    (wo->mppe_56 ? MPPE_56BIT : 0) |
			    (wo->mppe_128 ? MPPE_128BIT : 0) |
			    (wo->mppc ? MPPE_MPPC : 0);
		    } else {
			ho->mppe = ho->mppe_stateless = 0;
		    }
		} else {
		    /* MPPE is not compatible with other compression types */
		    if (wo->mppe) {
			ao->bsd_compress = 0;
			ao->predictor_1 = 0;
			ao->predictor_2 = 0;
			ao->deflate = 0;
		    }
		}
		if ((!ho->mppc || !ao->mppc) && !ho->mppe) {
		    p[2] = p2;
		    p[5] = p5;
		    newret = CONFREJ;
		    break;
		}
		if(newret == CONFACK)
		{
			int mtu;
			
			mtu = netif_get_mtu(f->pPppIf);
			if(mtu){
				netif_set_mtu(f->pPppIf,mtu-MPPE_PAD);
			}	
			else
			{
				newret = CONFREJ;
				if(f->pPppIf->ccp_wantoptions.mppe){
				diag_printf("Cannot adjust MTU needed by MPPE.");
				lcp_close(f->pPppIf);		
				}
			}	
					
		}	
		if((newret == CONFACK) || (newret == CONFNAK))
		{
    	    /* 
		     * The kernel ppp driver needs the session key 
                     * which is not sent via CCP :( 
		     */
		    unsigned int keysize =0;
		    unsigned char opt_buf[64];
		    opt_buf[0] = CI_MPPE;
		    opt_buf[1] = CILEN_MPPE;
		    if (p[2] & MPPE_STATELESS) {
		      ho->mppe_stateless=1;
		      opt_buf[2] = MPPE_STATELESS;
		    }
		    /* push in our send/receive keys */
		    if(p[5] & MPPE_40BIT) {
		    diag_printf(">>>>>>MPPE_40BIT>>>>>>\n");
		    mppe_bitkeylen = 40;
			ho->mppe_40 = 1;
			keysize = 8;
			
		        BCOPY(mppe_master_send_key_40, opt_buf+3, keysize);
		        BCOPY(mppe_master_recv_key_40, opt_buf+11, keysize);
            } else if(p[5] & MPPE_56BIT) {
            diag_printf(">>>>>>MPPE_56BIT>>>>>>\n");	
            mppe_bitkeylen = 56;
            ho->mppe_56 = 1;
			keysize = 8;
		        BCOPY(mppe_master_send_key_40, opt_buf+3, keysize);
		        BCOPY(mppe_master_recv_key_40, opt_buf+11, keysize);
		    } 
		    else if(p[5] & MPPE_128BIT) {
		    diag_printf(">>>>>>MPPE_128BIT>>>>>>\n");	
		    mppe_bitkeylen = 128;
			ho->mppe_128 = 1;
			keysize = 16;
		        BCOPY(mppe_master_send_key_128, opt_buf+3, keysize);
		        BCOPY(mppe_master_recv_key_128, opt_buf+19, keysize);
		    }
		    else
		    {   //yfchou added 5/25/2004
		    	if (!ho->mppe_40 && !ho->mppe_56 && !ho->mppe_128) {
		    		if (wo->mppe_40 || wo->mppe_56 || wo->mppe_128) {
						newret = CONFNAK;
						p[2] |= (wo->mppe_stateless ? MPPE_STATELESS : 0);
						p[5] |= (wo->mppe_40 ? MPPE_40BIT : 0) |
			  		  	(wo->mppe_56 ? MPPE_56BIT : 0) |
			  		  	(wo->mppe_128 ? MPPE_128BIT : 0) |
			 		   	(wo->mppc ? MPPE_MPPC : 0);
		  			} 
		  		}	 
		    	else {
		        ho->mppe = 0;
		        newret = CONFREJ;
		        break;
		    }
		    }
		    /* call ioctl and pass this nasty stuff to the kernel */
		    if (ccp_test(f->pPppIf, opt_buf, (2*keysize)+3, 1) <= 0){
			ho->mppe = 0;
			newret = CONFREJ;
			break;
		    }
		}
		break;
#endif /* MPPE */

	    case CI_BSD_COMPRESS:
		if (!ao->bsd_compress || clen != CILEN_BSD_COMPRESS) {
		    newret = CONFREJ;
		    break;
		}

		ho->bsd_compress = 1;
		ho->bsd_bits = nb = BSD_NBITS(p[2]);
		if (BSD_VERSION(p[2]) != BSD_CURRENT_VERSION
		    || nb > ao->bsd_bits || nb < BSD_MIN_BITS) {
		    newret = CONFNAK;
		    if (!dont_nak) {
			p[2] = BSD_MAKE_OPT(BSD_CURRENT_VERSION, ao->bsd_bits);
			/* fall through to test this #bits below */
		    } else
			break;
		}

		/*
		 * Check whether we can do BSD-Compress with the code
		 * size they want.  If the code size is too big, reduce
		 * it until the kernel can cope and nak with that.
		 * We only check this for the first option.
		 */
		if (p == p0) {
		    for (;;) {
			res = ccp_test(f->pPppIf, p, CILEN_BSD_COMPRESS, 1);
			if (res > 0)
			    break;
			if (res < 0 || nb == BSD_MIN_BITS || dont_nak) {
			    newret = CONFREJ;
			    p[2] = BSD_MAKE_OPT(BSD_CURRENT_VERSION,
						ho->bsd_bits);
			    break;
			}
			newret = CONFNAK;
			--nb;
			p[2] = BSD_MAKE_OPT(BSD_CURRENT_VERSION, nb);
		    }
		}
		break;

	    case CI_PREDICTOR_1:
		if (!ao->predictor_1 || clen != CILEN_PREDICTOR_1) {
		    newret = CONFREJ;
		    break;
		}

		ho->predictor_1 = 1;
		if (p == p0
		    && ccp_test(f->pPppIf, p, CILEN_PREDICTOR_1, 1) <= 0) {
		    newret = CONFREJ;
		}
		break;

	    case CI_PREDICTOR_2:
		if (!ao->predictor_2 || clen != CILEN_PREDICTOR_2) {
		    newret = CONFREJ;
		    break;
		}

		ho->predictor_2 = 1;
		if (p == p0
		    && ccp_test(f->pPppIf, p, CILEN_PREDICTOR_2, 1) <= 0) {
		    newret = CONFREJ;
		}
		break;

	    default:
		newret = CONFREJ;
	    }
	}

	if (newret == CONFNAK && dont_nak)
	    newret = CONFREJ;
	if (!(newret == CONFACK || (newret == CONFNAK && ret == CONFREJ))) {
	    /* we're returning this option */
	    if (newret == CONFREJ && ret == CONFNAK)
		retp = p0;
	    ret = newret;
	    if (p != retp)
		BCOPY(p, retp, clen);
	    retp += clen;
	}

	p += clen;
	len -= clen;
    }

    if (ret != CONFACK) {
	if (ret == CONFREJ && *lenp == retp - p0)
	    all_rejected = 1;
	else
	    *lenp = retp - p0;
    }
    return ret;
}

/*
 * Make a string name for a compression method (or 2).
 */
static char *
method_name(opt, opt2)
    ccp_options *opt, *opt2;
{
    static char result[64];

    if (!ANY_COMPRESS(*opt))
	return "(none)";
    switch (opt->method) {
    case CI_DEFLATE:
    case CI_DEFLATE_DRAFT:
	if (opt2 != NULL && opt2->deflate_size != opt->deflate_size)
	    snprintf(result, sizeof result, "Deflate%s (%d/%d)",
		    (opt->method == CI_DEFLATE_DRAFT? "(old#)": ""),
		    opt->deflate_size, opt2->deflate_size);
	else
	    snprintf(result, sizeof result, "Deflate%s (%d)",
		    (opt->method == CI_DEFLATE_DRAFT? "(old#)": ""),
		    opt->deflate_size);
	break;
#ifdef MPPE
    case CI_MPPE:
	if (opt->mppe_40) {
	    if (opt->mppe_stateless) {
		return "MPPE 40 bit, stateless";
	    } else {
		    return "MPPE 40 bit, stateful";
	    }
	} else if (opt->mppe_56) {
	    if (opt->mppe_stateless) {
		    return "MPPE 56 bit, stateless";
	    } else {
		    return "MPPE 56 bit, stateful";
	    }
	} else if (opt->mppe_128) {
	    if (opt->mppe_stateless) {
		return "MPPE 128 bit, stateless";
	    } else {
		    return "MPPE 128 bit, stateful";
	    }
	} else {
	    return "MPPC";
	}
#endif
    case CI_LZS:
	return "Stac LZS";
    case CI_BSD_COMPRESS:
	if (opt2 != NULL && opt2->bsd_bits != opt->bsd_bits)
	    snprintf(result, sizeof result,
		    "BSD-Compress (%d/%d)", opt->bsd_bits,
		    opt2->bsd_bits);
	else
	    snprintf(result, sizeof result, "BSD-Compress (%d)", opt->bsd_bits);
	break;
    case CI_PREDICTOR_1:
	return "Predictor 1";
    case CI_PREDICTOR_2:
	return "Predictor 2";
    default:
	snprintf(result, sizeof result, "Method %d", opt->method);
    }
    return result;
}

/*
 * CCP has come up - inform the kernel driver and log a message.
 */
static void
ccp_up(f)
    fsm *f;
{
    ccp_options *go = &f->pPppIf->ccp_gotoptions;
    ccp_options *ho = &f->pPppIf->ccp_hisoptions;
    char method1[64];

    ccp_flags_set(&f->pPppIf->pSc->sc_flags,1, 1);
    if (ANY_COMPRESS(*go)) {
	if (ANY_COMPRESS(*ho)) {
	    if (go->method == ho->method) {
		syslog(SYS_LOG_NOTICE, "%s compression enabled",
		       method_name(go, ho));
	    } else {
		strncpy(method1, method_name(go, NULL), sizeof method1);
		syslog(SYS_LOG_NOTICE, "%s / %s compression enabled",
		       method1, method_name(ho, NULL));
	    }
	} else
	    syslog(SYS_LOG_NOTICE, "%s receive compression enabled",
		   method_name(go, NULL));
    } else if (ANY_COMPRESS(*ho))
	syslog(SYS_LOG_NOTICE, "%s transmit compression enabled",
	       method_name(ho, NULL));
}

/*
 * CCP has gone down - inform the kernel driver.
 */
static void
ccp_down(f)
    fsm *f;
{
    if (ccp_localstate & RACK_PENDING)
	//UNTIMEOUT(ccp_rack_timeout, f);
	PPP_UNTIMEOUT(ccp_rack_timeout, (caddr_t)f,f->pPppIf);
    ccp_localstate = 0;
    ccp_flags_set(&f->pPppIf->pSc->sc_flags,1, 0);
}

/*
 * Print the contents of a CCP packet.
 */
static char *ccp_codenames[] = {
    "ConfReq", "ConfAck", "ConfNak", "ConfRej",
    "TermReq", "TermAck", "CodeRej",
    NULL, NULL, NULL, NULL, NULL, NULL,
    "ResetReq", "ResetAck",
};

static int
ccp_printpkt(p, plen, printer, arg)
    u_char *p;
    int plen;
    void (*printer)(void *, char *, ...);
    void *arg;
{
    u_char *p0, *optend;
    int code, id, len;
    int optlen;

    p0 = p;
    if (plen < HEADERLEN)
	return 0;
    code = p[0];
    id = p[1];
    len = (p[2] << 8) + p[3];
    if (len < HEADERLEN || len > plen)
	return 0;

    if (code >= 1 && code <= sizeof(ccp_codenames) / sizeof(char *)
	&& ccp_codenames[code-1] != NULL)
	printer(arg, " %s", ccp_codenames[code-1]);
    else
	printer(arg, " code=0x%x", code);
    printer(arg, " id=0x%x", id);
    len -= HEADERLEN;
    p += HEADERLEN;

    switch (code) {
    case CONFREQ:
    case CONFACK:
    case CONFNAK:
    case CONFREJ:
	/* print list of possible compression methods */
	while (len >= 2) {
	    code = p[0];
	    optlen = p[1];
	    if (optlen < 2 || optlen > len)
		break;
	    printer(arg, " <");
	    len -= optlen;
	    optend = p + optlen;
	    switch (code) {
	    case CI_DEFLATE:
	    case CI_DEFLATE_DRAFT:
		if (optlen >= CILEN_DEFLATE) {
		    printer(arg, "deflate%s %d",
			    (code == CI_DEFLATE_DRAFT? "(old#)": ""),
			    DEFLATE_SIZE(p[2]));
		    if (DEFLATE_METHOD(p[2]) != DEFLATE_METHOD_VAL)
			printer(arg, " method %d", DEFLATE_METHOD(p[2]));
		    if (p[3] != DEFLATE_CHK_SEQUENCE)
			printer(arg, " check %d", p[3]);
		    p += CILEN_DEFLATE;
		}
		break;
	    case CI_BSD_COMPRESS:
		if (optlen >= CILEN_BSD_COMPRESS) {
		    printer(arg, "bsd v%d %d", BSD_VERSION(p[2]),
			    BSD_NBITS(p[2]));
		    p += CILEN_BSD_COMPRESS;
		}
		break;
	    case CI_PREDICTOR_1:
		if (optlen >= CILEN_PREDICTOR_1) {
		    printer(arg, "predictor 1");
		    p += CILEN_PREDICTOR_1;
		}
		break;
	    case CI_MPPE:
		if (optlen >= CILEN_MPPE) {
		    printer(arg, "mppe %x %x %x %x",p[2],p[3],p[4],p[5]);
		    p += CILEN_MPPE;
		}
		break;
	    case CI_LZS:
		if (optlen >= CILEN_LZS) {
		    printer(arg, "lzs %x %x %x", p[2], p[3], p[4]);
		    p += CILEN_LZS;
		}
		break;
	    case CI_PREDICTOR_2:
		if (optlen >= CILEN_PREDICTOR_2) {
		    printer(arg, "predictor 2");
		    p += CILEN_PREDICTOR_2;
		}
		break;
	    }
	    while (p < optend)
		printer(arg, " %.2x", *p++);
	    printer(arg, ">");
	}
	break;

    case TERMACK:
    case TERMREQ:
	if (len > 0 && *p >= ' ' && *p < 0x7f) {
		//yfchou mark
	    //print_string(p, len, printer, arg);
	    p += len;
	    len = 0;
	}
	break;
    }

    /* dump out the rest of the packet in hex */
    while (--len >= 0)
	printer(arg, " %.2x", *p++);

    return p - p0;
}

/*
 * We have received a packet that the decompressor failed to
 * decompress.  Here we would expect to issue a reset-request, but
 * Motorola has a patent on resetting the compressor as a result of
 * detecting an error in the decompressed data after decompression.
 * (See US patent 5,130,993; international patent publication number
 * WO 91/10289; Australian patent 73296/91.)
 *
 * So we ask the kernel whether the error was detected after
 * decompression; if it was, we take CCP down, thus disabling
 * compression :-(, otherwise we issue the reset-request.
 */
 void
ccp_datainput(	PPP_IF_VAR_T *pPppIf)

{
    fsm *f;

    f = &pPppIf->ccp_fsm;
    if (f->state == OPENED) {
#if 0 //yfchou added   	
	if (ccp_fatal_error(unit)) {
	    /*
	     * Disable compression by taking CCP down.
	     */
	    syslog(SYS_LOG_ERR, "Lost compression sync: disabling compression");
	    ccp_close(unit, "Lost compression sync");
	} else
#endif	
    {
    	if ((f->pPppIf->ccp_gotoptions.method == CI_LZS) ||
		(f->pPppIf->ccp_gotoptions.mppe )) {
		diag_printf("\n[ccp_datainput]:sending reset request\n");
		fsm_sdata(f, CCP_RESETREQ, f->reqid = ++f->id, NULL, 0);
		return;
	    }
	    /*
	     * Send a reset-request to reset the peer's compressor.
	     * We don't do that if we are still waiting for an
	     * acknowledgement to a previous reset-request.
	     */
	    if (!(ccp_localstate & RACK_PENDING)) {
		fsm_sdata(f, CCP_RESETREQ, f->reqid = ++f->id, NULL, 0);
		//TIMEOUT(ccp_rack_timeout, f, RACKTIMEOUT);
		PPP_TIMEOUT((timeout_fun *)ccp_rack_timeout, f, RACKTIMEOUT,f->pPppIf);
		ccp_localstate |= RACK_PENDING;
	    } else
		ccp_localstate |= RREQ_REPEAT;
	}
    }
}

/*
 * Timeout waiting for reset-ack.
 */
static void
ccp_rack_timeout(arg)
    void *arg;
{
    fsm *f = arg;

    if (f->state == OPENED && ccp_localstate & RREQ_REPEAT) {
	fsm_sdata(f, CCP_RESETREQ, f->reqid, NULL, 0);
	//TIMEOUT(ccp_rack_timeout, f, RACKTIMEOUT);
	PPP_TIMEOUT(ccp_rack_timeout, f, RACKTIMEOUT, f->pPppIf);
	ccp_localstate &= ~RREQ_REPEAT;
    } else
	ccp_localstate &= ~RACK_PENDING;
}
// added for tmp
int
ccp_test(pPppIf, opt_ptr, opt_len, for_transmit)
    PPP_IF_VAR_T *pPppIf; 
    int opt_len, for_transmit;
    u_char *opt_ptr;
{
	struct ifnet *pIf = &pPppIf->ifnet;
	struct ppp_option_data data;

    data.ptr = opt_ptr;
    data.length = opt_len;
    data.transmit = for_transmit;
    
	if(pIf->if_ioctl(pIf, PPPIOCSCOMPRESS, (caddr_t) &data)>=0)
	{
		return 1;
	}
	return -1;
}
	
void ccp_flags_set (int *val, int isopen, int isup)
{
	int x = *val;
	x = isopen? x | SC_CCP_OPEN : x &~ SC_CCP_OPEN;
	x = isup?   x | SC_CCP_UP   : x &~ SC_CCP_UP;
	*val = x;
}

