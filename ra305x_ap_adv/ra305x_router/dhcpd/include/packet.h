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

#ifndef __DHCPD_PACKET_H__
#define __DHCPD_PACKET_H__

struct dhcpInfo
{
    int rcv_ifp;
    struct dhcpd payload;
};

int DHCPD_SendPacket(struct dhcpd *payload, int force_broadcast, int txifp);
int DHCPD_ReadPacket(struct dhcpInfo *packetinfo, int fd);
int DHCPD_rcv(int fd, struct timeval *tv, int flag);
int DHCPD_PktCheck(struct ifnet *ifp, char *head, struct mbuf *m, struct eth_input_func *hn);

#endif
