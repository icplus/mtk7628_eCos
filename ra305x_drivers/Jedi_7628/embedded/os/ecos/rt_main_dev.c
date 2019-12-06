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
    rt_main_end.c

    Abstract:
    Create and register network interface.

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#include "rt_config.h"


/*
========================================================================
Routine Description:
    Close raxx interface.

Arguments:
	*net_dev			the raxx interface pointer

Return Value:
    0					Open OK
	otherwise			Open Fail

Note:
	1. if open fail, kernel will not call the close function.
	2. Free memory for
		(1) Mlme Memory Handler:		MlmeHalt()
		(2) TX & RX:					RTMPFreeTxRxRingMemory()
		(3) BA Reordering: 				ba_reordering_resource_release()
========================================================================
*/
int rt28xx_close(IN VOID *dev)
{
#ifdef CONFIG_PCI_SUPPORT	
	extern int	pcie_int_line;
#endif 
	PNET_DEV net_dev = (PNET_DEV)dev;
	RTMP_ADAPTER	*pAd = NULL;
	POS_COOKIE		pOSCookie = NULL;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("===> rt28xx_close\n"));

	/* Sanity check for pAd */
	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(net_dev);	
	if (pAd == NULL)
		return 0; /* close ok */

	/* Clear handle interrupts */
#ifdef CONFIG_PCI_SUPPORT	
	if (pAd->infType == RTMP_DEV_INF_PCIE)
		cyg_drv_interrupt_mask(pcie_int_line);
	else	
#endif 
		cyg_drv_interrupt_mask(RTMP_INTERRUPT_INIC);

	/* Delete the message queue */
	pOSCookie = (POS_COOKIE) pAd->OS_Cookie;
	cyg_mbox_delete(pOSCookie->nettask_handle);

	RTMPDrvClose(pAd, net_dev);


	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<=== rt28xx_close\n"));
	
	return 0; /* close ok */
}


/*
========================================================================
Routine Description:
    Open raxx interface.

Arguments:
	*net_dev			the raxx interface pointer

Return Value:
    0					Open OK
	otherwise			Open Fail

Note:
========================================================================
*/

int rt28xx_open(IN VOID *dev)
{				 
#ifdef CONFIG_PCI_SUPPORT	
	extern int	pcie_int_line;
#endif 

	PNET_DEV net_dev = (PNET_DEV)dev;
	RTMP_ADAPTER	*pAd = NULL;
	POS_COOKIE		pOSCookie = NULL;    
	int i, retval = 0;
	ULONG OpMode;

//	if (sizeof(ra_dma_addr_t) < sizeof(dma_addr_t))
//		DBGPRINT(RT_DEBUG_ERROR, ("Fatal error for DMA address size!!!\n"));


	/* Sanity check for pAd */
	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(net_dev);
	if (pAd == NULL)
	{
		/* if 1st open fail, pAd will be free; So the net_dev->priv will be NULL in 2rd open */
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("pAd is NULL!\n"));
		return -1;
	}

	RTMP_DRIVER_MCU_SLEEP_CLEAR(pAd);

	//RTMP_DRIVER_OP_MODE_GET(pAd, &OpMode);

	/* Init IRQ parameters */
	RTMP_DRIVER_IRQ_INIT(pAd);
	
	/* Create the message queue */	//comment:memory leakage here when init interface.every interface create one mbox.
	pOSCookie = (POS_COOKIE) pAd->OS_Cookie;
	cyg_mbox_create(&pOSCookie->nettask_handle, &pOSCookie->nettask_mbox);
	

	/* Chip & other init */
	if (rt28xx_init(pAd, NULL, NULL) == FALSE)
		goto err;

#ifdef MBSS_SUPPORT
	RT28xx_MBSS_Init(pAd, (PNET_DEV)net_dev);
#endif /* MBSS_SUPPORT */
#ifdef WDS_SUPPORT
		RT28xx_WDS_Init(pAd, (PNET_DEV)net_dev);
#endif /* WDS_SUPPORT */

#ifdef APCLI_SUPPORT
	RT28xx_ApCli_Init(pAd, (PNET_DEV)net_dev);
#endif /* APCLI_SUPPORT */

#ifdef CONFIG_SNIFFER_SUPPORT
	RT28xx_Monitor_Init(pAd, net_dev);
