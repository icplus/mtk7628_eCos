/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************/

#include <l2tp.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <network.h>
#include <sys/time.h>
#include <l2tp_ppp.h>
#include <cfg_net.h>
#include <cfg_def.h>
#include <sys_status.h>
#include <eventlog.h>

//#define L2TP_TASK_PRIORITY    8
#define L2TP_TASK_PRIORITY    6
#define STACKSIZE	1024*6 //4096*2

struct l2tp_if  *L2TP_IfList = NULL;
struct l2tp_cfg *l2tp_IfList = 0;
//static char l2tp_stack[STACKSIZE];
static char *l2tp_stack=NULL;
static cyg_thread l2tp_thread_obj;
static cyg_handle_t l2tp_handle;

//static char l2tp_plus_stack[STACKSIZE];
static char *l2tp_plus_stack=NULL;
static cyg_thread l2tp_plus_thread_obj;
static cyg_handle_t l2tp_plus_handle;

cyg_flag_t L2tpEvents;
int L2TP_SEND_QHEAD;
int L2TP_SEND_QTAIL;
struct l2tp_mbox_struc L2TP_PACKET_QUEUE[L2TP_SEND_QUEUE_SIZE];
static int l2tp_DaemonStart_flag = 0;
static cyg_sem_t l2tp_Semaphore;

extern int encaps_l2tp(struct l2tp_if *ifp,struct mbuf *m0);
l2tp_peer l2tp_peercfg;


void	l2tpc(struct l2tp_if *pInfo);
void	l2tpc_send_process(void);
static int close_l2tp(struct l2tp_if *pInfo);
static int l2tp_reconnect_pppd(struct l2tp_if *pInfo);
static void l2tp_StartConnect(L2tpSelector *es);
int l2tp_HandleEvent(L2tpSelector *es, struct l2tp_if *pInfo);
int l2tp_HandleEvent_BoostMode(L2tpSelector *es, struct l2tp_if *pInfo, L2tpHandler *readable_eh, int fd, fd_set readfds);//Jody work
static int l2tp_stop(struct l2tp_cfg *pParam);

struct l2tp_cfg L2tp_config_param;
static int manual_flag = 0;
static int force_conn = 0;
static struct l2tp_if  L2TP_InfoListArray[1];
static int  L2TP_InfoListIndex = 0; 
static  struct l2tp_cfg l2tp_IfListArray[1];
static int  L2TP_IfListIndex; 
static int user_stop;
int l2tp_send_process_stop;
static cyg_mbox l2tp_tsk_mbox;

extern int (*dialup_checkp) __P((void));
int l2tp_dialup_checkp(void);
extern int gettimeofday (struct timeval *, struct timezone *tz);
#ifdef CONFIG_L2TP_OVER_PPPOE
typedef struct ppp_ctx_s {
	struct ip_set *pipset;
	cyg_uint16 ipset_flags;
	cyg_uint8 state;
	cyg_uint8 staticip;
	cyg_uint32 conn_mode;
	cyg_uint32 prev_dns[2];
} ppp_ctx_t;
static ppp_ctx_t l2tp_ppp_ctl= {pipset: NULL, state: PPP_NO_ETH};

extern struct ip_set wan_ip_set;
struct ip_set l2tp_wan_ip_set[CONFIG_PPP_NUM_SESSIONS];
extern int need_dialup_flag;

int l2tp_over_pppoe_mon(void);
int l2tp_over_pppoe_mon_down(void);

#endif

/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_IfNameToInfoList         
*---------------------------------------------------------------
* FUNCTION: This functions searchs L2TP interface information 
*
* INPUT:    pppname - Which L2TP interface to search 
* OUTPUT:   None 
* RETURN:   L2TP interface pointer                                               
* NOTE:     
*---------------------------------------------------------------*/
struct l2tp_if *l2tp_IfNameToInfoList(char *pppname)
{
    struct l2tp_if *pInfo;
    
    if (L2TP_IfList == NULL)
    {
        return NULL;
    }
    else
    {
        for(pInfo = L2TP_IfList; pInfo != NULL; pInfo = pInfo->next)
        {
            if (strcmp(pppname, pInfo->t_pppname) == 0)
            {
                return pInfo;
            }
        }
    }
    return NULL;
}

/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_InfoListAdd         
*---------------------------------------------------------------
* FUNCTION: Adds an interface to L2TP_IfList.
*
* INPUT:    pNew    -  which L2TP interfacene to add
* OUTPUT:   None 
* RETURN:   OK or ERROR                                               
* NOTE:     
*---------------------------------------------------------------*/
static int l2tp_InfoListAdd(struct l2tp_if *pNew)
{
    struct l2tp_if **ppInfo = &L2TP_IfList;

    pNew->next = NULL;
    cyg_scheduler_lock();

    while(*ppInfo)
    {
        if (strcmp(pNew->t_pppname, (*ppInfo)->t_pppname) == 0)
        {
            diag_printf("\n\rl2tp_InfoListAdd: Re-Add L2TP interface %s", 
                   pNew->t_pppname);

            cyg_scheduler_unlock();
            return ERROR;
        } 

        ppInfo = &((*ppInfo)->next);
    }

    *ppInfo = pNew;

    cyg_scheduler_unlock();

    return OK;
}

