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
    cfg_id.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#ifndef CFG_ID_H
#define CFG_ID_H

//------------------------------------------------------------------------------
// Config ID is encoding with 32 bit
// 31       23       15       7        0
// +--------+--------+--------+--------+
// | module |   id   |datatype| index  |
// +--------+--------+--------+--------+
//------------------------------------------------------------------------------

#define CFG_ID_IDX(id)      (id&0xff)
#define CFG_ID_TYPE(id)     ((id>>8)&0xff)
#define CFG_ID_SUB(id)      ((id>>16)&0xff)
#define CFG_ID_MOD(id)      ((id>>24)&0xff)

#define CFG_ID_(mod,sub,type,i) (((mod&0xff)<<24)| ((sub&0xff)<<16) | ((type&0xff)<<8)|(i&0xff))

#define CFG_TYPE_INT    1
#define CFG_TYPE_STR    2
#define CFG_TYPE_IP     3
#define CFG_TYPE_MAC    4
// since version string (w.x.y.z) is the same as ip format, just take it 
#define CFG_TYPE_VER    (CFG_TYPE_IP)

#define CFG_ID_SYS  1
#define CFG_ID_LAN  2
#define CFG_ID_WAN  3
#define CFG_ID_WLN  4
#define CFG_ID_RT   5
#define CFG_ID_MROUTED	5		/*  mrouted: part of routing  */
#define CFG_ID_DDNS 6
#define CFG_ID_POE  7
#define CFG_ID_PTP  8
#define CFG_ID_NAT  9
#define CFG_ID_FW   10
#define CFG_ID_UPP  11
#define CFG_ID_LOG  12
#define CFG_ID_L2T  13
#define CFG_ID_DNS	14
#define CFG_ID_QOS	15

#ifdef CONFIG_IPTV
#define  CFG_ID_IPTV   16
#endif
#define	CFG_ID_SOC	32		// SoC Settings
/*  Note: module ID behind 33 (included) will not be saved in the flash  */
#define CFG_ID_STS  33		// running status
#define CFG_ID_HW   129		// hardware, factory, firmware version, boot code

#define CFG_ID_HEADER		CFG_ID_(0,1,CFG_TYPE_INT,0)
#define CFG_ID_CHKSUM		CFG_ID_(CFG_ID_STS,0,CFG_TYPE_INT,0)
#define CFG_HEADER_MAGIC	0x55AA3225
#define CFG_ID_HEADER_USED	CFG_ID_(0,1,0,0)
#define CFG_ID_CHKSUM_USED	CFG_ID_(CFG_ID_STS,0,0,0)

#define CFG_ID_FLAG_RONLY  ((1<<7)<<24) // read only , don't save

#define CFG_MAX_OBJ_SZ      256
#define CFG_STR_MAX_SZ      CFG_MAX_OBJ_SZ

// DDNS
#define CFG_DDNS_(idx,type) CFG_ID_(CFG_ID_DDNS,idx,type,0)
#define CFG_DDNS_EN	            CFG_DDNS_(1,CFG_TYPE_INT)
#define CFG_DDNS_SET		CFG_DDNS_(8,CFG_TYPE_STR)
#define CFG_DDNS_IP			CFG_DDNS_(9,CFG_TYPE_IP)

// FIREWALL
#define CFG_FW_(idx,type)   CFG_ID_(CFG_ID_FW,idx,type,0)
#define CFG_FW_FLT_CLN_EN	CFG_FW_(1,CFG_TYPE_INT)
#define CFG_FW_FLT_CLN		CFG_FW_(2,CFG_TYPE_STR)
#define CFG_FW_FLT_MAC_EN	CFG_FW_(3,CFG_TYPE_INT)
#define CFG_FW_FLT_MAC      CFG_FW_(4,CFG_TYPE_STR)
#define CFG_FW_FLT_MAC_MODE CFG_FW_(5,CFG_TYPE_INT)
#define CFG_FW_FLT_URL_EN	CFG_FW_(6,CFG_TYPE_INT)
#define CFG_FW_FLT_URL      CFG_FW_(7,CFG_TYPE_STR)
#define CFG_FW_FRAG_OK	    CFG_FW_(8,CFG_TYPE_INT)
#define CFG_FW_HACKER_ATT	CFG_FW_(9,CFG_TYPE_INT)
#define CFG_FW_ICMP_RESP	CFG_FW_(10,CFG_TYPE_INT)
#define CFG_FW_IDENT_DIS	CFG_FW_(11,CFG_TYPE_INT)
#define CFG_FW_NBIOS_DROP	CFG_FW_(12,CFG_TYPE_INT)
#define CFG_FW_PING_DIS_LAN	CFG_FW_(13,CFG_TYPE_INT)
#define CFG_FW_PING_DIS_WAN	CFG_FW_(14,CFG_TYPE_INT)
#define CFG_FW_PSCAN_DROP	CFG_FW_(15,CFG_TYPE_INT)
//#define CFG_FW_RM_EN		CFG_FW_(16,CFG_TYPE_INT)
//#define CFG_FW_RM_IP		CFG_FW_(17,CFG_TYPE_IP)
//#define CFG_FW_RM_PORT		CFG_FW_(18,CFG_TYPE_INT)
#define CFG_FW_IP_SPOOF		CFG_FW_(19,CFG_TYPE_INT)
#define CFG_FW_SHORT		CFG_FW_(20,CFG_TYPE_INT)
#define CFG_FW_PING_DEATH	CFG_FW_(21,CFG_TYPE_INT)
#define CFG_FW_LAND			CFG_FW_(22,CFG_TYPE_INT)
#define CFG_FW_SNORK		CFG_FW_(23,CFG_TYPE_INT)
#define CFG_FW_UDP_PLOOP	CFG_FW_(24,CFG_TYPE_INT)
#define CFG_FW_TCP_NULL		CFG_FW_(25,CFG_TYPE_INT)
#define CFG_FW_SMURF		CFG_FW_(26,CFG_TYPE_INT)
#define CFG_FW_SYN_FLOOD	CFG_FW_(27,CFG_TYPE_INT)
#define CFG_FW_SYNF_RATE	CFG_FW_(28,CFG_TYPE_INT)
#define CFG_FW_EN			CFG_FW_(29,CFG_TYPE_INT)
#define CFG_FW_MSN_DIS		CFG_FW_(30,CFG_TYPE_INT)
#define CFG_FW_QQ_DIS		CFG_FW_(31,CFG_TYPE_INT)
#define CFG_FW_FLT_PKT      CFG_FW_(32,CFG_TYPE_STR)
#define CFG_FW_FLT_DN_MODE 	CFG_FW_(33,CFG_TYPE_INT)
#define CFG_FW_FLT_DNB      CFG_FW_(34,CFG_TYPE_STR)
#define CFG_FW_FLT_DNA      CFG_FW_(35,CFG_TYPE_STR)


