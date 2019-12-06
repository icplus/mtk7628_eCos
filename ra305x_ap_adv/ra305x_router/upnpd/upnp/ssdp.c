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
#endif /* CONFIG_WPS */

/*
 * method parsing lookup table
 */
static int ssdp_msearch(UPNP_CONTEXT *);
static int read_header(UPNP_CONTEXT *, struct upnp_method *);
 
struct upnp_method ssdp_methods[] =
{
        {"M-SEARCH ",   sizeof("M-SEARCH ")-1,  METHOD_MSEARCH,     ssdp_msearch},
        {0,             0,                      0,                  0           }
};

static struct upnp_state ssdp_fsm[] =
{
	{upnp_context_init,	0               },
	{read_header,		0               },
	{parse_method,		ssdp_methods    },
	{parse_uri,		0               },
	{parse_msghdr,		0               },
	{dispatch,		ssdp_methods    },
	{0,			0		},
};

/*==============================================================================
 * Function:    ssdp_send()
 *------------------------------------------------------------------------------
 * Description: Do socket send
 *
 *============================================================================*/
void ssdp_send(UPNP_CONTEXT *context)
{
	struct sockaddr_in dst;
	
	char *buf = context->head_buffer;
	int len = strlen (buf);
	
	/*
	 * if caller does not specify a unicast address, send multicast
	 * (239.255.255.250)
	 */
	if (context->dst == NULL)
	{
		dst.sin_family = AF_INET;
#ifdef BSD_SOCK
		dst.sin_len = sizeof(struct sockaddr_in);
#endif
		inet_aton(SSDP_ADDR, &dst.sin_addr);
		dst.sin_port = htons(SSDP_PORT);
	} else
		dst = *context->dst;
	
	/* Send packet */
	sendto(context->ssdp_sock, buf, len, 0, (struct sockaddr *)&dst, sizeof(dst));
	return;
}


/*==============================================================================
 * Function:    ssdp_response()
 *------------------------------------------------------------------------------
 * Description: Send out SSDP responses
 *
 *============================================================================*/
void ssdp_response(UPNP_CONTEXT *context, UPNP_ADVERTISE *advertise, int type)
{
	int len;
	unsigned long myip;
	char myaddr[sizeof("255.255.255.255:65535")];
	char time_buf[64];
	char *buf;
	char *p;
	char myipstr[sizeof("255.255.255.255:65535")];
	
	buf = context->head_buffer;
	myip = context->focus_ifp->ipaddr;

	sprintf(myipstr, "%lu.%lu.%lu.%lu",
		(myip >> 24) & 0xff,
		(myip >> 16) & 0xff,
		(myip >> 8) & 0xff,
		myip & 0xff);
        
	if (context->http_port != 80)
	{
		sprintf(myaddr, "%s:%d",
                                myipstr,
				context->http_port);
	}
	
	gmt_time(time_buf);    
	
	len = sprintf(buf, 
				"HTTP/1.1 200 OK\r\n"
				"CACHE-CONTROL: max-age=%d\r\n"
				"DATE: %s\r\n"
				"EXT: \r\n"
				"LOCATION: http://%s/%s\r\n"
				"SERVER: %s/%s, UPnP/1.0, %s/%s\r\n",
				context->adv_time,
				time_buf,
				myaddr, advertise->root_device_xml,
				upnp_os_name, upnp_os_ver, upnp_model_name, upnp_model_ver);
	
	p = buf + len;

	switch (type)
	{
	case MSEARCH_SERVICE:
		sprintf(p,
				"ST: urn:%s:service:%s:1\r\n"
				"USN: uuid:%s::urn:%s:service:%s:1\r\n"
				"\r\n",
				advertise->domain, advertise->name, advertise->uuid, advertise->domain, advertise->name);
		break;
	
	case MSEARCH_DEVICE:   
		sprintf(p,
				"ST: urn:%s:device:%s:1\r\n"
				"USN: uuid:%s::urn:%s:device:%s:1\r\n"
				"\r\n",
				advertise->domain,advertise->name, advertise->uuid, advertise->domain, advertise->name);
		break;
	
	case MSEARCH_UUID:
		sprintf(p,
				"ST: uuid:%s\r\n"
				"USN: uuid:%s\r\n"
				"\r\n",
				advertise->uuid, advertise->uuid);
		break;
	
	case MSEARCH_ROOTDEVICE:
		sprintf(p,
			   "ST: upnp:rootdevice\r\n"
			   "USN: uuid:%s::upnp:rootdevice\r\n"
			   "\r\n",
			   advertise->uuid);
		break;
	}

        DBGPRINTF(RT_DBG_INFO, "%s:\n%s\n", __FUNCTION__, context->head_buffer);
	
	/* send packet out */
	ssdp_send(context);
	return;
}