/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_InfoListDel         
*---------------------------------------------------------------
* FUNCTION: Deletes a L2TP interface from L2TP_IfList
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static int l2tp_InfoListDel(struct l2tp_if *pDel)
{
    struct l2tp_if **ppInfo = &L2TP_IfList;

    cyg_scheduler_lock();

    while (*ppInfo && *ppInfo != pDel )
    {
        ppInfo = &((*ppInfo)->next);
    }
    
    if (*ppInfo == L2TP_IfList)
        L2TP_IfList = NULL;
    else if (*ppInfo)
        *ppInfo = (*ppInfo)->next;

    cyg_scheduler_unlock();

    return OK;
}

/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_DaemonStart         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int l2tp_DaemonStart(struct l2tp_cfg *pParam)
{
    L2tpSelector *es;
    int debugmask = 1;
    struct l2tp_if *pInfo;
    int sever_fqdn;
    char sever_name[256];
	
	if (l2tp_IfNameToInfoList (pParam->pppname) != NULL)
    {
        diag_printf("L2TP interface has been started.\n");
        return OK;
    }
    user_stop = 0;
    l2tp_send_process_stop = 0;
    L2TP_SEND_QHEAD = 0;
    L2TP_SEND_QTAIL = 0;
	L2TP_InfoListIndex = 0;
	pInfo = &L2TP_InfoListArray[L2TP_InfoListIndex++];
	memset(pInfo, 0, sizeof(struct l2tp_if)); 
    es = &pInfo->es;
	es->pInfo = (void *)pInfo;
	
    l2tp_random_init();
	
	memset(&l2tp_peercfg, 0, sizeof(l2tp_peercfg));
	l2tp_peercfg.addr.sin_port = htons(1701);
	l2tp_peercfg.addr.sin_family = AF_INET;
	l2tp_peercfg.hide_avps = 0;
	l2tp_peercfg.retain_tunnel = 0;
	l2tp_peercfg.validate_peer_ip = 1;

    ld_SetBitMask(debugmask);
    
	
	CFG_get( CFG_L2T_SVRD_SEL, &sever_fqdn );
	if(sever_fqdn)
	{
	    struct hostent *he;
	    
		CFG_get( CFG_L2T_SVRDM, sever_name );
		he = gethostbyname(sever_name);
		if (!he) 
		{
			diag_printf("Could not resolve %s as IP address: error\n",sever_name);
	    	return -1;
		}
		memcpy(&l2tp_peercfg.addr.sin_addr, he->h_addr_list[0], he->h_length);
	}
	else
	{
		struct in_addr in;
		CFG_get_str(CFG_L2T_SVR, sever_name);
		inet_aton(sever_name, &in);
		l2tp_peercfg.addr.sin_addr.s_addr = in.s_addr;
	}	
	
	//init PPP parameters
    strcpy(pInfo->t_ifname, pParam->tunnel_ifname);
    strcpy(pInfo->t_pppname, pParam->pppname);
    strcpy(pInfo->t_user, pParam->username); 
    strcpy(pInfo->t_password, pParam->password);

    pInfo->t_idle  = pParam->idle_time;;
    pInfo->t_mtu   = pParam->mtu;
    pInfo->t_mru   = pInfo->t_mtu;

	
    pInfo->if_ppp.defaultroute = TRUE;
    pInfo->if_ppp.pOutput = encaps_l2tp;
    pInfo->if_ppp.pIfStopHook = close_l2tp;
    pInfo->if_ppp.pIfReconnectHook = l2tp_reconnect_pppd;
	#ifdef CONFIG_L2TP_OVER_PPPOE
	int l2tp_wanif;
	CFG_get( CFG_L2T_WANIF, &l2tp_wanif);
	if(l2tp_wanif == 2)
	  pInfo->if_ppp.unit = 1;
	#endif
	
	if(l2tp_NetworkInit(es)<0)
	{
		mon_snd_cmd(MON_CMD_WAN_UPDATE);
		return -1;
	}	
	
	// Adds to list
    l2tp_InfoListAdd(pInfo);

    cyg_flag_init(&L2tpEvents);
	cyg_mbox_create( &pInfo->msgq_id, &l2tp_tsk_mbox );
	cyg_semaphore_init(&l2tp_Semaphore, 0);

	if(l2tp_stack)
		free(l2tp_stack);
	l2tp_stack =(char *)malloc(STACKSIZE);
	cyg_thread_create(L2TP_TASK_PRIORITY, l2tpc , (cyg_addrword_t)pInfo, "L2TP Clent",
                     (void *)l2tp_stack, STACKSIZE, &l2tp_handle, &l2tp_thread_obj);
   
	if(l2tp_plus_stack)
		free(l2tp_plus_stack);
	l2tp_plus_stack =(char *)malloc(STACKSIZE);
    cyg_thread_create(L2TP_TASK_PRIORITY, l2tpc_send_process , (cyg_addrword_t)0, "L2TP_SEND PROCESS",
                     (void *)l2tp_plus_stack, STACKSIZE, &l2tp_plus_handle, &l2tp_plus_thread_obj); 
    cyg_thread_resume(l2tp_handle); 
    cyg_thread_resume(l2tp_plus_handle); 
    l2tp_DaemonStart_flag = 1;
	
    return OK;
}

