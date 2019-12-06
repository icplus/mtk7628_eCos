/*
	function handler for netlink socket!
 */
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "wsc_common.h"
#include "wsc_netlink.h"
#include "wsc_msg.h"
#include "wsc_msg.h"
#include "wsc_ioctl.h"/* for iwreq stuff */
#include "upnp.h"


typedef struct wps_cmd_s
{
	int	msgLen;
	char *pMsgBuf;
	struct wps_cmd_s *next;
} WPS_CMD_T;

static	WPS_CMD_T	wps_msg_pool[CYGNUM_KERNEL_SYNCH_MBOX_QUEUE_SIZE];
static	WPS_CMD_T	*wps_free_head;

static	cyg_mutex_t	wps_lock;
static	cyg_mbox	wps_msgBox;
int wps_msqid = -1;
int	nl_flag;
int stopNLThread = 1;


int wps_cmdq_create(void)
{
	cyg_handle_t	qid;
	
	int	i;
	
	// Create lock
	cyg_mutex_init(&wps_lock);
	
	// resource initialization
	for (i=0 ; i<CYGNUM_KERNEL_SYNCH_MBOX_QUEUE_SIZE; i++)
		wps_msg_pool[i].next = wps_msg_pool + i+1;
	
	wps_msg_pool[i-1].next = 0;
	wps_free_head = wps_msg_pool;
	
	// Create mail box
	cyg_mbox_create((cyg_handle_t*)&qid, &wps_msgBox);
	
	return (int)qid;
}


int wps_cmdq_delete(int	qid)
{	
	cyg_mbox_delete((cyg_handle_t)qid);
	cyg_mutex_destroy(&wps_lock);
	return 0;
}


static void wscEventHandler(char *data, int len)
{
	char *pos, *end;

	pos = data;
	end = data + len;

	if(strncmp(data, "RAWSCMSG", 8) == 0)
	{	
		DBGPRINTF(RT_DBG_INFO, "Recevive a RAWSCMSG segment\n");
		WscRTMPMsgHandler(data, len);
	}
}


static void wscNLEventHandle(void)
{
	DBGPRINTF(RT_DBG_INFO, "wscNLEventHandle while-loop start!\n");

    while (!stopNLThread)
    {
		WPS_CMD_T *pWscK2UMsg = NULL;

		// Receive the message
		pWscK2UMsg = cyg_mbox_get((cyg_handle_t)wps_msqid);

		if (pWscK2UMsg)
		{
			if (pWscK2UMsg->pMsgBuf)
			{
				wscEventHandler(pWscK2UMsg->pMsgBuf, pWscK2UMsg->msgLen);
			}
			else
			{
				printf("msg->pMsg == NULL\n");
			}

			free(pWscK2UMsg);
		}
		else
		{
			printf("cyg_mbox_get failed!\n");
		}
	}

	return;
}


/*==============================================================================
 * Function:    nl_thread_create()
 *------------------------------------------------------------------------------
 * Description: create dispatch thread for event from driver
 *
 *============================================================================*/
/*static*/ cyg_handle_t	wscNLEventHandle_tid;
static char 		wscNLEventHandle_stack[8192];
static cyg_thread	wscNLEventHandle_thread;

int nl_thread_create(void)
{
	cyg_thread_create(
			8,
			(cyg_thread_entry_t *)&wscNLEventHandle,
			0,
			"wscNLEventHandle",
			&wscNLEventHandle_stack[0],
			sizeof(wscNLEventHandle_stack),
			&wscNLEventHandle_tid,
			&wscNLEventHandle_thread);
			
	cyg_thread_resume(wscNLEventHandle_tid);
	
	return 0;
}


