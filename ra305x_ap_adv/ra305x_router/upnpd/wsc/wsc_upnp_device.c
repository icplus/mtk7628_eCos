///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000-2003 Ralink Corporation 
// All rights reserved. 
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met: 
//
// * Redistributions of source code must retain the above copyright notice, 
// this list of conditions and the following disclaimer. 
// * Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation 
// and/or other materials provided with the distribution. 
// * Neither name of Intel Corporation nor the names of its contributors 
// may be used to endorse or promote products derived from this software 
// without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __ECOS
#include <upnp.h>
#include <upnp_service.h>
#include <upnp_type.h>
#include <gena.h>
#else
#include "upnphttp.h"
#include "upnpevents.h"
#include "upnpglobalvars.h"
#endif /* __ECOS */
#include "wsc_msg.h"
#include "wsc_common.h"
#include "wsc_ioctl.h"
#include "wsc_upnp.h"
#include "wsc_upnp_device.h"

#define UUID_STR_LEN            36	
#define WSC_UPNP_UUID_STR_LEN	(5 + UUID_STR_LEN + 1)	// The UUID string get from the driver by ioctl and the strlen is 36 plus 1 '\0', 
						        // and we need extra 5 bytes for prefix string "uuid:"

struct upnpCPNode *WscCPList = NULL;

#ifdef SECOND_WIFI
	struct upnpDevice wscLocalDevice[2];// The data structure used the save the local Wsc UPnP Device.
	//sht:add 2nd device for 5G wifi,the first is still 2.4G wifi in use.
#else
	struct upnpDevice wscLocalDevice;
#endif

//struct upnpDevice wscLocalDevice[WSC_DEVICE_NUM];// The data structure used the save the local Wsc UPnP Device.
//sht:add 2nd device for 5G wifi,the first is still 2.4G wifi in use.
int wscDevHandle = -1;	// Device handle of "wscLocalDevice" supplied by UPnP SDK.
//sht:this device handle has not in use.

/* The amount of time (in seconds) before advertisements will expire */
int defAdvrExpires = 100;

/* 
	Structure for storing Wsc Device Service identifiers and state table 
*/
static char wscStateVarCont[WSC_STATE_VAR_COUNT][WSC_STATE_VAR_MAX_STR_LEN];
static char *wscStateVarDefVal[] = {"", "0", "0"};
#ifdef SECOND_WIFI
static char wscStateVarCont5G[WSC_STATE_VAR_COUNT][WSC_STATE_VAR_MAX_STR_LEN];
#endif
char wscAckMsg[]= {0x10, 0x4a, 0x00, 0x01, 0x10, 0x10, 0x22, 0x00, 0x01, 0x0d, 0x10, 0x1a, 0x00, 0x10, 0x00, 0x00,
				   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x39, 
				   0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
				   0x00, 0x00};


typedef enum _WSC_EVENT_STATUS_CODE{
	WSC_EVENT_STATUS_CONFIG_CHANGED = 0x00000001,
	WSC_EVENT_STATUS_AUTH_THRESHOLD_REACHED = 0x00000010,
}WSC_EVENT_STATUS_CODE;

typedef enum _WSC_EVENT_WLANEVENTTYPE_CODE{
	WSC_EVENT_WLANEVENTTYPE_PROBE = 1,
	WSC_EVENT_WLANEVENTTYPE_EAP = 2,
}WSC_EVENT_WLANEVENTTYPE_CODE;


int senderID = 1;
char CfgMacStr[2][18]={0};//here to cover dual band

extern cyg_mutex_t upnp_service_table_mutex;

/******************************************************************************
 * wscU2KMsgCreate
 *
 * Description: 
 *       Allocate the memory and copy the content to the buffer.
 *
 * Parameters:
 *    char **dstPtr - pointer used for refer the allocated U2Kmsg.
 *    char *srcPtr  - the message need to copy into the U2KMsg.
 *    int 	msgLen  - length of the message "srcPtr".
 *    int	EAPType - the EAP message type. 
 *    				 			1=Identity, 0xFE=reserved, used by WSC
 * Return Value:
 *    Total length of created U2KMsg
 *    	zero 	- if failure
 *    	others  - if success 
 *****************************************************************************/
int 
wscU2KMsgCreate(
	INOUT char **dstPtr,
	IN char *srcPtr,
	IN int msgLen,
	IN int EAPType)
{
	int totalLen;
	char *pPos = NULL, *pMsgPtr = NULL;
	RTMP_WSC_U2KMSG_HDR *pU2KMsgHdr; 
	IEEE8021X_FRAME *p1xFrameHdr;
	EAP_FRAME	*pEAPFrameHdr;
		
	/* Allocate the msg buffer and fill the content */
	totalLen = sizeof(RTMP_WSC_U2KMSG_HDR) + msgLen;
	if ((pMsgPtr = malloc(totalLen)) != NULL)
	{
		memset(pMsgPtr , 0, totalLen);
		pU2KMsgHdr = (RTMP_WSC_U2KMSG_HDR *)pMsgPtr;
		
		// create the IEEE8021X_FRAME header
		p1xFrameHdr = &pU2KMsgHdr->IEEE8021XHdr;
		p1xFrameHdr->Version = IEEE8021X_FRAME_VERSION;
		p1xFrameHdr->Length = htons(sizeof(EAP_FRAME) + msgLen);
		p1xFrameHdr->Type = IEEE8021X_FRAME_TYPE_EAP;

		// create the EAP header
		pEAPFrameHdr = &pU2KMsgHdr->EAPHdr;
		pEAPFrameHdr->Code = EAP_FRAME_CODE_RESPONSE;
		pEAPFrameHdr->Id = 1;  // The Id field is useless here.
		pEAPFrameHdr->Type = EAPType;
		pEAPFrameHdr->Length = htons(sizeof(EAP_FRAME) + msgLen);

		//copy the msg payload
		pPos = (char *)(pMsgPtr + sizeof(RTMP_WSC_U2KMSG_HDR));
		memcpy(pPos, srcPtr, msgLen);
		*dstPtr = pMsgPtr;
		DBGPRINTF(RT_DBG_INFO, "create U2KMsg success!MsgLen = %d, headerLen=%d! totalLen=%d!\n", msgLen, sizeof(RTMP_WSC_U2KMSG_HDR), totalLen);
	}
	else
	{
		DBGPRINTF(RT_DBG_INFO, "malloc allocation(size=%d) failed in wscU2KMsgCreate()!\n", totalLen);
		totalLen = 0;
	}

	return totalLen;
}


/*---------------------------action handler---------------------------*/

/******************************************************************************
 * WscDevGetAPSettings
 *
 * Description: 
 *       Wsc Service action callback used to get the AP's settings.
 *
 * Parameters:
 *    
 *    IXML_Document * in -  action request document
 *    IXML_Document **out - action result document
 *    char **errorString - errorString (in case action was unsuccessful)
 *  
 * Return:
 *    0 		- if success
 *    others 				- if failure
 *****************************************************************************/
int
WscDevGetAPSettings(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **errorString)
{
	// TODO: Need to complete it, currently do nothing and return directly.
	
	
	return 0;

}


/******************************************************************************
 * WscDevGetSTASettings
 *
 * Description: 
 *      Wsc Service action callback used to get the STA's settings.
 *
 * Parameters:
 *   
 *    IXML_Document * in -  action request document
 *    IXML_Document **out - action result document
 *    char **errorString - errorString (in case action was unsuccessful)
 *  
 * Return:
 *    0 		- if success
 *    others 				- if failure
 *****************************************************************************/
int
WscDevGetSTASettings(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **errorString)
{
	// TODO: Need to complete it, currently do nothing and return directly.
	
	
	return 0;

}

/******************************************************************************
 * WscDevSetAPSettings
 *
 * Description: 
 *       Wsc Service action callback used to set the AP's settings.
 *
 * Parameters:
 *   
 *    IXML_Document * in -  action request document
 *    IXML_Document **out - action result document
 *    char **errorString - errorString (in case action was unsuccessful)
 *  
 * Return:
 *    0 		- if success
 *    others 				- if failure
 *****************************************************************************/
int
WscDevSetAPSettings(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **errorString)
{
	// TODO: Need to complete it, currently do nothing and return directly.
	
	
	return 0;

}


/******************************************************************************
 * WscDevDelAPSettings
 *
 * Description: 
 *       Wsc Service action callback used to delete the AP's settings.
 *
 * Parameters:
 *   
 *    IXML_Document * in -  action request document
 *    IXML_Document **out - action result document
 *    char **errorString - errorString (in case action was unsuccessful)
 *  
 * Return:
 *    0 		- if success
 *    others 				- if failure
 *****************************************************************************/
int
WscDevDelAPSettings(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **errorString)
{
	// TODO: Need to complete it, currently do nothing and return directly.
	
	
	return 0;

}