/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_InfoListDel         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void l2tpc(struct l2tp_if *pInfo)
{
	int i = -1;
	L2tpSelector *es;
	L2tpHandler *eh;
	struct timeval now;
	l2tp_tunnel *tunnel;
	
	SysLog(LOG_USER|LOG_INFO|LOGM_L2TP,"l2tpc start up");
	es = &pInfo->es;
	while(1)
	{
		l2tp_StartConnect(es);
		while(1)
		{
			cyg_thread_delay(1);
			if(pInfo->ses->tunnel == NULL)
			{
				mon_snd_cmd(MON_CMD_WAN_UPDATE);
			}
			else
			{
				es->nestLevel++;
				for (eh=es->handlers; eh; eh=eh->next) 
				{
					if (eh->flags & EVENT_FLAG_DELETED) continue;
					if (eh->flags & EVENT_TIMER_BITS)
					{
	    				gettimeofday(&now, NULL);
			    		if (eh->tmout.tv_sec < now.tv_sec)
			    		{	
			    			eh->flags |= EVENT_FLAG_DELETED;
			    			es->opsPending = 1;
			    			if(pInfo->ses->tunnel != NULL)
								lt_HandleTimeout(NULL, 0, 0, (l2tp_tunnel *)pInfo->ses->tunnel);
			    		}	
					}
					if (eh->flags & EVENT_FLAG_READABLE) 
					{
						l2tp_TunnelCtl(es,eh->fd);
					}	
				}
				es->nestLevel--;
				if (!es->nestLevel && es->opsPending) 
				{
					l2tp_PendingChanges(es);
   				}
			
			}		
			tunnel = (l2tp_tunnel *)pInfo->ses->tunnel;
			if (tunnel != NULL && tunnel->state == TUNNEL_ESTABLISHED)
			{
				break;
			}	
			l2tp_ScanCmd();
			//if(pInfo->ses->tunnel !=NULL)
			//	pInfo->ses->tunnel = NULL;
		}	
		
        #ifdef CONFIG_L2TP_BOOST_MODE
		  int maxfd = -1;
	      fd_set readfds;
	      L2tpHandler *network_readableEh = NULL;
		  
		  FD_ZERO(&readfds);
		  for (eh=es->handlers; eh; eh=eh->next) 
		  {	
		    if (eh->flags & EVENT_FLAG_READABLE) 
		    {
		      network_readableEh = eh;
			  FD_SET(eh->fd, &readfds);
			  if (eh->fd > maxfd) 
			    maxfd = eh->fd;
			  break;
		    }
			
		  }
		#endif
		
		while(1) 
		{ 
		  #ifdef CONFIG_L2TP_BOOST_MODE
		  if(network_readableEh == NULL) return;
		  i = l2tp_HandleEvent_BoostMode(es, pInfo, network_readableEh, maxfd, readfds);			
		  if (i < 0) 
    	    return;
		  if(i==2)
   			break;
		  #else
		  i = l2tp_HandleEvent(es,pInfo);
		  if (i < 0) 
    		return;
   		  if(i==2)
   		    break;	
		  #endif	
		}
	}	
}	

/*--------------------------------------------------------------
* ROUTINE NAME - l2tpc_send_process         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void l2tpc_send_process(void)
{
	cyg_flag_value_t events;

	while(1)
	{
	    events =
	    cyg_flag_wait(&L2tpEvents,
                     L2TP_EVENT_RX|L2TP_EVENT_QUIT,
                     CYG_FLAG_WAITMODE_OR | CYG_FLAG_WAITMODE_CLR);
	    if(events & L2TP_EVENT_RX)
	    {
			//SEM_LOCK(L2tp_Queue_Lock);
			while(L2TP_SEND_QHEAD!=L2TP_SEND_QTAIL)
			{	
              #ifdef CONFIG_L2TP_BOOST_MODE
				encaps_l2tp_boostmode(L2TP_PACKET_QUEUE[L2TP_SEND_QHEAD].ifp,
		    	 				L2TP_PACKET_QUEUE[L2TP_SEND_QHEAD].m0);
			  #else
				encaps_l2tp_usermode(L2TP_PACKET_QUEUE[L2TP_SEND_QHEAD].ifp,
		    	 				L2TP_PACKET_QUEUE[L2TP_SEND_QHEAD].m0);
			  #endif
		    	 				
				L2TP_SEND_QHEAD++;
				if(L2TP_SEND_QHEAD>=L2TP_SEND_QUEUE_SIZE)
					L2TP_SEND_QHEAD = 0;
					
			}
			//SEM_UNLOCK(L2tp_Queue_Lock);
		}	
	 	if(events & L2TP_EVENT_QUIT)
	 		break;
	} 	
	while(L2TP_SEND_QHEAD!=L2TP_SEND_QTAIL)
	{
		m_freem(L2TP_PACKET_QUEUE[L2TP_SEND_QHEAD].m0);
		L2TP_SEND_QHEAD++;
		if(L2TP_SEND_QHEAD>=L2TP_SEND_QUEUE_SIZE)
			L2TP_SEND_QHEAD = 0;
	}	
	//cyg_semaphore_destroy(&L2tp_Queue_Lock);

	cyg_semaphore_post(&l2tp_Semaphore);
	cyg_thread_exit();
}		

/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_StartConnect         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static void l2tp_StartConnect(L2tpSelector *es)
{
    l2tp_session *sess;

    sess = ls_CallINS(&l2tp_peercfg, "foobar", es, NULL);
    if (!sess) 
	{
		diag_printf("L2TP:%s",l2tp_get_errmsg());
		return;
    }
}

/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_IfListAdd		                            
*---------------------------------------------------------------
* FUNCTION:  
* INPUT:										
* OUTPUT:
* RETURN:	
*--------------------------------------------------------------*/
static unsigned int l2tp_IfListAdd(struct l2tp_cfg *pParam)
{
    struct l2tp_cfg *pTemp, *pPrev, *pNew;

    // Add to Monitor list
    pPrev = 0;
    pTemp = l2tp_IfList;
    while (pTemp)
    {
        if (strcmp(pTemp->pppname, pParam->pppname) == 0)
            return 0;   // Already hooked

        pPrev = pTemp;
        pTemp = pTemp->next;
    }
	pNew = &l2tp_IfListArray[L2TP_IfListIndex++];
	memset(pNew,0,sizeof(struct l2tp_cfg));

    memcpy(pNew, pParam, sizeof(struct l2tp_cfg));
    pNew->next = 0;

    // Append to the list
    if (pPrev == 0)
        l2tp_IfList = pNew;
    else
        pPrev->next = pNew;
    
    return 0;
}


