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


#ifndef __UPNP_PORTMAP_H__
#define __UPNP_PORTMAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <upnp_type.h>

#define UPNP_PM_SIZE        32
#define UPNP_PM_DESC_SIZE   256

typedef struct upnp_portmap
{
    char            remote_host[sizeof("255.255.255.255")];
    unsigned short  external_port;
    char            protocol[8];
    unsigned short  internal_port;
    char            internal_client[sizeof("255.255.255.255")];
    unsigned int    enable;
    char            description[UPNP_PM_DESC_SIZE];
    unsigned long   duration;
    char            wan_if[IFNAMSIZ];
}
UPNP_PORTMAP;

int     upnp_portmap_init(void);
void    upnp_portmap_free(void);
void	upnp_portmap_timeout(time_t now);

int add_portmap
(
	char            *remote_host,
	unsigned short  external_port,
	char            *protocol,
	unsigned short  internal_port,
	char            *internal_client,
	unsigned int    enable,
	char            *description,
	unsigned long   duration,
	char            *wan_if
);

int del_portmap
(
	char            *remote_host,
	unsigned short  external_port,
	char            *protocol,
	char            *wan_if
);

UPNP_PORTMAP *find_portmap
(
	char            *remote_host,
	unsigned short  external_port,
	char            *protocol,
	char            *wan_if
);

UPNP_PORTMAP    *portmap_with_index(int index);
unsigned short  portmap_num(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UPNP_TYPE_H__ */


