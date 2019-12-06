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
	ap_ioctl.c

    Abstract:
    IOCTL related subroutines

    Revision History:
    Who          When          What
    --------    ----------      ------------------------------------------
*/

#include "rt_config.h"

#define NR_WEP_KEYS 				4
#define WEP_SMALL_KEY_LEN 			(40/8)
#define WEP_LARGE_KEY_LEN 			(104/8)

#define GROUP_KEY_NO                4

/*
This is required for LinEX2004/kernel2.6.7 to provide iwlist scanning function
*/

int
rt_ioctl_giwname(IN	PNET_DEV         dev,
		   struct iw_request_info *info,
		   char *name, char *extra)
{
	strncpy(name, "Ralink STA", IFNAMSIZ);
	return 0;
}

int rt_ioctl_siwfreq(IN	PNET_DEV         dev,
			struct iw_request_info *info,
			struct iw_freq *freq, char *extra)
{
	VOID *pAd = NULL;
/*	int 	chan = -1; */
	RT_CMD_STA_IOCTL_FREQ IoctlFreq, *pIoctlFreq = &IoctlFreq;

	GET_PAD_FROM_NET_DEV(pAd, dev);

    /*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        return -ENETDOWN;   
    }


	if (freq->e > 1)
		return -EINVAL;

    
	pIoctlFreq->m = freq->m;
	pIoctlFreq->e = freq->e;

	if (RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCSIWFREQ, 0,
							pIoctlFreq, 0, INT_MAIN) != NDIS_STATUS_SUCCESS)
		return -EINVAL;

	return 0;
}


int rt_ioctl_giwfreq(IN	PNET_DEV         dev,
		   struct iw_request_info *info,
		   struct iw_freq *freq, char *extra)
{
	VOID *pAd = NULL;
/*	UCHAR ch; */
	ULONG	m = 2412000;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	if (pAd == NULL)
	{
		/* if 1st open fail, pAd will be free;
		   So the net_dev->priv will be NULL in 2rd open */
		return -ENETDOWN;
	}

	/*check if the interface is down */
/*	if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
		return -ENETDOWN;   
	}

	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCGIWFREQ, 0,
						&m, INT_MAIN, INT_MAIN);

	freq->m = m * 100;
	freq->e = 1;
	freq->i = 0;
	
	return 0;
}


int rt_ioctl_siwmode(IN	PNET_DEV         dev,
		   struct iw_request_info *info,
		   __u32 *mode, char *extra)
{
	VOID *pAd = NULL;
	LONG Mode;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	/*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
    	DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
       	return -ENETDOWN;   
    }


	if (*mode == IW_MODE_ADHOC)
		Mode = RTMP_CMD_STA_MODE_ADHOC;
	else if (*mode == IW_MODE_INFRA)
		Mode = RTMP_CMD_STA_MODE_INFRA;
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("===>rt_ioctl_siwmode::SIOCSIWMODE (unknown %d)\n", *mode));
		return -EINVAL;
	}

	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCSIWMODE, 0,
							NULL, Mode, INT_MAIN);
	return 0;
}


int rt_ioctl_giwmode(IN	PNET_DEV         dev,
		   struct iw_request_info *info,
		   __u32 *mode, char *extra)
{
	VOID *pAd = NULL;
	ULONG Mode;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	if (pAd == NULL)
	{
		/* if 1st open fail, pAd will be free;
		   So the net_dev->priv will be NULL in 2rd open */
		return -ENETDOWN;
	}

	/*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        return -ENETDOWN;   
    }


	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCGIWMODE, 0,
						&Mode, 0, INT_MAIN);

	if (Mode == RTMP_CMD_STA_MODE_ADHOC)
		*mode = IW_MODE_ADHOC;
	else if (Mode == RTMP_CMD_STA_MODE_INFRA)
		*mode = IW_MODE_INFRA;
	else
		*mode = IW_MODE_AUTO;

	DBGPRINT(RT_DEBUG_TRACE, ("==>rt_ioctl_giwmode(mode=%d)\n", *mode));
	return 0;
}

int rt_ioctl_siwsens(IN	PNET_DEV         dev,
		   struct iw_request_info *info,
		   char *name, char *extra)
{
	VOID *pAd = NULL;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	/*check if the interface is down */
/*    	if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    	{
        	DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        	return -ENETDOWN;   
    	}

	return 0;
}

int rt_ioctl_giwsens(IN	PNET_DEV         dev,
		   struct iw_request_info *info,
		   char *name, char *extra)
{
	return 0;
}

int rt_ioctl_giwrange(IN	PNET_DEV         dev,
		   struct iw_request_info *info,
		   struct iw_point *data, char *extra)
{
	VOID *pAd = NULL;
	struct iw_range *range = (struct iw_range *) extra;
	u16 val;
	int i;
	ULONG Mode, ChannelListNum;
	UCHAR *pChannel;
	UINT32 *pFreq;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	if (pAd == NULL)
	{
		/* if 1st open fail, pAd will be free;
		   So the net_dev->priv will be NULL in 2rd open */
		return -ENETDOWN;
	}

#ifndef RT_CFG80211_SUPPORT
/*	if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
	{
    	DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
    	return -ENETDOWN;   
	}
#endif /* RT_CFG80211_SUPPORT */

	DBGPRINT(RT_DEBUG_TRACE ,("===>rt_ioctl_giwrange\n"));
	data->length = sizeof(struct iw_range);
	memset(range, 0, sizeof(struct iw_range));

	range->txpower_capa = IW_TXPOW_DBM;

	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCGIWMODE, 0,
						&Mode, 0, INT_MAIN);

/*	if (INFRA_ON(pAd)||ADHOC_ON(pAd)) */
	if ((Mode == RTMP_CMD_STA_MODE_INFRA) || (Mode == RTMP_CMD_STA_MODE_ADHOC))
	{
		range->min_pmp = 1 * 1024;
		range->max_pmp = 65535 * 1024;
		range->min_pmt = 1 * 1024;
		range->max_pmt = 1000 * 1024;
		range->pmp_flags = IW_POWER_PERIOD;
		range->pmt_flags = IW_POWER_TIMEOUT;
		range->pm_capa = IW_POWER_PERIOD | IW_POWER_TIMEOUT |
			IW_POWER_UNICAST_R | IW_POWER_ALL_R;
	}

	range->we_version_compiled = WIRELESS_EXT;
	range->we_version_source = 14;

	range->retry_capa = IW_RETRY_LIMIT;
	range->retry_flags = IW_RETRY_LIMIT;
	range->min_retry = 0;
	range->max_retry = 255;

/*	range->num_channels =  pAd->ChannelListNum; */
	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_CHAN_LIST_NUM_GET, 0,
						&ChannelListNum, 0, INT_MAIN);
	range->num_channels = ChannelListNum;

	os_alloc_mem(NULL, (UCHAR **)&pChannel, sizeof(UCHAR)*ChannelListNum);
	if (pChannel == NULL)
		return -ENOMEM;
	os_alloc_mem(NULL, (UCHAR **)&pFreq, sizeof(UINT32)*ChannelListNum);
	if (pFreq == NULL)
	{
		os_free_mem(NULL, pChannel);
		return -ENOMEM;
	}

	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_CHAN_LIST_GET, 0,
						pChannel, 0, INT_MAIN);
	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_FREQ_LIST_GET, 0,
						pFreq, 0, INT_MAIN);

	val = 0;
	for (i = 1; i <= range->num_channels; i++) 
	{
/*		u32 m = 2412000; */
		range->freq[val].i = pChannel[i-1];
/*		MAP_CHANNEL_ID_TO_KHZ(pAd->ChannelList[i-1].Channel, m); */
		range->freq[val].m = pFreq[i-1] * 100; /* OS_HZ */
		
		range->freq[val].e = 1;
		val++;
		if (val == IW_MAX_FREQUENCIES)
			break;
	}
	os_free_mem(NULL, pChannel);
	os_free_mem(NULL, pFreq);

	range->num_frequency = val;

	range->max_qual.qual = 100; /* what is correct max? This was not
					* documented exactly. At least
					* 69 has been observed. */
	range->max_qual.level = 0; /* dB */
	range->max_qual.noise = 0; /* dB */

	/* What would be suitable values for "average/typical" qual? */
	range->avg_qual.qual = 20;
	range->avg_qual.level = -60;
	range->avg_qual.noise = -95;
	range->sensitivity = 3;

	range->max_encoding_tokens = NR_WEP_KEYS;
	range->num_encoding_sizes = 2;
	range->encoding_size[0] = 5;
	range->encoding_size[1] = 13;

	range->min_rts = 0;
	range->max_rts = 2347;
	range->min_frag = 256;
	range->max_frag = 2346;