int wscNLSockRecv(char *pData, uint32 dataLen)
{
	int status = -1, memLen;
	char *memPtr;
	WPS_CMD_T *pK2uMsg;
	
	if (pData == NULL)
	{
		perror("wscNLSockRecv(pData == NULL)");
		return -1;
	}
	
	if (dataLen && (wps_msqid != -1))
	{
		memLen = dataLen + sizeof(WPS_CMD_T);
		memPtr = malloc(memLen);
		if (memPtr)
		{
			pK2uMsg = (WPS_CMD_T *)memPtr;
			pK2uMsg->pMsgBuf = memPtr + sizeof(WPS_CMD_T);
			pK2uMsg->msgLen = dataLen;
			
			memcpy(pK2uMsg->pMsgBuf, pData, dataLen);
			if (cyg_mbox_put((cyg_handle_t)wps_msqid, (void *)memPtr) == FALSE)
			{
				free(memPtr);
				status = 0;
			}
		}
	}
	
	DBGPRINTF(RT_DBG_INFO, "<== wscNLSockRecv()\n");

	return status;
}


/******************************************************************************
 * wscK2UModuleInit
 *
 * Description: 
 *       The Kernel space 2 user space msg subsystem entry point.
 *	We use netlink socket to receive the specific type msg
 *	  send from wireless driver.
 *		 This function mainly create a posix thread and recvive msg then dispatch
 *	  to coressponding handler.
 *
 * Parameters:
 *    None
 * 
 * Return:
 *    success: 1
 *    fail   : 0
 *****************************************************************************/
extern int rt28xx_ap_ioctl(
	PNET_DEV	pEndDev, 
	int			cmd, 
	caddr_t		pData);

int wscK2UModuleInit(void)
{
	int status;
    char name[IFNAMSIZ];
	cyg_netdevtab_entry_t *t;
	struct eth_drv_sc *sc;
	int found = 0;

        DBGPRINTF(RT_DBG_INFO, "====>%s\n", __FUNCTION__);

        /* To confirm the interest exist */
	strncpy(name, WSC_IOCTL_IF, IFNAMSIZ);//sht:set default interface is ra0 after init
	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;
		if (strcmp(sc->dev_name, name) == 0)
		{
			found = 1;
			break;
	    }
	}

        /* Set callback function address */
	if (found == 1)
	{
        struct iwreq iwr;

        memset(&iwr, 0, sizeof(iwr));
        iwr.u.data.pointer = (caddr_t) wscNLSockRecv;
        iwr.u.data.flags = RTPRIV_IOCTL_WSC_CALLBACK;
        iwr.u.data.flags |= OID_GET_SET_TOGGLE;
        iwr.u.data.length = sizeof(caddr_t);
#ifdef CONFIG_AP_SUPPORT		
		status = rt28xx_ap_ioctl(sc, RTPRIV_IOCTL_WSC_CALLBACK, (caddr_t)&iwr);
#endif
#ifdef CONFIG_STA_SUPPORT		
		status = rt28xx_sta_ioctl(sc, RTPRIV_IOCTL_WSC_CALLBACK, (caddr_t)&iwr);
#endif
		//diag_printf("!!Set callback for 2.4G device\n");
	} else
        return WSC_SYS_ERROR;

	if (status != 0)
	{
        DBGPRINTF(RT_DBG_ERROR, "%s: Can't set WSC Callback function, status=%d\n", __FUNCTION__, status);
		return WSC_SYS_ERROR;
	}
	
#ifdef SECOND_WIFI// 0//mytest SECOND_WIFI
	/* To confirm the interest exist */
	memset(name,0,IFNAMSIZ);
	strncpy(name, "rai0", IFNAMSIZ);//init the second wifi interface : rai0 :5G
	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;
		if (strcmp(sc->dev_name, name) == 0)
		{
			found = 1;
			break;
		}
	}

		/* Set callback function address */
	if (found == 1)
	{
		struct iwreq iwr;

		memset(&iwr, 0, sizeof(iwr));
		iwr.u.data.pointer = (caddr_t) wscNLSockRecv;
		iwr.u.data.flags = RTPRIV_IOCTL_WSC_CALLBACK;
		iwr.u.data.flags |= OID_GET_SET_TOGGLE;
		iwr.u.data.length = sizeof(caddr_t);
#ifdef CONFIG_AP_SUPPORT		
		status = rt28xx_ap_ioctl(sc, RTPRIV_IOCTL_WSC_CALLBACK, (caddr_t)&iwr);
#endif
#ifdef CONFIG_STA_SUPPORT		
		status = rt28xx_sta_ioctl(sc, RTPRIV_IOCTL_WSC_CALLBACK, (caddr_t)&iwr);
#endif

		//diag_printf("!!Set callback for 5G device\n");
	} else
		return WSC_SYS_ERROR;

	if (status != 0)
	{
		DBGPRINTF(RT_DBG_ERROR, "%s: Can't set WSC Callback function,interface=%s status=%d\n", __FUNCTION__, name,status);
		return WSC_SYS_ERROR;
	}