/*==============================================================================
 * Function:    ssdp_msearch_response()
 *------------------------------------------------------------------------------
 * Description: Respond to SSDP M-SEARCH messages
 *
 *============================================================================*/
void ssdp_msearch_response(UPNP_CONTEXT *context, char *name, int type)
{
        UPNP_ADVERTISE	*advertise = upnp_advertise_table;
	
	for (; advertise->name; advertise++)
	{
		/* check qualification function */
		switch (type)
		{
		case MSEARCH_ROOTDEVICE:
			if (advertise->type == ADVERTISE_ROOTDEVICE)
			{
				ssdp_response(context, advertise, MSEARCH_ROOTDEVICE);
				return;                
			}
			break;    
	   
		case MSEARCH_UUID:
			if (advertise->type != ADVERTISE_SERVICE && /* including ADVERTISE_ROOTDEVICE */
				strcmp(name, advertise->uuid) == 0)
			{
				ssdp_response(context, advertise, MSEARCH_UUID);
				return;
			}
			break;
			
		case MSEARCH_DEVICE:
			if (strcmp(name, advertise->name) == 0)
			{
				ssdp_response(context, advertise, MSEARCH_DEVICE);
			}
			break;
			
		case MSEARCH_SERVICE:
			if (strcmp(name, advertise->name) == 0)
			{
				ssdp_response(context, advertise, MSEARCH_SERVICE);
			}
			break;
		
		case MSEARCH_ALL:
			if (advertise->type == ADVERTISE_SERVICE)
			{	
				ssdp_response(context, advertise, MSEARCH_SERVICE);
			}
			else if (advertise->type == ADVERTISE_DEVICE)
			{
				ssdp_response(context, advertise, MSEARCH_DEVICE);
				ssdp_response(context, advertise, MSEARCH_UUID);
			}
			else if (advertise->type == ADVERTISE_ROOTDEVICE)
			{
				ssdp_response(context, advertise, MSEARCH_DEVICE);
				ssdp_response(context, advertise, MSEARCH_UUID);
				ssdp_response(context, advertise, MSEARCH_ROOTDEVICE);
			}
			break;
		}
	}
	
	return;
}


/*==============================================================================
 * Function:    ssdp_notify()
 *------------------------------------------------------------------------------
 * Description: Send SSDP notifications
 *
 *============================================================================*/
