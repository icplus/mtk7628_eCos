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
#include <stdio.h>
#include <string.h>
#include <network.h>
#include <sys/param.h>

#include <netinet/in.h>
#include <cfg_def.h>
#include <cfg_net.h>
#include <sys_status.h>
#include <macf.h>
#include <urlf.h>
#include <dnf.h>

#include <ip_compat.h>
#include <ip_fil.h>
#include <ip_nat.h>
#include <ipf.h>
#include <ip_defense.h>

//#define FWRULE_DEBUG		1
//==============================================================================
//                                    MACROS
//==============================================================================
#define FLT_MAC_MAX_NUM		50
#define FLT_URL_MAX_NUM		50
#define FLT_CLN_MAX_NUM		50
#define FLT_PKT_MAX_NUM		50
#define FLT_DNS_MAX_NUM		50

#ifdef FWRULE_DEBUG
#define DBGPRINTF(fmt, ...)		diag_printf(fmt, ## __VA_ARGS__)
#else
#define DBGPRINTF(fmt, ...)
#endif

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
unsigned int ip_defense=0;
struct rate_limit sync_rate;
int	use_inet6=0; 
int fw_default_outpolicy = 0;

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
extern int udp_blackhole;
extern int tcp_blackhole;
extern int drop_portscan;
extern int auth_port_open;
extern int block_pingerr;
extern void *block_ifp[];
extern int nat_enabled;
extern int NAT_getfwsched(char *, int *);
extern int str2arglist(char *, char **, char, int);
extern void nat_fwsync_down(void);

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
void FW_setrule(char *cmdbuf)
{
	struct	frentry	*fr;

	DBGPRINTF("[FW] add rule: %s", cmdbuf);
	fr = parse(cmdbuf, 0);

	if (fr != NULL)
		IPL_EXTERN(ioctl)(IPL_LOGIPF, SIOCINAFR, &fr, O_RDWR, NULL);
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
void FW_delrule(char *cmdbuf)
{
	struct	frentry	*fr;
	
	DBGPRINTF("[FW] del rule: %s", cmdbuf);
	fr = parse(cmdbuf, 0);
	IPL_EXTERN(ioctl)(IPL_LOGIPF, SIOCRMAFR, &fr, O_RDWR, NULL);
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
void FW_ruleflush(void)
{
	int fl = 0;
	
	fl |= (FR_OUTQUE|FR_INQUE);
	if(IPL_EXTERN(ioctl)(IPL_LOGIPF, SIOCIPFFL,(caddr_t)&fl, O_RDWR,NULL) == -1)
			perror("ioctl(SIOCIPFFL)");
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//		MAC filter format:
//			[enable];[description];[MAC address];[sched type];[time/sched name];[days]
//		ex. "1;test1;00:11:22:33:44:55"
//		ex. "1;test2;00:11:22:33:44:55;0"
//		ex. "1;test3;00:11:22:33:44:55;1;7200-48000;127"
//		ex. "1;test1;00:11:22:33:44:55;2;officehour"
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void FW_macf_init(void)
{
	char line[255];
	char *arglists[8], *etime;
	int i, val, argc, macf_sched[3];
	struct macfilter macinfo;
	struct ether_addr * tmp = NULL;
	
	mcfilter_init();
	
	if((CFG_get(CFG_FW_FLT_MAC_EN, &val) != -1)&&(val ==1)) {
		/*  Mac filter is enabled  */

		if(CFG_get(CFG_FW_FLT_MAC_MODE, &val) != -1) {
			if(val)
				default_action = MACF_BLOCK;
			else
				default_action = MACF_PASS;
		}
		
		/*  Parse all MAC filter rules  */
		for(i=0; i< FLT_MAC_MAX_NUM; i++) {
			if(CFG_get(CFG_FW_FLT_MAC+i+1, line) == -1)
				continue;
			argc = str2arglist(line, arglists, ';', 8);
			if ((argc < 3) || (atoi(arglists[0]) != 1))
				continue;

			memset(&macinfo, 0, sizeof(macinfo));

			/*  read in mac address  */
			tmp = ether_aton(arglists[2]);
			if (tmp ==NULL)
				{
				diag_printf("%s %d:ether address error\n",__FUNCTION__,__LINE__);
				return ;
				}
			memcpy(macinfo.mac, tmp, 6);
			macinfo.macf_mode = MACF_MODE_ALWAYS;	/*  Assume always on first  */
			if (argc >= 5) {
				switch(atoi(arglists[3])) {
				case 1:
					etime = arglists[4];
					if (etime ==NULL)
						{
						diag_printf("%s %d :etime null\n",__FUNCTION__,__LINE__);
						return ;
						}
					etime = strchr(etime, '-');
					if (etime == NULL)
						{
						diag_printf("%s:%d etime error\n",__FUNCTION__,__LINE__);
						break;
						}
					*etime++ = '\0';
					macinfo.macf_tstart = atoi(arglists[4]);
					macinfo.macf_tend = atoi(etime);
					macinfo.macf_tday = atoi(arglists[5]);
					macinfo.macf_mode = MACF_MODE_PERIOD;
					DBGPRINTF("[MACF] add rule: %s; %d-%d; %d\n", 
						arglists[2], macinfo.macf_tstart, 
						macinfo.macf_tend, macinfo.macf_tday);
					break;
				case 2:
					if (NAT_getfwsched(arglists[4], macf_sched) > 0) {
						macinfo.macf_tstart = macf_sched[0];
						macinfo.macf_tend = macf_sched[1];
						macinfo.macf_tday = macf_sched[2];	
						macinfo.macf_mode = MACF_MODE_PERIOD;
						DBGPRINTF("[MACF] add rule: %s; %s=%d-%d,%d\n", 
							arglists[2], arglists[4], macinfo.macf_tstart, 
							macinfo.macf_tend, macinfo.macf_tday);
					} else {
						/*  Assume to be always  */
						DBGPRINTF("[MACF] add rule: %s; %s\n", 
							arglists[2], arglists[4]);
					}
					break;
				default:
					DBGPRINTF("[MACF] add rule: %s\n", arglists[2]);
					break;
				}
			}
			macinfo.next = NULL;
			macfilter_add(&macinfo);
		}
		
		mcfilter_act();
	}
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//		URL filter format:
//			[enable];[client IP range];[URL string];[sched type];[time/sched name];[days]
//		ex. "1;192.168.2.50-192.168.2.100;urlString0"
//		ex. "1;192.168.2.100-192.168.2.200;urlString1;0"
//		ex. "1;192.168.2.100-192.168.2.200;urlString2;1;7200-48000;127
//		ex. "1;192.168.2.100-192.168.2.200;urlString3;2;officehour"
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void FW_urlf_init(void)
{
	char line[255];
	char *arglists[8], *etime;
	int i, val, argc, urlf_sched[3];
	struct urlfilter urlinfo;
	char *eip;
	
	if((CFG_get(CFG_FW_FLT_URL_EN, &val) != -1)&&(val >0))	{
		/*  URL filter is enabled  */

		urlfilter_setmode(val);
		
		/*  Parse all URL filter rules  */
		for(i=0; i< FLT_URL_MAX_NUM; i++) {
			if(CFG_get(CFG_FW_FLT_URL+i+1, line) == -1)
				continue;

			argc = str2arglist(line, arglists, ';', 8);
			if ((argc < 3) || (atoi(arglists[0]) != 1))
				continue;
			memset(&urlinfo, 0, sizeof(urlinfo));

			/*  Read in IP range  */
			if(atoi(arglists[1]) == 0) {
				urlinfo.sip = 0;
				urlinfo.eip = 0xffffffff;
			}
			else {
				eip = arglists[1];
				while(*eip) {
					if(*eip == '-') {
						*eip++ = 0;
						break;
					}
					eip++;
				}
				urlinfo.sip = ntohl(inet_addr(arglists[1]));
				urlinfo.eip = ntohl(inet_addr(eip));
			}
			
			/*  Read URL  */
			urlinfo.url = arglists[2];

			/*  Assume the URL filter to be always active  */
			urlinfo.urlf_mode = URLF_MODE_ALWAYS;

			if (argc >= 5) {
				switch(atoi(arglists[3])) {
				case 1:
					etime = arglists[4];
					etime = strchr(etime, '-');
					if(etime == NULL)
						{
						diag_printf("[%s:%d]etime == NULL,return here\n",__FUNCTION__,__LINE__);
						return;
						}
					*etime++ = '\0';
					urlinfo.urlf_tstart = atoi(arglists[4]);
					urlinfo.urlf_tend = atoi(etime);
					urlinfo.urlf_tday = atoi(arglists[5]);
					urlinfo.urlf_mode = URLF_MODE_PERIOD;
					DBGPRINTF("[URLF] add rule: %s; %d-%d; %d\n", 
						arglists[2], urlinfo.macf_tstart, 
						urlinfo.macf_tend, urlinfo.macf_tday);
					break;
				case 2:
					if (NAT_getfwsched(arglists[4], urlf_sched) > 0) {
						urlinfo.urlf_tstart = urlf_sched[0];
						urlinfo.urlf_tend = urlf_sched[1];
						urlinfo.urlf_tday = urlf_sched[2];	
						urlinfo.urlf_mode = URLF_MODE_PERIOD;
						DBGPRINTF("[URLF] add rule: %s; %s=%d-%d,%d\n", 
							arglists[2], arglists[4], urlinfo.urlf_tstart, 
							urlinfo.urlf_tend, urlinfo.urlf_tday);
					} else {
						/*  Assume to be always  */
						DBGPRINTF("[URLF] add rule: %s; %s\n", 
							arglists[2], arglists[4]);
					}
					break;
				default:
					DBGPRINTF("[URLF] add rule: %s\n", arglists[2]);
					break;
				}
			}
			urlinfo.next = NULL;
			urlfilter_add(&urlinfo);
		}
		urlfilter_act();
	}
}


//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//		Domain name filter format:
//			[enable];[client IP range];[domain name]
//		ex. "1;192.168.2.50-192.168.2.100;www.yahoo.com"
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void FW_dnf_init(void)
{
	char line[255];
	char *arglists[5];
	int i, mode=0;
	struct dnfilter dninfo;
	char *eip;
	
	if((CFG_get(CFG_FW_FLT_DN_MODE, &mode) != -1)&&(mode > 0))
	{
		if(mode==1)
			dn_default_action = DN_PASS;
		else
			dn_default_action = DN_DENY;
		for(i=0; i< FLT_DNS_MAX_NUM; i++)
		{
			if(mode == 1)
			{
				if(CFG_get(CFG_FW_FLT_DNB+i+1, line) == -1)
					continue;
			}
			else if(mode == 2)
			{
				if(CFG_get(CFG_FW_FLT_DNA+i+1, line) == -1)
					continue;
			}
					
			if((str2arglist(line, arglists, ';', 3) == 3) && (atoi(arglists[0])==1))
			{
				if(atoi(arglists[1]) == 0)
				{
					dninfo.sip = 0;
					dninfo.eip = 0xffffffff;	
				}
				else
				{
					eip = arglists[1];
					while(*eip)
					{
						if(*eip == '-') {
							*eip++ = 0;
							break;
						}
						eip++;
					}
				
					dninfo.sip = ntohl(inet_addr(arglists[1]));
					dninfo.eip = ntohl(inet_addr(eip));	
				}
				dninfo.query = arglists[2];
				dninfo.next = NULL;
				dnfilter_add(&dninfo);
			}
		}
		sprintf(line, "call dnfilter in quick on %s proto udp from any to any port = 53\n", SYS_lan_if);
		FW_setrule(line);
	}
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//		client filter format:
//			[enable];[description];[source IP range];[destination port range];[protocol];[schedule enable];[schedule time range];[schedule day]
//		ex. "1;test1;192.168.2.50-192.168.2.60;5000-6000;tcp/udp;0;7200-86400;1"
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void FW_client_init(void)
{
	char line[255], filcmd[255], subcmd[50];
	char *arglists[8];
	int i, val, fw_sched[3];
	char sip[16], eip[16], dp[30], *eeip, *eport, *etime;;
	volatile struct	in_addr	ip;	
	int j;
	
	if((CFG_get(CFG_FW_FLT_CLN_EN, &val) != -1)&&(val ==1))
	{
		for(i=0; i< FLT_CLN_MAX_NUM; i++)
		{
			if(CFG_get(CFG_FW_FLT_CLN+i+1, line) == -1)
				continue;
			if((str2arglist(line, arglists, ';', 8) == 8) && (atoi(arglists[0]) == 1))
			{
				eeip = arglists[2];
				
				eeip = strchr(eeip, '-');
					
				// Muster 2006/03/31
				if (eeip)
					*eeip++ = '\0';
				else
					eeip = arglists[2];
				
				inet_aton(arglists[2], &ip);
				ip.s_addr = htonl(htonl(ip.s_addr));
				strcpy(sip, inet_ntoa(ip));
				
				inet_aton(eeip, &ip);
				ip.s_addr = htonl(htonl(ip.s_addr));
				strcpy(eip, inet_ntoa(ip));
				
				if(*(arglists[3]) == '\0')
					*dp = '\0';
				else if(!(eport = strchr(arglists[3], '-')))
					sprintf(dp, "port = %s", arglists[3]);
				else
				{
					*eport++='\0';
					sprintf(dp, "port %s >< %s", arglists[3], eport);
				}

				subcmd[0] = '\0';	/*  Assume always on  */			
				switch (atoi(arglists[5]))
				{

				case 1:
					etime = arglists[6];
					etime = strchr(etime, '-');
					if(etime == NULL)
						{
						diag_printf("[%s:%d]etime == NULL,return here\n",__FUNCTION__,__LINE__);
						return;
						}
					*etime++ = '\0';
					sprintf(subcmd, "act-time period %s %s %s", arglists[7], arglists[6], etime);
					break;
					
				case 2:
					if (NAT_getfwsched(arglists[6], fw_sched) > 0)
						sprintf(subcmd, "act-time period %d %d %d", fw_sched[2], fw_sched[0], fw_sched[1]);
					break;
				default:
					/*  Always on  */
					break;
				}
				
				for (j=0; j<PRIMARY_WANIP_NUM; j++) {
					if ((primary_wan_ip_set[j].up != CONNECTED) || 
						!(primary_wan_ip_set[j].flags & IPSET_FG_FWIF) || 
						(primary_wan_ip_set[j].flags & IPSET_FG_FWCONFIG))
						continue;
					
					sprintf(filcmd, "block out quick on %s proto %s from %s><%s to any %s %s\n", 
						primary_wan_ip_set[j].ifname,  arglists[4], sip, eip, dp, subcmd);
					//diag_printf("filcmd:[%s]\n", filcmd);
					FW_setrule(filcmd);
				}
			}
		}
	}
}


//------------------------------------------------------------------------------
// FUNCTION
//		packet filter initialization
//
// DESCRIPTION
//		packet filter format:
//			[enable];[description];[policy];[source interface];[source IP range];[source port range];[destination interface];[destination IP range];[destination port range];[protocol];[schedule type];[schedule time range/shcedule name];[schedule day]
//			policy: 0 -> block, 1 -> pass
//			direction: 0 -> inbound, 1 -> outbound
//		ex. "1;test1;1;0;222.23.5.44;;192.168.0.100;any;all;0;7200-86400;1"
//		ex. "1;test2;1;0;222.23.5.44;;192.168.0.100;any;all;1;7200-86400;1"
//		ex. "1;test3;1;0;222.23.5.44;;192.168.0.100;any;all;2;officehour"
//  
// PARAMETERS
//		NONE
//  
// RETURN
//		NONE
//  
//------------------------------------------------------------------------------
void FW_packetf_init(void)
{
	char line[255], filcmd[255];
	char *arglists[15];
	int i, fw_sched[3];
	char *policy, *ssip, *esip, *sdip, *edip, *ssp, *esp, *sdp, *edp, *etime;
	char sip[30], dip[30], sp[30], dp[30], sched[50], prot[20];
	int j;
	
	for(i=0; i< FLT_PKT_MAX_NUM; i++)
	{
		if(CFG_get(CFG_FW_FLT_PKT+i+1, line) == -1)
			continue;
		//diag_printf("line:[%s]\n", line);
		if((str2arglist(line, arglists, ';', 13) == 13) && (atoi(arglists[0]) == 1))
		{
			// parse policy
			if(atoi(arglists[2]) == 1)
				policy = "pass";
			else
				policy = "block";
			
			
			// parse source IP
			ssip = arglists[4]; 
			if(*ssip == '\0')
				sprintf(sip, "any");
			else if(!(esip = strchr(ssip, '-')))
				sprintf(sip, "%s", ssip);
			else
			{
				*esip++='\0';
				sprintf(sip, "%s><%s", ssip, esip);
			}
			
			// parse destination IP
			sdip = arglists[7]; 
			if(*sdip == '\0')
				sprintf(dip, "any");
			else if(!(edip = strchr(sdip, '-')))
				sprintf(dip, "%s", sdip);
			else
			{
				*edip++='\0';
				sprintf(dip, "%s><%s", sdip, edip);
			}
			
			// parse source port
			ssp = arglists[5]; 
			if((*ssp == '\0') || (*(arglists[9])=='\0') || (!strcmp(arglists[9], "icmp")))
				*sp = '\0';
			else if(!(esp = strchr(ssp, '-')))
				sprintf(sp, "port = %s", ssp);
			else
			{
				*esp++='\0';
				sprintf(sp, "port %s >< %s", ssp, esp);
			}
			
			// parse destination port
			sdp = arglists[8]; 
			if((*sdp == '\0') || (*(arglists[9])=='\0') || (!strcmp(arglists[9], "icmp")))
				*dp = '\0';
			else if(!(edp = strchr(sdp, '-')))
				sprintf(dp, "port = %s", sdp);
			else
			{
				*edp++='\0';
				sprintf(dp, "port %s >< %s", sdp, edp);
			}
			
			sched[0] = '\0';		/*  Assume always on  */
			switch (atoi(arglists[10]))
			{
			case 1:
				etime = arglists[11];
				etime = strchr(etime, '-');
				if(etime == NULL)
				{
					diag_printf("[%s:%d]etime == NULL,return here\n",__FUNCTION__,__LINE__);
					return;
				}
				*etime++ = '\0';
				sprintf(sched, "act-time period %s %s %s", arglists[12], arglists[11], etime);
				break;
				
			case 2:
				if (NAT_getfwsched(arglists[11], fw_sched) > 0)
					sprintf(sched, "act-time period %d %d %d", fw_sched[2], fw_sched[0], fw_sched[1]);
				break;
				
			default:
				/*  Do nothing  */
				break;
			}
			
			*prot = '\0';
			if(*(arglists[9])!='\0')
			{
				/* To solve the Firewall problem of selecting [Deny All], but ICMP still can pass through.*/
				if((strcmp(policy, "block")!=0)||(strcmp(arglists[9], "tcp/udp")!=0))
					sprintf(prot,"proto %s", arglists[9]);
			}
			
			if((atoi(arglists[3]) != 2)&&((atoi(arglists[6]) == 2)||(atoi(arglists[6]) == 0)))
			{
				for (j=0; j<PRIMARY_WANIP_NUM; j++) {
					if ((primary_wan_ip_set[j].up != CONNECTED) || 
						!(primary_wan_ip_set[j].flags & IPSET_FG_FWIF) || 
						(primary_wan_ip_set[j].flags & IPSET_FG_FWCONFIG))
						continue;
	
					sprintf(filcmd, "%s out quick on %s in-via %s %s from %s %s to %s %s %s\n", 
						policy, primary_wan_ip_set[j].ifname, SYS_lan_if, prot, sip, sp, dip, dp, sched);
					//diag_printf("filcmd1:[%s]\n", filcmd);
					FW_setrule(filcmd);
#ifdef CONFIG_FW_AUTO_DEFAULT_POLICY
					if(atoi(arglists[2]) == 1)
						fw_default_outpolicy = 1; /* deny */
#endif
				}
			}
			
			if((atoi(arglists[3]) != 1)&&((atoi(arglists[6]) == 1)||(atoi(arglists[6]) == 0)))
			{
				for (j=0; j<PRIMARY_WANIP_NUM; j++) {
					if ((primary_wan_ip_set[j].up != CONNECTED) || 
						!(primary_wan_ip_set[j].flags & IPSET_FG_FWIF) || 
						(primary_wan_ip_set[j].flags & IPSET_FG_FWCONFIG))
						continue;

					sprintf(filcmd, "%s in quick on %s %s from %s %s to %s %s %s\n", 
						policy, primary_wan_ip_set[j].ifname, prot, sip, sp, dip, dp, sched);
					diag_printf("filcmd2:[%s]\n", filcmd);
					FW_setrule(filcmd);
				}
			}
		}
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
void FW_basic_rule(void)
{
	char filcmd[255];
	int val;
	char wanipstr[PRIMARY_WANIP_NUM][16], lanipstr[16];;
	int i;
	ip_defense = 0;
	

	for(i=0; i<PRIMARY_WANIP_NUM; i++)
		strcpy(wanipstr[i], NSTR(primary_wan_ip_set[i].ip));

	strcpy(lanipstr, NSTR(SYS_lan_ip));
	
	// unallow ping gateway from LAN
	if((CFG_get(CFG_FW_PING_DIS_LAN, &val) != -1) && (val ==1))
	{
		sprintf(filcmd, "block in quick on %s proto icmp from any to %s/32 icmp-type 8\n", SYS_lan_if, lanipstr);
		FW_setrule(filcmd);
	}

	if((CFG_get(CFG_FW_ICMP_RESP, &val) != -1) && (val == 0))
		block_pingerr = 1;
	else
		block_pingerr = 0;


	// allow Fragment packets come in
	if((CFG_get(CFG_FW_FRAG_OK, &val) != -1) && (val ==0))
		SET_DEF_IP_FRAG;
	
	// disable response ICMP unreachable message to do port scan hardly
	CFG_get(CFG_FW_PSCAN_DROP, &drop_portscan);
	
	// response port 113
	//CFG_get(CFG_FW_IDENT_DIS, &auth_port_open);

	for (i=0;  i<PRIMARY_WANIP_NUM; i++) {
		if ((primary_wan_ip_set[i].up != CONNECTED) || 
			!(primary_wan_ip_set[i].flags & IPSET_FG_FWIF) || 
			(primary_wan_ip_set[i].flags &IPSET_FG_FWCONFIG))
			continue;

		block_ifp[i] = ifunit(primary_wan_ip_set[i].ifname);
			
		// block all ping from WAN
		if(((CFG_get(CFG_FW_PING_DIS_LAN, &val) != -1)&&(val ==1)) || 
			((CFG_get(CFG_FW_PING_DIS_WAN, &val) != -1)&&(val ==1)))
		{
			sprintf(filcmd, "block in quick on %s proto icmp from any to %s/32 icmp-type 8\n", 
			primary_wan_ip_set[i].ifname, wanipstr[i]);
			FW_setrule(filcmd);
		}
		
		if ((i==0) && (CFG_get(CFG_NAT_ALG_PPTP, &val)!= -1)&&(val == 0))
		{
			sprintf(filcmd, "block out quick on %s proto tcp from any to any port = 1723\n", 
			primary_wan_ip_set[i].ifname);
			FW_setrule(filcmd);
		}
		
		if((CFG_get(CFG_NAT_ALG_IPSEC, &val)!= -1)&&(val == 0))
		{
			sprintf(filcmd, "block out quick on %s proto udp from any to any port = 500\n", primary_wan_ip_set[i].ifname);
			FW_setrule(filcmd);
			sprintf(filcmd, "block in quick on %s proto 50 all\n", SYS_lan_if);
			FW_setrule(filcmd);
		}
		// block all netbios packet to WAN
		if((CFG_get(CFG_FW_NBIOS_DROP, &val) != -1)&&(val == 1))
		{
			/* block port 135 ~ 139, 445 */
			sprintf(filcmd, "block out quick on %s proto tcp/udp from any to any port 135 >< 139\n", primary_wan_ip_set[i].ifname);
			FW_setrule(filcmd);
			sprintf(filcmd, "block out quick on %s proto tcp/udp from any to any port = 445\n", primary_wan_ip_set[i].ifname);
			FW_setrule(filcmd);
		}
		
		//
		
		// defense hacker pattern 
		if((CFG_get(CFG_FW_HACKER_ATT, &val) != -1)&&(val ==1))
		{
			if((CFG_get(CFG_FW_IP_SPOOF, &val) != -1)&&(val ==1))
				SET_DEF_IP_SPOOFING;
			if((CFG_get(CFG_FW_SHORT, &val) != -1)&&(val ==1))
				SET_DEF_SHORT_PACKET;
			if((CFG_get(CFG_FW_PING_DEATH, &val) != -1)&&(val ==1))
				SET_DEF_PING_OF_DEATH;
			if((CFG_get(CFG_FW_LAND, &val) != -1)&&(val ==1))
				SET_DEF_LAND_ATTACK;
			if((CFG_get(CFG_FW_SNORK, &val) != -1)&&(val ==1))
				SET_DEF_SNORK_ATTACK;
			if((CFG_get(CFG_FW_UDP_PLOOP, &val) != -1)&&(val ==1))
				SET_DEF_UDP_PORT_LOOP;
			if((CFG_get(CFG_FW_TCP_NULL, &val) != -1)&&(val ==1))
				SET_DEF_TCP_NULL_SCAN;
			if((CFG_get(CFG_FW_SMURF, &val) != -1)&&(val ==1))
				SET_DEF_SMURF_ATTACK;
			if((CFG_get(CFG_FW_SYN_FLOOD, &val) != -1)&&(val ==1))
				SET_DEF_SYN_FLOODING;
			
			/* only in NAT mode, IP Spoofing defense take effect */ 
			if(nat_enabled && DEF_IP_SPOOFING)
			{
				sprintf(filcmd, "block in seclog 1 quick on %s from %s/24 to any\n", primary_wan_ip_set[i].ifname, lanipstr);
				FW_setrule(filcmd);
			}
			
			if(DEF_SYN_FLOODING)
			{
				if((CFG_get(CFG_FW_SYNF_RATE, &val) != -1))
					sync_rate.maxpps = val;
				else
					sync_rate.maxpps = SYN_FLOOD_DAFAULT_RATE;
				sync_rate.curpps = 0;
				sync_rate.lasttime = 0;
			}
		}
		
		
		if((CFG_get(CFG_FW_QQ_DIS, &val) != -1)&&(val == 1))
		{
			struct dnfilter dninfo;
			
			sprintf(filcmd, "block out quick on %s proto tcp/udp from any to any port = 8000\n", primary_wan_ip_set[i].ifname);
			FW_setrule(filcmd);
			//sprintf(filcmd, "block out quick on %s proto tcp from any to tcpconn.tencent.com\n", primary_wan_ip_set[i].ifname);
			//FW_setrule(filcmd);
			sprintf(filcmd, "block out quick on %s proto tcp from any to 218.18.95.221\n", primary_wan_ip_set[i].ifname);
			FW_setrule(filcmd);
			sprintf(filcmd, "block out quick on %s proto tcp from any to 218.18.95.20\n", primary_wan_ip_set[i].ifname);
			FW_setrule(filcmd);
			sprintf(filcmd, "block out quick on %s proto tcp from any to 218.18.95.183\n", primary_wan_ip_set[i].ifname);
			FW_setrule(filcmd);
			//sprintf(filcmd, "block out quick on %s proto tcp from any to 218.18.95.00/24\n", primary_wan_ip_set[i].ifname);
			//FW_setrule(filcmd);
			//sprintf(filcmd, "block out quick on %s proto tcp from any to 219.133.38.29\n", primary_wan_ip_set[i].ifname);
			//FW_setrule(filcmd);
			//sprintf(filcmd, "block out quick on %s proto tcp from any to tcpconn2.tencent.com\n", primary_wan_ip_set[i].ifname);
			//FW_setrule(filcmd);
			//sprintf(filcmd, "block out quick on %s proto tcp from any to tcpconn3.tencent.com\n", primary_wan_ip_set[i].ifname);
			//FW_setrule(filcmd);
			//sprintf(filcmd, "block out quick on %s proto tcp from any to tcpconn4.tencent.com\n", primary_wan_ip_set[i].ifname);
			//FW_setrule(filcmd);
			//sprintf(filcmd, "block out quick on %s proto tcp from any to http.tencent.com\n", primary_wan_ip_set[i].ifname);
			//FW_setrule(filcmd);
			//sprintf(filcmd, "block out quick on %s proto tcp from any to http2.tencent.com\n", primary_wan_ip_set[i].ifname);
			//FW_setrule(filcmd);
			dninfo.sip = 0;
			dninfo.eip = 0xffffffff;	
			dninfo.query = "tencent.com";
			dninfo.next = NULL;
			dnfilter_add(&dninfo);
			dninfo.query = "qq.com";
			dnfilter_add(&dninfo);
			sprintf(filcmd, "call dnfilter in quick on %s proto udp from any to any port = 53\n", SYS_lan_if);
			FW_setrule(filcmd);
		}
		
		if((CFG_get(CFG_FW_MSN_DIS, &val) != -1)&&(val == 1))
		{
			sprintf(filcmd, "block out quick on %s proto tcp from any to any port = 1863\n", primary_wan_ip_set[i].ifname);
			FW_setrule(filcmd);
			sprintf(filcmd, "block out quick on %s proto tcp from any to messenger.hotmail.com port = 80\n", primary_wan_ip_set[i].ifname);
			FW_setrule(filcmd);
		}
		
		#if 0	
		// URL filter rule
		if((CFG_get(CFG_FW_FLT_URL_EN, &val) != -1)&&(val ==1))
		{
			sprintf(filcmd, "call urlfilter out quick on %s proto tcp from any to any port = 80\n", primary_wan_ip_set[i].ifname);
		}
		#endif
		#if 0	
		/* block all access our httpd from WAN side */
		if((CFG_get(CFG_FW_RM_EN, &val) == -1) ||(val == 0))
		{
			sprintf(filcmd, "block in quick on %s proto tcp from any to %s/32 port = 23\n", primary_wan_ip_set[i].ifname, wanipstr[i]);
			FW_setrule(filcmd);
		}
		sprintf(filcmd, "block in quick on %s proto tcp from any to %s/32 port = 80\n", primary_wan_ip_set[i].ifname, wanipstr[i]);
		FW_setrule(filcmd);
		#endif	
		#if 0	
		// keep state for tcp/udp reply packet
		sprintf(filcmd, "pass out quick on %s proto tcp/udp from any to any keep state\n", primary_wan_ip_set[i].ifname);
		FW_setrule(filcmd);
		// keep state for icmp reply packet
		sprintf(filcmd, "pass out quick on %s proto icmp from any to any keep state\n", primary_wan_ip_set[i].ifname);
		FW_setrule(filcmd);
		
		// block otherwise packets
		sprintf(filcmd, "block in on %s all\n", primary_wan_ip_set[i].ifname);
		FW_setrule(filcmd);
		#endif
		
		// pass all out packets
		if(fw_default_outpolicy)
			sprintf(filcmd, "block out on %s in-via %s all keep state\n", primary_wan_ip_set[i].ifname, SYS_lan_if);
		else
			sprintf(filcmd, "pass out on %s all keep state\n", primary_wan_ip_set[i].ifname);
		FW_setrule(filcmd);
		
		// block otherwise packets
		//sprintf(filcmd, "block in on %s all\n", primary_wan_ip_set[i].ifname);
		//FW_setrule(filcmd);
		
		primary_wan_ip_set[i].flags |= IPSET_FG_FWCONFIG;
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
void FW_flush(void)
{
	int i;

//	diag_printf("flush FW table\n");
	macfilter_flush();
	mcfilter_inact();
	urlfilter_flush();
	urlfilter_inact();
	dnfilter_flush();
	// flush all ipf entry
	//flushfilter("sa");
	nat_fwsync_down();
	FW_ruleflush();

	fw_default_outpolicy = 0;

	
	/*  Clear firewall config flag  */
	for (i=0; i<PRIMARY_WANIP_NUM; i++) {
		block_ifp[i] = NULL;
		primary_wan_ip_set[i].flags &= ~IPSET_FG_FWCONFIG;
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
void FW_init(void)
{
	int s;
	int val;
	
	if((CFG_get(CFG_FW_EN, &val) != -1) && (val == 0))
		return;

#ifdef CONFIG_FW_MACF
	FW_macf_init();
#endif
#ifdef CONFIG_FW_URLF	
	FW_urlf_init();
#endif
	FW_dnf_init();
	FW_client_init();
	FW_packetf_init();
	FW_basic_rule();
	
	// Reschedule time update
	s = splimp();
	fr_checkacttime();
	splx(s);
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
static void printlist(frentry_t *fp)
{
//	struct	frentry	fb;

	for (; fp; fp = fp->fr_next) 
	{
		printf("[%lu hits] ", fp->fr_hits);
		printfr(fp);
		if (fp->fr_grp)
			printlist(fp->fr_grp);
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
int FW_cmd(int argc, char* argv[])
{
	friostat_t fio;
	friostat_t *fiop = &fio;
	struct	frentry	*fp = NULL;
	int set;
	
	if ( argc < 1 )
	{
		printf("fw rule/mac/url/black\n\r");
		return -1;
	}
	
	if ( !strcmp(argv[0], "rule" ))
	{
		if(iplioctl(IPL_LOGIPF, SIOCGETFS, &fiop, O_RDONLY, NULL) == -1)
		{
			return -1;
		}
		
		set = fiop->f_active;
		
		printf("(out):\n");
		fp = (struct frentry *)fiop->f_fout[set];
		printlist(fp);
		
		printf("(in):\n");
		fp = (struct frentry *)fiop->f_fin[set];
		printlist(fp);
	}
#ifdef CONFIG_FW_MACF	
	else if(!strcmp(argv[0], "mac" ))
	{
		if ( argc < 2 )
		{
			printf("fw mac init/flush\n\r");
			return -1;
		}
		if ( !strcmp(argv[1], "init" ))
			FW_macf_init();
		else if( !strcmp(argv[1], "flush" ))
			macfilter_flush();
	}
#endif
#ifdef CONFIG_FW_URLF	
	else if(!strcmp(argv[0], "url" ))
	{
		if ( argc < 2 )
		{
			printf("fw url init/flush\n\r");
			return -1;
		}
		if ( !strcmp(argv[0], "init" ))
			FW_urlf_init();
		else if( !strcmp(argv[0], "flush" ))
			urlfilter_flush();
	}
#endif	
	else if(!strcmp(argv[0], "black" ))
	{
		show_attacker_list();
	}
	
	return 0;
}

