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

#include <porting.h>
#include <string.h>
#include <version.h>


char upnp_os_name[32];
char upnp_os_ver[32];


/*==============================================================================
 * Function:    upnp_get_os_name()
 *------------------------------------------------------------------------------
 * Description: Get kernel name
 *
 *
 *============================================================================*/
int upnp_get_os_name
(
	void
)
{
	strcpy(upnp_os_name, sys_os_name);
	return 0;
}

/*==============================================================================
 * Function:    upnp_get_os_ver()
 *------------------------------------------------------------------------------
 * Description: Get kernel version
 *
 *
 *============================================================================*/
int upnp_get_os_ver
(
	void
)
{
	strcpy(upnp_os_ver, sys_os_ver );
	return 0;
}


