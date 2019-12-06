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
#include <xml_data.h>


/*==============================================================================
 * Function:    description_send()
 *------------------------------------------------------------------------------
 * Description: Send description XML file
 *
 *============================================================================*/
int description_send(UPNP_CONTEXT *context, char *data_buf, int data_len)
{
	
	if (context->xml_stage == 0)
	{
		context->content_len += data_len;
	}
	else
	{
		char *p;
		int len;
		
		p = data_buf;
		
		while (data_len)
		{
			len = (data_len > UPNP_DESC_MAXLEN) ? UPNP_DESC_MAXLEN : data_len;
			
			if (send(context->fd, p, len, 0) == -1)
			{
				printf( "description_send() failed");
				return -1;
			}
			
			p += len;
			data_len -= len;
		}
	}
	
	return 0; 
}


/*==============================================================================
 * Function:    description_process()
 *------------------------------------------------------------------------------
 * Description: Description lookup and sending routine
 *
 *============================================================================*/
int description_process(UPNP_CONTEXT *context)
{
	UPNP_DESCRIPTION *desc = NULL;
	int len;
	int i;

        DBGPRINTF(RT_DBG_INFO, "===> %s\n", __FUNCTION__);
        
	/* search the table for target url */
	for (i=0; upnp_description_table[i].xml; i++)
	{
		if (strcmp(context->url, upnp_description_table[i].name) == 0)
		{
			/* match */
			desc = &upnp_description_table[i];
			break;
		}
	}
	
	if (!desc)
	{
		/* no match */
	        DBGPRINTF(RT_DBG_OFF, "%s:url(%s) is not match\n", __FUNCTION__, context->url);
		return R_NOT_FOUND;
	}
	
	/* Do the first time evaluation for content_len */
	context->xml_stage = 0;
	context->content_len = 0;
	(*desc->xml)(context);
	
	/* send header */
	len = sprintf(context->buf,
				  "HTTP/1.1 200 OK\r\n"
				  "Content-Type: text/xml\r\n"
				  "Content-Length: %d\r\n"
				  "Connection: close\r\n"
				  "Pragma: no-cache\r\n"
				  "\r\n",
				  context->content_len);
	
	if (send(context->fd, context->buf, len, 0) == -1)
	{
	        DBGPRINTF(RT_DBG_OFF, "%s: send failed\n", __FUNCTION__);
		return R_ERROR;
	}
	
	/* send description XML body */
	context->xml_stage++;
	(*desc->xml)(context);

        DBGPRINTF(RT_DBG_INFO, "<=== %s\n", __FUNCTION__);        
	return 0;
}


extern char *upnpxmldata_location;
extern upnpxmldata_entry upnpxmldata_table[];

/*==============================================================================
 * Function:    description_readXML()
 *------------------------------------------------------------------------------
 * Description: Description read and decompress compressed XML template of Description
 *
 *============================================================================*/

int description_readXML(int XMLIndex, char** dst, int *dlen)
{
	int datalen=0;
	char * dataptr = NULL;
	int offset=0;
	int i=0;

	if (upnpxmldata_location == 0)
		return 0;
	if(XMLIndex==0)
		offset=0;
	else
	{
		for(i=0;i<XMLIndex;i++)
		offset+=upnpxmldata_table[i].size;
	}	
	datalen = lzma_org_len(upnpxmldata_location+offset);
	//diag_printf("**length:[%d]**\n",datalen);
	
	dataptr = malloc(datalen+1);
	if (dataptr)
	{
		int try_counter=0;
		int error=0;
try_again:		
		try_counter++;

		error=uncompress(dataptr, &datalen, upnpxmldata_location+offset, upnpxmldata_table[XMLIndex].size);
		if (error != 0 && error != 6)
		{
			diag_printf("\nunzip XML_template[%d],error![%d]\n", XMLIndex,error);
			if(try_counter < 2)
				goto try_again;
			free(dataptr);
			goto perr;
		}
		*dlen = datalen;
		*dst = dataptr;
		dataptr[datalen] = '\0';
		//diag_printf("%c%c%c%c%c%c%c%c%c%c\n",dataptr[0],dataptr[1],dataptr[2],dataptr[3],dataptr[4],dataptr[5],dataptr[6],dataptr[7],dataptr[8],dataptr[9]);
		//diag_printf("%c%c%c%c%c%c%c%c%c%c\n",dataptr[datalen-10],dataptr[datalen-9],dataptr[datalen-8],dataptr[datalen-7],dataptr[datalen-6],dataptr[datalen-5],dataptr[datalen-4],dataptr[datalen-3],dataptr[datalen-2],dataptr[datalen-1]);
		return 1;
	}
	else
	{
		diag_printf("XML_template[%d], No memory for zip\n", XMLIndex);
		goto perr;
	}
	
perr:
	return 0;	
}

