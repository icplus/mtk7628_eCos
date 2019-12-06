/*
 * chap.c - Challenge Handshake Authentication Protocol.
 *
 * Copyright (c) 1993 The Australian National University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the Australian National University.  The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Copyright (c) 1991 Gregory M. Christy.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Gregory M. Christy.  The name of the author may not be used to
 * endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
  //#define DEBUGCHAP
#include <stdlib.h>
#include <sys_inc.h>
#include <pppd.h>

#ifdef CHAPMS
#include "chap_ms.h"
#endif
#include "eventlog.h"


static void ChapChallengeTimeout    (caddr_t);
static void ChapResponseTimeout     (caddr_t);
static void ChapReceiveChallenge    (chap_state *, u_char *, int, int);
static void ChapReceiveResponse     (chap_state *, u_char *, int, int);
static void ChapReceiveSuccess      (chap_state *, u_char *, int, int);
static void ChapReceiveFailure      (chap_state *, u_char *, int, int);
static void ChapSendStatus          (chap_state *, int);
static void ChapSendChallenge       (chap_state *);
static void ChapSendResponse        (chap_state *);
void ChapGenChallenge        (chap_state *);

#ifdef DEBUGCHAP
void dbg_hexdump(u_char *str, u_char *buf, int len)
{
	int i;
	u_char *p=buf;

	diag_printf("%s", str);
	for (i=0; i<len; i++) {
		if (!(i & 0xf))
			diag_printf("\n%04x:", i);
		diag_printf(" %02x", *p++);
	}
	diag_printf("\n");
}
#define DBG_HEXDUMP(a, b, c)		dbg_hexdump(a, b, c)
#else
#define DBG_HEXDUMP(a, b, c)
#endif

/*
 * ChapInit - Initialize a CHAP unit.
 */
void
ChapInit(PPP_IF_VAR_T *pPppIf)
{
    chap_state *cstate = &pPppIf->chap;

    BZERO((char *)cstate, sizeof(*cstate));
    cstate->pPppIf = pPppIf;
    cstate->clientstate = CHAPCS_INITIAL;
    cstate->serverstate = CHAPSS_INITIAL;
    cstate->timeouttime = CHAP_DEFTIMEOUT;
    cstate->max_transmits = CHAP_DEFTRANSMITS;
    
    srand((long) cyg_current_time());    /* joggle random number generator */
}


/*
 * ChapAuthWithPeer - Authenticate us with our peer (start client).
 *
 */
void
ChapAuthWithPeer(PPP_IF_VAR_T *pPppif, char *our_name, int digest)
{
    chap_state *cstate = &pPppif->chap;

    cstate->resp_name = our_name;
    cstate->resp_type = digest;

    if (cstate->clientstate == CHAPCS_INITIAL ||
        cstate->clientstate == CHAPCS_PENDING)
    {
        /* lower layer isn't up - wait until later */
        cstate->clientstate = CHAPCS_PENDING;
        return;
    }

    /*
     * We get here as a result of LCP coming up.
     * So even if CHAP was open before, we will 
     * have to re-authenticate ourselves.
     */
    cstate->clientstate = CHAPCS_LISTEN;
}


/*
 * ChapAuthPeer - Authenticate our peer (start server).
 */
void
ChapAuthPeer(PPP_IF_VAR_T *pPppIf, char *our_name, int digest)
{
    chap_state *cstate = &pPppIf->chap;
  
    cstate->chal_name = our_name;
    cstate->chal_type = digest;

    if (cstate->serverstate == CHAPSS_INITIAL ||
        cstate->serverstate == CHAPSS_PENDING)
    {
        /* lower layer isn't up - wait until later */
        cstate->serverstate = CHAPSS_PENDING;
        return;
    }

    ChapGenChallenge(cstate);
    ChapSendChallenge(cstate);      /* crank it up dude! */
    cstate->serverstate = CHAPSS_INITIAL_CHAL;
}


/*
 * ChapChallengeTimeout - Timeout expired on sending challenge.
 */