// LAN
#define CFG_LAN_(idx,type)		CFG_ID_(CFG_ID_LAN,idx,type,0)
#define CFG_LAN_DHCPD_DOMAIN	CFG_LAN_(1,CFG_TYPE_STR)
#define CFG_LAN_DHCPD_EN		CFG_LAN_(2,CFG_TYPE_INT)
#define CFG_LAN_DHCPD_END		CFG_LAN_(3,CFG_TYPE_IP)
#define CFG_LAN_DHCPD_LEASET	CFG_LAN_(4,CFG_TYPE_INT)
#define CFG_LAN_DHCPD_START		CFG_LAN_(5,CFG_TYPE_IP)
#define CFG_LAN_DHCPD_GW		CFG_LAN_(6,CFG_TYPE_INT)
#define CFG_LAN_GW				CFG_LAN_(7,CFG_TYPE_IP)
#define CFG_LAN_IP				CFG_LAN_(8,CFG_TYPE_IP)
#define CFG_LAN_MSK				CFG_LAN_(9,CFG_TYPE_IP)
#define CFG_LAN_IP_MODE			CFG_LAN_(10,CFG_TYPE_INT)
#define CFG_LAN_DHCPD_SLEASE	CFG_LAN_(11,CFG_TYPE_STR)
#define CFG_LAN_DHCPD_DLEASE	CFG_LAN_(12,CFG_TYPE_STR)
#define	CFG_LAN_DHCPD_PASS_WDN	CFG_LAN_(13,CFG_TYPE_INT)
#define CFG_LAN_PHY		CFG_LAN_(14,CFG_TYPE_INT)
//For IPv6
#define CFG_LAN_GW_v6			CFG_LAN_(15,CFG_TYPE_STR)
#define CFG_LAN_IP_v6				CFG_LAN_(16,CFG_TYPE_STR)
#define CFG_LAN_MSK_v6			CFG_LAN_(17,CFG_TYPE_STR)
#define CFG_LAN_DHCP_v6_EN		CFG_LAN_(18,CFG_TYPE_INT)
#define CFG_DHCPV6_START		CFG_LAN_(19,CFG_TYPE_STR)
#define CFG_DHCPV6_END			CFG_LAN_(20,CFG_TYPE_STR)
#define CFG_DHCPV6_VLIFE			CFG_LAN_(21,CFG_TYPE_INT)
#define CFG_DHCPV6_PLIFE			CFG_LAN_(22,CFG_TYPE_INT)
#define CFG_DHCPV6_SDUID		CFG_LAN_(23,CFG_TYPE_INT)


// NAT
#define CFG_NAT_(idx,type)  CFG_ID_(CFG_ID_NAT,idx,type,0)
#define CFG_NAT_DMZ_EN	    CFG_NAT_(1,CFG_TYPE_INT)
#define CFG_NAT_DMZ_HOST	CFG_NAT_(2,CFG_TYPE_STR)
#define CFG_NAT_PMAP   	    CFG_NAT_(3,CFG_TYPE_STR)
#define CFG_NAT_PTRG   	    CFG_NAT_(4,CFG_TYPE_STR)
#define CFG_NAT_VTS	        CFG_NAT_(5,CFG_TYPE_STR)
#define CFG_NAT_ALG_FTP		CFG_NAT_(6,CFG_TYPE_INT)
#define CFG_NAT_ALG_NM		CFG_NAT_(7,CFG_TYPE_INT)
#define CFG_NAT_ALG_PPTP	CFG_NAT_(8,CFG_TYPE_INT)
#define CFG_NAT_ALG_MSN		CFG_NAT_(9,CFG_TYPE_INT)
#define CFG_NAT_ALG_IPSEC	CFG_NAT_(10,CFG_TYPE_INT)
#define CFG_NAT_EN			CFG_NAT_(11,CFG_TYPE_INT)
#define CFG_NAT_ALG_FTP_NSP	CFG_NAT_(12,CFG_TYPE_INT)
#define CFG_NAT_ALG_BATTLEN	CFG_NAT_(13,CFG_TYPE_INT)
#define CFG_NAT_SDMZ_MAC	CFG_NAT_(14,CFG_TYPE_MAC)
#define CFG_NAT_ALG_L2TP	CFG_NAT_(15,CFG_TYPE_INT)
#define CFG_NAT_SCHED		CFG_NAT_(16,CFG_TYPE_STR)
#define CFG_NAT_ALG_RTSP	CFG_NAT_(17,CFG_TYPE_INT)
#define CFG_NAT_ALG_RTSP_NSP	CFG_NAT_(18,CFG_TYPE_INT)
#define CFG_NAT_ALG_SIP		CFG_NAT_(19,CFG_TYPE_INT)

// ROUTE
#define CFG_RT_(idx,type)   	CFG_ID_(CFG_ID_RT,idx,type,0)
#define CFG_MROUTED_(idx, type)	CFG_ID_(CFG_ID_MROUTED,idx, type, 0)
#define CFG_RT_ENTRY  		CFG_RT_(1,CFG_TYPE_STR)
#define CFG_MROUTED_EN		CFG_MROUTED_(2, CFG_TYPE_INT)


// SYSTEM variable
#define CFG_SYS_(idx,type)  CFG_ID_(CFG_ID_SYS,idx,type,0)
#define CFG_SYS_ADMPASS	    CFG_SYS_(1,CFG_TYPE_STR)
#define CFG_SYS_DAYLITE	    CFG_SYS_(2,CFG_TYPE_INT)
#define CFG_SYS_DOMAIN		CFG_SYS_(3,CFG_TYPE_STR)
#define CFG_SYS_IDLET		CFG_SYS_(4,CFG_TYPE_INT)
#define CFG_SYS_NAME		CFG_SYS_(5,CFG_TYPE_STR)
#define CFG_SYS_PASS		CFG_SYS_(6,CFG_TYPE_STR)
#define CFG_SYS_RM_EN		CFG_SYS_(7,CFG_TYPE_INT)
#define CFG_SYS_RM_IP		CFG_SYS_(8,CFG_TYPE_IP)
#define CFG_SYS_RM_PORT		CFG_SYS_(9,CFG_TYPE_INT)
#define CFG_SYS_TZONE		CFG_SYS_(10,CFG_TYPE_INT)
#define CFG_SYS_USERS		CFG_SYS_(11,CFG_TYPE_STR)
#define CFG_SYS_NTPTYPE		CFG_SYS_(12,CFG_TYPE_INT)
#define CFG_SYS_NTPSRV		CFG_SYS_(13,CFG_TYPE_STR)
#define CFG_SYS_DAYLITE_SM	CFG_SYS_(14,CFG_TYPE_INT)
#define CFG_SYS_DAYLITE_SD	CFG_SYS_(15,CFG_TYPE_INT)
#define CFG_SYS_DAYLITE_EM	CFG_SYS_(16,CFG_TYPE_INT)
#define CFG_SYS_DAYLITE_ED	CFG_SYS_(17,CFG_TYPE_INT)
#define CFG_SYS_WAN			CFG_SYS_(18,CFG_TYPE_STR)
#define CFG_SYS_LAN			CFG_SYS_(19,CFG_TYPE_STR)
#define CFG_SYS_OPMODE		CFG_SYS_(20,CFG_TYPE_INT)

