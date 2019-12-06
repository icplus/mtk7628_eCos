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
/*
 * File:		
 * Create Date: 2003.08.13
 * Description:
 */
 
#ifndef  __GENA_H__
#define  __GENA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <upnp_type.h>
#include <upnp_service.h>
#include <upnp_http.h>


/* Constants */
#define GENA_TIMEOUT            30      /* GENA check interval */
#define GENA_SUBTIME            1800    /* default subscription time */
#define GENA_MAX_HEADER         512
#define GENA_MAX_BODY           4096
#define GENA_MAX_URL            256
//#define GENA_MAX_VALUE_SIZE     128


/*
 * Functions
 */
int gena_update_event_var(UPNP_SERVICE *, char *, char *);
void gena_notify_complete(UPNP_SERVICE *);
void gena_notify(UPNP_CONTEXT *, UPNP_SERVICE *, char *);
int gena_process(UPNP_CONTEXT *);
void gena_timeout(UPNP_CONTEXT *, unsigned int);
void gena_event_alarm(UPNP_CONTEXT *, char *);
void gena_shutdown(UPNP_CONTEXT *);
int gena_init(UPNP_CONTEXT *);


#ifdef __cplusplus
}
#endif

#endif /* __GENA_H__ */

