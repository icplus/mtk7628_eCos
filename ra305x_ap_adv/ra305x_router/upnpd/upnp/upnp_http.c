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
 * Declarations for upnp_http engine
 */

/*
 * method parsing lookup table
 */
struct upnp_method upnp_http_methods[] =
{
    {"GET ",            sizeof("GET ")-1,           METHOD_GET,         description_process },
    {"POST ",           sizeof("POST ")-1,          METHOD_POST,        soap_process        },
    {"M-POST ",         sizeof("M-POST ")-1,        METHOD_MPOST,       soap_process        },
    {"SUBSCRIBE ",      sizeof("SUBSCRIBE ")-1,     METHOD_SUBSCRIBE,   gena_process        },
    {"UNSUBSCRIBE ",    sizeof("UNSUBSCRIBE ")-1,   METHOD_UNSUBSCRIBE, gena_process        },
    {0, 0, 0}
};
 
static int read_header	(UPNP_CONTEXT *, struct upnp_method *);
static int read_body	(UPNP_CONTEXT *, struct upnp_method *);

static struct upnp_state upnp_http_fsm[] =
{
	{upnp_context_init,	0			},
	{read_header,		0			},
	{parse_method,		upnp_http_methods	},
	{parse_uri,		0			},
	{parse_msghdr,		0			},
	{read_body,		0			},
	{dispatch,		upnp_http_methods	},
	{0,			0			},
};


/*==============================================================================
 * Function:    read_sock()
 *------------------------------------------------------------------------------
 * Description: 
 *
 *============================================================================*/
static int read_sock(int s, char *buf, int len, int flags)
{
    long     rc;
    fd_set   ibits;
    struct   timeval tv = {5, 0};   /* wait for at most 5 seconds */
	
    FD_ZERO(&ibits);
    FD_SET(s, &ibits);
	
    rc = select(s+1, &ibits, 0, 0, &tv);
    if (rc <= 0)
        return -1;
			
    if (FD_ISSET(s, &ibits))
    {
        return recv(s, buf, len, flags);
    }
	
    return -1;
}


/*==============================================================================
 * Function:    send_error_reply()
 *------------------------------------------------------------------------------
 * Description: Send error reply messages to clients
 *
 *============================================================================*/
static int send_error_reply(UPNP_CONTEXT * req)
{
	int len;
	char *buf = req->head_buffer;
	char *err_msg;
	
	/* get relevant error message */
	switch(req->status) 
	{
	case R_BAD_REQUEST: 
		err_msg = "400 Bad Request";
		break;
		
	case R_FORBIDDEN:
		err_msg = "403 Forbidden";
		break;
		
	case R_PRECONDITION_FAIL:
		err_msg = "412 Precondition Fail";
		break;
	   
	case R_METHOD_NA:
		err_msg = "405 Method Not Allowed";
		break;
		
	case R_NONE_ACC:
		err_msg = "406 Not Acceptable";
		break;
		
	case R_NOT_FOUND:
		err_msg = "404 Not Found";
		break;
		
	case R_ERROR:
		err_msg = "500 Internal Server Error";
		break;
		
	default:
		return 0;
	}
	
	// Generate header
	len = sprintf(buf, "HTTP1.1 %s\r\n", err_msg);
	
        /*
         * Only POST need to send Content-Type and title
         */
        if (req->method == METHOD_POST || req->method == METHOD_MPOST)
        {
                len += sprintf(buf+len,
			        "Content-Type: text/xml\r\n\r\n"
				"<title>%s</title>"
				"<body>%s</body>\r\n",
				err_msg,
				err_msg);
        }
	
        if (send(req->fd, buf, len, 0) == -1) 
                return (-1);
	
        return (0);
}


/*==============================================================================
 * Function:    read_body()
 *------------------------------------------------------------------------------
 * Description: Read body data from socket
 *
 *============================================================================*/
static int read_body(UPNP_CONTEXT *context, struct upnp_method *methods)
{
        char *content_len_p = NULL;
        int len = 0;
        int diff = 0;;
        int n  =0;

        /* NULL ended the body */
        context->content[context->content_len] = '\0';
    
        content_len_p = context->CONTENT_LENGTH;
        if (!content_len_p)
        {
                /* body could be empty */
                return 0;
        }
        else
        {
                len = atoi(content_len_p);
                if (len == 0)
                {
                        return 0;
                }    
                else if (len < 0)
                {   
                        DBGPRINTF(RT_DBG_OFF, "read_body() error (1)");
                        context->status = R_BAD_REQUEST;
                        return -1;
                }
                else if (context->content + len >=  context->buf + sizeof(context->buf))
                {
                        DBGPRINTF(RT_DBG_OFF, "read_body() error (2)");
                        context->status = R_ERROR;
                        return -1;
                }
        }

        /* Receive remainder */
        while ((diff = len - context->content_len) > 0)
        {
                n = read_sock(context->fd, &context->content[context->content_len], diff, 0);
                if (n <= 0)
                {
                        DBGPRINTF(RT_DBG_OFF, "read_body() error (3)");
                        context->status = R_ERROR;
                        return -2;
                }

                context->content_len += n;
        }

        /* NULL ended the body */
        context->content[context->content_len] = '\0';

        return 0;
}


