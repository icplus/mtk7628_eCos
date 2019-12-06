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
    cfg_api.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/
#ifndef	CGI_API_H
#define	CGI_API_H
//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <config.h>
#include <sys_status.h>
#include <version.h>
#include <eventlog.h>

#ifdef CONFIG_CGI_USERS
#include <cgi_aux.h>
#endif

extern char *config_cust_ver_str_internal;

//==============================================================================
//                               MACROS PORTED FROM APUI
//==============================================================================

typedef unsigned long long		UINT64;
typedef int					INT32;
typedef long long 				INT64;

typedef unsigned char			UCHAR;
typedef unsigned short			USHORT;
typedef unsigned int			UINT;
typedef unsigned long			ULONG;

typedef unsigned char *		PUINT8;
typedef unsigned short *		PUINT16;
typedef unsigned int *			PUINT32;
typedef unsigned long long *	PUINT64;
typedef int *					PINT32;
typedef long long * 			PINT64;
typedef char 					STRING;
typedef signed char			CHAR;

typedef signed short			SHORT;
typedef signed int				INT;
typedef signed long			LONG;
typedef signed long long		LONGLONG;	

typedef unsigned long long		ULONGLONG;
 
typedef unsigned char			BOOLEAN;
typedef void					VOID;

typedef char *				PSTRING;
typedef VOID *				PVOID;
typedef CHAR *				PCHAR;
typedef UCHAR * 				PUCHAR;
typedef USHORT *				PUSHORT;
typedef LONG *				PLONG;
typedef ULONG *				PULONG;
typedef UINT *				PUINT;

typedef union _LARGE_INTEGER {
    struct {
#ifdef RT_BIG_ENDIAN
        INT32 HighPart;
        UINT LowPart;
#else
        UINT LowPart;
        INT32 HighPart;
#endif
    } u;
    INT64 QuadPart;
} LARGE_INTEGER;

#define MODE_CCK			0
#define MODE_OFDM			1
#define MODE_HTMIX			2
#define MODE_HTGREENFIELD	3

#define RT_OID_QUERY_DRIVER_VERSION				(0x0D010781 & 0xffff)
#define RT_OID_QUERY_RFIC_TYPE					(0x0D010782 & 0xffff)

//#define 	SIOCIWFIRSTPRIV 							0x00
#define SIOCIWFIRSTPRIV									0x8BE0

#define 	RT_PRIV_IOCTL								(SIOCIWFIRSTPRIV + 0x01)
#define 	RTPRIV_IOCTL_SET							(SIOCIWFIRSTPRIV + 0x02)
#define 	RTPRIV_IOCTL_BBP                            			(SIOCIWFIRSTPRIV + 0x03)
#define 	RTPRIV_IOCTL_MAC                            			(SIOCIWFIRSTPRIV + 0x05)
#define 	RTPRIV_IOCTL_E2P                            			(SIOCIWFIRSTPRIV + 0x07)
#define 	RTPRIV_IOCTL_GET_MAC_TABLE				(SIOCIWFIRSTPRIV + 0x0F)
#define 	RTPRIV_IOCTL_SHOW						(SIOCIWFIRSTPRIV + 0x11)
#define	RTPRIV_IOCTL_WSC_PROFILE                   	 	(SIOCIWFIRSTPRIV + 0x12)
#define 	RTPRIV_IOCTL_RF                             			(SIOCIWFIRSTPRIV + 0x13)
//get nintendo all entry table
#define RTPRIV_IOCTL_NINTENDO_GET_TABLE			(SIOCIWFIRSTPRIV + 0x14)
//set nintendo ds login
#define RTPRIV_IOCTL_NINTENDO_SET_TABLE			(SIOCIWFIRSTPRIV + 0x15)
#define RTPRIV_IOCTL_QUERY_BATABLE				(SIOCIWFIRSTPRIV + 0x16)

#define	OID_802_11_NETWORK_TYPES_SUPPORTED	0x0103

#define RT_QUERY_ATE_TXDONE_COUNT   				0x0401
#define SIOCGIWSTATS                						0x8B0F
#define SIOCGIWAPLIST               						0x8B17
#define SIOCSIWSCAN                 						0x8B18
#define SIOCGIWSCAN                 						0x8B19

#define OID_GET_SET_TOGGLE						0x8000
#define	RT_OID_VERSION_INFO						0x0608

#define RT_OID_WSC_QUERY_STATUS					0x0751
#define RT_OID_WSC_PIN_CODE						0x0752
#define RT_OID_WSC_UUID							0x0753
#define RT_OID_WSC_CONF_STATUS					0x0761
#define RT_SET_DEL_MAC_ENTRY						0x0406
#define	OID_802_11_STATISTICS						0x060E

