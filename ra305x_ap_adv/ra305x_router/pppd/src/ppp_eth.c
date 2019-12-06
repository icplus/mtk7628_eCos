#include <sys_inc.h>
#include <string.h>
#include "pppd.h"
#include <syslog.h>
#ifdef MPPE
#include "mppe.h"
#endif
#include "ppp_comp.h"
#include <cfg_net.h>
#include <cfg_def.h>

#define ppp_task_priority    6
/**********************************/
/* Data sturcture definition      */
/**********************************/
static int  PppEth_Attach(PPP_IF_VAR_T *pPppIf);
static int  PppEth_Ioctl(PPP_IF_VAR_T *pPppIf, u_long cmd, char *data);
static int  PppEth_Close(int unit);

/* ppp thread stack */
#define STACKSIZE	1024*5 //4096*10/5 //chang ppp STACKSIZE 8192->5*1024
static char taskname[CONFIG_PPP_NUM_SESSIONS][20];
static char statck[CONFIG_PPP_NUM_SESSIONS][STACKSIZE];
static cyg_thread thread_obj[CONFIG_PPP_NUM_SESSIONS];

//yfchou add for compress
struct ppp_softc Ppp_softc[CONFIG_PPP_NUM_SESSIONS];

/*--------------------------------------------------------------
* ROUTINE NAME - PppEth_Start
*---------------------------------------------------------------
* FUNCTION: Open   
*
* INPUT:    None 
---------------------------------------------------------------*/
int  PppEth_Start(PPP_IF_VAR_T  *pPppIf)
{
    //u_long argv[4] = {0};
    //u_long state;
	//char taskname[50];
	int i;
	
	extern	int	lcp_echo_fails_max;
	extern	int lcp_echo_interval;
	
	extern	int lcp_passive_echo_reply;
	extern	int lcp_serv_echo_fails_max;
	extern	int lcp_serv_echo_interval;
	
	
    if (pPppIf == NULL)
    {
        return ERROR;
    }
	
    if (ppp_IfAdd(pPppIf) == ERROR)
	{
		diag_printf("\n\rThis PPP interface have started already!");
		return ERROR;
	}
	
	i = pPppIf->unit;
	
    /* Initialize ppp interface variables */
    pPppIf->peer_mru        = pPppIf->mru;
    pPppIf->debug           = 1;
    pPppIf->unknownProto    = 0;
    pPppIf->hungup          = 0;

    strcpy(pPppIf->our_name, pPppIf->user);
    pPppIf->our_name[MAXNAMELEN-1] = 0;


	//pPppIf->lcp_echo_interval   = 30;
	//pPppIf->lcp_echo_fails      = 5;
    //pPppIf->lcp_echo_fails      = 10;
	pPppIf->lcp_echo_interval		= lcp_echo_interval;
	pPppIf->lcp_echo_fails			= lcp_echo_fails_max;
	
	pPppIf->lcp_passive_echo_reply	= lcp_passive_echo_reply;
	pPppIf->lcp_serv_echo_interval	= lcp_serv_echo_interval;
	pPppIf->lcp_serv_echo_fails		= lcp_serv_echo_fails_max;
	
//    pPppIf->flags = OPT_NO_VJ|OPT_NO_ACC|OPT_NO_PC | 
    pPppIf->flags = OPT_NO_VJ|OPT_NO_PC | 
                    OPT_IPCP_ACCEPT_LOCAL | OPT_IPCP_ACCEPT_REMOTE;

    pPppIf->pIoctl = PppEth_Ioctl;
    pPppIf->pAttach = PppEth_Attach;
    
    pPppIf->_hostname[MAXNAMELEN-1] = 0;

    pPppIf->inq.ifq_maxlen = IFQ_MAXLEN; 

#ifdef MPPE
	{ 
		int ipmode = 0;
		CFG_get(CFG_WAN_IP_MODE, &ipmode);
		#ifdef	CONFIG_PPTP_PPPOE
		int pptp_wanif = 0;
		//Added by haitao,for pppoe-->pptp vpn case.
		CFG_get(CFG_PTP_WANIF, &pptp_wanif);
		if((ipmode == PPTPMODE) && (pptp_wanif == 2) && (!strcmp(pPppIf->pppname,"ppp0")))ipmode=PPPOEMODE;
		#endif
		if (ipmode == PPPOEMODE)
			pPppIf->mppe = 0;//1;
	}
#endif    
    
    pPppIf->pSc = &Ppp_softc[i];
    pPppIf->pSc->sc_xc_state = NULL;
    pPppIf->pSc->sc_rc_state = NULL;
    pPppIf->pSc->sc_flags = 0;
    
	sprintf(taskname[i], "PPP%d_daemon", i);
	
    cyg_thread_create(ppp_task_priority, ppp_daemon , (cyg_addrword_t)pPppIf, taskname[i],
                     &statck[i], STACKSIZE, &pPppIf->task_id, &thread_obj[i]);
    cyg_thread_resume(pPppIf->task_id);
	
	return(OK);    
}

