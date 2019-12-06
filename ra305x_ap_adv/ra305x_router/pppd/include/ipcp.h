/* ipcp.h - IP Control Protocol header */

/*
 * ipcp.h - IP Control Protocol definitions.
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

#ifndef __INCipcph
#define __INCipcph

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * Options.
 */
#define CI_ADDRS                1       /* IP Addresses */
#define CI_COMPRESSTYPE         2       /* Compression Type */
#define CI_ADDR                 3

//panda+
#define CI_PDNSADDRS            0x81    /* CODE - Primary DNS */
#define CI_PNBNSADDRS           0x82    /* CODE - Primary NBNS */
#define CI_SDNSADDRS            0x83    /* CODE - Secondary DNS */
#define CI_SNBNSADDRS           0x84    /* CODE - Secondary NBNS */
//panda+ above

#define MAX_STATES              16      /* from ppp_slcompress.h */

#define IPCP_VJMODE_OLD         1       /* "old" mode (option # = 0x0037) */
#define IPCP_VJMODE_RFC1172     2       /* "old-rfc"mode (option # = 0x002d) */
#define IPCP_VJMODE_RFC1332     3       /* "new-rfc"mode (option # = 0x002d, */
                                        /*  maxslot and slot number compression) */

#define IPCP_VJ_COMP            0x002d  /* current value for VJ compression option*/
#define IPCP_VJ_COMP_OLD        0x0037  /* "old" (i.e, broken) value for VJ */
                                        /* compression option*/ 

/*
 * Lengths of configuration options.
 */
#define CILEN_VOID              2
#define CILEN_COMPRESS          4       /* min length for compression protocol opt. */
#define CILEN_VJ                6       /* length for RFC1332 Van-Jacobson opt. */
#define CILEN_ADDR              6       /* new-style single address option */
#define CILEN_ADDRS             10      /* old-style dual address option */


typedef struct ipcp_options {
    u_int neg_addr : 1;                   /* Negotiate IP Address? */
    u_int old_addrs : 1;                  /* Use old (IP-Addresses) option? */
    u_int req_addr : 1;                   /* Ask peer to send IP address? */
    u_int default_route : 1;              /* Assign default route through interface? */
    u_int proxy_arp : 1;                  /* Make proxy ARP entry for peer? */
    u_int neg_vj : 1;                     /* Van Jacobson Compression? */
    u_int old_vj : 1;                     /* use old (short) form of VJ option? */
    u_int accept_local : 1;               /* accept peer's value for ouraddr */
    u_int accept_remote : 1;              /* accept peer's value for hisaddr */
    u_int neg_pdns : 1;                   /* Primary DNS? */
    u_int neg_pnbns : 1;                  /* Primary NBNS? */
    u_int neg_sdns : 1;                   /* Secondary DNS? */
    u_int neg_snbns : 1;                  /* Secondary NBNS? */
    u_short vj_protocol;                /* protocol value to use in VJ option */
    u_char  maxslotindex,
            cflag;                      /* values for RFC1332 VJ compression neg. */
    u_long  ouraddr,
            hisaddr,
            pdnsaddr,
            pnbnsaddr,
            sdnsaddr,
            snbnsaddr;                  /* Addresses in NETWORK BYTE ORDER panda+dns&nbns*/
} ipcp_options;

extern void ipcp_init           (PPP_IF_VAR_T *pPppIf);
extern void ipcp_open           (PPP_IF_VAR_T *pPppIf);
extern void ipcp_close          (PPP_IF_VAR_T *pPppIf);
extern void ipcp_lowerup        (PPP_IF_VAR_T *pPppIf);
extern void ipcp_lowerdown      (PPP_IF_VAR_T *pPppIf);
extern void ipcp_input          (PPP_IF_VAR_T *pPppIf, u_char *p, int len);
extern void ipcp_protrej        (PPP_IF_VAR_T *pPppIf);
extern char *ip_ntoa            (u_long);

#ifdef  __cplusplus
}
#endif

#endif /* __INCipcph */


