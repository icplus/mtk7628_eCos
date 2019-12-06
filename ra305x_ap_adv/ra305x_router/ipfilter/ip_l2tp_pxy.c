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

//==============================================================================
//                                    MACROS
//==============================================================================
#define IPF_L2TP_PROXY

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
struct l2tphdr
{
    uint16_t msg_type;		/* Message type */
    uint16_t length;		/* Length (opt) */
    uint16_t tid;		/* Tunnel ID */
    uint16_t sid;		/* Session ID */
    uint16_t Ns;		/* Ns (opt) */
    uint16_t Nr;		/* Nr (opt) */
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
int ippr_l2tp_init(void)
{
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
int ippr_l2tp_new(fin, aps, nat)
fr_info_t *fin;
ap_session_t *aps;
nat_t *nat;
{
	KMALLOCS(aps->aps_data, u_32_t *, sizeof(u_32_t));
	if(aps->aps_data == NULL) return -1;
	memset(aps->aps_data, 0, sizeof(u_32_t));
	aps->aps_psiz = sizeof(u_32_t);
	
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
int ippr_l2tp_match(fin, aps, nat)
fr_info_t *fin;
ap_session_t *aps;
nat_t *nat;
{
	struct l2tphdr l2tp;
	unsigned short	*tid;
	mb_t *m;
	int off;
	
	if ((fin->fin_dlen-sizeof(udphdr_t)) < sizeof(struct l2tphdr))
		return -1;
		
	m = *(mb_t **)fin->fin_mp;
	off = fin->fin_hlen + sizeof(udphdr_t);
	m_copydata(m, off, sizeof(struct l2tphdr), (char *)&l2tp);
	tid = (unsigned short *)(aps->aps_data);
	if(tid[fin->fin_out] == 0) 
	{
		tid[fin->fin_out] = l2tp.tid;
	} 
	else 
	{
		if(tid[fin->fin_out] != l2tp.tid)
		{
			return -1;
		}
	}
	
	return 0;
}