static void
ChapChallengeTimeout(arg)
    caddr_t arg;
{
    chap_state *cstate = (chap_state *) arg;
  
    /* if we aren't sending challenges, don't worry.  then again we */
    /* probably shouldn't be here either */
    if (cstate->serverstate != CHAPSS_INITIAL_CHAL &&
        cstate->serverstate != CHAPSS_RECHALLENGE)
        return;

    if (cstate->chal_transmits >= cstate->max_transmits)
    {
        /* give up on peer */
        CHAPDEBUG("Peer failed to respond to CHAP challenge\n");
        cstate->serverstate = CHAPSS_BADAUTH;
        auth_peer_fail(cstate->pPppIf, CHAP);
        return;
    }

    ChapSendChallenge(cstate);      /* Re-send challenge */
}


/*
 * ChapResponseTimeout - Timeout expired on sending response.
 */
static void
ChapResponseTimeout(arg)
    caddr_t arg;
{
    chap_state *cstate = (chap_state *) arg;

    /* if we aren't sending a response, don't worry. */
    if (cstate->clientstate != CHAPCS_RESPONSE)
        return;

    ChapSendResponse(cstate);       /* re-send response */
}


/*
 * ChapRechallenge - Time to challenge the peer again.
 */
static void
ChapRechallenge(arg)
    caddr_t arg;
{
    chap_state *cstate = (chap_state *) arg;

    /* if we aren't sending a response, don't worry. */
    if (cstate->serverstate != CHAPSS_OPEN)
        return;

    ChapGenChallenge(cstate);
    ChapSendChallenge(cstate);
    cstate->serverstate = CHAPSS_RECHALLENGE;

    if (cstate->chal_interval != 0)
        PPP_TIMEOUT(ChapRechallenge, (caddr_t) cstate, cstate->chal_interval, cstate->pPppIf);
}


/*
 * ChapLowerUp - The lower layer is up.
 *
 * Start up if we have pending requests.
 */
void
ChapLowerUp(PPP_IF_VAR_T *pPppIf)
{
    chap_state *cstate = &pPppIf->chap;
  
    if (cstate->clientstate == CHAPCS_INITIAL)
        cstate->clientstate = CHAPCS_CLOSED;
    else if (cstate->clientstate == CHAPCS_PENDING)
        cstate->clientstate = CHAPCS_LISTEN;

    if (cstate->serverstate == CHAPSS_INITIAL)
        cstate->serverstate = CHAPSS_CLOSED;
    else if (cstate->serverstate == CHAPSS_PENDING)
    {
        ChapGenChallenge(cstate);
        ChapSendChallenge(cstate);
        cstate->serverstate = CHAPSS_INITIAL_CHAL;
    }
}


/*
 * ChapLowerDown - The lower layer is down.
 *
 * Cancel all timeouts.
 */
void
ChapLowerDown(PPP_IF_VAR_T *pPppIf)
{
    chap_state *cstate = &pPppIf->chap;
  
    /* Timeout(s) pending?  Cancel if so. */
    if (cstate->serverstate == CHAPSS_INITIAL_CHAL ||
        cstate->serverstate == CHAPSS_RECHALLENGE)
    {
        PPP_UNTIMEOUT(ChapChallengeTimeout, (caddr_t) cstate, cstate->pPppIf);
    }
    else if (cstate->serverstate == CHAPSS_OPEN
             && cstate->chal_interval != 0)
    {
        PPP_UNTIMEOUT(ChapRechallenge, (caddr_t) cstate, cstate->pPppIf);
    }

    if (cstate->clientstate == CHAPCS_RESPONSE)
        PPP_UNTIMEOUT(ChapResponseTimeout, (caddr_t) cstate, cstate->pPppIf);

    cstate->clientstate = CHAPCS_INITIAL;
    cstate->serverstate = CHAPSS_INITIAL;
}


/*
 * ChapProtocolReject - Peer doesn't grok CHAP.
 */
