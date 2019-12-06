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
    dhcpc.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#ifndef __DHCPC_H__
#define __DHCPC_H__

#include <pkgconf/system.h>
#include <pkgconf/net.h>
#include <machine/types.h>
#include <cyg/kernel/kapi.h>
#include <netinet/udp.h>
#include <netinet/ip.h>

struct dhcpc
{
    unsigned char op;
    unsigned char htype;
    unsigned char hlen;
    unsigned char hops;
    unsigned long xid;
    unsigned short secs;
    unsigned short flags;
    unsigned long ciaddr;
    unsigned long yiaddr;
    unsigned long siaddr;
    unsigned long giaddr;
    unsigned char chaddr[16];
    unsigned char sname[64];
    unsigned char file[128];
    unsigned long cookie;
    unsigned char options[308]; /* 312 - cookie */
};

struct udp_dhcpc_packet
{
    struct ip ip;
    struct udphdr udp;
    struct dhcpc data;
};

// DHCPC interface state machine states; (ref: page 35 of RFC2131)
#define DHCPSTATE_STOPPED       0
#define DHCPSTATE_INIT          1
#define DHCPSTATE_SELECTING     2  // wait for replies to b/c DISCOVER
#define DHCPSTATE_REQUESTING        3
#define DHCPSTATE_REQUEST_RECV      4  // wait for replies to b/c REQUEST
#define DHCPSTATE_BOUND         5
#define DHCPSTATE_RENEWING      6  // wait for replies to u/c REQUEST
#define DHCPSTATE_RENEW_RECV        7
#define DHCPSTATE_REBINDING     8  // wait for replies to b/c REQUEST
#define DHCPSTATE_REBIND_RECV       9
#define DHCPSTATE_NOTBOUND          10 // To let app tidy up
#define DHCPSTATE_DO_RELEASE        11 // Force release of the current lease
#define DHCPSTATE_FREE              12 // Force free dhcpc related data

#define DHCP_LEASE_T1               1
#define DHCP_LEASE_T2               2
#define DHCP_LEASE_EX               4

#define CMD_DO_RELEASE              1
#define CMD_FREE                2
#define CMD_DO_RENEW                3
#define CMD_TIMEOUT             4
#define CMD_CHANGE_STATE            5

#define STACK_SIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL)*2

struct dhcp_lease
{
    cyg_tick_count_t time_stamp;
    unsigned int leased_time;
    unsigned int expiry;
    unsigned int t1;
    unsigned int t2;
    volatile int curr;
    volatile int next;
};

typedef struct t_dhcpc_if
{
    struct t_dhcpc_if *next;

    cyg_handle_t dhcpc_thread_h;
    cyg_thread dhcpc_thread;
	cyg_uint8 dhcpc_stack[4096];//[ STACK_SIZE];//change dhcpc stack size:8192->4096
    cyg_handle_t mbox_id;
    cyg_mbox mbox_obj;

    int user_flag;
    char  *ifname;
    struct ip_set *setp;
    cyg_uint8 state;
    cyg_uint8 *pstate_out;
    cyg_uint16 flags;
    unsigned long server;
    unsigned long req_ip;

    char macaddr[6];
    unsigned int xid;
    struct dhcpc txpkt;
    struct dhcpc rxpkt;

    struct ifnet *ifnetp;
    int listen_socket;
    int countdown;
    int secs;
    struct dhcp_lease lease;
} dhcpc_if;

#define DHCPC_IFFG_NODNS_UPDATE         0x0001

#endif