#if WIRELESS_EXT > 17
	/* IW_ENC_CAPA_* bit field */
	range->enc_capa = IW_ENC_CAPA_WPA | IW_ENC_CAPA_WPA2 | 
					IW_ENC_CAPA_CIPHER_TKIP | IW_ENC_CAPA_CIPHER_CCMP;
#endif

	return 0;
}

int rt_ioctl_giwpriv(	
	IN	PNET_DEV         dev,
	struct iw_request_info *info,
	struct iw_point *dwrq,	char *extra)
{
#ifdef ANDROID_SUPPORT
	VOID *pAd = NULL;
	int len = 0;
	char *ext;
	int ret = 0;

	len = dwrq->length;
	ext = kmalloc(len, /*GFP_KERNEL*/GFP_ATOMIC);
	if(!ext)	
		return -ENOMEM;

	if (copy_from_user(ext, dwrq->pointer, len))
	{
		kfree(ext);
		printk("andriod_handle_private   copy_from_user\n");
		return -EFAULT;
	}
	ext[len-1] = 0x00;
	GET_PAD_FROM_NET_DEV(pAd, dev);

	if(strcasecmp(ext,"START") == 0)
	{
		//Turn on Wi-Fi hardware
		//OK if successful
		printk("sSTART Turn on Wi-Fi hardware \n");
		kfree(ext);
		return -1;
	}
	else if(strcasecmp(ext,"STOP") == 0)
	{
		printk("STOP Turn off  Wi-Fi hardware \n");
		kfree(ext);
		return -1;
	}
	else if(strcasecmp(ext,"RSSI") == 0)
	{
		CHAR AvgRssi0;
		RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCSIWPRIVRSSI,
								0, &AvgRssi0, 0, INT_MAIN);
		snprintf(ext, min(dwrq->length, (UINT16)(strlen(ext)+1)),"rssi %d", AvgRssi0);
	}
	else if(strcasecmp(ext,"LINKSPEED") == 0)
	{
		snprintf(ext, min(dwrq->length, (UINT16)(strlen(ext)+1)),"LINKSPEED %d", 150);
	}
	else if(strcasecmp(ext,"MACADDR") == 0)
	{
		UCHAR mac[6];
		RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCGIFHWADDR,
								0, mac, 0, INT_MAIN);
		snprintf(ext, min(dwrq->length, (UINT16)(strlen(ext)+1)),
			"MACADDR = %02x.%02x.%02x.%02x.%02x.%02x",
			mac[0], mac[1], mac[2],
			mac[3], mac[4], mac[5]);
	}
	else if(strcasecmp(ext,"SCAN-ACTIVE") == 0)
	{
		snprintf(ext, min(dwrq->length, (UINT16)(strlen(ext)+1)),"OK");
	}
	else if(strcasecmp(ext,"SCAN-PASSIVE") == 0)
	{
		snprintf(ext, min(dwrq->length, (UINT16)(strlen(ext)+1)),"OK");
	}
	else
	{
		goto FREE_EXT;
	}

	if (copy_to_user(dwrq->pointer, ext, min(dwrq->length, (UINT16)(strlen(ext)+1)) ) )
		ret = -EFAULT;

FREE_EXT:

	kfree(ext);

	return ret;
#else
	return 0;
#endif
}


int rt_ioctl_siwap(IN	PNET_DEV         dev,
		      struct iw_request_info *info,
		      struct sockaddr *ap_addr, char *extra)
{
	VOID *pAd = NULL;
    UCHAR Bssid[6];

	GET_PAD_FROM_NET_DEV(pAd, dev);

	/*check if the interface is down */
/*	if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
	{
       	DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
       	return -ENETDOWN;   
    }


	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCSIWAP, 0,
					(VOID *)(ap_addr->sa_data), 0, INT_MAIN);

    memcpy(Bssid, ap_addr->sa_data, MAC_ADDR_LEN);

    DBGPRINT(RT_DEBUG_TRACE, ("IOCTL::SIOCSIWAP %02x:%02x:%02x:%02x:%02x:%02x\n",
        Bssid[0], Bssid[1], Bssid[2], Bssid[3], Bssid[4], Bssid[5]));

	return 0;
}

int rt_ioctl_giwap(IN	PNET_DEV         dev,
		      struct iw_request_info *info,
		      struct sockaddr *ap_addr, char *extra)
{
	VOID *pAd = NULL;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	if (pAd == NULL)
	{
		/* if 1st open fail, pAd will be free;
		   So the net_dev->priv will be NULL in 2rd open */
		return -ENETDOWN;
	}

	/*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        return -ENETDOWN;   
    }


	if (RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCGIWAP, 0,
						(VOID *)(ap_addr->sa_data), 0,
						INT_MAIN) != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("IOCTL::SIOCGIWAP(=EMPTY)\n"));
		return -ENOTCONN;
	}
	ap_addr->sa_family = ARPHRD_ETHER;

	return 0;
}

/*
 * Units are in db above the noise floor. That means the
 * rssi values reported in the tx/rx descriptors in the
 * driver are the SNR expressed in db.
 *
 * If you assume that the noise floor is -95, which is an
 * excellent assumption 99.5 % of the time, then you can
 * derive the absolute signal level (i.e. -95 + rssi). 
 * There are some other slight factors to take into account
 * depending on whether the rssi measurement is from 11b,
 * 11g, or 11a.   These differences are at most 2db and
 * can be documented.
 *
 * NB: various calculations are based on the orinoco/wavelan
 *     drivers for compatibility
 */
static void set_quality(VOID *pAd,
                        struct iw_quality *iq, 
						RT_CMD_STA_IOCTL_BSS *pBss)
/*                        PBSS_ENTRY pBssEntry) */
{

	iq->qual = pBss->ChannelQuality;
	iq->level = (__u8)(pBss->Rssi);
	iq->noise = pBss->Noise;

/*    iq->updated = pAd->iw_stats.qual.updated; */
/*	iq->updated = ((struct iw_statistics *)(pAd->iw_stats))->qual.updated; */
	iq->updated = 1;     /* Flags to know if updated */

#if WIRELESS_EXT >= 17
	iq->updated = IW_QUAL_QUAL_UPDATED | IW_QUAL_LEVEL_UPDATED | IW_QUAL_NOISE_UPDATED;
#endif

#if WIRELESS_EXT >= 19
	iq->updated |= IW_QUAL_DBM;	/* Level + Noise are dBm */
#endif
}

int rt_ioctl_iwaplist(IN	PNET_DEV         dev,
			struct iw_request_info *info,
			struct iw_point *data, char *extra)
{
 	VOID *pAd = NULL;	

/*	struct sockaddr addr[IW_MAX_AP]; */
	struct sockaddr *addr = NULL;
	struct iw_quality qual[IW_MAX_AP];
	int i;
	RT_CMD_STA_IOCTL_BSS_LIST BssList, *pBssList = &BssList;
	RT_CMD_STA_IOCTL_BSS *pList;

	GET_PAD_FROM_NET_DEV(pAd, dev);

   	/*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
       	DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
		data->length = 0;
		return 0;
        /*return -ENETDOWN; */
	}

	/* allocate memory */
	os_alloc_mem(NULL, (UCHAR **)&(pBssList->pList), sizeof(RT_CMD_STA_IOCTL_BSS_LIST) * IW_MAX_AP);
	if (pBssList->pList == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Allocate memory fail!!!\n", __FUNCTION__));
		return 0;
	}

	os_alloc_mem(NULL, (UCHAR **)&addr, sizeof(struct sockaddr) * IW_MAX_AP);
	if (addr == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Allocate memory fail!!!\n", __FUNCTION__));
		os_free_mem(NULL, pBssList);
		return 0;
	}

	pBssList->MaxNum = IW_MAX_AP;
	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_BSS_LIST_GET, 0,
						pBssList, 0, INT_MAIN);

	for (i = 0; i <IW_MAX_AP ; i++)
	{
		if (i >=  pBssList->BssNum) /*pAd->ScanTab.BssNr) */
			break;
		addr[i].sa_family = ARPHRD_ETHER;
		pList = (pBssList->pList) + i;
		memcpy(addr[i].sa_data, pList->Bssid, MAC_ADDR_LEN);
		set_quality(pAd, &qual[i], pList); /*&pAd->ScanTab.BssEntry[i]); */
	}
	data->length = i;
	memcpy(extra, &addr, i*sizeof(addr[0]));
	data->flags = 1;		/* signal quality present (sort of) */
	memcpy(extra + i*sizeof(addr[0]), &qual, i*sizeof(qual[i]));

	os_free_mem(NULL, addr);
	os_free_mem(NULL, pBssList->pList);
	return 0;
}
int rt_ioctl_siwessid(IN	PNET_DEV         dev,
			 struct iw_request_info *info,
			 struct iw_point *data, char *essid)
{
	VOID *pAd = NULL;
	RT_CMD_STA_IOCTL_SSID IoctlEssid, *pIoctlEssid = &IoctlEssid;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	/*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
       	DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
       	return -ENETDOWN;   
    }

	if (data->flags)
	{
		/* Includes null character. */
		if (data->length > (IW_ESSID_MAX_SIZE + 1)) 
			return -E2BIG;
	}


	pIoctlEssid->FlgAnySsid = data->flags;
	pIoctlEssid->SsidLen = data->length;
	pIoctlEssid->pSsid = (CHAR *)essid;
	pIoctlEssid->Status = 0;
	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCSIWESSID, 0,
						pIoctlEssid, 0, INT_MAIN);

	RT_CMD_STATUS_TRANSLATE(pIoctlEssid->Status);
	return pIoctlEssid->Status;
}

