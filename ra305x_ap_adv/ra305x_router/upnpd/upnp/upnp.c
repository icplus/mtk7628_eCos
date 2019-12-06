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
#ifdef CONFIG_WPS
#include "wsc/wsc_common.h"
#include "wsc/wsc_upnp.h"
#endif /* CONFIG_WPS */

/*
 * Variables
 */
int upnp_debug_level = RT_DBG_ERROR;
int upnp_msqid = -1;
int upnp_flag;
UPNP_CONTEXT upnp_context;

#ifdef CONFIG_WPS
char wps_uuidvalue[] = "uuid:00000000-0000-0000-0000-000000000000";
char wps_serialnumber[SERIALNUMBER_MAX_LEN] = "87654321";

#ifdef SECOND_WIFI
char wps_5G_uuidvalue[] = "uuid:00000000-0000-0000-0000-000000000000";
//char wps_serialnumber[SERIALNUMBER_MAX_LEN] = "87654321";
#endif /* SECOND_WIFI */

#endif /* CONFIG_WPS */

cyg_mutex_t upnp_service_table_mutex;



/* Functions */
extern int upnp_device_init(void);
extern void upnp_device_shutdown(void);
extern void upnp_device_timeout(time_t);


//###############################################################
// DAEMON
//###############################################################
/*==============================================================================
 * Function:    upnp_daemon()
 *------------------------------------------------------------------------------
 * Description: Process the newly accepted socket by UPnP daemon process
 *
 *============================================================================*/
void upnp_daemon()
{
	UPNP_CONTEXT *context = &upnp_context;

        DBGPRINTF(RT_DBG_ERROR, "%s Start, Adv_time=%d, Sub_time=%d\n", __FUNCTION__, context->adv_time, context->sub_time);

        while (1)
        {	
                time_t now;        
		int op;
		int fd;

		upnp_cmdq_rcv(upnp_msqid, &op, &fd);

		switch (op)
		{
			case UPNP_HTTP_NEWFD:
				upnp_http_process(context, fd);
				break;

            case UPNP_SSDP_NEWFD:
                ssdp_process(context);
                break;
                                
			case UPNP_TIMER_ALARM:
				/* Update time */
				upnp_curr_time(&now);

				/* check for advertisement interval */
				ssdp_timeout(context, now);
				
				/* check for subscription expirations */
				gena_timeout(context, now);

				/* Add device timer here */
				upnp_device_timeout(now); 
				break;

			case UPNP_EVENT_ALARM:
				gena_event_alarm(context, (char *)fd);
				break;

			case UPNP_DAEMON_STOP:
				/* delete the message queue */
				upnp_cmdq_delete(upnp_msqid);
				upnp_msqid = -1;				
				/* should not be reached */
				upnp_thread_exit();
				
			default:
				break;
		}			

	}

EXIT:
        DBGPRINTF(RT_DBG_INFO, "%s: End\n", __FUNCTION__);
}


//###############################################################
// STARTUP/SHUTDOWN
//###############################################################
/*==============================================================================
 * Function:    upnp_daemon_shutdown()
 *------------------------------------------------------------------------------
 * Description: Notify upnp_daemon thread to exit
 *
 *============================================================================*/
void upnp_daemon_shutdown(void)
{
	if (upnp_msqid == -1)
		return;
	
	/* tell upnp_daemon thread to exit */
	while (upnp_cmdq_send(upnp_msqid, UPNP_DAEMON_STOP, -1) != 0)
	{
		// Continue until queue empty
		upnp_thread_sleep(1);
	}
	
	/* Wait until all closed */
	while (upnp_msqid != -1)
	{
		upnp_thread_sleep(1);
	}

        DBGPRINTF(RT_DBG_INFO, "%s\n", __FUNCTION__);
	return;
}


/*==============================================================================
 * Function:    shutdown_upnp()
 *------------------------------------------------------------------------------
 * Description: UPnP daemon shutdown routine
 *              
 *
 *============================================================================*/
