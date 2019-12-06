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
    config_options.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <config.h>
#include <time.h>
#include <cyg/kernel/kapi.h>
#include <cfg_def.h> 
#include <cfg_id.h>
#include <version.h>
#include <network.h>
#include <sys_status.h>
#include <led.h>

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


//
// Software reset pins
//
#ifdef	CONFIG_RST

#endif	// CONFIG_RST


#ifdef	CONFIG_RA305X
//
// Switch port mask settings
//
int ra305x_esw_portmask[] = 
{
	CONFIG_RA305X_LAN_PORTMASK,
	CONFIG_RA305X_WAN_PORTMASK
};
int ra305x_act_portmask = (CONFIG_RA305X_LAN_PORTMASK | CONFIG_RA305X_WAN_PORTMASK);


#endif  // CONFIG_RA305X


//********************
// Watchdog timer
//********************
#ifdef	CONFIG_WATCHDOG
int	config_watchdog_interval = CONFIG_WATCHDOG_INTERVAL;
#else
void watchdog_tmr_func(void)	{ }
#endif


//********************
// TCP/IP options
//********************
int is_net_cluster_cached(void)
{
	return 1;
}

//********************
// DHCPD options
//********************
#ifdef	CONFIG_DHCPS

#ifdef	CONFIG_DHCPS_WAN_DOMAIN
int	config_dhcps_wan_domain = 1;
#else
int config_dhcps_wan_domain = 0;
#endif

#ifndef DHCPD_SAVE_DYNAMIC_CLIENTS
void DHCPD_WriteDynLeases(void)	{return;}
void DHCPD_LoadDynLeases(void)	{return;}
#endif

struct in_addr	DHCPD_GetGwConf(struct in_addr gw)
{
	struct in_addr sa;
	
	if (CFG_get(CFG_LAN_DHCPD_GW, &sa) == -1) 
	{
#ifdef	CONFIG_NAT
		sa = gw;				// set myself as gateway
#else
        sa.s_addr = SYS_lan_gw;	// ap/bridge case, use lan's gw
#endif
	}
	
	return sa;
}

#endif

//********************
// PPPoE options
//********************
#ifdef	CONFIG_PPPOE

#if	defined(CONFIG_POE_DONT_SEND_PADT_WRONG_SID)
int	send_padt_flag = 0;
#else
int	send_padt_flag = 1;
#endif

extern void pppoe_def_password(char *, char *);

typedef struct pppoe_passalg_s {
	int algid;
	void (*pppoe_passalg_fun)(char *, char *);
} pppoe_passalg_t;

pppoe_passalg_t pppoe_pass_algtab[] = {
	{0x1, pppoe_def_password},	/*  Must be the first entry  */
};

int pppoe_avail_pass_alg(void)
{
	return sizeof(pppoe_pass_algtab)/sizeof(pppoe_passalg_t);
}

#endif	/*  CONFIG_PPPOE  */

//********************
// PPP options
//********************
#ifdef	CONFIG_PPP

#if defined(CONFIG_PPP_MANUAL_MODE_WITH_IDLE)
int manual_connect_with_idle = 1;
#else
int manual_connect_with_idle = 0;
#endif

#if	defined(CONFIG_POE_PASSIVE_LCP_ECHO_REPLY)
int	lcp_passive_echo_reply = 1;
int	lcp_serv_echo_fails_max = CONFIG_MAX_LCP_SERV_ECHO_FAILS;
int	lcp_serv_echo_interval = CONFIG_LCP_SERV_ECHO_INTERVAL;
#else
int	lcp_passive_echo_reply = 0;
int	lcp_serv_echo_fails_max = 0;
int lcp_serv_echo_interval = 0;
#endif

int lcp_echo_fails_max = CONFIG_MAX_LCP_ECHO_FAILS;
int lcp_echo_interval = CONFIG_LCP_ECHO_INTERVAL;

#if	!defined(CONFIG_CHAPMS)
void ChapMS()			{return;}
int  ChapMS_Resp()		{return 0;}
void ChapMS_v2()		{return;}
int  ChapMS_v2_Resp()	{return 0;}
void ChapMS_v2_Auth()	{return;}

int	config_chapms = 0;
#else

#if !defined(CONFIG_MPPE)
void mppe_gen_master_key()		{return;}
void mppe_gen_master_key_v2()	{return;}
#endif

int config_chapms = 1;
#endif

#if !defined(CONFIG_MPPE)
void ccp_init()			{return;}
void ccp_lowerup()		{return;}
void ccp_open()			{return;}
void ccp_input()		{return;}
void ccp_protrej()		{return;}
void ppp_ccp_closed()	{return;}
int  ppp_ccp_ioctl()	{return -1;}
void *ppp_ccp_pktin()	{return 0;}
void *ppp_dequeue()		{return 0;}
#endif

#endif


//********************
// HTTPD
//********************
#ifdef	CONFIG_HTTPD

#if	defined(CONFIG_ZWEB)
int	config_zweb = 1;
#else
int config_zweb = 0;
#endif

#if defined(CONFIG_ZWEB) && defined(CONFIG_ZWEB_MAX_PAGE_SIZE)
int	zweb_page_size_max = CONFIG_ZWEB_MAX_PAGE_SIZE;
#else
int	zweb_page_size_max = 32768;
#endif

#if	defined(CONFIG_MODEL_NAME)
char *basic_realm = CONFIG_MODEL_NAME;
#else
char *basic_realm = "";
#endif

char *config_default_page(void)
{
#if	defined(CONFIG_DEFAULT_PAGE)
	if (CONFIG_DEFAULT_PAGE[0] == 0)
		return "index.htm";
	else
		return CONFIG_DEFAULT_PAGE;
#else
	return "index.htm";
#endif
}

