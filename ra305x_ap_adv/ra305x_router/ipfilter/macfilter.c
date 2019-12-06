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
#include <net/if.h>
#include <netinet/in_var.h>
#include <sys/mbuf.h>
#include <stdio.h>
#include <macf.h>
#include <cfg_net.h>
#include <sys_status.h>
#include <lib_packet.h>
#include <eventlog.h>

//==============================================================================
//                                    MACROS
//==============================================================================
#define MAC2INDEX(mac)	((mac)[3]+(mac)[4]+(mac)[5])&0xff

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
#define HASHTB_NUM		256
static struct macfilter* macfilhash[HASHTB_NUM];
static char daymap[7]={4,5,6,0,1,2,3};
int default_action;

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================

//------------------------------------------------------------------------------
// FUNCTION
//		macfilter_check_acttime
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
void macfilter_check_acttime(void)
{
	int i;
	struct macfilter *pfilter;
	int curtime, secs, weekday;
		
	curtime = GMTtime(0);
	weekday = curtime % 604800;
	secs = weekday % 86400;
	weekday /= 86400;

	for (i=0; i< HASHTB_NUM; i++) {
		for (pfilter=macfilhash[i]; pfilter; pfilter=pfilter->next) {
			if (pfilter->flags & MACF_FG_VALID) {
				switch(pfilter->macf_mode) {
				case MACF_MODE_PERIOD:
					if(weekday >= 0 && weekday <= 6 )
						{
					if (!(pfilter->macf_tday & (1<<daymap[weekday]))) {
						if (pfilter->flags & MACF_FG_ACTIVE)
							pfilter->flags &= ~MACF_FG_ACTIVE;
						continue;
					}
						}
					/*  Fall to MACF_MODE_ONE  */
				case MACF_MODE_ONE:
					if ((secs >= pfilter->macf_tstart) && 
						(secs <= (pfilter->macf_tend ? pfilter->macf_tend : 86400))) {
						if (!(pfilter->flags & MACF_FG_ACTIVE))
							pfilter->flags |= MACF_FG_ACTIVE;
					} else {
						if (pfilter->flags & MACF_FG_ACTIVE)
							pfilter->flags &= ~MACF_FG_ACTIVE;
					}
					break;
				case MACF_MODE_ALWAYS:
				default:
					/*  Make the rule active if it is not.  */
					if (!(pfilter->flags & MACF_FG_ACTIVE))
						pfilter->flags |= MACF_FG_ACTIVE;
					break;
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
// FUNCTION
//		macfilter_match
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
int macfilter_match(struct ifnet *ifp, char *head, struct mbuf *m)
{
	struct macfilter *entry;
	struct ip *ip;
	int idx;

	if(m->m_pkthdr.len < 20)
		return default_action;
	ip = mtod(m, struct ip *);
    if ((ip->ip_dst.s_addr == SYS_lan_ip) ||
    	(ip->ip_dst.s_addr == INADDR_BROADCAST)){
        return MACF_PASS;
    }
        
	idx = MAC2INDEX(&head[6]);
	
	for (entry = macfilhash[idx]; entry != NULL; entry = entry->next) {
		if (! (entry->flags & MACF_FG_ACTIVE))
			continue;
		if(!memcmp(&head[6], entry->mac, 6)) {
#ifdef CONFIG_MACFLT_LOG
			if((MACF_BLOCK^default_action) == MACF_BLOCK)
				SysLog(LOG_DROPPK|SYS_LOG_NOTICE|LOGM_FIREWALL, "**%s** [%s] match", STR_FILTER, ether_ntoa((struct ether_addr *)(entry->mac)));
#endif
			return MACF_BLOCK^default_action;
		}
	}
	return default_action;
}

//------------------------------------------------------------------------------
// FUNCTION
//		mcfilter_init
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
void mcfilter_init(void)
{
	memset(macfilhash, 0, sizeof(macfilhash));
	default_action = MACF_PASS;
}

//------------------------------------------------------------------------------
// FUNCTION
//		mcfilter_act
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
void mcfilter_act(void)
{
	struct ifnet *ifp;
	ifp = ifunit(LAN_NAME);
	LIB_ether_input_register_tail(ifp, macfilter_match, FORWARDQ);
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
void mcfilter_inact(void)
{
	struct ifnet *ifp;
	ifp = ifunit(LAN_NAME);
	LIB_ether_input_unregister(ifp, macfilter_match, FORWARDQ);
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
int macfilter_del(struct macfilter *macinfo)
{
	int idx = MAC2INDEX(macinfo->mac);
	struct macfilter *entry, *pre_entry=0;
	
	entry = macfilhash[idx];
	while(entry) {
		if(!memcmp(macinfo->mac, entry->mac, 6)){
			if(pre_entry)
				pre_entry->next = entry->next;
			else
				macfilhash[idx] = entry->next;
			free(entry);
			return 0;
		}
		pre_entry = entry;
		entry = entry->next;
	}
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
int macfilter_add(struct macfilter *macinfo)
{
	struct macfilter *entry;
	int idx = MAC2INDEX(macinfo->mac);
	
	macfilter_del(macinfo); // prevent dupication
	
	entry = (struct macfilter *)malloc(sizeof(struct macfilter));
	if(!entry)
		return -1;
		
	memcpy(entry, macinfo, sizeof(struct macfilter));
	entry->flags = MACF_FG_VALID;
	if (macinfo->macf_mode == MACF_MODE_ALWAYS)
		entry->flags |= MACF_FG_ACTIVE;
	entry->next = macfilhash[idx];
	macfilhash[idx] = entry;
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
void macfilter_flush(void)
{
	struct macfilter *entry, *next_entry;
	int i;
	
	for(i=0 ; i< 256 ; i++) {
		entry = macfilhash[i];
		macfilhash[i] = NULL;
		while(entry) {
			next_entry = entry->next;
			free(entry);
			entry = next_entry;
		}
	}
//	memset(macfilhash, 0, HASHTB_NUM*sizeof(struct macfilter *));
}