#define RFIC_2820                   1       	/* 2.4G 2T3R */
#define RFIC_2850                   2       	/* 2.4G/5G 2T3R */
#define RFIC_2720                   3       	/* 2.4G 1T2R */
#define RFIC_2750                   4       	/* 2.4G/5G 1T2R */
#define RFIC_3020                   5       	/* 2.4G 1T1R */
#define RFIC_2020                   6       	/* 2.4G B/G */
#define RFIC_3021                   7       	/* 2.4G 1T2R */
#define RFIC_3022                   8       	/* 2.4G 2T2R */
#define RFIC_3052                   9       	/* 2.4G/5G 2T2R */
#define RFIC_2853		    10	  	/* 2.4G.5G 3T3R */
#define RFIC_3320                   11    	/* 2.4G 1T1R with PA (RT3350/RT3370/RT3390) */
#define RFIC_3322                   12      	/* 2.4G 2T2R with PA (RT3352/RT3371/RT3372/RT3391/RT3392) */
#define RFIC_3053                   13      	/* 2.4G/5G 3T3R (RT3883/RT3563/RT3573/RT3593/RT3662) */
#define RFIC_3853                   13      	/* 2.4G/5G 3T3R (RT3883/RT3563/RT3573/RT3593/RT3662) */
#define RFIC_UNKNOWN		0xff

// Wsc status code
#define	STATUS_WSC_NOTUSED						0
#define	STATUS_WSC_IDLE							1
#define 	STATUS_WSC_FAIL        			        		2		// WSC Process Fail
#define	STATUS_WSC_LINK_UP						3		// Start WSC Process
#define	STATUS_WSC_EAPOL_START_RECEIVED		4		// Received EAPOL-Start
#define	STATUS_WSC_EAP_REQ_ID_SENT				5		// Sending EAP-Req(ID)
#define	STATUS_WSC_EAP_RSP_ID_RECEIVED			6		// Receive EAP-Rsp(ID)
#define	STATUS_WSC_EAP_RSP_WRONG_SMI			7		// Receive EAP-Req with wrong WSC SMI Vendor Id
#define	STATUS_WSC_EAP_RSP_WRONG_VENDOR_TYPE	8	// Receive EAPReq with wrong WSC Vendor Type
#define	STATUS_WSC_EAP_REQ_WSC_START			9		// Sending EAP-Req(WSC_START)
#define	STATUS_WSC_EAP_M1_SENT					10		// Send M1
#define	STATUS_WSC_EAP_M1_RECEIVED				11		// Received M1
#define	STATUS_WSC_EAP_M2_SENT					12		// Send M2
#define	STATUS_WSC_EAP_M2_RECEIVED				13		// Received M2
#define	STATUS_WSC_EAP_M2D_RECEIVED			14		// Received M2D
#define	STATUS_WSC_EAP_M3_SENT					15		// Send M3
#define	STATUS_WSC_EAP_M3_RECEIVED				16		// Received M3
#define	STATUS_WSC_EAP_M4_SENT					17		// Send M4
#define	STATUS_WSC_EAP_M4_RECEIVED				18		// Received M4
#define	STATUS_WSC_EAP_M5_SENT					19		// Send M5
#define	STATUS_WSC_EAP_M5_RECEIVED				20		// Received M5
#define	STATUS_WSC_EAP_M6_SENT					21		// Send M6
#define	STATUS_WSC_EAP_M6_RECEIVED				22		// Received M6
#define	STATUS_WSC_EAP_M7_SENT					23		// Send M7
#define	STATUS_WSC_EAP_M7_RECEIVED				24		// Received M7
#define	STATUS_WSC_EAP_M8_SENT					25		// Send M8
#define	STATUS_WSC_EAP_M8_RECEIVED				26		// Received M8
#define	STATUS_WSC_EAP_RAP_RSP_ACK				27		// Processing EAP Response (ACK)
#define	STATUS_WSC_EAP_RAP_REQ_DONE_SENT		28		// Processing EAP Request (Done)
#define	STATUS_WSC_EAP_RAP_RSP_DONE_SENT		29		// Processing EAP Response (Done)
#define 	STATUS_WSC_EAP_FAIL_SENT               		30      	// Sending EAP-Fail
#define 	STATUS_WSC_ERROR_HASH_FAIL              		31      	// WSC_ERROR_HASH_FAIL
#define 	STATUS_WSC_ERROR_HMAC_FAIL              		32      	// WSC_ERROR_HMAC_FAIL
#define 	STATUS_WSC_ERROR_DEV_PWD_AUTH_FAIL     33     	// WSC_ERROR_DEV_PWD_AUTH_FAIL
#define 	STATUS_WSC_CONFIGURED					34      	// Finish WSC process
#define	STATUS_WSC_SCAN_AP						35		// Scanning AP
#define	STATUS_WSC_EAPOL_START_SENT			36
#define 	STATUS_WSC_EAP_RSP_DONE_SENT			37

#define MAC_ADDR_LEN 								6
#define MAC_ADDR_LENGTH                 					6
#define MAX_NUMBER_OF_MAC						75 		// if MAX_MBSSID_NUM is 8, this value can't be larger than 211

//==============================================================================
//                               Definitions PORTED FROM APUI
//==============================================================================

