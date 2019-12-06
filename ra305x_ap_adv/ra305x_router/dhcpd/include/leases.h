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

#ifndef _LEASES_H
#define _LEASES_H


struct dhcpOfferedAddr
{
    u_int8_t chaddr[16];
    struct in_addr yiaddr;
    u_int32_t expires;  /* all variable is host oder except for address */
    unsigned char *_hostname;
    u_int8_t flag;  /* static lease?*/
};

void DHCPD_ClearLease(unsigned char *chaddr, struct in_addr yiaddr);
void DHCPD_FlushLeaseEntry(struct dhcpOfferedAddr *lease);
void DHCPD_FlushLeases(void);
void DHCPD_FlushStaticLeases(void);

int DHCPD_LeaseExpired(struct dhcpOfferedAddr *lease);
struct dhcpOfferedAddr *DHCPD_OldestExpiredLease(void);

struct dhcpOfferedAddr *DHCPD_FindLeaseByChaddr(unsigned char *chaddr);
struct dhcpOfferedAddr *DHCPD_FindLeaseByYiaddr(struct in_addr yiaddr);
struct dhcpOfferedAddr *DHCPD_AddLease(unsigned char *chaddr, struct in_addr yiaddr, unsigned long lease, unsigned char *_hostname, int restore);
struct dhcpOfferedAddr *DHCPD_FindAndSetLease(unsigned char *chaddr, struct in_addr yiaddr, int flag);

struct in_addr DHCPD_FindAddress(int check_expired);
int DHCPD_CheckIp(struct in_addr addr, unsigned char *chaddr, u_int32_t reserved);

int fix_lease_error(void);


/* flag */
#define DYNAMICL                0x0
#define STATICL                 0x1
#define DISABLED                0x2
#define RESERVED                0x4
#define DELETED                 0x8

#define SHOW_DYNAMIC            0x1
#define SHOW_STATIC             0x2
#define SHOW_DYNAMIC_STATIC     0x3
#define SHOW_ALL                0x8

void DHCPD_WriteDynLeases(void);
void DHCPD_LoadDynLeases(void);

#endif