#define CFG_SYS_TIME		CFG_SYS_(21,CFG_TYPE_INT)
#define CFG_SYS_TZONE1		CFG_SYS_(23,CFG_TYPE_INT)
#define CFG_SYS_LM			CFG_SYS_(24,CFG_TYPE_INT)
#define CFG_SYS_Login_Flags	CFG_SYS_(25,CFG_TYPE_INT)
#define CFG_SYS_Language	CFG_SYS_(26,CFG_TYPE_INT)

// UPNP
#define CFG_UPP_(idx,type)  CFG_ID_(CFG_ID_UPP,idx,type,0)
#define CFG_UPP_AD_TIME		CFG_UPP_(1,CFG_TYPE_INT)
#define CFG_UPP_EN		    CFG_UPP_(2,CFG_TYPE_INT)
#define CFG_UPP_PORT		CFG_UPP_(3,CFG_TYPE_INT)
#define CFG_UPP_SUB_TIMEOUT	CFG_UPP_(4,CFG_TYPE_INT)

// WAN
#define CFG_WAN_(idx,type)  CFG_ID_(CFG_ID_WAN,idx,type,0)
#define CFG_WAN_DHCP_BPA_EN	CFG_WAN_(9,CFG_TYPE_INT)
#define CFG_WAN_DHCP_MAC	CFG_WAN_(8,CFG_TYPE_MAC)
#define CFG_WAN_DHCP_NAME	CFG_WAN_(7,CFG_TYPE_STR)
#define CFG_WAN_DNS			CFG_WAN_(6,CFG_TYPE_IP)
#define CFG_WAN_GW		    CFG_WAN_(4,CFG_TYPE_IP)
#define CFG_WAN_IP		    CFG_WAN_(2,CFG_TYPE_IP)
#define CFG_WAN_MSK		    CFG_WAN_(3,CFG_TYPE_IP)
#define CFG_WAN_IP_MODE		CFG_WAN_(1,CFG_TYPE_INT)
#define CFG_WAN_IP_MANY		CFG_WAN_(5,CFG_TYPE_INT)
#define CFG_WAN_IP_MANY_IP	CFG_WAN_(10,CFG_TYPE_IP)
#define CFG_WAN_IP_MANY_MSK	CFG_WAN_(11,CFG_TYPE_IP)
#define CFG_WAN_DHCP_MAC_EN	CFG_WAN_(12,CFG_TYPE_INT)
#define CFG_WAN_DHCP_BPA_USER	CFG_WAN_(13,CFG_TYPE_STR)
#define CFG_WAN_DHCP_BPA_PASS	CFG_WAN_(14,CFG_TYPE_STR)
#define CFG_WAN_DHCP_BPA_SERV	CFG_WAN_(15,CFG_TYPE_STR)
#define CFG_WAN_DHCP_REQIP	CFG_WAN_(16,CFG_TYPE_IP)
#define CFG_WAN_MTU			CFG_WAN_(17,CFG_TYPE_INT)
#define CFG_WAN_DHCP_MTU	CFG_WAN_(18,CFG_TYPE_INT)
#define CFG_WAN_DNS_EN		CFG_WAN_(19,CFG_TYPE_INT)
#define CFG_WAN_DNS_DEF		CFG_WAN_(20,CFG_TYPE_IP)
#define CFG_WAN_DHCP_MAN	CFG_WAN_(21,CFG_TYPE_INT)
#define CFG_WAN_PHY			CFG_WAN_(22,CFG_TYPE_INT)
//For IPv6
#define CFG_WAN_IP_v6			CFG_WAN_(23,CFG_TYPE_STR)
#define CFG_WAN_MSK_v6			CFG_WAN_(24,CFG_TYPE_STR)
#define CFG_WAN_GW_v6			CFG_WAN_(25,CFG_TYPE_STR)
#define CFG_WAN_DNS_v6			CFG_WAN_(26,CFG_TYPE_STR)
#define CFG_WAN_DHCP_v6_EN		CFG_LAN_(27,CFG_TYPE_INT)
#define CFG_WAN_DHCPV6_CDUID		CFG_LAN_(28,CFG_TYPE_INT)


// PPPOE
#define CFG_POE_(idx,type)  CFG_ID_(CFG_ID_POE,idx,type,0)
#define CFG_POE_AUTO	    CFG_POE_(1,CFG_TYPE_INT)
#define CFG_POE_IDLET	    CFG_POE_(2,CFG_TYPE_INT)
#define CFG_POE_MTU		    CFG_POE_(3,CFG_TYPE_INT)
#define CFG_POE_PASS	    CFG_POE_(4,CFG_TYPE_STR)
#define CFG_POE_SVC		    CFG_POE_(5,CFG_TYPE_STR)
#define CFG_POE_USER	    CFG_POE_(6,CFG_TYPE_STR)
#define	CFG_POE_AC			CFG_POE_(7,CFG_TYPE_STR)
#define	CFG_POE_ST			CFG_POE_(8,CFG_TYPE_INT)
#define	CFG_POE_ET			CFG_POE_(9,CFG_TYPE_INT)
#define	CFG_POE_SEID		CFG_POE_(10,CFG_TYPE_INT)
#define	CFG_POE_SIPE		CFG_POE_(11,CFG_TYPE_INT)
#define	CFG_POE_SIP			CFG_POE_(12,CFG_TYPE_IP)
#define	CFG_POE_EN			CFG_POE_(13,CFG_TYPE_INT)
#define	CFG_POE_DFLRT		CFG_POE_(14,CFG_TYPE_INT)
#define	CFG_POE_PASSWDALG	CFG_POE_(15,CFG_TYPE_INT)
#define	CFG_POE_PA_MODE		CFG_POE_(16,CFG_TYPE_INT)
#define	CFG_POE_WANIF		CFG_POE_(17,CFG_TYPE_INT)
#define CFG_POE_IP		CFG_POE_(18,CFG_TYPE_IP)
#define CFG_POE_MSK		CFG_POE_(19,CFG_TYPE_IP)
#define CFG_POE_GW		CFG_POE_(20,CFG_TYPE_IP)
#define CFG_POE_DNS		CFG_POE_(21,CFG_TYPE_IP)


