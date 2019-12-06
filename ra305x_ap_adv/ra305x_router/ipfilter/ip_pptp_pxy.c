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
#define IPF_PPTP_PROXY

#define PPTP_CONTROL_PACKET            1
#define PPTP_MGMT_PACKET               2

/* PptpControlMessageType values */
#define PPTP_START_SESSION_REQUEST     1
#define PPTP_START_SESSION_REPLY       2
#define PPTP_STOP_SESSION_REQUEST      3
#define PPTP_STOP_SESSION_REPLY        4
#define PPTP_ECHO_REQUEST              5
#define PPTP_ECHO_REPLY                6
#define PPTP_OUT_CALL_REQUEST          7
#define PPTP_OUT_CALL_REPLY            8
#define PPTP_IN_CALL_REQUEST           9
#define PPTP_IN_CALL_REPLY             10
#define PPTP_IN_CALL_CONNECT		   11
#define PPTP_CALL_CLEAR_REQUEST        12
#define PPTP_CALL_DISCONNECT_NOTIFY    13
#define PPTP_WAN_ERROR_NOTIFY          14
#define PPTP_SET_LINK_INFO             15

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
struct	pptphdr
{
	u_short		len;
	u_short		pptptype;
	u_long		cookie;
	u_short		ctrltype;
	u_short		reserved1;
	u_short		callid;
	u_short		pcallid;
};

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
static	frentry_t	pptp_fr;

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
int ippr_pptp_init(void)
{
	bzero((char *)&pptp_fr, sizeof(pptp_fr));
	pptp_fr.fr_ref = 1;
	pptp_fr.fr_flags = FR_INQUE|FR_PASS|FR_QUICK|FR_KEEPSTATE;

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
int ippr_pptp_new(fr_info_t *fin, ap_session_t *aps, nat_t *nat)
{
	aps->aps_data = NULL;
	aps->aps_psiz = 0;

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
void ippr_pptp_del(ap_session_t *aps)
{
	
	return;
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
int ippr_pptp_out(fr_info_t *fin, ip_t *ip, ap_session_t *aps, nat_t *nat)
{
	int pptplen;
	tcphdr_t *tcp;
	struct pptphdr *pptp;
	int type;
	
	tcp = (tcphdr_t *)fin->fin_dp;
	pptp = (unsigned char *)tcp + (tcp->th_off << 2);
	pptplen = fin->fin_dlen - (tcp->th_off << 2);
	
	// if length less than 4, do not check
	if(pptplen < 4)
		return 0;
		
	if(ntohs(pptp->pptptype) != PPTP_CONTROL_PACKET)
		return 0;
		
	type = ntohs(pptp->ctrltype);
	switch(type)
	{
		case PPTP_OUT_CALL_REQUEST:
		case PPTP_OUT_CALL_REPLY:
		case PPTP_CALL_DISCONNECT_NOTIFY:
		case PPTP_CALL_CLEAR_REQUEST:
		{
			fr_info_t fi;
			nat_t     *ipn;	
			grehdr_t gre;
			
			gre.gr_call = pptp->callid;
			memcpy(&fi, fin, sizeof(fr_info_t));
			fi.fin_data[0] = 0;
			fi.fin_data[1] = 0;
			fi.fin_fi.fi_p = IPPROTO_GRE;
			fi.fin_dp = (char *)&gre;
			ipn = nat_outlookup(&fi, NULL, IPPROTO_GRE,
				nat->nat_inip, ip->ip_dst, 0);
			
			if((type==PPTP_OUT_CALL_REQUEST || type == PPTP_OUT_CALL_REPLY) && (ipn == NULL))
			{
				fi.fin_src = nat->nat_inip;
				ipn = nat_new(&fi, ip, nat->nat_ptr, NULL, NULL, NAT_OUTBOUND);
				if(ipn)
				{
					ipn->nat_gre.gs_call = pptp->callid;
					ipn->nat_fstate.rule = nat->nat_fstate.rule;
				}
			}else
			if((type==PPTP_CALL_DISCONNECT_NOTIFY|| type == PPTP_CALL_CLEAR_REQUEST) && (ipn != NULL))
				ipn->nat_age = 1;
			break;
		}
		
		default:
			break;
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
int ippr_pptp_in(fr_info_t *fin, ip_t *ip, ap_session_t *aps, nat_t *nat)
{
	int pptplen;
	tcphdr_t *tcp;
	struct pptphdr *pptp;
	int type;
	
	tcp = (tcphdr_t *)fin->fin_dp;
	pptp = (unsigned char *)tcp + (tcp->th_off << 2);
	pptplen = fin->fin_dlen - (tcp->th_off << 2);
	// if length less than 4, do not check
	if(pptplen < 4)
		return 0;
		
	if(ntohs(pptp->pptptype) != PPTP_CONTROL_PACKET)
		return 0;
		
	type = ntohs(pptp->ctrltype);
	switch(type)
	{
		case PPTP_OUT_CALL_REQUEST:
		case PPTP_OUT_CALL_REPLY:
		case PPTP_CALL_DISCONNECT_NOTIFY:
		case PPTP_CALL_CLEAR_REQUEST:
		{
			fr_info_t fi;
			nat_t     *ipn;	
			grehdr_t gre;
			
			gre.gr_call = pptp->callid;
			memcpy(&fi, fin, sizeof(fr_info_t));
			fi.fin_data[0] = 0;
			fi.fin_data[1] = 0;
			fi.fin_fi.fi_p = IPPROTO_GRE;
			fi.fin_dp = (char *)&gre;
			ipn = nat_outlookup(&fi, NULL, IPPROTO_GRE,
				nat->nat_inip, ip->ip_src, 0);
			
			if((type==PPTP_OUT_CALL_REQUEST || type == PPTP_OUT_CALL_REPLY) && (ipn == NULL))
			{
				fi.fin_src = nat->nat_inip;
				fi.fin_dst = ip->ip_src;
				ipn = nat_new(&fi, ip, nat->nat_ptr, NULL, NULL, NAT_OUTBOUND);
				if(ipn)
				{
					ipn->nat_gre.gs_call = pptp->callid;
					ipn->nat_fstate.rule = nat->nat_fstate.rule;
				}
			}else
			if((type==PPTP_CALL_DISCONNECT_NOTIFY|| type == PPTP_CALL_CLEAR_REQUEST) && (ipn != NULL))
				ipn->nat_age = 1;
			break;
		}
		
		default:
			break;
	}
	 
	return 0;
}