/******************************************************************************
 * WscDevSetSTASettings
 *
 * Description: 
 *       Wsc Service action callback used to set the STA's settings.
 *
 * Parameters:
 *  
 *    IXML_Document * in -  action request document
 *    IXML_Document **out - action result document
 *    char **errorString - errorString (in case action was unsuccessful)
 *  
 * Return:
 *    0 		- if success
 *    others 				- if failure
 *****************************************************************************/
int
WscDevSetSTASettings(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **errorString)
{

	// TODO: Need to complete it, currently do nothing and return directly.
	
	
	return 0;

}


/******************************************************************************
 * WscDevRebootAP
 *
 * Description: 
 *       Wsc Service action callback used to reboot the AP.
 *
 * Parameters:
 *   
 *    IXML_Document * in -  action request document
 *    IXML_Document **out - action result document
 *    char **errorString - errorString (in case action was unsuccessful)
 *  
 * Return:
 *    0 		- if success
 *    others 				- if failure
 *****************************************************************************/
int
WscDevRebootAP(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **errorString)
{
	// TODO: Need to complete it, currently do nothing and return directly.
	
	
	return 0;

}


/******************************************************************************
 * WscDevResetAP
 *
 * Description: 
 *       Wsc Service action callback used to reset the AP device to factory default config.
 *
 * Parameters:
 *   
 *    IXML_Document * in -  action request document
 *    IXML_Document **out - action result document
 *    char **errorString - errorString (in case action was unsuccessful)
 *  
 * Return:
 *    0 		- if success
 *    others 				- if failure
 *****************************************************************************/
int 
WscDevResetAP(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **errorString)
{
	// TODO: Need to complete it, currently do nothing and return directly.
	
	
	return 0;
}


/******************************************************************************
 * WscDevRebootSTA
 *
 * Description: 
 *       Wsc Service action callback used to reboot the STA.
 *
 * Parameters:
 *   
 *    IXML_Document * in -  action request document
 *    IXML_Document **out - action result document
 *    char **errorString - errorString (in case action was unsuccessful)
 *  
 * Return:
 *    0 		- if success
 *    others 				- if failure
 *****************************************************************************/
int
WscDevRebootSTA(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **errorString)
{
	// TODO: Need to complete it, currently do nothing and return directly.
	
	
	return 0;
}


/******************************************************************************
 * WscDevResetSTA
 *
 * Description: 
 *       Wsc Service action callback used to reset the STA to factory default value.
 *
 * Parameters:
 *   
 *    IXML_Document * in -  action request document
 *    IXML_Document **out - action result document
 *    char **errorString - errorString (in case action was unsuccessful)
 *  
 * Return:
 *    0 		- if success
 *    others 				- if failure
 *****************************************************************************/
int
WscDevResetSTA(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **errorString)
{
	
	// TODO: Need to complete it, currently do nothing and return directly.

	return 0;

}


/******************************************************************************
 * WscDevSetSelectedRegistrar
 *
 * Description: 
 *       This action callback used to receive a SetSelectedRegistar message send by
 *       contorl Point(Registrar). 
 *
 * Parameters:
 *   
 *    IXML_Document * in -  action request document
 *    IXML_Document **out - action result document
 *    char **errorString - errorString (in case action was unsuccessful)
 *  
 * Return:
 *    0 - if success
 *    others - if failure
 *****************************************************************************/
int WscDevSetSelectedRegistrar(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **errorString)
{
	char *inStr = NULL;
	unsigned char *decodeStr = NULL;
	int decodeLen, retVal = -1;
	int inStrLen=0;	

	(*errorString) = NULL;

//	wsc_chardump("context->content", (char *)(context->content), context->content_len);
//	wsc_chardump("in_argument->value", (char *)(in_argument->value.str), strlen(in_argument->value.str));

//	printf("--->%s\n", __FUNCTION__);
//	printf("strlen(in_argument->value.str)=%d\n", strlen(in_argument->value.str));

	inStr = in_argument->value.str;
	inStrLen = strlen(in_argument->value.str);

	ASSERT(inStrLen > 0);
	if (!inStr)
	{
		(*errorString) = "Invalid SetSelectedRegistrar mesg parameter";
		perror("402 : Invalid Args");		
		return -1;
	}
	decodeLen = ILibBase64Decode((unsigned char *)inStr, inStrLen, &decodeStr);
	if (decodeLen == 0 || decodeStr == NULL)
	{
		goto done;
	}

	//sht:look for right interface,then send the msg to kernel space.
#ifdef SECOND_WIFI//sht??? mutex protect,race condtion
			if (strcmp("/control", service->control_url) == 0)//"/control" is 2.4G device service.
			{
				memset(&WSC_IOCTL_IF[0], 0, IFNAMSIZ);
				strcpy(&WSC_IOCTL_IF[0], "ra0");
				//diag_printf("!!WscDevSetSelectedRegistrar:ra0\n");
			}	
			else
			{
				memset(&WSC_IOCTL_IF[0], 0, IFNAMSIZ);
				strcpy(&WSC_IOCTL_IF[0], "rai0");		
				//diag_printf("!!WscDevSetSelectedRegistrar:rai0\n");
			}	
#endif

	/* Now send ioctl to wireless driver to set the ProbeReponse bit field. */
	if (ioctl_sock >= 0)
		retVal = wsc_set_oid(RT_OID_WSC_SET_SELECTED_REGISTRAR, (char *)decodeStr, decodeLen);

	if (retVal != 0)
		goto done;
	
done:
	if (inStr)
	{
//		free(inStr);
	}
	if (decodeStr)
		free(decodeStr);

	if (retVal == 0)
		return retVal;
	else
	{
		(*errorString) = "Internal Error";
		return -1;
	}	
}


/******************************************************************************
 * WscDevPutWLANResponse
 *
 * Description: 
 *      When Device in Proxy Mode, the Registrar will use this Action response 
 *      the M2/M2D, M4, M6, and M8 messages.
 *
 * Parameters:
 *   
 *    IXML_Document * in -  action request document
 *    IXML_Document **out - action result document
 *    char **errorString - errorString (in case action was unsuccessful)
 *  
 * Return:
 *    0 - if success
 *    -1 - if failure
 *****************************************************************************/
int
WscDevPutWLANResponse(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **errorString)
{
	char *inStr = NULL;
	unsigned char *decodeStr = NULL;
	int decodeLen, retVal = -1;
	
	char *pWscU2KMsg = NULL;
	RTMP_WSC_U2KMSG_HDR *msgHdr = NULL;
	int wscU2KMsgLen;
	int inStrLen=0;	
	unsigned long ipaddr;
		
	(*errorString) = NULL;

	ipaddr = ntohl(context->dst_addr.sin_addr.s_addr);

//	wsc_chardump("context->content", (char *)(context->content), context->content_len);
//	wsc_chardump("in_argument->value", (char *)(in_argument->value.str), strlen(in_argument->value.str));

//	printf("--->%s\n", __FUNCTION__);	
//	printf("strlen(in_argument->value.str)=%d\n", strlen(in_argument->value.str));	

	inStr = in_argument->value.str;
	inStrLen = strlen(in_argument->value.str);

	ASSERT(inStrLen > 0);
	if (!inStr)
	{
		(*errorString) = "Invalid PutWLANResponse mesg";
		perror("402 : Invalid Args");		
		return retVal;
	}
	decodeLen = ILibBase64Decode((unsigned char *)inStr, inStrLen, &decodeStr);
	if (decodeLen == 0 || decodeStr == NULL)
	{
		goto done;
	}
	
	/* Prepare the msg buffers */
	wscU2KMsgLen = wscU2KMsgCreate(&pWscU2KMsg, (char *)decodeStr, decodeLen, EAP_FRAME_TYPE_WSC);
	if (wscU2KMsgLen == 0)
		goto done;

	// Fill the sessionID, Addr1, and Addr2 to the U2KMsg buffer header.
	msgHdr = (RTMP_WSC_U2KMSG_HDR *)pWscU2KMsg;
	msgHdr->envID = 0;
	memcpy(msgHdr->Addr1, HostMacAddr, MAC_ADDR_LEN);
	memcpy(msgHdr->Addr2, /* ip ? mac ? */(char *)(&ipaddr)/* ipAddr */, sizeof(unsigned int));
	
	DBGPRINTF(RT_DBG_INFO, "(%s):Ready to send pWscU2KMsg(len=%d) to kernel by ioctl(%d)!\n", __FUNCTION__, 
				wscU2KMsgLen, ioctl_sock);

	//wsc_hexdump("U2KMsg", pWscU2KMsg, wscU2KMsgLen);
	
	//sht:look for right interface,then send the msg to kernel space.
#ifdef SECOND_WIFI//sht??? mutex protect,race condtion
		if (strcmp("/control", service->control_url) == 0)//"/control" is 2.4G device service.
		{
			memset(&WSC_IOCTL_IF[0], 0, IFNAMSIZ);
			strcpy(&WSC_IOCTL_IF[0], "ra0");
			//diag_printf("!!WscDevPutWLANResponse:ra0\n");
		}	
		else
		{
			memset(&WSC_IOCTL_IF[0], 0, IFNAMSIZ);
			strcpy(&WSC_IOCTL_IF[0], "rai0");
			//diag_printf("!!WscDevPutWLANResponse:rai0\n");
		}	
#endif		
	// Now send the msg to kernel space.
	if (ioctl_sock >= 0)
		retVal = wsc_set_oid(RT_OID_WSC_EAPMSG, (char *)pWscU2KMsg, wscU2KMsgLen);

	// Waiting for the response from the kernel space.
	DBGPRINTF(RT_DBG_INFO, "ioctl retval=%d! senderID=%d!\n", retVal, senderID);

	if(retVal != 0)
	{
		DBGPRINTF(RT_DBG_INFO, "(%s): failed when send the msg to kernel space\n", __FUNCTION__);
	}

/*
	if (*out != NULL)
	{
		free(*out);
	}
*/
done:

	if (inStr)
	{
//		free(inStr);
	}
	if (decodeStr)
		free(decodeStr);
	if (pWscU2KMsg)
		free(pWscU2KMsg);
	
	if (retVal == 0)
		return 0;
	else 
	{
		(*errorString) = "Internal Error";
		return -1;
	}
}