/*==============================================================================
 * Function:    parse_uri()
 *------------------------------------------------------------------------------
 * Description: 
 *============================================================================*/
int parse_uri(UPNP_CONTEXT *context, struct upnp_method *methods)
{
        char *p = context->url;
        int pos;

        /* skip white spaces */
        while (*p == ' ' || *p == '\t')
                p++;
        
        pos = strcspn(p, "\t ");
        
        context->url = p;
        context->url[pos] = 0;

        /* SSDP only accepts "*" */
        if (context->method == METHOD_MSEARCH && strcmp(context->url, "*") != 0)
                return -1;
    
        /* skip URL and white spaces */
        p += pos + 1;
        while (*p == ' ' || *p == '\t')
                p++;

        /* also check HTTP version */
        if (memcmp(p, "HTTP/1.0", 8) != 0 
                && memcmp(p, "HTTP/1.1", 8) != 0)
        {
                DBGPRINTF(RT_DBG_OFF, "%s: HTTP version failed\n", __FUNCTION__);
                return -1;
        }

        return 0;
}


/*==============================================================================
 * Function:    parse_method()
 *------------------------------------------------------------------------------
 * Description: Determine method type
 *              (1) GET
 *              (2) POST
 *              (3) M-POST
 *              (4) SUBSCRIBE
 *              (5) UNSUBSCRIBE
 *============================================================================*/
int parse_method(UPNP_CONTEXT *context, struct upnp_method *methods)
{
        int i = 0;
        char *p = context->msghdrs[0];

        DBGPRINTF(RT_DBG_PKT, "%s: %s\n\n", __FUNCTION__, p);

        while (methods[i].str != 0)
        {
                if (memcmp(p, methods[i].str, methods[i].len) == 0)
                {
                        context->method_id  = i;
                        context->method     = methods[i].method;
                        context->url        = p + methods[i].len;
                        break;
                }
                i++;
        }

        if (methods[i].str == 0)
        {
                context->status = R_METHOD_NA;
//                DBGPRINTF(RT_DBG_OFF, "%s: Can't match the method\n%s\n\n", __FUNCTION__, p);
                return -1;
        }

    return 0;
}


/*==============================================================================
 * Function:    dispatch()
 *------------------------------------------------------------------------------
 * Description: Invoke method handling function
 *
 *============================================================================*/
int dispatch(UPNP_CONTEXT *context, struct upnp_method *methods)
{
        context->status = (*methods[context->method_id].dispatch)(context);
        if (context->status != 0)
        {
                DBGPRINTF(RT_DBG_OFF, "%s Fail, method=%s, status=%d\n", __FUNCTION__, methods->str, context->status);
                return -1;
        }

        return 0;
}


/*==============================================================================
 * Function:    save_header()
 *------------------------------------------------------------------------------
 * Description: save HTTP headers
 *
 *============================================================================*/
static void save_header(UPNP_CONTEXT *context, char *name, char *value)
{
	if (strcmp(name, "CONTENT-LENGTH") == 0)
	{
		context->CONTENT_LENGTH = value;
	}
	else if (strcmp(name, "SOAPACTION") == 0)
	{
		context->SOAPACTION = value;
	}
	else if (strcmp(name, "SID") == 0)
	{
		context->SID = value;
	}
	else if (strcmp(name, "CALLBACK") == 0)
	{
		context->CALLBACK = value;
	}
	else if (strcmp(name, "TIMEOUT") == 0)
	{
		context->TIMEOUT = value;
	}
	else if (strcmp(name, "NT") == 0)
	{
		context->NT = value;
	}
	else if (strcmp(name, "HOST") == 0)
	{
		context->HOST = value;
	}
	else if (strcmp(name, "MAN") == 0)
	{
		context->MAN = value;
	}
	else if (strcmp(name, "ST") == 0)
	{
		context->ST = value;
	}
	
	return;
}


/*==============================================================================
 * Function:    parse_msghdr()
 *------------------------------------------------------------------------------
 * Description: Parse HTTP header and set appropriate env variables
 *
 *============================================================================*/
