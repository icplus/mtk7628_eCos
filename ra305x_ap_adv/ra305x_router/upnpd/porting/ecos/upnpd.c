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

#include <upnp.h>
#include <upnp_portmap.h>

typedef struct upnp_cmd_s
{
	int	op;
	int	fd;
	struct upnp_cmd_s *next;
}
UPNP_CMD_T;

#define UPNP_PRIORITY  16 
#define UPNP_STACKSIZE 4096 

cyg_handle_t upnp_main_tid;
cyg_thread upnp_main_thread;
unsigned char upnp_main_stack[UPNP_STACKSIZE];
extern cyg_mutex_t upnp_service_table_mutex;
//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void UPNP_init(void) 
{
	int newEn;
	static int oldEn = 0;

	CFG_get(CFG_UPP_EN, &newEn);
	if ((oldEn==0) && (newEn==1))
	{
		cyg_thread_create(UPNP_PRIORITY,
				(cyg_thread_entry_t *)&upnp_main,
				0,
				"upnp_main",
				upnp_main_stack,
				sizeof(upnp_main_stack),
				&upnp_main_tid,
				&upnp_main_thread);
		
		cyg_thread_resume(upnp_main_tid);
	} 
	else if ((oldEn==1) && (newEn==0))
	{
		upnp_flag = UPNP_FLAG_SHUTDOWN;
	}
	else if ((oldEn==1) && (newEn==1))
	{
		upnp_flag = UPNP_FLAG_RESTART;
	}
	
	oldEn = newEn;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void getUpnpPmapEntry(int idx,char *entry) 
{
	UPNP_PORTMAP *upnpPmEntry = portmap_with_index(idx) ; 
	
	if (upnpPmEntry != NULL) {
		sprintf(entry,"%d;%s;%d;%s;%d;%s;%ld;%s",
			upnpPmEntry->enable,
			upnpPmEntry->remote_host,
			upnpPmEntry->external_port,
			upnpPmEntry->internal_client,
			upnpPmEntry->internal_port,
			upnpPmEntry->protocol,
			upnpPmEntry->duration,
			upnpPmEntry->description) ;
	}
	else
		strcpy(entry, "0;0;0;0;0;0;0;<NULL>");
}


//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
#ifdef CONFIG_CLI_NET_CMD
int upp_cmd(int argc, char* argv[])
{
	extern UPNP_PORTMAP *upnp_pmlist;
	extern UPNP_CONTEXT upnp_context;
	
	UPNP_CONTEXT *context = &upnp_context;
	UPNP_SERVICE *service = upnp_service_table;
	UPNP_SUBSCRIBER	*subscriber;

	UPNP_INTERFACE *ifp;
	
        if (argc==0 || !strcmp(argv[0], "show") )
        {
                printf("# Interfaces\n");
		for (ifp=context->iflist; ifp; ifp=ifp->next)
		{
			printf("%10s %6d\n", ifp->ifname, ifp->http_sock);
		}
		
		printf("\n");
		
                if (upnp_pmlist==0)
                {
                        printf("no port map!\n");
                }
                else
                {
			extern void upp_show_pmap(void);
			
			upp_show_pmap();
                        printf("\n");
#ifdef CONFIG_WPS                        
                        dumpDevCPNodeList();
#endif /* CONFIG_WPS */
                }
		
                printf("\n");
                printf("# Subscriber service\n");
		cyg_mutex_lock(&upnp_service_table_mutex);		
		for (service = upnp_service_table; service->event_url; service++)
		{
			subscriber = service->subscribers;
			
			while (subscriber)
			{
				printf("%lu.%lu.%lu.%lu:%d%s %s %u\n",
						(subscriber->ipaddr >> 24) & 0xff,
						(subscriber->ipaddr >> 16) & 0xff,
						(subscriber->ipaddr >> 8) & 0xff,
						subscriber->ipaddr & 0xff,
						subscriber->port,
						subscriber->uri,
						subscriber->sid,
						subscriber->expire_time);
				
				subscriber = subscriber->next;
			}
		}
		cyg_mutex_unlock(&upnp_service_table_mutex);
        }
	else if (( argc == 2 ) && (!strcmp(argv[0], "debug" )))
	{
                int level = 0;
        
                level = atoi(argv[1]);    
                if (level > 0)
                        upnp_debug_level = level;
                else
                        upnp_debug_level = RT_DBG_OFF;


                DBGPRINTF(RT_DBG_OFF, "UPNPD debug level = %d\n", upnp_debug_level);
	}        
        else
                goto err1;

        return 0;

err1:
        printf("upnp show\n");
        printf("upnp debug <value>\n");
        return -1;
}

#endif
/* this function is called by ../init/wan_start.c */
void upnp_update_wanstatus(void)
{
	upnp_event_alarm("/event?WANIPConnection");
}


//####################################################
// OS related functions needed by upnp.c
//####################################################

/*==============================================================================
 * Function:    read_config()
 *------------------------------------------------------------------------------
 * Description: Read UPnP daemon configuration from file
 *
 *============================================================================*/
int upnp_read_config(void *config)
{
	UPNP_CONTEXT *context = (UPNP_CONTEXT *)config;
	
	/* initialize upnp_config to default values */
	CFG_get(CFG_UPP_PORT,		&context->http_port);
	CFG_get(CFG_UPP_AD_TIME,	&context->adv_time);
	CFG_get(CFG_UPP_SUB_TIMEOUT,&context->sub_time);
	
	strcpy(context->upnp_if[0].ifname, LAN_NAME);
	return 0;
}


/*==============================================================================
 * Function:    upnp_if_hwaddr()
 *------------------------------------------------------------------------------
 * Description: adjust time for upnp because of NTP
 *
 *============================================================================*/
int upnp_if_hwaddr(void *upnp_ifp)
{
	// Should we implement the direct send to kernel?
	return 0;
}


/*==============================================================================
 * Function:    upnp_curr_time()
 *------------------------------------------------------------------------------
 * Description: adjust time for upnp because of NTP
 *
 *============================================================================*/
time_t upnp_curr_time(time_t *now_p)
{
	time_t	now;
	
	extern	time_t ntp_getlocaltime(time_t *);
	
	now = GMTtime(0);

	if (now_p)
		*now_p = now;

	return now;
}


/*==============================================================================
 * Function:    upnp_mbox_create()
 *------------------------------------------------------------------------------
 * Description: create mail box for dispatch program to receive message
 *
 *============================================================================*/
static	UPNP_CMD_T	msg_pool[CYGNUM_KERNEL_SYNCH_MBOX_QUEUE_SIZE];
static	UPNP_CMD_T	*free_head;

static	cyg_mutex_t	lock;
static	cyg_mbox	msgBox;

static UPNP_CMD_T *msg_alloc(void)
{
	UPNP_CMD_T *msg;
	
	// allocate a memory for command to send
	cyg_mutex_lock(&lock);
	
	msg = free_head;
	if (msg)
		free_head = free_head->next;
	
	cyg_mutex_unlock(&lock);
	
	return msg;
}

static void msg_free(UPNP_CMD_T *msg)
{
	cyg_mutex_lock(&lock);
	
	msg->next = free_head;
	free_head = msg;
	
	cyg_mutex_unlock(&lock);
	
	return;
}


int upnp_cmdq_create(void)
{
	cyg_handle_t	qid;
	
	int	i;
	
	// Create lock
	cyg_mutex_init(&lock);
	
	// resource initialization
	for (i=0 ; i<CYGNUM_KERNEL_SYNCH_MBOX_QUEUE_SIZE; i++)
		msg_pool[i].next = msg_pool + i+1;
	
	msg_pool[i-1].next = 0;
	free_head = msg_pool;
	
	// Create mail box
	cyg_mbox_create((cyg_handle_t*)&qid, &msgBox);
	
	return (int)qid;
}

int upnp_cmdq_send(int qid,int op,int fd)
{
	UPNP_CMD_T *msg;
	
	// allocate a memory for command to send
	msg = msg_alloc();
	if (msg == 0)
		return -1;
	
	// Copy the message to send
	msg->op = op;
	msg->fd = fd;
	if (cyg_mbox_put((cyg_handle_t)qid, msg) == 0)
	{
		msg_free(msg);
		return -1;
	}
	
	return 0;
}


int upnp_cmdq_rcv(int qid, int *op, int *fd)
{
	UPNP_CMD_T	*msg;
	
	// Receive the message
	msg = cyg_mbox_get((cyg_handle_t)qid);
	
	// Copy to upper layer
	*op = msg->op;
	*fd = msg->fd;
	
	// Free this message
	msg_free(msg);
	return 0;
}


int upnp_cmdq_delete(int qid)
{	
	cyg_mbox_delete((cyg_handle_t)qid);
	cyg_mutex_destroy(&lock);
	return 0;
}


/*==============================================================================
 * Function:    upnp_thread_create()
 *------------------------------------------------------------------------------
 * Description: create dispatch thread
 *
 *============================================================================*/
static cyg_handle_t	upnp_daemon_tid;
static char 		upnp_daemon_stack[8192];
static cyg_thread	upnp_daemon_thread;

int upnp_thread_create(void)
{
	cyg_thread_create(
			8,
			(cyg_thread_entry_t *)&upnp_daemon,
			0,
			"upnp_daemon",
			&upnp_daemon_stack[0],
			sizeof(upnp_daemon_stack),
			&upnp_daemon_tid,
			&upnp_daemon_thread);
			
	cyg_thread_resume(upnp_daemon_tid);
	
	return 0;
}


/*==============================================================================
 * Function:    upnp_timer_init()
 *------------------------------------------------------------------------------
 * Description: timer initialization
 *
 *============================================================================*/
static void signal_alarm(int sig)
{
	upnp_timer_alarm();
	timeout((timeout_fun *)signal_alarm, 0, 200);
}

void upnp_timer_shutdown(void)
{
	untimeout((timeout_fun *)signal_alarm, 0);
}

void upnp_timer_init(void)
{
	timeout((timeout_fun *)signal_alarm, 0, 200);
}

