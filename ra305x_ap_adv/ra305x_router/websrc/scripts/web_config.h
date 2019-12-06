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
    web_config.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#ifndef	WEB_CONFIG_H
#define WEB_CONFIG_H
//==============================================================================
//                                INCLUDE FILES
//==============================================================================

//==============================================================================
//                                    MACROS
//==============================================================================

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
/* specify default page */
//char default_page[]="index.htm";

/* prcoess command page */
char cmd_page[]="do_cmd.htm";

char restart_msg[]=	"<html><script>\nfunction to(){\n"\
					"window.location='/'}\n"\
					"</script>\n<body OnLoad=\"setTimeout('to()',9000)\">System Restart .."\
					"</body></html>\n";
					
/* customize desired error pages */
struct err_page error_files[]={
	{403,"duplicate_login.htm"},
	{0, NULL},
};

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================

#endif //WEB_CONFIG_H


