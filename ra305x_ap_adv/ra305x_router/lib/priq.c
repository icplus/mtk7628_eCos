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

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <network.h>
#include <netinet/if_ether.h>
#include <sys/mbuf.h>
#include <net/if_qos.h>
#include <priq.h>

//==============================================================================
//                                    MACROS
//==============================================================================

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
static inline struct mbuf *_getq(struct class_queue *clq)
{
	struct mbuf  *m, *m0;
	
	if ((m = qtail(clq)) == NULL)
		return (NULL);
	if ((m0 = m->m_nextpkt) != m)
		m->m_nextpkt = m0->m_nextpkt;
	else
		qtail(clq) = NULL;
	qlen(clq)--;
	m0->m_nextpkt = NULL;
	return (m0); 
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
static inline void _addq(struct class_queue *clq, struct mbuf *m)
{
        struct mbuf *m0;
	
	if ((m0 = qtail(clq)) != NULL)
		m->m_nextpkt = m0->m_nextpkt;
	else
		m0 = m;
	m0->m_nextpkt = m;
	qtail(clq) = m;
	qlen(clq)++;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
static inline void _purgeq(struct class_queue *clq)
{
	struct mbuf *m;

	if (qempty(clq))
		return;

	while ((m = _getq(clq)) != NULL) {
		m_freem(m);
	}
	clq->qlen = 0;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int priQ_class_create(struct schedq *sq, int type, unsigned char tos)
{
	struct priq_attr_t *attr=sq->qattr;
	struct class_queue *clq;
	int i;
	
	if(attr->priq_max >= PRIQ_MAX)
	{
		diag_printf("excess %d classes, can not create\n", PRIQ_MAX);
		return -1;
	}	
	
	clq = (struct class_queue *)malloc(sizeof(struct class_queue));
	if(clq == NULL)
	{
		diag_printf("No Memory for create class\n");
		return -1;
	}
	
	bzero(clq, sizeof(struct class_queue));
	
	for(i=0; i<PRIQ_MAX; i++)
	{
		if(attr->priq[i] == NULL)
		{
			clq->cl_fil.type = type;
			if(type == CLASS_DEFAULT)
				attr->def = clq;
			if(type == CLASS_TOS)
				clq->cl_fil.tos_msk = tos;
			
			clq->qlim = IFQ_MAXLEN;	// need adjust ?
			attr->priq[i] = clq;
			attr->priq_max++;
			
			return 0;
		}
	}
	
	free(clq);
	
	return -1;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int priQ_class_destroy_all(struct schedq *sq)
{
	struct priq_attr_t *attr=sq->qattr;
	struct class_queue *clq;
	int i;
	
	for(i=0; i<attr->priq_max; i++)
	{
		if((clq = attr->priq[i]) != NULL)
		{
			if (!qempty(clq))
				_purgeq(clq);
			free(attr->priq[i]);
			attr->priq[i] = NULL;
			attr->priq_max--;
		}
	}
	return 0;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
struct class_queue *priq_classifier(struct schedq *sq, struct mbuf *m)
{
	struct ip *ip;
	struct priq_attr_t *attr=sq->qattr;
	int idx;
	register struct class_queue *clq;
	register struct ether_header *eh;
	
	eh = mtod(m, struct ether_header *);
	if(eh->ether_type == htons(ETHERTYPE_IP))
	{
		ip = (struct ip *)((char *)eh+14);
	}
	else
	{
		clq = attr->def;
		//*(unsigned long *)0xb4000090 = '+';
		goto out;
	}
		
	for(idx=0; idx<attr->priq_max ;idx++)
	{
		if((clq = attr->priq[idx])==NULL)
			continue;
		if(clq->cl_fil.type == CLASS_TOS)
		{
			if(clq->cl_fil.tos_msk == ip->ip_tos)
			{
				//*(unsigned long *)0xb4000090 = '!';
				break;
			}
		}
	}
	
	if(idx == attr->priq_max)
	{
		//*(unsigned long *)0xb4000090 = '+';
		clq = attr->def;
	}
out:	
	return clq;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int priq_enqueue(struct schedq *sq, struct mbuf *m)
{
	register struct class_queue *clq; 
	
	if((clq = priq_classifier(sq, m))==NULL)
		return (-1);
	
	if (qlen(clq) >= qlimit(clq)) {
		m_freem(m);
		return (-1);
	}
	
	_addq(clq, m);
	IFQ_INC_LEN((struct	ifqueue *)(sq->ifq));
	
	return 0;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
struct	mbuf * priq_dequeue(struct schedq *sq)
{
	int i;
	struct priq_attr_t *attr=sq->qattr;
	register struct class_queue *clq; 
	struct mbuf *m;
	
	for(i=0; i<attr->priq_max; i++)
	{
		clq = attr->priq[i];
		
		if(clq && !qempty(clq))
		{
			m = _getq(clq);
			if(m != NULL)
				IFQ_DEC_LEN((struct	ifqueue *)(sq->ifq));
				
			//*(unsigned long *)0xb4000090 = '-';
			return m;
		}
	}
	
	return NULL;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
static void priq_purge(struct schedq *sq)
{
	int i;
	struct priq_attr_t *attr=sq->qattr;
	struct class_queue *clq; 
	
	for(i=0; i<attr->priq_max; i++)
	{
		clq = attr->priq[i];
		_purgeq(clq);
	}
	((struct	ifqueue *)(sq->ifq))->ifq_len = 0;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void priq_requst(struct schedq *sq, int flag)
{
	
	switch(flag)
	{
		case SCHED_FLUSHQ:
			priq_purge(sq);
			break;
		default:
			break;
	}
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void schedQ_register(struct ifnet *ifp, struct schedq *sq)
{
	struct	ifqueue *ifq = &ifp->if_snd;
	
	/* flush older packet */
	if(ifq->ifq_len != 0)
		if_qflush(&ifp->if_snd);
		
	sq->ifq = ifq;
	ifq->schedq = sq;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void schedQ_unregister(struct ifnet *ifp)
{
	struct	ifqueue *ifq = &ifp->if_snd;
	ifq->schedq = NULL;
}