void ssdp_notify(UPNP_CONTEXT *context, UPNP_ADVERTISE *advertise, int adv_type, int ssdp_type)
{
	int len;
	char *buf;
	char *p; 
		
	buf = context->head_buffer;
	
	if (ssdp_type == SSDP_ALIVE)
	{
		char myaddr[sizeof("255.255.255.255:65535")];
		unsigned long myip;
		char myipstr[sizeof("255.255.255.255:65535")];
		
		myip = context->focus_ifp->ipaddr;

        	sprintf(myipstr, "%lu.%lu.%lu.%lu",
        		(myip >> 24) & 0xff,
        		(myip >> 16) & 0xff,
        		(myip >> 8) & 0xff,
        		myip & 0xff);
                
        	if (context->http_port != 80)
        	{
        		sprintf(myaddr, "%s:%d",
                                        myipstr,
        				context->http_port);
        	}
				
		len = sprintf(buf, 
					"NOTIFY * HTTP/1.1\r\n"
					"Host: 239.255.255.250:1900\r\n"
					"Cache-Control: max-age=%d\r\n"
					"Location: http://%s/%s\r\n"
					"NTS: ssdp:alive\r\n"
					"Server: %s/%s UPnP/1.0 %s/%s\r\n",
					context->adv_time,
					myaddr,
					advertise->root_device_xml,
					upnp_os_name,
					upnp_os_ver,
					upnp_model_name,
					upnp_model_ver);
	}            
	else
	{
		/* SSDP_BYEBYE */
		len = sprintf(buf, 
					"NOTIFY * HTTP/1.1\r\n"
					"Host: 239.255.255.250:1900\r\n"
					"NTS: ssdp:byebye\r\n");
	}
	
	p = buf + len;
	
	switch (adv_type)
	{
	case ADVERTISE_SERVICE:
		sprintf(p,
				"NT: urn:%s:service:%s:1\r\n"
				"USN: uuid:%s::urn:%s:service:%s:1\r\n"
				"\r\n",
				advertise->domain, advertise->name, advertise->uuid, advertise->domain, advertise->name);
		break;
	
	case ADVERTISE_DEVICE:   
		sprintf(p,
				"NT: urn:%s:device:%s:1\r\n"
				"USN: uuid:%s::urn:%s:device:%s:1\r\n"
				"\r\n",
				advertise->domain, advertise->name, advertise->uuid, advertise->domain, advertise->name);
		break;
	
	case ADVERTISE_UUID:
		sprintf(p,
				"NT: uuid:%s\r\n"
				"USN: uuid:%s\r\n"
				"\r\n",
				advertise->uuid, advertise->uuid);
		break;
	
	case ADVERTISE_ROOTDEVICE:
		sprintf(p,
			   "NT: upnp:rootdevice\r\n"
			   "USN: uuid:%s::upnp:rootdevice\r\n"
			   "\r\n",
			   advertise->uuid);
		break;
	}
	
	/* Send packet out */
	if (ssdp_type == SSDP_ALIVE)
	{	
		ssdp_send(context);
	}
	else /* SSDP_BYEBYE */
	{
		ssdp_lpf_send(context);
	}
	
	return;
}


/*==============================================================================
 * Function:    ssdp_adv_process()
 *------------------------------------------------------------------------------
 * Description: Send SSDP advertisement messages
 *
 *============================================================================*/
void ssdp_adv_process(UPNP_CONTEXT *context, int ssdp_type)
{
	UPNP_ADVERTISE *advertise = upnp_advertise_table;

        DBGPRINTF(RT_DBG_INFO, "===> %s\n", __FUNCTION__);
	
	for (; advertise->name; advertise++)
	{
		/* check qualification function */
		switch (advertise->type)
		{
		case ADVERTISE_SERVICE:
			/* services */
			ssdp_notify(context, advertise, ADVERTISE_SERVICE,    ssdp_type);
			break;
			
		case ADVERTISE_DEVICE:
			/* devices */
			ssdp_notify(context, advertise, ADVERTISE_DEVICE,     ssdp_type);
			ssdp_notify(context, advertise, ADVERTISE_UUID,       ssdp_type);
			break;
			
		default:
			/* rootdevice */
			ssdp_notify(context, advertise, ADVERTISE_DEVICE,     ssdp_type);
			ssdp_notify(context, advertise, ADVERTISE_UUID,       ssdp_type);
			ssdp_notify(context, advertise, ADVERTISE_ROOTDEVICE, ssdp_type);
			break;
		}
	}

        DBGPRINTF(RT_DBG_INFO, "<=== %s\n", __FUNCTION__);
	
	return;
}


/*==============================================================================
 * Function:    ssdp_byebye()
 *------------------------------------------------------------------------------
 * Description: Send SSDP_BYEBYE message
 *
 *============================================================================*/
static inline void ssdp_byebye(UPNP_CONTEXT *context)
{
	ssdp_adv_process(context, SSDP_BYEBYE);
}


/*==============================================================================
 * Function:    ssdp_alive()
 *------------------------------------------------------------------------------
 * Description: Send SSDP_ALIVE message
 *
 *============================================================================*/
static inline void ssdp_alive(UPNP_CONTEXT *context)
{
	ssdp_adv_process(context, SSDP_ALIVE);
}


/*==============================================================================
 * Function:    ssdp_msearch()
 *------------------------------------------------------------------------------
 * Description: Parse SSDP discovery values
 *
 *============================================================================*/
