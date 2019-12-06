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
 ***************************************************************************

    Module Name:
    mod_init.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/


//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <stdio.h>
#include <config.h>
#include <network.h>
#include <sys/mbuf.h>
#include <net/if.h>
//Eddy6 #include <net/if_qos.h>
#include <priq.h>
#include <cfg_net.h>
#include <sys_status.h>
#include <cfg_id.h>

#include <cyg/hal/hal_if.h>     // For CYGACC_CALL_IF_DELAY_US
#include <cyg/hal/hal_cache.h>  // For HAL_DCACHE_INVALIDATE
#include <cyg/kernel/kapi.h>
#include <cyg/io/eth/netdev.h>  // For struct eth_drv_sc
#include <cyg/io/eth/eth_drv.h> // For eth_drv_netde

//==============================================================================
//                                    MACROS
//==============================================================================

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
extern int opmode;
void interface_config(void)
{
	char line[100], *p;
	int i;
#ifdef CONFIG_BRIDGE
	extern char bridge_cfg[128];
#endif
	
	/* config WAN interface name */
	CFG_get_str(CFG_SYS_OPMODE, line);
	opmode = strtol(line, &p, 10);
	if (opmode != 3) {
		if (opmode == 2) {
			strcpy(line, "apcli0");
			diag_printf("Operation Mode: AP Client\n");
		} else {
			opmode = 1;
			strcpy(line, CONFIG_DEFAULT_WAN_NAME);
			diag_printf("Operation Mode: Gateway\n");
		}
		
		strcpy(WAN_NAME, line);
		strcpy(SYS_wan_if, WAN_NAME);
	}	
	
	/* config LAN interface name */
	strcpy(line, CONFIG_DEFAULT_LAN_NAME);	
	strcpy(LAN_NAME, line);
	strcpy(SYS_lan_if, LAN_NAME);
	
#ifdef CONFIG_BRIDGE
	sprintf(bridge_cfg, "%s ", line);
	if ((opmode == 2) || (opmode == 3))
	{
		i = strlen(bridge_cfg);
		if(i > 100)
			i= 100;
		strcpy(bridge_cfg+i,CONFIG_DEFAULT_WAN_NAME);
		//sprintf(bridge_cfg, "%s %s", bridge_cfg, CONFIG_DEFAULT_WAN_NAME);
	}	
#endif // CONFIG_BRIDGE //
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION:	This function override the ip_init function in 
// 				IP protocol switch to initialize the ipfilter module firstly.
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void iplinit(void)
{
	extern void ip_init(void);
	
#if (MODULE_NAT == 1)
	extern int ipl_enable(void);
	
	if (ipl_enable() != 0)
	{
		printf("%s: IP Filter failed to attach\n", __FUNCTION__);
	}
#endif

	ip_init();
}

