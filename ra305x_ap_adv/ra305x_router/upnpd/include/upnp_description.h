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


#ifndef __UPNP_DESCRIPTION_H__
#define __UPNP_DESCRIPTION_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <upnp_type.h>
#include <upnp_http.h>

/* Constants */
#define UPNP_DESC_MAXLEN 2048    /* maximum length per send */

/* Macro */
#define CONTEXT_SEND(str, len)	description_send(context, str, len)


/*
 * Structures
 */
typedef struct _UPNP_DESCRIPTION
{
    char            *name;
    int             (*xml)(UPNP_CONTEXT *context);
}
UPNP_DESCRIPTION;
/*
* XML template index in upnpxmldata.bin
* Note:please make sure the right sequence when use XML template.
*/
enum
{
	XML_INDEX_InternetGatewayDevice = 0,
	XML_INDEX_x_lanhostconfigmanagement,
	XML_INDEX_x_layer3forwarding,
	XML_INDEX_x_wancommoninterfaceconfig,
	XML_INDEX_x_wanipconnection,
	XML_INDEX_x_wfadevicedesc,
	XML_INDEX_x_wfawlanconfigscpd,
	XML_INDEX_x_wfa5Gdevicedesc,
	XML_INDEX_x_wfa5Gwlanconfigscpd,
	XML_INDEX_InternetGatewayDevice2WLAN,
	//add the new XML here.
};


/*
 * Functions
 */
#ifdef CONFIG_WPS
int wps_description_process (UPNP_CONTEXT *context);
#endif /* CONFIG_WPS */
int description_process (UPNP_CONTEXT *context);
int description_send    (UPNP_CONTEXT *context, char *data_buf, int data_len);
int description_readXML(int XMLIndex, char** dst, int *dlen);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UPNP_DESCRIPTION_H__ */


