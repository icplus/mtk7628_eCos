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
    http_conf.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#ifndef HTTP_CONF_H
#define HTTP_CONF_H

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <cyg/kernel/kapi.h>

//==============================================================================
//                                    MACROS
//==============================================================================
#define DEFAULT_HTTP_PORT 80
#define DEFAULT_HTTPS_PORT 443

#define HTTPD_DAEMON_PRIORITY	8
#define HTTPD_PROCESS_PRIORITY	8
#define HTTPD_DAEMON_STACKSIZE		4096//8192
#define HTTPD_PROCESS_STACKSIZE		(1024*16)



#define SLEEP(t)

//#define CHARSET		"iso-8859-1" /* default */
#define CHARSET			""


/* specify name of http server */
#define SERVER_NAME		"HTTPD"

extern char cmd_page[];
extern char restart_msg[];
#define CMD_PAGE			cmd_page
#define RESTART_MSG_PAGE	restart_msg


extern int http_user_num_max;

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
extern struct err_page error_files[];

#endif /* HTTP_CONF_H */


