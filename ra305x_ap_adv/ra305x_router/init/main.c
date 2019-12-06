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
    main.c

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
#include <cfg_def.h> 
#include <version.h>
#include <network.h>
#include <sys_status.h>
#include <eventlog.h>
#include <led.h>
#include <tfq.h>
#ifdef BRIDGE
#include <net/bridge.h>
#endif

#include <serial_communication.h>
//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
extern void CFG_start(int cmd);
extern void interface_config(void);
extern void mon_start(int cmd);
extern void ip_fastpath_init(void);
extern int  InitEvent (void);
extern int  LAN_start(int cmd);
#ifdef	CONFIG_WIRELESS
extern int WLAN_start(int cmd);
#endif
extern void DNS_config(void);
extern void DHCPD_init(void);
extern void DDNS_init(void);
extern void DNS_init(void);
extern void RT_staticrt_allset(void);
extern void cyg_ntp_start(void);
extern int  HTTPD_init(void);
#ifdef CONFIG_USBHOST
extern int usb_test_init(void);
#endif
extern void UPNP_init(void);
extern void CLI_start(int cmd);
extern void rand_initialize(void);
extern void lo_set_ip(void);
#ifdef CONFIG_BRIDGE
extern int BRG_start(int cmd);
extern void BRG_show();
#endif /* CONFIG_BRIDGE */
#ifdef CONFIG_HARDWARE_WATCHDOG
extern void Watchdog_start(void);
#endif // CONFIG_HARDWARE_WATCHDOG	
extern void ra35xx_cpuload_init(void);
extern void usb_fs_dm_init(void);
extern void HELPERD_init(void);


/* !!!! IMPORTANT !!!!
   reaper_mbox_handle is exposed to anyone who need a reaper!
*/
cyg_handle_t reaper_mbox_handle;
static cyg_mbox     reaper_mbox_info;


int opmode = 1; /*1:Gateway, 2:Apcli as WAN port, 3:Repeater Mode */
int main(void)
{
	diag_printf("\n*******************Starting system init****************************\n");
	//board_init();

#ifdef CONFIG_CPULOAD
	ra35xx_cpuload_init();
#endif
	CFG_start(1);		

#ifdef CONFIG_HARDWARE_WATCHDOG
	Watchdog_start();
#endif // CONFIG_WATCHDOG_SUPPORT
	interface_config();

#ifdef CONFIG_USBHOST
		MUSB_Init(1);
#endif
#if (MODULE_QOS == 1)
	if (opmode != 3)
		tfq_load_config();
#endif

#if MODULE_NAT
	if (opmode != 3)
		ip_fastpath_init();
#endif

#if (MODULE_SYSLOG == 1)
	InitEvent();
#endif
	
#if (MODULE_LANIF == 1)
	LAN_start(1);
#endif

#if (MODULE_WIRELESSIF == 1)
    WLAN_start(1);
#endif

#if (MODULE_BRIDGE == 1)
    cyg_thread_delay(200);
    bdginit();
    BRG_start(1);
    BRG_show();
#endif

	if (opmode != 3)	
		DNS_config();

#if (MODULE_DHCPS == 1)
	if (opmode != 3)
		DHCPD_init();
#endif

#if (MODULE_DDNS == 1)	
	if (opmode != 3)
		DDNS_init();
#endif

#if (MODULE_DNS == 1)
	if (opmode != 3)
		DNS_init();	
#endif 

	if (opmode != 3)	
		RT_staticrt_allset(); // set static route
	
#if (MODULE_NTP == 1)
    cyg_ntp_start();
#endif    
	
#if (MODULE_HTTPD == 1)
	HTTPD_init();
#endif	
#ifdef CONFIG_FILE_SYSTEM
	usb_fs_dm_init();
#endif
	

#ifdef CONFIG_SUPPORT_FTPD
    cyg_mbox_create(&reaper_mbox_handle, &reaper_mbox_info);

    FTPD_Init();
#endif
	SysLog(LOG_USER|SYS_LOG_NOTICE|LOGM_SYSTEM,STR_System_start);

	// start the CLI
    {
        char build[80] ;
#ifndef	CONFIG_CUST_VER_STR
        char ver[16] ;
        CFG_get_str(CFG_STS_FMW_VER, ver);
        CFG_get_str(CFG_STS_FMW_INFO, build);
        SysLog(LOG_USER|SYS_LOG_INFO|LOGM_SYSTEM,STR_Ver " %s %s",  ver, build);
#else
        CFG_get_str(CFG_STS_FMW_INFO, build);
        SysLog(LOG_USER|SYS_LOG_INFO|LOGM_SYSTEM,STR_Ver " %s %s",  sys_cust_ver, build);
#endif
    }

	CLI_start(1);
#ifdef CONFIG_USBHOST
	usb_test_init();
#endif
#if (MODULE_UPNP == 1)
	UPNP_init() ;
#endif
#ifdef RALINK_LLTD_SUPPORT
	LLTP_Start();
#endif//RALINK_LLTD_SUPPORT

#ifdef RALINK_1X_SUPPORT
	{
		Dot1x_Reboot();
	}
#endif //RALINK_1X_SUPPORT
	mon_start(1);

	/* Add by Lea, for some dirty jobs */
	HELPERD_init();

	mjh_start(1);
	
	/* Add by Shawn, use for serial console /dev/tty2 */
	serial_communication_init();
	return 0;
}