#if 0
/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_IfListDel		                            
*---------------------------------------------------------------
* FUNCTION:  
* INPUT:										
* OUTPUT:
* RETURN:	
*--------------------------------------------------------------*/
static unsigned int l2tp_IfListDel(char *pppname)
{
    struct l2tp_cfg *pTemp, *pPrev;

    // Add to Monitor list
    pPrev = 0;
    pTemp = l2tp_IfList;
    while (pTemp)
    {
        if (strcmp(pTemp->pppname, pppname) == 0)
            break;

        pPrev = pTemp;
        pTemp = pTemp->next;
    }    

    // delete from the list if found
    if (pTemp)
    {
        if (pPrev == 0)
            l2tp_IfList = pTemp->next;
        else
            pPrev->next = pTemp->next;
        free (pTemp);
    }
}
#endif

/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_InfoListDel         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int l2tp_init_param(struct l2tp_cfg *pParam)
{
	int mtu=1460;
	
	CFG_get( CFG_L2T_MTU, &mtu);
	pParam->mtu = mtu;
	CFG_get( CFG_L2T_IDLET, &pParam->idle_time);
	CFG_get(CFG_L2T_AUTO, &pParam->auto_reconnect);
	#ifdef CONFIG_L2TP_OVER_PPPOE
	  int l2tp_wanif;
	  CFG_get( CFG_L2T_WANIF, &l2tp_wanif);
	  if(l2tp_wanif == 2)
	    strcpy(pParam->pppname, "ppp1");
	  else
	  	strcpy(pParam->pppname, "ppp0");
	#else
	  strcpy(pParam->pppname, "ppp0");
	#endif
    strcpy(pParam->tunnel_ifname, WAN_NAME);
    CFG_get(CFG_L2T_USER,&pParam->username);
    CFG_get(CFG_L2T_PASS,&pParam->password);

    return OK;
}	

/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_InfoListDel         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int L2tp_doConnect(void)	
{
	struct l2tp_cfg *pParam;

	pParam = &L2tp_config_param;
	memset((char *)pParam,0,sizeof(struct l2tp_cfg));
	l2tp_init_param(pParam);
	if(manual_flag)
	    pParam->auto_reconnect = 2;
	l2tp_start(pParam);
	manual_flag = 0;
	force_conn = 0;
	
	return OK;
}