// PPTP
#define CFG_PTP_(idx,type)  CFG_ID_(CFG_ID_PTP,idx,type,0)
#define CFG_PTP_AUTO	    CFG_PTP_(1,CFG_TYPE_INT)
#define CFG_PTP_HOST	    CFG_PTP_(2,CFG_TYPE_STR)
#define CFG_PTP_ID		    CFG_PTP_(3,CFG_TYPE_STR)
#define CFG_PTP_IDLET	    CFG_PTP_(4,CFG_TYPE_INT)
#define CFG_PTP_IP		    CFG_PTP_(5,CFG_TYPE_IP)
#define CFG_PTP_MSK		    CFG_PTP_(6,CFG_TYPE_IP)
#define CFG_PTP_MTU		    CFG_PTP_(7,CFG_TYPE_INT)
#define CFG_PTP_PASS	    CFG_PTP_(8,CFG_TYPE_STR)
#define CFG_PTP_SVR		    CFG_PTP_(9,CFG_TYPE_IP)
#define CFG_PTP_USER	    CFG_PTP_(10,CFG_TYPE_STR)
#define CFG_PTP_SVRDM		CFG_PTP_(11,CFG_TYPE_STR)
#define CFG_PTP_SVRD_SEL	CFG_PTP_(12,CFG_TYPE_INT)
#define CFG_PTP_WANIF		CFG_PTP_(13,CFG_TYPE_INT)
#define CFG_PTP_CLONE_MAC	CFG_PTP_(14,CFG_TYPE_MAC)
#define CFG_PTP_DHCP_MAC_EN CFG_PTP_(15,CFG_TYPE_INT)
#define CFG_PTP_MPPE_EN 	CFG_PTP_(16,CFG_TYPE_INT)
#define CFG_PTP_GW		    CFG_PTP_(17,CFG_TYPE_IP)
#define CFG_PTP_DNS		CFG_POE_(18,CFG_TYPE_IP)

// L2TP
#define CFG_L2T_(idx,type)  CFG_ID_(CFG_ID_L2T,idx,type,0)
#define CFG_L2T_AUTO	    CFG_L2T_(1,CFG_TYPE_INT)
#define CFG_L2T_IDLET	    CFG_L2T_(4,CFG_TYPE_INT)
#define CFG_L2T_IP		    CFG_L2T_(5,CFG_TYPE_IP)
#define CFG_L2T_MSK		    CFG_L2T_(6,CFG_TYPE_IP)
#define CFG_L2T_MTU		    CFG_L2T_(7,CFG_TYPE_INT)
#define CFG_L2T_PASS	    CFG_L2T_(8,CFG_TYPE_STR)
#define CFG_L2T_SVR		    CFG_L2T_(9,CFG_TYPE_IP)
#define CFG_L2T_USER	    CFG_L2T_(10,CFG_TYPE_STR)
#define CFG_L2T_SVRDM		CFG_L2T_(11,CFG_TYPE_STR)
#define CFG_L2T_SVRD_SEL	CFG_L2T_(12,CFG_TYPE_INT)
#define CFG_L2T_WANIF		CFG_L2T_(13,CFG_TYPE_INT)
#define CFG_L2T_CLONE_MAC	CFG_L2T_(14,CFG_TYPE_MAC)
#define CFG_L2T_DHCP_MAC_EN CFG_L2T_(15,CFG_TYPE_INT)
#define CFG_L2T_GW		    CFG_L2T_(16,CFG_TYPE_IP)
#define CFG_L2T_DNS		CFG_POE_(17,CFG_TYPE_IP)

// DNS proxy
#define CFG_DNS_(idx,type)  CFG_ID_(CFG_ID_DNS,idx,type,0)
#define CFG_DNS_EN	    	CFG_DNS_(1,CFG_TYPE_INT) 
#define CFG_DNS_SVR	    	CFG_DNS_(2,CFG_TYPE_IP) 
#define CFG_DNS_FIX	    	CFG_DNS_(3,CFG_TYPE_INT) 
#define CFG_DNS_DEF		    CFG_DNS_(4,CFG_TYPE_IP)
#define CFG_DNS_FIX_FIRST	CFG_DNS_(5,CFG_TYPE_INT) 
#define CFG_DNS_HOSTS		CFG_DNS_(6,CFG_TYPE_STR) 

#ifdef CONFIG_IPTV
#define CFG_IPTV_(idx,type)   CFG_ID_(CFG_ID_IPTV,idx,type,0)
#define CFG_IPTV_EN	    		CFG_IPTV_(1,CFG_TYPE_INT) 
#define  CFG_IPTV_PORT  	CFG_IPTV_(2,CFG_TYPE_INT) 
#endif
// QOS
#define CFG_QOS_(idx,type)  CFG_ID_(CFG_ID_QOS,idx,type,0)
#define CFG_QOS_EN	    	CFG_QOS_(1,CFG_TYPE_INT) 
#define CFG_QOS_IPTABLE	CFG_QOS_(2,CFG_TYPE_STR)