int ssdp_msearch(UPNP_CONTEXT *context)
{
	char name[128];
	int  type;
	char *host;
	char *man;
	char *st;
	int pos;

        DBGPRINTF(RT_DBG_INFO, "===> %s \n", __FUNCTION__);
	
	/* check HOST:239.255.255.250:1900 */
	host = context->HOST;
	if (!host || strcmp(host, "239.255.255.250:1900") != 0)
        {       
                DBGPRINTF(RT_DBG_OFF, "Host is not match 239.255.255.250:1900\n");
		goto error_out;
        }
		
	/* check MAN:"ssdp:discover" */
	man = context->MAN;
	if (!man || strcmp(man, "\"ssdp:discover\"") != 0)
        {       
                DBGPRINTF(RT_DBG_OFF, "MAN is not match \"ssdp:discover\"\n");
		goto error_out;
        }
		
	/* process search target */
	st = context->ST;
	if (!st)
        {       
                DBGPRINTF(RT_DBG_OFF, "ST is NULL\n");
		goto error_out;
        }
		
	if (strcmp(st, "ssdp:all") == 0)
	{        
		type = MSEARCH_ALL;
	}
	else if (strcmp(st, "upnp:rootdevice") == 0)
	{   
		type = MSEARCH_ROOTDEVICE;
	}
	else if (memcmp(st, "urn:schemas-upnp-org:device:", 28) == 0)
	{
		/* device */
		type = MSEARCH_DEVICE;
	
		st += 28;
		pos = strcspn(st, ":");
		strncpy(name, st, pos);
		name[pos] = 0;
	}
	else if (memcmp(st, "urn:schemas-upnp-org:service:", 29) == 0)
	{
		/* service */
		type = MSEARCH_SERVICE;
	
		st += 29;
		pos = strcspn(st, ":");
		strncpy(name, st, pos);
		name[pos] = 0;
	}
#ifdef CONFIG_WPS
	else if (memcmp(st, "urn:schemas-wifialliance-org:device:", 36) == 0)
	{
		/* device */
		type = MSEARCH_DEVICE;
	
		st += 36;
		pos = strcspn(st, ":");
		strncpy(name, st, pos);
		name[pos] = 0;
	}
	else if (memcmp(st, "urn:schemas-wifialliance-org:service:", 37) == 0)
	{
		/* service */
		type = MSEARCH_SERVICE;
	
		st += 37;
		pos = strcspn(st, ":");
		strncpy(name, st, pos);
		name[pos] = 0;
	}
#endif /* CONFIG_WPS */
	else if (memcmp(st, "uuid:", 5) == 0)
	{
		/* uuid */
		type = MSEARCH_UUID;
	
		st += 5;
		strcpy(name, st);        
	}
	else
	{
		return -1;    
	}

	/* do M-SEARCH response */
	ssdp_msearch_response(context, name, type);

        DBGPRINTF(RT_DBG_INFO, "<=== %s \n", __FUNCTION__);
	return 0;

error_out:
        DBGPRINTF(RT_DBG_INFO, "Error<=== %s \n", __FUNCTION__);
        return -1;
}


/*==============================================================================
 * Function:    read_header()
 *------------------------------------------------------------------------------
 * Description: Read SSDP message
 *
 *============================================================================*/
static int read_header(UPNP_CONTEXT *context, struct upnp_method *methods)
{
	UPNP_INTERFACE *ifp = context->iflist;
	int size = sizeof (struct sockaddr_in);
	int len;

	memset (&context->dst_addr, 0, size);

	len = recvfrom(context->ssdp_sock, context->buf, sizeof(context->buf), 
				   0, (struct sockaddr *)&context->dst_addr, &size);// arg6 differ in signedness

	/* Read done */
	context->fd = context->ssdp_sock;		

	if (len > 0)
	{
		/* locate UPnP interface */
		while (ifp)
		{
			if ((ifp->ipaddr & ifp->netmask) == 
				(ntohl(context->dst_addr.sin_addr.s_addr) & ifp->netmask))
			{
				context->focus_ifp = ifp;
				context->dst = &context->dst_addr;
				break;
			}
			
			ifp = ifp->next;
		}

		if (ifp == NULL)
			return -1;

		context->buf[len] = 0;
		context->end = len;

                DBGPRINTF(RT_DBG_INFO, "%s:Message Size:%d, \n%s\n", __FUNCTION__, (context->end - context->index),  &context->buf[context->index]);

		/* Get message header */
		get_msghdr(context);
 
		if (context->status != 0)
			return -1;

		return 0;
	}

	return -1;
}


