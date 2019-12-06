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


#ifndef __SSDP_H__
#define __SSDP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <upnp_type.h>

/*
 * Constants & Definitions
 */
#define SSDP_ADDR       "239.255.255.250"
#define SSDP_PORT       1900
#define SSDP_MAXLEN     2048    /* SSDP response and advertise buffer size */
 
enum SSDP_TYPE_E
{ 
    SSDP_BYEBYE,
    SSDP_ALIVE
};

enum MSEARCH_TYPE_E 
{ 
    MSEARCH_NONE,
    MSEARCH_ALL,
    MSEARCH_ROOTDEVICE,
    MSEARCH_UUID,
    MSEARCH_SERVICE,
    MSEARCH_DEVICE,
    MSEARCH_OTHER
};

enum ADVERTISE_TYPE_E
{ 
    ADVERTISE_DEVICE,
    ADVERTISE_SERVICE,
    ADVERTISE_UUID,
    ADVERTISE_ROOTDEVICE
};

/*
 * Structures
 */
typedef struct _UPNP_ADVERTISE
{
    char    *name;
    char    *contrlURL;
    char    *uuid; 
    char    *root_device_xml; 
    char    *domain; 
    int     type; 
}
UPNP_ADVERTISE;

/*
 * Functions
 */
void	ssdp_process    (UPNP_CONTEXT *);
void	ssdp_timeout	(UPNP_CONTEXT *, unsigned int);
void	ssdp_shutdown   (UPNP_CONTEXT *);
int	ssdp_init       (UPNP_CONTEXT *);
int ssdp_udp_init (UPNP_CONTEXT *);

#ifdef __cplusplus
}
#endif

#endif /* __SSDP_H__ */