#endif /* CONFIG_SNIFFER_SUPPORT */


	RTMPDrvOpen(pAd);

	/* Set handle interrupts */
#ifdef CONFIG_PCI_SUPPORT	
	if (pAd->infType == RTMP_DEV_INF_PCIE)
		cyg_drv_interrupt_unmask(pcie_int_line);
	else
#endif /* SECOND_WIFI */
		cyg_drv_interrupt_unmask(RTMP_INTERRUPT_INIC);

	return (retval);

err:
	RTMP_DRIVER_IRQ_RELEASE(pAd);
	return (-1);
}


/*
========================================================================
Routine Description:
	Early check if the packet has enough packet headeroom for latter handling.

Arguments:
	pEndObj		the end object.
	pMblk		the pointer refer to a packet buffer
	
Return Value:
	packet buffer pointer:	if sanity check success.
	NULL:				if failed

Note:
	This function is the entry point of Tx Path for Os delivery packet to 
	our driver. You only can put OS-depened & STA/AP common handle procedures 
	in here.
========================================================================
*/

static inline void RtmpVxNetPktCheck(
	IN PRTMP_ADAPTER pAd)
/*static inline M_BLK_ID RtmpVxNetPktCheck( */
	/*IN END_OBJ *pEndObj, */
	/*IN M_BLK_ID pMblk) */
{
}

/*
========================================================================
Routine Description:
    The entry point for Linux kernel sent packet to our driver.

Arguments:
    sk_buff *skb		the pointer refer to a sk_buffer.

Return Value:
    0					

Note:
	This function is the entry point of Tx Path for Os delivery packet to 
	our driver. You only can put OS-depened & STA/AP common handle procedures 
	in here.
========================================================================
*/
int rt28xx_packet_xmit(void *pPacketSrc)
{
	PECOS_PKT_BUFFER pPacket = (PECOS_PKT_BUFFER)pPacketSrc;
	PNET_DEV net_dev = pPacket->net_dev;
	PRTMP_ADAPTER   pAd;
	struct wifi_dev *wdev;

	wdev = RTMP_OS_NETDEV_GET_WDEV(net_dev);
	ASSERT(wdev);

	return RTMPSendPackets((NDIS_HANDLE)wdev, (PPNDIS_PACKET) &pPacket, 1,
							pPacket->pktLen, NULL);	
}

/*
========================================================================
Routine Description:
    Send a packet to WLAN.

Arguments:
    skb_p           points to our adapter
    dev_p           which WLAN network interface

Return Value:
    0: transmit successfully
    otherwise: transmit fail

Note:
========================================================================
*/
int rt28xx_send_packets(PECOS_PKT_BUFFER skb, PNET_DEV ndev)
{
	if (!(RTMP_OS_NETDEV_STATE_RUNNING(ndev)))
	{
		RELEASE_NDIS_PACKET(NULL, (PNDIS_PACKET)skb, NDIS_STATUS_FAILURE);
		return 0;
	}

	skb->net_dev = ndev;
	NdisZeroMemory((PUCHAR)&skb->cb[CB_OFF], 26);
	MEM_DBG_PKT_ALLOC_INC(skb);

	return rt28xx_packet_xmit(skb);
}



