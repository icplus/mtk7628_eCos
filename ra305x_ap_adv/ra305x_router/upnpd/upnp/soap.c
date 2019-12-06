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
#include "wsc_upnp_device.h"
#include "wsc_common.h"
#include "wsc_netlink.h"
#endif /* CONFIG_WPS */

extern int wsc_msg_sync_id;
extern cyg_mutex_t upnp_service_table_mutex;

/*==============================================================================
 * Function:    strtok_n()
 *------------------------------------------------------------------------------
 * Description: Return token before and character pointer after the delimiter
 *
 *============================================================================*/
static char *strtok_n(char *s, char *delim, char **endp)
{
	char *p = s;
	char *token = NULL;
	
	
	/* pre-condition */    
	*endp = NULL;
	
	if (p == NULL)
		return NULL;
	
	/* consume leading delimiters */
	while (*p)
	{
		if (strchr(delim, *p) == NULL)
		{
			token = p;
			break;
		}
	
		*p++ = 0;
	}
	
	if (*p == 0)
		return NULL;
		
	/* make this token */
	while (*p)
	{
		if (strchr(delim, *p) != NULL)
			break;
		
		p++;
	}
	
	if (*p != 0)
	{
		*p++ = 0;
	
		/* consume tailing delimiters */
		while (*p)
		{
			if (strchr(delim, *p) == NULL)
				break;
		
			*p++ = 0;
		}
	
		if (*p)
			*endp = p;
	}
	
	return token;
}


/*==============================================================================
 * Function:    parse_element()
 *------------------------------------------------------------------------------
 * Description: Parse elments in an XML file
 *
 *============================================================================*/
