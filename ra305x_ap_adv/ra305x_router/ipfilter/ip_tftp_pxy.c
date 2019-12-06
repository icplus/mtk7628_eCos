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
#define IPF_TFTP_PROXY

#define TFTP_RRQ	1
#define TFTP_WRQ	2

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
int ippr_tftp_init(void)
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
int ippr_tftp_out(fin, ip, aps, nat)
fr_info_t *fin;
ip_t *ip;
ap_session_t *aps;
nat_t *nat;
{
	unsigned short opcode;
	udphdr_t *udp;
	
	udp = (udphdr_t *)fin->fin_dp;
	opcode = ntohs(*(unsigned short *)((unsigned char *)udp + sizeof(udphdr_t)));
	
	if((opcode == TFTP_RRQ) || (opcode == TFTP_WRQ))
	{
		nat_t     *ipn;
	
		ipn = nat_alloc(NAT_OUTBOUND, IPN_UDP|FI_W_DPORT);
		if (ipn == NULL) 
		{
			nat_stats.ns_memfail++;
			return 0;
		}
		ipn->nat_ifp = fin->fin_ifp;
		nat_setup(ipn, fin->fin_src.s_addr, fin->fin_dst.s_addr, SYS_wan_ip, fin->fin_p, nat->nat_inport, 0, nat->nat_outport);
		ipn->nat_fstate.rule = nat->nat_fstate.rule;
	}
	return 0;
}