void tbtt_tasklet_old(unsigned long data)
{

/*#define MAX_TX_IN_TBTT		(16) */

#ifdef CONFIG_AP_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) data;

#ifdef RTMP_MAC_PCI
	if (pAd->OpMode == OPMODE_AP)
	{
		/* update channel utilization */
		QBSS_LoadUpdate(pAd, 0);

	}
#endif /* RTMP_MAC_PCI */

	if (pAd->OpMode == OPMODE_AP)
	{
		/* */
		/* step 7 - if DTIM, then move backlogged bcast/mcast frames from PSQ to TXQ whenever DtimCount==0 */
		if (pAd->ApCfg.DtimCount == 0)
		{
			PQUEUE_ENTRY    pEntry;
			BOOLEAN			bPS = FALSE;
			UINT 			count = 0;
			unsigned long 		IrqFlags;
			
			RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
			while (pAd->MacTab.McastPsQueue.Head)
			{
				bPS = TRUE;
				if (pAd->TxSwQueue[QID_AC_BE].Number <= (pAd->TxSwQMaxLen + MAX_PACKETS_IN_MCAST_PS_QUEUE))
				{
					pEntry = RemoveHeadQueue(&pAd->MacTab.McastPsQueue);
					/*if(pAd->MacTab.McastPsQueue.Number) */
					if (count)
					{
						RTMP_SET_PACKET_MOREDATA(pEntry, TRUE);
					}
					InsertHeadQueue(&pAd->TxSwQueue[QID_AC_BE], pEntry);
					count++;
				}
				else
				{
					break;
				}
			}
			RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
						
			if (pAd->MacTab.McastPsQueue.Number == 0)
			{
				UINT bss_index;

				/* clear MCAST/BCAST backlog bit for all BSS */
				for(bss_index=BSS0; bss_index<pAd->ApCfg.BssidNum; bss_index++)
					WLAN_MR_TIM_BCMC_CLEAR(bss_index);
			}
			pAd->MacTab.PsQIdleCount = 0;

			/* Dequeue outgoing framea from TxSwQueue0..3 queue and process it */
			if (bPS == TRUE) 
			{
				// TODO: shiang-usw, modify the WCID_ALL to pMBss->tr_entry because we need to tx B/Mcast frame here!!
				RTMPDeQueuePacket(pAd, FALSE, NUM_OF_TX_RING, WCID_ALL, /*MAX_TX_IN_TBTT*/MAX_PACKETS_IN_MCAST_PS_QUEUE);
			}
		}

	}
#endif /* CONFIG_AP_SUPPORT */
}

