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
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <net/if.h>
#include <stdio.h>
#include <ip_compat.h>
#include <ip_fil.h>
#include <ip_defense.h>
#include <eventlog.h>

//==============================================================================
//                                    MACROS
//==============================================================================
#define ATT_PASS	0
#define ATT_BLOCK	1
#define ATT_MAX_NUM	50

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
struct attacker_e{
	unsigned int addr;
	int count;
	int rec_time;
	char flag;
	struct attacker_e *next;
};

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
struct attacker_e *attacker_list = NULL;
static int attacker_num=0;

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================
void security_log(fr_info_t *fin, char *typestr, int logclass);

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
void attacker_record(fr_info_t *fin)
{
	struct attacker_e *ent=attacker_list;
	//diag_printf("attacker_record: fin->fin_saddr:%x, port:%x\n", fin->fin_saddr, fin->fin_data[1]);
	for(; ent; ent = ent->next)
	{
		if(ent->addr == fin->fin_saddr)
		{
			if((ent->count)++ >= PORTSCAN_REC )
			{
				char srcip[15], dstip[15];
				char block_msg[20];
				*block_msg='\0';
				strcpy(srcip, inet_ntoa(*(struct in_addr *)&fin->fin_saddr));
				strcpy(dstip, inet_ntoa(*(struct in_addr *)&fin->fin_daddr));
#ifdef CONFIG_FW_ISOLATE_PORTSCANER
				ent->flag = ATT_BLOCK;
				sprintf(block_msg, ", "STR_deny " %s for 5 " STR_mins, srcip);
#endif
				SysLog(LOG_AUTH|SYS_LOG_NOTICE|LOGM_FIREWALL,"**" STR_Port_Scan_Detected "** %s -> %s%s", srcip, dstip, block_msg);
			}
			
			break;
		}
	}
	/* add new record into list */
	if(attacker_num < ATT_MAX_NUM && !ent)
	{
		ent = malloc(sizeof(struct attacker_e));
		if(ent)
		{	
			ent->addr = fin->fin_saddr;
			ent->count = 1;
			ent->flag = ATT_PASS;
			ent->next = attacker_list;
			attacker_list = ent;
			attacker_num++;
		}
	}
	
	if(ent)
		ent->rec_time = time(0);
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
int attacker_clean(void)
{
	struct attacker_e *ent=attacker_list;
	struct attacker_e *pre=0;
	int curt;
	
	if(attacker_num == 0)
		return 0;
		
	curt = time(0);
	while(ent)
	{
		if((ent->flag != ATT_BLOCK) && ((curt - ent->rec_time) >= 1))
		{
			ent->count = 0;
		}
		
		if((curt - ent->rec_time) > 300)
		{
			cyg_scheduler_lock();
			if(pre)
				pre->next = ent->next;
			else
				attacker_list = ent->next;
			free(ent);
			if(pre)
				ent = pre->next;
			else
				ent = attacker_list;
				
			attacker_num--;
			cyg_scheduler_unlock();
		}
		else
		{
			pre = ent;
			ent = ent->next;
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
int check_attacker(fr_info_t *fin)
{
	struct attacker_e *ent=attacker_list;
	
	/* check attacker list */
	if(attacker_num > 0)
	{	
		for(; ent; ent = ent->next)
		{
			if(ent->addr == fin->fin_saddr)
			{
				if(ent->flag)
					return ent->flag;
				else
					break;
			}
		}	
	}
	
	if(DEF_IP_FRAG && (fin->fin_fl & FI_FRAG))
	{
		return 1;
	}
	if(DEF_SHORT_PACKET && (fin->fin_fl & FI_SHORT))
	{
		security_log(fin, "IP with zero Length", LOG_AUTH);
		return 1;
	}
	if(DEF_PING_OF_DEATH && (fin->fin_fl & FI_FRAG) && 
		(((fin->fin_off<<3) + fin->fin_dlen + fin->fin_hlen) > (unsigned int)(0xffff)))
	{
		security_log(fin, "Ping of Death", LOG_AUTH);
		return 1;
	}
	
	/* below defense rule do not check fragement packet */
	if((fin->fin_fl & FI_FRAG)&&(fin->fin_off != 0))
		return 0;
	
	if(DEF_LAND_ATTACK && (fin->fin_p == IPPROTO_TCP) && 
		(fin->fin_saddr == fin->fin_daddr) && (fin->fin_data[0]==fin->fin_data[1]))
	{
		security_log(fin, "Land Attack", LOG_AUTH);
		return 1;
	}
	if(DEF_SNORK_ATTACK && (fin->fin_p == IPPROTO_UDP) && 
		((fin->fin_data[0] == 7 || fin->fin_data[0] == 19 || fin->fin_data[0] == 135) && fin->fin_data[1] == 135))
	{
		security_log(fin, "Snork", LOG_AUTH);
		return 1;
	}
	if(DEF_UDP_PORT_LOOP && (fin->fin_p == IPPROTO_UDP) && 
		((fin->fin_data[0] == 7 || fin->fin_data[0] == 17 || fin->fin_data[0] == 19) && 
		(fin->fin_data[1] == 7 || fin->fin_data[1] == 17 || fin->fin_data[1] == 19)))
	{
		security_log(fin, "UDP Port LookBack", LOG_AUTH);
		return 1;
	}
	if(DEF_TCP_NULL_SCAN && (fin->fin_p == IPPROTO_TCP) && !fin->fin_tcpf)
	{
		security_log(fin, "TCP Null Scan", LOG_AUTH);
		return 1;
	}
	if(DEF_SYN_FLOODING && (fin->fin_p == IPPROTO_TCP) &&
		(fin->fin_tcpf == TH_SYN) && !ppsratecheck(&SYN_FLOOD_RATE))
	{
		security_log(fin, "TCP SYN Flooding", LOG_AUTH);
		return 1;
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
void show_attacker_list(void)
{
	struct attacker_e *ent=attacker_list;
	int curt = time(0);
	
	diag_printf("Total Number:%d\n", attacker_num); 	
	for(; ent; ent = ent->next)
	{
		diag_printf("%x block:%x count:%d time:%d\n", ent->addr, ent->flag, ent->count, (curt - ent->rec_time)); 	
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
int ppsratecheck(struct rate_limit *rate)
{
	struct timeval tv;
	unsigned int delta;
	int rv;

	microtime(&tv);

	delta = tv.tv_sec - rate->lasttime;

	/*
	 * check for 0,0 is so that the message will be seen at least once.
	 * if more than one second have passed since the last update of
	 * lasttime, reset the counter.
	 *
	 * we do increment *curpps even in *curpps < maxpps case, as some may
	 * try to use *curpps for stat purposes as well.
	 */
	if ((rate->lasttime == 0) || (delta >= 1)) {
		rate->lasttime = tv.tv_sec;
		rate->curpps = 0;
		rv = 1;
	} else if (rate->maxpps == 0)
		rv = 1;
	else if (rate->curpps < rate->maxpps)
		rv = 1;
	else
		rv = 0;
	rate->curpps = rate->curpps + 1;

	return (rv);
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
void security_log(fr_info_t *fin, char *typestr, int logclass)
{
	char srcip[15], dstip[15];
	char *ptype;
	
	switch(fin->fin_p)
	{
		case IPPROTO_ICMP:
			ptype = "IP/ICMP";
			break;
		case IPPROTO_TCP:
			ptype = "IP/TCP";
			break;
		case IPPROTO_UDP:
			ptype = "IP/UDP";
			break;
		default:
			ptype = "";
			break;
	}
	strcpy(srcip, inet_ntoa(*(struct in_addr *)&fin->fin_saddr));
	strcpy(dstip, inet_ntoa(*(struct in_addr *)&fin->fin_daddr));
	if((fin->fin_p == IPPROTO_TCP) ||(fin->fin_p == IPPROTO_UDP))
		SysLog(logclass|SYS_LOG_NOTICE|LOGM_FIREWALL,"**%s** %s %s:%d->%s:%d", 
			typestr, ptype, srcip, fin->fin_data[0], dstip, fin->fin_data[1]);
	else
		SysLog(logclass|SYS_LOG_NOTICE|LOGM_FIREWALL, "**%s** %s %s->%s", 
			typestr, ptype, srcip, dstip);
}