typedef struct  _WSC_CONFIGURED_VALUE {
	USHORT WscConfigured; // 1 un-configured; 2 configured
	UCHAR	WscSsid[32 + 1];
	USHORT WscAuthMode;	// mandatory, 0x01: open, 0x02: wpa-psk, 0x04: shared, 0x08:wpa, 0x10: wpa2, 0x20: wpa2-psk
	USHORT	WscEncrypType;	// 0x01: none, 0x02: wep, 0x04: tkip, 0x08: aes
	UCHAR	DefaultKeyIdx;
	UCHAR	WscWPAKey[64 + 1];
} WSC_CONFIGURED_VALUE;

typedef struct
{
	const char*	cmd;					// Input command string
	int	(*func)(int argc, char* argv[]);
	const char*	msg;					// Help message
} COMMAND_TABLE;

struct iw_point
{
	PVOID		pointer;
	USHORT		length;
	USHORT		flags;
};
	
union iwreq_data
{
	struct iw_point data;
};

struct iwreq 
{
	union
	{
		char    ifrn_name[IFNAMSIZ];    /* if name, e.g. "eth0" */
	} ifr_ifrn;
	
	union   	iwreq_data      u;
};

typedef struct multi_ssid_s {
	int		enable;
	char		SSID[33];
	int		SecurityMode;
	char		AuthMode[14];
	char		EncrypType[8];
	char		WPAPSK[65];
	int		DefaultKeyID;
	int		Key1Type;
	char		Key1Str[27];
	int		Key2Type;
	char		Key2Str[27];
	int		Key3Type;
	char		Key3Str[27];
	int		Key4Type;
	char		Key4Str[27];
	int		IEEE8021X;
	int		WapiKeyType;
	char		WapiKey[65];
	int		TxRate;
	int		HideSSID;
	int		PreAuth;
	char		RekeyMethod[8];
	int		RekeyInterval;
	int		PMKCachePeriod;
	int		WscModeOption;
	int		WscConfStatus;
	char		VlanName[20];
	int		VlanId;
	int		VlanPriority;
	char		radius_server[16];
	int		radius_port;
	char		radius_secret[65];
}multi_ssid_s;

typedef struct wds_tbl_s {
	int		enable;
	char	PeerMac[18];
	char	PhyMode[11];
	char	EncrypType[5];
	int		DefaultKeyID;
	char	Key[65];
}wds_tbl_s;

typedef struct ft_tbl_s {
	int		FtSupport;
	int		FtRic;
	int		FtOtd;
	char	FtMdId[3];
	char	FtR0khId[49];
	int		FtKeyLifeTime;
	int		FtReassociateDealine;
}ft_tbl_s;

// MIMO Tx parameter, ShortGI, MCS, STBC, etc.  these are fields in TXWI. Don't change this definition!!!
typedef union  _MACHTTRANSMIT_SETTING {
#ifdef RT_BIG_ENDIAN
	struct	{
	unsigned short		MODE:2;		// Use definition MODE_xxx.  
	unsigned short		rsv:3;
	unsigned short		STBC:2;		//SPACE 
	unsigned short		ShortGI:1;
	unsigned short		BW:1;		//channel bandwidth 20MHz or 40 MHz
	unsigned short   		MCS:7; 		// MCS
	}	field;
#else
	struct	{
	unsigned short   		MCS:7;         	// MCS
	unsigned short		BW:1;		//channel bandwidth 20MHz or 40 MHz
	unsigned short		ShortGI:1;
	unsigned short		STBC:2;		//SPACE 
	unsigned short		rsv:3;
	unsigned short		MODE:2;		// Use definition MODE_xxx.  
	}	field;
#endif
	unsigned short		word;
 } MACHTTRANSMIT_SETTING, *PMACHTTRANSMIT_SETTING;
typedef union _HTTRANSMIT_SETTING {
#ifdef RT_BIG_ENDIAN
	struct {
		USHORT MODE:3;	/* Use definition MODE_xxx. */
		USHORT iTxBF:1;
		USHORT eTxBF:1;
		USHORT STBC:1;	/* only support in HT/VHT mode with MCS0~7 */
		USHORT ShortGI:1;
		USHORT BW:2;	/* channel bandwidth 20MHz/40/80 MHz */
		USHORT ldpc:1;
		USHORT MCS:6;	/* MCS */
	} field;
#else
	struct {
		USHORT MCS:6;
		USHORT ldpc:1;
		USHORT BW:2;
		USHORT ShortGI:1;
		USHORT STBC:1;
		USHORT eTxBF:1;
		USHORT iTxBF:1;
		USHORT MODE:3;
	} field;
#endif
	USHORT word;
} HTTRANSMIT_SETTING, *PHTTRANSMIT_SETTING;

