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
    pirq.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/
#ifndef PRIQ_H
#define PRIQ_H

//==============================================================================
//                                INCLUDE FILES
//==============================================================================

//==============================================================================
//                                    MACROS
//==============================================================================
#define PRIQ_MAX	5

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
struct priq_attr_t{
	struct class_queue *priq[PRIQ_MAX];
	int priq_max;
	int def;
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
extern int priQ_class_create(struct schedq *sq, int type, unsigned char tos);
extern int priQ_class_destroy_all(struct schedq *sq);
extern int priq_enqueue(struct schedq *sq, struct mbuf *m);
extern struct	mbuf * priq_dequeue(struct schedq *sq);
extern void priq_requst(struct schedq *sq, int flag);

#endif // PRIQ_H