void
ChapProtocolReject(PPP_IF_VAR_T *pPppIf)
{
    chap_state *cstate = &pPppIf->chap;

    if (cstate->serverstate != CHAPSS_INITIAL &&
        cstate->serverstate != CHAPSS_CLOSED)
    {
        auth_peer_fail(pPppIf, CHAP);
    }
    if (cstate->clientstate != CHAPCS_INITIAL &&
        cstate->clientstate != CHAPCS_CLOSED)
    {
        auth_withpeer_fail(pPppIf, CHAP);
    }

    ChapLowerDown(pPppIf);        /* shutdown chap */
}


/*
 * ChapInput - Input CHAP packet.
 */
void
ChapInput (PPP_IF_VAR_T *pPppIf, u_char *inpacket, int packet_len)
{
    chap_state *cstate = &pPppIf->chap;
    u_char *inp;
    u_char code, id;
    int len;
    /*
     * Parse header (code, id and length).
     * If packet too short, drop it.
     */
    inp = inpacket;
    if (packet_len < CHAP_HEADERLEN)
    {
        CHAPDEBUG("ChapInput: rcvd short header.\n");
        return;
    }

    GETCHAR(code, inp);
    GETCHAR(id, inp);
    GETSHORT(len, inp);
    if (len < CHAP_HEADERLEN)
    {
        CHAPDEBUG("ChapInput: rcvd illegal length.\n");
        return;
    }

    if (len > packet_len)
    {
        CHAPDEBUG("ChapInput: rcvd short packet.\n");
        return;
    }
    len -= CHAP_HEADERLEN;
  
    /*
     * Action depends on code (as in fact it usually does :-).
     */
    switch (code) 
    {
    case CHAP_CHALLENGE:
        ChapReceiveChallenge(cstate, inp, id, len);
        break;

    case CHAP_RESPONSE:
        ChapReceiveResponse(cstate, inp, id, len);
        break;

    case CHAP_FAILURE:
        ChapReceiveFailure(cstate, inp, id, len);
        break;

    case CHAP_SUCCESS:
        ChapReceiveSuccess(cstate, inp, id, len);
        break;

    default:            /* Need code reject? */
        CHAPDEBUG("Unknown CHAP code (%d) received.\n", code);
        break;
    }
}


/*
 * ChapReceiveChallenge - Receive Challenge and send Response.
 */
static void
ChapReceiveChallenge(cstate, inp, id, len)
    chap_state *cstate;
    u_char *inp;
    int id;
    int len;
{
    int rchallenge_len;
    u_char *rchallenge;
    int secret_len;
    char secret[MAXSECRETLEN];
    char rhostname[256];
    MD5_CTX mdContext;
 
    CHAPDEBUG("ChapReceiveChallenge: Rcvd id %d.\n", id);
    if (cstate->clientstate == CHAPCS_CLOSED ||
        cstate->clientstate == CHAPCS_PENDING)
    {
        CHAPDEBUG("ChapReceiveChallenge: in state %d\n", cstate->clientstate);
        return;
    }

    if (len < 2)
    {
        CHAPDEBUG("ChapReceiveChallenge: rcvd short packet.\n");
        return;
    }

    GETCHAR(rchallenge_len, inp);
    len -= sizeof (u_char) + rchallenge_len;    /* now name field length */
    if (len < 0) 
    {
        CHAPDEBUG("ChapReceiveChallenge: rcvd short packet.\n");
        return;
    }
    rchallenge = inp;
    INCPTR(rchallenge_len, inp);

    if (len >= sizeof(rhostname))
        len = sizeof(rhostname) - 1;

    BCOPY((char *)inp, rhostname, len);
    rhostname[len] = '\000';

    CHAPDEBUG("ChapReceiveChallenge: received name field: %s\n", rhostname);

    /* get secret for authenticating ourselves with the specified host */
    if (!get_secret(cstate->pPppIf, cstate->resp_name, rhostname,
                    secret, &secret_len, 0))
    {
        secret_len = 0;         /* assume null secret if can't find one */
        CHAPDEBUG("No CHAP secret found for authenticating us to %s\n",
               rhostname);
    }

    /* cancel response send timeout if necessary */
    if (cstate->clientstate == CHAPCS_RESPONSE)
        PPP_UNTIMEOUT(ChapResponseTimeout, (caddr_t) cstate, cstate->pPppIf);

    cstate->resp_id = id;
    cstate->resp_transmits = 0;

    /*  generate MD based on negotiated type */
    cstate->chal_type = cstate->resp_type;

    switch (cstate->resp_type)
    {
    case CHAP_DIGEST_MD5:       /* only MD5 is defined for now */
        MD5Init(&mdContext);
        MD5Update(&mdContext, &cstate->resp_id, 1);
        MD5Update(&mdContext, (u_char *) secret, secret_len);
        MD5Update(&mdContext, rchallenge, rchallenge_len);
        MD5Final((char *)cstate->response, &mdContext);
        cstate->resp_length = MD5_SIGNATURE_SIZE;
        break;

#ifdef CHAPMS
    case CHAP_MICROSOFT:
	    CHAPDEBUG(("ChapReceiveChallenge: rcvd type MS-CHAP-V1.\n"));
	    if (rchallenge_len < 8)
	    {
	        CHAPDEBUG(("Invalid challenge length for MS-CHAP-V1\n"));
	        return;
	    }
	    ChapMS(cstate, rchallenge, rchallenge_len, secret, secret_len);
	    break;
    case CHAP_MICROSOFT_V2:
        CHAPDEBUG(("ChapReceiveChallenge: rcvd type MS-CHAP-V2.\n"));
	    if (rchallenge_len != 16)
	    {
	        CHAPDEBUG(("Invalid challenge length for MS-CHAP-V2\n"));
	        return;
	    }
	    ChapMS_v2(cstate, rchallenge, rchallenge_len, secret, secret_len);
	    break;
#endif

    default:
        CHAPDEBUG("unknown digest type %d\n", cstate->resp_type);
        return;
    }

    BZERO(secret, sizeof(secret));
    ChapSendResponse(cstate);
}