int rt_ioctl_giwessid(IN	PNET_DEV         dev,
			 struct iw_request_info *info,
			 struct iw_point *data, char *essid)
{
	VOID *pAd = NULL;
	RT_CMD_STA_IOCTL_SSID IoctlEssid, *pIoctlEssid = &IoctlEssid;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	if (pAd == NULL)
	{
		/* if 1st open fail, pAd will be free;
		   So the net_dev->priv will be NULL in 2rd open */
		return -ENETDOWN;
	}

	/*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
       	DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        return -ENETDOWN;
	}

	data->flags = 1;


	pIoctlEssid->pSsid = (CHAR *)essid;
	pIoctlEssid->Status = 0;
	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCGIWESSID, 0,
						pIoctlEssid, 0, INT_MAIN);
	data->length = pIoctlEssid->SsidLen;

	RT_CMD_STATUS_TRANSLATE(pIoctlEssid->Status);
	return pIoctlEssid->Status;
}

int rt_ioctl_siwnickn(IN	PNET_DEV         dev,
			 struct iw_request_info *info,
			 struct iw_point *data, char *nickname)
{
	VOID *pAd = NULL;

	GET_PAD_FROM_NET_DEV(pAd, dev);

    /*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
        DBGPRINT(RT_DEBUG_TRACE ,("INFO::Network is down!\n"));
        return -ENETDOWN;   
    }

	if (data->length > IW_ESSID_MAX_SIZE)
		return -EINVAL;


	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCSIWNICKN, 0,
							nickname, data->length, INT_MAIN);
	return 0;
}

int rt_ioctl_giwnickn(IN	PNET_DEV         dev,
			 struct iw_request_info *info,
			 struct iw_point *data, char *nickname)
{
	VOID *pAd = NULL;
	RT_CMD_STA_IOCTL_NICK_NAME NickName, *pNickName = &NickName;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	if (pAd == NULL)
	{
		/* if 1st open fail, pAd will be free;
		   So the net_dev->priv will be NULL in 2rd open */
		return -ENETDOWN;
	}

	/*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
       	DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
		data->length = 0;
        return -ENETDOWN;
	}


	pNickName->NameLen = data->length;
	pNickName->pName = (CHAR *)nickname;

	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCGIWNICKN, 0,
							pNickName, 0, INT_MAIN);

	data->length = pNickName->NameLen;
	return 0;
}

int rt_ioctl_siwrts(IN	PNET_DEV         dev,
		       struct iw_request_info *info,
		       struct iw_param *rts, char *extra)
{
	VOID *pAd = NULL;
	u16 val;

	GET_PAD_FROM_NET_DEV(pAd, dev);

    /*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        return -ENETDOWN;   
    }
	
	if (rts->disabled)
		val = MAX_RTS_THRESHOLD;
	else if (rts->value < 0 || rts->value > MAX_RTS_THRESHOLD)
		return -EINVAL;
	else if (rts->value == 0)
	    val = MAX_RTS_THRESHOLD;
	else
		val = rts->value;
	
/*	if (val != pAd->CommonCfg.RtsThreshold) */
/*		pAd->CommonCfg.RtsThreshold = val; */

	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCSIWRTS, 0,
						NULL, val, INT_MAIN);
	return 0;
}

int rt_ioctl_giwrts(IN	PNET_DEV         dev,
		       struct iw_request_info *info,
		       struct iw_param *rts, char *extra)
{
	VOID *pAd = NULL;
	USHORT RtsThreshold;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	if (pAd == NULL)
	{
		/* if 1st open fail, pAd will be free;
		   So the net_dev->priv will be NULL in 2rd open */
		return -ENETDOWN;
	}

	/*check if the interface is down */
/*    	if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
		if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    	{
      		DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        	return -ENETDOWN;   
    	}

	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCGIWRTS, 0,
						&RtsThreshold, 0, INT_MAIN);
	rts->value = RtsThreshold;
	rts->disabled = (rts->value == MAX_RTS_THRESHOLD);
	rts->fixed = 1;

	return 0;
}

int rt_ioctl_siwfrag(IN	PNET_DEV         dev,
			struct iw_request_info *info,
			struct iw_param *frag, char *extra)
{
	VOID *pAd = NULL;
	u16 val;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	/*check if the interface is down */
/*    	if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
		if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    	{
      		DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        	return -ENETDOWN;   
    	}

	if (frag->disabled)
		val = MAX_FRAG_THRESHOLD;
	else if (frag->value >= MIN_FRAG_THRESHOLD || frag->value <= MAX_FRAG_THRESHOLD)
        val = __cpu_to_le16(frag->value & ~0x1); /* even numbers only */
	else if (frag->value == 0)
	    val = MAX_FRAG_THRESHOLD;
	else
		return -EINVAL;

/*	pAd->CommonCfg.FragmentThreshold = val; */
	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCSIWFRAG, 0,
						NULL, val, INT_MAIN);
	return 0;
}

int rt_ioctl_giwfrag(IN	PNET_DEV         dev,
			struct iw_request_info *info,
			struct iw_param *frag, char *extra)
{
	VOID *pAd = NULL;
	USHORT FragmentThreshold;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	if (pAd == NULL)
	{
		/* if 1st open fail, pAd will be free;
		   So the net_dev->priv will be NULL in 2rd open */
		return -ENETDOWN;
	}

	/*check if the interface is down */
/*    	if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
		if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    	{
      		DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        	return -ENETDOWN;   
    	}
		
	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCGIWFRAG, 0,
						&FragmentThreshold, 0, INT_MAIN);
	frag->value = FragmentThreshold;
	frag->disabled = (frag->value == MAX_FRAG_THRESHOLD);
	frag->fixed = 1;

	return 0;
}

#define MAX_WEP_KEY_SIZE 13
#define MIN_WEP_KEY_SIZE 5
int rt_ioctl_siwencode(IN	PNET_DEV         dev,
			  struct iw_request_info *info,
			  struct iw_point *erq, char *extra)
{
	VOID *pAd = NULL;
	RT_CMD_STA_IOCTL_SECURITY IoctlSec, *pIoctlSec = &IoctlSec;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	/*check if the interface is down */
/*    	if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
		if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    	{
      		DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        	return -ENETDOWN;   
    	}


	pIoctlSec->pData = (CHAR *)extra;
	pIoctlSec->length = erq->length;
	pIoctlSec->KeyIdx = (erq->flags & IW_ENCODE_INDEX) - 1;
	pIoctlSec->flags = 0;

	if (erq->flags & IW_ENCODE_DISABLED)
		pIoctlSec->flags |= RT_CMD_STA_IOCTL_SECURITY_DISABLED;
	if (erq->flags & IW_ENCODE_RESTRICTED)
		pIoctlSec->flags |= RT_CMD_STA_IOCTL_SECURITY_RESTRICTED;
	if (erq->flags & IW_ENCODE_OPEN)
		pIoctlSec->flags |= RT_CMD_STA_IOCTL_SECURITY_OPEN;
	if (erq->flags & IW_ENCODE_NOKEY)
		pIoctlSec->flags |= RT_CMD_STA_IOCTL_SECURITY_NOKEY;
	if (erq->flags & IW_ENCODE_MODE)
		pIoctlSec->flags |= RT_CMD_STA_IOCTL_SECURITY_MODE;

	pIoctlSec->Status = 0;

	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCSIWENCODE, 0,
						pIoctlSec, 0, INT_MAIN);
	RT_CMD_STATUS_TRANSLATE(pIoctlSec->Status);
	return pIoctlSec->Status;
}

int
rt_ioctl_giwencode(IN	PNET_DEV         dev,
			  struct iw_request_info *info,
			  struct iw_point *erq, char *key)
{
/*	int kid; */
	VOID *pAd = NULL;
	RT_CMD_STA_IOCTL_SECURITY IoctlSec, *pIoctlSec = &IoctlSec;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	if (pAd == NULL)
	{
		/* if 1st open fail, pAd will be free;
		   So the net_dev->priv will be NULL in 2rd open */
		return -ENETDOWN;
	}