#endif


//********************
// DNS options
//********************
// See dns_config.c

//********************
// DDNS options
//********************
#ifdef	CONFIG_DDNS
#if	!defined(CONFIG_ORAY_CLIENTID)
unsigned short	oray_clientid = 0x1001;
unsigned short	oray_clientver = 0x3612;
unsigned long	oray_clientkey = 0x19790629;
#else
unsigned short	oray_clientid = CONFIG_ORAY_CLIENTID;
unsigned short	oray_clientver = CONFIG_ORAY_CLIENTVER;
unsigned long	oray_clientkey = CONFIG_ORAY_CLIENTKEY;
#endif
#endif

//********************
// NAT options
//********************
#ifdef	CONFIG_NAT

#if	defined(CONFIG_NAT_TABLE_MAX)
int	nat_table_max =	CONFIG_NAT_TABLE_MAX;
#else
int nat_table_max =	2047;
#endif	/*  defined(CONFIG_NAT_TABLE_MAX)  */
#endif	/*  CONFIG_NAT  */

//********************
// CONFIG_DL_COOKIE
//********************
#ifdef	CONFIG_DL_COOKIE
#include <dl_cookie.h>
int	config_dl_cookie = 1;
char dl_cookie[] = {DL_COOKIE};
#else
int config_dl_cookie = 0;
char dl_cookie[] = {0};
#endif


//********************
// NTP options
//********************
#ifdef	CONFIG_NTP

#ifdef CONFIG_NTP_NO_SAVE_TIME
int config_ntp_save_time = 0;
#else
int config_ntp_save_time = 1;
#endif

#else

int config_ntp_save_time = 0;

time_t ntp_time()					{return time(0);}
time_t ntp_getlocaltime(int flag)	{return time(0);}
time_t ntp_getlocaltime_offset()	{return 0;}

int Is_ntp_ever_sync()	{return 1;}
int get_ntp_type()		{return 0;}
int get_non_ntp_flag()	{return 1;}

#endif

//********************
// eventlog options
//********************
#ifdef	CONFIG_SYSLOG

int	log_buf_len = CONFIG_LOG_BUF_LEN;

#if defined(CONFIG_LOG_MAIL_THRESH)
int log_mail_thresh = CONFIG_LOG_MAIL_THRESH;
#else
int log_mail_thresh = 400;
#endif

int log_pool_num = 1;

/*
#if defined(CONFIG_LOG_SINGLE_POOL)
int log_pool_num = 1;
#else
int log_pool_num = 2;
#endif
*/
#if	defined(CONFIG_LOG_MAX_COUNT)
int log_count_max = CONFIG_LOG_MAX_COUNT;
#else
int log_count_max = 100;
#endif

#if	defined(CONFIG_LOG_MAIL_ADDR_NUM)
int log_mail_addr_num = CONFIG_LOG_MAIL_ADDR_NUM;
#else
int	log_mail_addr_num = 1;
#endif

#else

int LogEventNow()				{return 0;}

#endif

//********************************
// Firewall for illegal TCP RST
//********************************
#ifdef CONFIG_DROP_ILLEGAL_TCPRST

#else

int (*drop_segmentp) __P((struct ip*, int, struct ifnet *)) = NULL;

#endif


#ifdef CONFIG_TCP_DROP_ILL_SYN_ACK
#define	FR_BLOCK_ILL_SYNACK		0x01
#else
#define FR_BLOCK_ILL_SYNACK		0
#endif
#ifdef CONFIG_DROP_ILL_HTTP_RESP
#define FR_BLOCK_ILL_HTTP_RESP		0x02
#else
#define FR_BLOCK_ILL_HTTP_RESP		0
#endif
#ifdef CONFIG_TCP_BLOCK_NETSNIPER
#define FR_BLOCK_NETSNIPER		0x04
#else
#define FR_BLOCK_NETSNIPER		0
#endif

unsigned short fr_special_block_fg = (FR_BLOCK_ILL_SYNACK | FR_BLOCK_ILL_HTTP_RESP | FR_BLOCK_NETSNIPER);

//********************************
// Firewall URL filter option
//********************************
#ifdef CONFIG_FW_URL_BLOCK_PAGE
char *_url_blockpage_name = CONFIG_FW_URL_BLOCK_PAGE;
#else
char *_url_blockpage_name = NULL;
#endif

//********************************
// mbuf and cluster options
//********************************
#ifdef	CONFIG_NET_MBUF_USAGE
int config_net_mbuf_usage = CONFIG_NET_MBUF_USAGE;
#endif

#ifdef	CONFIG_NET_CLUSTER_USAGE
int config_net_cluster_usage = CONFIG_NET_CLUSTER_USAGE;
#endif

#ifdef	CONFIG_NET_CLUSTER_GROUP2
int config_net_cluster_4k_usage = CONFIG_NET_CLUSTER_4K_USAGE;
#endif


//********************
// CFG options
//********************
#ifdef CONFIG_CFG_FMW_IDSTR_EN
char *_cfgfmw_idstr = CONFIG_CFG_FMW_IDSTR;
#else
char *_cfgfmw_idstr = NULL;

#endif	/*  CONFIG_CFG_FMW_IDSTR_EN  */


char *config_cust_ver_str = SYS_CUST_VER_STR;
#ifndef CONFIG_CUST_VER_STR_INTERNAL
char *config_cust_ver_str_internal = SYS_CUST_VER_STR;
#else
char *config_cust_ver_str_internal = CONFIG_CUST_VER_STR_INTERNAL;
#endif