/******************************************************************************
 * WscDevPutMessageResp
 *
 * Description: 
 *       This action used by Registrar send WSC_MSG(M2,M4,M6, M8) to Enrollee.
 *
 * Parameters:
 *    
 *    IXML_Document * in - document of action request
 *    IXML_Document **out - action result
 *    char **errorString - errorString (in case action was unsuccessful)
 *
 * Return:
 *    0 - if success
 *    -1 - if failure
 *****************************************************************************/
int 
WscDevPutMessageResp(
	IN WscEnvelope *msgEnvelope,
	IN UPNP_CONTEXT *context)
{
	int retVal= -1;
	char *pWscU2KMsg = NULL;
//	char *out = NULL; 
//	struct upnphttp * h = msgEnvelope->h;
	OUT_ARGUMENT *out_argument = context->out_arguments;

	DBGPRINTF(RT_DBG_INFO, "Got msg from netlink! envID=0x%x!\n", msgEnvelope->envID);

//	( out ) = malloc(2500);

	if ((msgEnvelope->flag != WSC_ENVELOPE_SUCCESS) || 
		(msgEnvelope->pMsgPtr == NULL) || 
		(msgEnvelope->msgLen == 0))
	{
		goto done;
	}

	// Response the msg from state machine back to the remote UPnP control device.
	if (msgEnvelope->pMsgPtr)
	{
		unsigned char *encodeStr = NULL;
		RTMP_WSC_MSG_HDR *rtmpHdr = NULL;		
		int len;

		DBGPRINTF(RT_DBG_INFO, "(%s):Ready to parse the K2U msg(len=%d)!\n", __FUNCTION__, msgEnvelope->msgLen);
		//wsc_hexdump("WscDevPutMessage-K2UMsg", msgEnvelope->pMsgPtr, msgEnvelope->msgLen);
		
		rtmpHdr = (RTMP_WSC_MSG_HDR *)(msgEnvelope->pMsgPtr);
	
		if(rtmpHdr->msgType == WSC_OPCODE_UPNP_CTRL || rtmpHdr->msgType == WSC_OPCODE_UPNP_MGMT)
		{
			DBGPRINTF(RT_DBG_INFO, "Receive a UPnP Ctrl/Mgmt Msg!\n");
			goto done;
		}
		
		if(rtmpHdr->msgSubType == WSC_UPNP_DATA_SUB_ACK)
		{
			out_argument->next = NULL;
			out_argument->name = NULL;
			out_argument->type = UPNP_TYPE_STR;

			retVal = 0;

		}
		else 
		{
			len = ILibBase64Encode((unsigned char *)((unsigned char *)(msgEnvelope->pMsgPtr) + sizeof(RTMP_WSC_MSG_HDR)), 
									rtmpHdr->msgLen, &encodeStr);
/*
			if (len >0)
				retVal = UpnpAddToActionResponse(&out, "PutMessage", WscServiceTypeStr, 
													"NewOutMessage", (char *)encodeStr);
*/
			out_argument->next = NULL;
			out_argument->name = "NewOutMessage";
			out_argument->type = UPNP_TYPE_STR;

			strncpy(out_argument->value.str, encodeStr, len/* strlen((char *)encodeStr) */);
			out_argument->value.str[len] = '\0';			

			retVal = 0;

			if (encodeStr != NULL)
				free(encodeStr);
		}

/*
		if (out != NULL)
		{
			free(out);
		}
*/
	} 

done:
	if (pWscU2KMsg)
		free(pWscU2KMsg);
//	wscEnvelopeFree(msgEnvelope);

	if (retVal == 0)
	{
		return retVal;
	}
	else
	{
		return -1;
	}
}



/******************************************************************************
 * WscDevPutMessage
 *
 * Description: 
 *       This action used by Registrar request WSC_MSG(M2,M4,M6, M8) from WIFI driver to Enrollee.
 *
 * Parameters:
 *    
 *    IXML_Document * in - document of action request
 *    IXML_Document **out - action result
 *    char **errorString - errorString (in case action was unsuccessful)
 *
 * Return:
 *    0 - if success
 *    -1 - if failure
 *****************************************************************************/
int 
WscDevPutMessage(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **errorString)
{
	char *inStr = NULL;
	int inStrLen=0;
	unsigned char *decodeStr = NULL;
	int decodeLen, retVal = -1;
	
	char *pWscU2KMsg = NULL;
	RTMP_WSC_U2KMSG_HDR *msgHdr = NULL;
	WscEnvelope *msgEnvelope = NULL;
	int wscU2KMsgLen;
	int msgQIdx = -1;
	unsigned long ipaddr;
		
	(*errorString) = NULL;
	ipaddr = ntohl(context->dst_addr.sin_addr.s_addr);

//	wsc_chardump("context->content", (char *)(context->content), context->content_len);
//	wsc_chardump("in_argument->value", (char *)(in_argument->value.str), strlen(in_argument->value.str));

//	printf("--->%s\n", __FUNCTION__);	
//	printf("strlen(in_argument->value.str)=%d\n", strlen(in_argument->value.str));	

	inStr = in_argument->value.str;
	inStrLen = strlen(in_argument->value.str);

	ASSERT(inStrLen > 0);
	if (!inStr)
	{
		(*errorString) = "Invalid PutMessage mesg";
		perror("402 : Invalid Args");	
		return retVal;
	}
	decodeLen = ILibBase64Decode((unsigned char *)inStr, inStrLen, &decodeStr);
	if (decodeLen == 0 || decodeStr == NULL)
	{
		goto done;
	}

	/* Prepare the msg buffers */
	wscU2KMsgLen = wscU2KMsgCreate(&pWscU2KMsg, (char *)decodeStr, decodeLen, EAP_FRAME_TYPE_WSC);
	if (wscU2KMsgLen == 0)
		goto done;

	/* Prepare the msg envelope */
	if ((msgEnvelope = wscEnvelopeCreate()) == NULL)
		goto done;
	
	/* Lock the msgQ and check if we can get a valid mailbox to insert our request! */
	if(wscMsgQInsert(msgEnvelope, &msgQIdx)!= WSC_SYS_SUCCESS)
	{
		goto done;
	}

	/* log the socket ID and callback in the msgEnvelope */
	msgEnvelope->DevCallBack = WscDevPutMessageResp;
	
	// Fill the sessionID, Addr1, and Adde2 to the U2KMsg buffer header.
	msgHdr = (RTMP_WSC_U2KMSG_HDR *)pWscU2KMsg;
	msgHdr->envID = msgEnvelope->envID;
#ifndef SECOND_WIFI	
	memcpy(msgHdr->Addr1, &HostMacAddr[0], MAC_ADDR_LEN);
#endif
	memcpy(msgHdr->Addr2, /* ip ? mac ? */(char *)(&ipaddr)/* ipAddr */, sizeof(unsigned long));
	
	DBGPRINTF(RT_DBG_OFF, "(%s):Ready to send pWscU2KMsg(len=%d) to kernel by ioctl(%d)!\n", __FUNCTION__, 
				wscU2KMsgLen, ioctl_sock);
	//wsc_hexdump("U2KMsg", pWscU2KMsg, wscU2KMsgLen);
	
	//sht:look for right interface,then send the msg to kernel space.
#ifdef SECOND_WIFI//sht??? mutex protect,race condtion
		if (strcmp("/control", service->control_url) == 0)//"/control" is 2.4G device service.
		{
			memcpy(msgHdr->Addr1, HostMacAddr[0], MAC_ADDR_LEN);
			memset(&WSC_IOCTL_IF[0], 0, IFNAMSIZ);
			strcpy(&WSC_IOCTL_IF[0], "ra0");
			//diag_printf("!!WscDevPutMessage:ra0\n");
		}	
		else
		{	memcpy(msgHdr->Addr1, HostMacAddr[1], MAC_ADDR_LEN);
			memset(&WSC_IOCTL_IF[0], 0, IFNAMSIZ);
			strcpy(&WSC_IOCTL_IF[0], "rai0");	
			//diag_printf("!!WscDevPutMessage:rai0\n");
		}	
#endif	

	// Now send the msg to kernel space.
	if (ioctl_sock >= 0)
		retVal = wsc_set_oid(RT_OID_WSC_EAPMSG, (char *)pWscU2KMsg, wscU2KMsgLen);

	if (retVal == 0)
	{
		// Waiting for the response from the kernel space.
		DBGPRINTF(RT_DBG_INFO, "%s():ioctl to kernel success, waiting for condition!\n", __FUNCTION__);

		if (inStr)
//			free(inStr);// I have no right to free this pointer, because the buffer is not alloc by me.
		if (decodeStr)
			free(decodeStr);
		if (pWscU2KMsg)
			free(pWscU2KMsg);
		return retVal;
	}
	else
	{
		DBGPRINTF(RT_DBG_INFO, "%s():ioctl to kernel failed, retVal=%d, goto done!\n", __FUNCTION__, retVal);
		wscMsgQRemove(Q_TYPE_PASSIVE, msgQIdx);
		goto done;
	}

done:
	if (inStr)
	{
//		free(inStr);// I have no right to free this pointer, because the buffer is not alloc by me.
	}
	if (decodeStr)
		free(decodeStr);
	if (pWscU2KMsg)
		free(pWscU2KMsg);

	wscEnvelopeFree(msgEnvelope);

	if (retVal == 0)
	{
		return retVal;
	}
	else
	{
		(*errorString) = "Internal Error";
		return -1;
	}
}