	/*check if the interface is down */
/*	if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
	{
  		DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
    	return -ENETDOWN;   
	}
		

	pIoctlSec->pData = (CHAR *)key;
	pIoctlSec->KeyIdx = erq->flags & IW_ENCODE_INDEX;
	pIoctlSec->length = erq->length;

	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCGIWENCODE, 0,
						pIoctlSec, 0, INT_MAIN);

	erq->length = pIoctlSec->length;
	erq->flags = pIoctlSec->KeyIdx;
	if (pIoctlSec->flags & RT_CMD_STA_IOCTL_SECURITY_DISABLED)
		erq->flags = RT_CMD_STA_IOCTL_SECURITY_DISABLED;
	{
		if (pIoctlSec->flags & RT_CMD_STA_IOCTL_SECURITY_ENABLED)
			erq->flags |= IW_ENCODE_ENABLED;
		if (pIoctlSec->flags & RT_CMD_STA_IOCTL_SECURITY_RESTRICTED)
			erq->flags |= IW_ENCODE_RESTRICTED;	
		if (pIoctlSec->flags & RT_CMD_STA_IOCTL_SECURITY_OPEN)
			erq->flags |= IW_ENCODE_OPEN;
	}
	return 0;

}

int rt_ioctl_setparam(IN	PNET_DEV         dev, struct iw_request_info *info,
			 void *w, char *extra)
{
	VOID *pAd;
/*	POS_COOKIE pObj; */
	PSTRING this_char = extra;
	PSTRING value = NULL;
	int  Status=0;
	RT_CMD_PARAM_SET CmdParam;

	//GET_PAD_FROM_NET_DEV(pAd, dev);
	pAd = RTMP_OS_NETDEV_GET_PRIV(dev);

	if (pAd == NULL)
	{
		/* if 1st open fail, pAd will be free;
		   So the net_dev->priv will be NULL in 2rd open */
		return -ENETDOWN;
	}


	
	if (!*this_char)
		return -EINVAL;
																															
	if ((value = rtstrchr(this_char, '=')) != NULL) 																			
		*value++ = 0;

	/*check if the interface is down */
/*    	if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
		if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, this_char) != NDIS_STATUS_SUCCESS)
    	{
    		DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
			return -ENETDOWN;
    	}
	else
	{
		if (!value && (strcmp(this_char, "SiteSurvey") != 0))
			return -EINVAL;
		else if (!value && (strcmp(this_char, "SiteSurvey") == 0))
			goto SET_PROC;

		/* reject setting nothing besides ANY ssid(ssidLen=0) */
		if (!*value && (strcmp(this_char, "SSID") != 0))
			return -EINVAL; 
	}

SET_PROC:
	CmdParam.pThisChar = this_char;
	CmdParam.pValue = value;
	
	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_PARAM_SET, 0,
						&CmdParam, 0, INT_MAIN);
/*	Status = RTMPSTAPrivIoctlSet(pAd, this_char, value); */
		
    return Status;
}


#ifdef WSC_STA_SUPPORT

static int
rt_private_set_wsc_u32_item(IN	PNET_DEV         dev, struct iw_request_info *info,
			 u32 *uwrq, char *extra)
{
    VOID *pAd = NULL;
/*    int  Status=0; */
/*    u32 subcmd = *uwrq; */
/*    PWSC_PROFILE    pWscProfile = NULL; */
/*   	u32 value = 0; */
	RT_CMD_STA_IOCTL_WSC_U32_ITEM IoctlWscU32, *pIoctlWscU32 = &IoctlWscU32;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	/*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
       	DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        return -ENETDOWN;
	}

    
	pIoctlWscU32->pUWrq = uwrq;
	pIoctlWscU32->Status = NDIS_STATUS_SUCCESS;
	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_IW_SET_WSC_U32_ITEM, 0,
						pIoctlWscU32, 0, INT_MAIN);

	RT_CMD_STATUS_TRANSLATE(pIoctlWscU32->Status);
    return pIoctlWscU32->Status;
}

static int
rt_private_set_wsc_string_item(IN	PNET_DEV         dev, struct iw_request_info *info,
		struct iw_point *dwrq, char *extra)
{
	RT_CMD_STA_IOCTL_WSC_STR_ITEM IoctlWscStr, *pIoctlWscStr = &IoctlWscStr;
/*    int  Status=0; */
/*    u32 subcmd = dwrq->flags; */
/*    u32 tmpProfileIndex = (u32)(extra[0] - 0x30); */
/*    u32 dataLen; */
    VOID *pAd = NULL;
/*    PWSC_PROFILE    pWscProfile = NULL; */
/*    USHORT  tmpAuth = 0, tmpEncr = 0; */

	GET_PAD_FROM_NET_DEV(pAd, dev);

	/*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
       	DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        return -ENETDOWN;
	}


	pIoctlWscStr->Subcmd = dwrq->flags;
	pIoctlWscStr->pData = (CHAR *)extra;
	pIoctlWscStr->length = dwrq->length;

	pIoctlWscStr->Status = RTMP_STA_IoctlHandle(pAd, NULL,
						CMD_RTPRIV_IOCTL_STA_IW_SET_WSC_STR_ITEM, 0,
						pIoctlWscStr, 0, INT_MAIN);

	RT_CMD_STATUS_TRANSLATE(pIoctlWscStr->Status);
    return pIoctlWscStr->Status;
}
#endif /* WSC_STA_SUPPORT */

static int
rt_private_get_statistics(IN	PNET_DEV         dev, struct iw_request_info *info,
		struct iw_point *wrq, char *extra)
{
	INT				Status = 0;
    VOID   *pAd = NULL;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	/*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
       	DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        return -ENETDOWN;
	}

    if (extra == NULL)
    {
        wrq->length = 0;
        return -EIO;
    }
    
    memset(extra, 0x00, IW_PRIV_SIZE_MASK);


	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_IW_GET_STATISTICS, 0,
						extra, IW_PRIV_SIZE_MASK, INT_MAIN);

    wrq->length = strlen(extra) + 1; /* 1: size of '\0' */
    DBGPRINT(RT_DEBUG_TRACE, ("<== rt_private_get_statistics, wrq->length = %d\n", wrq->length));

    return Status;
}


static int
rt_private_show(IN	PNET_DEV         dev, struct iw_request_info *info,
		struct iw_point *wrq, PSTRING extra)
{
	RTMP_IOCTL_INPUT_STRUCT wrqin;
	INT				Status = 0;
	VOID   			*pAd;
/*	POS_COOKIE		pObj; */
	u32             subcmd = wrq->flags;
	RT_CMD_STA_IOCTL_SHOW IoctlShow, *pIoctlShow = &IoctlShow;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	if (pAd == NULL)
	{
		/* if 1st open fail, pAd will be free;
		   So the net_dev->priv will be NULL in 2rd open */
		return -ENETDOWN;
	}


/*	pObj = (POS_COOKIE) pAd->OS_Cookie; */
	if (extra == NULL)
	{
		wrq->length = 0;
		return -EIO;
	}
	memset(extra, 0x00, IW_PRIV_SIZE_MASK);
    
    
	wrqin.u.data.pointer = wrq->pointer;
	wrqin.u.data.length = wrq->length;

	pIoctlShow->pData = (CHAR *)extra;
	pIoctlShow->MaxSize = IW_PRIV_SIZE_MASK;
	pIoctlShow->InfType = INT_MAIN;
	RTMP_STA_IoctlHandle(pAd, &wrqin, CMD_RTPRIV_IOCTL_SHOW, subcmd,
						pIoctlShow, 0, INT_MAIN);

	wrq->length = wrqin.u.data.length;
    return Status;
}

#ifdef SIOCSIWMLME
int rt_ioctl_siwmlme(IN	PNET_DEV         dev,
			   struct iw_request_info *info,
			   union iwreq_data *wrqu,
			   char *extra)
{
	VOID   *pAd = NULL;
	struct iw_mlme *pMlme = (struct iw_mlme *)wrqu->data.pointer;
/*	MLME_QUEUE_ELEM				MsgElem; */
/*	MLME_QUEUE_ELEM				*pMsgElem = NULL; */
/*	MLME_DISASSOC_REQ_STRUCT	DisAssocReq; */
/*	MLME_DEAUTH_REQ_STRUCT      DeAuthReq; */
	ULONG Subcmd = 0;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	DBGPRINT(RT_DEBUG_TRACE, ("====> %s\n", __FUNCTION__));

	/*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
       	DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        return -ENETDOWN;
	}

	if (pMlme == NULL)
		return -EINVAL;


	switch(pMlme->cmd)
	{
#ifdef IW_MLME_DEAUTH	
		case IW_MLME_DEAUTH:
			Subcmd = RT_CMD_STA_IOCTL_IW_MLME_DEAUTH;
			break;
#endif /* IW_MLME_DEAUTH */
#ifdef IW_MLME_DISASSOC
		case IW_MLME_DISASSOC:
			Subcmd = RT_CMD_STA_IOCTL_IW_MLME_DISASSOC;
			break;
#endif /* IW_MLME_DISASSOC */
		default:
			DBGPRINT(RT_DEBUG_TRACE, ("====> %s - Unknow Command\n", __FUNCTION__));
			break;
	}

	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCSIWMLME, Subcmd,
						NULL, pMlme->reason_code, INT_MAIN);
	return 0;
}
#endif /* SIOCSIWMLME */

