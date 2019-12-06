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
    mon_task.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <config.h>
#include <cyg/kernel/kapi.h>
#include <stdio.h>
#include <cfg_def.h>
#include <network.h>
#include <cfg_net.h>
#include <sys_status.h>



//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
static cyg_mbox mon_tsk_mbox;
static cyg_handle_t mon_tsk_mbox_h;

static cyg_handle_t h_mon_t;
static cyg_thread mon_t;
static char stack[1024*8];

#if defined(CONFIG_WANIF)
char chk_link=0;
#endif

extern int config_ntp_save_time;

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
extern void sys_reboot(void);
extern int ifr_link_check(char * ifn);
extern void RT_staticrt_allset(void);
extern void RT_staticrt_flush(void);

#ifdef	CONFIG_RST
extern void swrst_init(void);
#endif
#ifdef	CONFIG_WATCHDOG
extern void watchdog_init(void);
#endif
#if (MODULE_WANIF == 1)	
extern int WAN_start(int cmd);
extern int Wan_stop_stage2(void);
#endif
#if (MODULE_LANIF == 1)
extern int LAN_start(int cmd);
#endif
#ifdef	CONFIG_DNS
extern void DNS_shutdown(void);
extern void DNS_update(void);
extern void DNS_config(void);
#endif
#ifdef	CONFIG_DHCPS
extern void DHCPD_init(void);
extern void DHCPD_down(void);
#endif
#if (MODULE_FW == 1)
extern void FW_init(void);
extern void FW_flush(void);
#endif
#if (MODULE_NAT == 1)
extern void NAT_init(void);
extern void NAT_flush(int);
#endif
#ifdef	CONFIG_DDNS
extern void DDNS_down(void);
extern void DDNS_update(void);
extern void DDNS_config(void);
extern void DDNS_corrtime(void);
#endif
#if (MODULE_HTTPD == 1)
extern void HTTPD_update(void);
#endif
#if (MODULE_UPNP == 1)	
extern void UPNP_init(void);
#endif
#if (MODULE_NTP ==1)
extern void ntp_update(int flag);
#endif
#if (MODULE_SYSLOG == 1)
extern void eventlog_update(void);
//extern void eventlog_time_correct(void);
#endif

#ifdef	CONFIG_PPTP_PPPOE
extern int pptp_vpn_mon_down(void);
extern int pptp_vpn_mon(void);
extern int net_ping_test(unsigned int, int, int);
extern unsigned int nstr2ip(char *);
#endif