/*
 * ChapReceiveResponse - Receive and process response.
 */
static void
ChapReceiveResponse(cstate, inp, id, len)
    chap_state *cstate;
    u_char *inp;
    int id;
    int len;
{
    u_char *remmd, remmd_len;
    int secret_len, old_state;
    int code;
    char rhostname[256];
    MD5_CTX mdContext;
    char secret[MAXSECRETLEN];
    u_char hash[MD5_SIGNATURE_SIZE];

    CHAPDEBUG("ChapReceiveResponse: Rcvd id %d.\n", id);

    if (cstate->serverstate == CHAPSS_CLOSED ||
        cstate->serverstate == CHAPSS_PENDING)
    {
        CHAPDEBUG("ChapReceiveResponse: in state %d\n", cstate->serverstate);
        return;
    }

    if (id != cstate->chal_id)
        return;                 /* doesn't match ID of last challenge */

    /*
     * If we have received a duplicate or bogus Response,
     * we have to send the same answer (Success/Failure)
     * as we did for the first Response we saw.
     */
    if (cstate->serverstate == CHAPSS_OPEN)
    {
        ChapSendStatus(cstate, CHAP_SUCCESS);
        return;
    }
    if (cstate->serverstate == CHAPSS_BADAUTH)
    {
        ChapSendStatus(cstate, CHAP_FAILURE);
        return;
    }

    if (len < 2)
    {
        CHAPDEBUG("ChapReceiveResponse: rcvd short packet.\n");
        return;
    }

    GETCHAR(remmd_len, inp);        /* get length of MD */
    remmd = inp;                    /* get pointer to MD */
    INCPTR(remmd_len, inp);

    len -= sizeof (u_char) + remmd_len;
    if (len < 0)
    {
        CHAPDEBUG("ChapReceiveResponse: rcvd short packet.\n");
        return;
    }

    PPP_UNTIMEOUT(ChapChallengeTimeout, (caddr_t) cstate, cstate->pPppIf);

    if (len >= sizeof(rhostname))
        len = sizeof(rhostname) - 1;

    BCOPY((char *)inp, rhostname, len);
    rhostname[len] = '\000';

    CHAPDEBUG("ChapReceiveResponse: received name field: %s\n", rhostname);

    /*
     * Get secret for authenticating them with us,
     * do the hash ourselves, and compare the result.
     */
    code = CHAP_FAILURE;
    if (!get_secret(cstate->pPppIf, rhostname, cstate->chal_name,
                    secret, &secret_len, 1))
    {
        CHAPDEBUG("No CHAP secret found for authenticating %s\n", rhostname);
    }
    else
    {
        /*  generate MD based on negotiated type */
        switch (cstate->chal_type)
        {
        case CHAP_DIGEST_MD5:       /* only MD5 is defined for now */
            if (remmd_len != MD5_SIGNATURE_SIZE)
                break;          /* it's not even the right length */

            MD5Init(&mdContext);
            MD5Update(&mdContext, &cstate->chal_id, 1);
            MD5Update(&mdContext, (u_char *) secret, secret_len);
            MD5Update(&mdContext, cstate->challenge, cstate->chal_len);
            MD5Final(hash, &mdContext); 

            /* compare local and remote MDs and send the appropriate status */
            if (bcmp ((char *)hash, (char *)remmd, MD5_SIGNATURE_SIZE) == 0)
                code = CHAP_SUCCESS;    /* they are the same! */
            break;

#ifdef CHAPMS
	    case CHAP_MICROSOFT:
	        CHAPDEBUG(("ChapReceiveResponse: rcvd type MS-CHAP-V1\n"));
	        if (remmd_len != MS_CHAP_RESPONSE_LEN)
		        break;
			
	        if (ChapMS_Resp(cstate, secret, secret_len, remmd) == 0)
		        code = CHAP_SUCCESS;
	        break;
			
	    case CHAP_MICROSOFT_V2:
	        CHAPDEBUG(("ChapReceiveResponse: rcvd type MS-CHAP-V2\n"));
	        if (remmd_len != MS_CHAP_RESPONSE_LEN)
		        break;
			
	        if (ChapMS_v2_Resp(cstate,secret,secret_len,remmd,rhostname) == 0)
	        {
		        code = CHAP_SUCCESS_R;
		        ChapMS_v2_Auth(cstate, secret, secret_len, remmd, rhostname);
	        }
	        break;
#endif
		
        default:
            CHAPDEBUG("unknown digest type %d\n", cstate->chal_type);
        }
    }

    BZERO(secret, sizeof(secret));
    ChapSendStatus(cstate, code);

    if (code == CHAP_SUCCESS || (code == CHAP_SUCCESS_R))
    {
        old_state = cstate->serverstate;
        cstate->serverstate = CHAPSS_OPEN;
        if (old_state == CHAPSS_INITIAL_CHAL)
        {
            auth_peer_success(cstate->pPppIf, CHAP);
        }
		
        if (cstate->chal_interval != 0)
            PPP_TIMEOUT(ChapRechallenge, (caddr_t) cstate,
                        cstate->chal_interval, cstate->pPppIf);

	    switch (cstate->chal_type) 
        { 
	    case CHAP_DIGEST_MD5:
	        CHAPDEBUG("CHAP peer authentication succeeded for %q\n", rhostname);
	        break;
#ifdef CHAPMS
	    case CHAP_MICROSOFT:
	        CHAPDEBUG("MSCHAP peer authentication succeeded for %q\n", rhostname);
	        break;
	    case CHAP_MICROSOFT_V2:
	        CHAPDEBUG("MSCHAP-v2 peer authentication succeeded for %q\n", rhostname);
	        break;
#endif
	    default:
	        CHAPDEBUG("CHAP (unknown) peer authentication succeeded for %q\n", rhostname);
	        break;
	    }
    }
    else
    {
	    switch (cstate->chal_type)
        { 
	    case CHAP_DIGEST_MD5:
	        CHAPDEBUG("CHAP peer authentication failed for remote host %q\n", rhostname);
	        break;
#ifdef CHAPMS
	    case CHAP_MICROSOFT:
	        CHAPDEBUG("MSCHAP peer authentication failed for remote host %q\n", rhostname);
	        break;
	    case CHAP_MICROSOFT_V2:
	        CHAPDEBUG("MSCHAP-v2 peer authentication failed for remote host %q\n", rhostname);
	        break;
#endif
	    default:
	        CHAPDEBUG("CHAP (unknown) peer authentication failed for remote host %q\n", rhostname);
	        break;
	    }

        //CHAPDEBUG("CHAP peer authentication failed\n");
        cstate->serverstate = CHAPSS_BADAUTH;
        auth_peer_fail(cstate->pPppIf, CHAP);
    }
}

