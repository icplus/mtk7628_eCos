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
#ifndef __UPNP_H__
#define __UPNP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <upnp_type.h>
#include <upnp_service.h>
#include <upnp_description.h>
#include <upnp_http.h>
#include <upnp_util.h>
#include <ssdp.h>
#include <soap.h>
#include <gena.h>

// Debug
typedef enum{
	RT_DBG_OFF	= 0,
	RT_DBG_ERROR	= 1,
	RT_DBG_PKT	= 2,
	RT_DBG_INFO	= 3,
	RT_DBG_LOUD	= 4,
	RT_DBG_ALL
}WSC_DEBUG_LEVEL;

extern int upnp_debug_level;	
#define DBGPRINTF(Level, fmt, args...)   \
{                                       \
        if (Level <= upnp_debug_level)    \
        {                                \
                diag_printf("[UPNPD]");  \
		diag_printf( fmt, ## args); \
        }                                   \
}

// For message passing between two thread
#define UPNP_HTTP_NEWFD		1
#define UPNP_SSDP_NEWFD		2
#define UPNP_TIMER_ALARM	3
#define	UPNP_EVENT_ALARM	4
#define UPNP_DAEMON_STOP	5

/*
 * Declaration
 */
extern	UPNP_ADVERTISE upnp_advertise_table[];
extern	UPNP_SERVICE upnp_service_table[];
extern	UPNP_DESCRIPTION upnp_description_table[];
extern	UPNP_CONTEXT upnp_context;

#ifdef CONFIG_WPS
extern	UPNP_SERVICE wps_upnp_service_table[];

#define SERIALNUMBER_MAX_LEN (10)
extern char wps_uuidvalue[];
extern char wps_serialnumber[SERIALNUMBER_MAX_LEN];
extern char *netlink_buf;
extern unsigned int netlink_buf_len;

#ifdef SECOND_WIFI
extern char wps_5G_uuidvalue[];
#endif

#endif /* CONFIG_WPS */



extern	char upnp_os_name[];
extern	char upnp_os_ver[];
extern	char upnp_model_name[];
extern	char upnp_model_ver[];

#define	UPNP_FLAG_SHUTDOWN	1
#define	UPNP_FLAG_RESTART	2
extern	int	upnp_flag;

// Functions
void upnp_main(void);
void upnp_daemon(void);
void upnp_timer_alarm(void);
void upnp_event_alarm(char *);
int context_init(UPNP_CONTEXT *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UPNP_H__ */

