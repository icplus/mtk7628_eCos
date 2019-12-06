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
#include "wsc/wsc_upnp_device.h"

extern int WscDevHandleSubscriptionReq(char *req_SID, unsigned long ipaddr);
extern int WscDevCPNodeRemove(char *SID);

#endif /* CONFIG_WPS */

/*
 * Variables
 */
static  unsigned int    unique_id_count = 1;

extern cyg_mutex_t upnp_service_table_mutex;

/*==============================================================================
 * Function:    delete_subscriber()
 *------------------------------------------------------------------------------
 * Description: remove subscriber from list
 *
 *============================================================================*/
static void delete_subscriber(UPNP_SERVICE *service, UPNP_SUBSCRIBER *subscriber)
{
        if (subscriber)
        {
	        cyg_scheduler_lock();
        	if (subscriber->prev) 
        	{
        		/* remove from queue */
        		subscriber->prev->next = subscriber->next;
        	}
        	else
        	{    
        		/* first node, reconfigure the list pointer */
        		service->subscribers = subscriber->next;
        	}
        	
        	if (subscriber->next) 
        		subscriber->next->prev = subscriber->prev;

#ifdef CONFIG_WPS
        	/*  Remove this Control Point from WscCPList*/
        	DBGPRINTF(RT_DBG_ERROR, "%s: Remove from WscCPList sid=%s\n", __FUNCTION__, subscriber->sid);
        	WscDevCPNodeRemove((char *)(subscriber->sid));
#endif /* CONFIG_WPS */	
			//diag_printf("[delete_subscriber]subscriber:%x",subscriber);
        	free(subscriber);
        	cyg_scheduler_unlock();
        }

	return;
}


/*==============================================================================
 * Function:    parse_callback()
 *------------------------------------------------------------------------------
 * Description: Parse callback to get host address (ip, port) and uri
 *
 *============================================================================*/
char *parse_callback(char *callback, unsigned long *ipaddr, unsigned short *port)
{
	char *p;
	int pos;
	
	char host[sizeof("255.255.255.255:65535")];
	char *uri;
	
	int host_port;
	struct in_addr host_ip;
	
	/* callback: "<http://192.168.2.13:5000/notify>" */
	if (memcmp(callback, "<http://", 8) != 0)
	{
		/* not a HTTP URL, return NULL pointer */
		return 0;
	}
	
	/* make <http:/'\0'192.168.2.13:5000/notify'\0' */
	pos = strcspn(callback, ">");
	callback[pos] = 0;
	
	callback[7] = 0;
	
	/* Locate uri */
	p = callback + 8;
	pos = strcspn(p, "/");
	if (pos > sizeof(host)-1)
		return 0;
	
	if (p[pos] != '/')
		uri = callback + 6;
	else
		uri = p + pos; 
	
	strncpy(host, p, pos);
	host[pos] = 0;
	
	/* make port */
	host_port = 0;
	
	pos = strcspn(host, ":");
	if (host[pos] == ':')
		host_port = atoi(host + pos + 1);
	
	if (host_port > 65535)
		return 0;
	
	if (host_port == 0)
		host_port = 80;
	
	/* make ip */
	host[pos] = 0;
	if (inet_aton(host, &host_ip) == 0)
		return 0;
	
	*ipaddr = ntohl(host_ip.s_addr);
	*port = host_port;
	return uri;
}


/*==============================================================================
 * Function:    gena_read_sock()
 *------------------------------------------------------------------------------
 * Description: Consume the input buffer after sending the notification
 *
 *============================================================================*/
static int gena_read_sock(int s, char *buf, int len, int flags)
{
	long     rc;
	fd_set   ibits;
	struct   timeval tv = {0, 5000};   /* wait for at most 1 seconds */
	int      n_read;
	
	FD_ZERO(&ibits);
	FD_SET(s, &ibits);
	
	rc = select(s+1, &ibits, 0, 0, &tv);
	if (rc <= 0)
		return -1;
			
	if (FD_ISSET(s, &ibits))
	{
		n_read = recv(s, buf, len, flags);
		return n_read;
	}
	
	return -1;
}