/*--------------------------------------------------------------
* ROUTINE NAME - L2tp_down         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void L2tp_down(void)
{
	struct l2tp_cfg *pParam;
	struct l2tp_if *pInfo;
	PPP_IF_VAR_T *pPppIf;
	void *data;

    diag_printf("L2tp_down...\n");
	dialup_checkp = NULL;
	pParam = &L2tp_config_param;
	#ifdef CONFIG_L2TP_OVER_PPPOE
	int l2tp_wanif;
	CFG_get(CFG_L2T_WANIF, &l2tp_wanif);
    if((pParam->auto_reconnect == 1) && (l2tp_wanif == 2)){
	  need_dialup_flag = 0;
    }
	#endif
	pInfo = l2tp_IfNameToInfoList (pParam->pppname);
	if(!pInfo)
	   return;
	data = pInfo->ses->tunnel;
	pPppIf =&pInfo->if_ppp;
	
	if(pPppIf->ifnet.if_reset && pInfo)
	{	
	    pPppIf->hungup = 1;
        die(pPppIf, 1);
    }
 	l2tp_stop(pParam);
	if(l2tp_DaemonStart_flag)
	{
	  cyg_semaphore_wait(&l2tp_Semaphore);
	}else
	  l2tp_send_process_stop++;

	if(l2tp_DaemonStart_flag)
	{
	  cyg_flag_destroy(&L2tpEvents);
	  cyg_semaphore_destroy(&l2tp_Semaphore);
	  if(l2tp_handle)
	  {
	    cyg_thread_delete(l2tp_handle); // some condition need force terminated
	    l2tp_handle = 0;
		if(l2tp_stack)
		{
		  free(l2tp_stack);
		  l2tp_stack =NULL;
		}
	  }   
			   
	  if(l2tp_plus_handle)
	  {
		cyg_thread_delete(l2tp_plus_handle); // some condition need force terminated
		l2tp_plus_handle = 0; 
		if(l2tp_plus_stack)
		{
		  free(l2tp_plus_stack);
		  l2tp_plus_stack =NULL;
		}
	  }
	}

    l2tp_DaemonStart_flag = 0;
}

/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_start         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int l2tp_start(struct l2tp_cfg *pParam)
{
	dialup_checkp = NULL;
	if (pParam->auto_reconnect == 0) //keep alive
    {
    	pParam->idle_time = 0;
    	if(!force_conn)
			l2tp_DaemonStart(pParam);
	}	
	else if(pParam->auto_reconnect == 1) //auto connection
    {
    	dialup_checkp = l2tp_dialup_checkp;
    }
    else if(pParam->auto_reconnect == 2) //connect by manual
    {
    	pParam->idle_time = 0;
    }
    else
    {
      diag_printf("l2tp: wrong connect mode\n");	
    }  
	#ifdef CONFIG_L2TP_OVER_PPPOE
	int mode, l2tp_wanif;
	CFG_get(CFG_WAN_IP_MODE, &mode);
	CFG_get(CFG_L2T_WANIF, &l2tp_wanif);
	if((mode == L2TPMODE) && (l2tp_wanif == 2) && (pParam->auto_reconnect != 0)){
		force_conn = 1;
	}
    if((pParam->auto_reconnect == 1) && (l2tp_wanif == 2))
	  need_dialup_flag ++;
    #endif
    if(force_conn)
    	l2tp_DaemonStart(pParam);
    L2TP_IfListIndex = 0;
	l2tp_IfListAdd(pParam);
	return 0;
	
}


/*--------------------------------------------------------------
* ROUTINE NAME - close_l2tp         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static int close_l2tp(struct l2tp_if *pInfo)
{
	void *data = pInfo->ses->tunnel;
	char *argv[1];
	
	SysLog(LOG_USER|LOG_INFO|LOGM_L2TP,"l2tpc clean up");
	lt_Cleanup(&pInfo->es);
	if(pInfo->ses->tunnel)
		lt_FreeAllHandler(data);	

	argv[0] = "down";
	l2tp_cmd(1, argv);
	
	return 0;
	
}	
/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_stop         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static int l2tp_stop(struct l2tp_cfg *pParam)
{
	
	int cmd;
	struct l2tp_if *pInfo;

	if ((pInfo = l2tp_IfNameToInfoList (pParam->pppname)) == NULL)
    {
        return ERROR;
    }
    if (pInfo->msgq_id == 0)
        return ERROR;
    
    cmd = L2TP_CMD_STOP;    
    
    cyg_mbox_tryput(pInfo->msgq_id, cmd);
    
    return 0;
}

/*--------------------------------------------------------------
* ROUTINE NAME - L2tp_HandleEvent         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int l2tp_HandleEvent(L2tpSelector *es, struct l2tp_if *pInfo)
{
    fd_set readfds, writefds;
    fd_set *rd, *wr;
    unsigned int flags;

    struct timeval abs_timeout, now={0,0};
    struct timeval timeout;
    struct timeval *tm;
    struct timeval tt={1,0};
    L2tpHandler *eh;

    int r = 0;
    int errno_save = 0;
    int foundTimeoutEvent = 0;
    int foundReadEvent = 0;
    int foundWriteEvent = 0;
    int maxfd = -1;
    int pastDue;
    int try_cnt=0;

     /* Build the select sets */
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    eh = es->handlers;
    for (eh=es->handlers; eh; eh=eh->next) 
	{
		if (eh->flags & EVENT_FLAG_DELETED) continue;
		if (eh->flags & EVENT_FLAG_READABLE) 
		{
			foundReadEvent = 1;
			FD_SET(eh->fd, &readfds);
			if (eh->fd > maxfd) maxfd = eh->fd;
		}
		if (eh->flags & EVENT_FLAG_WRITEABLE) 
		{
			foundWriteEvent = 1;
			FD_SET(eh->fd, &writefds);
			if (eh->fd > maxfd) maxfd = eh->fd;
		}
		if (eh->flags & EVENT_TIMER_BITS) 
		{
			if (!foundTimeoutEvent) 
			{
				abs_timeout = eh->tmout;
				foundTimeoutEvent = 1;
			} 
			else 
			{
				if (eh->tmout.tv_sec < abs_timeout.tv_sec ||
					(eh->tmout.tv_sec == abs_timeout.tv_sec &&
					 eh->tmout.tv_usec < abs_timeout.tv_usec)) 
				{
					abs_timeout = eh->tmout;
				}
			}
		}
    }
	
    if (foundReadEvent) 
	{
		rd = &readfds;
    } 
	else 
	{
		rd = NULL;
    }
	
    if (foundWriteEvent) 
	{
		wr = &writefds;
    } 
	else 
	{
		wr = NULL;
    }

    if (foundTimeoutEvent) 
	{
		gettimeofday(&now, NULL);
		/* Convert absolute timeout to relative timeout for select */
		timeout.tv_usec = abs_timeout.tv_usec - now.tv_usec;
		timeout.tv_sec = abs_timeout.tv_sec - now.tv_sec;
		if (timeout.tv_usec < 0) 
		{
			timeout.tv_usec += 1000000;
			timeout.tv_sec--;
		}
		if (timeout.tv_sec < 0 ||
			(timeout.tv_sec == 0 && timeout.tv_usec < 0)) 
		{
			timeout.tv_sec = 0;
			timeout.tv_usec = 0;
		}
		tm = &timeout;
    } 
	else 
	{
		tm = NULL;
    }

    if (foundReadEvent || foundWriteEvent || foundTimeoutEvent) 
	{
		if(tm)	
			try_cnt = tm->tv_sec;	
		for(;;) 
		{
			l2tp_ScanCmd();
			if(try_cnt<0)
				break;
			//r = select(maxfd+1, rd, wr, NULL, tm);
			r = select(maxfd+1, rd, wr, NULL, &tt);
			if (r <= 0) 
			{
				if(r==0)
				{
					try_cnt--;
					continue;
				}	
				if (errno == EINTR) continue;
			}
			break;
		}
    }
   	
    if (foundTimeoutEvent) gettimeofday(&now, NULL);
    errno_save = errno;
    es->nestLevel++;
    if (r >= 0) 
	{
		/* Call handlers */
		for (eh=es->handlers; eh; eh=eh->next) 
		{
			/* Pending delete for this handler?  Ignore it */
			if (eh->flags & EVENT_FLAG_DELETED) continue;
	
			flags = 0;
			if ((eh->flags & EVENT_FLAG_READABLE) &&
			FD_ISSET(eh->fd, &readfds)) 
			{
				flags |= EVENT_FLAG_READABLE;
			}
			if ((eh->flags & EVENT_FLAG_WRITEABLE) &&
				FD_ISSET(eh->fd, &writefds)) 
			{
				flags |= EVENT_FLAG_WRITEABLE;
			}
			if (eh->flags & EVENT_TIMER_BITS) 
			{
				pastDue = (eh->tmout.tv_sec < now.tv_sec ||
					   (eh->tmout.tv_sec == now.tv_sec &&
						eh->tmout.tv_usec <= now.tv_usec));
				if (pastDue) 
				{
					flags |= EVENT_TIMER_BITS;
					if (eh->flags & EVENT_FLAG_TIMER) 
					{
					/* Timer events are only called once */
					es->opsPending = 1;
					eh->flags |= EVENT_FLAG_DELETED;
					}
				}
			}
			/* Do callback */
			if (flags) 
			{
				eh->fn(es, eh->fd, flags, eh->data);
			}
		}
    }

    es->nestLevel--;

    if (!es->nestLevel && es->opsPending) 
	{
		l2tp_PendingChanges(es);
    }
    errno = errno_save;
    return r;
}
int l2tp_HandleEvent_BoostMode(L2tpSelector *es, struct l2tp_if *pInfo, L2tpHandler *readable_eh, int fd, fd_set readfds)
{
    fd_set *rd, *wr;
    unsigned int flags;

    struct timeval abs_timeout, now;
    struct timeval timeout;
    struct timeval *tm;
    struct timeval tt={1,0};
    L2tpHandler *eh;

    int r = 0;
    int errno_save = 0;
    int pastDue;
    int try_cnt=0;

    if (fd > 0) 
	{	
		try_cnt = 1;//for min timeout:1s
		for(;;) 
		{
			l2tp_ScanCmd();
			if(try_cnt<0)
				break;
			r = select(fd+1, &readfds, NULL, NULL, &tt);
			if (r <= 0) 
			{
				if(r==0)
				{
					try_cnt--;
					continue;
				}	
				if (errno == EINTR) continue;
			}
			break;
		}
    }
   	
    if (r > 0) 
	{
	  readable_eh->fn(es, readable_eh->fd, EVENT_FLAG_READABLE, readable_eh->data);
    }else if(r == 0)
    { 
	  gettimeofday(&now, NULL);
      errno_save = errno;
      es->nestLevel++;
	  
	  /* Call handlers */
	  for (eh=es->handlers; eh; eh=eh->next) 
	  {		
	    flags = 0;
		if (eh->flags & EVENT_TIMER_BITS) 
		{
		  pastDue = (eh->tmout.tv_sec < now.tv_sec ||
							 (eh->tmout.tv_sec == now.tv_sec &&
							  eh->tmout.tv_usec <= now.tv_usec));
		  if (pastDue) 
		  {
		    flags |= EVENT_TIMER_BITS;
			if (eh->flags & EVENT_FLAG_TIMER) 
			{
			  /* Timer events are only called once */
			  es->opsPending = 1;
			  eh->flags |= EVENT_FLAG_DELETED;
			}
		  }
		    
	    }

	    if (flags) 
	    {
		  //diag_printf("******enter l2tp_HandleEvent, timerout events happened! pay attenation!******\n");
		  eh->fn(es, eh->fd, flags, eh->data);
	    }
	  }
    
      es->nestLevel--;
      if (!es->nestLevel && es->opsPending) 
	  {
		l2tp_PendingChanges(es);
      }
      errno = errno_save;
    }
	
    return r;
}
/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_hangup         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int	l2tp_hangup(struct l2tp_if *pInfo)
{
    int cmd;

	if(!pInfo->msgq_id)
		return -1;
    cmd = L2TP_CMD_HANGUP;
    cyg_mbox_tryput(pInfo->msgq_id, cmd);
    
    return 0;
}
/*--------------------------------------------------------------
* ROUTINE NAME - L2tp_HandleEvent         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static void usage()
{
	diag_printf("usage: l2tp [up|down]\n");
}

int l2tp_cmd(int argc,char *argv[])
{
	struct l2tp_cfg  *pParam;
	struct l2tp_if   *pInfo;
	PPP_IF_VAR_T *pPppIf;
	
	pParam = &L2tp_config_param;
	pInfo = l2tp_IfNameToInfoList(pParam->pppname);
	pPppIf =&pInfo->if_ppp;

	diag_printf("l2tp_cmd: argc:%d, argv:%s\n", argc, argv[0]);
	if(argc<1)
	{
	   usage();
	   return 1;
	}   
	
	if ( !strcmp(argv[0], "up" ))
	{
		  if(SYS_wan_up != CONNECTED)
		  {
		  	force_conn =1;
		  	mon_snd_cmd(MON_CMD_WAN_UPDATE);
		  }	
	}	
	else if(!strcmp(argv[0], "down" ))
	{
		   //if(!pInfo)	
		   //		return;
		   mon_snd_cmd( MON_CMD_WAN_STOP );
		   cyg_thread_delay(50);
		   //if disconnect button push, change to manual mode
#ifdef CONFIG_POE_KL_FORCE_DISCON			   
		   if(pParam->auto_reconnect != 1) // for auto-connection, pParam->idle_time should not be zero
		   {	
		   		manual_flag = 1;
		   		pParam->auto_reconnect = 2; 
		   }		
#endif		 
		   mon_snd_cmd(MON_CMD_WAN_UPDATE);
    }	
	else
	  usage();
	return 0;
}
#define _string(s) #s
#define	STR(s)	_string(s)
/*--------------------------------------------------------------
* ROUTINE NAME - L2tp_HandleEvent         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int Get_l2tp_up_flag(void)
{
   int flags = 0;
   int wan_mode,auto_mode;
   
   CFG_get( CFG_WAN_IP_MODE, &wan_mode);
   CFG_get( CFG_L2T_AUTO, &auto_mode);

   IfLib_GetFlag (&flags, "ppp0");

   if(wan_mode==5 && auto_mode==1)
   {
      if((flags&IFF_UP)) 
      {
          return true;
      }    
      else
      {
          return false;	
      }    
   }
   else
   {
      return true;       
   }   
}
/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_DaemonWakeup         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void l2tp_DaemonWakeup(void)
{
	if(L2tp_config_param.auto_reconnect == 1)
    	l2tp_DaemonStart(&L2tp_config_param);
}  	

/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_reconnect_pppd         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static int l2tp_reconnect_pppd(struct l2tp_if *pInfo)
{
//    int cmd;
	
    return 0;
}

/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_dialup_checkp         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int l2tp_dialup_checkp(void)
{
	if(!Get_l2tp_up_flag())
	{
		diag_printf("Dial up: l2tp_DaemonWakeup\n");
		l2tp_DaemonWakeup();
		return 0;
	}	
	else
	    return 1;
}	

/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_ScanCmd         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int l2tp_ScanCmd(void)
{
	struct l2tp_cfg  *pParam;
	struct l2tp_if   *pInfo;
	int cmd;
	
	pParam = &L2tp_config_param;
	pInfo =	l2tp_IfNameToInfoList (pParam->pppname); 	
	if(!pInfo)
		return 1;
	if(!pInfo->msgq_id)
		return 1;	
	
	if((cmd = (int)cyg_mbox_tryget(pInfo->msgq_id)) != 0)
    {
           	if (cmd == L2TP_CMD_STOP || cmd == L2TP_CMD_HANGUP)
           	{
           		if(pInfo->if_ppp.ifnet.if_reset && pInfo)
				{	
	   				pInfo->if_ppp.hungup = 1;
    				die(&(pInfo->if_ppp), 1);
    			}
    			else
    			{
    				void *data = pInfo->ses->tunnel;
					if(pInfo->ses->tunnel)
    				{
						lt_StopTunnel(pInfo->ses->tunnel,"Force stop tunnel");
			     		if(pInfo->ses->tunnel)
			     		{
			     			lt_ForceFinallyCleanup(data);
			     		}	
			     	}				
    			}
				
				l2tp_send_process_stop++;
				if(Sock>0)
				{
					close(Sock);
					Sock = -1;	
				}
    			cyg_flag_setbits(&L2tpEvents, L2TP_EVENT_QUIT);
				l2tp_InfoListDel(pInfo);
    			L2TP_InfoListIndex--;
    			      
   				if(pInfo->msgq_id)
    			{
    		      cyg_mbox_delete(pInfo->msgq_id);
    			  pInfo->msgq_id = 0;
    			}	

    			cyg_thread_exit();   
       		}	
    }
	return 0;
}	

#ifdef CONFIG_L2TP_OVER_PPPOE
int l2tp_over_pppoe_mon(void)
{
	ppp_ctx_t *pctrl;
	struct ip_set *pipset;

    if(l2tp_ppp_ctl.pipset == NULL){
	  l2tp_ppp_ctl.pipset = &l2tp_wan_ip_set[0];
	  l2tp_ppp_ctl.ipset_flags = IPSET_FG_DEFROUTE|IPSET_FG_NATIF|IPSET_FG_FWIF;
	  l2tp_ppp_ctl.state = PPP_NO_ETH;
	  l2tp_ppp_ctl.prev_dns[0] = l2tp_ppp_ctl.prev_dns[1] = 0;
	  strcpy(l2tp_ppp_ctl.pipset->ifname, "ppp1");
    }
	pctrl = &l2tp_ppp_ctl;
	pipset = pctrl->pipset;

	SysLog(LOG_USER|LOG_INFO|LOGM_L2TP,"l2tp over pppoe up, pctrl->state:%d", pctrl->state);
	switch(pctrl->state) {
	  case PPP_NO_ETH:
			pipset->mode = L2TPMODE;
			pipset->up = DISCONNECTED;
			pipset->flags = 0;
			pipset->data = &l2tp_wan_ip_set[1];
			pctrl->state = PPP_WAIT_ETH;

		case PPP_WAIT_ETH:
			pctrl->state = PPP_CHK_SRV;
			//DNS_CFG_get_autodns(pctrl->prev_dns, 2);
	
			/*	Fall to PPP_CHK_SRV  */
		case PPP_CHK_SRV:
			{
			struct in_addr dst, mask, gw, hostmask;
			int i,fqdn=0;
			char sever_name[256];
			struct hostent *hostinfo;
			//DNS_CFG_autodns_update(2, (char *)pctrl->prev_dns);
	
			hostmask.s_addr =	0xffffffff;
			mask.s_addr =		primary_wan_ip_set[0].netmask;
			gw.s_addr = 		primary_wan_ip_set[0].gw_ip;
			dst.s_addr =		primary_wan_ip_set[0].server_ip;
	
			/* Add new route for Russia DualWAN case*/
			if (pctrl->staticip == 0)
			{
			  if ((dst.s_addr & mask.s_addr) != (gw.s_addr & mask.s_addr))
				RT_add_del(NULL, &dst, &hostmask, &gw, 1, 0, ROUTE_ADD);
			}
	
				/*	 Set route to the DNS server  */
			for (i=0; i<2; i++) {
			  if ((dst.s_addr = pctrl->prev_dns[i]) == 0)
				continue;
	
			  if ((dst.s_addr&mask.s_addr) != (gw.s_addr & mask.s_addr)) 
				RT_add_del(NULL, &dst, &hostmask, &gw, 1, 0, ROUTE_ADD);
			}
				
			CFG_get( CFG_L2T_SVRD_SEL, &fqdn );
			if (fqdn) {
			  CFG_get( CFG_L2T_SVRDM, &sever_name );
			  hostinfo = gethostbyname(sever_name);
			  if (hostinfo && (hostinfo->h_length == 4)) {
				memcpy(&(dst.s_addr), hostinfo->h_addr_list[0], 4);
				pipset->tunnel_gw = dst.s_addr;
			  } else{
				timeout(mon_snd_cmd, MON_CMD_WAN_INIT, 200);
				return 0;
			  }
			} else {
			  CFG_get(CFG_L2T_SVR,&dst.s_addr);
			  pipset->tunnel_gw = dst.s_addr; 
			}
			/*	l2tp tunnel server might be not in the WAN domain.	*/
			if((gw.s_addr &mask.s_addr)!= (dst.s_addr & mask.s_addr)) {
			  if(dst.s_addr)
			    RT_add_del(NULL, &dst, &hostmask, &gw, 1, 0, ROUTE_ADD);
			}	
			}
	
			/*	Fall to PPP_GET_ETH  */
		case PPP_GET_ETH:
			//DNS_CFG_autodns_update(2, (char *)pctrl->prev_dns);
			pctrl->state = PPP_IN_BEGIN;
	        //pipset->up = CONNECTING;
			//RT_add_default(primary_wan_ip_set[1].ifname, primary_wan_ip_set[1].gw_ip);
			L2tp_doConnect();
			break;
	
		case PPP_IN_BEGIN:
		case PPP_IN_ACTIVE:
			//if (pctrl->staticip == 0)
			//	DHCPC_stop_DNS_update(WAN_NAME);
			memcpy(pipset, &wan_ip_set, sizeof(struct ip_set));
			//pipset->data = &l2tp_wan_ip_set[1];
			pipset->tunnel_mode = 1;
			pipset->up = CONNECTED;
			pipset->flags = pctrl->ipset_flags;
			pctrl->state = PPP_IN_ACTIVE;
			
			//RT_del_default(primary_wan_ip_set[1].ifname, primary_wan_ip_set[1].gw_ip);
			/*	Configure wan interface ip and routes  */
			//config_wans();
			break;
		default:
			diag_printf("Unknown L2TP wan state: %d\n", pctrl->state);
			break;
		}

		return 0;
}