int parse_msghdr(UPNP_CONTEXT *context, struct upnp_method *methods)
{
	char *token, *value;
	char *p;
	int i;

	/* skip the first line (method, url, and HTTP version), which we already processed */
	for (i=1; i<context->header_num; i++)
	{
		token = strtok(context->msghdrs[i], ":");
		
		if (token != NULL)
		{
			/* skip white spaces */
			while (*token == '\t' || *token == ' ')
				token++;
				
			value = strtok(NULL, "");
			
			if (value != NULL)
			{
				/* skip white spaces */
				while (*value == '\t' || *value == ' ')
					value++;
				
				/* capitalize token*/
				p = token;  
				while (*p++)
					*p = toupper(*p);
				
				save_header(context, token, value);
			}
			else
			{
        		        DBGPRINTF(RT_DBG_OFF, "Null value in parse_msghdr()\n");
				goto error_out;
			}
		}
		else
		{
		        DBGPRINTF(RT_DBG_OFF, "Null token in parse_msghdr()\n");
			goto error_out;
		}
	}
	return 0;
	
error_out:
	context->status = R_ERROR;
	return -1;
}


/*==============================================================================
 * Function:    record_msghdr()
 *------------------------------------------------------------------------------
 * Description: Disassemble header and record the message headers
 *
 *============================================================================*/
static int record_msghdr(UPNP_CONTEXT *context, char *ptr, int eol, int eoh)
{
        int i;
    
        /* clear end of line characters */
        for (i=0; i<eol; i++)
        {
                context->buf[context->index-i-1] = 0;
        }

        /* clear end of header characters */
        for (i=0; i<eoh; i++)
        {
                context->buf[context->index+i] = 0;
        }
            
        if (context->header_num == MAX_HEADERS)
        {
                DBGPRINTF(RT_DBG_OFF, "Too many header to handle!\n");
                return -1;
        }

        context->msghdrs[context->header_num] = ptr; 
        context->header_num++;
        
        return 0;
}


/*==============================================================================
 * Function:    get_msghdr()
 *------------------------------------------------------------------------------
 * Description: Find end of a message header
 *
 *============================================================================*/
int get_msghdr(UPNP_CONTEXT *context)
{
    int i       = context->index;
    int end     = context->end;
    char *ptr   = &context->buf[i];
    int eoh     = 0;
    int eol     = 0;
    char c;
	
    /* start parsing */
    while (i < end) /* start < end */
    {
        c = context->buf[i++];
		
        if (c == '\n')
        {
            if (i < end && 
                context->buf[i] != '\r' &&
                context->buf[i] != '\n')
            {
                /* end of line */
                if (i>1 && context->buf[i-2] == '\r')
                    eol = 2;  /* \r\n */
                else
                    eol = 1;  /* \n */
            }
            else if (i < end && context->buf[i] == '\n')
            {
                /* end of headers: \n\n */
                eol = 1;
                eoh = 1;
            }
            else if (i > 1 && i+1 < end &&
                     context->buf[i-2] == '\r' &&
                     context->buf[i] == '\r' && context->buf[i+1] == '\n')
            {
                /* end of headers: \r\n\r\n */
                eol = 2;
                eoh = 2;
            }

            if (eol)
            {
                /* make header */
                context->index = i;
                
                if (record_msghdr(context, ptr, eol, eoh) != 0)
                {
                    context->status = R_ERROR;
                    return 0;
                }
                    
                ptr = &context->buf[i];
            }

            if (eoh)
            {
                context->index += eoh;
                return 0;
            }
            else
                eol = 0;
        }
    }

    return -1;
}



/*==============================================================================
 * Function:    read_header()
 *------------------------------------------------------------------------------
 * Description: Read data from socket until end of header of error occurs
 *
 *============================================================================*/
static int read_header(UPNP_CONTEXT *context, struct upnp_method *methods)
{
        int buf_size;
        int n;

        DBGPRINTF(RT_DBG_INFO, "%s\n", __FUNCTION__);
        while (get_msghdr(context))
        {
                /* check buffer size */
                buf_size = sizeof(context->buf) - context->end;
                if (buf_size <= 0)
                {
                        DBGPRINTF(RT_DBG_ERROR, "%s: Cannot get enough buffer\n", __FUNCTION__);
                        context->status = R_BAD_REQUEST;
                        break;
                } 

                /* read socket into the buffer */
                n = read_sock(context->fd, &context->buf[context->end], buf_size, 0);
                if (n <= 0)
                {
                        context->status = R_ERROR;
                        return -2;
                }
                DBGPRINTF(RT_DBG_INFO, "Length=%d\n%s\n", n, &context->buf[context->end]);
                
                /* Move end */
                context->end += n;
        }

        if (context->status != 0)
                return -1;
    
        context->content = &context->buf[context->index];
        context->content_len = context->end - context->index;
    
        return 0;
}


