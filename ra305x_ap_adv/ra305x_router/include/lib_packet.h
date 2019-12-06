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
    lib_packet.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#ifndef __LIB_PACKET_H__
#define __LIB_PACKET_H__

//==============================================================================
//                                INCLUDE FILES
//==============================================================================

//==============================================================================
//                                    MACROS
//==============================================================================
enum{
	FORWARDQ = 1,
	HOSTQ
};

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
typedef int (*pfun)();

struct eth_input_func{
	pfun func;
	struct ifnet	*ifp;
	struct eth_input_hook *next; 
};

struct pktsocket{
	struct eth_input_func *handler;
	cyg_handle_t Qhandler;
	cyg_mbox Qobject;
	struct pktsocket *next;
};

struct rcv_info{
	struct mbuf *m;
	char *head;
	struct ifnet *ifp;
	unsigned char *buf;
	int len;
};

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
struct ether_addr;
struct ether_addr *ether_aton(char *s);
void ether_aton_r(char *s, char *mac);
char *ether_ntoa(struct ether_addr *e);

int LIB_ether_input_register(struct ifnet *ifp, pfun func, int flag);
int LIB_ether_input_register_tail(struct ifnet *ifp, pfun func, int flag);
int LIB_ether_input_unregister(struct ifnet *ifp, pfun func, int flag);
int LIB_ether_input_func(struct ifnet *ifp, char *head, struct mbuf *m, int flag);
void link_addr(const char *addr,struct sockaddr_dl *sdl);
int LIB_pktsocket_create(char *inf, pfun func);
void LIB_pktsocket_close(int s);
int LIB_pktsocket_enQ(struct ifnet *ifp, char *head, struct mbuf *m, struct eth_input_func *hn);
struct rcv_info * LIB_pktsocket_deQ(struct pktsocket *pkts, struct timeval *tv);
int LIB_rcvinfo_read(int fd, char *buf, int len);
void LIB_rcvinfo_close(int fd);
void LIB_rcvinfo_getifp(int fd, int *ifp);
int LIB_packet_send(struct ifnet *ifp, struct sockaddr *dst, char *data, int len);

#endif	/*  __LIB_PACKET_H__  */