static char *parse_element(char *str, char **tag, char **name, char **attr, char **next)
{
	char *ptr = str;
	char *start;
	char *element;
	char *aux;
	char *value;
	char *value_end;
	char *next_element;
	
	/* eat leading white space */
	while (*ptr &&
		   (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n'))
	{
		ptr++;
	}
		
	if (*ptr != '<')
		return NULL;
		
	/* locate '<' and '>' */
	start = ptr;
	
	if (strtok_n(start, ">", &value) == NULL)
		return NULL;
	
	/* locate attribute */
	strtok_n(start, " \t\r\n", &aux);
	
	/* parse "<s:element" and convert to "/s:element" */
	ptr = strchr(start, ':');
	if (!ptr)
	{
		element = start+1;
	}
	else
	{
		element = ptr+1;
	}
	
	*start = '/';
	
	/*
	 * locate "</s:element>";
	 * sometimes, XP forgets to send the balance element
	 */
	
	/* value can be 0 */
	if (value)
	{
		ptr = strstr(value, start);
	}
	else
	{
		ptr = 0;
	}
	
	if (ptr)
	{
		/* Check "<" */
		if (*(ptr-1) != '<')
			return NULL;
	
		*(ptr-1) = '\0';
		
		/* save value end for eat white space */
		value_end = ptr-2; 
		
		/* Check ">" */
		ptr += strlen(start);
		if (*ptr != '>')
			return NULL;
		
		ptr++;
		next_element = ptr;
		
		/* consume leading white spaces */
		while (value <= value_end)
		{
			if (*value == ' ' || *value == '\t' || *value == '\r' || *value == '\n')
				value++;
			else
				break;
		}
		
		/* consume tailing white spaces */
		while (value_end >= value)
		{
			if (*value_end == ' ' || *value_end == '\t' || *value_end == '\r' || *value_end == '\n')
			{
				*value_end = '\0';
				value_end--;
			}
			else
				break;
		}
	}
	else
	{
		next_element = NULL;
	}
	
	/* output values */
	if (tag)
	{
		if (element == start+1)
		{
			*tag = NULL;
		}
		else
		{
			*tag = start+1;        /* Skip < */
			*(element-1) = 0;      /* Set ":" to NULL */
		}
	}
	
	if (name)
		*name = element;
	
	if (attr)
		*attr = aux;
	
	if (next)
		*next = next_element;
	
	return value;
}


/*==============================================================================
 * Function:    soap_send_error
 *------------------------------------------------------------------------------
 * Description: Send error response
 *
 *============================================================================*/
void soap_send_error(UPNP_CONTEXT *context, int error, char *err_desc)
{
	char time_buf[64];
	char *err_str;
	int body_len;
	int len;
	
	switch (error)
	{
	case SOAP_INVALID_ACTION:
		err_str = "Invalid Action";
		break;
	
	case SOAP_INVALID_ARGS:
		err_str = "Invalid Argument";
		break;
		
	case SOAP_OUT_OF_SYNC:
		err_str = "Out of Sync";
		break;
		
	case SOAP_INVALID_VARIABLE:
		err_str = "Invalid Variable";
		break;
		
	case SOAP_DEVICE_INTERNAL_ERROR:
		err_str = "Action Failed";
		break;
		
	default:
		err_str = err_desc;
		break;
	}
	
	/* format body */
	body_len = sprintf(context->body_buffer, 
						"<s:Envelope "
							"xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
							"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
						"<s:Body>"
						"<s:Fault>"
							"<faultcode>s:Client</faultcode>"
							"<faultstring>UPnPError</faultstring>"
							"<detail>"
								"<UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">"
									"<errorCode>%d</errorCode>"
									"<errorDescription>%s</errorDescription>"
								"</UPnPError>"
							"</detail>"
						"</s:Fault>"
						"</s:Body>"
						"</s:Envelope>",
						error,
						err_str);
			
	/* format header */
	gmt_time(time_buf);  /* get GMT time */
	len = sprintf(context->head_buffer,
						"HTTP/1.1 500 Internal Server Error\r\n"
						"Content-Length: %d\r\n"
						"Content-Type: text/xml; charset=\"utf-8\"\r\n"
						"Date: %s\r\n"
						"EXT: \r\n"
						"Server: %s/%s UPnP/1.0 %s/%s\r\n"
						"Connection: close\r\n"
						"\r\n"
						"%s",
						body_len,
						time_buf,
						upnp_os_name,
						upnp_os_ver,
						upnp_model_name,
						upnp_model_ver,
						context->body_buffer);
	
	/* send error message */
	if (send(context->fd, context->head_buffer, len, 0) == -1)
	{
		printf( "soap_send_error() failed");
	}
	 
	return ;
}


/*==============================================================================
 * Function:    soap_send
 *------------------------------------------------------------------------------
 * Description: Send out response
 *
 *============================================================================*/
void soap_send(UPNP_CONTEXT *context)
{
	char time_buf[64];	
	int head_len;
	int body_len;
	
	/* get GMT time string */
	gmt_time(time_buf);
	
	// Get Content-Length
	body_len = strlen(context->body_buffer);   

	// Print String
	head_len = sprintf(context->head_buffer, 
						"HTTP/1.1 200 OK\r\n"
						"Content-Length: %d\r\n"
						"Content-Type: text/xml; charset=\"utf-8\"\r\n"
						"Date: %s\r\n"
						"EXT: \r\n"
						"Server: %s/%s UPnP/1.0 %s/%s\r\n"
						"Connection: close\r\n"
						"\r\n",
						body_len,
						time_buf,
						upnp_os_name,
						upnp_os_ver,
						upnp_model_name,
						upnp_model_ver);
	memcpy(context->head_buffer+head_len,context->body_buffer,body_len+1);
			
	if (send(context->fd, context->head_buffer, head_len+body_len, 0) == -1)
	{
		printf("send Invoke action header failed!!\n");
	}

	
	return ;
}


#ifdef CONFIG_WPS
/*==============================================================================
 * Function:    send_action_response()
 *------------------------------------------------------------------------------
 * Description: Compose action response message and send
 *
 *============================================================================*/
void wps_send_action_response(UPNP_CONTEXT *context, UPNP_SERVICE *service, UPNP_ACTION *action)
{
	char *p = context->body_buffer;
	OUT_ARGUMENT *temp = context->out_arguments;
	int len;
	
	len = sprintf(context->body_buffer,
				"<?xml version=\"1.0\"?>\r\n"
				"<s:Envelope "
					"xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
					"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
				"<s:Body>\r\n"
				"<u:%sResponse xmlns:u=\"urn:schemas-wifialliance-org:service:%s:1\">\r\n",
				action->name,
				service->name);
	
	p += len;

	/* concatenate output values */
	while (temp)
	{
		translate_value(temp->type, &temp->value);
		len = sprintf(p,
					"<%s>%s</%s>\r\n",
					temp->name,
					temp->value.str,
					temp->name);
	
		p += len;
		temp = temp->next;
	}
	
	/* adding closure */
	sprintf(p,
			"</u:%sResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n",
			action->name);

	soap_send(context);
	
	return ;
}
#endif /* CONFIG_WPS */


/*==============================================================================
 * Function:    send_action_response()
 *------------------------------------------------------------------------------
 * Description: Compose action response message and send
 *
 *============================================================================*/
void send_action_response
(
	UPNP_CONTEXT *context,
	UPNP_SERVICE *service,
	UPNP_ACTION *action
)
{
	char *p = context->body_buffer;
	OUT_ARGUMENT *temp = context->out_arguments;
	int len;
	
	len = sprintf(context->body_buffer,
				"<s:Envelope "
					"xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
					"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
				"<s:Body>\r\n"
				"<u:%sResponse xmlns:u=\"urn:schemas-upnp-org:service:%s:1\">\r\n",
				action->name,
				service->name);
	
	p += len;
	
	/* concatenate output values */
	while (temp)
	{
		translate_value(temp->type, &temp->value);
		len = sprintf(p,
					"<%s>%s</%s>\r\n",
					temp->name,
					temp->value.str,
					temp->name);
	
		p += len;
		temp = temp->next;
	}
	
	/* adding closure */
	sprintf(p,
			"</u:%sResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n",
			action->name);
			
	soap_send(context);
	
	return ;
}


/*==============================================================================
 * Function:    soap_control()
 *------------------------------------------------------------------------------
 * Description: Action invoke function.
 *              (1) verify all required input arguments
 *              (2) prepare all output arguements buffer
 *              (3) invoke action function
 *              (4) send back soap action response
 *============================================================================*/
int soap_control(UPNP_CONTEXT *context, UPNP_SERVICE *service, UPNP_ACTION *action, int bForWPS)
{
	int error = SOAP_INVALID_ARGS;
	char errmsg[SOAP_MAX_ERRMSG];
	
	int i;
	IN_ARGUMENT	*in_temp;
	OUT_ARGUMENT *temp;
	
	// Set errmsg to null string
	errmsg[0] = 0;
	
	for (i=0; i<action->in_num; i++)
	{
		in_temp = context->in_args; /* in argument list */
		while (in_temp)
		{
			if (strcmp(in_temp->name, action->in_argument[i].name) == 0)
			{
				/* convert types */
				in_temp->type = action->in_argument[i].type;
				
				if (convert_value(in_temp) != 0)
					goto err_out;
				
				break;
			}
			
			in_temp = in_temp->next;
		}
		
		if (in_temp == NULL)
			goto err_out;
	}
	
	/* allocate buffer for all output arguments */
	temp = context->out_args;
	for (i=0; i<action->out_num; i++, temp++)
	{
		if (temp == context->out_args + UPNP_MAX_OUT_ARG)
		{
		        DBGPRINTF(RT_DBG_ERROR, "%s: Check UPNP_MAX_OUT_ARG!!\n", __FUNCTION__);
			return R_ERROR;
		}
		
		temp->name = action->out_argument[i].name;
		temp->type = action->out_argument[i].type;
		
		/* append to out_argument list */
		temp->next = 0;
		
		if (i > 0)
			(temp-1)->next = temp;
	}
	
	/* invoke action */
	context->in_arguments = action->in_num ? context->in_args : 0;
	context->out_arguments = action->out_num ? context->out_args : 0;
	
	error = (*action->action)(service,
                                context->in_arguments,
                                context->out_arguments,
                                errmsg);
	if (error == 0)
	{
#ifdef CONFIG_WPS
		if (bForWPS)
		{
        		DBGPRINTF(RT_DBG_INFO, "%s: action->name=%s\n", __FUNCTION__, action->name);

			/* GetDeviceDescription, PutMessage(M2, M4, M6, M8), etc */
			if ((strcmp(action->name, "GetDeviceInfo")==0) || (strcmp(action->name, "PutMessage")==0))
			{
				WscEnvelope *envelope = NULL;
				envelope = cyg_mbox_get((cyg_handle_t)wsc_msg_sync_id);
				
					DBGPRINTF(RT_DBG_ERROR,"%s, Start to do M2, M4, M6 and M8 and location is %p\n",__FUNCTION__,envelope);
				if (envelope->flag != WSC_ENVELOPE_TIME_OUT) // success
				{
					wps_send_action_response(context, service, action);
					wscEnvelopeFree(envelope);
				}
				else // timeout
				{
					DBGPRINTF(RT_DBG_ERROR, "%s: ****************WSC_ENVELOPE_TIME_OUT\n\n\n\n\n", __FUNCTION__);
					wscEnvelopeFree(envelope);
					error = SOAP_DEVICE_INTERNAL_ERROR;
					goto err_out;
				}
			}
			else /* SetSelectedRegistrar, PutWLANReponse{M2, M4, M6, M8), etc*/
			{
 				wps_send_action_response(context, service, action);
               		        DBGPRINTF(RT_DBG_ERROR, "%s: PutWLANReponse{M2, M4, M6, M8)\n", __FUNCTION__);                                
			}
		}
		else
#endif /* CONFIG_WPS */
		send_action_response(context, service, action);

		return 0;
	}
	
err_out:
	soap_send_error(context, error, errmsg);
	return 0;
}


/*==============================================================================
 * Function:    action_process()
 *------------------------------------------------------------------------------
 * Description: Parse the elements and get the action name and argument lists,
 *              then invoke action
 *
 *============================================================================*/
int action_process(UPNP_CONTEXT *context, UPNP_SERVICE *service, int bWPS_action)
{
	char *p = context->content;
	UPNP_ACTION *action;
	int span;
	char *element;
	char *next;
	int error = 0;
	char *arg_name;
	char *arg_value;
	
	IN_ARGUMENT *temp;

	/* process <?xml version="1.0"?> */
	if (memcmp(p, "<?xml version=\"1.0\"?>", 21) == 0)
	{
		p += 21;
		span = strspn(p, "\r\n");
		p += span;
	} else if (memcmp(p, "<?xml version=\"1.0\" encoding=\"utf-8\"?>", 38) == 0) {
		p += 38;
		span = strspn(p, "\r\n");
		p += span;
	} else {
                DBGPRINTF(RT_DBG_PKT, "%s: Can not parsing content:\n%s\n", __FUNCTION__, p);
	}
	

	/* process Envelope */
	p = parse_element(p, 0, &element, 0, 0);
	if (p == NULL || strcmp(element, "Envelope") != 0)
		return R_BAD_REQUEST;
	
	/* process Body */
	p = parse_element(p, 0, &element, 0, 0);
	if (p == NULL || strcmp(element, "Body") != 0)
		return R_BAD_REQUEST;
	
	/* process action */
	p = parse_element(p, 0, &element, 0, 0);
	if (strcmp(element, context->SOAPACTION) != 0)
		return R_BAD_REQUEST;
	
	/* find action */
	for (action = service->action_table; action->name; action++)
	{
		if (strcmp(context->SOAPACTION, action->name) == 0)
			break;
	}
	
	if (action->name == NULL)
	{
		soap_send_error(context, SOAP_INVALID_ACTION, NULL);
		return 0;
	}                   
	
	/* process argument list */
	temp = context->in_args;
	next = p;
	while (next)
	{
		arg_value = parse_element(next, 0, &arg_name, 0, &next);
		if (!arg_value)
			break;
		
		if (temp == context->in_args + UPNP_MAX_IN_ARG)
		{
			soap_send_error(context, SOAP_INVALID_ARGS, NULL);
			return 0;
		}
		
		temp->name = arg_name;
		temp->value.str = arg_value;
		
		/* append to in argument list */
		temp->next = 0;
		if (temp != context->in_args)
			(temp-1)->next = temp;
		
		temp++;
	}
	
	/* Make sure in argument count is same */
	if (context->in_args + action->in_num != temp)
	{
		soap_send_error(context, SOAP_INVALID_ARGS, NULL);
		return 0;
	}
	
	/* invoke control action */
	error = soap_control(context, service, action, bWPS_action);
	if (error == 0)
	{
#if 0
		/* check whether we need further eventing processing */
		if ((service->evented)/* && (bWPS_action==0)*/)
		{
//			printf("\n--->gena_notify\n");
			/* invoke gena function */       
			gena_notify(context, service, NULL); //Eddy Add
//			printf("\n--->gena_notify_complete\n");			
			gena_notify_complete(service);
//			printf("\n<---gena_notify_complete\n");
		}
#endif
	}
	else
		DBGPRINTF(RT_DBG_INFO, "===> %s Error\n", __FUNCTION__);
	return error;
}


/*==============================================================================
 * Function:    send_query_response
 *------------------------------------------------------------------------------
 * Description: Send query response
 *
 *============================================================================*/
void send_query_response(UPNP_CONTEXT *context, char *value)
{
	sprintf(context->body_buffer,
			"<s:Envelope "
				"xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
				"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
			"<s:Body>\r\n"
			"<u:QueryStateVariableResponse xmlns:u=\"urn:schemas-upnp-org:control1-1-0\">\r\n"
			"<return>"
			"%s"
			"</return>\r\n"
			"</u:QueryStateVariableResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n",
			value);
	
	soap_send(context);
	
	return;
}


/*==============================================================================
 * Function:    soap_query
 *------------------------------------------------------------------------------
 * Description: Invoke statevar query function
 *
 *============================================================================*/
int soap_query(UPNP_CONTEXT *context, UPNP_SERVICE *service, UPNP_STATE_VAR *statevar)
{
	UPNP_VALUE value;
	int error = 0;
	char errmsg[SOAP_MAX_ERRMSG];
	
	// Initial errmsg to null string
	errmsg[0] = 0;
	
	/* invoke query function */
	if (statevar->func)
	{
		error = (*statevar->func)(service, &value, errmsg);
		if (error != 0)
		{
			soap_send_error(context, error, errmsg);
			return 0;
		}
		
		/* translate value to string */
		translate_value(statevar->type, &value);
	}
	else
	{
		// Send null string because some variables
		// are only able to get by action process
		value.str[0] = 0;
	}
			
	send_query_response(context, value.str);
	return 0;
}


/*==============================================================================
 * Function:    query_process()
 *------------------------------------------------------------------------------
 * Description: Parse the elements and get the statevar name, then invoke query
 *
 *============================================================================*/
int query_process(UPNP_CONTEXT *context, UPNP_SERVICE *service)
{
	UPNP_STATE_VAR *statevar;
	char *p = context->content;
	char *var_name;
	char *element;
	int span;

	/* process <?xml version="1.0"?> */
	if (memcmp(p, "<?xml version=\"1.0\"?>", 21) == 0)
	{
		p += 21;
		span = strspn(p, "\r\n");
		p += span;
	} else if (memcmp(p, "<?xml version=\"1.0\" encoding=\"utf-8\"?>", 38) == 0) {
		p += 38;
		span = strspn(p, "\r\n");
		p += span;
	} else {
		DBGPRINTF(RT_DBG_ERROR, "%s: Can not parsing content:\n%s\n", __FUNCTION__, p);
	}
		
	/* process Envelope */
	p = parse_element(p, 0, &element, 0, 0);
	if (p == NULL || strcmp(element, "Envelope") != 0)
		return R_BAD_REQUEST;
	
	/* process Body */
	p = parse_element(p, 0, &element, 0, 0);
	if (p == NULL || strcmp(element, "Body") != 0)
		return R_BAD_REQUEST;
	
	/* process QueryStateVariable */
	p = parse_element(p, 0, &element, 0, 0);
	if (p == NULL || strcmp(element, "QueryStateVariable") != 0)
		return R_BAD_REQUEST;
	
	/* process varName */
	p = parse_element(p, 0, &element, 0, 0);
	if (p == NULL || strcmp(element, "varName") != 0)
		return R_BAD_REQUEST;
	
	/* find state variable */   
	var_name = p;
	for (statevar = service->statevar_table; statevar->name; statevar++)
	{
		if (strcmp(var_name, statevar->name) == 0)
			break;
	}    
	
	if (statevar->name == NULL)
	{
		soap_send_error(context, SOAP_INVALID_VARIABLE, NULL);
		return 0;
	}
	
	return soap_query(context, service, statevar);
}

/*==============================================================================
 * Function:    soap_process()
 *------------------------------------------------------------------------------
 * Description: SOAP process entry; parse header and decide which control
 *
 *============================================================================*/
int soap_process(UPNP_CONTEXT *context)
{
	char *action_field;
	UPNP_SERVICE *service;
        int status = R_BAD_REQUEST;

        DBGPRINTF(RT_DBG_INFO, "===> %s\n", __FUNCTION__);

	cyg_mutex_lock(&upnp_service_table_mutex);	
	/* find service */
	service = get_service(context->url);
	if (service == NULL)
        {
                status = R_NOT_FOUND;
                goto EXIT;
        }       

	action_field = context->SOAPACTION;
	if (action_field == NULL)
        {
                status = R_BAD_REQUEST;
                goto EXIT;
        }       
	
	if (strcmp(action_field, "\"urn:schemas-upnp-org:control-1-0#QueryStateVariable\"") == 0 ||
		strcmp(action_field, "urn:schemas-upnp-org:control-1-0#QueryStateVariable") == 0)
	{
		/* query control */
                status = query_process(context, service);
                goto EXIT;
	}
	else if (memcmp(action_field, "\"urn:schemas-upnp-org:service:", 30) == 0 ||
			 memcmp(action_field, "urn:schemas-upnp-org:service:", 29) == 0)
	{
		/* action control */
		char *service_name;
		char *action_name;
		int pos;
		
		if (*action_field == '\"')
		{
			action_field++;
		}
		action_field += 29; /* "urn:schemas-upnp-org:service:" */
		
		/* get service name */
		service_name = action_field;
		pos = strcspn(action_field, ":");
		action_field += pos+1;
		service_name[pos] = 0; /* null ended */
		
		/* save service name */
		if (strcmp(service_name, service->name) != 0)
		{
			cyg_mutex_unlock(&upnp_service_table_mutex);
			return R_BAD_REQUEST;
		}	
		
		/* get action name */
		pos = strcspn(action_field, "#");
		action_field += pos+1;
		
		action_name = action_field;
		pos = strcspn(action_field, "\"");
		action_name[pos] = 0;
		
		context->SOAPACTION = action_name;
		
                status = action_process(context, service, 0);
                goto EXIT;                
	}
#ifdef CONFIG_WPS        
	else if (memcmp(action_field, "\"urn:schemas-wifialliance-org:service:", 38) == 0 ||
			 memcmp(action_field, "urn:schemas-wifialliance-org:service:", 37) == 0)
	{
		/* action control */
		char *service_name;
		char *action_name;
		int pos;
		
		if (*action_field == '\"')
		{
			action_field++;
		}
		action_field += 37; /* "urn:schemas-wifialliance-org:service:" */
		
		/* get service name */
		service_name = action_field;
		pos = strcspn(action_field, ":");
		action_field += pos+1;
		service_name[pos] = 0; /* null ended */
		
		/* save service name */
		if (strcmp(service_name, service->name) != 0)
		{
			cyg_mutex_unlock(&upnp_service_table_mutex);
			return R_BAD_REQUEST;
		}
		
		/* get action name */
		pos = strcspn(action_field, "#");
		action_field += pos+1;
		
		action_name = action_field;
		pos = strcspn(action_field, "\"");
		action_name[pos] = 0;
		
		context->SOAPACTION = action_name;

                status = action_process(context, service, 1/* for WPS */);
                goto EXIT;
	}
#endif /* CONFIG_WPS */


EXIT:
        DBGPRINTF(RT_DBG_INFO, "<=== %s, status=%d\n", __FUNCTION__, status);
		cyg_mutex_unlock(&upnp_service_table_mutex);
        return status;
}


/*==============================================================================
 * Function:    get_service()
 *------------------------------------------------------------------------------
 * Description: Search the service table for the target control URL
 *
 *============================================================================*/
UPNP_SERVICE *get_service(char *url)
{
	UPNP_SERVICE *service;
	
	for (service=upnp_service_table; service->control_url; service++)
	{
		if (strcmp(url, service->control_url) == 0)
			return service;
	}

	return NULL;
}

