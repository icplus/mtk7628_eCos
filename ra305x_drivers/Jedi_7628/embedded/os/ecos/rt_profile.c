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
	rt_profile.c
 
    Abstract:
 
    Revision History:
    Who          When          What
    --------    ----------      ------------------------------------------
*/
 
#include "rt_config.h"

#ifdef BRANCH_ADV
extern unsigned char *wlanbuf;
#else /* !BRANCH_ADV */
static PUCHAR nv_buffer = 
"#The word of \"Default\" must not be removed\n"
"Default\n"
"CountryRegion=5\n"
"CountryRegionABand=1\n"
"CountryCode=\n"
"BssidNum=1\n"
"SSID1=RT3350AP\n"
"WirelessMode=9\n"
"NetworkType=Infra\n"
"TxRate=0\n"
"Channel=1\n"
"BasicRate=15\n"
"BeaconPeriod=100\n"
"DtimPeriod=1\n"
"TxPower=100\n"
"DisableOLBC=0\n"
"BGProtection=0\n"
"TxAntenna=\n"
"RxAntenna=\n"
"TxPreamble=0\n"
"RTSThreshold=2347\n"
"FragThreshold=2346\n"
"TxBurst=1\n"
"PktAggregate=0\n"
"TurboRate=0\n"
"WmmCapable=0\n"
"APAifsn=3;7;1;1\n"
"APCwmin=4;4;3;2\n"
"APCwmax=6;10;4;3\n"
"APTxop=0;0;94;47\n"
"APACM=0;0;0;0\n"
"BSSAifsn=3;7;2;2\n"
"BSSCwmin=4;4;3;2\n"
"BSSCwmax=10;10;4;3\n"
"BSSTxop=0;0;94;47\n"
"BSSACM=0;0;0;0\n"
"AckPolicy=0;0;0;0\n"
"APSDCapable=0\n"
"DLSCapable=0\n"
"NoForwarding=0\n"
"NoForwardingBTNBSSID=0\n"
"HideSSID=0\n"
"ShortSlot=1\n"
"AutoChannelSelect=0\n"
"SecurityMode=0\n"
"VLANEnable=0\n"
"VLANName=\n"
"VLANID=0\n"
"VLANPriority=0\n"
"WscConfMode=0\n"
"WscConfStatus=2\n"
"WscAKMP=1\n"
"WscConfigured=1\n"
"WscModeOption=0\n"
"WscActionIndex=9\n"
"WscPinCode=\n"
"WscRegResult=1\n"
"WscUseUPnP=1\n"
"WscUseUFD=0\n"
"WscSSID=RalinkInitialAP\n"
"WscKeyMGMT=WPA-EAP\n"
"WscConfigMethod=138\n"
"WscAuthType=1\n"
"WscEncrypType=1\n"
"WscNewKey=scaptest\n"
"IEEE8021X=0\n"
"IEEE80211H=0\n"
"CSPeriod=6\n"
"PreAuth=0\n"
"AuthMode=OPEN\n"
"EncrypType=NONE\n"
"RekeyInterval=3600\n"
"RekeyMethod=DISABLE\n"
"PMKCachePeriod=10\n"
"WPAPSK1=12345678\n"
"DefaultKeyID=1\n"
"Key1Type=0\n"
"Key1Str1=\n"
"Key2Type=0\n"
"Key2Str1=\n"
"Key3Type=0\n"
"Key3Str1=\n"
"Key4Type=0\n"
"Key4Str1=\n"
"HSCounter=0\n"
"HT_HTC=0\n"
"HT_RDG=1\n"
"HT_LinkAdapt=0\n"
"HT_OpMode=0\n"
"HT_MpduDensity=5\n"
"HT_EXTCHA=1\n"
"HT_BW=1\n"
"HT_AutoBA=1\n"
"HT_BADecline=0\n"
"HT_AMSDU=0\n"
"HT_BAWinSize=64\n"
"HT_GI=1\n"
"HT_STBC=1\n"
"HT_MCS=33\n"
"HT_PROTECT=1\n"
"HT_MIMOPS=3\n"
"HT_40MHZ_INTOLERANT=0\n"
"HT_TxStream=2\n"
"HT_RxStream=2\n"
"HT_BSSCoexistence=0\n"
"NintendoCapable=0\n"
"AccessPolicy0=0\n"
"AccessControlList0=\n"
"AccessPolicy1=0\n"
"AccessControlList1=\n"
"AccessPolicy2=0\n"
"AccessControlList2=\n"
"AccessPolicy3=0\n"
"AccessControlList3=\n"
"WdsEnable=0\n"
"WdsPhyMode=0\n"
"WdsEncrypType=NONE\n"
"WdsList=\n"
"WdsKey=\n"
"WirelessEvent=0\n"
"own_ip_addr=\n"
"RADIUS_Server=\n"
"RADIUS_Port=1900\n"
"RADIUS_Key=ralink\n"
"RADIUS_Acct_Server=\n"
"RADIUS_Acct_Port=1813\n"
"RADIUS_Acct_Key=\n"
"EAPifname=bridge0\n"
"session_timeout_interval=0\n"
"idle_timeout_interval=0\n"
"staWirelessMode=9\n"
"upnpEnabled=0\n"
"pppoeREnabled=0\n"
"RDRegion=JAP\n"
;
#endif /* BRANCH_ADV */
struct dev_type_name_map{
	INT type;
	PSTRING prefix[2];
};