/*==============================================================================
 * Function:    context_init()
 *------------------------------------------------------------------------------
 * Description: Initialize the upnp_http context
 *
 *============================================================================*/
int upnp_context_init(UPNP_CONTEXT *context, struct upnp_method *methods)
{
	context->header_num = 0;
	context->status = 0;
	context->index  = 0;
	context->end    = 0;
	
	context->CONTENT_LENGTH = 0;
	context->SOAPACTION = 0;
	context->SID = 0;
	context->CALLBACK = 0;
	context->TIMEOUT = 0;
	context->NT = 0;
	context->HOST = 0;
	context->MAN = 0;
	context->ST = 0;

	return 0;
}


/*==============================================================================
 * Function:    upnp_http_engine()
 *------------------------------------------------------------------------------
 * Description: Parse and process client (control point) requests
 *
 *============================================================================*/
int upnp_http_engine(UPNP_CONTEXT *context, struct upnp_state *fsm)
{
	int i;
	int ret = 0;
	
	for (i=0; fsm[i].func; i++)
	{
		ret = (*fsm[i].func)(context, fsm[i].parm);
		if (ret < 0)
			break;
	}
	
	return ret;
}


/*==============================================================================
 * Function:    upnp_http_process()
 *------------------------------------------------------------------------------
 * Description: Process client (control point) requests
 *
 *============================================================================*/
void upnp_http_process(UPNP_CONTEXT *context, int s)
{
        DBGPRINTF(RT_DBG_INFO, "===> %s\n", __FUNCTION__);
        
	context->fd = s;	
	/* process HTTP header */
	if (upnp_http_engine(context, upnp_http_fsm) == -1)
		send_error_reply(context);
	
	close(s);

        DBGPRINTF(RT_DBG_INFO, "<=== %s\n", __FUNCTION__);
	return;
}


/*==============================================================================
 * Function:    open_sock()
 *------------------------------------------------------------------------------
 * Description: Open upnp http socket
 *
 *============================================================================*/
static int open_sock(UPNP_CONTEXT *context, u_long addr)
{
	int s;
	int reuse = 1;
	struct sockaddr_in sin;
	
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return -1;
	
#ifdef BSD_SOCK
	if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0)
                DBGPRINTF(RT_DBG_OFF, "Cannot set socket option (SO_REUSEPORT)\n");
#else
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
                DBGPRINTF(RT_DBG_OFF, "Cannot set socket option (SO_REUSEPORT)\n");
#endif
	
	memset(&sin,0, sizeof(sin));
	
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(addr);
	sin.sin_port = htons(context->http_port);
#ifdef BSD_SOCK
	sin.sin_len = sizeof(sin);
#endif
	
        DBGPRINTF(RT_DBG_ERROR, "Bind IP:%d.%d.%d.%d, Port:%d \n"
                , ((addr & 0xff000000) >> 24)
                , ((addr & 0x00ff0000) >> 16)
                , ((addr & 0x0000ff00) >> 8)
                , ((addr & 0x000000ff))
                , context->http_port);

	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0 ||
		listen (s, MAX_WAITS) < 0)
	{
		close(s);
		return -1;
	}
	
	return s;
}


/*==============================================================================
 * Function:    upnp_http_shutdown()
 *------------------------------------------------------------------------------
 * Description: Initialize upnp_http port number
 *
 *============================================================================*/
void upnp_http_shutdown(UPNP_CONTEXT *context)
{
	UPNP_INTERFACE *ifp;
	
	if (context->http_sock != -1)
	{
		close(context->http_sock);
		context->http_sock = -1;
	}
	
	ifp = context->iflist;
	while (ifp)
	{
		if (ifp->http_sock != -1)
		{
			close(ifp->http_sock);
			ifp->http_sock = -1;
		}
		
		ifp = ifp->next;
	}
}


/*==============================================================================
 * Function:    upnp_http_init()
 *------------------------------------------------------------------------------
 * Description: Initialize upnp_http port number
 *
 *============================================================================*/
int upnp_http_init(UPNP_CONTEXT *context)
{
#ifdef	BIND_TO_INADDR_ANY
	context->http_sock = open_sock(context, INADDR_ANY);
#else	
	UPNP_INTERFACE *ifp = context->iflist;
	
	while (ifp)
	{
		ifp->http_sock = open_sock(context, ifp->ipaddr);
		if (ifp->http_sock == -1)
			return -1;
		
		ifp = ifp->next;
	}
#endif
	
	return 0;
}

