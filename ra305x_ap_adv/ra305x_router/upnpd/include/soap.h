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


#ifndef __SOAP_H__
#define __SOAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <upnp_type.h>
#include <upnp_service.h>
#include <upnp_http.h>

#define SOAP_MAX_ERRMSG     256
#define SOAP_MAX_BUF        2048

enum SOAP_ERROR_E
{ 
    SOAP_INVALID_ACTION = 401,
    SOAP_INVALID_ARGS,
    SOAP_OUT_OF_SYNC,
    SOAP_INVALID_VARIABLE,
    SOAP_DEVICE_INTERNAL_ERROR = 501
};

/*
 * Functions
 */
UPNP_SERVICE    *get_service    (char *purl);
int             soap_process    (UPNP_CONTEXT *context);
#ifdef CONFIG_WPS
int             wps_soap_process    (UPNP_CONTEXT *context);
#endif /* CONFIG_WPS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SOAP_H__ */