/******************************************************************************
 * WscDevGetDeviceInfoResp
 *
 * Description: 
 *       This action callback used to send AP's M1 message to the Control Point(Registrar).
 *
 * Parameters:
 *
 *    WscEnvelope *msgEnvelope  - response from wifi driver of action request
 *    
 *    
 *    
 *
 * Return:
 *    0 - if success
 *    -1 - if failure
 *****************************************************************************/
int
WscDevGetDeviceInfoResp(
	IN WscEnvelope *msgEnvelope,
	IN UPNP_CONTEXT *context)
{
	int retVal= -1;
	char *pWscU2KMsg = NULL;
//	char *out = NULL; 
//	struct upnphttp * h = msgEnvelope->h;
	OUT_ARGUMENT *out_argument = context->out_arguments;

	DBGPRINTF(RT_DBG_INFO, "(%s):Got msg from netlink! envID=0x%x, flag=%d!\n", 
				__FUNCTION__, msgEnvelope->envID, msgEnvelope->flag);

//	( out ) = malloc(2500);

	if ((msgEnvelope->flag != WSC_ENVELOPE_SUCCESS) || 
		(msgEnvelope->pMsgPtr == NULL) || 
		(msgEnvelope->msgLen == 0))
	{
		goto done;
	}

	// Response the msg from state machine back to the remote UPnP control device.
	if (msgEnvelope->pMsgPtr)
	{
		unsigned char *encodeStr = NULL;
		RTMP_WSC_MSG_HDR *rtmpHdr = NULL;		
		int len;

		rtmpHdr = (RTMP_WSC_MSG_HDR *)(msgEnvelope->pMsgPtr);
		DBGPRINTF(RT_DBG_INFO, "(%s):Ready to parse the K2U msg(TotalLen=%d, headerLen=%d)!\n", __FUNCTION__, 
						msgEnvelope->msgLen, sizeof(RTMP_WSC_MSG_HDR));
		DBGPRINTF(RT_DBG_INFO, "\tMsgType=%d!\n" 
								"\tMsgSubType=%d!\n"
								"\tipAddress=0x%x!\n"
								"\tMsgLen=%d!\n",
								rtmpHdr->msgType, rtmpHdr->msgSubType, rtmpHdr->ipAddr, rtmpHdr->msgLen);
		
		if(rtmpHdr->msgType == WSC_OPCODE_UPNP_CTRL || rtmpHdr->msgType == WSC_OPCODE_UPNP_MGMT)
		{
			DBGPRINTF(RT_DBG_INFO, "Receive a UPnP Ctrl/Mgmt Msg!\n");
			goto done;
		}
		//wsc_hexdump("WscDevGetDeviceInfoResp-K2UMsg", msgEnvelope->pMsgPtr, msgEnvelope->msgLen);
		
		if(rtmpHdr->msgSubType == WSC_UPNP_DATA_SUB_ACK)
		{
			out_argument->next = NULL;
			out_argument->name = NULL;
			out_argument->type = UPNP_TYPE_STR;

			retVal = 0;
		}
		else
		{
			len = ILibBase64Encode((unsigned char *)((unsigned char *)(msgEnvelope->pMsgPtr) + sizeof(RTMP_WSC_MSG_HDR)), 
									rtmpHdr->msgLen, &encodeStr);
			if (len > 0)
			{
/*
				retVal = UpnpAddToActionResponse(&out, "GetDeviceInfo", WscServiceTypeStr, 
												"NewDeviceInfo", (char *)encodeStr);
*/
				out_argument->next = NULL;
				out_argument->name = "NewDeviceInfo";
				out_argument->type = UPNP_TYPE_STR;
				strncpy(out_argument->value.str, encodeStr, len/* strlen((char *)encodeStr) */);
				out_argument->value.str[len] = '\0';

				retVal = 0;
			}

			if (encodeStr != NULL)
			{
				free(encodeStr);
			}
		}

/*
		if (out != NULL)
		{
			free(out);
		}
*/
	} 


done:
	if (pWscU2KMsg)
	{
		free(pWscU2KMsg);
	}

//	wscEnvelopeFree(msgEnvelope);

//	printf("<---WscDevGetDeviceInfoResp\n");	

	if (retVal == 0)
	{
		return retVal;
	}
	else
	{
		return -1;
	}

}

	
/******************************************************************************
 * WscDevGetDeviceInfo
 *
 * Description: 
 *       This action callback used to send AP's M1 message to the Control Point(Registrar).
 *
 * Parameters:
 *
 *    IXML_Document * in  - document of action request
 *    uint32 ipAddr       - ipAddr,
 *    IXML_Document **out - action result
 *    char **errorString  - errorString (in case action was unsuccessful)
 *
 * Return:
 *    0 - if success
 *    -1 - if failure
 *****************************************************************************/
int
WscDevGetDeviceInfo(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **errorString)
{
	char UPnPIdentity[] = {"WFA-SimpleConfig-Registrar"};
	int retVal= -1;
	char *pWscU2KMsg = NULL;
	WscEnvelope *msgEnvelope = NULL;
	RTMP_WSC_U2KMSG_HDR *msgHdr = NULL;
	int wscU2KMsgLen;
	int msgQIdx = -1;
	unsigned long ipaddr;	
	
	DBGPRINTF(RT_DBG_INFO, "Receive a GetDeviceInfo msg from Remote Upnp Control Point!\n");
	
	(*errorString) = NULL;
	ipaddr = ntohl(context->dst_addr.sin_addr.s_addr);
	
	/* Prepare the msg buffers */
	wscU2KMsgLen = wscU2KMsgCreate(&pWscU2KMsg, UPnPIdentity, strlen(UPnPIdentity), EAP_FRAME_TYPE_IDENTITY);
	if (wscU2KMsgLen == 0)
		goto done;

	/* Prepare the msg envelope */
	if ((msgEnvelope = wscEnvelopeCreate()) == NULL)
		goto done;
	
	/* Lock the msgQ and check if we can get a valid mailbox to insert our request! */
	if (wscMsgQInsert(msgEnvelope, &msgQIdx) != WSC_SYS_SUCCESS)
	{
		goto done;
	}

	/* log the http info and callback at the msgEnvelope */
	msgEnvelope->DevCallBack = WscDevGetDeviceInfoResp;
	
	// Fill the sessionID, Addr1, and Adde2 to the U2KMsg buffer header.
	msgHdr = (RTMP_WSC_U2KMSG_HDR *)pWscU2KMsg;
	msgHdr->envID = msgEnvelope->envID;
#ifndef SECOND_WIFI
	memcpy(msgHdr->Addr1, &HostMacAddr[0], MAC_ADDR_LEN);
#endif
	memcpy(msgHdr->Addr2, /* ip ? mac ? */(char *)(&ipaddr)/* ipAddr */, sizeof(int));
	
	// Now send the msg to kernel space.
	DBGPRINTF(RT_DBG_INFO, "(%s):Ready to send pWscU2KMsg(len=%d) to kernel by ioctl(%d)!\n", __FUNCTION__, 
				wscU2KMsgLen, ioctl_sock);
	//sht:look for right interface,then send the msg to kernel space.
#ifdef SECOND_WIFI//sht??? mutex protect,race condtion
		if (strcmp("/control", service->control_url) == 0)//"/control" is 2.4G device service.
		{
			memcpy(msgHdr->Addr1, HostMacAddr[0], MAC_ADDR_LEN);
			memset(&WSC_IOCTL_IF[0], 0, IFNAMSIZ);
			strcpy(&WSC_IOCTL_IF[0], "ra0");
			//diag_printf("!!WscDevGetDeviceInfo:ra0\n");
		}	
		else
		{
			memcpy(msgHdr->Addr1, HostMacAddr[1], MAC_ADDR_LEN);
			memset(&WSC_IOCTL_IF[0], 0, IFNAMSIZ);
			strcpy(&WSC_IOCTL_IF[0], "rai0");	
			//diag_printf("!!WscDevGetDeviceInfo:rai0\n");
		}
#endif
	if (ioctl_sock >= 0)
		retVal = wsc_set_oid(RT_OID_WSC_EAPMSG, pWscU2KMsg, wscU2KMsgLen);
	else
		retVal = -1;
	
	if (retVal == 0)
	{
		// Waiting for the response from the kernel space.
		DBGPRINTF(RT_DBG_INFO, "%s():ioctl to kernel success, waiting for response!\n", __FUNCTION__);

		if (pWscU2KMsg)
			free(pWscU2KMsg);
		return retVal;
	}
	else
	{
		DBGPRINTF(RT_DBG_ERROR, "%s():ioctl to kernel failed, retVal=%d, goto done!\n", __FUNCTION__, retVal);
		wscMsgQRemove(Q_TYPE_PASSIVE, msgQIdx);
		goto done;
	}

	/* moved to WscDevGetDeviceInfoResp() */

done:
	if (pWscU2KMsg)
		free(pWscU2KMsg);
	
	wscEnvelopeFree(msgEnvelope);
	
	if (retVal == 0)
		return retVal;
	else
	{
		(*errorString) = "Internal Error";
		return -1;
	}
}
/*---------------------------end of action handler---------------------------*/