/*
 * ChapReceiveSuccess - Receive Success
 */
static void
ChapReceiveSuccess(cstate, inp, id, len)
    chap_state *cstate;
    u_char *inp;
    u_char id;
    int len;
{

    CHAPDEBUG("ChapReceiveSuccess: Rcvd id %d.\n", id);

    if (cstate->clientstate == CHAPCS_OPEN)
        /* presumably an answer to a duplicate response */
        return;

    if (cstate->clientstate != CHAPCS_RESPONSE)
    {
        /* don't know what this is */
        CHAPDEBUG("ChapReceiveSuccess: in state %d\n", cstate->clientstate);
        return;
    }

    PPP_UNTIMEOUT(ChapResponseTimeout, (caddr_t) cstate, cstate->pPppIf);

    /*
     * Print message.
     */
    if (len > 0)
    {
        inp[len] = '\0';
        diag_printf("Remote message: %s\n", (char *) inp);
    }

    cstate->clientstate = CHAPCS_OPEN;

    auth_withpeer_success(cstate->pPppIf, CHAP);
}


/*
 * ChapReceiveFailure - Receive failure.
 */
static void
ChapReceiveFailure(cstate, inp, id, len)
    chap_state *cstate;
    u_char *inp;
    u_char id;
    int len;
{
	SysLog(LOG_USER|SYS_LOG_NOTICE|LOGM_PPPD,"Chap Receive Failure (username/password)");
    CHAPDEBUG("ChapReceiveFailure: Rcvd id %d.\n", id);

    if (cstate->clientstate != CHAPCS_RESPONSE)
    {
        /* don't know what this is */
        CHAPDEBUG("ChapReceiveFailure: in state %d\n", cstate->clientstate);
        return;
    }

    PPP_UNTIMEOUT(ChapResponseTimeout, (caddr_t) cstate, cstate->pPppIf);

    /*
     * Print message.
     */
    if (len > 0) {
        inp[len] = '\0';
        diag_printf("Remote message: %s\n", (char *) inp);
    }

    CHAPDEBUG("CHAP authentication failed\n");
    auth_withpeer_fail(cstate->pPppIf, CHAP);
}