#endif


	/* start a mailbox receiver handle thread */
	status = nl_thread_create();
	if(status != 0)
	{
        DBGPRINTF(RT_DBG_ERROR, "%s: Can't start mailbox handle thread, status=%d\n", __FUNCTION__, status);
		nl_flag = NL_FLAG_SHUTDOWN;
		return WSC_SYS_ERROR;	
	}

        DBGPRINTF(RT_DBG_INFO, "<====%s\n", __FUNCTION__);
	return WSC_SYS_SUCCESS;
}


int wscK2UModuleExit(void)
{
	int status;
    char name[IFNAMSIZ];
	cyg_netdevtab_entry_t *t;
	struct eth_drv_sc *sc;
	int found = 0;

        DBGPRINTF(RT_DBG_INFO, "====>%s\n", __FUNCTION__);

        /* To confirm the interest exist *///handle 2.4G interface:ra0.
	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;
		if (strcmp(sc->dev_name, "ra0") == 0)
		{
			found =1;
			break;
	    }
	}

	/* Clear callback function address */
	if (found == 1)
	{
        	struct iwreq iwr;
                memset(&iwr, 0, sizeof(iwr));
                iwr.u.data.pointer = (caddr_t) NULL;
                iwr.u.data.flags = RTPRIV_IOCTL_WSC_CALLBACK;
                iwr.u.data.flags |= OID_GET_SET_TOGGLE;
                iwr.u.data.length = sizeof(caddr_t);
#ifdef CONFIG_AP_SUPPORT				
		status = rt28xx_ap_ioctl(sc, RT_PRIV_IOCTL, (caddr_t)&iwr);
#endif
#ifdef CONFIG_STA_SUPPORT				
		status = rt28xx_sta_ioctl(sc, RT_PRIV_IOCTL, (caddr_t)&iwr);
#endif

		//diag_printf("!!Clear callback for 2.4G device\n");
	}
	else
	{
		return WSC_SYS_ERROR;
	}

#ifdef SECOND_WIFI
		/* To confirm the interest exist *///handle 5G interface:rai0.
	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;
		if (strcmp(sc->dev_name, "rai0") == 0)
		{
			found =1;
			break;
		}
	}

	/* Clear callback function address */
	if (found == 1)
	{
			struct iwreq iwr;
				memset(&iwr, 0, sizeof(iwr));
				iwr.u.data.pointer = (caddr_t) NULL;
				iwr.u.data.flags = RTPRIV_IOCTL_WSC_CALLBACK;
				iwr.u.data.flags |= OID_GET_SET_TOGGLE;
				iwr.u.data.length = sizeof(caddr_t);
#ifdef CONFIG_AP_SUPPORT				
		status = rt28xx_ap_ioctl(sc, RT_PRIV_IOCTL, (caddr_t)&iwr);
#endif
#ifdef CONFIG_STA_SUPPORT				
		status = rt28xx_sta_ioctl(sc, RT_PRIV_IOCTL, (caddr_t)&iwr);
#endif

		//diag_printf("!!Clear callback for 5G device\n");
	}
	else
	{
		return WSC_SYS_ERROR;
	}

#endif
        /* stop a mailbox receiver handle thread */
        nl_flag = NL_FLAG_SHUTDOWN;
	
        DBGPRINTF(RT_DBG_INFO, "<====%s\n", __FUNCTION__);
	return WSC_SYS_SUCCESS;
}