static struct dev_type_name_map prefix_map[] =
{
	{INT_MAIN, 		{INF_MAIN_DEV_NAME, SECOND_INF_MAIN_DEV_NAME}},
#ifdef CONFIG_AP_SUPPORT
#ifdef MBSS_SUPPORT
	{INT_MBSSID, 	{INF_MBSSID_DEV_NAME, SECOND_INF_MBSSID_DEV_NAME}},
#endif /* MBSS_SUPPORT */
#ifdef APCLI_SUPPORT
	{INT_APCLI, 		{INF_APCLI_DEV_NAME, SECOND_INF_APCLI_DEV_NAME}},
#endif /* APCLI_SUPPORT */
#ifdef WDS_SUPPORT
	{INT_WDS, 		{INF_WDS_DEV_NAME, SECOND_INF_WDS_DEV_NAME}},
#endif /* WDS_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_SNIFFER_SUPPORT
		{INT_MONITOR,	{INF_MONITOR_DEV_NAME, SECOND_INF_MONITOR_DEV_NAME}},
#endif /* CONFIG_SNIFFER_SUPPORT */

	{0},
};

struct dev_id_name_map{
	INT chip_id;
	PSTRING chip_name;
};

//not in use
/*
static const struct dev_id_name_map id_name_list[]=
{
	{7610, "7610, 7610e 7610u"},

};
*/
 INT get_dev_config_idx(RTMP_ADAPTER *pAd)
{
	INT idx = 0;

	if (IS_RT8592(pAd))
		idx = 0;
	else if (IS_RT5392(pAd) || IS_MT76x0(pAd))
		idx = 1;

	pAd->dev_idx = idx;
	
	return idx;
}


UCHAR *get_dev_name_prefix(RTMP_ADAPTER *pAd, INT dev_type)
{
	struct dev_type_name_map *map;
	INT type_idx = 0, dev_idx = get_dev_config_idx(pAd);

	do {
		map = &prefix_map[type_idx];
		if (map->type == dev_type) {
			DBGPRINT(RT_DEBUG_TRACE, ("%s(): dev_idx = %d, dev_name_prefix=%s\n",
						__FUNCTION__, dev_idx, map->prefix[dev_idx]));
			return map->prefix[dev_idx];
		}
		type_idx++;
	} while (prefix_map[type_idx].type != 0);

	return NULL;
}


static UCHAR *get_dev_profile(RTMP_ADAPTER *pAd)
{
	DBGPRINT(RT_DEBUG_ERROR, ("%s(%d) TODO\n", __FUNCTION__, __LINE__));

	return NULL;
}




NDIS_STATUS	RTMPReadParametersHook(
	IN	PRTMP_ADAPTER pAd)
{
#ifdef DOT1X_SUPPORT
	pAd->ApCfg.own_ip_addr = 16885952; /*Default 192.168.1.1 */
#endif /* DOT1X_SUPPORT */

	if ((pAd->infType == RTMP_DEV_INF_PCIE) && (wlanbuf != NULL))
		{
			RTMPSetProfileParameters(pAd, (PSTRING)wlanbuf);	//comment:second wifi use pcie should use wlanbuf_i.
			diag_printf("RTMPReadParametersHook->second wifi\n");
		}
	else
		{
			RTMPSetProfileParameters(pAd, (PSTRING)wlanbuf);
			diag_printf("RTMPReadParametersHook->first wifi\n");
		}	
	
#ifdef SINGLE_SKU_V2
			RTMPSetSingleSKUParameters(pAd);
#endif /* SINGLE_SKU_V2 */

	return (NDIS_STATUS_SUCCESS);	
}


