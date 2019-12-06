/*
 * upap.h - User/Password Authentication Protocol definitions.
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

#ifndef __INCupaph
#define __INCupaph

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * Packet header = Code, id, length.
 */
#define UPAP_HEADERLEN      (sizeof (u_char) + sizeof (u_char) + sizeof (u_short))


/*
 * UPAP codes.
 */
#define UPAP_AUTHREQ    1   /* Authenticate-Request */
#define UPAP_AUTHACK    2   /* Authenticate-Ack */
#define UPAP_AUTHNAK    3   /* Authenticate-Nak */


/*
 * Each interface is described by upap structure.
 */
typedef struct upap_state {
    PPP_IF_VAR_T *pPppIf;       /* Back pointer to PPP_IF_VAR_T */
    char    *us_user;           /* User */
    int     us_userlen;         /* User length */
    char    *us_passwd;         /* Password */
    int     us_passwdlen;       /* Password length */
    int     us_clientstate;     /* Client state */
    int     us_serverstate;     /* Server state */
    u_char  us_id;              /* Current id */
    int     us_timeouttime;     /* Timeout time in milliseconds */
    int     us_transmits;       /* Number of auth-reqs sent */
    int     us_maxtransmits;    /* Maximum number of auth-reqs to send */
} upap_state;


/*
 * Client states.
 */
#define UPAPCS_INITIAL  0   /* Connection down */
#define UPAPCS_CLOSED   1   /* Connection up, haven't requested auth */
#define UPAPCS_PENDING  2   /* Connection down, have requested auth */
#define UPAPCS_AUTHREQ  3   /* We've sent an Authenticate-Request */
#define UPAPCS_OPEN     4   /* We've received an Ack */
#define UPAPCS_BADAUTH  5   /* We've received a Nak */

/*
 * Server states.
 */
#define UPAPSS_INITIAL  0   /* Connection down */
#define UPAPSS_CLOSED   1   /* Connection up, haven't requested auth */
#define UPAPSS_PENDING  2   /* Connection down, have requested auth */
#define UPAPSS_LISTEN   3   /* Listening for an Authenticate */
#define UPAPSS_OPEN     4   /* We've sent an Ack */
#define UPAPSS_BADAUTH  5   /* We've sent a Nak */


/*
 * Timeouts.
 */
#define UPAP_DEFTIMEOUT     3   /* Timeout time in seconds */

extern void upap_init           (PPP_IF_VAR_T *pPppIf);
extern void upap_authwithpeer   (PPP_IF_VAR_T *, char *, char *);
extern void upap_authpeer       (PPP_IF_VAR_T *);
extern void upap_lowerup        (PPP_IF_VAR_T *);
extern void upap_lowerdown      (PPP_IF_VAR_T *);
extern void upap_input          (PPP_IF_VAR_T *pPppIf, u_char *inpacket, int l);
extern void upap_protrej        (PPP_IF_VAR_T *);

#ifdef  __cplusplus
}
#endif

#endif  /* __INCupaph */


