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
#include <os_info.h>
#include <model_info.h>
#include <upnp_portmap.h>


/*==============================================================================
 * Function:    upnp_device_init()
 *------------------------------------------------------------------------------
 * Description: UPnP Device data hooking
 *
 *============================================================================*/
int upnp_device_init(void)
{
    upnp_get_os_name();
    upnp_get_os_ver();
    upnp_get_model_name();
    upnp_get_model_ver();
	
    /* NAT traversal initialization */
    if (upnp_portmap_init() != 0)
        return -1;
	
    return 0;
}

void upnp_device_shutdown(void)
{
    /* cleanup NAT traversal structures */
    upnp_portmap_free();
}

void upnp_device_timeout(time_t now)
{
	upnp_portmap_timeout(now);
	return;
}

