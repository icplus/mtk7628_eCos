/* lcp.h - Link Control Protocol header */

/*
 * lcp.h - Link Control Protocol definitions.
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

#ifndef __INClcph
#define __INClcph

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * Options.
 */
#define CI_MRU              1       /* Maximum Receive Unit */
#define CI_ASYNCMAP         2       /* Async Control Character Map */
#define CI_AUTHTYPE         3       /* Authentication Type */
#define CI_QUALITY          4       /* Quality Protocol */
#define CI_MAGICNUMBER      5       /* Magic Number */
#define CI_PCOMPRESSION     7       /* Protocol Field Compression */
#define CI_ACCOMPRESSION    8       /* Address/Control Field Compression */
#define CI_CALLBACK	        13	    /* callback */
#define CI_MRRU		        17	    /* max reconstructed receive unit; multilink */
#define CI_SSNHF	        18	    /* short sequence numbers for multilink */
#define CI_EPDISC	        19	    /* endpoint discriminator */

/*
 * LCP-specific packet types.
 */
#define PROTREJ             8       /* Protocol Reject */
#define ECHOREQ             9       /* Echo Request */
#define ECHOREP             10      /* Echo Reply */
#define DISCREQ             11      /* Discard Request */
#define CBCP_OPT	        6	    /* Use callback control protocol */

/*
 * Length of each type of configuration option (in octets)
 */
#define CILEN_VOID          2
#define CILEN_SHORT         4       /* CILEN_VOID + sizeof(short) */
#define CILEN_CHAP          5       /* CILEN_VOID + sizeof(short) + 1 */
#define CILEN_LONG          6       /* CILEN_VOID + sizeof(long) */
#define CILEN_LQR           8       /* CILEN_VOID + sizeof(short) + sizeof(long) */

/*
 * The state of options is described by an lcp_options structure.
 */
typedef struct lcp_options {
    u_int passive : 1;                /* Don't die if we don't get a response */
    u_int silent : 1;                 /* Wait for the other end to start first */
    u_int restart : 1;                /* Restart vs. exit after close */
    u_int neg_mru : 1;                /* Negotiate the MRU? */
    u_int neg_asyncmap : 1;           /* Negotiate the async map? */
    u_int neg_upap : 1;               /* Ask for UPAP authentication? */
    u_int neg_chap : 1;               /* Ask for CHAP authentication? */
    u_int neg_magicnumber : 1;        /* Ask for magic number? */
    u_int neg_pcompression : 1;       /* HDLC Protocol Field Compression? */
    u_int neg_accompression : 1;      /* HDLC Address/Control Field Compression? */
    u_int neg_lqr : 1;                /* Negotiate use of Link Quality Reports */
    u_int use_digest : 1;
    u_int use_chapms : 1;
    u_int use_chapms_v2 : 1;
    u_short mru;                    /* Value of MRU */
    u_char chap_mdtype;             /* which MD type (hashing algorithm) */
    u_long asyncmap;                /* Value of async map */
    u_long magicnumber;
    int numloops;                   /* Number of loops during magic number neg. */
    u_long lqr_period;              /* Reporting period for link quality */
} lcp_options;

#define DEFMRU              1500    /* Try for this */
#define MINMRU              128     /* No MRUs below this */
#define MAXMRU              16384   /* Normally limit MRU to this */

extern void lcp_init        (PPP_IF_VAR_T *pPppIf);
extern void lcp_open        (PPP_IF_VAR_T *pPppIf);
extern void lcp_close       (PPP_IF_VAR_T *pPppIf);
extern void lcp_lowerup     (PPP_IF_VAR_T *pPppIf);
extern void lcp_lowerdown   (PPP_IF_VAR_T *pPppIf);
extern void lcp_input       (PPP_IF_VAR_T *pPppIf, u_char *p, int len);
extern void lcp_protrej     (PPP_IF_VAR_T *pPppIf);
extern void lcp_sprotrej    (PPP_IF_VAR_T *pPppIf, u_char *p, int len);

extern int lcp_warnloops;           /* Warn about a loopback this often */
#define DEFWARNLOOPS        10      /* Default value for above */

#ifdef  __cplusplus
}
#endif

#endif	/* __INClcph */