// WLAN
#define CFG_WLN_(idx,type)  CFG_ID_(CFG_ID_WLN,idx,type,0)
#define CFG_WLN_AP_MAC		CFG_WLN_(1,CFG_TYPE_MAC)
#define CFG_WLN_AP_MODE		CFG_WLN_(2,CFG_TYPE_INT)
#define CFG_WLN_AP_PEER		CFG_WLN_(3,CFG_TYPE_MAC)
#define CFG_WLN_AUTH		CFG_WLN_(4,CFG_TYPE_INT)
#define CFG_WLN_BasicRate	CFG_WLN_(5,CFG_TYPE_INT)
#define CFG_WLN_BeaconPeriod		CFG_WLN_(6,CFG_TYPE_INT)
#define CFG_WLN_Channel		    CFG_WLN_(7,CFG_TYPE_INT)
#define CFG_WLN_CLN_MAC		CFG_WLN_(8,CFG_TYPE_MAC)
#define CFG_WLN_DtimPeriod		CFG_WLN_(9,CFG_TYPE_INT)
#define CFG_WLN_FLT_ALLOW	CFG_WLN_(10,CFG_TYPE_INT)
#define CFG_WLN_FLT_EN		CFG_WLN_(11,CFG_TYPE_INT)
#define CFG_WLN_FLT_MAC	    CFG_WLN_(12,CFG_TYPE_MAC)
#define CFG_WLN_PKTSZ		CFG_WLN_(13,CFG_TYPE_INT)
#define CFG_WLN_PREAMP		CFG_WLN_(14,CFG_TYPE_INT)
#define CFG_WLN_RTSThreshold		    CFG_WLN_(15,CFG_TYPE_INT)
#define CFG_WLN_SSID		CFG_WLN_(16,CFG_TYPE_STR)
#define CFG_WLN_SSIDBCST	CFG_WLN_(17,CFG_TYPE_INT)
#define CFG_WLN_TxPower		CFG_WLN_(18,CFG_TYPE_INT)
#define CFG_WLN_WEP_EN		CFG_WLN_(19,CFG_TYPE_INT)
#define CFG_WLN_WEP_KEY	    CFG_WLN_(20,CFG_TYPE_STR)
#define CFG_WLN_WepKeyLen		CFG_WLN_(229,CFG_TYPE_STR)
#define CFG_WLN_WEP_KEYMODE	CFG_WLN_(22,CFG_TYPE_INT)
#define CFG_WLN_WEP_KEYSET	CFG_WLN_(23,CFG_TYPE_INT)
#define CFG_WLN_WEP_PASSPHASE	CFG_WLN_(24,CFG_TYPE_STR)
#define CFG_WLN_EN			CFG_WLN_(25,CFG_TYPE_INT)
#define CFG_WLN_DOMAIN		CFG_WLN_(26,CFG_TYPE_INT)
#define CFG_WLN_TxRate		CFG_WLN_(27,CFG_TYPE_INT)
#define CFG_WLN_RADIUS_Server	CFG_WLN_(27,CFG_TYPE_STR)
#define CFG_WLN_RADIUS_Port	CFG_WLN_(28,CFG_TYPE_STR)
#define CFG_WLN_RADIUS_SECRETE 	CFG_WLN_(29,CFG_TYPE_STR)
#define CFG_WLN_WPA_REKEYTIME	CFG_WLN_(30,CFG_TYPE_INT)
#define CFG_WLN_WPA_ENCRYTYPE	CFG_WLN_(31,CFG_TYPE_INT)
#define CFG_WLN_WPA_PASSPHASE	CFG_WLN_(32,CFG_TYPE_STR)
#define	CFG_WLN_RATEMODE	CFG_WLN_(33,CFG_TYPE_INT)
#define CFG_WLN_MODE            CFG_WLN_(34,CFG_TYPE_INT)
#define CFG_WLN_SECURITY_MODE   CFG_WLN_(35,CFG_TYPE_INT)
#define CFG_WLN_WPA_REKEYTYPE	CFG_WLN_(36,CFG_TYPE_INT)
#define CFG_WLN_BGProtection      CFG_WLN_(37,CFG_TYPE_INT)
#define CFG_WLN_DISABLE_OLBC	CFG_WLN_(38,CFG_TYPE_INT)
#define CFG_WLN_TxBurst         CFG_WLN_(49,CFG_TYPE_INT)
#define CFG_WLN_PktAggregate       CFG_WLN_(40,CFG_TYPE_INT)
#define CFG_WLN_TURBO           CFG_WLN_(41,CFG_TYPE_INT)
#define CFG_WLN_ShortSlot       CFG_WLN_(42,CFG_TYPE_INT)
#define CFG_WLN_IEEE80211H          CFG_WLN_(43,CFG_TYPE_STR)
#define CFG_WLN_PREAUTH         CFG_WLN_(44,CFG_TYPE_INT)
#define CFG_WLN_NOFORWARD       CFG_WLN_(45,CFG_TYPE_INT)
#define CFG_WLN_ANTENNA		CFG_WLN_(46,CFG_TYPE_INT)
#define CFG_WLN_SENS		CFG_WLN_(47,CFG_TYPE_INT)

/* WLAN, added by Rorscha */
#define CFG_WLN_WirelessMode	CFG_WLN_(51,CFG_TYPE_INT)
#define CFG_WLN_CountryCode	CFG_WLN_(52,CFG_TYPE_STR)
#define CFG_WLN_HideSSID		CFG_WLN_(53,CFG_TYPE_STR)
#define CFG_WLN_BssidNum		CFG_WLN_(54,CFG_TYPE_INT)
#define CFG_WLN_AutoChannelSelect	CFG_WLN_(55,CFG_TYPE_INT)
#define CFG_WLN_WdsEnable		CFG_WLN_(56,CFG_TYPE_INT)
#define CFG_WLN_WdsEncrypType	CFG_WLN_(57,CFG_TYPE_STR)
#define CFG_WLN_DefaultKeyID	CFG_WLN_(58,CFG_TYPE_STR)
#define CFG_WLN_WdsKey			CFG_WLN_(59,CFG_TYPE_STR)
#define CFG_WLN_HT_OpMode		CFG_WLN_(64,CFG_TYPE_INT)
#define CFG_WLN_HT_BW			CFG_WLN_(65,CFG_TYPE_INT)
#define CFG_WLN_HT_GI			CFG_WLN_(66,CFG_TYPE_INT)
#define CFG_WLN_HT_MCS		CFG_WLN_(67,CFG_TYPE_INT)
#define CFG_WLN_HT_HTC		CFG_WLN_(68,CFG_TYPE_INT)
#define CFG_WLN_HT_RDG		CFG_WLN_(69,CFG_TYPE_INT)
#define CFG_WLN_HT_LinkAdapt	CFG_WLN_(70,CFG_TYPE_INT)
#define CFG_WLN_HT_EXTCHA		CFG_WLN_(71,CFG_TYPE_INT)
#define CFG_WLN_HT_AMSDU		CFG_WLN_(72,CFG_TYPE_INT)
#define CFG_WLN_EncrypType		CFG_WLN_(73,CFG_TYPE_STR)
#define CFG_WLN_WdsPhyMode	CFG_WLN_(74,CFG_TYPE_STR)
#define CFG_WLN_ModuleName	CFG_WLN_(75,CFG_TYPE_STR)
#define CFG_WLN_HT_STBC		CFG_WLN_(76,CFG_TYPE_INT)
#define CFG_WLN_HT_DisallowTKIP	CFG_WLN_(77,CFG_TYPE_INT)
#define CFG_WLN_HT_40MHZ_INTOLERANT	CFG_WLN_(78,CFG_TYPE_INT)
#define CFG_WLN_HT_RxStream	CFG_WLN_(79,CFG_TYPE_INT)
#define CFG_WLN_HT_TxStream	CFG_WLN_(80,CFG_TYPE_INT)
#define CFG_WLN_WiFiTest		CFG_WLN_(81,CFG_TYPE_INT)
#define CFG_WLN_FragThreshold	CFG_WLN_(82,CFG_TYPE_STR)
#define CFG_WLN_TxPreamble		CFG_WLN_(83,CFG_TYPE_STR)
#define CFG_WLN_WmmCapable	CFG_WLN_(84,CFG_TYPE_STR)
#define CFG_WLN_APSDCapable	CFG_WLN_(85,CFG_TYPE_STR)
#define CFG_WLN_DLSCapable		CFG_WLN_(86,CFG_TYPE_STR)
#define CFG_WLN_APAifsn			CFG_WLN_(87,CFG_TYPE_STR)
#define CFG_WLN_APCwmin		CFG_WLN_(88,CFG_TYPE_STR)
#define CFG_WLN_APCwmax		CFG_WLN_(89,CFG_TYPE_STR)
#define CFG_WLN_APTxop			CFG_WLN_(90,CFG_TYPE_STR)
#define CFG_WLN_APACM			CFG_WLN_(91,CFG_TYPE_STR)
#define CFG_WLN_BSSAifsn		CFG_WLN_(92,CFG_TYPE_STR)
#define CFG_WLN_BSSCwmin		CFG_WLN_(93,CFG_TYPE_STR)
#define CFG_WLN_BSSCwmax		CFG_WLN_(94,CFG_TYPE_STR)
#define CFG_WLN_BSSTxop		CFG_WLN_(95,CFG_TYPE_STR)
#define CFG_WLN_BSSACM		CFG_WLN_(96,CFG_TYPE_STR)
#define CFG_WLN_AckPolicy		CFG_WLN_(97,CFG_TYPE_STR)
#define CFG_WLN_HT_AutoBA		CFG_WLN_(98,CFG_TYPE_STR)
#define CFG_WLN_HT_MpduDensity CFG_WLN_(99,CFG_TYPE_STR)
#define CFG_WLN_HT_BAWinSize	CFG_WLN_(100,CFG_TYPE_STR)
#define CFG_WLN_HT_BADecline	CFG_WLN_(101,CFG_TYPE_STR)
#define CFG_WLN_NintendoCapable	CFG_WLN_(102,CFG_TYPE_STR)
#define CFG_WLN_RADIUS_Key	CFG_WLN_(103,CFG_TYPE_STR)
#define CFG_WLN_session_timeout_interval	CFG_WLN_(104,CFG_TYPE_STR)
#define CFG_WLN_idle_timeout_interval	CFG_WLN_(105,CFG_TYPE_STR)
#define CFG_WLN_AccessPolicy0	CFG_WLN_(106,CFG_TYPE_INT)
#define CFG_WLN_AccessPolicy1	CFG_WLN_(107,CFG_TYPE_INT)
#define CFG_WLN_AccessPolicy2	CFG_WLN_(108,CFG_TYPE_INT)
#define CFG_WLN_AccessPolicy3	CFG_WLN_(109,CFG_TYPE_INT)
#define CFG_WLN_wanConnectionMode	CFG_WLN_(110,CFG_TYPE_STR)
#define CFG_WLN_WapiEnable		CFG_WLN_(111,CFG_TYPE_STR)
#define CFG_WLN_WscModeOption	CFG_WLN_(112,CFG_TYPE_INT)
#define CFG_WLN_WscConfigured	CFG_WLN_(113,CFG_TYPE_INT)
#define CFG_WLN_WscPinCode		CFG_WLN_(114,CFG_TYPE_INT)
#define CFG_WLN_WdsList			CFG_WLN_(115,CFG_TYPE_STR)
#define CFG_WLN_Wds1Key		CFG_WLN_(116,CFG_TYPE_STR)
#define CFG_WLN_Wds2Key		CFG_WLN_(117,CFG_TYPE_STR)
#define CFG_WLN_Wds3Key		CFG_WLN_(118,CFG_TYPE_STR)
#define CFG_WLN_Wds4Key		CFG_WLN_(119,CFG_TYPE_STR)
#define CFG_WLN_WdsDefaultKeyID		CFG_WLN_(120,CFG_TYPE_STR)

