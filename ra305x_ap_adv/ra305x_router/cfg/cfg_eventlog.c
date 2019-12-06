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
    cfg_eventlog.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <config.h>
#include <cyg/kernel/kapi.h>

#include <pkgconf/system.h>
#ifdef CYGBLD_DEVS_ETH_DEVICE_H    // Get the device config if it exists
#include CYGBLD_DEVS_ETH_DEVICE_H
#endif

#include <cfg_id.h>
#include <cfg_def.h>
#include <network.h>
#include <cfg_net.h>
#include <sys_status.h>
#include <eventlog.h>

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================

//********************
// eventlog options
//********************

void EventLog_CfgRead(struct eventlog_cfg *cfg)
{
	int i;
	extern int log_mail_addr_num;
	unsigned char mac[6];
	
	CFG_get_str(CFG_SYS_NAME, cfg->sysName) ; 
	
	if (SYS_wan_mode == DHCPMODE)
		strcpy(cfg->sysDomain, SYS_wan_domain);
	else
		CFG_get_str(CFG_SYS_DOMAIN, cfg->sysDomain);
	CFG_get(CFG_HW_MAC+2, mac);
	sprintf(cfg->sysID, "RID-%02x%02x%02x", mac[3], mac[4], mac[5]);
	
	CFG_get(CFG_LOG_MODE, &cfg->logmode);
	CFG_get(CFG_LOG_RM_EN, &cfg->doRemoteLog);
	CFG_get(CFG_LOG_RM_TYPE, &cfg->doRemoteMail);
	CFG_get_str( CFG_LOG_SYS_IP, cfg->remote_ip);
	
	CFG_get_str(CFG_LOG_MAIL_IP, cfg->Local_Server_IP);
	
	// Read email address to send
	CFG_get_str(CFG_LOG_E_MAIL, cfg->MailTo[0]);
	
	for (i=1; i<log_mail_addr_num; i++)
		CFG_get_str(CFG_LOG_E_MAIL+i, cfg->MailTo[i]);
	
	return;
}