#if WIRELESS_EXT > 17


int rt_ioctl_siwauth(IN	PNET_DEV         dev,
			  struct iw_request_info *info,
			  union iwreq_data *wrqu, char *extra)
{
	VOID   *pAd = NULL;
	struct iw_param *param = &wrqu->param;
	RT_CMD_STA_IOCTL_SECURITY_ADV IoctlWpa, *pIoctlWpa = &IoctlWpa;

	GET_PAD_FROM_NET_DEV(pAd, dev);

    /*check if the interface is down */
/*	if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
	{
  		DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
    	return -ENETDOWN;   
	}


	pIoctlWpa->flags = 0;
	pIoctlWpa->value = param->value; /* default */

	switch (param->flags & IW_AUTH_INDEX) {
    	case IW_AUTH_WPA_VERSION:
			pIoctlWpa->flags = RT_CMD_STA_IOCTL_WPA_VERSION;
            if (param->value == IW_AUTH_WPA_VERSION_WPA)
				pIoctlWpa->value = RT_CMD_STA_IOCTL_WPA_VERSION1;
            else if (param->value == IW_AUTH_WPA_VERSION_WPA2)
				pIoctlWpa->value = RT_CMD_STA_IOCTL_WPA_VERSION2;

            DBGPRINT(RT_DEBUG_TRACE, ("%s::IW_AUTH_WPA_VERSION - param->value = %d!\n", __FUNCTION__, param->value));
            break;
    	case IW_AUTH_CIPHER_PAIRWISE:
			pIoctlWpa->flags = RT_CMD_STA_IOCTL_WPA_PAIRWISE;
            if (param->value == IW_AUTH_CIPHER_NONE)
                pIoctlWpa->value = RT_CMD_STA_IOCTL_WPA_PAIRWISE_NONE;
            else if (param->value == IW_AUTH_CIPHER_WEP40)
				pIoctlWpa->value = RT_CMD_STA_IOCTL_WPA_PAIRWISE_WEP40;
            else if (param->value == IW_AUTH_CIPHER_WEP104)
                pIoctlWpa->value = RT_CMD_STA_IOCTL_WPA_PAIRWISE_WEP104;
            else if (param->value == IW_AUTH_CIPHER_TKIP)
                pIoctlWpa->value = RT_CMD_STA_IOCTL_WPA_PAIRWISE_TKIP;
            else if (param->value == IW_AUTH_CIPHER_CCMP)
                pIoctlWpa->value = RT_CMD_STA_IOCTL_WPA_PAIRWISE_CCMP;
            DBGPRINT(RT_DEBUG_TRACE, ("%s::IW_AUTH_CIPHER_PAIRWISE - param->value = %d!\n", __FUNCTION__, param->value));
            break;
    	case IW_AUTH_CIPHER_GROUP:
			pIoctlWpa->flags = RT_CMD_STA_IOCTL_WPA_GROUP;
            if (param->value == IW_AUTH_CIPHER_NONE)
				pIoctlWpa->value = RT_CMD_STA_IOCTL_WPA_GROUP_NONE;
            else if (param->value == IW_AUTH_CIPHER_WEP40)
				pIoctlWpa->value = RT_CMD_STA_IOCTL_WPA_GROUP_WEP40;
			else if (param->value == IW_AUTH_CIPHER_WEP104)
				pIoctlWpa->value = RT_CMD_STA_IOCTL_WPA_GROUP_WEP104;
            else if (param->value == IW_AUTH_CIPHER_TKIP)
				pIoctlWpa->value = RT_CMD_STA_IOCTL_WPA_GROUP_TKIP;
            else if (param->value == IW_AUTH_CIPHER_CCMP)
				pIoctlWpa->value = RT_CMD_STA_IOCTL_WPA_GROUP_CCMP;
            DBGPRINT(RT_DEBUG_TRACE, ("%s::IW_AUTH_CIPHER_GROUP - param->value = %d!\n", __FUNCTION__, param->value));
            break;
    	case IW_AUTH_KEY_MGMT:
			pIoctlWpa->flags = RT_CMD_STA_IOCTL_WPA_KEY_MGMT;
            if (param->value == IW_AUTH_KEY_MGMT_802_1X)
				pIoctlWpa->value = RT_CMD_STA_IOCTL_WPA_KEY_MGMT_1X;
            DBGPRINT(RT_DEBUG_TRACE, ("%s::IW_AUTH_KEY_MGMT - param->value = %d!\n", __FUNCTION__, param->value));
            break;
    	case IW_AUTH_RX_UNENCRYPTED_EAPOL:
			pIoctlWpa->flags = RT_CMD_STA_IOCTL_WPA_AUTH_RX_UNENCRYPTED_EAPOL;
            break;
    	case IW_AUTH_PRIVACY_INVOKED:
			pIoctlWpa->flags = RT_CMD_STA_IOCTL_WPA_AUTH_PRIVACY_INVOKED;
            DBGPRINT(RT_DEBUG_TRACE, ("%s::IW_AUTH_PRIVACY_INVOKED - param->value = %d!\n", __FUNCTION__, param->value));
    		break;
    	case IW_AUTH_DROP_UNENCRYPTED:
			pIoctlWpa->flags = RT_CMD_STA_IOCTL_WPA_AUTH_DROP_UNENCRYPTED;
            DBGPRINT(RT_DEBUG_TRACE, ("%s::IW_AUTH_DROP_UNENCRYPTED - param->value = %d!\n", __FUNCTION__, param->value));
    		break;
    	case IW_AUTH_80211_AUTH_ALG: 
			pIoctlWpa->flags = RT_CMD_STA_IOCTL_WPA_AUTH_80211_AUTH_ALG;
			if (param->value & IW_AUTH_ALG_SHARED_KEY) 
				pIoctlWpa->value = RT_CMD_STA_IOCTL_WPA_AUTH_80211_AUTH_ALG_SHARED;
            else if (param->value & IW_AUTH_ALG_OPEN_SYSTEM)
				pIoctlWpa->value = RT_CMD_STA_IOCTL_WPA_AUTH_80211_AUTH_ALG_OPEN;
            else if (param->value & IW_AUTH_ALG_LEAP)
				pIoctlWpa->value = RT_CMD_STA_IOCTL_WPA_AUTH_80211_AUTH_ALG_LEAP;
            DBGPRINT(RT_DEBUG_TRACE, ("%s::IW_AUTH_80211_AUTH_ALG - param->value = %d!\n", __FUNCTION__, param->value));
			break;
    	case IW_AUTH_WPA_ENABLED:
			pIoctlWpa->flags = RT_CMD_STA_IOCTL_WPA_AUTH_WPA_ENABLED;
    		DBGPRINT(RT_DEBUG_TRACE, ("%s::IW_AUTH_WPA_ENABLED - Driver supports WPA!(param->value = %d)\n", __FUNCTION__, param->value));
    		break;
	case IW_AUTH_TKIP_COUNTERMEASURES:
		pIoctlWpa->flags = RT_CMD_STA_IOCTL_WPA_AUTH_COUNTERMEASURES;
		pIoctlWpa->value = param->value;
    		DBGPRINT(RT_DEBUG_TRACE, ("%s::IW_AUTH_TKIP_COUNTERMEASURES -(param->value = %d)\n", __FUNCTION__, param->value));
		break;
    	default:
    		return -EOPNOTSUPP;
}

	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCSIWAUTH, 0,
						pIoctlWpa, 0, INT_MAIN);

	return 0;
}