#if 0
/*--------------------------------------------------------------
* ROUTINE NAME - ppp_Ether2Ppp         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int ppp_Ether2Ppp(struct mbuf *m, PPP_IF_VAR_T *pPppIf)
{
    char *ptr;
	
    /* carve space for HDLC-link frame (address field: 0xff and Control field : 0x03 */
    if (m == NULL)
    {
      diag_printf("ppp_Ether2Ppp:NULL packet...\n");
	  return OK;
    }
	
	M_PREPEND(m, 2, M_DONTWAIT);
	if (m == NULL)
		return OK;
	
	/* set version and type */
	ptr = mtod(m, char *);
	*ptr = PPP_ALLSTATIONS;
	*(ptr+1) = PPP_UI;
	
	ppppktin(pPppIf, m);
	
	return OK;
}
#endif

/*--------------------------------------------------------------
* ROUTINE NAME - PppEth_Attach
*---------------------------------------------------------------
* FUNCTION:  Attachs PPP interface to ifnet list  
*
* INPUT:    None 
---------------------------------------------------------------*/
static int PppEth_Attach(PPP_IF_VAR_T *pPppIf)
{
    struct ifnet *pIf = &pPppIf->ifnet;
    unsigned len, m;
    char *pDevName;
    const char *cp;
    int unit;
    char c;
    int s;

    len = strlen(pPppIf->pppname);
    if (len < 2 || len > IFNAMSIZ)
        return NULL;
    cp = pPppIf->pppname + len - 1;
    c = *cp;
    if (c < '0' || c > '9')
        return NULL;		/* trailing garbage */
    unit = 0;
    m = 1;
    do {
        if (cp == pPppIf->pppname)
            return NULL;	/* no interface name */
        unit += (c - '0') * m;
        if (unit > 1000000)
            return NULL;	/* number is unreasonable */
        m *= 10;
        c = *--cp;
    } while (c >= '0' && c <= '9');
    len = cp - pPppIf->pppname + 1;
    pDevName = (char *)malloc(IFNAMSIZ+1);
	if(!pDevName) 
		{
		panic("PppEth_Attach:pDevName=NULL\n");
		return ERROR;//just for klocwork. actual ,you cann't return
		}
    bcopy(pPppIf->pppname, pDevName, len);
    pDevName[len] = '\0';
    pIf->if_name = pDevName;
    pIf->if_unit	= unit;
    strcpy(pIf->if_xname, pPppIf->pppname);

    pIf->if_mtu = pPppIf->mtu;
    pIf->if_flags = IFF_POINTOPOINT | IFF_MULTICAST;
	
    pIf->if_type = IFT_PPP;
    pIf->if_hdrlen = PPP_HDRLEN;
    pIf->if_ioctl = pppioctl;
    pIf->if_output = pppoutput;
    pIf->if_reset = PppEth_Close;
    pIf->if_start = ppp_ifstart;
    pPppIf->inq.ifq_maxlen = IFQ_MAXLEN;
	
    s = splimp();
    if_attach(pIf);
    splx(s);
	
    return (OK);
}