typedef struct _RT_802_11_MAC_ENTRY {
	UCHAR 		ApIdx;
	UCHAR       	Addr[MAC_ADDR_LENGTH];
	UCHAR       	Aid;
	UCHAR       	Psm;     // 0:PWR_ACTIVE, 1:PWR_SAVE
	UCHAR		MimoPs;  // 0:MMPS_STATIC, 1:MMPS_DYNAMIC, 3:MMPS_Enabled
	CHAR		AvgRssi0;
	CHAR		AvgRssi1;
	CHAR		AvgRssi2;
	UINT32		ConnectedTime;
#ifdef MT7628_ASIC_SUPPORT
	MACHTTRANSMIT_SETTING	TxRate;
#else
    HTTRANSMIT_SETTING      TxRate;
#endif
	UINT32		LastRxRate;
	SHORT		StreamSnr[3];
	SHORT		SoundingRespSnr[3];
} RT_802_11_MAC_ENTRY, *PRT_802_11_MAC_ENTRY;

typedef struct _RT_802_11_MAC_TABLE {
    ULONG       Num;
    RT_802_11_MAC_ENTRY Entry[MAX_NUMBER_OF_MAC];
} RT_802_11_MAC_TABLE, *PRT_802_11_MAC_TABLE;

//For QureyBATableOID use;
typedef enum _REC_BLOCKACK_STATUS
{	
    Recipient_NONE=0,
	Recipient_USED,
	Recipient_HandleRes,
    Recipient_Accept
} REC_BLOCKACK_STATUS, *PREC_BLOCKACK_STATUS;

typedef enum _ORI_BLOCKACK_STATUS
{
    Originator_NONE=0,
	Originator_USED,
    Originator_WaitRes,
    Originator_Done
} ORI_BLOCKACK_STATUS, *PORI_BLOCKACK_STATUS;

//For QureyBATableOID use;
typedef struct  _OID_BA_REC_ENTRY{
	UCHAR   MACAddr[MAC_ADDR_LEN];
	UCHAR   BaBitmap;   // if (BaBitmap&(1<<TID)), this session with{MACAddr, TID}exists, so read BufSize[TID] for BufferSize
	UCHAR   rsv; 
	UCHAR   BufSize[8];
	REC_BLOCKACK_STATUS	REC_BA_Status[8];
} OID_BA_REC_ENTRY, *POID_BA_REC_ENTRY;

//For QureyBATableOID use;
typedef struct  _OID_BA_ORI_ENTRY{
	UCHAR   MACAddr[MAC_ADDR_LEN];
	UCHAR   BaBitmap;  // if (BaBitmap&(1<<TID)), this session with{MACAddr, TID}exists, so read BufSize[TID] for BufferSize, read ORI_BA_Status[TID] for status
	UCHAR   rsv; 
	UCHAR   BufSize[8];
	ORI_BLOCKACK_STATUS  ORI_BA_Status[8];
} OID_BA_ORI_ENTRY, *POID_BA_ORI_ENTRY;

typedef struct _QUERYBA_TABLE{
	OID_BA_ORI_ENTRY       BAOriEntry[32];
	OID_BA_REC_ENTRY       BARecEntry[32];
	UCHAR   OriNum;// Number of below BAOriEntry
	UCHAR   RecNum;// Number of below BARecEntry
} QUERYBA_TABLE, *PQUERYBA_TABLE;

typedef struct _NDIS_802_11_STATISTICS
{
   ULONG           Length;             // Length of structure
   LARGE_INTEGER   TransmittedFragmentCount;
   LARGE_INTEGER   MulticastTransmittedFrameCount;
   LARGE_INTEGER   FailedCount;
   LARGE_INTEGER   RetryCount;
   LARGE_INTEGER   MultipleRetryCount;
   LARGE_INTEGER   RTSSuccessCount;
   LARGE_INTEGER   RTSFailureCount;
   LARGE_INTEGER   ACKFailureCount;
   LARGE_INTEGER   FrameDuplicateCount;
   LARGE_INTEGER   ReceivedFragmentCount;
   LARGE_INTEGER   MulticastReceivedFrameCount;
   LARGE_INTEGER   FCSErrorCount;
   LARGE_INTEGER   TransmittedFrameCount;
   LARGE_INTEGER   WEPUndecryptableCount;       
   LARGE_INTEGER   TKIPLocalMICFailures;
   LARGE_INTEGER   TKIPRemoteMICErrors;
   LARGE_INTEGER   TKIPICVErrors;
   LARGE_INTEGER   TKIPCounterMeasuresInvoked;
   LARGE_INTEGER   TKIPReplays;
   LARGE_INTEGER   CCMPFormatErrors;
   LARGE_INTEGER   CCMPReplays;
   LARGE_INTEGER   CCMPDecryptErrors;
   LARGE_INTEGER   FourWayHandshakeFailures;   
} NDIS_802_11_STATISTICS, *PNDIS_802_11_STATISTICS;

//==============================================================================
//                                    MACROS
//==============================================================================
#define	CGI_MAP(name,id)	CGI_var_map(req,#name,id)
#define	CGI_CFG_ID(id)		WEB_printf(req,"%u",id)
#define	CGI_CFG_GET(id)		CGI_get_cfg(req,id)
#define CGI_GET_LM(id)		CGI_get_LM(req,id)
#define	CGI_QUERY(var)		CGI_put_var(req,var)
#define	CGI_GET_RTE() 		CGI_show_routing_table(req)
#if MODULE_UPNP
#define	CGI_GET_UPP_PMAP()	CGI_show_upnp_pmap(req)
#else
#define	CGI_GET_UPP_PMAP()
#endif