int rt_ioctl_giwauth(IN	PNET_DEV         dev,
			       struct iw_request_info *info,
			       union iwreq_data *wrqu, char *extra)
{
	VOID   *pAd = NULL;
	struct iw_param *param = &wrqu->param;
	RT_CMD_STA_IOCTL_SECURITY_ADV IoctlWpa, *pIoctlWpa = &IoctlWpa;

	GET_PAD_FROM_NET_DEV(pAd, dev);

    /*check if the interface is down */
/*	if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
  		DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
    	return -ENETDOWN;   
    }


	pIoctlWpa->flags = 0;
	pIoctlWpa->value = 0;

	switch (param->flags & IW_AUTH_INDEX) {
	case IW_AUTH_DROP_UNENCRYPTED:
		pIoctlWpa->flags = RT_CMD_STA_IOCTL_WPA_AUTH_DROP_UNENCRYPTED;
		break;

	case IW_AUTH_80211_AUTH_ALG:
		pIoctlWpa->flags = RT_CMD_STA_IOCTL_WPA_AUTH_80211_AUTH_ALG;
		break;

	case IW_AUTH_WPA_ENABLED:
		pIoctlWpa->flags = RT_CMD_STA_IOCTL_WPA_AUTH_WPA_ENABLED;
		break;

	default:
		return -EOPNOTSUPP;
	}

	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCGIWAUTH, 0,
						pIoctlWpa, 0, INT_MAIN);

	switch (param->flags & IW_AUTH_INDEX) {
	case IW_AUTH_DROP_UNENCRYPTED:
		param->value = pIoctlWpa->value;
		break;

	case IW_AUTH_80211_AUTH_ALG:
		param->value = (pIoctlWpa->value == 0) ? IW_AUTH_ALG_SHARED_KEY : IW_AUTH_ALG_OPEN_SYSTEM;
		break;

	case IW_AUTH_WPA_ENABLED:
		param->value = pIoctlWpa->value;
		break;
	}

    DBGPRINT(RT_DEBUG_TRACE, ("rt_ioctl_giwauth::param->value = %d!\n", param->value));
	return 0;
}


int rt_ioctl_siwencodeext(IN	PNET_DEV         dev,
			   struct iw_request_info *info,
			   union iwreq_data *wrqu,
			   char *extra)
{
	VOID   *pAd = NULL;
	struct iw_point *encoding = &wrqu->encoding;
	struct iw_encode_ext *ext = (struct iw_encode_ext *)extra;
    int /* keyIdx, */ alg = ext->alg;
	RT_CMD_STA_IOCTL_SECURITY IoctlSec, *pIoctlSec = &IoctlSec;
	
	GET_PAD_FROM_NET_DEV(pAd, dev);
	
    /*check if the interface is down */
/*	if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
	{
  		DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
    	return -ENETDOWN;   
	}

			
	pIoctlSec->pData = (CHAR *)ext->key;
	pIoctlSec->length = ext->key_len;
	pIoctlSec->KeyIdx = (encoding->flags & IW_ENCODE_INDEX) - 1;
	if (alg == IW_ENCODE_ALG_NONE )
		pIoctlSec->Alg = RT_CMD_STA_IOCTL_SECURITY_ALG_NONE;
	else if (alg == IW_ENCODE_ALG_WEP)
		pIoctlSec->Alg = RT_CMD_STA_IOCTL_SECURITY_ALG_WEP;
	else if (alg == IW_ENCODE_ALG_TKIP)
		pIoctlSec->Alg = RT_CMD_STA_IOCTL_SECURITY_ALG_TKIP;
	else if (alg == IW_ENCODE_ALG_CCMP)
		pIoctlSec->Alg = RT_CMD_STA_IOCTL_SECURITY_ALG_CCMP;
	else 
	{
		DBGPRINT(RT_DEBUG_WARN, ("Warning: Security type is not supported. (alg = %d) \n", alg));
		pIoctlSec->Alg = alg;
		return -EOPNOTSUPP;
	}
	pIoctlSec->ext_flags = 0;
	if (ext->ext_flags & IW_ENCODE_EXT_SET_TX_KEY)
		pIoctlSec->ext_flags |= RT_CMD_STA_IOCTL_SECURTIY_EXT_SET_TX_KEY;
	if (ext->ext_flags & IW_ENCODE_EXT_GROUP_KEY)
		pIoctlSec->ext_flags |= RT_CMD_STA_IOCTL_SECURTIY_EXT_GROUP_KEY;
	if (encoding->flags & IW_ENCODE_DISABLED)
		pIoctlSec->flags = RT_CMD_STA_IOCTL_SECURITY_DISABLED;
	else
		pIoctlSec->flags = 0;

	if (RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCSIWENCODEEXT, 0,
				pIoctlSec, 0, INT_MAIN) != NDIS_STATUS_SUCCESS)
		return -EINVAL;

    return 0;
}

int
rt_ioctl_giwencodeext(IN	PNET_DEV         dev,
			  struct iw_request_info *info,
			  union iwreq_data *wrqu, char *extra)
{
	VOID *pAd = NULL;
/*	PCHAR pKey = NULL; */
	struct iw_point *encoding = &wrqu->encoding;
	struct iw_encode_ext *ext = (struct iw_encode_ext *)extra;
	int /* idx, */ max_key_len;
	RT_CMD_STA_IOCTL_SECURITY IoctlSec, *pIoctlSec = &IoctlSec;

	GET_PAD_FROM_NET_DEV(pAd, dev);

	DBGPRINT(RT_DEBUG_TRACE ,("===> rt_ioctl_giwencodeext\n"));

	/*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
       	DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        return -ENETDOWN;
	}

	max_key_len = encoding->length - sizeof(*ext);
	if (max_key_len < 0)
		return -EINVAL;
	memset(ext, 0, sizeof(*ext));


	memset(pIoctlSec, 0, sizeof(RT_CMD_STA_IOCTL_SECURITY));
	pIoctlSec->KeyIdx = encoding->flags & IW_ENCODE_INDEX;
	pIoctlSec->MaxKeyLen = max_key_len;

	if (RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCGIWENCODEEXT, 0,
				pIoctlSec, 0, INT_MAIN) != NDIS_STATUS_SUCCESS)
	{
		ext->key_len = 0;
		RT_CMD_STATUS_TRANSLATE(pIoctlSec->Status);
		return pIoctlSec->Status;
	}

	encoding->flags = pIoctlSec->KeyIdx;
	ext->key_len = pIoctlSec->length;

	if (pIoctlSec->Alg == RT_CMD_STA_IOCTL_SECURITY_ALG_NONE)
		ext->alg = IW_ENCODE_ALG_NONE;
	else if (pIoctlSec->Alg == RT_CMD_STA_IOCTL_SECURITY_ALG_WEP)
		ext->alg = IW_ENCODE_ALG_WEP;
	else if (pIoctlSec->Alg == RT_CMD_STA_IOCTL_SECURITY_ALG_TKIP)
		ext->alg = IW_ENCODE_ALG_TKIP;
	else if (pIoctlSec->Alg == RT_CMD_STA_IOCTL_SECURITY_ALG_CCMP)
		ext->alg = IW_ENCODE_ALG_CCMP;

	if (pIoctlSec->flags & RT_CMD_STA_IOCTL_SECURITY_DISABLED)
		encoding->flags |= IW_ENCODE_DISABLED;

	if (ext->key_len && pIoctlSec->pData)
	{
		encoding->flags |= IW_ENCODE_ENABLED;
		memcpy(ext->key, pIoctlSec->pData, ext->key_len);
	}
	
	return 0;
}

#ifdef SIOCSIWGENIE
int rt_ioctl_siwgenie(IN	PNET_DEV         dev,
			  struct iw_request_info *info,
			  union iwreq_data *wrqu, char *extra)
{
	VOID   *pAd = NULL;

	GET_PAD_FROM_NET_DEV(pAd, dev);	
	
	/*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
       	DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        return -ENETDOWN;
	}	

	return -EOPNOTSUPP;
}
#endif /* SIOCSIWGENIE */

int rt_ioctl_giwgenie(IN	PNET_DEV         dev,
			       struct iw_request_info *info,
			       union iwreq_data *wrqu, char *extra)
{
	VOID   *pAd = NULL;
	RT_CMD_STA_IOCTL_RSN_IE IoctlRsnIe, *pIoctlRsnIe = &IoctlRsnIe;

	GET_PAD_FROM_NET_DEV(pAd, dev);	
	