#define CFG_WLN_CountryRegion 			CFG_WLN_(121,CFG_TYPE_INT)
#define CFG_WLN_CountryRegionABand		CFG_WLN_(122,CFG_TYPE_INT)
#define CFG_WLN_DisableOLBC				CFG_WLN_(123,CFG_TYPE_INT)
#define CFG_WLN_TxAntenna 				CFG_WLN_(124,CFG_TYPE_INT)
#define CFG_WLN_RxAntenna				CFG_WLN_(125,CFG_TYPE_INT)
#define CFG_WLN_TurboRate				CFG_WLN_(126,CFG_TYPE_INT)
#define CFG_WLN_NoForwarding			CFG_WLN_(127,CFG_TYPE_STR)
#define CFG_WLN_NoForwardingBTNBSSID	CFG_WLN_(128,CFG_TYPE_INT)
#define CFG_WLN_SecurityMode			CFG_WLN_(129,CFG_TYPE_STR)
#define CFG_WLN_VLANEnable				CFG_WLN_(130,CFG_TYPE_INT)
#define CFG_WLN_VLANName				CFG_WLN_(131,CFG_TYPE_STR)
#define CFG_WLN_VLANID					CFG_WLN_(132,CFG_TYPE_INT)
#define CFG_WLN_VLANPriority				CFG_WLN_(133,CFG_TYPE_INT)
#define CFG_WLN_WscConfMode			CFG_WLN_(134,CFG_TYPE_INT)
#define CFG_WLN_WscConfStatus			CFG_WLN_(135,CFG_TYPE_INT)
#define CFG_WLN_WscAKMP				CFG_WLN_(136,CFG_TYPE_INT)
#define CFG_WLN_WscActionIndex			CFG_WLN_(137,CFG_TYPE_INT)
#define CFG_WLN_WscRegResult			CFG_WLN_(138,CFG_TYPE_INT)
#define CFG_WLN_WscUseUPnP			CFG_WLN_(139,CFG_TYPE_INT)
#define CFG_WLN_WscUseUFD				CFG_WLN_(140,CFG_TYPE_INT)
#define CFG_WLN_WscSSID				CFG_WLN_(141,CFG_TYPE_STR)
#define CFG_WLN_WscKeyMGMT			CFG_WLN_(142,CFG_TYPE_STR)
#define CFG_WLN_WscConfigMethod		CFG_WLN_(143,CFG_TYPE_INT)
#define CFG_WLN_WscAuthType			CFG_WLN_(144,CFG_TYPE_INT)
#define CFG_WLN_WscEncrypType			CFG_WLN_(145,CFG_TYPE_INT)
#define CFG_WLN_WscNewKey				CFG_WLN_(146,CFG_TYPE_STR)
#define CFG_WLN_IEEE8021X				CFG_WLN_(147,CFG_TYPE_STR)
#define CFG_WLN_CSPeriod				CFG_WLN_(148,CFG_TYPE_INT)
#define CFG_WLN_PreAuth				CFG_WLN_(149,CFG_TYPE_STR)
#define CFG_WLN_AuthMode				CFG_WLN_(150,CFG_TYPE_STR)
#define CFG_WLN_RekeyInterval			CFG_WLN_(151,CFG_TYPE_STR)
#define CFG_WLN_RekeyMethod			CFG_WLN_(152,CFG_TYPE_STR)
#define CFG_WLN_PMKCachePeriod			CFG_WLN_(153,CFG_TYPE_STR)
#define CFG_WLN_WPAPSK1				CFG_WLN_(154,CFG_TYPE_STR)
#define CFG_WLN_Key1Type				CFG_WLN_(155,CFG_TYPE_STR)
#define CFG_WLN_Key1Str1				CFG_WLN_(156,CFG_TYPE_STR)
#define CFG_WLN_Key2Type				CFG_WLN_(157,CFG_TYPE_STR)
#define CFG_WLN_Key2Str1				CFG_WLN_(158,CFG_TYPE_STR)
#define CFG_WLN_Key3Type				CFG_WLN_(159,CFG_TYPE_STR)
#define CFG_WLN_Key3Str1				CFG_WLN_(160,CFG_TYPE_STR)
#define CFG_WLN_Key4Type				CFG_WLN_(161,CFG_TYPE_STR)
#define CFG_WLN_Key4Str1				CFG_WLN_(162,CFG_TYPE_STR)
#define CFG_WLN_HSCounter				CFG_WLN_(163,CFG_TYPE_INT)
#define CFG_WLN_HT_PROTECT			CFG_WLN_(164,CFG_TYPE_INT)
#define CFG_WLN_HT_MIMOPS				CFG_WLN_(165,CFG_TYPE_INT)
#define CFG_WLN_AccessControlList0		CFG_WLN_(166,CFG_TYPE_STR)
#define CFG_WLN_AccessControlList1		CFG_WLN_(167,CFG_TYPE_STR)
#define CFG_WLN_AccessControlList2		CFG_WLN_(168,CFG_TYPE_STR)
#define CFG_WLN_AccessControlList3		CFG_WLN_(169,CFG_TYPE_STR)
#define CFG_WLN_WirelessEvent			CFG_WLN_(170,CFG_TYPE_INT)
#define CFG_WLN_RADIUS_Acct_Server		CFG_WLN_(171,CFG_TYPE_STR)
#define CFG_WLN_RADIUS_Acct_Port		CFG_WLN_(172,CFG_TYPE_INT)
#define CFG_WLN_RADIUS_Acct_Key		CFG_WLN_(173,CFG_TYPE_INT)
#define CFG_WLN_staWirelessMode			CFG_WLN_(174,CFG_TYPE_INT)
#define CFG_WLN_upnpEnabled			CFG_WLN_(175,CFG_TYPE_INT)
#define CFG_WLN_pppoeREnabled			CFG_WLN_(176,CFG_TYPE_INT)
#define CFG_WLN_RDRegion				CFG_WLN_(177,CFG_TYPE_STR)
#define CFG_WLN_NewVersion				CFG_WLN_(178,CFG_TYPE_INT)
#define CFG_WLN_SSID2					CFG_WLN_(179,CFG_TYPE_STR)
#define CFG_WLN_SSID3					CFG_WLN_(180,CFG_TYPE_STR)
#define CFG_WLN_SSID4					CFG_WLN_(181,CFG_TYPE_STR)
#define CFG_WLN_SSID1					CFG_WLN_(60,CFG_TYPE_STR)
#define CFG_WLN_AutoSSID				CFG_WLN_(252,CFG_TYPE_INT)
#define CFG_WLN_APclientScan            CFG_WLN_(61,CFG_TYPE_STR)
#define CFG_WLN_WPAPSK				CFG_WLN_(183,CFG_TYPE_STR)
#define CFG_WLN_WPAPSK2				CFG_WLN_(184,CFG_TYPE_STR)
#define CFG_WLN_WPAPSK3				CFG_WLN_(185,CFG_TYPE_STR)
#define CFG_WLN_WPAPSK4				CFG_WLN_(186,CFG_TYPE_STR)
#define CFG_WLN_Key1Str					CFG_WLN_(187,CFG_TYPE_STR)
#define CFG_WLN_Key1Str2				CFG_WLN_(188,CFG_TYPE_STR)
#define CFG_WLN_Key1Str3				CFG_WLN_(189,CFG_TYPE_STR)
#define CFG_WLN_Key1Str4				CFG_WLN_(190,CFG_TYPE_STR)
#define CFG_WLN_Key2Str					CFG_WLN_(191,CFG_TYPE_STR)
#define CFG_WLN_Key2Str2				CFG_WLN_(192,CFG_TYPE_STR)
#define CFG_WLN_Key2Str3				CFG_WLN_(193,CFG_TYPE_STR)
#define CFG_WLN_Key2Str4				CFG_WLN_(194,CFG_TYPE_STR)
#define CFG_WLN_Key3Str					CFG_WLN_(195,CFG_TYPE_STR)
#define CFG_WLN_Key3Str2				CFG_WLN_(196,CFG_TYPE_STR)
#define CFG_WLN_Key3Str3				CFG_WLN_(197,CFG_TYPE_STR)
#define CFG_WLN_Key3Str4				CFG_WLN_(198,CFG_TYPE_STR)
#define CFG_WLN_Key4Str					CFG_WLN_(199,CFG_TYPE_STR)
#define CFG_WLN_Key4Str2				CFG_WLN_(200,CFG_TYPE_STR)
#define CFG_WLN_Key4Str3				CFG_WLN_(201,CFG_TYPE_STR)
#define CFG_WLN_Key4Str4				CFG_WLN_(202,CFG_TYPE_STR)
#define CFG_WLN_WapiPskType			CFG_WLN_(203,CFG_TYPE_INT)
#define CFG_WLN_WapiPsk1				CFG_WLN_(204,CFG_TYPE_STR)
#define CFG_WLN_WapiPsk2				CFG_WLN_(205,CFG_TYPE_STR)
#define CFG_WLN_WapiPsk3				CFG_WLN_(206,CFG_TYPE_STR)
#define CFG_WLN_WapiPsk4				CFG_WLN_(207,CFG_TYPE_STR)
#define CFG_WLN_RADIUS_Key1			CFG_WLN_(208,CFG_TYPE_STR)
#define CFG_WLN_RADIUS_Key2			CFG_WLN_(209,CFG_TYPE_STR)
#define CFG_WLN_RADIUS_Key3			CFG_WLN_(210,CFG_TYPE_STR)
#define CFG_WLN_RADIUS_Key4			CFG_WLN_(211,CFG_TYPE_STR)
#define CFG_WLN_ApCliEnable				CFG_WLN_(212,CFG_TYPE_INT)
#define CFG_WLN_ApCliBssid				CFG_WLN_(213,CFG_TYPE_STR)
#define CFG_WLN_ApCliAuthMode			CFG_WLN_(214,CFG_TYPE_STR)
#define CFG_WLN_ApCliEncrypType			CFG_WLN_(215,CFG_TYPE_STR)
#define CFG_WLN_ApCliKey1Type			CFG_WLN_(216,CFG_TYPE_INT)
#define CFG_WLN_ApCliKey1Str			CFG_WLN_(217,CFG_TYPE_STR)
#define CFG_WLN_ApCliKey2Str			CFG_WLN_(218,CFG_TYPE_STR)
#define CFG_WLN_ApCliKey3Str			CFG_WLN_(219,CFG_TYPE_STR)
#define CFG_WLN_ApCliKey4Str			CFG_WLN_(220,CFG_TYPE_STR)
#define CFG_WLN_ApCliDefaultKeyID		CFG_WLN_(221,CFG_TYPE_INT)
#define CFG_WLN_ApCliWPAPSK			CFG_WLN_(222,CFG_TYPE_STR)
#define CFG_WLN_ApCliSsid				CFG_WLN_(223,CFG_TYPE_STR)
#define CFG_WLN_SSIDNum				CFG_WLN_(224,CFG_TYPE_INT)
#define CFG_WLN_WapiKeyType			CFG_WLN_(225,CFG_TYPE_STR)
#define CFG_WLN_WapiKey				CFG_WLN_(226,CFG_TYPE_STR)
#define CFG_WLN_AccessPolicy				CFG_WLN_(227,CFG_TYPE_STR)
#define CFG_WLN_HT_BSSCoexistence		CFG_WLN_(228,CFG_TYPE_INT)
#define CFG_WLN_KeyType				CFG_WLN_(230,CFG_TYPE_STR)
#define CFG_WLN_RadioOn				CFG_WLN_(231,CFG_TYPE_INT)
#define CFG_WLN_IgmpSnEnable			CFG_WLN_(232,CFG_TYPE_INT)
#define CFG_WLN_SSIDIndex				CFG_WLN_(233,CFG_TYPE_INT)
#define CFG_WLN_ApCliKey2Type			CFG_WLN_(234,CFG_TYPE_INT)
#define CFG_WLN_ApCliKey3Type			CFG_WLN_(235,CFG_TYPE_INT)
#define CFG_WLN_ApCliKey4Type			CFG_WLN_(236,CFG_TYPE_INT)
#define CFG_WLN_PMFMFPC				CFG_WLN_(237,CFG_TYPE_INT)
#define CFG_WLN_PMFMFPR				CFG_WLN_(238,CFG_TYPE_INT)
#define CFG_WLN_PMFSHA256				CFG_WLN_(239,CFG_TYPE_INT)
#define CFG_WLN_MACRepeaterEn			CFG_WLN_(240,CFG_TYPE_INT)
#define CFG_WLN_MACRepeaterOuiMode		CFG_WLN_(241,CFG_TYPE_INT)
/* Add by Lea */
#define CFG_WLN_ApCliAutoConnect		CFG_WLN_(242,CFG_TYPE_INT)

