/*
 * pppd.h - PPP daemon global declarations.
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

#ifndef __PPPD_H__
#define __PPPD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

#ifndef __ARGS
#ifdef __STDC__
#define __ARGS(x)       x
#else
#define __ARGS(x)       ()
#endif
#endif

#include <sys_inc.h>

typedef struct ppp_if_var PPP_IF_VAR_T;

#include <ppp.h>
#include <fsm.h>
#include <chap.h>
#include <ipcp.h>
#include <if_ppp.h>
#include <ppp_auth.h>
#include <lcp.h>
#include <magic.h>
#include <md5.h>
#include <options.h>
#include <sys-ecos.h>
#include <upap.h>
#include <ccp.h>


/* defines */

/*
 * Patch level
 */
#define PATCHLEVEL      2
#define VERSION "2.1"
#define DATE "9 June 94"


/*
 * Values for phase.
 */
#define PHASE_DEAD              0
#define PHASE_ESTABLISH         1
#define PHASE_AUTHENTICATE      2
#define PHASE_NETWORK           3
#define PHASE_TERMINATE         4
#define PHASE_IDLE              5

#ifndef	SIOCSIFMTU
#define SIOCSIFMTU      _IOW('i', 140, int)
#endif	/* SIOCSIFMTU */
#ifndef	SIOCGIFMTU
#define SIOCGIFMTU      _IOR('i', 141, int)
#endif	/* SIOCGIFMTU */

/* Used for storing a sequence of words.  Usually malloced. */
struct wordlist 
{
	struct wordlist *next;
	char word[1];
};
/*
 * Limits.
 */
#define MAXWORDLEN      1024    /* max length of word in file (incl null) */
#define MAXARGS         1       /* max # args to a command */
#define MAXNAMELEN      256     /* max length of hostname or name for auth */
#define MAXSECRETLEN    256     /* max length of password or secret */

#include <cyg/kernel/kapi.h>
typedef int 		(*FUNCPTR) ();	   /* ptr to function returning int */
    /* PPP task variables structure */
struct ppp_if_var
{
    struct ifnet    ifnet;              /* network-visible interface */
    struct ppp_if_var *ppp_next;         /* Pointer to next PPP_IF_VAR_T */

    int flags;                  /* Configuration flag options */
    cyg_handle_t task_id;			/* Task id of pppd task */
    cyg_flag_t event_id;       /* event */

    /* interface varibles */
	int			unit;
    char        ifname[NET_IFNAME_LEN];		/* device interface name */
    char        pppname[NET_IFNAME_LEN];	/* logical ppp interface name */

	struct  ifqueue inq;                /* device side input queue */

    /* timer variables */
    struct ppp_callout  *callout;	/* Callout list */
    time_t          schedtime;		/* Time last timeout was set */
    
    /* configured variables */
    int             debug;			/* Debug flag */
    int             inspeed;        /* Input/Output speed requested */
    u_long          netmask;        /* Netmask to use on PPP interface */
    u_long          my_ip;              /* My IP address */
    u_long          peer_ip;            /* Peer IP address */
	u_long			pri_dns;
	u_long			snd_dns;
    int             auth_required;  /* Require peer to authenticate */
    int             defaultroute;   /* Assign default rte through interf */
    int             proxyarp;       /* Set entry in arp table */
    int             uselogin;       /* Check PAP info against login table */
    int             phase;          /* Where the link is at */
    int             baud_rate;      /* Bits/sec currently used */
    char            user[MAXNAMELEN+33];		/* User name */
    char            passwd[MAXSECRETLEN+33];      /* Password for PAP */
    int             disable_defaultip;		/* Use hostname IP address */
    int             kdebugflag;			/* Enable driver debug */
    char            _hostname[MAXNAMELEN+33];	/* Our host name */
    char            our_name[MAXNAMELEN+33];	/* Authentication host name */
    char            remote_name[MAXNAMELEN+33];	/* Remote host name */

    int             ttystate;			/* Initial TTY state */
    
    int             restore_term;        	/* 1 => we've munged the terminal */
    
    /* Buffer for outgoing packet */
    u_char          outpacket_buf[MTU + DLLHEADERLEN];
    /* Buffer for incoming packet */
    u_char          inpacket_buf[MTU + DLLHEADERLEN];
    
    int             hungup;                     /* Terminal has been hung up */
    
    int             peer_mru;			/* Peer MRU */
    
    /* Records which authentication operations haven't completed yet. */
    int             auth_pending;		/* Authentication state */
    struct wordlist *addresses;	/* List of acceptable IP addresses */
    