/******************************************************************************
 * WscDevCPNodeInsert
 *
 * Description: 
 *       Depends on the UDN String, service identifier and Subscription ID, find 
 *		 the IP address of the remote Control Point. And insert the Control Point
 *       into the "WscCPList".  
 *
 * NOTE: 
 *		 Since this function blocks on the mutex WscDevMutex, to avoid a hang this 
 *		 function should not be called within any other function that currently has 
 *		 this mutex locked.
 *
 *		 Besides, this function use the internal data structure of libupnp-1.3.1 
 *		 SDK, so if you wanna change the libupnp to other versions, please make sure
 *		 the data structure refered in this function was not changed!
 *
 * Parameters:
 *   UpnpDevice_Handle  - UpnpDevice handle of WscLocalDevice assigned by UPnP SDK.
 *   char *UDN			- The UUID string of the Subscription requested.
 *   char *servId 		- The service Identifier of the Subscription requested.
 *   char *sid 			- The Subscription ID of the ControlPoint.
 * 
 * Return:
 *   TRUE  - If success
 *   FALSE - If failure
 *****************************************************************************/
static int 
WscDevCPNodeInsert(
	IN int device_handle,
	IN const char *sid)
{
	int found = 0;
	struct upnpCPNode *CPNode, *newCPNode;
	
	HandleLock();

	CPNode = WscCPList;
	while(CPNode)
	{
		if (strcmp(CPNode->device.SID, sid) == 0)
		{
			found = 1;
			break;
		}
		CPNode = CPNode->next;
	}

	if (found)
	{
		strncpy(CPNode->device.SID, sid, NAME_SIZE);
	}
	else
	{
		// It's a new subscriber, insert it.
		if ((newCPNode = malloc(sizeof(struct upnpCPNode))) != NULL)
		{
			memset(newCPNode, 0, sizeof(struct upnpCPNode));
			strncpy(newCPNode->device.SID, sid, NAME_SIZE);
			newCPNode->next = NULL;

			if(WscCPList)
			{
				newCPNode->next = WscCPList->next;
				WscCPList->next = newCPNode;
			}
			else
			{
				WscCPList = newCPNode;
			}
		}
		else 
		{
			goto Fail;
		}
	}

	HandleUnlock();
	DBGPRINTF(RT_DBG_INFO, "Insert ControlPoint success!\n");
		
	return TRUE;

Fail:
	
	HandleUnlock();
	DBGPRINTF(RT_DBG_ERROR, "Insert ControlPoint failed!\n");	
	return FALSE;
}


/******************************************************************************
 * WscDevCPNodeSearch
 *
 * Description: 
 *   Search for specific Control Point Node by ip address in WscCPList
 *
 * Parameters:
 *   unsigned int ipAddr - ip address of the contorl point we want to seach.
 *   char *sid           - used to copy the SID string
 *
 * Return:
 *   1 - if found
 *   0 - if not found
 *****************************************************************************/
static int 
WscDevCPNodeSearch(
	IN unsigned int ipAddr,
	OUT char **strSID)
{
	struct upnpCPNode *CPNode;
	int found = 0;
	
	
	CPNode = WscCPList;
	while (CPNode)
	{
		if(CPNode->device.ipAddr == ipAddr)
		{
			*strSID = strdup(CPNode->device.SID);
			found = 1;
			break;
		}
		CPNode = CPNode->next;
	}

	return found;
}


/******************************************************************************
 * WscDevCPListRemoveAll
 *
 * Description: 
 *   Remove all Control Point Node in WscCPList
 *
 * Parameters:
 *   None
 *
 * Return:
 *   TRUE
 *****************************************************************************/
static int WscDevCPListRemoveAll(void)
{
	struct upnpCPNode *CPNode;

	while((CPNode = WscCPList))
	{
		WscCPList = CPNode->next;
		free(CPNode);
	}
	
	return TRUE;
}


/******************************************************************************
 * WscDevCPNodeRemove
 *
 * Description: 
 *       Remove the ControlPoint Node in WscCPList depends on the subscription ID.
 *
 * Parameters:
 *   char *SID - The subscription ID of the ControlPoint will be deleted
 * 
 * Return:
 *   TRUE  - If success
 *   FALSE - If failure
 *****************************************************************************/
int WscDevCPNodeRemove(IN char *SID)
{
	struct upnpCPNode *CPNode, *prevNode = NULL;
	
	CPNode = prevNode = WscCPList;

	if(strcmp(WscCPList->device.SID, SID) == 0)
	{
		WscCPList = WscCPList->next;
		free(CPNode);
	} 
	else
	{
		while((CPNode = CPNode->next))
		{
			if(strcmp(CPNode->device.SID, SID) == 0)
			{
				prevNode->next = CPNode->next;
				free(CPNode);
				break;
			}
			prevNode = CPNode;
		}
	}

	return TRUE;
}


/******************************************************************************
 * dumpDevCPNodeList
 *
 * Description: 
 *       Dump the WscCPList. Used for debug.
 *
 * Parameters:
 *   	void
 *
 *****************************************************************************/
void dumpDevCPNodeList(void)
{
	struct upnpCPNode *CPNode;
	int i=0;
	
	printf("#Dump The UPnP Subscribed ControlPoint:\n");

	CPNode = WscCPList;	
	while(CPNode)
	{
		i++;
		printf("ControlPoint Node[%d]:\n", i);
		printf("\t ->sid=%s\n", CPNode->device.SID);
//		printf("\t ->SubURL=%s\n", CPNode->device.SubURL);
		printf("\t ->ipAddr=0x%x!\n", CPNode->device.ipAddr);
//		printf("\t ->SubTimeOut=%d\n", CPNode->device.SubTimeOut);
		CPNode = CPNode->next;
	}

}


/******************************************************************************
 * WscDevStateVarUpdate
 *
 * Description: 
 *       Update the Wsc Device Service State variable, and notify all subscribed 
 *       Control Points of the updated state.  Note that since this function
 *       blocks on the mutex WscDevMutex, to avoid a hang this function should 
 *       not be called within any other function that currently has this mutex 
 *       locked.
 *
 * Parameters:
 *   variable -- The variable number (WSC_EVENT_WLANEVENT, WSC_EVENT_APSTATUS,
 *                   WSC_EVENT_STASTATUS)
 *   value -- The string representation of the new value
 *
 *****************************************************************************/
