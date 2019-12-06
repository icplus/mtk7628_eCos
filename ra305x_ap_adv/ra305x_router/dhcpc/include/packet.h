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

#ifndef __DHCPC_PACKET_H__
#define __DHCPC_PACKET_H__

#include <lib_packet.h>

u_int16_t checksum(void *addr, int count);

int raw_packet(struct dhcpc *payload, u_int32_t source_ip, int source_port,
               u_int32_t dest_ip, int dest_port, unsigned char *src_arp, unsigned char *dest_arp, int txifp);

int kernel_packet(struct dhcpc *payload, u_int32_t source_ip, int source_port,
                  u_int32_t dest_ip, int dest_port);

int dhcpc_check(struct ifnet *ifp, char *head, struct mbuf *m, struct eth_input_func *hn);
int dhcpc_rcv(int fd, struct timeval *tv);
#endif