extern void RT_ifp_flush(char *ifname);
extern void *l2tp_ppp_ifnet;

int l2tp_over_pppoe_mon_down(void)
{
	ppp_ctx_t *pctrl = &l2tp_ppp_ctl;
	struct ip_set *pipset = pctrl->pipset;
	SysLog(LOG_USER|LOG_INFO|LOGM_L2TP,"l2tp over pppoe down");

    if(pipset != NULL){
	  /*  Shutdown L2TP  */
	  L2tp_down();    		
	  cyg_thread_delay(30);  //wait for l2tp clean up
	  pctrl->state = PPP_CHK_SRV;
	  //if (pctrl->staticip == 0)
	  //	DHCPC_start_DNS_update(WAN_NAME);
	
	  /*  Remove all routes related with pptp interface */
	  RT_ifp_flush(pipset->ifname);
	  RT_ifa_flush(pipset->ip);

	  /*  Remove L2TP IP address  */
	  //if (!netif_del(pipset->ifname, NULL))
	  //  diag_printf("l2tp:del l2tp ip failed\n");

	  if (l2tp_ppp_ifnet)
		if_detach(l2tp_ppp_ifnet);
	  l2tp_ppp_ifnet = NULL;
	
	  pctrl->state = PPP_NO_ETH;
	  pctrl->pipset = NULL;
	  memset(&l2tp_wan_ip_set[0], 0, sizeof(struct ip_set));
	  l2tp_wan_ip_set[0].flags |= IPSET_FG_IFCLOSED;
	  l2tp_wan_ip_set[0].up = DISCONNECTED;	
    }
	
	return 0;  
}
#endif