static int
WscDevStateVarUpdate(
	IN unsigned int variable,
	IN char *value,
	IN char *SubsId,
	IN int interfaceIndex)
{
       	UPNP_SERVICE *service=NULL;
	
	if((variable >= WSC_STATE_VAR_MAXVARS) || (strlen(value) >= WSC_STATE_VAR_MAX_STR_LEN))
		return (0);
	#ifdef SECOND_WIFI
	if(interfaceIndex == 1)
	{
		//diag_printf("!!WscDevStateVarUpdate:ra0\n");
		strcpy(wscLocalDevice[0].services.StateVarVal[variable], value);//for 2.4G wifi interface	
	}
	else if(interfaceIndex == 2)
	{
		//diag_printf("!!WscDevStateVarUpdate:rai0\n");
		strcpy(wscLocalDevice[1].services.StateVarVal[variable], value);//for 5G wifi interface
	}
	#else
	if(interfaceIndex == 1)
		strcpy(wscLocalDevice.services.StateVarVal[variable], value);//for 2.4G wifi interface
	#endif	
		
    DBGPRINTF(RT_DBG_INFO, "%s: %d\n", __FUNCTION__, variable);

	cyg_mutex_lock(&upnp_service_table_mutex);
	/* find event */
	if(interfaceIndex == 1)	
		service = find_event("/event");//2.4G event
	#ifdef SECOND_WIFI
	else if(interfaceIndex == 2)
		service = find_event("/event?5G");//5G event
	#endif	
		
    if (service == NULL)
    {   
    	cyg_mutex_unlock(&upnp_service_table_mutex);
    	DBGPRINTF(RT_DBG_ERROR, "%s: Can not find the service url=/event \n", __FUNCTION__);
		return (0);
    }
        
	switch (variable)
	{
		case WSC_EVENT_WLANEVENT:
		    gena_update_event_var(service, "WLANEvent", value);//sht:????? 2.4G/5G event
			gena_notify(&upnp_context, service, NULL);		
			/* Do we need to call gena_notify_complete(service) here? */
			gena_notify_complete(service);
			break;
		
		case WSC_EVENT_APSTATUS:
			/* never be this case */
			gena_update_event_var(service, "APStatus", value);
			gena_notify(&upnp_context, service, SubsId);
			gena_notify_complete(service);
			break;
		
		case WSC_EVENT_STASTATUS:
			/* never be this case */
		    gena_update_event_var(service, "STAStatus", value);
			gena_notify(&upnp_context, service, SubsId);
			gena_notify_complete(service);
			break;
		
		default:
			/* should not be reached */
			perror("unknown state variable event!"); 
			break;
	}
	cyg_mutex_unlock(&upnp_service_table_mutex);
	
    return (1);
}


/*
	The format of UPnP WLANEvent message send to Remote Registrar.
	  WLANEvent:
		=>This variable represents the concatenation of the WLANEventType, WLANEventMac and the 
		  802.11 WSC message received from the Enrollee & forwarded by the proxy over UPnP.
		=>the format of this variable
			->represented as base64(WLANEventType || WLANEventMAC || Enrollee's message).
	  WLANEventType:
		=>This variable represents the type of WLANEvent frame that was received by the proxy on 
		  the 802.11 network.
		=>the options for this variable
			->1: 802.11 WCN-NET Probe Frame
			->2: 802.11 WCN-NET 802.1X/EAP Frame
	  WLANEventMAC:
		=>This variable represents the MAC address of the WLAN Enrollee that generated the 802.11 
	 	  frame that was received by the proxy.
	 	=>Depends on the WFAWLANConfig:1  Service Template Version 1.01, the format is
 			->"xx:xx:xx:xx:xx:xx", case-independent, 17 char
*/	
int WscEventCtrlMsgRecv(
	IN char *pBuf,
	IN int  bufLen,
	IN int interfaceIndex)
{

	DBGPRINTF(RT_DBG_INFO, "Receive a Control Message!\n");
	return 0;

}


//receive a WSC message from kernal space(wifi driver) and send it to remote UPnP Contorl Point
int WscEventDataMsgRecv(
	IN char *pBuf,
	IN int  bufLen,
	IN int  interfaceIndex)
{
	RTMP_WSC_MSG_HDR *pHdr = NULL;
	unsigned char *encodeStr = NULL, *pWscMsg = NULL, *pUPnPMsg = NULL;
	int encodeLen = 0, UPnPMsgLen = 0;
	int retVal;
	uint32 wscLen;
	char *strSID = NULL;
	unsigned char includeMAC = 0;
	char curMacStr[18];
	
	if ((WscUPnPOpMode & WSC_UPNP_OPMODE_DEV) == 0)
		return -1;
		
	pHdr = (RTMP_WSC_MSG_HDR *)pBuf;
		
	// Find the SID.
	if (WscDevCPNodeSearch(pHdr->ipAddr, &strSID) == 0)
	{
		DBGPRINTF(RT_DBG_INFO, "%s(): Didn't find the SID by ip(0x%x)!\n", __FUNCTION__, pHdr->ipAddr);
	}
	else
	{
		DBGPRINTF(RT_DBG_INFO, "%s(): The SID(%s) by ip(0x%x)!\n", __FUNCTION__, strSID, pHdr->ipAddr);
	}

	DBGPRINTF(RT_DBG_INFO, "%s:Receive a Data event, msgSubType=%d!\n", __FUNCTION__, pHdr->msgSubType);

	memset(curMacStr, 0 , sizeof(curMacStr));
	if ((pHdr->msgSubType & WSC_UPNP_DATA_SUB_INCLUDE_MAC) == WSC_UPNP_DATA_SUB_INCLUDE_MAC)
	{
		if (pHdr->msgLen < 6){
			if (strSID)
				free(strSID);	
			DBGPRINTF(RT_DBG_ERROR, "pHdr->msgLen(%d) didn't have enoguh length!\n", pHdr->msgLen);
			return -1;
		}
		includeMAC = 1;
		pHdr->msgSubType &= 0x00ff;
		pWscMsg = (unsigned char *)(pBuf + sizeof(RTMP_WSC_MSG_HDR));
		snprintf(curMacStr, 18, "%02x:%02x:%02x:%02x:%02x:%02x", pWscMsg[0], pWscMsg[1], pWscMsg[2], pWscMsg[3],pWscMsg[4],pWscMsg[5]);	
	}
	else
	{
		if(interfaceIndex==1)
			memcpy(&curMacStr[0], CfgMacStr[0], 17);
		#ifdef SECOND_WIFI
		else if(interfaceIndex==2)
			memcpy(&curMacStr[0], CfgMacStr[1], 17);
		#endif
		else
		{
			if (strSID)
				free(strSID);
			DBGPRINTF(RT_DBG_ERROR, "interfaceIndex is out of range!\n");
			return -1;	
		}	
	}

	
	if (pHdr->msgSubType == WSC_UPNP_DATA_SUB_NORMAL || 
		pHdr->msgSubType == WSC_UPNP_DATA_SUB_TO_ALL ||
		pHdr->msgSubType == WSC_UPNP_DATA_SUB_TO_ONE)
	{

		DBGPRINTF(RT_DBG_INFO, "(%s):Ready to parse the K2U msg(len=%d)!\n", __FUNCTION__, bufLen);

		pWscMsg = (unsigned char *)(pBuf + sizeof(RTMP_WSC_MSG_HDR));
		//Get the WscData Length
		wscLen = pHdr->msgLen;
		
		if (includeMAC)
		{
			wscLen -= MAC_ADDR_LEN;
			pWscMsg += MAC_ADDR_LEN;
		}
		
		DBGPRINTF(RT_DBG_INFO, "(%s): pWscMsg Len=%d!\n", __FUNCTION__, wscLen);
			
		UPnPMsgLen = wscLen + 18;
		pUPnPMsg = malloc(UPnPMsgLen);
		if(pUPnPMsg)
		{
			memset(pUPnPMsg, 0, UPnPMsgLen);
			pUPnPMsg[0] = WSC_EVENT_WLANEVENTTYPE_EAP;
			memcpy(&pUPnPMsg[1], &curMacStr[0], 17);

			//Copy the WscMsg to pUPnPMsg buffer
			memcpy(&pUPnPMsg[18], pWscMsg, wscLen);
			//wsc_hexdump("UPnP WLANEVENT Msg", (char *)pUPnPMsg, UPnPMsgLen);

			//encode the message use base64
			encodeLen = ILibBase64Encode(pUPnPMsg, UPnPMsgLen, &encodeStr);
			
			//Send event out
			if (encodeLen > 0)
			{
				DBGPRINTF(RT_DBG_INFO, "EAP->Msg=%s!\n", encodeStr);
				retVal = WscDevStateVarUpdate(WSC_EVENT_WLANEVENT, (char *)encodeStr, strSID,interfaceIndex);
			}
			if (encodeStr != NULL)
				free(encodeStr);
			free(pUPnPMsg);
		}
	}
	
	if (strSID)
		free(strSID);
	
	return 0;
	
}


/*
	Format of iwcustom msg WSC RegistrarSelected message:
	1. The Registrar ID which send M2 to the Enrollee(4 bytes):
								
			  4
		+-----------+
		|RegistrarID|
*/
static int WscEventMgmt_RegSelect(	
	IN char *pBuf,
	IN int  bufLen,
	IN int interfaceIndex)
{
	char *pWscMsg = NULL;
	int registrarID = 0;
		
	if ((WscUPnPOpMode & WSC_UPNP_OPMODE_DEV) == 0)
		return -1;
	
	DBGPRINTF(RT_DBG_INFO, "(%s):Ready to parse the K2U msg(len=%d)!\n", __FUNCTION__, bufLen);
	//wsc_hexdump("WscEventMgmt_RegSelect-K2UMsg", pBuf, bufLen);

	pWscMsg = (char *)(pBuf + sizeof(RTMP_WSC_MSG_HDR));

	registrarID = *((int *)pWscMsg);
	DBGPRINTF(RT_DBG_INFO, "The registrarID=%d!\n", registrarID);
	
	return 0;

}