#if MODULE_DHCPS
#define	CGI_GET_DHC_LIST()	CGI_show_dhcpd_clist(req)
#define	CGI_GET_DHC_DYN_LIST()	CGI_show_dhcpd_dyn_clist(req)
#else
#define	CGI_GET_DHC_LIST()
#define	CGI_GET_DHC_DYN_LIST()
#endif

#define	CGI_MAP_ARRAY(name,id,num) 	CGI_var_map_array(req,#name,id,num)
#define	CGI_GET_CLN_MAC()	WEB_printf(req, "'%s'", CGI_get_client_mac(req))
#define	CGI_GET_SYSLOG()	CGI_print_log(req, SYSTEM_LOG)
#define CGI_GET_SYSLOG_LIST() CGI_print_log_list(req, SYSTEM_LOG)
#define	CGI_GET_SECURELOG()	CGI_print_log(req, SECURE_LOG)
#define CGI_GET_SECURELOG_LIST() CGI_print_log_list(req, SECURE_LOG)

#define CGI_HTTP_LOGOUT()	HTTP_logout(req)
#define	CGI_WAN_STAT()		CGI_get_wan_state(req,1)
#define	CGI_WAN_IP()		CGI_get_wan_state(req,2)
#define	CGI_WAN_MSK()		CGI_get_wan_state(req,3)
#define	CGI_WAN_GW()		CGI_get_wan_state(req,4)
#define	CGI_DNS1()			CGI_get_wan_state(req,5)
#define	CGI_DNS2()			CGI_get_wan_state(req,6)
#define	CGI_DOMAIN()		CGI_get_wan_state(req,8)
#define	CGI_WAN_CONN_TIME()	CGI_get_wan_state(req,9)
#define	CGI_WAN_MAC()		CGI_get_wan_state(req,7 )
#define CGI_TIME()			CGI_get_time(req)
#define CGI_UPTIME()		CGI_get_uptime(req)
#define CGI_DHCPCRMTIME()	CGI_get_wan_state(req,10 )
#define CGI_GET_NTPIFO()    CGI_get_ntpinfo(req)
#define	CGI_CUST_VER_STR()	WEB_printf(req, "%s", sys_cust_ver)
#define CGI_CUST_VER_STR_INTERNAL() WEB_printf(req, "%s", config_cust_ver_str_internal)
#define	CGI_FMW_BUILD_SEC()	WEB_printf(req, "%u", sys_build_time)
#define	CGI_CUST_HD_VER_STR()	WEB_printf(req, "%s", sys_hd_cust_ver)
#define	CGI_DEVICE_NAME()	WEB_printf(req, "%s", sys_device_name)

//#include <sys_status.h>
//#define	CGI_LAN_MAC()		{ char * buf = ESTR(SYS_lan_mac); WEB_printf(req, buf ); }
#define	CGI_DHCP_CLIENTS() 	CGI_show_dhcpd_leases_num(req)
#define	CGI_DO_CMD()		WEB_printf(req, "%d", CGI_do_cmd(req))
#define	CGI_RESULT()		WEB_printf(req, "%d", req->result)
#define	CGI_DO_UPGRADE()	WEB_printf(req, "%d", CGI_upgrade(req))

#define	CGI_GET_IF_CNT(ifn,id)	{ WEB_printf(req, "%u", netif_get_cnt(ifn, id)); }