/*==============================================================================
 * Function:    ssdp_process()
 *------------------------------------------------------------------------------
 * Description: SSDP process entry
 *
 *============================================================================*/
void ssdp_process(UPNP_CONTEXT *context)
{
        DBGPRINTF(RT_DBG_INFO, "===> %s\n", __FUNCTION__);
	context->fd = context->ssdp_sock;

	upnp_http_engine(context, ssdp_fsm);

        DBGPRINTF(RT_DBG_INFO, "<=== %s\n", __FUNCTION__);
	return ;
}


/*==============================================================================
 * Function:    ssdp_timeout()
 *------------------------------------------------------------------------------
 * Description: SSDP advertising timer
 *
 *============================================================================*/
void ssdp_timeout(UPNP_CONTEXT *context, unsigned int now)
{
	unsigned int past;
	
	past = now - context->last_check_adv_time;
	if (past >= context->adv_time)
	{
        	UPNP_INTERFACE *ifp = context->iflist;
                
		while (ifp)
		{
			context->dst = NULL;
			ssdp_alive(context);
			ifp = ifp->next;
		}
		
		context->last_check_adv_time = now;
	}
}


/*==============================================================================
 * Function:    ssdp_add_multi()
 *------------------------------------------------------------------------------
 * Description: Setup IP level options for SSDP multicast networking
 *
 *============================================================================*/
int ssdp_add_multi(UPNP_CONTEXT *context)
{
	int ret;
	char optval;
	struct in_addr inaddr;
	struct ip_mreq mreq;
	
	UPNP_INTERFACE *ifp = context->focus_ifp;
	
	/* 
	 * receive multicast packets
	 */
	inaddr.s_addr = htonl(ifp->ipaddr);
	ret = setsockopt(context->ssdp_sock, IPPROTO_IP, IP_MULTICAST_IF,
					 &inaddr, sizeof(inaddr)); 
	if (ret) 
	{
                DBGPRINTF(RT_DBG_ERROR, "%s: set_if_sock() set IP_MULTICAST_IF failed\n", __FUNCTION__);
		return -1;
	}
	
	/*
	 * setup time-to-live
	 */
	optval = 4; /* Hop count */
	ret = setsockopt(context->ssdp_sock, IPPROTO_IP, IP_MULTICAST_TTL,
					 &optval, sizeof(optval));
	if (ret)  
	{
                DBGPRINTF(RT_DBG_ERROR, "%s: set_if_sock() set IP_MULTICAST_TTL failed\n", __FUNCTION__);
		return -1;
	}
	
	/*
	 * turn off multicast loopback
	 */
	optval = 0;
	ret = setsockopt(context->ssdp_sock, IPPROTO_IP, IP_MULTICAST_LOOP,
					 &optval, sizeof(optval)); 
	if (ret) 
	{
                DBGPRINTF(RT_DBG_ERROR, "%s: set_if_sock() set IP_MULTICAST_LOOP failed\n", __FUNCTION__);	
		return -1;
	}
	
	/*
	 * join SSDP multicast group 
	 */
	inet_aton(SSDP_ADDR,  &mreq.imr_multiaddr);
	mreq.imr_interface.s_addr = htonl(ifp->ipaddr);
	ret = setsockopt(context->ssdp_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
					 (char *)&mreq, sizeof(mreq));
	if (ret)
	{
                DBGPRINTF(RT_DBG_ERROR, "%s: set_if_sock() set IP_ADD_MEMBERSHIP failed\n", __FUNCTION__);	
		return -1;
	}
	
	/* Multicast group joined successfully */
	ifp->flag |= IFF_MJOINED;
	
	// Should we send message to another thread?
	/* send alive message */
	ssdp_alive(context);
	
	return 0;
}