/*
 * ChapSendChallenge - Send an Authenticate challenge.
 */
static void
ChapSendChallenge(cstate)
    chap_state *cstate;
{
    u_char *outp;
    int chal_len, name_len;
    int outlen;

    chal_len = cstate->chal_len;
    name_len = strlen(cstate->chal_name);
    outlen = CHAP_HEADERLEN + sizeof (u_char) + chal_len + name_len;
    outp = cstate->pPppIf->outpacket_buf;

    MAKE_PPPHEADER(outp, CHAP);		/* paste in a CHAP header */
    
    PUTCHAR(CHAP_CHALLENGE, outp);
    PUTCHAR(cstate->chal_id, outp);
    PUTSHORT(outlen, outp);

    PUTCHAR(chal_len, outp);            /* put length of challenge */
    BCOPY((char *)cstate->challenge, (char *)outp, chal_len);
    INCPTR(chal_len, outp);

    BCOPY(cstate->chal_name, (char *)outp, name_len);   /* append hostname */

    output(cstate->pPppIf, cstate->pPppIf->outpacket_buf, outlen + PPPHEADER_LEN);
  
    CHAPDEBUG("ChapSendChallenge: Sent id %d.\n", cstate->chal_id);

    PPP_TIMEOUT(ChapChallengeTimeout, (caddr_t) cstate, cstate->timeouttime, cstate->pPppIf);
    ++cstate->chal_transmits;
}


/*
 * ChapSendStatus - Send a status response (ack or nak).
 */