#define CGI_GET_WLN_MAC_ADDRESS() 				CGI_get_wlan_mac_address(req)
#define CGI_GET_WLN_MAC_ADDRESS1() 				CGI_get_wlan_mac_address1(req)
#define CGI_GET_WLN_11A_CHN_LIST() 				CGI_get_wlan_11a_channel_list(req)
#define CGI_GET_WLN_11B_CHN_LIST() 				CGI_get_wlan_11b_channel_list(req)
#define CGI_GET_WLN_11G_CHN_LIST() 				CGI_get_wlan_11g_channel_list(req)
#define CGI_WLN_Channel()							CGI_get_wlan_channel(req)
#define CGI_GET_WLN_WSC_CONFIG_STATE() 			CGI_get_wlan_wsc_config_state(req)
#define CGI_GET_WLN_SSID_NUM()					CGI_get_wlan_ssid_num(req)
#define CGI_GET_WLN_WDS_PHY_MODE()				CGI_get_wlan_wds_phy_mode(req)
#define CGI_GET_WLN_WDS_DEFAULT_KEY()			CGI_get_wlan_wds_default_key(req)
#define CGI_GET_WLN_WDS_ENCRPT_KEY()				CGI_get_wlan_wds_encryp_key(req)
#define CGI_WLN_WIFI_APCLI_INCLUDE()				CGI_get_wlan_apcli_include(req)
#define CGI_GET_WLN_ALL_BSS_TKIP_OR_WEP()		CGI_get_wlan_all_bss_tkip_or_wep(req)
#define CGI_WLN_HT_STA_ASSOCIATE_LIST()			CGI_get_wlan_ht_sta_associate_list(req)
#define CGI_WLN_HT_BA_ORI_STA_INFO()				CGI_get_wlan_ht_ba_ori_sta_info(req)
#define CGI_WLN_HT_BA_REC_STA_INFO()				CGI_get_wlan_ht_ba_rec_sta_info(req)
#define CGI_WLN_WSC_USE_PROXY()					CGI_get_wlan_wsc_use_proxy(req)
#define CGI_WLN_WSC_USE_INTERNAL_REG()			CGI_get_wlan_wsc_use_internal_reg(req)
#define CGI_WLN_WSC_PIN_CODE()					CGI_get_wlan_wsc_pin_code(req)
#define CGI_WLN_WSC_UUID()							CGI_get_wlan_wsc_uuid(req)
#define CGI_WLN_WSC_DEV_PIN_CODE()				CGI_get_wlan_wsc_dev_pin_code(req)
#define CGI_WLN_WSC_CURRENT_STATUS()			CGI_get_wlan_wsc_current_status(req)
#define CGI_WLN_WSC_ENCR_TYPE()					CGI_get_wlan_wsc_encr_type(req)
#define CGI_WLN_WSC_AUTH_MODE()					CGI_get_wlan_wsc_auth_mode(req)
#define CGI_WLN_WSC_SSID()							CGI_get_wlan_wsc_ssid(req)
#define CGI_WLN_WSC_PASS_PHRASE()				CGI_get_wlan_wsc_pass_phrase(req)
#define CGI_WLN_WPS_PROCESS_STATUS()				CGI_get_wlan_wps_process_status(req)
#define CGI_GET_WLN_MAIN_SSID_FOR_OVERVIEW()	CGI_get_wlan_mainssid_for_overview(req)
#define CGI_GET_WLN_MULTI_SSID1_FOR_OVERVIEW()	CGI_get_wlan_multissid1_for_overview(req)
#define CGI_GET_WLN_MULTI_SSID2_FOR_OVERVIEW()	CGI_get_wlan_multissid2_for_overview(req)
#define CGI_GET_WLN_MULTI_SSID3_FOR_OVERVIEW()	CGI_get_wlan_multissid3_for_overview(req)
#define CGI_GET_WLN_DRIVER_VERSION()				CGI_get_wlan_driver_version(req)
#define CGI_GET_WLN_RC_TYPE()						CGI_get_wlan_rc_type(req)
#define CGI_GET_WLN_CURRENT_WIRELESS_MODE()	CGI_get_wlan_current_wireless_mode(req)
#define CGI_GET_WLN_CURRENT_CHANNEL()			CGI_get_wlan_current_channel(req)
#define CGI_GET_WLN_CURRENT_BROADCAST_SSID()	CGI_get_wlan_current_broadcast_ssid(req)
#define CGI_WLN_APCLIENT_BSSID()					CGI_get_wlan_apclient_bssid(req)
#define CGI_WLN_APCLIENT_AUTH()					CGI_get_wlan_apclient_auth(req)
#define CGI_WLN_APCLIENT_ENCRYP_TYPE()			CGI_get_wlan_apclient_encryp_type(req)
#define CGI_WLN_APCLIENT_WEP_KEY_TYPE()			CGI_get_wlan_apclient_wep_key_type(req)
#define CGI_WLN_APCLIENT_WEP_KEY1()				CGI_get_wlan_apclient_wep_key1(req)
#define CGI_WLN_APCLIENT_WEP_KEY2()				CGI_get_wlan_apclient_wep_key2(req)
#define CGI_WLN_APCLIENT_WEP_KEY3()				CGI_get_wlan_apclient_wep_key3(req)
#define CGI_WLN_APCLIENT_WEP_KEY4()				CGI_get_wlan_apclient_wep_key4(req)
#define CGI_WLN_APCLIENT_WEP_DEFAULT_KEY()		CGI_get_wlan_apclient_default_key(req)
#define CGI_WLN_APCLIENT_WPAPSK()					CGI_get_wlan_apclient_wpapsk(req)
#define CGI_WLN_APCLIENT_SSID()					CGI_get_wlan_apclient_ssid(req)
#define CGI_WLN_WAN_CONNECTION_MODE()			CGI_get_wlan_wan_connection_mode(req)
#define CGI_WLN_SSID_NUM()							CGI_get_wlan_ssid_num(req)
#define CGI_WLN_STATION_ASSOCIATE_LIST()			CGI_get_wlan_station_associate_list(req)
#define CGI_GET_WLN_CURRENT_MAC_ADDRESS()		CGI_get_wlan_current_mac_address(req)
#define CGI_GET_WLN_MCS_LIST()						CGI_get_wlan_mcs_list(req)
#define CGI_GET_WLN_ANTENNA_TYPE()				CGI_get_wlan_antenna_type(req)
#define CGI_GET_WLN_TX_STREAM_LIST()				CGI_get_wlan_tx_stream_list(req)
#define CGI_GET_WLN_RX_STREAM_LIST()				CGI_get_wlan_rx_stream_list(req)