/*
	Format of iwcustom msg WSC clientJoin message:
	1. SSID which station want to probe(32 bytes):
			<SSID string>
		*If the length if SSID string is small than 32 bytes, fill 0x0 for remaining bytes.
	2. Station MAC address(6 bytes):
	3. Status:
		Value 1 means change APStatus as 1. 
		Value 2 means change STAStatus as 1.
		Value 3 means trigger msg.
								
			32         6       1 
		+----------+--------+------+
		|SSIDString| SrcMAC |Status|
*/
static int WscEventMgmt_ConfigReq(
	IN char *pBuf,
	IN int  bufLen,
	IN int interfaceIndex)
{
	unsigned char *encodeStr = NULL, *pWscMsg = NULL, *pUPnPMsg = NULL;
	int encodeLen = 0, UPnPMsgLen = 0;
	int retVal;
	unsigned char Status;
	char triggerMac[18];
		
	
	DBGPRINTF(RT_DBG_INFO, "(%s):Ready to parse the K2U msg(len=%d)!\n", __FUNCTION__, bufLen);
	//wsc_hexdump("WscEventMgmt_ConfigReq-K2UMsg", pBuf, bufLen);

	pWscMsg = (unsigned char *)(pBuf + sizeof(RTMP_WSC_MSG_HDR));

	memset(&triggerMac[0], 0, 18);
	memcpy(&triggerMac[0], pWscMsg, 18);

	//Skip the SSID field
	pWscMsg += 32;
			
	//Skip the SrcMAC field
	pWscMsg += 6;

	//Change APStatus and STAStatus
	Status = *(pWscMsg);
	DBGPRINTF(RT_DBG_INFO, "(%s): Status =%d!\n", __FUNCTION__, Status);

	if ((WscUPnPOpMode & WSC_UPNP_OPMODE_DEV) && (strlen(triggerMac) == 0))
	{
		#ifdef SECOND_WIFI
		if(interfaceIndex==1)
		{
			strcpy(wscLocalDevice[0].services.StateVarVal[WSC_EVENT_APSTATUS], "0");
			strcpy(wscLocalDevice[0].services.StateVarVal[WSC_EVENT_STASTATUS], "0");
			//diag_printf("!!WscEventMgmt_ConfigReq:ra0\n");
		}
		else if(interfaceIndex==2)
		{
			strcpy(wscLocalDevice[1].services.StateVarVal[WSC_EVENT_APSTATUS], "0");
			strcpy(wscLocalDevice[1].services.StateVarVal[WSC_EVENT_STASTATUS], "0");
			//diag_printf("!!WscEventMgmt_ConfigReq:rai0\n");
		}
		#else
		if(interfaceIndex==1)
		{
			strcpy(wscLocalDevice.services.StateVarVal[WSC_EVENT_APSTATUS], "0");
			strcpy(wscLocalDevice.services.StateVarVal[WSC_EVENT_STASTATUS], "0");
		}
        #endif
		//Prepare the message.
		UPnPMsgLen = 18 + sizeof(wscAckMsg);
		pUPnPMsg = malloc(UPnPMsgLen);
		
		if(pUPnPMsg)
		{
			memset(pUPnPMsg, 0, UPnPMsgLen);
			pUPnPMsg[0] = WSC_EVENT_WLANEVENTTYPE_EAP;
			
			if(interfaceIndex==1)
				memcpy(&pUPnPMsg[1], CfgMacStr[0], 17);
			#ifdef SECOND_WIFI
			else if(interfaceIndex==2)
				memcpy(&pUPnPMsg[1], CfgMacStr[1], 17);
			#endif

			//jump to the WscProbeReqData and copy to pUPnPMsg buffer
			pWscMsg++;	
			memcpy(&pUPnPMsg[18], wscAckMsg, sizeof(wscAckMsg));
			//wsc_hexdump("UPnP WLANEVENT Msg", (char *)pUPnPMsg, UPnPMsgLen);
					
			//encode the message use base64
			encodeLen = ILibBase64Encode(pUPnPMsg, UPnPMsgLen, &encodeStr);
			if(encodeLen > 0)
				DBGPRINTF(RT_DBG_INFO, "ConfigReq->Msg=%s!\n", encodeStr);

			//Send event out
			if (encodeLen > 0)
				retVal = WscDevStateVarUpdate(WSC_EVENT_WLANEVENT, (char *)encodeStr, NULL, interfaceIndex);

			if (encodeStr != NULL)
				free(encodeStr);
			free(pUPnPMsg);
		}
	}

	if ((WscUPnPOpMode & WSC_UPNP_OPMODE_CP) && (strlen(triggerMac) > 0))
	{
		
	}

	DBGPRINTF(RT_DBG_INFO, "<-----End(%s)\n", __FUNCTION__);
	return 0;
}


/*
	Format of iwcustom msg WSC ProbeReq message:
	1. SSID which station want to probe(32 bytes):
			<SSID string>
		*If the length if SSID string is small than 32 bytes, fill 0x0 for remaining bytes.
	2. Station MAC address(6 bytes):
	3. element ID(OID)(1 byte):
			val=0xDD
	4. Length of "WscProbeReqData" in the probeReq packet(1 byte):
	5. "WscProbeReqData" info in ProbeReq:
								
			32        6      1          1          variable length
		+----------+--------+---+-----------------+----------------------+
		|SSIDString| SrcMAC |eID|LenOfWscProbeData|    WscProbeReqData   |

*/
static int WscEventMgmt_ProbreReq(
	IN char *pBuf,
	IN int  bufLen,
	IN int interfaceIndex)
{
	unsigned char *encodeStr = NULL, *pWscMsg = NULL, *pUPnPMsg = NULL;
	char srcMacStr[18];
	int encodeLen = 0, UPnPMsgLen = 0;
	int retVal;
	unsigned char wscLen;
		
	if ((WscUPnPOpMode & WSC_UPNP_OPMODE_DEV) == 0)
		return -1;
	
	DBGPRINTF(RT_DBG_INFO, "(%s):Ready to parse the K2U msg(len=%d)!\n", __FUNCTION__, bufLen);
	//wsc_hexdump("WscMgmtEvent_ProbreReq-K2UMsg", pBuf, bufLen);

	pWscMsg = (unsigned char *)(pBuf + sizeof(RTMP_WSC_MSG_HDR));
	//Skip the SSID field
	pWscMsg += 32;
			
	/* Get the SrcMAC and copy to the WLANEVENTMAC, 
		depends on the WFAWLANConfig:1  Service Template Version 1.01, 
		the MAC format is "xx:xx:xx:xx:xx:xx", case-independent, 17 char.
	*/
	memset(srcMacStr, 0 , sizeof(srcMacStr));
	snprintf(srcMacStr, 17, "%02x:%02x:%02x:%02x:%02x:%02x", pWscMsg[0], pWscMsg[1], pWscMsg[2],pWscMsg[3],pWscMsg[4],pWscMsg[5]);
	DBGPRINTF(RT_DBG_INFO, "ProbeReq->Mac=%s!\n", srcMacStr);

	//Skip the SrcMAC field & eID field
	pWscMsg += (6 + 1);

	//Get the WscProbeData Length
	wscLen = *(pWscMsg);
	DBGPRINTF(RT_DBG_INFO, "(%s): pWscHdr Len=%d!\n", __FUNCTION__, wscLen);
		
	UPnPMsgLen = wscLen + 18;
	pUPnPMsg = malloc(UPnPMsgLen);
	
	if(pUPnPMsg)
	{
		memset(pUPnPMsg, 0, UPnPMsgLen);
		pUPnPMsg[0] = WSC_EVENT_WLANEVENTTYPE_PROBE;
		memcpy(&pUPnPMsg[1], srcMacStr, 17);

		//jump to the WscProbeReqData and copy to pUPnPMsg buffer
		pWscMsg++;	
		memcpy(&pUPnPMsg[18], pWscMsg, wscLen);
		//wsc_hexdump("UPnP WLANEVENT Msg", (char *)pUPnPMsg, UPnPMsgLen);
				
		//encode the message use base64
		encodeLen = ILibBase64Encode(pUPnPMsg, UPnPMsgLen, &encodeStr);
			
		//Send event out
		if (encodeLen > 0){
			DBGPRINTF(RT_DBG_INFO, "ProbeReq->Msg=%s!\n", encodeStr);
			retVal = WscDevStateVarUpdate(WSC_EVENT_WLANEVENT, (char *)encodeStr, NULL, interfaceIndex);
		}
		if (encodeStr != NULL)
			free(encodeStr);
		free(pUPnPMsg);
	}
	
	return 0;
	
}