/*==============================================================================
 * Function:    notify_prop_change()
 *------------------------------------------------------------------------------
 * Description: Send out property event changes message
 *
 *============================================================================*/
void notify_prop_change(UPNP_CONTEXT *context, UPNP_SUBSCRIBER *subscriber)
{
	struct sockaddr_in sockaddr;
	
	int s;
	int len;
	int on;
	
	// Fill socket address to connect
	memset(&sockaddr, 0, sizeof(sockaddr));
	
	sockaddr.sin_addr.s_addr = htonl(subscriber->ipaddr);
	sockaddr.sin_port = htons(subscriber->port);
	sockaddr.sin_family = AF_INET;
#ifdef BSD_SOCK
	sockaddr.sin_len = sizeof(sockaddr);
#endif
	
	/* open socket */
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		goto error_out;
	
	/* set non-blocking IO to have connect() return without pending */
	on = 1;
	if ((ioctl(s, FIONBIO, (int)&on)) == -1)
	{
		diag_printf( "cannot set socket as non-blocking IO\n");
		goto error_out;
	}
	
	/* connect */
	if (connect(s, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
	{ 
		struct timeval tv = {0, 20000};
		fd_set fds;
		int i;
		
		FD_ZERO(&fds);
		FD_SET(s, &fds);
		
		// timeout after 3 seconds
#define	GENA_NOTIFY_SELECT_MAX	25
		for (i=0; i<GENA_NOTIFY_SELECT_MAX; i++)
		{
			if (select(s+1, 0, &fds, 0, &tv) > 0)
			{
				if (FD_ISSET(s, &fds))
					break;
			}
		}
		
		if (i == GENA_NOTIFY_SELECT_MAX)
		{
			subscriber->retry_count += 1;
			DBGPRINTF(RT_DBG_INFO, "Notify connect failed! subscriber->ipaddr=%x, retry=%d\n", subscriber->ipaddr, subscriber->retry_count);
			goto error_out;
		} else
                        subscriber->retry_count = 0;
	}
	
	/* set non-blocking IO to have connect() return without pending */
	on = 0;
	if ((ioctl(s, FIONBIO, (int)&on)) == -1)
	{
		diag_printf("cannot set socket as blocking IO");
		goto error_out;
	}
	
	// Send message out
	len = strlen(context->head_buffer);
	if (send(s, context->head_buffer, len, 0) == -1)
	{
		subscriber->retry_count += 1;
		DBGPRINTF(RT_DBG_INFO, "Notify failed! subscriber->ipaddr=%x, retry=%d\n", subscriber->ipaddr, subscriber->retry_count);
		goto error_out;
	} else {
	        subscriber->retry_count = 0;
	        DBGPRINTF(RT_DBG_PKT, "Notify Success\n");
        }
	
	// try to read response
	//gena_read_sock(s, context->head_buffer, GENA_MAX_BODY, 0);
	
error_out:
	if (s >= 0)
	{
		close(s);
	}
	
	return;
}


/*==============================================================================
 * Function:    submit_prop_event_message()
 *------------------------------------------------------------------------------
 * Description: Construct property event changes message
 *
 *============================================================================*/
void submit_prop_event_message(UPNP_CONTEXT *context, UPNP_SERVICE *service, UPNP_SUBSCRIBER *subscriber)
{
	UPNP_STATE_VAR *statevar = service->event_var_list;
	UPNP_VALUE value;
	
	char host[sizeof("255.255.255.255:65535")];
	char errmsg[SOAP_MAX_ERRMSG];
	char *p;
	int len;
	
	
	/* construct body */
	p = context->body_buffer;
	len = sprintf(context->body_buffer, "<e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\">\r\n");
	p += len;
	
	while (statevar)
	{
		if (statevar->evented->init == FALSE)
		{
			errmsg[0] = 0;
			// Set up converting buffer
			if ((*statevar->func)(service, &value, errmsg) == 0)
			{
				translate_value(statevar->type, &value);
				strcpy(statevar->evented->value, value.str);
				statevar->evented->init = TRUE;
			}
		}
		
		if (subscriber->seq == 0)
		{
			/* new subscription */
			len = sprintf(p, "<e:property><e:%s>%s</e:%s></e:property>\r\n",
						  statevar->name,
						  statevar->evented->value,
						  statevar->name);
			p += len;
		}
		else if (statevar->evented->changed)
		{
			/* state changed */
			len = sprintf(p, "<e:property><e:%s>%s</e:%s></e:property>\r\n",
						  statevar->name,
						  statevar->evented->value,
						  statevar->name);
			p += len;
		}
		
		statevar = statevar->next;
	}
	
	strcat(context->body_buffer, "</e:propertyset>\r\n\r\n");
	
	
	/* construct header */
	/* make host */
	len = sprintf(host, "%ld.%ld.%ld.%ld",
						(subscriber->ipaddr >> 24) & 0xff,
						(subscriber->ipaddr >> 16) & 0xff,
						(subscriber->ipaddr >> 8) & 0xff,
						subscriber->ipaddr & 0xff);
			
	if (subscriber->port != 80)
	{
		sprintf(host+len, ":%d", subscriber->port);
	}
	
	/* Make notify string */
	sprintf(context->head_buffer, "NOTIFY %s HTTP/1.1\r\n"
                                        "Host: %s\r\n"
					"Content-Type: text/xml\r\n"
					"Content-Length: %d\r\n"
					"NT: upnp:event\r\n"
					"NTS: upnp:propchange\r\n"
					"SID: %s\r\n"
					"SEQ: %d\r\n"
					"Connection: close\r\n\r\n"
					"%s",
					subscriber->uri,
					host,
					(int)strlen(context->body_buffer),
					subscriber->sid,
					subscriber->seq,
					context->body_buffer);
	
	// Send out
	notify_prop_change(context, subscriber);
	return;
}


/*==============================================================================
 * Function:    find_event()
 *------------------------------------------------------------------------------
 * Description: Search GENA event lists for a target event
 *
 *============================================================================*/
UPNP_SERVICE *find_event(char *event_url)
{
	UPNP_SERVICE *service = upnp_service_table;
	
	for (; service->event_url; service++)
	{
		if (strcmp(service->event_url, event_url) == 0)
			return service;
	}
	
	return NULL;
}


/*==============================================================================
 * Function:    gena_notify()
 *------------------------------------------------------------------------------
 * Description: Submit property events to subscribers
 *
 *============================================================================*/
void gena_notify(UPNP_CONTEXT *context, UPNP_SERVICE *service, char *sid) 
{
	UPNP_SUBSCRIBER *subscriber;
	UPNP_SUBSCRIBER *next_subscriber;
	time_t now;

	upnp_curr_time(&now);

	/* walk through the subscribers */
	for (subscriber = service->subscribers; subscriber != NULL; subscriber = next_subscriber) {
		next_subscriber = subscriber->next;
	
		/*  if the subscriber had already expired, delete it  */
		if ((subscriber->expire_time && (now > subscriber->expire_time))
                         || (subscriber->retry_count > 30)) {
			DBGPRINTF(RT_DBG_ERROR, "%s: Deleter subscriber\n", __FUNCTION__);
			delete_subscriber(service, subscriber);
			continue;
		}
		
		if (sid)
		{
			if (strcmp(sid, subscriber->sid) == 0)
			{ 
				/* for specific URL */
				submit_prop_event_message(context,service, subscriber);
				subscriber->seq++;
				break;
			}
		}
		else
		{
			/* for all */
			submit_prop_event_message(context, service, subscriber);
			subscriber->seq++;
		}
	}
	
	return;
}

/*==============================================================================
 * Function:    gena_notify_complete()
 *------------------------------------------------------------------------------
 * Description: Completion function after gena_notify
 *
 *============================================================================*/
void gena_notify_complete(UPNP_SERVICE *service)
{
	UPNP_STATE_VAR *statevar;
	
	statevar = service->event_var_list;
	while (statevar)
	{
		if (statevar->evented->changed == TRUE)
			statevar->evented->changed = FALSE;
		
		statevar= statevar->next;
	}
	
	service->evented = FALSE;
	return;
}

/*==============================================================================
 * Function:    gena_update_event_var()
 *------------------------------------------------------------------------------
 * Description: Process statevar value change
 *
 *============================================================================*/
int gena_update_event_var(UPNP_SERVICE *service, char *name, char *value)
{
	UPNP_STATE_VAR *statevar;
	
	if (service == 0)
		return FALSE;
	
	/* find statevar */
	statevar = service->event_var_list;
	
	while (statevar)
	{
		if (strcmp(name, statevar->name) == 0)
		{
			if (strncmp(value, statevar->evented->value, UPNP_EVENTED_VALUE_SIZE) == 0)
				return FALSE;
					
			strncpy(statevar->evented->value, value, UPNP_EVENTED_VALUE_SIZE);
                        statevar->evented->value[strlen(value)] = '\0';
			statevar->evented->changed = TRUE;
			
			/* Tell SOAP to send notification */
			service->evented = TRUE;
			return TRUE;
		}
		
		statevar= statevar->next;
	}
	
	return FALSE;
}


/*==============================================================================
 * Function:    unsubscribe()
 *------------------------------------------------------------------------------
 * Description: GENA unusubscription process routine
 *
 *============================================================================*/
int unsubscribe(UPNP_CONTEXT *context)
{
	UPNP_SERVICE *service;
	UPNP_SUBSCRIBER *subscriber;
	char *gena_sid = context->SID;

	cyg_mutex_lock(&upnp_service_table_mutex);
	/* find event */
	service = find_event(context->url);
	if (service == 0)
	{	
		cyg_mutex_unlock(&upnp_service_table_mutex);
		return R_NOT_FOUND;
	}
	
	/* find SID */
	subscriber = service->subscribers;
	while (subscriber)
	{
		if (strcmp(subscriber->sid, gena_sid) == 0)
			break;
		
		subscriber = subscriber->next;
	}
	
	if (!subscriber)
	{
		cyg_mutex_unlock(&upnp_service_table_mutex);
		return R_PRECONDITION_FAIL;
	}
	
	delete_subscriber(service, subscriber);
	
	cyg_mutex_unlock(&upnp_service_table_mutex);
	
	/* send reply */
	strcpy(context->head_buffer,
		"HTTP/1.1 200 OK\r\n"
		"Connection: close\r\n"
		"\r\n");
	
	if (send(context->fd, context->head_buffer, strlen(context->head_buffer), 0) == -1)
                return -1;
                
	
	return 0;
}


/*==============================================================================
 * Function:    get_unique_id()
 *------------------------------------------------------------------------------
 * Description: Get a unique id string
 *
 *============================================================================*/
int get_unique_id(char *unique_id, unsigned int size)
{
	time_t curr_time;
	unsigned long	pid = upnp_pid();
	
	
	if (size < 8)
	{
		return FALSE;
	}
	 
	/* increment the counter */
	unique_id_count++;
	 
	/* get current time */
	time(&curr_time);
	 
	if (size < 17)
	{
		sprintf(unique_id, "%lx", (u_long)curr_time);
	}
	else if (size < 26)
	{
		sprintf(unique_id, "%lx-%lx", (u_long)curr_time, (u_long)pid);
	}
	else
	{
		sprintf(unique_id, "%lx-%lx-%lx", (u_long)curr_time, (u_long)pid, (u_long)unique_id_count);
	}
	
	return TRUE;
}


/*==============================================================================
 * Function:    subscribe()
 *------------------------------------------------------------------------------
 * Description: GENA subscription process routine
 *
 *============================================================================*/
int subscribe(UPNP_CONTEXT *context)
{
	UPNP_SERVICE *service;
	UPNP_SUBSCRIBER *subscriber;
	int infinite = FALSE;
	int interval = context->sub_time; 
	time_t now;
	char time_buf[64];
	char timeout[64];	
	char *gena_timeout  = context->TIMEOUT;
	char *gena_sid      = context->SID;
	char *gena_callback = context->CALLBACK;
	
	
	/*
	 * process subscription time interval
	 */
	if (gena_timeout)
	{
		char *ptr;
		
		if (memcmp(gena_timeout, "Second-", 7) != 0)
			return R_PRECONDITION_FAIL;
			
		ptr = gena_timeout + 7;
		if (strcmp(ptr, "infinite") == 0)
		{
			infinite = TRUE;
		}
		else
		{
			interval = atoi(ptr);
			if (interval == 0)
				interval = context->sub_time;
		}
	}
	else
	{
		/* no TIMEOUT header */
		sprintf(timeout, "Second-%d", context->sub_time);
		gena_timeout = timeout; 
	}

	cyg_mutex_lock(&upnp_service_table_mutex);
	/* find event */
	service = find_event(context->url);
	if (service == NULL)
	{
		cyg_mutex_unlock(&upnp_service_table_mutex);
		DBGPRINTF(RT_DBG_ERROR, "%s Service is not found\n", __FUNCTION__);	
		return R_NOT_FOUND;
	}
	
	/*
	 * process SID and Callback
	 */
	
	/* new subscription */    
	if (gena_sid == NULL) 
	{
		unsigned long ipaddr;
		unsigned short port;
		char *uri;
		int len;

		uri = parse_callback(gena_callback, &ipaddr, &port);
		if (uri == NULL)
		{
			cyg_mutex_unlock(&upnp_service_table_mutex);
			return R_ERROR;
		}	
		//
		subscriber = service->subscribers;
		while (subscriber)
		{
			if (subscriber->ipaddr == ipaddr &&
				subscriber->port == port &&
				strcmp(subscriber->uri, uri) == 0)
			{
				delete_subscriber(service, subscriber);
				break;	
			}
			
			subscriber = subscriber->next;
		}
			
		/*
		 * There may be multiple subscribers for the same
		 * callback, create a new subscriber anyway
		 */
		len = sizeof(*subscriber) + strlen(uri) + 1;
		subscriber = (UPNP_SUBSCRIBER *)calloc(1, len);
		if (subscriber == NULL)
		{
			cyg_mutex_unlock(&upnp_service_table_mutex);
			return R_ERROR;
		}	
		//if(subscriber < 0x802e3e40)
		//	diag_printf("\n[subscribe]subscriber:%x\n",subscriber);
		// Setup subscriber data
		subscriber->ipaddr = ipaddr;
		subscriber->port = port;
                subscriber->retry_count = 0;

		subscriber->uri = (char *)(subscriber + 1);
		strcpy(subscriber->uri, uri);
		
		strcpy(subscriber->sid, "uuid:");
		get_unique_id(subscriber->sid+5, sizeof(subscriber->sid)-5-1);
		
		/* insert queue */		
		cyg_scheduler_lock();
 		subscriber->next = service->subscribers;
		if (service->subscribers)
			service->subscribers->prev = subscriber;
		
		service->subscribers = subscriber;
		cyg_scheduler_unlock();

		/* set sequence number */
		subscriber->seq = 0;
#ifdef CONFIG_WPS//any subscriber should into WSC??? why, if WAN LAN device,it's a problem????
		/* Insert this Control Point into WscCPList */
		WscDevHandleSubscriptionReq(subscriber->sid, subscriber->ipaddr);
#endif /* CONFIG_WPS */
	}
	else
	{
		subscriber = service->subscribers;
		while (subscriber)
		{
			if (strcmp(subscriber->sid, gena_sid) == 0)
				break;
			
			subscriber = subscriber->next;
		}
		
		if (!subscriber)
		{
			cyg_mutex_unlock(&upnp_service_table_mutex);
			return R_PRECONDITION_FAIL;
		}	
	}
	
	/* update expiration time */
	if (infinite)
	{
		subscriber->expire_time = 0;
	}
	else
	{
		upnp_curr_time(&now);
		
		subscriber->expire_time = now + interval;
		
		if (subscriber->expire_time == 0)
			subscriber->expire_time = 1;
	}

	cyg_mutex_unlock(&upnp_service_table_mutex);
	
	/* send reply */
	gmt_time(time_buf);

	cyg_mutex_lock(&upnp_service_table_mutex);
	sprintf(context->head_buffer,
			"HTTP/1.1 200 OK\r\n"
			"Server: %s/%s UPnP/1.0 %s/%s\r\n"
			"Date: %s\r\n"
			"SID: %s\r\n"
			"Timeout: %s\r\n"
			"Connection: close\r\n"
			"\r\n",
			upnp_os_name, 
			upnp_os_ver,
			upnp_model_name,
			upnp_model_ver,
			time_buf,
			subscriber->sid,
			gena_timeout);
	cyg_mutex_unlock(&upnp_service_table_mutex);
	
	if (send(context->fd, context->head_buffer, strlen(context->head_buffer), 0) == -1)
                return -1;

	cyg_mutex_lock(&upnp_service_table_mutex);
	/* send initial property change notifications if first subscription */
	if (subscriber->seq == 0)
	{            
		DBGPRINTF(RT_DBG_ERROR, "%s Gena_notify\n", __FUNCTION__);
		gena_notify(context, service, subscriber->sid);
	}
	cyg_mutex_unlock(&upnp_service_table_mutex);
	
	return 0;
}


/*==============================================================================
 * Function:    gena_process()
 *------------------------------------------------------------------------------
 * Description: GENA process entry
 *
 *============================================================================*/
int gena_process(UPNP_CONTEXT *context)
{
	char *nt = context->NT;
	char *sid = context->SID;
	char *callback = context->CALLBACK;
        int status;

        DBGPRINTF(RT_DBG_INFO, "===> %s\n", __FUNCTION__);	
	if (context->method == METHOD_SUBSCRIBE)
	{
		/*
		 * process Callback/NT and SID
		 * Callback/NT and SID are mutual-exclusive
		 */
		if (!sid)
		{
			if (!nt || strcmp(nt, "upnp:event") != 0 || !callback)
			{
			        status = R_BAD_REQUEST;
                                goto EXIT;
			}
		}
		else
		{
			if (nt || callback)
			{
			        status = R_BAD_REQUEST;
                                goto EXIT;
			}
		}

                status = subscribe(context);
                DBGPRINTF(RT_DBG_ERROR, "%s: subscribe, status=%d\n", __FUNCTION__, status);
                goto EXIT;
	}
	else
	{
		if (!sid)
		{
                        status = R_PRECONDITION_FAIL;
                        goto EXIT;
		}
		else
		{
			if (nt || callback)
                        {
                                status = R_BAD_REQUEST;
                                goto EXIT;
                        }
		}
		
                status = unsubscribe(context);
                DBGPRINTF(RT_DBG_ERROR, "%s: unsubscribe, status=%d\n", __FUNCTION__, status);
                goto EXIT;
	}
        
EXIT:        
        DBGPRINTF(RT_DBG_INFO, "<=== %s\n", __FUNCTION__);	
        return status;
}


/*==============================================================================
 * Function:    check_expire()
 *------------------------------------------------------------------------------
 * Description: Check and remove expired subscribers
 *
 *============================================================================*/
void check_expire(void)
{
	UPNP_SERVICE *service = upnp_service_table;
	UPNP_SUBSCRIBER *subscriber;
	UPNP_SUBSCRIBER *temp;
	time_t now;
	
        DBGPRINTF(RT_DBG_INFO, "===> %s\n", __FUNCTION__);
		
 	cyg_mutex_lock(&upnp_service_table_mutex);
	upnp_curr_time(&now);
	
	for (; service->event_url; service++)
	{
		subscriber = service->subscribers;
		while (subscriber)
		{
			temp = subscriber->next;
			
			if (subscriber->expire_time && 
				(now > subscriber->expire_time))
			{
				delete_subscriber(service, subscriber);
			}
			
			subscriber = temp;
		}
	}
	cyg_mutex_unlock(&upnp_service_table_mutex);
        DBGPRINTF(RT_DBG_INFO, "<=== %s\n", __FUNCTION__);
}


/*==============================================================================
 * Function:    gena_timeout()
 *------------------------------------------------------------------------------
 * Description: GENA subscription check timer
 *
 *============================================================================*/
void gena_timeout(UPNP_CONTEXT *context, unsigned int now)
{
	unsigned int past;
	
	past = now - context->last_check_gena_time;
	if (past >= GENA_TIMEOUT)
	{
		/* timeout subscriptions */
		check_expire();
		
		/* update check time */
		context->last_check_gena_time = now;
	}
}


/*==============================================================================
 * Function:    gena_event_alarm()
 *------------------------------------------------------------------------------
 * Description: Process statevar value change
 *
 *============================================================================*/
void gena_event_alarm(UPNP_CONTEXT *context, char *event_url)
{
	UPNP_SERVICE *service;
	UPNP_STATE_VAR *statevar;

	cyg_mutex_lock(&upnp_service_table_mutex);

	service = find_event(event_url);
	if (service == NULL)
	{
		cyg_mutex_unlock(&upnp_service_table_mutex); 
		return;
	}	
	
	statevar = service->event_var_list;
	
	while (statevar)
	{
		statevar->evented->init = FALSE;
		statevar->evented->changed = TRUE;
		
		statevar= statevar->next;
	}
	
	gena_notify(context, service, NULL);
	gena_notify_complete(service);

	cyg_mutex_unlock(&upnp_service_table_mutex); 
	
	return;
}

/*==============================================================================
 * Function:    init_event_vars()
 *------------------------------------------------------------------------------
 * Description: Initialize Gena event and prepend to event list
 *
 *============================================================================*/
void init_event_vars(UPNP_SERVICE *service) 
{
	/* construct event_var_list */
	UPNP_STATE_VAR *state_var = service->statevar_table;
	
	while (state_var->name)
	{
		if (state_var->evented)
		{
			state_var->evented->init = FALSE;
			state_var->evented->changed = FALSE;
			
			/* do prepend */
			state_var->next = service->event_var_list;
			service->event_var_list = state_var;
		}
		
		state_var++;
	}
	
	return;
}


/*==============================================================================
 * Function:    gena_shutdown()
 *------------------------------------------------------------------------------
 * Description: Release GENA event sources
 *
 *============================================================================*/
void gena_shutdown(UPNP_CONTEXT *context)
{
	UPNP_SERVICE *service;
	cyg_mutex_lock(&upnp_service_table_mutex);
	for (service = upnp_service_table; service->event_url; service++)
	{
		while (service->subscribers)
			delete_subscriber(service, service->subscribers);
			
		service->event_var_list = 0;
	}
	cyg_mutex_unlock(&upnp_service_table_mutex);
}


/*==============================================================================
 * Function:    gena_init()
 *------------------------------------------------------------------------------
 * Description: Initialize GENA subscription time and event sources
 *
 *============================================================================*/
int gena_init(UPNP_CONTEXT *context)
{
	UPNP_SERVICE *service;
	
        DBGPRINTF(RT_DBG_INFO, "====> %s\n", __FUNCTION__);

	/* initilize subscription time interval */
	upnp_curr_time(&context->last_check_gena_time);

	cyg_mutex_lock(&upnp_service_table_mutex);
	/* initialize event sources */
	for (service = upnp_service_table; service->event_url; service++)
	{
		init_event_vars(service);
	}
	cyg_mutex_unlock(&upnp_service_table_mutex);
	
        DBGPRINTF(RT_DBG_INFO, "<==== %s\n", __FUNCTION__);
	
	return 0;
}