/*==============================================================================
 * Function:    ssdp_del_multi()
 *------------------------------------------------------------------------------
 * Description: Shutdown SSDP interface; send SSDP_BYEBYE as required
 *
 *============================================================================*/
void ssdp_del_multi(UPNP_CONTEXT *context)
{
	struct ip_mreq mreq;
	UPNP_INTERFACE *ifp = context->focus_ifp;
	
	
	/* For ip changed or shutdown, we have to drop membership of multicast */
	if (ifp->flag & IFF_MJOINED)
	{
		/* XXX -- Send SSDP byebye in case IP address changed */
		ssdp_byebye(context);
		
		/* Give a chance for bye-bye to send out */
		upnp_thread_sleep(5);
		
		inet_aton (SSDP_ADDR,  &mreq.imr_multiaddr);
		mreq.imr_interface.s_addr = htonl(ifp->ipaddr);
		
		setsockopt (context->ssdp_sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *) &mreq, sizeof(mreq));
		ifp->flag &= ~IFF_MJOINED;
	}
	
	return;
}


/*==============================================================================
 * Function:    ssdp_shutdown()
 *------------------------------------------------------------------------------
 * Description: Close ssdp_sock
 *
 *============================================================================*/
void ssdp_shutdown(UPNP_CONTEXT *context)
{
	UPNP_INTERFACE *ifp;
	
	if (context->ssdp_sock != -1)
	{
		ifp = context->iflist;
		while (ifp)
		{
			context->focus_ifp = ifp;
			context->dst = 0;
			
			ssdp_del_multi(context);
			
			ifp = ifp->next;
		}
		
		// Close the ssdp socket
		close(context->ssdp_sock);
		context->ssdp_sock = -1;	    
	}
}


/*==============================================================================
 * Function:    ssdp_init()
 *------------------------------------------------------------------------------
 * Description: Initialize SSDP advertisement interval and open ssdp_sock
 *
 *============================================================================*/