    /* lcp echo variables */
    u_long          lcp_echos_pending;	/* Number of outstanding echo msgs */
    u_long          lcp_echo_number;    	/* ID number of next echo frame */
    u_short         lcp_echo_timer_running;	/* TRUE if a timer running */
    u_short	         lcp_line_active;		/*  True if line is active  */
    u_long          lcp_echo_interval;	/* Seconds between intervals */
    u_long          lcp_echo_fails;		/* Number of echo failures */
    
//#ifdef CONFIG_POE_PASSIVE_LCP_ECHO_REPLY
	int				lcp_passive_echo_reply;
	u_long			lcp_serv_echos_pending;
	u_long          lcp_serv_echo_timer_running;
	u_long          lcp_serv_echo_interval;
	u_long          lcp_serv_echo_fails;
//#endif
    
    /* chap variables */
    chap_state      chap;		/* CHAP structure */
    
    /* ipcp variables */
    ipcp_options    ipcp_wantoptions;	/* Options we want to request */
    ipcp_options    ipcp_gotoptions; 	/* Options that peer ack'd */
    ipcp_options    ipcp_allowoptions;	/* Options we allow peer to request */
    ipcp_options    ipcp_hisoptions;	/* Options that we ack'd */
    fsm             ipcp_fsm;            	/* IPCP fsm structure */
    int             cis_received;          	/* # Conf-Reqs received */
    
    /* lcp variables */
    lcp_options     lcp_wantoptions;	/* Options that we want to request */
    lcp_options     lcp_gotoptions;	/* Options that peer ack'd */
    lcp_options     lcp_allowoptions;	/* Options we allow peer to request */
    lcp_options     lcp_hisoptions;	/* Options that we ack'd */
    u_long          xmit_accm[8];		/* extended transmit ACCM */
    fsm             lcp_fsm;			/* LCP fsm structure */
    
    /* ccp variables */
    ccp_options 	ccp_wantoptions;
    ccp_options		ccp_gotoptions;
    ccp_options		ccp_allowoptions;
    ccp_options		ccp_hisoptions;
    fsm				ccp_fsm;
	
    /* upap variables */
    upap_state      upap;		/* UPAP state; one for each unit */
    
    /* SNMP MIBII counters */
    u_long	        unknownProto;		/* MIBII: ifInUnknownProtos */
    int             encapsulate_type;       /* PPP packet encapsulate type e.g. ethernet or AAL5 */ 
    int             mtu;
    int             mru;                /* MTU and MRU that we allow */
	
    int             idle;               /* idle time */
	u_long			last_idle_ticks;	//++ Simon 2006/01/06
	
    int				mppe;
	
    FUNCPTR         pOutput;            /* PPP output function */
    FUNCPTR         pIfStopHook;        /* physical layer interface call back function */
    FUNCPTR         pIfReconnectHook;   /* physical layer reconnection */ 
    FUNCPTR         pIoctl;
    FUNCPTR         pAttach;
    struct ppp_softc *pSc;
    
    //int wan_mode;
    //int lan_ifnet;		// Simon, I don't know what's that
    int ref;
    int authfail;
};

/*
 * This is a copy of /usr/include/sys/callout.h with the c_func
 * member of struct callout changed from a pointer to a function of type int
 * to a pointer to a function of type void (generic pointer) as per ANSI C
 */

struct  ppp_callout 
{
    int     c_time;         /* incremental time */
    caddr_t c_arg;          /* argument to routine */
    PPP_IF_VAR_T *pPppIf;   /* Back pointer to PPP_IF_VAR_T */
    void    (*c_func)();    /* routine (changed to void (*)() */
    struct  ppp_callout *c_next;
};


/*
 * Inline versions of get/put char/short/long.
 * Pointer is advanced; we assume that both arguments
 * are lvalues and will already be in registers.
 * cp MUST be u_char *.
 */
#define GETCHAR(c, cp) { \
	(c) = *(cp)++; \
}
#define PUTCHAR(c, cp) { \
	*(cp)++ = (u_char) (c); \
}


#define GETSHORT(s, cp) { \
	(s) = *(cp)++ << 8; \
	(s) |= *(cp)++; \
}
#define PUTSHORT(s, cp) { \
	*(cp)++ = (u_char) ((s) >> 8); \
	*(cp)++ = (u_char) (s); \
}

#define GETLONG(l, cp) { \
	(l) = *(cp)++ << 8; \
	(l) |= *(cp)++; (l) <<= 8; \
	(l) |= *(cp)++; (l) <<= 8; \
	(l) |= *(cp)++; \
}
#define PUTLONG(l, cp) { \
	*(cp)++ = (u_char) ((l) >> 24); \
	*(cp)++ = (u_char) ((l) >> 16); \
	*(cp)++ = (u_char) ((l) >> 8); \
	*(cp)++ = (u_char) ((l)); \
}

#define INCPTR(n, cp)	((cp) += (n))
#define DECPTR(n, cp)	((cp) -= (n))