#define	CGI_GET_IF_STAT(ifn,id) { char buf[32]; netif_get_stat(ifn,buf,id); WEB_printf(req, buf ); }
#define	CGI_WLN_MAC()
#define 	CGI_WLN_SSID()
#define 	CGI_WLN_CHN()
#define 	CGI_WLN_WEPEN()
#define	CGI_WLN_TXBAD()
#define	CGI_WLN_RXBAD()
#define	CGI_WLN_TXB()
#define	CGI_WLN_RXB()


#define	CGI_LAN_MAC()			CGI_GET_IF_STAT(LAN_NAME,3)
#define	CGI_LAN_TXOK()			CGI_GET_IF_CNT(LAN_NAME,1)
#define	CGI_LAN_TXBAD()		CGI_GET_IF_CNT(LAN_NAME,2)
#define	CGI_LAN_RXOK()			CGI_GET_IF_CNT(LAN_NAME,3)
#define	CGI_LAN_RXBAD()		CGI_GET_IF_CNT(LAN_NAME,4)
#define	CGI_LAN_TXB()			CGI_GET_IF_CNT(LAN_NAME,5)
#define	CGI_LAN_RXB()			CGI_GET_IF_CNT(LAN_NAME,6)
#define	CGI_WLN_TXOK()	      	CGI_GET_IF_CNT(LAN_NAME,7)
#define	CGI_WLN_RXOK()	      	CGI_GET_IF_CNT(LAN_NAME,8)
#define	CGI_LAN_IP()			CGI_GET_IF_STAT(LAN_NAME,0)
#define	CGI_LAN_MSK()			CGI_GET_IF_STAT(LAN_NAME,1)
#define	CGI_LAN_GW()			CGI_GET_IF_STAT(LAN_NAME,2)
#define	CGI_WAN_DEF_MAC()	{ char mac[6]; CFG_get_mac(1, mac);  WEB_printf(req, "'%s'", ether_ntoa(mac) ); }

#define	CGI_WAN_TXOK()		CGI_GET_IF_CNT(WAN_NAME,1)
#define	CGI_WAN_TXBAD()		CGI_GET_IF_CNT(WAN_NAME,2)
#define	CGI_WAN_RXOK()		CGI_GET_IF_CNT(WAN_NAME,3)
#define	CGI_WAN_RXBAD()		CGI_GET_IF_CNT(WAN_NAME,4)
#define	CGI_WAN_TXB()			CGI_GET_IF_CNT(WAN_NAME,5)
#define	CGI_WAN_RXB()			CGI_GET_IF_CNT(WAN_NAME,6)

#ifdef	CONFIG_WP322X
#define CGI_ETH_VCT()			CGI_do_ether_VCT(req)
#else
#define CGI_ETH_VCT()			WEB_printf(req, "5,5,5,5,5")
#endif

#if	MODULE_VPN
#define CGI_GET_VPN_STATUS()	CGI_get_vpn_status(req)
#endif

#define CGI_GET_NAT_BRIF()		CGI_get_nat_session_brief(req)
#define CGI_GET_NAT_LIST()		CGI_get_nat_session_list(req)

#define CGI_GET_DDNS_ORAY_LIST(name)	CGI_get_ddns_oray_list(req, name)

#ifdef CONFIG_DUALWAN
#define CGI_DWAN_IP()			CGI_get_dwan(req)
#endif

enum {
		CGI_RC_OK= 0,
		CGI_RC_CFG_OK=1,
		CGI_RC_PING_OK=2,
		CGI_RC_REBOOT=3,
		CGI_RC_SNDMAIL=4,
		CGI_RC_UPGRADE=5,
		CGI_RC_FACTORY_DEFAULT=6,
		CGI_RC_CONNECTING=7,
		CGI_RC_ERR=-1,
		CGI_RC_CFG_ERR=-2,
		CGI_RC_CFG_RANG_ERR=-3,
		CGI_RC_PING_TIMEOUT=-4,
		CGI_RC_PRIVILEGE_ERR=-5,
		CGI_RC_FILE_INVALID=-6,
};

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
extern void CGI_var_map(http_req *req, char *name, int id);
extern void CGI_var_map_array(http_req *req, char *name, int id, int num);
extern int CGI_do_cmd(http_req *req);

extern void CGI_get_cfg(http_req *req, int tag);
extern void CGI_get_wan_state(http_req *req, int term);
extern char * CGI_get_client_mac(http_req *req);
extern void CGI_get_time(http_req *req);
extern void CGI_get_uptime(http_req *req);
extern void CGI_get_ntpinfo(http_req *req);

extern void CGI_show_routing_table(http_req *req);
#if MODULE_DHCPS
extern void CGI_show_dhcpd_clist(http_req *req);
extern void CGI_show_dhcpd_leases_num(http_req *req);
#endif
#if MODULE_UPNP
extern void CGI_show_upnp_pmap(http_req *req);
#endif
extern void CGI_print_log(http_req *req, int type);