// status
#define CFG_STS_(idx,type)  CFG_ID_(CFG_ID_STS,idx,type,0)
#define CFG_STS_FMW_VER	    CFG_STS_(1,CFG_TYPE_VER)
#define CFG_STS_FMW_INFO	CFG_STS_(2,CFG_TYPE_STR)

// Hardware related
#define CFG_HW_(idx,type)   CFG_ID_(CFG_ID_HW,idx,type,0)
#define CFG_HW_VER		    CFG_HW_(1,CFG_TYPE_VER)
#define CFG_HW_SN		    CFG_HW_(2,CFG_TYPE_INT)
#define CFG_HW_NAME		    CFG_HW_(3,CFG_TYPE_STR)
#define CFG_HW_PARM		    CFG_HW_(4,CFG_TYPE_STR)
#define CFG_HW_LAN_MAC		CFG_HW_(5,CFG_TYPE_MAC)
#define CFG_HW_WAN_MAC		CFG_HW_(6,CFG_TYPE_MAC)
#define CFG_HW_WLN_MAC		CFG_HW_(7,CFG_TYPE_MAC)
#define CFG_HW_MAC		    CFG_HW_(5,CFG_TYPE_MAC)
#define CFG_HW_BOT_VER	    CFG_HW_(8,CFG_TYPE_VER)
#define CFG_HW_APP	        CFG_HW_(33,CFG_TYPE_INT)
#define CFG_HW_VLAN	        CFG_HW_(34,CFG_TYPE_INT)
#define CFG_HW_INTF	        CFG_HW_(35,CFG_TYPE_INT)
#define CFG_HW_RUN_LOC	    CFG_HW_(36,CFG_TYPE_INT)
#define CFG_HW_IMG_SZ	    CFG_HW_(37,CFG_TYPE_INT)
#define CFG_HW_IP		    CFG_HW_(38,CFG_TYPE_IP)
#define CFG_HW_MSK		    CFG_HW_(39,CFG_TYPE_IP)
#define CFG_HW_GW		    CFG_HW_(40,CFG_TYPE_IP)
#define CFG_HW_SVR		    CFG_HW_(41,CFG_TYPE_IP)
#define CFG_HW_IMG_LOC	    CFG_HW_(42,CFG_TYPE_INT)
#define CFG_HW_CFG_LOC	    CFG_HW_(43,CFG_TYPE_INT)
#define CFG_HW_CFG_SZ	    CFG_HW_(44,CFG_TYPE_INT)