int RTMPSendPackets(
	IN NDIS_HANDLE dev_hnd,
	IN PPNDIS_PACKET pkt_list,
	IN UINT pkt_cnt,
	IN UINT32 pkt_total_len,
	IN RTMP_NET_ETH_CONVERT_DEV_SEARCH Func)
{
	struct wifi_dev *wdev = (struct wifi_dev *)dev_hnd;
	RTMP_ADAPTER *pAd;
	PNDIS_PACKET pPacket = pkt_list[0];

	ASSERT(wdev->sys_handle);
	pAd = (RTMP_ADAPTER *)wdev->sys_handle;
	INC_COUNTER64(pAd->WlanCounters.TransmitCountFrmOs);

	if (pPacket == NULL)
		return 0;

	if (pkt_total_len < 14)
	{
		hex_dump("bad packet", GET_OS_PKT_DATAPTR(pPacket), pkt_total_len);
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		return 0;
	}

#ifdef CONFIG_ATE
	if (ATE_ON(pAd))
	{
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_SUCCESS);
		return 0;
	}
#endif /* CONFIG_ATE */


	wdev_tx_pkts((NDIS_HANDLE)pAd, (PPNDIS_PACKET) &pPacket, 1, wdev);

	return 0;
}

#ifdef CONFIG_SNIFFER_SUPPORT


int sniffer_debug_level = DBG_LVL_ERROR;
extern int Sniffer_Packet_Process;
extern int Sniffer_Packet_In;
extern int Sniffer_Packet_Drop;

int SnifferDebugCmd(int argc, char* argv[])
{
	if (( argc == 2 ) && (!strcmp(argv[0], "debug" )))
	{
		int level = 0;

		level = atoi(argv[1]);	  
		if (level > 0)
				sniffer_debug_level = level;
		else
				sniffer_debug_level = DBG_LVL_OFF;


		diag_printf("sniffer debug level = %d\n", sniffer_debug_level);
	}
	if (( argc == 2 ) && (!strcmp(argv[0], "stat" )))
	{
		if(!strcmp(argv[1], "show" ))
		{
			diag_printf("====Sniffer Statistics====\n");
			diag_printf("Sniffer_Packet_Process = %d\n", Sniffer_Packet_Process);
			diag_printf("Sniffer_Packet_In = %d\n", Sniffer_Packet_In);
			diag_printf("Sniffer_Packet_Drop = %d\n", Sniffer_Packet_Drop);
			diag_printf("====Sniffer Statistics end====\n");
		}
		if(!strcmp(argv[1], "reset" ))
		{
			Sniffer_Packet_Process = 0;
			Sniffer_Packet_In = 0;
			Sniffer_Packet_Drop = 0;
			diag_printf("====Sniffer Statistics====\n");
			diag_printf("Sniffer_Packet_Process = %d\n", Sniffer_Packet_Process);
			diag_printf("Sniffer_Packet_In = %d\n", Sniffer_Packet_In);
			diag_printf("Sniffer_Packet_Drop = %d\n", Sniffer_Packet_Drop);	
			diag_printf("====Sniffer Statistics end====\n");
		}
	}	
	
	return TRUE;
}

void sniffer_hex_dump(char *str, unsigned char *pSrcBufVA, unsigned int SrcBufLen)
{
	unsigned char *pt;
	int x;

	if (sniffer_debug_level < 4)
		return;
	
	pt = pSrcBufVA;
	printf("%s: %p, len = %d\n",str,  pSrcBufVA, SrcBufLen);
	for (x=0; x<SrcBufLen; x++)
	{
		if (x % 16 == 0) 
			printf("0x%04x : ", x);
		printf("%02x ", ((unsigned char)pt[x]));
		if (x%16 == 15) printf("\n");
	}
    printf("\n");
}



INT Monitor_VirtualIF_Open(PNET_DEV dev_p)
{
	VOID *pAd;

	pAd = RTMP_OS_NETDEV_GET_PRIV(dev_p);
	ASSERT(pAd);

	DBGPRINT(RT_DEBUG_ERROR, ("%s: ===> %s\n", __FUNCTION__, RTMP_OS_NETDEV_GET_DEVNAME(dev_p)));

	if (VIRTUAL_IF_UP(pAd) != 0)
		return -1;

	/* increase MODULE use count */
	RT_MOD_INC_USE_COUNT();
	RTMP_COM_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_SNIFF_OPEN, 0, dev_p, 0);


	//Monitor_Open(pAd,dev_p);

	return 0;
}