	/*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
       	DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        return -ENETDOWN;
	}


	pIoctlRsnIe->length = wrqu->data.length;
	pIoctlRsnIe->pRsnIe = (UCHAR *)extra;

	if (RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCGIWGENIE, 0,
						pIoctlRsnIe, 0,
						INT_MAIN) != NDIS_STATUS_SUCCESS)
		return -E2BIG;

	wrqu->data.length = pIoctlRsnIe->length;
	return 0;
}

int rt_ioctl_siwpmksa(IN	PNET_DEV         dev,
			   struct iw_request_info *info,
			   union iwreq_data *wrqu,
			   char *extra)
{
	VOID   *pAd = NULL;
	struct iw_pmksa *pPmksa = (struct iw_pmksa *)wrqu->data.pointer;
/*	INT	CachedIdx = 0, idx = 0; */
	RT_CMD_STA_IOCTL_PMA_SA IoctlPmaSa, *pIoctlPmaSa = &IoctlPmaSa;

	GET_PAD_FROM_NET_DEV(pAd, dev);	

	/*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
       	DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
        return -ENETDOWN;
	}

	if (pPmksa == NULL)
		return -EINVAL;

	DBGPRINT(RT_DEBUG_TRACE ,("===> rt_ioctl_siwpmksa\n"));


	if (pPmksa->cmd == IW_PMKSA_FLUSH)
		pIoctlPmaSa->Cmd = RT_CMD_STA_IOCTL_PMA_SA_FLUSH;
	else if (pPmksa->cmd == IW_PMKSA_REMOVE)
		pIoctlPmaSa->Cmd = RT_CMD_STA_IOCTL_PMA_SA_REMOVE;
	else if (pPmksa->cmd == IW_PMKSA_ADD)
		pIoctlPmaSa->Cmd = RT_CMD_STA_IOCTL_PMA_SA_ADD;
	else
		pIoctlPmaSa->Cmd = 0;
	pIoctlPmaSa->pBssid = (UCHAR *)pPmksa->bssid.sa_data;
	pIoctlPmaSa->pPmkid = pPmksa->pmkid;

	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCSIWPMKSA, 0,
						pIoctlPmaSa, 0, INT_MAIN);

	return 0;
}
#endif /* #if WIRELESS_EXT > 17 */

#ifdef DBG
static int
rt_private_ioctl_bbp(IN	PNET_DEV         dev, struct iw_request_info *info,
		struct iw_point *wrq, char *extra)
{
	RTMP_IOCTL_INPUT_STRUCT wrqin;
	INT					Status = 0;
    VOID       			*pAd = NULL;

	GET_PAD_FROM_NET_DEV(pAd, dev);	


	memset(extra, 0x00, IW_PRIV_SIZE_MASK);

	wrqin.u.data.pointer = wrq->pointer;
	wrqin.u.data.length = wrq->length;

	RTMP_STA_IoctlHandle(pAd, &wrqin, CMD_RTPRIV_IOCTL_BBP, 0,
						extra, IW_PRIV_SIZE_MASK, INT_MAIN);

	wrq->length = wrqin.u.data.length;

	
	DBGPRINT(RT_DEBUG_TRACE, ("<==rt_private_ioctl_bbp\n\n"));	
    
    return Status;
}
#endif /* DBG */

int rt_ioctl_siwrate(IN	PNET_DEV         dev,
			struct iw_request_info *info,
			union iwreq_data *wrqu, char *extra)
{
    VOID   *pAd = NULL;
    UINT32          rate = wrqu->bitrate.value, fixed = wrqu->bitrate.fixed;
	RT_CMD_RATE_SET CmdRate;

	GET_PAD_FROM_NET_DEV(pAd, dev);

    /*check if the interface is down */
/*	if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
	{
  		DBGPRINT(RT_DEBUG_TRACE, ("rt_ioctl_siwrate::Network is down!\n"));
    	return -ENETDOWN;   
	}    

    DBGPRINT(RT_DEBUG_TRACE, ("rt_ioctl_siwrate::(rate = %d, fixed = %d)\n", rate, fixed));
    /* rate = -1 => auto rate
       rate = X, fixed = 1 => (fixed rate X)       
    */


	CmdRate.Rate = rate;
	CmdRate.Fixed = fixed;

	if (RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCSIWRATE, 0,
							&CmdRate, 0,
							INT_MAIN) != NDIS_STATUS_SUCCESS)
		return -EOPNOTSUPP;

    return 0;
}

int rt_ioctl_giwrate(IN	PNET_DEV         dev,
			       struct iw_request_info *info,
			       union iwreq_data *wrqu, char *extra)
{
    VOID   *pAd = NULL;
/*    int rate_index = 0, rate_count = 0; */
/*    HTTRANSMIT_SETTING ht_setting; */
	ULONG Rate;

	GET_PAD_FROM_NET_DEV(pAd, dev);

    /*check if the interface is down */
/*	if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
	{
  		DBGPRINT(RT_DEBUG_TRACE, ("INFO::Network is down!\n"));
    	return -ENETDOWN;   
	}


	RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCGIWRATE, 0,
						&Rate, 0, INT_MAIN);
	wrqu->bitrate.value = Rate;
    wrqu->bitrate.disabled = 0;

    return 0;
}


INT rt28xx_sta_ioctl(
	IN	PNET_DEV	net_dev, 
	IN	INT			cmd, 
	IN	caddr_t		pData)
{
	RTMP_ADAPTER	*pAd = NULL;
	INT				retVal = 0;
	INT 			index = 0;
	POS_COOKIE		pObj;
	UCHAR			apidx=0;
	UINT32          subcmd;
	struct iwreq	*wrqin = (struct iwreq *)pData;
	RTMP_IOCTL_INPUT_STRUCT rt_wrq, *wrq = &rt_wrq;
	INT			Status = NDIS_STATUS_SUCCESS;
	UINT32			org_len;

	
	if (net_dev == NULL)
		return (EINVAL);
		
	pAd = RTMP_OS_NETDEV_GET_PRIV(net_dev);
	
	if (pAd == NULL)
	{
		/* if 1st open fail, pAd will be free; So the net_dev->priv will be NULL in 2rd open */
		return (EINVAL);
	}

	NdisZeroMemory(&rt_wrq, sizeof(struct __RTMP_IOCTL_INPUT_STRUCT));
	wrq->u.data.pointer = wrqin->u.data.pointer;
	wrq->u.data.length = wrqin->u.data.length;
	wrq->u.data.flags = wrqin->u.data.flags;
	org_len = wrq->u.data.length;


	pObj = (POS_COOKIE) pAd->OS_Cookie;

    /* determine this ioctl command is comming from which interface. */
	if ((strcmp(net_dev->dev_name,"ra0") == 0) || (strcmp(net_dev->dev_name,"rai0") == 0))
    {
		pObj->ioctl_if_type = INT_MAIN;
       	pObj->ioctl_if = MAIN_MBSSID;
    }
    else
    {	
    	return -EOPNOTSUPP;
    }


    /*check if the interface is down */
/*    if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) */
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) != NDIS_STATUS_SUCCESS)
    {
	    if (wrqin->u.data.pointer == NULL)
	    {
		    return Status;
	    }

		if (cmd != RTPRIV_IOCTL_SET)
		{
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("INFO::Network is down!\n"));
		    return -ENETDOWN;  
        }
    }
	
	switch(cmd)
	{			
		case RTPRIV_IOCTL_ATE:
			{
				/*
					ATE is always controlled by ra0
				*/
				RTMP_COM_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_ATE, 0, wrqin->ifr_name, 0);
			}
			break;

        case SIOCGIFHWADDR:
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("IOCTL::SIOCGIFHWADDR\n"));
/*			memcpy(wrqin->u.name, pAd->CurrentAddress, ETH_ALEN); */
			RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCGIFHWADDR,
							0, wrqin->u.data.pointer, 0, RT_DEV_PRIV_FLAGS_GET(net_dev));
			wrq->u.data.length += ETH_ALEN*2 + 5;
			break;	
