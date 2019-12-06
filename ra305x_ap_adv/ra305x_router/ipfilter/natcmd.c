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
#include <config.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <cfg_def.h>
#include <network.h>
#include <cfg_net.h>
#include <sys_status.h>

#include <sys/param.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <ip_compat.h>
#include <ip_fil.h>
#include <ip_nat.h>
#include <ipf.h>
#include <ip_proxy.h>

//==============================================================================
//                                    MACROS
//==============================================================================
#define MAX_NAT_ARGS	10

#define PMAP_MAX_NUM	50
#define VTS_MAX_NUM		50
#define PTRG_MAX_NUM	50
#define DMZ_MAX_NUM		(WAN_IPALIAS_NUM+1)
#define MAX_SCHE_NUM		20


//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================
int nat_enabled=0;
#ifdef CONFIG_SUPERDMZ
char *superdmz_mac=0;
#endif


//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
extern ipnat_t *natparse(char *, int);
extern aproxy_t *appr_find(int, unsigned short);
extern int str2arglist(char *, char **, char, int);


//------------------------------------------------------------------------------
// FUNCTION  
//		NAT_getfwsched
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
int NAT_getfwsched(char *name, int *sched)
{
	char line[255];
	char *arglists[8];
	int i;
	char *e_time;
	
	if (name == NULL || sched == NULL)
		return -1;
	if (strcasecmp(name, "always") == 0)
		return 0;
	
	for (i=0; i<MAX_SCHE_NUM; i++)
	{
		if ((CFG_get(CFG_NAT_SCHED+i+1, line) == -1))
			continue;
		
		if (str2arglist(line, arglists, ';', 8) >= 4)
		{
			if (strcasecmp(name, arglists[0]) == 0)
			{
				e_time = arglists[1];
				e_time = strchr(e_time, '-');
				
				if (e_time)
					*e_time++ = '\0';
				else
					e_time = arglists[1];
				
				sched[0] = atoi(arglists[1]);
				sched[1] = atoi(e_time);
				sched[2] = atoi(arglists[2]);
				
				return (i+1);
			}
		}
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
int natargslist(char *buf, int *list)
{
	char *idx = buf;
	int j=0;
	
	list[j++] = (int)buf;
	while(*idx) {
		if(*idx == '=' || *idx == ',' || *idx == '\n') {
			*idx = 0;
			list[j++] = ((int) idx) +1;
		}
		idx++;	
	}
	if(j==1 && !(*buf)) // No args
		j = 0;
		
	return j;
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
int NAT_setrule(char *natcmd)
{
	ipnat_t	*np;

	np = natparse(natcmd, 0);
	if  (!np)
		goto fail;

	if (nat_ioctl(&np, SIOCADNAT, O_RDWR) == -1)
		goto fail;

	return 0;

fail:
	diag_printf("add NAT entry fail: %s\n", natcmd);
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
int NAT_delrule(char *natcmd)
{
	ipnat_t	*np;
	
	np = natparse(natcmd, 0);
	if ( nat_ioctl(&np, SIOCRMNAT, O_RDWR) == -1) {
		diag_printf("delete NAT entry fail\n");
		return -1;
	}
	return 0;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//		convert to ipfilter rule
// NAT_PMAP[NUM]=[enable];[description];[external port];[external protocol];[internal IP]
// ex. NAT_PMAP1=1;gameabc;3333-4444;tcp;192.168.5.100
// 	
// NAT_PTRG[NUM]=[enable];[description];[external port];[external protocol];[trigger port];[trigger protocol]
// ex. NAT_PTRG1=1;gameqqq;2210-2215;udp;2208-2209;tcp
//
// NAT_VTS[NUM]=[enable];[description];[external port];[external protocol];[internal IP];[internal port];[schedule enable];[schedule time range];[schedule day]
// ex. NAT_VTS1=0;webserver;80;tcp;192.168.5.120;8080
//
// NAT_ALG[NUM]=[enable];[description];[external port];[external protocol]
// ex. NAT_ALG=0;ftp;21;tcp
//
// NAT_DMZ_HOST[NUM]=[enable];[session];[internal IP]
//	ex.	NAT_DMZ_HOST1=1;0;192.168.5.100
// PARAMETERS
//      
//      
// RETURN
//      
//      
//------------------------------------------------------------------------------
void NAT_cmd2rule(void)
{
	char line[256];
	char *arglists[MAX_NAT_ARGS];
	int i, j, val;
	char wanipstr[PRIMARY_WANIP_NUM][16], ipstr[16];
	char cmdstr[256], scmdstr[100], schedcmd[50];
	struct in_addr addr;
	
#ifdef VIRTUAL_SERVER_ALG
	aproxy_t *apr;
	struct virtual_aproxy *vapr=NULL;
#endif
	for(i=0; i<PRIMARY_WANIP_NUM; i++)
		strcpy(wanipstr[i], NSTR(primary_wan_ip_set[i].ip));
	
	/* port mapping to ipnat rule */
	for(i=0; i< PMAP_MAX_NUM; i++)
	{
		if(CFG_get(CFG_NAT_PMAP+i+1, line) == -1)
			continue;
		if((str2arglist(line, arglists, ';', 5) == 5) && (atoi(arglists[0]) == 1))
		{
			char *mport, ports[12], *porte, *remain;
			mport = arglists[2];
			
			while(mport)
			{
				remain = strchr(mport, ',');
				if(remain)
					*remain++ = '\0';
				strcpy(ports, mport);
				porte =	strchr(ports, '-');
				if(porte)
					*porte= '\0';
					
				inet_aton(arglists[4], &addr);	
				addr.s_addr = (SYS_lan_ip & SYS_lan_mask)|(addr.s_addr & ~SYS_lan_mask);		
				strcpy(ipstr, NSTR(addr));
				for (j=0; j<PRIMARY_WANIP_NUM; j++) {
					
					if (!(primary_wan_ip_set[j].flags & IPSET_FG_NATIF))
						continue;
					sprintf(cmdstr, "rdr all %s/32 port %s -> %s port %s %s\n",
							wanipstr[j], mport, ipstr, ports, arglists[3]);
					
					if ((primary_wan_ip_set[j].up == CONNECTED) && 
						!(primary_wan_ip_set[j].flags & IPSET_FG_NATCONFIG))
							NAT_setrule(cmdstr);
					else if ((primary_wan_ip_set[j].up != CONNECTED) && 
						(primary_wan_ip_set[j].flags & IPSET_FG_NATCONFIG))
							NAT_delrule(cmdstr);
				}
				mport = remain;
			}
		}
	}
	
	/* virtual server to ipnat rule */
	for(i=0; i< VTS_MAX_NUM; i++)
	{
		char *etime;
		int nat_sched[3];

		if(CFG_get(CFG_NAT_VTS+i+1, line) == -1)
			continue;
#ifndef VIRTUAL_SERVER_ALG

	if((str2arglist(line, arglists, ';', 9) == 9) && (atoi(arglists[0]) == 1))	
#else
		if((str2arglist(line, arglists, ';', 10) ==10) && (atoi(arglists[0]) == 1))
#endif
		{
			char ports[12], *porte;
			int val2=0;
			
			scmdstr[0] = '\0';
			strcpy(ports, arglists[2]);
			porte =	strchr(ports, '-');
			if(porte)
			{
				*porte++ = '\0';
				val2 = atoi(porte);
			}
			val = atoi(ports);

			schedcmd[0] = 0;		/*  Assume always on first  */
			switch (atoi(arglists[6]))
			{
			case 1:
				etime = arglists[7];
				etime = strchr(etime, '-');
				if(etime)
					*etime++ = '\0';
				else
					panic("NAT_cmd2rule etime=NULL\n");
				sprintf(schedcmd, "act-time period %s %s %s", arglists[8], arglists[7], etime);
				break;
			case 2:
				if (NAT_getfwsched(arglists[7], nat_sched) > 0)
					sprintf(schedcmd, "act-time period %d %d %d", nat_sched[2], nat_sched[0], nat_sched[1]);
				break;
			default:
				break;
			}

			/* special application */
			if((val == 1720) || (val2 && ((val <= 1720)&&(val2 >= 1720))))
			{
				for (j=0; j<PRIMARY_WANIP_NUM; j++) {
					if (!(primary_wan_ip_set[j].flags & IPSET_FG_NATIF))
						continue;
					
					sprintf(cmdstr, "rdr %s %s/32 port 1503 -> %s port 1503\n",
						primary_wan_ip_set[j].ifname, wanipstr[j], arglists[4]);

					if ((primary_wan_ip_set[j].up == CONNECTED) && 
						!(primary_wan_ip_set[j].flags & IPSET_FG_NATCONFIG))
							NAT_setrule(cmdstr);
					else if ((primary_wan_ip_set[j].up != CONNECTED) && 
						(primary_wan_ip_set[j].flags & IPSET_FG_NATCONFIG))
							NAT_delrule(cmdstr);
				}
			}	
				
			inet_aton(arglists[4], &addr);	
			addr.s_addr = (SYS_lan_ip & SYS_lan_mask)|(addr.s_addr & ~SYS_lan_mask);		
			strcpy(ipstr, NSTR(addr));
			
			for (j=0; j<PRIMARY_WANIP_NUM; j++) {
				if (!(primary_wan_ip_set[j].flags & IPSET_FG_NATIF))
					continue;
				sprintf(cmdstr, "rdr all %s/32 port %s -> %s port %s %s%s %s\n",
				wanipstr[j], arglists[2], ipstr, arglists[5], arglists[3], scmdstr, schedcmd);
				if ((primary_wan_ip_set[j].up == CONNECTED) && 
					!(primary_wan_ip_set[j].flags & IPSET_FG_NATCONFIG))
					NAT_setrule(cmdstr);
				else if ((primary_wan_ip_set[j].up != CONNECTED) && 
					(primary_wan_ip_set[j].flags & IPSET_FG_NATCONFIG))
					NAT_delrule(cmdstr);
			}
#ifdef VIRTUAL_SERVER_ALG

			for (apr = ap_proxies; apr->apr_p; apr++)
				{
					if (strcmp(arglists[9],apr->apr_label))
						continue;
					vapr= malloc(sizeof(struct virtual_aproxy));
					if (vapr == NULL )
						{
						diag_printf("[%s]ERROR: fail to malloc virtual_aproxy \n",__FUNCTION__);
						break;
						}
					memset(vapr,0,sizeof(struct virtual_aproxy));

					strcpy(vapr->apr_label,arglists[9]);
					vapr->apr = apr;
					vapr->apr_p = apr->apr_p;
					vapr->apr_port = val;
					if (virtual_aproxy_head == NULL)
						virtual_aproxy_head = vapr;
					else 
						{
						vapr->next = virtual_aproxy_head;
						virtual_aproxy_head= vapr;
						}
					break;
			}
#endif
			
		}
	}
	
	/* port trigger to ipnat rule */
	for(i=0; i< PTRG_MAX_NUM; i++)
	{
		if(CFG_get(CFG_NAT_PTRG+i+1, line) == -1)
			continue;
		if((str2arglist(line, arglists, ';', 6) == 6) && (atoi(arglists[0]) == 1))
		{
			for (j=0; j<PRIMARY_WANIP_NUM; j++) {
				if (!(primary_wan_ip_set[j].flags & IPSET_FG_NATIF))
					continue;
				sprintf(cmdstr, "rdr %s %s/32 port multirange %s trigger %s port %s %s\n",
					primary_wan_ip_set[j].ifname, wanipstr[j], 
					arglists[2], arglists[3], arglists[4], arglists[5]);
				if ((primary_wan_ip_set[j].up == CONNECTED) && 
					!(primary_wan_ip_set[j].flags & IPSET_FG_NATCONFIG))
					NAT_setrule(cmdstr);
				else if ((primary_wan_ip_set[j].up != CONNECTED) && 
					(primary_wan_ip_set[j].flags & IPSET_FG_NATCONFIG))
					NAT_delrule(cmdstr);
			}
		}
	}

	/* ALG to ipnat rule */
	if((CFG_get(CFG_NAT_ALG_FTP, &val) != -1))
	{
		aproxy_t *apr;
		
		apr = appr_find(IPPROTO_TCP, 21);		
		if(apr)	
		apr->apr_enable = val;
	}
	if((CFG_get(CFG_NAT_ALG_NM, &val) != -1))
	{
		aproxy_t *apr;
		
		apr = appr_find(IPPROTO_TCP, 1720);		
		if(apr)	
		apr->apr_enable = val;
	}
	if((CFG_get(CFG_NAT_ALG_MSN, &val) != -1))
	{
		aproxy_t *apr;
		
		apr = appr_find(IPPROTO_TCP, 1863);		
		if(apr)	
		apr->apr_enable = val;
	}
	if((CFG_get(CFG_NAT_ALG_IPSEC, &val) != -1))
	{
		aproxy_t *apr;
		
		apr = appr_find(IPPROTO_UDP, 500);		
		if(apr)	
		apr->apr_enable = val;
	}
	if((CFG_get(CFG_NAT_ALG_PPTP, &val) != -1))
	{
		aproxy_t *apr;
		
		apr = appr_find(IPPROTO_TCP, 1723);		
		if(apr)
		apr->apr_enable = val;
	}
	if((CFG_get(CFG_NAT_ALG_FTP_NSP, &val) != -1))
	{
		aproxy_t *apr;
		
		apr = appr_find(IPPROTO_TCP, 21);		
		if(apr)		
		apr->apr_oport = val;
	}
	else
	{
		// musterc+, 2006.04.03: reset apr_oport if CFG_NAT_ALG_FTP_NSP is empty
		aproxy_t *apr;
		
		apr = appr_find(IPPROTO_TCP, 21);		
		if(apr)		
		apr->apr_oport = 0;
	}
	if((CFG_get(CFG_NAT_ALG_BATTLEN, &val) != -1))
	{
		aproxy_t *apr;
		
		apr = appr_find(IPPROTO_UDP, 6112);		
		if(apr)
		apr->apr_enable = val;
	}
	if((CFG_get(CFG_NAT_ALG_L2TP, &val) != -1))
	{
		aproxy_t *apr;
		
		apr = appr_find(IPPROTO_UDP, 1701);		
		if(apr)	
		apr->apr_enable = val;
	}
	if((CFG_get(CFG_NAT_ALG_RTSP, &val) != -1))
	{
		aproxy_t *apr;
		
		apr = appr_find(IPPROTO_TCP, 554);		
		if(apr)	
		apr->apr_enable = val;
	}
	if((CFG_get(CFG_NAT_ALG_RTSP_NSP, &val) != -1))
	{
		aproxy_t *apr;
		
		apr = appr_find(IPPROTO_TCP, 554);		
		if(apr)		
			apr->apr_oport = val;
	}
	if((CFG_get(CFG_NAT_ALG_SIP, &val) != -1))
	{
		aproxy_t *apr;
		
		apr = appr_find(IPPROTO_UDP, 5060);		
		if(apr)	
		apr->apr_enable = val;
	}
	else
	{
		aproxy_t *apr;
		
		apr = appr_find(IPPROTO_TCP, 554);		
		if(apr)		
			apr->apr_oport = 0;
	}

	/* DMZ to rule */
	if(CFG_get(CFG_NAT_DMZ_EN, &val) != -1)
	{
		memset(line, 0, sizeof(line));
		if(val == 1)
		{
			for(i=0; i< DMZ_MAX_NUM; i++)
			{
				int session;
			
				if(CFG_get(CFG_NAT_DMZ_HOST+i+1, line) == -1)
					continue;
				if((str2arglist(line, arglists, ';', 3) == 3) && (atoi(arglists[0]) == 1))
				{
					inet_aton(arglists[2], &addr);
					addr.s_addr = (SYS_lan_ip & SYS_lan_mask)|(addr.s_addr & ~SYS_lan_mask);
					strcpy(ipstr, NSTR(addr));		
				
					if((session=atoi(arglists[1])) == 0)
					{
						for (j=0; j<PRIMARY_WANIP_NUM; j++) {
							if (!(primary_wan_ip_set[j].flags & IPSET_FG_NATIF))
								continue;
							
							/*  Only translate all ip traffics from the 1st wan to DMZ .  */
							sprintf(cmdstr, "rdr all %s/32 port 0 -> %s port 0 %s dmz\n", 
								wanipstr[j], ipstr, ((j==0) ? "ip" : "tcp/udp"));
						
							if ((primary_wan_ip_set[j].up == CONNECTED) && 
								!(primary_wan_ip_set[j].flags & IPSET_FG_NATCONFIG))
							NAT_setrule(cmdstr);
							else if ((primary_wan_ip_set[j].up != CONNECTED) && 
								(primary_wan_ip_set[j].flags & IPSET_FG_NATCONFIG))
							NAT_delrule(cmdstr);
						}
					}
					else
					{ //one to one NAT
						if(session < WAN_IPALIAS_NUM && session >= 1)
						if(alias_wan_ip_set[session-1].up == CONNECTED)
						{
						sprintf(cmdstr, "bimap all %s/32 -> %s/32 \n", ipstr, NSTR(alias_wan_ip_set[session-1].ip));
						NAT_setrule(cmdstr);
						}
					}
				}
			}
		}
#ifdef CONFIG_SUPERDMZ
		else if(val == 2)
		{
			if(CFG_get(CFG_NAT_SDMZ_MAC, line) != -1)
			{
				for(i=0; i<6; i++)
				{
					val |= line[i];
				}
				
				if(val != 0)
				{	
					char mssclampstr[20];
						
					superdmz_mac = malloc(6);
					if(superdmz_mac)
					{
						memcpy(superdmz_mac, line, 6);
						DHCPD_superdmz_set(superdmz_mac, 1);
					}
					if(SYS_wan_mtu)
						sprintf(mssclampstr, "mssclamp %d", SYS_wan_mtu-40);
					else
						mssclampstr[0] = '\0';
					
					for (j=0; j<PRIMARY_WANIP_NUM; j++) {
						if (!(primary_wan_ip_set[j].flags & IPSET_FG_NATIF))
							continue;
						sprintf(cmdstr, "bimap all fromif %s %s/32 -> %s/32 dmz %s\n", SYS_lan_if, wanipstr[j], wanipstr[j], mssclampstr);
						if ((primary_wan_ip_set[j].up == CONNECTED) && 
							!(primary_wan_ip_set[j].flags & IPSET_FG_NATCONFIG))
						NAT_setrule(cmdstr);
						else if ((primary_wan_ip_set[j].up != CONNECTED) && 
							(primary_wan_ip_set[j].flags & IPSET_FG_NATCONFIG))
						NAT_delrule(cmdstr);
					}
				}
			}
		}
#endif
	}
	
}

//------------------------------------------------------------------------------
// FUNCTION
//		static void napt_init(void)
//
// DESCRIPTION
//		do basic NAPT setup
//  
// PARAMETERS
//		NONE
//  
// RETURN
//		NONE
//  
//------------------------------------------------------------------------------
static void napt_init(void)
{
	char naptcmd[256];
	char mssclampstr[20];
	int j;

	if(SYS_wan_mtu)
		sprintf(mssclampstr, "mssclamp %d", SYS_wan_mtu-40);
	else
		mssclampstr[0] = '\0';
	
	for (j=0; j<PRIMARY_WANIP_NUM; j++) {
		if (primary_wan_ip_set[j].up != CONNECTED)
			continue;

		if (!(primary_wan_ip_set[j].flags & IPSET_FG_NATIF) || 
			(primary_wan_ip_set[j].flags & IPSET_FG_NATCONFIG))
			continue;

		if (primary_wan_ip_set[j].flags & IPSET_FG_DEFROUTE) {
#ifndef CONFIG_NAPT_SEQUENTIAL_PORT
			sprintf(naptcmd, "map %s fromif %s 0/0 -> 0/32 portmap tcp/udp auto %s default\n", 
				primary_wan_ip_set[j].ifname, SYS_lan_if, mssclampstr);
#else
			sprintf(naptcmd, "map %s fromif %s 0/0 -> 0/32 portmap tcp/udp 4096:65535 %s default\n", 
				primary_wan_ip_set[j].ifname, SYS_lan_if, mssclampstr);
#endif
		}
		else {
#ifndef CONFIG_NAPT_SEQUENTIAL_PORT
			sprintf(naptcmd, "map %s fromif %s 0/0 -> 0/32 portmap tcp/udp auto\n", 
				primary_wan_ip_set[j].ifname, SYS_lan_if);
#else
			sprintf(naptcmd, "map %s fromif %s 0/0 -> 0/32 portmap tcp/udp 4096:65535\n", 
				primary_wan_ip_set[j].ifname, SYS_lan_if);
#endif
		}
		NAT_setrule(naptcmd);
	
		sprintf(naptcmd, "map %s fromif %s 0/0 -> 0/32 icmpidmap icmp 45000:65535\n", 
			primary_wan_ip_set[j].ifname, SYS_lan_if);
		NAT_setrule(naptcmd);
		
		/*  Mark rule set  */
		primary_wan_ip_set[j].flags |= IPSET_FG_NATCONFIG;

	}
	
	nat_enabled = 1;
}


//------------------------------------------------------------------------------
// FUNCTION
//		static void napt_clean(void)
//
// DESCRIPTION
//		Delete basic NAPT rules
//  
// PARAMETERS
//		NONE
//  
// RETURN
//		NONE
//  
//------------------------------------------------------------------------------
static void napt_clean(void)
{
	char naptcmd[256];
	char mssclampstr[20];
	int j;
	int natif=0;

	if(SYS_wan_mtu)
		sprintf(mssclampstr, "mssclamp %d", SYS_wan_mtu-40);
	else
		mssclampstr[0] = '\0';
	
	for (j=0; j<PRIMARY_WANIP_NUM; j++) {
		
		if (!(primary_wan_ip_set[j].flags & IPSET_FG_NATCONFIG))
			continue;

		natif ++;
		
		if (primary_wan_ip_set[j].up == CONNECTED)
			continue;

		/* set UDP NAT age was 90 secs */
		if (primary_wan_ip_set[j].flags & IPSET_FG_DEFROUTE) {
#ifndef CONFIG_NAPT_SEQUENTIAL_PORT
			sprintf(naptcmd, "map %s fromif %s 0/0 -> 0/32 portmap tcp/udp auto %s default\n", 
				primary_wan_ip_set[j].ifname, SYS_lan_if, mssclampstr);
#else
			sprintf(naptcmd, "map %s fromif %s 0/0 -> 0/32 portmap tcp/udp 4096:65535 %s default\n", 
				primary_wan_ip_set[j].ifname, SYS_lan_if, mssclampstr);
#endif
		}
		else {
#ifndef CONFIG_NAPT_SEQUENTIAL_PORT
			sprintf(naptcmd, "map %s fromif %s 0/0 -> 0/32 portmap tcp/udp auto\n", 
				primary_wan_ip_set[j].ifname, SYS_lan_if);
#else
			sprintf(naptcmd, "map %s fromif %s 0/0 -> 0/32 portmap tcp/udp 4096:65535\n", 
				primary_wan_ip_set[j].ifname, SYS_lan_if);
#endif
		}
		NAT_delrule(naptcmd);
	
		//sprintf(naptcmd, "map %s %s/%d -> 0/32 icmpidmap icmp 45000:65535\n", 
		//	SYS_wan_if, NSTR(SYS_lan_ip), countbits(SYS_lan_mask));
		sprintf(naptcmd, "map %s fromif %s 0/0 -> 0/32 icmpidmap icmp 45000:65535\n", 
			primary_wan_ip_set[j].ifname, SYS_lan_if);
		NAT_delrule(naptcmd);
		
		/*  Mark rule set  */
		primary_wan_ip_set[j].flags &= ~IPSET_FG_NATCONFIG;
		natif --;
	}
	nat_enabled = (natif > 0) ? 1 : 0;
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
void NAT_flush(int cmd)
{
	int  i;
	int natif;
	struct ifnet *ifp;

	if (cmd == 0)  {
nat_flushall:
// diag_printf("natflush\n");
		/* flush all rule */
		flushtable(O_RDWR, OPT_CLEAR|OPT_FLUSH);
#ifdef CONFIG_SUPERDMZ
		if(superdmz_mac)
		{
			DHCPD_superdmz_set(superdmz_mac, 0);
			free(superdmz_mac);
			superdmz_mac = 0;
		}
#endif

		nat_enabled = 0;
		
		/*  Clear firewall config flag  */
		for (i=0; i<PRIMARY_WANIP_NUM; i++)
			primary_wan_ip_set[i].flags &= ~IPSET_FG_NATCONFIG;
	}
	else {

		for (natif=0, i=0; i<PRIMARY_WANIP_NUM; i++) {
			if ((primary_wan_ip_set[i].up == CONNECTED) && 
				(primary_wan_ip_set[i].flags & IPSET_FG_NATCONFIG))
				natif ++;
		}
		if (natif == 0)
			goto nat_flushall;

		/*  Mark all NAT sessions associated with disconnected interface deleted  */
		for (natif=0, i=0; i<PRIMARY_WANIP_NUM; i++) {
			if ((primary_wan_ip_set[i].up != CONNECTED) && 
					(primary_wan_ip_set[i].flags & IPSET_FG_NATCONFIG)) {
				if ((ifp = ifunit(primary_wan_ip_set[i].ifname)) != NULL) {
//					diag_printf("nat: flush sessions on %s\n", primary_wan_ip_set[i].ifname);
					nat_sync_ifdown(ifp);
				}
				else {
					diag_printf("natsync: unable to resolve ifp for %s\n", primary_wan_ip_set[i].ifname);
				}
			}
		}

		NAT_cmd2rule();
#ifdef CONFIG_SUPERDMZ
		if(superdmz_mac)
		{
			DHCPD_superdmz_set(superdmz_mac, 0);
			free(superdmz_mac);
			superdmz_mac = 0;
		}
#endif
	
#if (MODULE_UPNP == 1)
		upnp_portmap_clean();
#endif
		napt_clean();
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
void NAT_init(void)
{
	int s;
	int val;
	
	if((CFG_get(CFG_NAT_EN, &val) != -1) && (val == 0))
		return;
	
	if((strcmp(WAN_NAME, "") == 0)||(strcmp(LAN_NAME, "") == 0))
		return;
	
	if (!SYS_lan_up)
		return;
			
	/* reconstruct all rule from CFG */
	NAT_cmd2rule();
#if (MODULE_UPNP == 1)
	upnp_portmap_into_nat();
#endif
	napt_init();

	// Reschedule time update
	s = splimp();
	fr_checkacttime();
	splx(s);
}