#define CFG_HW_WLN_GAIN     CFG_HW_(64,CFG_TYPE_INT)

//syslog
#define CFG_LOG_(idx,type)  CFG_ID_(CFG_ID_LOG,idx,type,0)
#define CFG_LOG_RM_EN		CFG_LOG_(1,CFG_TYPE_INT)
#define CFG_LOG_RM_TYPE		CFG_LOG_(2,CFG_TYPE_INT)
#define CFG_LOG_SYS_IP		CFG_LOG_(3,CFG_TYPE_IP)
#define CFG_LOG_MAIL_IP		CFG_LOG_(4,CFG_TYPE_STR)
#define CFG_LOG_E_MAIL		CFG_LOG_(5,CFG_TYPE_STR)
#define CFG_LOG_MODE		CFG_LOG_(6,CFG_TYPE_INT)


// SoC
#define CFG_SOC_(idx,type)		CFG_ID_(CFG_ID_SOC,idx,type,0)
#define	CFG_SOC_WDG				CFG_SOC_(1,CFG_TYPE_INT)

/*  Headden value  */
#define CFG_SYS_FMW_IDSTR	CFG_SOC_(100,CFG_TYPE_STR)


// the result code
#define CFG_OK          0   // normal access
#define CFG_NFOUND      -1  // not existing obj
#define CFG_ETYPE       -2  // not support type
#define CFG_ERANGE      -3  // not in the range
#define CFG_EIDSTR	    -4  // not the same ID string
#define CFG_INSERT      1   // new value insert 
#define CFG_CHANGE      2   // new value overwrite the older one

#define	CFG_NORMAL		0	// normal case
#define	CFG_NO_EVENT	1	// don't send event to mon task
#define	CFG_DELAY_EVENT	2	// delay to send the event
#define	CFG_COM_SAVE	4	// save immediately

#define	CFG_OBJ_SIZE	16384

typedef unsigned int cfgid ;

struct cfgid_str_t
{
    int id;
    char * str;
};

extern struct cfgid_str_t cfgid_str_table[];
extern int cfgid_str_table_sz;
extern char cfg_data[CFG_OBJ_SIZE];

#define CFG_ID_STR_NUM  cfgid_str_table_sz

#endif /* CFG_ID_H */