void shutdown_upnp(void)
{
	UPNP_CONTEXT *context_upnp = &upnp_context;

	/* Shutdown protocol */
	upnp_timer_shutdown();
	
	upnp_daemon_shutdown();

	upnp_http_shutdown(context_upnp);
	
	ssdp_shutdown(context_upnp);
	
	gena_shutdown(context_upnp);
	
	/* Unhook device database */
	upnp_device_shutdown();

	cyg_mutex_destroy(&upnp_service_table_mutex);
        DBGPRINTF(RT_DBG_ERROR, "%s: UPnP daemon is stopped\n", __FUNCTION__);        
}


/*==============================================================================
 * Function:    get_if_ipaddr()
 *------------------------------------------------------------------------------
 * Description: Get interface IP addresses and netmasks from interface names
 *              
 *
 *============================================================================*/
static int get_if_ipaddr(UPNP_INTERFACE *ifp)
{
	int s;
	int status;
	struct ifreq  ifr;
	
	/* Open a raw socket */
	if ((s = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;
	
	strncpy(ifr.ifr_name, ifp->ifname, sizeof (ifr.ifr_name));
        ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
	
	/* Get interface address */
	status = ioctl(s, SIOCGIFADDR, (int)&ifr);
	if (status != 0)
	{
		close(s);
		return -1;
	}
	
	ifp->ipaddr = ntohl(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);
	
	/* Get interface netmask */
	strncpy(ifr.ifr_name, ifp->ifname, sizeof (ifr.ifr_name));
        ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
	status = ioctl(s, SIOCGIFNETMASK, (int)&ifr);
	if (status != 0)
	{
		close(s);
		return -1;
	}
	
	ifp->netmask = ntohl(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);
	
	close(s);
	return 0;
}


/*==============================================================================
 * Function:    init_if()
 *------------------------------------------------------------------------------
 * Description: Add an interface to the upnp_iflist
 *
 *============================================================================*/
static int init_if(UPNP_INTERFACE *ifp)
{
	// Initialize this interface
	ifp->http_sock = -1;
	
	if (get_if_ipaddr(ifp) != 0)
	{
	        DBGPRINTF(RT_DBG_OFF, "Cannot get IP address of %s\n", ifp->ifname);
		return -1;
	}
	
	if (upnp_if_hwaddr(ifp) != 0)
	{
	        DBGPRINTF(RT_DBG_OFF, "Cannot get MAC address of %s", ifp->ifname);
		return -1;
	}
	
	return 0;
}


/*==============================================================================
 * Function:    context_init()
 *------------------------------------------------------------------------------
 * Description: Initialize the context used by wps_upnp_daemon and ssdp_daemon 
 *
 *============================================================================*/
int context_init(UPNP_CONTEXT *context)
{
	UPNP_INTERFACE *ifp;

	/* Clean up */
	memset(context, 0, sizeof(*context));
	context->http_sock = -1;
	context->ssdp_sock = -1;

	/* read upnpd conf */
	if (upnp_read_config(context) == -1)
	{
                DBGPRINTF(RT_DBG_OFF, "Configuration file read failed!\n");	
		goto error_out;
	}

	/* Should initialize interface */
	ifp = context->upnp_if;
	while (ifp != context->upnp_if + UPNP_IF_NUM &&
		   ifp->ifname[0] != 0)
	{
		if (init_if(ifp) != 0)
			goto error_out;
		
		ifp->next = context->iflist;
		context->iflist = ifp;
		
		/* Advance to next */
		ifp++;
	}	

	if (context->iflist == 0)
	{
		/* no interface found, return directly */
                DBGPRINTF(RT_DBG_OFF, "No UPnP interface specified, bye!");
		goto error_out;
	}
	
	return 0;

error_out:
	upnp_flag = UPNP_FLAG_SHUTDOWN;/* for main loop */
	return -1;
}

int ssdp_Update_MulticastIP(void)
{
        UPNP_CONTEXT *context_upnp = &upnp_context;

	if (context_upnp->ssdp_sock != -1) {
                UPNP_INTERFACE *ifp;
                
        	ifp = context_upnp->iflist;
        	while (ifp)
        	{
        		context_upnp->focus_ifp = ifp;
        		
        		if (ssdp_add_multi(context_upnp) == -1)
        			return -1;
        		
        		ifp = ifp->next;
        	}
        }
}

int ssdp_Delete_MulticastIP(void)
{
        UPNP_CONTEXT *context_upnp = &upnp_context;

	if (context_upnp->ssdp_sock != -1) {
                UPNP_INTERFACE *ifp;
                
        	ifp = context_upnp->iflist;
        	while (ifp)
        	{
        		context_upnp->focus_ifp = ifp;
        		
                        ssdp_del_multi(context_upnp);
        		
        		ifp = ifp->next;
        	}
        }
}

/*==============================================================================
 * Function:    start_upnp()
 *------------------------------------------------------------------------------
 * Description: UPnP daemon initialization
 *              
 *
 *============================================================================*/
int start_upnp(void)
{
	UPNP_CONTEXT *context_upnp = &upnp_context;

	/*
	 * Init mutex
	 */
	cyg_mutex_init(&upnp_service_table_mutex);

	// Hook device table
	if (upnp_device_init() != 0)
		goto error_out;
	
	/* upnp xml port initialization */
        DBGPRINTF(RT_DBG_ERROR, "Init UPNP\n");
	if (upnp_http_init(context_upnp) != 0)
		goto error_out;	
	
	/* Initialize SSDP */
        DBGPRINTF(RT_DBG_ERROR, "Init SSDP\n");
	if (ssdp_init(context_upnp) != 0)
		goto error_out;
	
	//added by haitao:context->udp_sock has not in use.
	/*if (ssdp_udp_init(context_upnp) != 0)
		goto error_out;*/
		
	/* Initialize Gena */
        DBGPRINTF(RT_DBG_ERROR, "Init Gena\n");
	if (gena_init(context_upnp) != 0)
		goto error_out;

	/* Create upnp_http message queue */
	upnp_msqid = upnp_cmdq_create();
	if (upnp_msqid == -1)
	{
	        DBGPRINTF(RT_DBG_ERROR, "upnp_start: Cannot create upnp_http message queue\n");
		goto error_out;
	}
	
	/*
	 * create upnp_daemon thread
	 */
	if (upnp_thread_create() != 0)
		goto error_out;
	
	/*
	 * Init timer
	 */
	upnp_timer_init();
	
    DBGPRINTF(RT_DBG_ERROR, "UPnP daemon is ready to run\n");
	return 0;
	
error_out:
	upnp_flag = UPNP_FLAG_SHUTDOWN;/* for main loop */
	return -1;
}


/*==============================================================================
 * Function:    upnp_http_accept()
 *------------------------------------------------------------------------------
 * Description: Accept a connection and notify upnp_daemon thread
 *              to handle the new connection.
 *============================================================================*/
static void upnp_http_accept(int s)
{
	struct sockaddr_in addr;
	int addr_len;
	int ns;

	/* accept new upnp_http socket */
	addr_len = sizeof(struct sockaddr_in);
	ns = accept(s, (struct sockaddr *)&addr, &addr_len);// arg3 differ in signedness
	if (ns == -1)
		return ;
		
	if (upnp_cmdq_send(upnp_msqid, UPNP_HTTP_NEWFD, ns) != 0)
	{
		printf("upnp_cmdq_send() failed\n");
		close(ns);
	}

	return;
}



/*==============================================================================
 * Function:    upnp_select()
 *------------------------------------------------------------------------------
 * Description: Process UPnP packets.
 *
 *
 *============================================================================*/
int upnp_select(void)
{
	UPNP_INTERFACE *ifp;
	UPNP_CONTEXT *context_upnp = &upnp_context;
	struct  timeval tv = {1, 0};    /* timed out every second */
	int n;
	fd_set  fds;
	
	
	FD_ZERO(&fds);
	
	/* Set select sockets */
	if (context_upnp->http_sock != -1)
		FD_SET(context_upnp->http_sock, &fds);
	
	ifp = context_upnp->iflist;
	while (ifp)
	{
		if (ifp->http_sock != -1)
			FD_SET(ifp->http_sock, &fds);
		
		ifp = ifp->next;
	}
	
	FD_SET(context_upnp->ssdp_sock, &fds);
	
	/* select sockets */
	n = select(FD_SETSIZE, &fds, (fd_set *)NULL, (fd_set *)NULL, &tv);
	if (n > 0)
	{
		/* process upnp_http */
		if (context_upnp->http_sock != -1 && FD_ISSET(context_upnp->http_sock, &fds))
		{
			upnp_http_accept(context_upnp->http_sock);
		}
		
		ifp = context_upnp->iflist;
		while (ifp)
		{
			if (ifp->http_sock != -1 && FD_ISSET(ifp->http_sock, &fds))
			{
				upnp_http_accept(ifp->http_sock);
			}
			
			ifp = ifp->next;
		}


		/* process ssdp */
		if (FD_ISSET(context_upnp->ssdp_sock, &fds))
		{
			context_upnp->fd = 0;
			if (upnp_cmdq_send(upnp_msqid, UPNP_SSDP_NEWFD, -1) == 0)
			{
				while (((volatile UPNP_CONTEXT *)context_upnp)->fd == 0)
						;
			}
		}


	}
	
	return 0;
}


/*==============================================================================
 * Function:    upnp_main()
 *------------------------------------------------------------------------------
 * Description: UPnP daemon routine
 *              (1) daemon initialization
 *              (2) enter the main loop
 *============================================================================*/
void upnp_main(void)
{
	UPNP_CONTEXT *context_upnp = &upnp_context;

	/* main loop */
	while (1)
	{
		upnp_flag = 0;

		/* initialize upnp */

		if (context_init(context_upnp) != 0)
		{
        		DBGPRINTF(RT_DBG_OFF, "upnp context init failed\n");
			return;
		}
		
		start_upnp();

#ifdef CONFIG_WPS
		if (!WPSInit())
                {
		        DBGPRINTF(RT_DBG_OFF, "WPSInit() failed !\n");
			return;
		}
#endif /* CONFIG_WPS */

		/* loop to process packets */
		while (upnp_flag == 0)
			upnp_select();
		
		/* start error, shutdown or restart all
		   go here. Anyway, do shutdown before
		   futher process */
		shutdown_upnp();

#ifdef CONFIG_WPS
                DBGPRINTF(RT_DBG_INFO, "wscSystemStop()...\n");
                wscSystemStop();		
#endif /* CONFIG_WPS */
		
		if (upnp_flag == UPNP_FLAG_SHUTDOWN)
			break;
		
                DBGPRINTF(RT_DBG_OFF, "UPnP Restarted\n");
	}
	
	/* gotta leave now */
        DBGPRINTF(RT_DBG_OFF, "UPnP daemon is shut down!\n");
}


//###############################################################
// External call in functions
//###############################################################
/*==============================================================================
 * Function:    upnp_timer_alarm()
 *------------------------------------------------------------------------------
 * Description: Notify upnp_daemon thread to handle timer alarm timers.
 *============================================================================*/
void upnp_timer_alarm(void)
{
	if (upnp_msqid == -1)
		return;
	
	upnp_cmdq_send(upnp_msqid, UPNP_TIMER_ALARM, -1);
	return;
}


/*==============================================================================
 * Function:    upnp_event_alarm()
 *------------------------------------------------------------------------------
 * Description: Notify upnp_daemon thread to handle event alarm timers.
 *============================================================================*/
void upnp_event_alarm(char *event_url)
{
	if (upnp_msqid == -1)
		return;
	
	upnp_cmdq_send(upnp_msqid, UPNP_EVENT_ALARM, (int)event_url);
	return;
}

