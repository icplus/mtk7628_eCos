#ifdef __cplusplus
extern "C" {
#endif

#ifndef  PPP_API_H
#define  PPP_API_H

#include <sys_inc.h>
#include <pppd.h>

#define PPP_DEBUG_PRINT 1

#if PPP_DEBUG_PRINT
#define PPP_PRINTF diag_printf
#else
#define PPP_PRINTF
#endif

#define PPP_NUM_OF_TTY_UNIT 1
#define PPP_TTY_UNIT        0
#define PPP_PPPOE_UNIT      1

/*******************************/
/* PPP event flag defination   */
/*******************************/
#define PPP_EVNET_HUNGUP        0x01   /* Hungup */
#define PPP_EVENT_INTERRUPT     0x02   /* INTERRUPT */
#define PPP_EVENT_TERMINATE     0x04   /* Terminate */
#define PPP_EVENT_TIMEOUT       0x08   /* Timeout */
#define PPP_EVENT_INPUT         0x10   /* Input available */
#define PPP_EVENT_ADJ_IDLE      0x20   /* Adjust idle time-out */

#define PPP_EVENT_ALL           (PPP_EVNET_HUNGUP | PPP_EVENT_INTERRUPT | PPP_EVENT_TERMINATE | PPP_EVENT_TIMEOUT | PPP_EVENT_INPUT | PPP_EVENT_ADJ_IDLE)


/**********************************/
/* data sturcture definition      */
/**********************************/
enum PPP_ENCAPSULATE_TYPE_E
{
    PPP_ENCAP_TTY, 
    PPP_ENCAP_ETHERNET,
    PPP_ENCAP_AAL5,
};


/* PPP parameter structure */
//typedef struct ppp_params
//{
//    char    ifname[255+1];     // device name
//    u_long  type;                           // encapsulate type
//    char    username[256];                  // user name of PAP/CHAP
//    char    password[256];                  // password of PAP/CHAP
//    int     fd;                             // file descritor, only for tty
//    int     baud;                           // baud rate of <tty>; NULL = default 
//    u_long  idle;                           // idle time
//    u_long  mtu;                            // Maximun transmit unit 
//    u_long  mru;                            // Maximun receive unit
//	int     flags;                          // Flag options
//    int     defaultroute;                   // default route flag
//} PPP_PARAMS_T;


/*------------------------------------------------------*/
/*  defination                                          */
/*------------------------------------------------------*/  
int         PppEth_Start(PPP_IF_VAR_T  *pPppIf);
//void        ppp_Stop        (PPP_IF_VAR_T *pPppIf);
int         ppp_IfAdd(PPP_IF_VAR_T *pPppIf);
int         ppp_IfDelete(PPP_IF_VAR_T *pPppIf);
//PPP_IF_VAR_T *ppp_IfFind    (u_char *pIfname);
//int         ppp_GetIp       (char *pIfname, u_long *pLocal, u_long *pPeer, u_long *pMask);
//int         ppp_IdleTimerChange(int idle, char *pIfName);
//int         ppp_IdleState   (int unit);
//void        ppp_StartIdleTimeOut(PPP_IF_VAR_T *pPppIf);
//int         ppp_ParseIfname(char *ifname, char *pDevname, short *pUnit);
//
//extern int  ppp_Ether2Ppp (struct mbuf * mBuf, PPP_IF_VAR_T *pPppIf);

#endif /* PPP_API_H */
#ifdef __cplusplus
}
#endif


