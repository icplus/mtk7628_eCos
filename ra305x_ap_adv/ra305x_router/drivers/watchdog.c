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
    watchdog.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <stdio.h>
#include <cyg/kernel/kapi.h>
#include <cfg_id.h>
#include <cfg_def.h>
#include <network.h>


//==============================================================================
//                                    MACROS
//==============================================================================
#include <cyg/hal/hal_io.h>

static	unsigned long watchdog_enabled = 0;
static	unsigned long watchdog_time = 15 * 100;
extern	int config_watchdog_interval;

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================
void watchdog_tmr_func(void)
{
	/*  Add watchdog reset code here */

	
	// Setup watchdog periodical timer
	timeout((timeout_fun *)&watchdog_tmr_func, 0, watchdog_time/2);
	
	return;
}


void watchdog_init(void)
{
	int enabled = 0;

	if (CFG_get(CFG_SOC_WDG, &enabled) < 0)
		return;

	if ((enabled > 0) && (watchdog_enabled == 0)) {
		/*  Add watchdog init code here */

		watchdog_time = config_watchdog_interval * 100;
		if (watchdog_time == 0)
			watchdog_time = 400;		// Default set to 4 seconds
	
		watchdog_tmr_func();
	} else if ((enabled == 0) && (watchdog_enabled > 0)) {
		/*  Add watchdog stop code here  */

		untimeout((timeout_fun *)&watchdog_tmr_func, 0);
	}
	watchdog_enabled = enabled;
	return;
}


static void usage(void)
{
	printf("Usage:   wdog <on/off>\n");
}

int watchdog_cmd(int argc, char* argv[])
{
	int enable;
	
	if (argc == 1)
	{
		if (strcmp(argv[0], "on") == 0)
		{
			enable = 1;
		}
		else if (strcmp(argv[0], "off") == 0)
		{
			enable = 0;
		}
		else
		{
			goto help;
		}
		
		CFG_set(CFG_SOC_WDG, &enable);
		
		// Reinit watchdog;
		watchdog_init();
	}
	else if (argc > 1)
	{
		goto help;
	}
	
	printf("Watchdog timer is %s", watchdog_enabled ? "on" : "off");
	return 0;

help:
	usage();
	return 0;
}