#ifdef CONFIG_L2TP_OVER_PPPOE
extern int l2tp_over_pppoe_flag;
extern int l2tp_over_pppoe_mon(void);
#endif

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
int mon_snd_cmd(int cmd)
{
        if (mon_tsk_mbox_h && cyg_mbox_tryput(mon_tsk_mbox_h, (void *)cmd))
                return 0;
        else
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
static void mon_main(void)
{
        int cmd, cmd2;
        int wancmd;


	mon_snd_cmd(MON_CMD_LINK_CHK);
	
#ifdef	CONFIG_RST
	swrst_init();
#endif
	
#ifdef	CONFIG_WATCHDOG
	watchdog_init();
#endif


    while(1)
    {
	wancmd = 0;
        cmd = (int) cyg_mbox_get(mon_tsk_mbox_h);
//		diag_printf("mcmd:[%x]\n", cmd);
		
		// system command
        switch (cmd & 0xff)
        {
#if		MODULE_AP_CLIENT
		case 13:    // clone mac detect
			{
				int ap_mode;
				
				CFG_get( CFG_WLN_AP_MODE, &ap_mode);
				if (ap_mode == 1 || ap_mode == 2)
				{
					char clmac[6];
					int rc;
					
					rc=net_clone_mac_get(clmac);
					if (rc)
					{
						net_clone_mac_del(LAN_NAME);
						CFG_set(CFG_WLN_CLN_MAC, clmac);
						CFG_commit(0);
						
						cmd = (MON_CMD_UPDATE|MOD_WL);
					}
				}
			}
			break;
#endif

		case MON_CMD_LINK_CHK:
#ifdef CONFIG_WANIF
			{
				int link;
				

				static int link_check_cnt = 0;
				if (link_check_cnt < 8)
				{
					link_check_cnt++;
					timeout((timeout_fun *)mon_snd_cmd, (void *)MON_CMD_LINK_CHK, 50);
					break;
				}
				else
				{
					untimeout((timeout_fun *)mon_snd_cmd, (void *)MON_CMD_LINK_CHK);
					link_check_cnt = 6;
				}

				
				link = ifr_link_check(WAN_NAME);
				if (chk_link == 0 && link)
				{
					cmd = MON_CMD_WAN_INIT; /* 516 */
				}
				else if (chk_link == 1 && link == 0)
				{
					cmd = MON_CMD_WAN_STOP; /* 520 */
				}
				
				chk_link = (char)link;
			}
#endif //CONFIG_WANIF
			break;
				
		case MON_CMD_REBOOT:
			cmd |= MON_CMD_WAN_STOP;
			break;
        }
        
        /******* Modules down *******/
        if ((cmd & 0xff) & (MON_CMD_STOP | MON_CMD_DOWN))
        {
			cmd2 = cmd;
			if (cmd2 & MOD_LAN)
				cmd2 |= MOD_DHCPD|MOD_RIP|MOD_NAT|MOD_FW|MOD_UPNP|MOD_HTTPD|MOD_DNS|MOD_MROUTED;
			
			if (cmd2 & MOD_WAN)
				cmd2 |= MOD_RIP|MOD_NAT|MOD_FW|MOD_UPNP|MOD_DDNS|MOD_DNS|MOD_MROUTED;

#if (MODULE_WANIF == 1)	
			if ((cmd2 & MOD_WAN) && !(cmd2 & MON_CMD_DOWN))
			{
				if (chk_link && !(cmd2 & MON_CMD_CFGCHANGE)) {
					wancmd = 3;
					WAN_start(wancmd);
				}
				else
					WAN_start(0);
			}
#endif

#ifdef	CONFIG_DNS
			if (cmd2 & MOD_DNS)
			{
				DNS_shutdown();
			}
#endif
#if (MODULE_UPNP == 1)
			if (cmd2 & MOD_UPNP)
			{
				// Don't shutdown UPNP
			}
#endif
#if (MODULE_HTTPD == 1)
			if (cmd2 & MOD_HTTPD)
			{
				HTTPD_update();
			}	
#endif
			if ((cmd2 & MOD_SRT) || (cmd2 & MOD_RIP))
			{
				RT_staticrt_flush();
			}

#if (MODULE_DDNS == 1)
			if (cmd2 & MOD_DDNS)
			{
				DDNS_down();
			}
#endif
#if (MODULE_NAT == 1)
			if (cmd2 & MOD_NAT)
			{
				NAT_flush(wancmd);
			}
#endif
#if (MODULE_FW == 1)
			if (cmd2 & MOD_FW)
			{
				FW_flush();
			}
#endif
#if (MODULE_DHCPS == 1)	
			if (cmd2 & MOD_DHCPD)
			{
				DHCPD_down();
			}
#endif
#if (MODULE_LANIF == 1)
			if (cmd2 & MOD_LAN)
			{
				LAN_start(0); 
			}
#endif
#ifdef	CONFIG_DNS
			if (cmd2 & MOD_DNS)
			{
				// Udate DNS after shutdown
				DNS_update();
			}
#endif
#if (MODULE_WANIF == 1)	
			if ((cmd2 & MOD_WAN) && !(cmd2 & MON_CMD_DOWN)) {
				Wan_stop_stage2();
			}
#endif
			//we need time to do shutdown
			cyg_thread_delay(100);
        } // if ((cmd&0xff) & (MON_CMD_STOP|MON_CMD_DOWN))
        
        /******* Modules up *******/
        if ((cmd & 0xff) & MON_CMD_INIT)
        {
        	cmd2 = cmd;
        	if (cmd2 & MOD_SYS)
			{
				// musterc+, 2006.05.03: when domain name changed (CFG_SYS_DOMAIN),
				// DHCPD should be restarted to get the correct domain name.
				cmd2 |= MOD_DHCPD;
				cmd2 |= MOD_HTTPD;
				cmd2 |= MOD_NTP;
				cmd2 |= MOD_LOG;
			}

#if (MODULE_SYSLOG == 1)
			if (cmd2 & MOD_LOG)
			{
				eventlog_update();
			}
#endif
			
#if (MODULE_LANIF == 1)
			if (cmd2 & MOD_LAN)
			{
				if (LAN_start(1) != -1) 
					cmd2 |= MOD_NAT|MOD_FW|MOD_RIP|MOD_DHCPD|MOD_HTTPD|MOD_UPNP|MOD_DNS|MOD_SRT|MOD_NTP|MOD_MROUTED;
				else
					cmd2 &= ~(MOD_NAT|MOD_FW);
			}
#endif
#if (MODULE_WANIF == 1)	
			if (cmd2 & MOD_WAN)
			{
				if (chk_link) {
					switch(WAN_start(1)) {
					case 1:
					default:
						cmd2 |= MOD_NAT|MOD_FW|MOD_RIP|MOD_DDNS|MOD_DNS|MOD_HTTPD|MOD_SRT|MOD_DHCPD|MOD_NTP|MOD_MROUTED;
						#ifdef CONFIG_PPTP_PPPOE
						//for pppoe  + pptp case
						int ipmode,pptp_wanif;
				
						CFG_get(CFG_WAN_IP_MODE, &ipmode);
						CFG_get( CFG_PTP_WANIF, &pptp_wanif );
						//pppoe  + pptp case
						if(ipmode==4 && pptp_wanif==2)//pptp vpn over pppoe special mode.
							cmd2 |= MOD_VPN;
						#endif
						#ifdef CONFIG_L2TP_OVER_PPPOE
						  //pppoe finish, then l2tp follow.
						  if(l2tp_over_pppoe_flag){
						  	l2tp_over_pppoe_mon();	
						  	}
						#endif
						break;
					case 0:
						cmd2 |= MOD_NAT|MOD_FW|MOD_DNS|MOD_HTTPD|MOD_SRT|MOD_MROUTED;
						break;
					case -1:
						cmd2 &= ~(MOD_NAT|MOD_FW);
						break;
					}
				}
				else {
					cmd2 &= ~(MOD_NAT|MOD_FW);
				}
			}
#endif
#ifdef	CONFIG_PPTP_PPPOE
			if ((cmd2 & MOD_VPN)&&!(cmd2 & MOD_WAN))//means recieve MON_CMD_VPN_INIT
			{
				pptp_vpn_mon();
				cmd2 |= MOD_DNS;
			}

#endif
#if (MODULE_DNS == 1)
			if (cmd2 & MOD_DNS)
			{
				DNS_config();
				DNS_shutdown();
				DNS_update();	
				cmd2 |= MOD_DHCPD;
			}
#endif
#if (MODULE_DHCPS == 1)	
			if (cmd2 & MOD_DHCPD)
			{
				DHCPD_init();
			}
#endif
			if ((cmd2 & MOD_SRT) || (cmd2 & MOD_RIP))
			{
				// set static route
				RT_staticrt_allset();
			}
#if (MODULE_FW == 1)
			if (cmd2 & MOD_FW)
			{
				FW_init();
			}
#endif
#if (MODULE_NAT == 1)
			if (cmd2 & MOD_NAT)
			{
				NAT_init();
			}
#endif
#if (MODULE_DDNS == 1)
			if (cmd2 & MOD_DDNS)
			{
				if (cmd & MOD_DDNS)
					DDNS_config();
				else
					DDNS_update();
			}
#endif
#if (MODULE_HTTPD == 1)
			if (cmd2 & MOD_HTTPD)
			{
				HTTPD_update();
			}
#endif
#if (MODULE_UPNP == 1)	
			if (cmd2 & MOD_UPNP)
			{
				UPNP_init();
			}
#endif
#if (MODULE_NTP ==1)
			if (cmd2 & MOD_NTP)
			{
				if (cmd2 & MOD_SYS)
				{
					ntp_update(1);
				}
				else if ((cmd2 & MOD_LAN) || (cmd2 & MOD_WAN))
				{
					ntp_update(0);
				}
				else
				{
#ifdef	CONFIG_DHCPS
					DHCPD_init(); // adjust expired time
#endif
#ifdef	CONFIG_DDNS
					DDNS_corrtime();
#endif
				}
			}
#endif
#ifdef	CONFIG_PPTP_PPPOE
			int dsst,rtt;
			char *buf;
			if ((cmd2 & MOD_VPN) && (cmd2 & MOD_WAN))
			{	
				//if(cmd2 & MOD_WAN)
				//{
					//we need time to wait for other module finish
					cyg_thread_delay(300);
					//first down
					buf = NSTR(SYS_wan_serverip);//sys_wan_ip is pppoe ppp0 ip
					dsst = nstr2ip(buf);					
					if((rtt=net_ping_test(dsst, 1, 3000))<0)
						diag_printf("timeout\n");
					pptp_vpn_mon_down();
					//we need time to do shutdown
					cyg_thread_delay(100);	
				//}
				//then connect
				pptp_vpn_mon();
			}
#endif
        } //if ((cmd&0xff) & MON_CMD_INIT)

		if (cmd & MON_CMD_REBOOT)
        {
			CFG_commit(CFG_COM_SAVE);
			cyg_thread_delay(200);
#if 0 //def	CONFIG_WIRELESS
			WLAN_start(0);
#ifdef SECOND_WIFI
			WLAN_second_start(0);
#endif /* SECOND_WIFI */
#endif
			*((volatile cyg_uint32 *)0xb0000034) = 1<<26;//sht:note this is reg(0xb0000034) is mt7620 RST CTRL register,other chips may be diff addr,be care!!!
			*((volatile cyg_uint32 *)0xb0000038) = 0x1<<9;
			cyg_thread_delay(20);
			sys_reboot();
			return;
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
void mon_start(int cmd)
{
	cyg_mbox_create(&mon_tsk_mbox_h, &mon_tsk_mbox);
	
	cyg_thread_create(5, 
					mon_main,
					(cyg_addrword_t) 0,
					"monitor_thread",
					(void *)&stack[0],
					sizeof(stack),
					&h_mon_t,
					&mon_t);
					  
	cyg_thread_resume(h_mon_t);
}

