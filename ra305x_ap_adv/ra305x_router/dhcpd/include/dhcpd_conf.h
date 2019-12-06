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

#ifndef _DHCPD_CONF_H
#define _DHCPD_CONF_H

#include <eventlog.h>

#define DHCPD_MAX_LEASES    254

#define DEFAULT_START           "192.168.0.2"
#define DEFAULT_END             "192.168.0.253"
#define DEFAULT_LEASE_TIME      3600
#define DEFAULT_AUTO_TIME       3
#define DEFAULT_CONFLICT_TIME   3600
#define DEFAULT_DECLINE_TIME    3600
#define DEFAULT_MIN_LEASE_TIME  60
#define DEFAULT_SNAME           ""
#define DEFAULT_BOOT_FILE       ""
#define DEFAULT_BOOT_IP         "0.0.0.0"
#define DEFAULT_MASK            "255.255.255.0"
#define DEFAULT_GATEWAY          "192.168.0.1"
#define DEFAULT_DNS             "168.95.1.1"
#define DEFAULT_DOMAIN          "HomeGateway"

#define DHCPD_DEBUG
//#undef    DHCPD_DEBUG

#define DHCPD_DIAG(c...)    SysLog(SYS_LOG_INFO|LOGM_DHCPD, ##c)
#define DHCPD_LOG(c...)     SysLog(LOG_USER|SYS_LOG_INFO|LOGM_DHCPD, ##c)

#define IGNORE_BROADCASTFLAG

#define _string(s) #s
#define STR(s)  _string(s)

#endif /* _DHCPD_CONF_H */
