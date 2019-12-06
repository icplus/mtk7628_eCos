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
    tfq.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/
#ifndef TFQ_H
#define TFQ_H

//==============================================================================
//                                INCLUDE FILES
//==============================================================================

//==============================================================================
//                                    MACROS
//==============================================================================
#define TFQ_MAX	256

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
struct tfq_entry_t{
	u_int lanip_;
	u_int upbytes;		/* Uplink bytes */
	u_int downbytes;		/* Downlink bytes */
	u_int upmaxrate_;		/* Uplink bytes per second */
	u_int downmaxrate_;	/* Downlink bytes per second */
	struct timeval	uplast_;
	struct timeval	downlast_;
};


struct tfq_attr_t{
	struct tfq_entry_t list[TFQ_MAX];
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
extern void tfq_load_config();
extern int tfq_uplink(struct mbuf *m, struct ether_header *eh);
extern int tfq_downlink(struct mbuf *m);
extern int tfq_enqueue(struct schedq *sq, struct mbuf *m);
extern struct mbuf * tfq_dequeue(struct schedq *sq);
extern void tfq_requst(struct schedq *sq, int flag);
extern void tfq_schedQ_register(struct ifnet *ifp, struct schedq *sq);
extern void tfq_schedQ_unregister(struct ifnet *ifp);

#endif // TFQ_H


