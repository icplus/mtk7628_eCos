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
    if_qos.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/
#ifndef LIB_QOS_H
#define LIB_QOS_H

//==============================================================================
//                                INCLUDE FILES
//==============================================================================

//==============================================================================
//                                    MACROS
//==============================================================================
#define CLASS_PROTO		0x00000001
#define CLASS_SPORT		0x00000002
#define CLASS_DPOSR		0x00000004
#define CLASS_SADDR		0x00000008
#define CLASS_DADDR		0x00000010
#define CLASS_TOS		0x00000020
#define CLASS_DEFAULT	0x00100000

#define CLASS_ALL		(CLASS_PROTO|CLASS_SPORT|CLASS_DPOSR|CLASS_SADDR|CLASS_DADDR|CLASS_TOS)

#define SCHED_FLUSHQ	1

#define	IFQ_INC_LEN(ifq)		((ifq)->ifq_len++)
#define	IFQ_DEC_LEN(ifq)		(--(ifq)->ifq_len)

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
// musterc+: 2004.07.23 to avoid compile warnings
struct mbuf;

struct schedq{
	void *qattr;
	int	(*enqueue) __P((struct schedq *, struct mbuf *));
	struct	mbuf *(*dequeue) __P((struct schedq *));
	void (*request) __P((struct schedq *, int));
	struct ifqueue *ifq;
};

struct class_queue{
	struct mbuf	*tail;	/* Tail of packet queue */
	int	qlen;		/* Queue length (in number of packets) */
	int	qlim;		/* Queue limit (in number of packets*) */
	
	struct filter{
		int type;
		unsigned char tos_msk;
	}cl_fil;
};

#define	qlimit(q)	(q)->qlim		/* Max packets to be queued */
#define	qlen(q)		(q)->qlen		/* Current queue length. */
#define	qtail(q)	(q)->tail		/* Tail of the queue */
#define	qhead(q)	((q)->tail ? (q)->tail->m_nextpkt : NULL)
#define	qempty(q)	((q)->qlen == 0)	/* Is the queue empty?? */

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================

#endif // LIB_QOS_H