int ssdp_init(UPNP_CONTEXT *context)
{
	UPNP_INTERFACE *ifp;
	int s;
	struct sockaddr_in sinaddr;
	int reuse = 1;
	
        DBGPRINTF(RT_DBG_INFO, "====> %s\n", __FUNCTION__);
	
	/*
	 * Initialize adv_seconds to avoid advertising at startup;
	 * the first advertisements should be sent when interfaces are added
	 */
	upnp_curr_time(&context->last_check_adv_time);
	
	/* create UDP socket */
	s = socket (PF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		return -1;
	
#ifdef BSD_SOCK
	if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0)
                DBGPRINTF(RT_DBG_OFF, "Cannot set socket option (SO_REUSEPORT)\n");
#else
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
                DBGPRINTF(RT_DBG_OFF, "Cannot set socket option (SO_REUSEPORT)\n");
#endif
	
	/* bind socket to recive discovery */
	memset((char *)&sinaddr, 0, sizeof(sinaddr));
	
	/* Only socket with ANY addr can accept multi-cast */
	sinaddr.sin_family = AF_INET;
#ifdef BSD_SOCK
	sinaddr.sin_len = sizeof(sinaddr);
#endif
	sinaddr.sin_addr.s_addr = INADDR_ANY;
	sinaddr.sin_port = htons (SSDP_PORT);
	
	if (bind(s, (struct sockaddr *)&sinaddr, sizeof (sinaddr)) < 0)
	{
                DBGPRINTF(RT_DBG_OFF, "ssdp::setsockopt: bind failed!\n");
		close(s);
		return -1;
	}
	
	// save ssdp socket
	context->ssdp_sock = s;
	
	// Bind Multicast to interface IP
	ifp = context->iflist;
	while (ifp)
	{
		context->focus_ifp = ifp;
		context->dst = 0;
		
		if (ssdp_add_multi(context) == -1)
			return -1;
		
		ifp = ifp->next;
	}
	
        DBGPRINTF(RT_DBG_INFO, "<==== %s\n", __FUNCTION__);
	
	return 0;
}
int ssdp_udp_init(UPNP_CONTEXT *context)
{
	UPNP_INTERFACE *ifp;
	int s;
	struct sockaddr_in sinaddr;
	int reuse = 1;
	
        DBGPRINTF(RT_DBG_INFO, "====> %s\n", __FUNCTION__);
	
	/*
	 * Initialize adv_seconds to avoid advertising at startup;
	 * the first advertisements should be sent when interfaces are added
	 */
	//upnp_curr_time(&context->last_check_adv_time);
	
	/* create UDP socket */
	s = socket (PF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		return -1;
	
#ifdef BSD_SOCK
	if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0)
                DBGPRINTF(RT_DBG_OFF, "Cannot set socket option (SO_REUSEPORT)\n");
#else
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
                DBGPRINTF(RT_DBG_OFF, "Cannot set socket option (SO_REUSEPORT)\n");
#endif
	
	/* bind socket to recive discovery */
	memset((char *)&sinaddr, 0, sizeof(sinaddr));
	
	/* Only socket with ANY addr can accept multi-cast */
	sinaddr.sin_family = AF_INET;
#ifdef BSD_SOCK
	sinaddr.sin_len = sizeof(sinaddr);
#endif
	sinaddr.sin_addr.s_addr = INADDR_ANY;
	sinaddr.sin_port = htons (3078);
	
	if (bind(s, (struct sockaddr *)&sinaddr, sizeof (sinaddr)) < 0)
	{
                DBGPRINTF(RT_DBG_OFF, "udp::setsockopt: bind failed!\n");
		close(s);
		return -1;
	}
	
	// save udp socket
	context->udp_sock = s;
	
	// Bind Multicast to interface IP
	ifp = context->iflist;
	while (ifp)
	{
		context->focus_ifp = ifp;
		context->dst = 0;
		
		if (udp_add_multi(context) == -1)
			return -1;
		
		ifp = ifp->next;
	}
	
        DBGPRINTF(RT_DBG_INFO, "<==== %s\n", __FUNCTION__);
	
	return 0;
}
int udp_add_multi(UPNP_CONTEXT *context)
{
	int ret;
	char optval;
	struct in_addr inaddr;
	struct ip_mreq mreq;
	
	UPNP_INTERFACE *ifp = context->focus_ifp;
	
	/* 
	 * receive multicast packets
	 */
	inaddr.s_addr = htonl(ifp->ipaddr);
	ret = setsockopt(context->ssdp_sock, IPPROTO_IP, IP_MULTICAST_IF,
					 &inaddr, sizeof(inaddr)); 
	if (ret) 
	{
                DBGPRINTF(RT_DBG_ERROR, "%s: set_if_sock() set IP_MULTICAST_IF failed\n", __FUNCTION__);
		return -1;
	}
	
	/*
	 * setup time-to-live
	 */
	optval = 4; /* Hop count */
	ret = setsockopt(context->ssdp_sock, IPPROTO_IP, IP_MULTICAST_TTL,
					 &optval, sizeof(optval));
	if (ret)  
	{
                DBGPRINTF(RT_DBG_ERROR, "%s: set_if_sock() set IP_MULTICAST_TTL failed\n", __FUNCTION__);
		return -1;
	}
	
	/*
	 * turn off multicast loopback
	 */
	optval = 0;
	ret = setsockopt(context->udp_sock, IPPROTO_IP, IP_MULTICAST_LOOP,
					 &optval, sizeof(optval)); 
	if (ret) 
	{
                DBGPRINTF(RT_DBG_ERROR, "%s: set_if_sock() set IP_MULTICAST_LOOP failed\n", __FUNCTION__);	
		return -1;
	}
	
	/*
	 * join UDP multicast group 
	 */
	inet_aton(SSDP_ADDR,  &mreq.imr_multiaddr);
	mreq.imr_interface.s_addr = htonl(ifp->ipaddr);
	ret = setsockopt(context->udp_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
					 (char *)&mreq, sizeof(mreq));
	if (ret)
	{
                DBGPRINTF(RT_DBG_ERROR, "%s: set_if_sock() set IP_ADD_MEMBERSHIP failed\n", __FUNCTION__);	
		return -1;
	}
	
	/* Multicast group joined successfully */
	ifp->flag |= IFF_MJOINED;
	
	// Should we send message to another thread?
	/* send alive message */
	//ssdp_alive(context);
	
	return 0;
}