/*--------------------------------------------------------------
* ROUTINE NAME - PppEth_Close
*---------------------------------------------------------------
* FUNCTION: Close a ppp interface that is over ethernet. 
*
* INPUT:    
---------------------------------------------------------------*/
void *ppp_ifnet=0;
#ifdef	CONFIG_PPTP_PPPOE
void *ppp_ifnet_pptp_vpn=0;
#endif
#ifdef CONFIG_L2TP_OVER_PPPOE
void *l2tp_ppp_ifnet = 0;
#endif
int PppEth_Close(int unit)
{
    PPP_IF_VAR_T *pPppIf = (PPP_IF_VAR_T *)unit;
    struct mbuf *m;
    int   s;
    
	diag_printf("%s PppEth_Close called\n", pPppIf->pppname);
	
    s = splimp();

    if_down(&pPppIf->ifnet);

    for (;;) 
    {
        IF_DEQUEUE(&pPppIf->inq, m);
        if (m == NULL)
            break;
        m_freem(m);
    }

    pPppIf->ifnet.if_flags &= ~(IFF_UP|IFF_RUNNING);

    splx(s);

	#ifdef	CONFIG_PPTP_PPPOE
	int type, pptp_wanif;
	CFG_get(CFG_WAN_IP_MODE, &type);
	CFG_get(CFG_PTP_WANIF, &pptp_wanif);
	if((type == PPTPMODE) && (pptp_wanif == 2)){
	  if(!strcmp(pPppIf->pppname,"ppp1"))//note rule:ppp1 fixed for pptp vpn use. the wan mode:pppoe/pptp/l2tp use fixed ppp0. 
	    ppp_ifnet_pptp_vpn= &pPppIf->ifnet;
	  else
	    ppp_ifnet = &pPppIf->ifnet;
	}else
	  ppp_ifnet = &pPppIf->ifnet;
	#endif
	
	#ifdef CONFIG_L2TP_OVER_PPPOE
	int mode, l2tp_wanif;
	CFG_get(CFG_WAN_IP_MODE, &mode);
	CFG_get(CFG_L2T_WANIF, &l2tp_wanif);
	if((mode == L2TPMODE) && (l2tp_wanif == 2)){
	  if(!strcmp(pPppIf->pppname,"ppp1"))
	    l2tp_ppp_ifnet = &pPppIf->ifnet;
	  else
	  	ppp_ifnet = &pPppIf->ifnet;
	}else
	  ppp_ifnet = &pPppIf->ifnet;
	#endif

	#if !defined(CONFIG_PPTP_PPPOE) && !defined(CONFIG_L2TP_OVER_PPPOE)
      ppp_ifnet = &pPppIf->ifnet;
    #endif

    return 0;
}

/*--------------------------------------------------------------
* ROUTINE NAME - PppEth_Ioctl
*---------------------------------------------------------------
* FUNCTION: Close a ppp interface that is over ethernet. 
*
* INPUT:    
* RETURN:
---------------------------------------------------------------*/
static int PppEth_Ioctl(PPP_IF_VAR_T *pPppIf, u_long cmd, char *data)
{
    switch (cmd)
    {
    case SIOCSIFMTU:
	    pPppIf->ifnet.if_mtu = *(int *)data;
	break;

    case SIOCGIFMTU:
        *(int *)data = pPppIf->ifnet.if_mtu;
        break;
    case PPPIOCGFLAGS:
    	*(int *)data = pPppIf->pSc->sc_flags;
    	break;
    case PPPIOCSFLAGS:
    	pPppIf->pSc->sc_flags = *(int *)data;
//    	diag_printf("set ppp flags=%08x\n", *(int *)data);
        break;
    }
    return OK;
}