INT Monitor_VirtualIF_Close(PNET_DEV dev_p)
{
	VOID *pAd;

	pAd = RTMP_OS_NETDEV_GET_PRIV(dev_p);
	ASSERT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("%s: ===> %s\n", __FUNCTION__, RTMP_OS_NETDEV_GET_DEVNAME(dev_p)));

	RTMP_COM_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_SNIFF_CLOSE, 0, dev_p, 0);
	//Monitor_Close(pAd,dev_p);

	VIRTUAL_IF_DOWN(pAd);

	RT_MOD_DEC_USE_COUNT();

	return 0;
}


VOID RT28xx_Monitor_Init(VOID *pAd, PNET_DEV main_dev_p)
{
	RTMP_OS_NETDEV_OP_HOOK netDevOpHook;	

	/* init operation functions */
	NdisZeroMemory(&netDevOpHook, sizeof(RTMP_OS_NETDEV_OP_HOOK));
	netDevOpHook.open = Monitor_VirtualIF_Open;
	netDevOpHook.stop = Monitor_VirtualIF_Close;
	netDevOpHook.xmit = rt28xx_send_packets;
	netDevOpHook.ioctl = rt28xx_ioctl;
	DBGPRINT(RT_DEBUG_OFF, ("%s: %d !!!!####!!!!!!\n",__FUNCTION__, __LINE__)); 
	RTMP_COM_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_SNIFF_INIT,	0, &netDevOpHook, 0);

}
VOID RT28xx_Monitor_Remove(VOID *pAd)
{

	RTMP_COM_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_SNIFF_REMOVE, 0, NULL, 0);
	
}

int Sniffer_Packet_Monitor=0;