void tbtt_tasklet(unsigned long data)
{
#ifdef CONFIG_AP_SUPPORT
/*#ifdef WORKQUEUE_BH
	struct work_struct *work = (struct work_struct *)data;
	POS_COOKIE pObj = container_of(work, struct os_cookie, tbtt_task);
	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)pObj->pAd_va;
#else*/
		PRTMP_ADAPTER pAd = (RTMP_ADAPTER *)data;
//#endif /* WORKQUEUE_BH */

#ifdef RTMP_MAC_PCI
	if (pAd->OpMode == OPMODE_AP)
	{
#ifdef AP_QLOAD_SUPPORT
		/* update channel utilization */
		QBSS_LoadUpdate(pAd, 0);
#endif /* AP_QLOAD_SUPPORT */

	}
#endif /* RTMP_MAC_PCI */

#ifdef RT_CFG80211_P2P_SUPPORT
		if (pAd->cfg80211_ctrl.isCfgInApMode == RT_CMD_80211_IFTYPE_AP)	
#else
	if (pAd->OpMode == OPMODE_AP)
#endif /* RT_CFG80211_P2P_SUPPORT */
	{
		/* step 7 - if DTIM, then move backlogged bcast/mcast frames from PSQ to TXQ whenever DtimCount==0 */
#ifdef RTMP_MAC_PCI
		/* 
			NOTE:
			This updated BEACON frame will be sent at "next" TBTT instead of at cureent TBTT. The reason is
			because ASIC already fetch the BEACON content down to TX FIFO before driver can make any
			modification. To compenstate this effect, the actual time to deilver PSQ frames will be
			at the time that we wrapping around DtimCount from 0 to DtimPeriod-1
		*/
		if (pAd->ApCfg.DtimCount == 0)
#endif /* RTMP_MAC_PCI */
		{
			QUEUE_ENTRY *pEntry;
			BOOLEAN bPS = FALSE;
			UINT count = 0;
			unsigned long IrqFlags;
			
#ifdef MT_MAC
			UINT apidx = 0, mac_val = 0, deq_cnt = 0;
			
			if ((pAd->chipCap.hif_type == HIF_MT) && (pAd->MacTab.fAnyStationInPsm == TRUE))
			{
#ifdef USE_BMC
				#define MAX_BMCCNT 16
				int fcnt = 0, max_bss_cnt = 0;

				/* BMC Flush */
				mac_val = 0x7fff0001;
				RTMP_IO_WRITE32(pAd, ARB_BMCQCR1, mac_val);
				
				for (fcnt=0;fcnt<100;fcnt++)
				{
				RTMP_IO_READ32(pAd, ARB_BMCQCR1, &mac_val);
					if (mac_val == 0)
						break;
				}
				
				if (fcnt == 100)
				{
					MTWF_LOG(DBG_CAT_ALL,DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: flush not complete, flush cnt=%d\n", __FUNCTION__, fcnt));
					return;
				}
				
				if ((pAd->ApCfg.BssidNum == 0) || (pAd->ApCfg.BssidNum == 1))
				{
					max_bss_cnt = 0xf;
				}
				else
				{
					max_bss_cnt = MAX_BMCCNT / pAd->ApCfg.BssidNum;
				}
#endif
				for(apidx=0;apidx<pAd->ApCfg.BssidNum;apidx++)		
            	{
	                BSS_STRUCT *pMbss;
					UINT wcid = 0, PseFcnt = 0, cnt = 0, bmc_cnt = 0;
					STA_TR_ENTRY *tr_entry = NULL;
			
					pMbss = &pAd->ApCfg.MBSSID[apidx];
				
					wcid = pMbss->wdev.tr_tb_idx;
					tr_entry = &pAd->MacTab.tr_entry[wcid]; 
				
					if (tr_entry->tx_queue[QID_AC_BE].Head != NULL)
					{
#ifdef USE_BMC
						if ((apidx >= 0) && (apidx <= 4))
						{
							RTMP_IO_READ32(pAd, ARB_BMCQCR2, &mac_val);
							if (apidx == 0)
								bmc_cnt = mac_val & 0xf;
							else 
								bmc_cnt = (mac_val >> (12+ (4*apidx))) & 0xf;
						}
						else if ((apidx >= 5) && (apidx <= 12))
						{
							RTMP_IO_READ32(pAd, ARB_BMCQCR3, &mac_val);
							bmc_cnt = (mac_val >> (4*(apidx-5))) & 0xf;
						}
						else if ((apidx >=13) && (apidx <= 15))
						{
							RTMP_IO_READ32(pAd, ARB_BMCQCR3, &mac_val);
							bmc_cnt = (mac_val >> (4*(apidx-13))) & 0xf;
						}
						else
						{
							MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL,DBG_LVL_ERROR, ("%s: apidx(%d) not support\n", __FUNCTION__, apidx));
		                    return;
						}	
						
						if (bmc_cnt >= max_bss_cnt)
							deq_cnt = 0;
						else
							deq_cnt = max_bss_cnt - bmc_cnt;
						
						if (tr_entry->tx_queue[QID_AC_BE].Number <= deq_cnt)
#endif /* USE_BMC */
							deq_cnt = tr_entry->tx_queue[QID_AC_BE].Number;
					
						RTMPDeQueuePacket(pAd, FALSE, QID_AC_BE, wcid, deq_cnt); 
			
						MTWF_LOG(DBG_CAT_ALL,DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s: bss:%d, deq_cnt = %d\n", __FUNCTION__, apidx, deq_cnt));
					}
			
					if (WLAN_MR_TIM_BCMC_GET(apidx) == 0x01)
					{
						if  ( (tr_entry->tx_queue[QID_AC_BE].Head == NULL) && 
							(tr_entry->EntryType == ENTRY_CAT_MCAST))
						{
							WLAN_MR_TIM_BCMC_CLEAR(tr_entry->func_tb_idx);	/* clear MCAST/BCAST TIM bit */
							MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL,DBG_LVL_WARN | DBG_FUNC_UAPSD, ("%s: clear MCAST/BCAST TIM bit \n", __FUNCTION__));
						}
					}
				}
#ifdef USE_BMC				
				/* BMC start */
				RTMP_IO_WRITE32(pAd, ARB_BMCQCR0, 0x7fff0001);
#endif				
			}
#else /* MT_MAC */
			RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
			while (pAd->MacTab.McastPsQueue.Head)
			{
				bPS = TRUE;
				if (pAd->TxSwQueue[QID_AC_BE].Number <= (pAd->TxSwQMaxLen + MAX_PACKETS_IN_MCAST_PS_QUEUE))
				{
					pEntry = RemoveHeadQueue(&pAd->MacTab.McastPsQueue);
					/*if(pAd->MacTab.McastPsQueue.Number) */
					if (count)
					{
						RTMP_SET_PACKET_MOREDATA(pEntry, TRUE);
						RTMP_SET_PACKET_TXTYPE(pEntry, TX_LEGACY_FRAME);
					}
					InsertHeadQueue(&pAd->TxSwQueue[QID_AC_BE], pEntry);
					count++;
				}
				else
				{
					break;
				}
			}
			RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
			

			if (pAd->MacTab.McastPsQueue.Number == 0)
			{			
		                UINT bss_index;

                		/* clear MCAST/BCAST backlog bit for all BSS */
				for(bss_index=BSS0; bss_index<pAd->ApCfg.BssidNum; bss_index++)
					WLAN_MR_TIM_BCMC_CLEAR(bss_index);
			}
			pAd->MacTab.PsQIdleCount = 0;

			if (bPS == TRUE) 
			{
				// TODO: shiang-usw, modify the WCID_ALL to pMBss->tr_entry because we need to tx B/Mcast frame here!!
				RTMPDeQueuePacket(pAd, FALSE, NUM_OF_TX_RING, WCID_ALL, /*MAX_TX_IN_TBTT*/MAX_PACKETS_IN_MCAST_PS_QUEUE);
			}
#endif /* !MT_MAC */			
		}
	}
#endif /* CONFIG_AP_SUPPORT */
}