//#ifdef SHT_TO_DO			
		case SIOCGIWNAME:
        {
        	char *name=&wrqin->u.name[0];
        	rt_ioctl_giwname(net_dev, NULL, name, NULL);
            break;
		}
		case SIOCGIWESSID:  /*Get ESSID */
        {
        	struct iw_point *essid=&wrqin->u.essid;
        	rt_ioctl_giwessid(net_dev, NULL, essid, essid->pointer);
            break;
		}
		case SIOCSIWESSID:  /*Set ESSID */
        	{
        	struct iw_point	*essid=&wrqin->u.essid;
        	rt_ioctl_siwessid(net_dev, NULL, essid, essid->pointer);
            break;  
		}
		case SIOCSIWNWID:   /* set network id (the cell) */
		case SIOCGIWNWID:   /* get network id */
			Status = -EOPNOTSUPP;
			break;
		case SIOCSIWFREQ:   /*set channel/frequency (Hz) */
        	{
        	struct iw_freq *freq=&wrqin->u.freq;
        	rt_ioctl_siwfreq(net_dev, NULL, freq, NULL);
			break;
		}
		case SIOCGIWFREQ:   /* get channel/frequency (Hz) */
        	{
        	struct iw_freq *freq=&wrqin->u.freq;
        	rt_ioctl_giwfreq(net_dev, NULL, freq, NULL);
			break;
		}
		case SIOCSIWNICKN: /*set node name/nickname */
        	{
        	/*struct iw_point *data=&wrq->u.data; */
        	/*rt_ioctl_siwnickn(net_dev, NULL, data, NULL); */
			break;
			}
		case SIOCGIWNICKN: /*get node name/nickname */
        {
			RT_CMD_STA_IOCTL_NICK_NAME NickName, *pNickName = &NickName;
			CHAR nickname[IW_ESSID_MAX_SIZE+1];
			struct iw_point	*erq = NULL;
        	erq = &wrqin->u.data;

			pNickName->NameLen = IW_ESSID_MAX_SIZE+1;
			pNickName->pName = (CHAR *)nickname;
			RTMP_STA_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_STA_SIOCGIWNICKN, 0,
							pNickName, 0, RT_DEV_PRIV_FLAGS_GET(net_dev));

            erq->length = pNickName->NameLen; /*strlen((RTMP_STRING *) pAd->nickname); */
            Status = copy_to_user(erq->pointer, nickname, erq->length);
			break;
		}
		case SIOCGIWRATE:   /*get default bit rate (bps) */
		    rt_ioctl_giwrate(net_dev, NULL, &wrqin->u, NULL);
            break;
	    case SIOCSIWRATE:  /*set default bit rate (bps) */
	        rt_ioctl_siwrate(net_dev, NULL, &wrqin->u, NULL);
            break;
        case SIOCGIWRTS:  /* get RTS/CTS threshold (bytes) */
        	{
        	struct iw_param *rts=&wrqin->u.rts;
        	rt_ioctl_giwrts(net_dev, NULL, rts, NULL);
            break;
		}
        case SIOCSIWRTS:  /*set RTS/CTS threshold (bytes) */
        	{
        	struct iw_param *rts=&wrqin->u.rts;
        	rt_ioctl_siwrts(net_dev, NULL, rts, NULL);
            break;
		}
        case SIOCGIWFRAG:  /*get fragmentation thr (bytes) */
        	{
        	struct iw_param *frag=&wrqin->u.frag;
        	rt_ioctl_giwfrag(net_dev, NULL, frag, NULL);
            break;
		}
        case SIOCSIWFRAG:  /*set fragmentation thr (bytes) */
        	{
        	struct iw_param *frag=&wrqin->u.frag;
        	rt_ioctl_siwfrag(net_dev, NULL, frag, NULL);
            break;
		}
        case SIOCGIWENCODE:  /*get encoding token & mode */
        	{
        	struct iw_point *erq=&wrqin->u.encoding;
        	if(erq)
        		rt_ioctl_giwencode(net_dev, NULL, erq, erq->pointer);
            break;
		}
        case SIOCSIWENCODE:  /*set encoding token & mode */
        	{
        	struct iw_point *erq=&wrqin->u.encoding;
        	if(erq)
        		rt_ioctl_siwencode(net_dev, NULL, erq, erq->pointer);
            break;
		}
		case SIOCGIWAP:     /*get access point MAC addresses */
        	{
        	struct sockaddr *ap_addr=&wrqin->u.ap_addr;
        	rt_ioctl_giwap(net_dev, NULL, ap_addr, ap_addr->sa_data);
			break;
		}
	    case SIOCSIWAP:  /*set access point MAC addresses */
        	{
        	struct sockaddr *ap_addr=&wrqin->u.ap_addr;
        	rt_ioctl_siwap(net_dev, NULL, ap_addr, ap_addr->sa_data);
            break;
		}
		case SIOCGIWMODE:   /*get operation mode */
        	{
        	__u32 *mode=&wrqin->u.mode;
        	rt_ioctl_giwmode(net_dev, NULL, mode, NULL);
            break;
		}
		case SIOCSIWMODE:   /*set operation mode */
        	{
        	__u32 *mode=&wrqin->u.mode;
        	rt_ioctl_siwmode(net_dev, NULL, mode, NULL);
            break;
		}
		case SIOCGIWSENS:   /*get sensitivity (dBm) */
		case SIOCSIWSENS:	/*set sensitivity (dBm) */
		case SIOCGIWPOWER:  /*get Power Management settings */
		case SIOCSIWPOWER:  /*set Power Management settings */
		case SIOCGIWTXPOW:  /*get transmit power (dBm) */
		case SIOCSIWTXPOW:  /*set transmit power (dBm) */
		case SIOCGIWRANGE:	/*Get range of parameters */
		case SIOCGIWRETRY:	/*get retry limits and lifetime */
		case SIOCSIWRETRY:	/*set retry limits and lifetime */
			Status = -EOPNOTSUPP;
			break;
//#endif
		case RT_PRIV_IOCTL:
#ifdef RT_CFG80211_ANDROID_PRIV_LIB_SUPPORT
			//YF: Android Private Lib Entry
			rt_android_private_command_entry(pAd, net_dev, rq, cmd); 
			break;
#endif /* RT_CFG80211_ANDROID_PRIV_LIB_SUPPORT */

        case RT_PRIV_IOCTL_EXT:
			subcmd = wrqin->u.data.flags;

			Status = RTMP_STA_IoctlHandle(pAd, wrq, CMD_RT_PRIV_IOCTL, subcmd,
										NULL, 0, RT_DEV_PRIV_FLAGS_GET(net_dev));
			break;		
		/*case SIOCGIWPRIV:
			if (wrqin->u.data.pointer) 
			{
				if ( access_ok(VERIFY_WRITE, wrqin->u.data.pointer, sizeof(privtab)) != TRUE)
					break;
				if ((sizeof(privtab) / sizeof(privtab[0])) <= wrq->u.data.length)
				{
					wrqin->u.data.length = sizeof(privtab) / sizeof(privtab[0]);
					if (copy_to_user(wrqin->u.data.pointer, privtab, sizeof(privtab)))
						Status = -EFAULT;
				}
				else
					Status = -E2BIG;
			}
			break;*/
		/*case RTPRIV_IOCTL_SET:
			if(access_ok(VERIFY_READ, wrqin->u.data.pointer, wrqin->u.data.length) != TRUE)   
					break;
			return rt_ioctl_setparam(net_dev, NULL, NULL, wrqin->u.data.pointer);
			break;*/
			
		case RTPRIV_IOCTL_SET:
			{	
				Status = rt_ioctl_setparam(net_dev, NULL, NULL, wrqin->u.data.pointer);
			}
			break;
		    		
		case RTPRIV_IOCTL_GSITESURVEY:
			RTMP_STA_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_SITESURVEY_GET, 0,
								NULL, 0, RT_DEV_PRIV_FLAGS_GET(net_dev));
/*			RTMPIoctlGetSiteSurvey(pAd, wrq); */
		    break;			
#ifdef DBG
		case RTPRIV_IOCTL_MAC:
			RTMP_STA_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_MAC, 0,
								NULL, 0, RT_DEV_PRIV_FLAGS_GET(net_dev));
/*			RTMPIoctlMAC(pAd, wrq); */
			break;
		case RTPRIV_IOCTL_E2P:
			RTMP_STA_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_E2P, 0,
								NULL, 0, RT_DEV_PRIV_FLAGS_GET(net_dev));
/*			RTMPIoctlE2PROM(pAd, wrq); */
			break;
#ifdef RTMP_RF_RW_SUPPORT
		case RTPRIV_IOCTL_RF:
			RTMP_STA_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_RF, 0,
								NULL, 0, RT_DEV_PRIV_FLAGS_GET(net_dev));
/*			RTMPIoctlRF(pAd, wrq); */
			break;
#endif /* RTMP_RF_RW_SUPPORT */
#endif /* DBG */

     //   case SIOCETHTOOL:
     //           break;
		default:
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("IOCTL::unknown IOCTL's cmd = 0x%08x\n", cmd));
			Status = -EOPNOTSUPP;
			break;
	}

	if (Status != 0)
	{
		RT_CMD_STATUS_TRANSLATE(Status);
	}
	else
	{
		/*
			If wrq length is modified, we reset the lenght of origin wrq;

			Or we can not modify it because the address of wrq->u.data.length
			maybe same as other union field, ex: iw_range, etc.

			if the length is not changed but we change it, the value for other
			union will also be changed, this is not correct.
		*/
		if (wrq->u.data.length != org_len)
			wrqin->u.data.length = wrq->u.data.length;
	}

	return Status;
}