void STA_MonPktSend(RTMP_ADAPTER *pAd, RX_BLK *pRxBlk)
{
	PNET_DEV pNetDev;
	PNDIS_PACKET pRxPacket;
	PHEADER_802_11 pHeader;
	USHORT DataSize;
	CHAR MaxRssi, RSSI1;
	UINT32 timestamp = 0;
	CHAR RssiForRadiotap = 0;
	UCHAR L2PAD, PHYMODE, BW, ShortGI, MCS, LDPC, LDPC_EX_SYM, AMPDU, STBC;
	UCHAR BssMonitorFlag11n, Channel, CentralChannel = 0;
	UCHAR *pData, *pDevName;
	UCHAR sniffer_type = pAd->sniffer_ctl.sniffer_type;
	UCHAR sideband_index = 0;
	
	SNI_DBGPRINTF(RT_DEBUG_INFO, "STA_MonPktSend =>\n");
	
	ASSERT(pRxBlk->pRxPacket);
	
	Sniffer_Packet_Monitor++;
	
	if (pRxBlk->DataSize < 10) {
		DBGPRINT(RT_DEBUG_ERROR, ("%s : Size is too small! (%d)\n", __FUNCTION__, pRxBlk->DataSize));
		goto err_free_sk_buff;
	}

	if (sniffer_type == RADIOTAP_TYPE) {
		if (pRxBlk->DataSize + sizeof(struct mtk_radiotap_header) > RX_BUFFER_AGGRESIZE) {
			DBGPRINT(RT_DEBUG_ERROR, ("%s : Size is too large! (%d)\n", __FUNCTION__, pRxBlk->DataSize + sizeof(struct mtk_radiotap_header)));
			goto err_free_sk_buff;
		}
	}

	if (sniffer_type == PRISM_TYPE) {
		if (pRxBlk->DataSize + sizeof(wlan_ng_prism2_header) > RX_BUFFER_AGGRESIZE) {
			DBGPRINT(RT_DEBUG_ERROR, ("%s : Size is too large! (%d)\n", __FUNCTION__, pRxBlk->DataSize + sizeof(wlan_ng_prism2_header)));
			goto err_free_sk_buff;
		}
	}

	MaxRssi = RTMPMaxRssi(pAd,
				ConvertToRssi(pAd, (struct raw_rssi_info *)(&pRxBlk->rx_signal.raw_rssi[0]), RSSI_IDX_0),
				ConvertToRssi(pAd, (struct raw_rssi_info *)(&pRxBlk->rx_signal.raw_rssi[0]), RSSI_IDX_1),
				ConvertToRssi(pAd, (struct raw_rssi_info *)(&pRxBlk->rx_signal.raw_rssi[0]), RSSI_IDX_2));

	if (sniffer_type == RADIOTAP_TYPE){
		RssiForRadiotap = RTMPMaxRssi(pAd,
				ConvertToRssi(pAd, (struct raw_rssi_info *)(&pRxBlk->rx_signal.raw_rssi[0]), RSSI_IDX_0),
				ConvertToRssi(pAd, (struct raw_rssi_info *)(&pRxBlk->rx_signal.raw_rssi[0]), RSSI_IDX_1),
				ConvertToRssi(pAd, (struct raw_rssi_info *)(&pRxBlk->rx_signal.raw_rssi[0]), RSSI_IDX_2));
	}

	pNetDev = pAd->monitor_ctrl.wdev.if_dev;  //send packet to mon0
	//pNetDev = get_netdev_from_bssid(pAd, BSS0);
	pRxPacket = pRxBlk->pRxPacket;
	pHeader = pRxBlk->pHeader;
	pData = pRxBlk->pData;
	DataSize = pRxBlk->DataSize;
	L2PAD = pRxBlk->pRxInfo->L2PAD;
	PHYMODE = pRxBlk->rx_rate.field.MODE;
	BW = pRxBlk->rx_rate.field.BW;

	ShortGI = pRxBlk->rx_rate.field.ShortGI;
	MCS = pRxBlk->rx_rate.field.MCS;
	LDPC = pRxBlk->rx_rate.field.ldpc;
	if (pAd->chipCap.hif_type == HIF_RLT)
		LDPC_EX_SYM = pRxBlk->ldpc_ex_sym;
	else
		LDPC_EX_SYM = 0;
		
	AMPDU = pRxBlk->pRxInfo->AMPDU;
	STBC = pRxBlk->rx_rate.field.STBC;
	RSSI1 = pRxBlk->rx_signal.raw_rssi[1];
	//if(pRxBlk->pRxWI->RXWI_N.bbp_rxinfo[12] != 0)
#ifdef RLT_MAC
	if(pAd->chipCap.hif_type == HIF_RLT)
		NdisCopyMemory(&timestamp,&pRxBlk->pRxWI->RXWI_N.bbp_rxinfo[12],4);
#endif
#ifdef MT_MAC
	if(pAd->chipCap.hif_type == HIF_MT)
		timestamp = pRxBlk->TimeStamp;
#endif

	BssMonitorFlag11n = 0;
#ifdef MONITOR_FLAG_11N_SNIFFER_SUPPORT
	BssMonitorFlag11n = (pAd->StaCfg.BssMonitorFlag & MONITOR_FLAG_11N_SNIFFER);
#endif /* MONITOR_FLAG_11N_SNIFFER_SUPPORT */
	pDevName = (UCHAR *)RtmpOsGetNetDevName(pAd->net_dev);
	Channel = pAd->CommonCfg.Channel;

	if (BW == BW_20)
		CentralChannel = Channel;
	else if (BW == BW_40)
	CentralChannel = pAd->CommonCfg.CentralChannel;



	if (sniffer_type == RADIOTAP_TYPE) {
		send_radiotap_monitor_packets(pNetDev, pRxPacket, (void *)pHeader, pData, DataSize,
									  L2PAD, PHYMODE, BW, ShortGI, MCS, LDPC, LDPC_EX_SYM, 
									  AMPDU, STBC, RSSI1, pDevName, Channel, CentralChannel,
							 		  sideband_index, RssiForRadiotap,timestamp);
	}

	if (sniffer_type == PRISM_TYPE) {
		send_prism_monitor_packets(pNetDev, pRxPacket, (void *)pHeader, pData, DataSize,
						L2PAD, PHYMODE, BW, ShortGI, MCS, AMPDU, STBC, RSSI1,
						BssMonitorFlag11n, pDevName, Channel, CentralChannel,
						MaxRssi);
	}

	return;

err_free_sk_buff:
	RELEASE_NDIS_PACKET(pAd, pRxBlk->pRxPacket, NDIS_STATUS_FAILURE);
	return;
}
#endif /* CONFIG_SNIFFER_SUPPORT */