INT rt28xx_ioctl(
	IN	PNET_DEV	endDev, 
	IN	caddr_t		data,
	IN	int			cmd)
{
	RTMP_ADAPTER	*pAd = NULL;
	INT				ret = 0;
	ULONG OpMode;

	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(endDev);
	if (pAd == NULL)
	{
		/* if 1st open fail, pAd will be free;
		   So the net_dev->priv will be NULL in 2rd open */
		return -ENETDOWN;
	}

	RTMP_DRIVER_OP_MODE_GET(pAd, &OpMode);

#ifdef CONFIG_AP_SUPPORT
	RT_CONFIG_IF_OPMODE_ON_AP(OpMode)
	{
		ret = rt28xx_ap_ioctl(endDev, cmd, data);
	}
#endif /* CONFIG_AP_SUPPORT */


	return ret;
}


/*
    ========================================================================

    Routine Description:
        return ethernet statistics counter

    Arguments:
        net_dev                     Pointer to net_device

    Return Value:
        net_device_stats*

    Note:

    ========================================================================
*/
NET_DEV_STATS *RT28xx_get_ether_stats(PNET_DEV net_dev)
{
	return NULL;
}

/*
========================================================================
Routine Description:
    Allocate memory for adapter control block.

Arguments:
    pAd					Pointer to our adapter

Return Value:
	NDIS_STATUS_SUCCESS
	NDIS_STATUS_FAILURE
	NDIS_STATUS_RESOURCES

Note:
========================================================================
*/
NDIS_STATUS AdapterBlockAllocateMemory(
	IN	PVOID					handle,
	OUT	PVOID					*ppAd,
	IN	UINT32					SizeOfpAd)
{
	*ppAd = (PVOID)kmalloc(sizeof(RTMP_ADAPTER), GFP_ATOMIC);

	if (*ppAd) 
	{
		NdisZeroMemory((PCHAR) *ppAd, sizeof(RTMP_ADAPTER));
		((PRTMP_ADAPTER)*ppAd)->OS_Cookie = handle;
		return (NDIS_STATUS_SUCCESS);
	}
	else
	{
		return (NDIS_STATUS_FAILURE);
	}
}

BOOLEAN RtmpPhyNetDevExit(
	IN RTMP_ADAPTER *pAd, 
	IN PNET_DEV net_dev)
{
	/*END_OBJ *pEndDev; */

	/*pEndDev = (END_OBJ *)net_dev; */

#ifdef CONFIG_SNIFFER_SUPPORT
	RT28xx_Monitor_Remove(pAd);
#endif	/* CONFIG_SNIFFER_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
	/* remove all AP-client virtual interfaces. */
	RT28xx_ApCli_Remove(pAd);
#endif /* APCLI_SUPPORT */

#ifdef WDS_SUPPORT
	/* remove all WDS virtual interfaces. */
	RT28xx_WDS_Remove(pAd);
#endif /* WDS_SUPPORT */

#ifdef MBSS_SUPPORT
	RT28xx_MBSS_Remove(pAd);
#endif /* MBSS_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

	/* Unregister network device */
	if (net_dev != NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("RtmpOSNetDevDetach(): RtmpOSNetDeviceDetach(), dev->name=%s!\n", 
					net_dev->dev_name));
		RtmpOSNetDevDetach(net_dev);
	}

	return TRUE;
}

