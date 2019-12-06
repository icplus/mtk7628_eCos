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


#ifndef __UPNP_SERVICE_H__
#define __UPNP_SERVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <upnp_type.h>

/* Constants */
#define UPNP_MAX_SID				32
#define	UPNP_EVENTED_VALUE_SIZE		2048/*128*/ 

typedef struct _UPNP_SERVICE
{
	char					*control_url;
	char					*event_url;
	char					*uuid;
	char					*name;
	char					*service_id;
	struct upnp_action		*action_table;
	struct upnp_state_var	*statevar_table;
	
	int						evented;
	struct upnp_state_var	*event_var_list;
	struct upnp_subscriber	*subscribers;
}
UPNP_SERVICE;

typedef int  (*ACTION_FUNC)(UPNP_SERVICE *, IN_ARGUMENT *, OUT_ARGUMENT *, char *);

typedef int  (*QUERY_FUNC)(UPNP_SERVICE *, UPNP_VALUE *, char *);

typedef struct
{
	char    *name;
	int     type;
}
UPNP_ARGUMENT;

typedef struct upnp_action
{
	char                *name;
	int                 in_num;
	UPNP_ARGUMENT       *in_argument;
	int                 out_num; 
	UPNP_ARGUMENT       *out_argument; 
	ACTION_FUNC         action;
}
UPNP_ACTION;

typedef	struct
{
	int		init;
	int		changed;
	char	value[UPNP_EVENTED_VALUE_SIZE];
}
UPNP_EVENTED;

typedef struct upnp_state_var 
{
	char                    *name;
	int                     type;
	QUERY_FUNC              func;
	UPNP_EVENTED			*evented;
	
	struct upnp_state_var   *next;
}
UPNP_STATE_VAR;

/* List of subscribers for each event source. */
typedef struct upnp_subscriber
{
	struct upnp_subscriber  *next;
	struct upnp_subscriber  *prev;
	unsigned long			ipaddr;
	unsigned short			port;
	char					*uri;
	char                    sid[UPNP_MAX_SID];
	unsigned int            expire_time;
	unsigned int            seq;
	unsigned int            retry_count;
} 
UPNP_SUBSCRIBER;


#ifdef __cplusplus
}
#endif

#endif /* __UPNP_SERVICE_H__ */