extern char * CGI_put_var(http_req *req, char *var);
extern int CGI_upgrade(http_req *req);
extern int CGI_upgrade_upload(http_req *req);
extern int CGI_upgrade_save(http_req *req);
extern void CGI_upgrade_free(void);

extern int CGI_get_nat_session_brief(http_req *req);
extern int CGI_get_nat_session_list(http_req *req);

extern void CGI_get_ddns_oray_list(http_req *req, char *name);

#ifdef CONFIG_DUALWAN
extern void CGI_get_dwan(http_req *req);
#endif

//==============================================================================
//                              EXTERNAL FUNCTIONS FOR WLAN
//==============================================================================

extern int  parse_config_file(void);
extern void CGI_get_wlan_mac_address(http_req *req);
extern void CGI_get_wlan_11a_channel_list(http_req *req);
extern void CGI_get_wlan_11b_channel_list(http_req *req);
extern void CGI_get_wlan_11g_channel_list(http_req *req);
extern void CGI_get_wlan_channel(http_req *req);
extern void CGI_get_wlan_wsc_config_state(http_req *req);
extern void CGI_get_wlan_ssid_num(http_req *req);
extern void CGI_get_wlan_wds_phy_mode(http_req *req);
extern void CGI_get_wlan_wds_default_key(http_req *req);
extern void CGI_get_wlan_wds_encryp_key(http_req *req);
extern void CGI_get_wlan_apcli_include(http_req *req);
extern void CGI_get_wlan_all_bss_tkip_or_wep(http_req *req);
extern void CGI_get_wlan_ht_sta_associate_list(http_req *req);
extern void CGI_get_wlan_ht_ba_ori_sta_info(http_req *req);
extern void CGI_get_wlan_ht_ba_rec_sta_info(http_req *req);
extern void CGI_get_wlan_wsc_use_proxy(http_req *req);
extern void CGI_get_wlan_wsc_use_internal_reg(http_req *req);
extern void CGI_get_wlan_wsc_pin_code(http_req *req);
extern void CGI_get_wlan_wsc_uuid(http_req *req);
extern void CGI_get_wlan_wsc_dev_pin_code(http_req *req);
extern void CGI_get_wlan_wsc_current_status(http_req *req);
extern void CGI_get_wlan_wsc_encr_type(http_req *req);
extern void CGI_get_wlan_wsc_auth_mode(http_req *req);
extern void CGI_get_wlan_wsc_ssid(http_req *req);
extern void CGI_get_wlan_wsc_pass_phrase(http_req *req);
extern void CGI_get_wlan_wps_process_status(http_req *req);
extern void CGI_get_wlan_mainssid_for_overview(http_req *req);
extern void CGI_get_wlan_multissid1_for_overview(http_req *req);
extern void CGI_get_wlan_multissid2_for_overview(http_req *req);
extern void CGI_get_wlan_multissid3_for_overview(http_req *req);
extern void CGI_get_wlan_driver_version(http_req *req);
extern void CGI_get_wlan_rc_type(http_req *req);
extern void CGI_get_wlan_current_wireless_mode(http_req *req);
extern void CGI_get_wlan_current_channel(http_req *req);
extern void CGI_get_wlan_current_broadcast_ssid(http_req *req);
extern void CGI_get_wlan_apclient_bssid(http_req *req);
extern void CGI_get_wlan_apclient_auth(http_req *req);
extern void CGI_get_wlan_apclient_encryp_type(http_req *req);
extern void CGI_get_wlan_apclient_wep_key_type(http_req *req);
extern void CGI_get_wlan_apclient_wep_key1(http_req *req);
extern void CGI_get_wlan_apclient_wep_key2(http_req *req);
extern void CGI_get_wlan_apclient_wep_key3(http_req *req);
extern void CGI_get_wlan_apclient_wep_key4(http_req *req);
extern void CGI_get_wlan_apclient_default_key(http_req *req);
extern void CGI_get_wlan_apclient_wpapsk(http_req *req);
extern void CGI_get_wlan_apclient_ssid(http_req *req);
extern void CGI_get_wlan_wan_connection_mode(http_req *req);
extern void CGI_get_wlan_station_associate_list(http_req *req);
extern void CGI_get_wlan_current_mac_address(http_req *req);
char *  CGI_get_wlan_tx_rx_count(void);
extern void CGI_get_wlan_mcs_list(http_req *req);
extern void CGI_get_wlan_antenna_type(http_req *req);
extern void CGI_get_wlan_tx_stream_list(http_req *req);
extern void CGI_get_wlan_rx_stream_list(http_req *req);
#endif //CGI_API_H

//Add by seen
extern void CGI_get_scan_result(http_req *req);
#define CGI_GET_SCAN_RESULT()	CGI_get_scan_result(req)

extern void CGI_get_conn_stat(http_req *req);
#define CGI_GET_CONN_STAT() CGI_get_conn_stat(req)