static void
ChapSendStatus(cstate, code)
    chap_state *cstate;
    int code;
{
    u_char *outp;
    int outlen, msglen;
    char msg[256];

    if (code == CHAP_SUCCESS)
        sprintf(msg, "CHAP authentication success, unit %d", cstate->pPppIf->ifname);
    else
        sprintf(msg, "CHAP authentication failure, unit %d", cstate->pPppIf->ifname);

    msglen = strlen(msg);

    outlen = CHAP_HEADERLEN + msglen;
    outp = cstate->pPppIf->outpacket_buf;

    MAKE_PPPHEADER(outp, CHAP);		/* paste in a CHAP header */
  
    PUTCHAR(code, outp);
    PUTCHAR(cstate->chal_id, outp);
    PUTSHORT(outlen, outp);
    BCOPY(msg, (char *)outp, msglen);
    output(cstate->pPppIf, cstate->pPppIf->outpacket_buf, outlen + PPPHEADER_LEN);
  
    CHAPDEBUG("ChapSendStatus: Sent code %d, id %d.\n", code, cstate->chal_id);
}

/*
 * ChapGenChallenge is used to generate a pseudo-random challenge string of
 * a pseudo-random length between min_len and max_len.  The challenge
 * string and its length are stored in *cstate, and various other fields of
 * *cstate are initialized.
 */

void
ChapGenChallenge(cstate)
    chap_state *cstate;
{
    int chal_len;
    u_char *ptr = cstate->challenge;
    unsigned int i;

#ifdef CHAPMS
    if (cstate->chal_type == CHAP_MICROSOFT)
	    chal_len = 8;
    else if (cstate->chal_type == CHAP_MICROSOFT_V2)
	    chal_len = 16;
    else
#endif
    /* pick a random challenge length between MIN_CHALLENGE_LENGTH and 
       MAX_CHALLENGE_LENGTH */  
#if 0
    chal_len =  (unsigned) ((random() % (MAX_CHALLENGE_LENGTH - MIN_CHALLENGE_LENGTH)) +
                            MIN_CHALLENGE_LENGTH);
#else
    chal_len =  (unsigned) ((rand() % (MAX_CHALLENGE_LENGTH - MIN_CHALLENGE_LENGTH)) +
                            MIN_CHALLENGE_LENGTH);
#endif
                            
    cstate->chal_len = chal_len;
    cstate->chal_id = ++cstate->id;
    cstate->chal_transmits = 0;

    /* generate a random string */
    for (i = 0; i < chal_len; i++ )
    #if 0
        *ptr++ = (char) (random() % 0xff);
    #else
    	*ptr++ = (char) (rand() % 0xff);
    #endif    
}

/*
 * ChapSendResponse - send a response packet with values as specified
 * in *cstate.
 */
/* ARGSUSED */
static void
ChapSendResponse(cstate)
    chap_state *cstate;
{
    u_char *outp;
    int outlen, md_len, name_len;

    md_len = cstate->resp_length;
    name_len = strlen(cstate->resp_name);
    outlen = CHAP_HEADERLEN + sizeof (u_char) + md_len + name_len;
    outp = cstate->pPppIf->outpacket_buf;

    MAKE_PPPHEADER(outp, CHAP);		/* paste in a CHAP header */

    PUTCHAR(CHAP_RESPONSE, outp);       /* we are a response */
    PUTCHAR(cstate->resp_id, outp);     /* copy id from challenge packet */
    PUTSHORT(outlen, outp);             /* packet length */

    PUTCHAR(md_len, outp);              /* length of MD */
    BCOPY((char *)cstate->response, (char *)outp, md_len);      /* copy MD to buffer */
    INCPTR(md_len, outp);

    BCOPY(cstate->resp_name, (char *)outp, name_len);           /* append our name */

    /* send the packet */
    output(cstate->pPppIf, cstate->pPppIf->outpacket_buf, outlen + PPPHEADER_LEN);

    cstate->clientstate = CHAPCS_RESPONSE;
    PPP_TIMEOUT(ChapResponseTimeout, (caddr_t) cstate, cstate->timeouttime, cstate->pPppIf);
    ++cstate->resp_transmits;
}