int WscEventMgmtMsgRecv(
	IN char *pBuf,
	IN int  bufLen,
	IN int interfaceIndex)
{
	RTMP_WSC_MSG_HDR *pHdr = NULL;	
	
	pHdr = (RTMP_WSC_MSG_HDR *)pBuf;
	if (!pHdr)
		return -1;

	if (pHdr->msgType != WSC_OPCODE_UPNP_MGMT)
		return -1;

	DBGPRINTF(RT_DBG_INFO, "%s:Receive a MGMT event, msgSubType=%d\n", __FUNCTION__, pHdr->msgSubType);
	switch(pHdr->msgSubType)
	{
		case WSC_UPNP_MGMT_SUB_PROBE_REQ:
			WscEventMgmt_ProbreReq(pBuf, bufLen,interfaceIndex);
			break;
		case WSC_UPNP_MGMT_SUB_CONFIG_REQ:
			WscEventMgmt_ConfigReq(pBuf, bufLen,interfaceIndex);
			break;
		case WSC_UPNP_MGMT_SUB_REG_SELECT:
			WscEventMgmt_RegSelect(pBuf, bufLen,interfaceIndex);
			break;
		default:
			DBGPRINTF(RT_DBG_ERROR, "Un-Supported WSC Mgmt event type(%d)!\n", pHdr->msgSubType);
			break;
	}

	return 0;
}


/******************************************************************************
 * WscDevHandleSubscriptionReq
 *
 * Description: 
 *       Called during a subscription request callback.  If the subscription 
 *       request is for the Wsc device and match the serviceId, then accept it.
 *
 * Parameters:
 *   sr_event -- The "Upnp_Subscription_Request" structure
 *
 * Return:
 *   TRUE 
 *****************************************************************************/
int WscDevHandleSubscriptionReq(IN char *req_SID, IN unsigned long ipaddr)
{
	struct upnpCPNode *CPNode;

	/*  Insert this Control Point into WscCPList*/
	WscDevCPNodeInsert(wscDevHandle, req_SID);
        CPNode = WscCPList;
        while (CPNode)
	{
	        if(strcmp(CPNode->device.SID, req_SID) == 0)
                {       
                        CPNode->device.ipAddr = ipaddr;
                        break;
                }       
                CPNode = CPNode->next;                
        }
        
        if (upnp_debug_level > RT_DBG_OFF)
        {
                dumpDevCPNodeList();
        }

	return TRUE;
}


/******************************************************************************
 * WscLocalDevServTbInit
 *
 * Description: 
 *     Initializes the service table for the Wsc UPnP service.
 *     Note that knowledge of the service description is assumed. 
 *
 * Paramters:
 *     None
 *
 * Return:
 *      always TRUE.
 *****************************************************************************/
static int WscLocalDevServTbInit(void)
{
	unsigned int i = 0;
	
#ifdef SECOND_WIFI

	for (i = 0; i < WSC_STATE_VAR_MAXVARS; i++)
	{
		wscLocalDevice[0].services.StateVarVal[i] = wscStateVarCont[i];
		strncpy(wscLocalDevice[0].services.StateVarVal[i], wscStateVarDefVal[i], WSC_STATE_VAR_MAX_STR_LEN-1);
		//diag_printf("!!WscLocalDevServTbInit:ra0\n");
	}	
	for (i = 0; i < WSC_STATE_VAR_MAXVARS; i++)
	{
		wscLocalDevice[1].services.StateVarVal[i] = wscStateVarCont5G[i];
		strncpy(wscLocalDevice[1].services.StateVarVal[i], wscStateVarDefVal[i], WSC_STATE_VAR_MAX_STR_LEN-1);
		//diag_printf("!!WscLocalDevServTbInit:rai0\n");
	}
#else	
	for (i = 0; i < WSC_STATE_VAR_MAXVARS; i++)
	{
		wscLocalDevice.services.StateVarVal[i] = wscStateVarCont[i];
		strncpy(wscLocalDevice.services.StateVarVal[i], wscStateVarDefVal[i], WSC_STATE_VAR_MAX_STR_LEN-1);
	}

#endif

	return TRUE;
}


/******************************************************************************
 * WscUPnPDevStop
 *
 * Description: 
 *     Stops the device. Uninitializes the sdk. 
 *
 * Parameters:
 *     None
 * Return:
 *     TRUE 
 *****************************************************************************/
int WscUPnPDevStop(void)
{
	WscDevCPListRemoveAll();
	
	return 0;
}


/******************************************************************************
 * WscUPnPDevStart
 *
 * Description: 
 *      Registers the UPnP device, and sends out advertisements.
 *
 * Parameters:
 *
 *   char *ipAddr 		 - ip address to initialize the Device Service.
 *   unsigned short port - port number to initialize the Device Service.
 *   char *descDoc  	 - name of description document.
 *                   		If NULL, use default description file name. 
 *   char *webRootDir 	 - path of web directory.
 *                   		If NULL, use default PATH.
 * Return:
 *    success - WSC_SYS_SUCCESS
 *    failed  - WSC_SYS_ERROR
 *****************************************************************************/
int WscUPnPDevStart(void)
{
	int ret, strLen = 0;
	char udnStr[WSC_UPNP_UUID_STR_LEN]={0};

	memset(CfgMacStr, 0 , sizeof(CfgMacStr));
	snprintf(CfgMacStr[0], 18, "%02x:%02x:%02x:%02x:%02x:%02x", HostMacAddr[0][0], HostMacAddr[0][1], 
				HostMacAddr[0][2],HostMacAddr[0][3],HostMacAddr[0][4],HostMacAddr[0][5]);
#ifdef SECOND_WIFI
	snprintf(CfgMacStr[1], 18, "%02x:%02x:%02x:%02x:%02x:%02x", HostMacAddr[1][0], HostMacAddr[1][1], 
				HostMacAddr[1][2],HostMacAddr[1][3],HostMacAddr[1][4],HostMacAddr[1][5]);

#endif

	memset(udnStr, 0, WSC_UPNP_UUID_STR_LEN);
	ret = wsc_get_oid(RT_OID_WSC_UUID, &udnStr[5], &strLen);//sht:??? add parameter:interface.
	if (ret == 0)
	{
		memcpy(&udnStr[0], "uuid:", 5);
		DBGPRINTF(RT_DBG_PKT, "UUID Str=%s!\n", udnStr);
		if(strLen <= strlen(wps_uuidvalue)-5)
		{
			/* We replace the uuid string generated by init() of miniupnpd. */
			strncpy(wps_uuidvalue+5, &udnStr[5], strLen);
			
			ASSERT((*(wps_uuidvalue+5+strLen-1)) == '\0');
		}	
		else
		{
			/* We replace the uuid string generated by init() of miniupnpd. */
			strncpy(wps_uuidvalue+5, &udnStr[5], strlen(wps_uuidvalue)-5);
			
			ASSERT((*(wps_uuidvalue+strlen(wps_uuidvalue)-1)) == '\0');

		}
	}
	else
	{
		//diag_printf("Get 2.4G UUID string failed -- ret=%d\n", ret);
		goto failed;
	}

#ifdef SECOND_WIFI
	//mytest
	
	memset(&WSC_IOCTL_IF[0], 0, IFNAMSIZ);
	strcpy(&WSC_IOCTL_IF[0], "rai0");//adjust interface rai0 for wsc_get_oid()

	memset(udnStr, 0, WSC_UPNP_UUID_STR_LEN);
	ret = wsc_get_oid(RT_OID_WSC_UUID, &udnStr[5], &strLen);//sht:??? add parameter:interface.
	//rollback WSC_IOCTL_IF to ra0.
	memset(&WSC_IOCTL_IF[0], 0, IFNAMSIZ);
	strcpy(&WSC_IOCTL_IF[0], "ra0");//adjust interface rai0 for wsc_get_oid()
	
	if (ret == 0)
	{
		memcpy(&udnStr[0], "uuid:", 5);
		DBGPRINTF(RT_DBG_PKT, "UUID Str=%s!\n", udnStr);

		// We replace the uuid string generated by init() of miniupnpd. 
		strncpy(wps_5G_uuidvalue+5, &udnStr[5], strLen);
		ASSERT((*(wps_5G_uuidvalue+5+strLen-1)) == '\0');
		//diag_printf("5G WPS 5G UUID=%s!\n", wps_5G_uuidvalue);
	}
	else
	{
		//diag_printf("Get 5G UUID string failed -- ret=%d\n", ret);
		goto failed;
	}
	
	memset(wscLocalDevice, 0, sizeof(wscLocalDevice));
	
#else

	memset(&wscLocalDevice, 0, sizeof(wscLocalDevice));

#endif

	//memset(&wscLocalDevice, 0, sizeof(wscLocalDevice));

	WscLocalDevServTbInit();	
	DBGPRINTF(RT_DBG_PKT, "WscLocalDevServTbInit Initialized\n");

	// Init the ControlPoint List.
	WscCPList = NULL;

	return 0;

failed:
	return WSC_SYS_ERROR;
}