/*
 * System dependent definitions for user-level 4.3BSD UNIX implementation.
 */

#define DEMUXPROTREJ(u, p)      demuxprotrej(u, p)

#define PPP_TIMEOUT(r, f, t, u) ppp_timeout((r), (f), (t), (u))
#define PPP_UNTIMEOUT(r, f, u)	ppp_untimeout((r), (f), (u))

#define BCOPY(s, d, l)          bcopy((char *) s, (char *) d, (int) l)
#define BZERO(s, n)             bzero(s, n)
#define EXIT(u)                 die(u, 0)


/*
 * MAKEHEADER - Add Header fields to a packet.
 */
#define MAKE_HDLC_HEADER(p) do{ \
    PUTCHAR(ALLSTATIONS, p); \
    PUTCHAR(UI, p); \
} while(0)

#define MAKE_PPPHEADER(p, t) 		PUTSHORT(t, p)


#ifndef MIN
#define MIN(a, b)       ((a) < (b)? (a): (b))
#endif
#ifndef MAX
#define MAX(a, b)       ((a) > (b)? (a): (b))
#endif


//***************************
// function declarations
//***************************
#if 0
extern void ppp_daemon      (PPP_IF_VAR_T *pPppIf);
#else
extern void ppp_daemon(cyg_addrword_t argv);
#endif

extern void	die             (PPP_IF_VAR_T *, int status);
extern void	ppp_timeout     (void (*func) (), caddr_t arg, int seconds, PPP_IF_VAR_T *pPppIf);
extern void	ppp_untimeout   (void (*func) (), caddr_t arg, PPP_IF_VAR_T *pPppIf);
extern void	adjtimeout      (PPP_IF_VAR_T *pPppIf);
extern void adjidle			(PPP_IF_VAR_T *pPppIf);
extern void	demuxprotrej    (PPP_IF_VAR_T *pPppIf, u_short protocol);
extern void	novm            (char *msg);
extern void pppSendTimerEvent(PPP_IF_VAR_T *pPppIf);
extern int  pppwrite (PPP_IF_VAR_T *pPppIf, char *buf, int count);
extern int  ppppktin(PPP_IF_VAR_T *pPppIf, struct mbuf *m);
extern int pppoutput(struct ifnet *ifp, struct mbuf *m0, 
                     struct sockaddr *dst, struct rtentry *rt);
extern int   pppioctl(struct ifnet *ifp, u_long cmd, char *data);
extern void ppp_ifstart(struct ifnet *ifp);

extern void pppSendHungupEvent(PPP_IF_VAR_T *pPppIf);
extern void pppSendInterruptEvent(PPP_IF_VAR_T *pPppIf);
extern void pppSendTerminateEvent(PPP_IF_VAR_T *pPppIf);
extern void pppSendTimerEvent(PPP_IF_VAR_T *pPppIf);
extern void pppSendInputEvent(PPP_IF_VAR_T *pPppIf);
extern void pppSendAdjIdleEvent(PPP_IF_VAR_T *pPppIf);
  
#ifdef __cplusplus
}
#endif

#define PRINTMSG(m, l)  { m[l] = '\0'; diag_printf("Remote message: %s\n", m); }

#ifdef DEBUGMAIN
#define MAINDEBUG(fmt, ...)		diag_printf("pppm: " fmt "\n", ##  __VA_ARGS__)
#else
#define MAINDEBUG(fmt, ...)
#endif

#ifdef DEBUGFSM
#define FSMDEBUG(fmt, ...)		diag_printf("pppfsm: " fmt "\n", ##  __VA_ARGS__)
#else
#define FSMDEBUG(fmt, ...)
#endif

#ifdef DEBUGLCP
#define LCPDEBUG(fmt, ...)		diag_printf("lcp: " fmt , ##  __VA_ARGS__)
#else
#define LCPDEBUG(fmt, ...)
#endif

#ifdef DEBUGIPCP
#define IPCPDEBUG(fmt, ...)		diag_printf("ipcp: " fmt, ##  __VA_ARGS__)
#else
#define IPCPDEBUG(fmt, ...)
#endif

#ifdef DEBUGUPAP
#define UPAPDEBUG(fmt, ...)		diag_printf("pap: " fmt "\n", ##  __VA_ARGS__)
#else
#define UPAPDEBUG(fmt, ...)
#endif

#ifdef DEBUGCHAP
#define CHAPDEBUG(fmt,...)	diag_printf("chap: " fmt , ##  __VA_ARGS__)
#else
#define CHAPDEBUG(fmt,...)
#endif

#ifdef DEBUGAUTH
#define AUTHDEBUG(fmt, ...)		diag_printf("pppauth: " fmt "\n", ##  __VA_ARGS__)
#else
#define AUTHDEBUG(fmt, ...)
#endif

#endif	/* __PPPD_H__ */



