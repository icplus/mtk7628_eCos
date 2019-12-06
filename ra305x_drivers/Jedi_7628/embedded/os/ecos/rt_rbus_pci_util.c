/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	rt_rbus_pci_util.c

	Abstract:

	Revision History:
	Who			When		What
    --------    ----------      ------------------------------------------
*/

#include "rt_config.h"

/***********************************************************************************
 *	Memory pool implement
 ***********************************************************************************/ 
#define MemPool_TYPE_Header 3
#define MemPool_TYPE_MBUF 4
#define BUFFER_HEAD_RESVERD 64


PUCHAR RTMP_MemPool_Alloc (
	IN PRTMP_ADAPTER pAd,
	IN ULONG Length,
	IN INT Type)
{
	/* Note: pAd maybe NULL */
    struct mbuf *pMBuf = NULL;

#ifdef ECOS_DEBUG
#ifdef CONFIG_NET_CLUSTER_SIZE_2048
	if(Length > 2048) panic("RTMP_MemPool_Alloc > 2048");
#else
	if(Length > 4096) panic("RTMP_MemPool_Alloc > 4096");
#endif

#endif

	switch (Type)
	{        
		case MemPool_TYPE_Header:
			MGETHDR(pMBuf, M_DONTWAIT, MT_DATA);
			if (pMBuf== NULL)
			{
				//diag_printf("%s %d MGETHDR fail!\n", __FUNCTION__,__LINE__);
				return NULL;
			}
            pMBuf->m_len = 0;
            break;
		case MemPool_TYPE_MBUF:
			MGETHDR(pMBuf, M_DONTWAIT, MT_DATA);                        
			if (pMBuf== NULL)
			{
				//diag_printf("%s %d MGETHDR fail!\n", __FUNCTION__,__LINE__);
				return NULL;
			}	
			MCLGET(pMBuf, M_DONTWAIT);
			if ((pMBuf->m_flags & M_EXT) == 0)
            {
				m_freem(pMBuf);
				//diag_printf("%s %d MCLGET fail!\n", __FUNCTION__,__LINE__);
                return NULL;
            }
            pMBuf->m_len = 0;
            break;
        default:
	        diag_printf(("%s: Unknown Type %d\n", __FUNCTION__, Type));
    		break;
	}

	if (pMBuf != NULL)
		MEM_DBG_PKT_ALLOC_INC(pAd);

    return (PUCHAR)pMBuf;
}

VOID RTMP_MemPool_Free (
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pBuffer,
	IN INT Type)
{
	/* Note: pAd maybe NULL */

	if (pBuffer == NULL)
	{
		DBGPRINT_ERR(("%s: Error buffer pointer \n", __FUNCTION__));
       	goto exit;
	}

	switch (Type)
	{
        case MemPool_TYPE_Header:
            m_freem(pBuffer);
			MEM_DBG_PKT_FREE_INC(pAd);
            break;
        case MemPool_TYPE_MBUF:
            m_freem(pBuffer);
			MEM_DBG_PKT_FREE_INC(pAd);
            break;
        default:
	        DBGPRINT_ERR(("%s: Unknown Type %d\n", __FUNCTION__, Type));
            break;
    }
    
exit:
	return;
}

#ifdef CONFIG_NET_CLUSTER_GROUP2
extern  struct	mbstat mbstat_4k;
extern  union	mcluster_4k *mclfree_4k;

PUCHAR RTMP_MemPool_Alloc_4k (
	IN PRTMP_ADAPTER pAd,
	IN ULONG Length,
	IN INT Type)
{
	/* Note: pAd maybe NULL */
    struct mbuf *pMBuf = NULL;

#ifdef ECOS_DEBUG
/*
#ifdef CONFIG_NET_CLUSTER_SIZE_2048
	if(Length > 2048) panic("RTMP_MemPool_Alloc > 2048");
#else
*/
	if(Length > NET_CLUSTER_GROUP2_SIZE) panic("RTMP_MemPool_Alloc > NET_CLUSTER_GROUP2_SIZE");
//#endif

#endif

	switch (Type)
	{        
		case MemPool_TYPE_Header:
			MGETHDR(pMBuf, M_DONTWAIT, MT_DATA);
			if (pMBuf== NULL)
			{
				//diag_printf("%s %d MGETHDR fail!\n", __FUNCTION__,__LINE__);
				return NULL;
			}
            pMBuf->m_len = 0;
            break;
		case MemPool_TYPE_MBUF:
			MGETHDR(pMBuf, M_DONTWAIT, MT_DATA);                        
			if (pMBuf== NULL)
			{
				//diag_printf("%s %d MGETHDR fail!\n", __FUNCTION__,__LINE__);
				return NULL;
			}	
			MCLGET_4k(pMBuf, M_DONTWAIT);
			if ((pMBuf->m_flags & M_EXT) == 0)
            {
				m_freem_4k(pMBuf);
				//diag_printf("%s %d MCLGET fail!\n", __FUNCTION__,__LINE__);
                return NULL;
            }
            pMBuf->m_len = 0;
            break;
        default:
	        diag_printf(("%s: Unknown Type %d\n", __FUNCTION__, Type));
    		break;
	}

	if (pMBuf != NULL)
		MEM_DBG_PKT_ALLOC_INC(pAd);

    return (PUCHAR)pMBuf;
}

VOID RTMP_MemPool_Free_4k(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pBuffer,
	IN INT Type)
{
	/* Note: pAd maybe NULL */

	if (pBuffer == NULL)
	{
		DBGPRINT_ERR(("%s: Error buffer pointer \n", __FUNCTION__));
       	goto exit;
	}

	switch (Type)
	{
        case MemPool_TYPE_Header:
            m_freem_4k(pBuffer);
			MEM_DBG_PKT_FREE_INC(pAd);
            break;
        case MemPool_TYPE_MBUF:
            m_freem_4k(pBuffer);
			MEM_DBG_PKT_FREE_INC(pAd);
            break;
        default:
	        DBGPRINT_ERR(("%s: Unknown Type %d\n", __FUNCTION__, Type));
            break;
    }
    
exit:
	return;
}

#endif


#define DescMBuf_SIZE (NUM_OF_TX_RING*3)

static char DescMBuf[DescMBuf_SIZE][MGMT_RING_SIZE * TXD_SIZE];
//comment: should be care about MGMT_RING_SIZE/TX_RING_SIZE/RX_RING_SIZE value. set the biggest.

static INT DescMBuf_Index = 0;

/* Function for Tx/Rx/Mgmt Desc Memory allocation. */
void RtmpAllocDescBuf(
	IN PPCI_DEV pPciDev,
	IN UINT Index,
	IN ULONG Length,
	IN BOOLEAN Cached,
	OUT VOID **VirtualAddress,
	OUT PNDIS_PHYSICAL_ADDRESS PhysicalAddress)
{
	if(Length > MGMT_RING_SIZE * TXD_SIZE)	//comment: should be care about MGMT_RING_SIZE/TX_RING_SIZE/RX_RING_SIZE value. set the biggest.
		panic("RtmpAllocDescBuf Length over TX_RING_SIZE * TXD_SIZE");
	
	if (Cached)
		//*VirtualAddress = (PVOID) CYGARC_CACHED_ADDRESS(&DescMBuf[DescMBuf_Index][0]);
		*VirtualAddress = (PVOID) CYGARC_UNCACHED_ADDRESS(&DescMBuf[DescMBuf_Index][0]);
	else
		*VirtualAddress = (PVOID) CYGARC_UNCACHED_ADDRESS(&DescMBuf[DescMBuf_Index][0]);

    DescMBuf_Index++;
    if (DescMBuf_Index == DescMBuf_SIZE)
    {
    	//diag_printf("%s Error! FirstTxBuffer is not enough\n",__FUNCTION__);
		panic("Error! RtmpAllocDescBuf is not enough\n");
        //FirstTx_Index = 0;		
    }	
	*PhysicalAddress = (NDIS_PHYSICAL_ADDRESS) CYGARC_PHYSICAL_ADDRESS(*VirtualAddress);	
}


/* Function for free allocated Desc Memory. */
void RtmpFreeDescBuf(
	IN	PPCI_DEV	pPciDev,
	IN	ULONG	Length,
	IN	PVOID	VirtualAddress,
	IN	NDIS_PHYSICAL_ADDRESS PhysicalAddress)
{

}


/*EddyTODO */
/* Function for TxData DMA Memory allocation. */

#define NUM_OF_FirstTxBuffer (NUM_OF_TX_RING+1+1)	//NUM_OF_FirstTxBuffer means NUM_OF_TX_RING + 1 for BMC queue use +1 for reduce.

static char FirstTx_SIZE[NUM_OF_FirstTxBuffer][TX_RING_SIZE * TX_DMA_1ST_BUFFER_SIZE];
static INT FirstTx_Index = 0;
void RTMP_AllocateFirstTxBuffer(
	IN	PPCI_DEV pPciDev,
	IN	UINT	Index,
	IN	ULONG	Length,
	IN	BOOLEAN	Cached,
	OUT	PVOID	*VirtualAddress,
	OUT	PNDIS_PHYSICAL_ADDRESS PhysicalAddress)
{
	if (Cached)
		//*VirtualAddress = (PVOID) CYGARC_CACHED_ADDRESS(&FirstTx_SIZE[FirstTx_Index][0]);
		*VirtualAddress = (PVOID) CYGARC_UNCACHED_ADDRESS(&FirstTx_SIZE[FirstTx_Index][0]);
	else
		*VirtualAddress = (PVOID) CYGARC_UNCACHED_ADDRESS(&FirstTx_SIZE[FirstTx_Index][0]);

    FirstTx_Index++;
    if (FirstTx_Index == NUM_OF_FirstTxBuffer)
    {
    	//diag_printf("%s Error! FirstTxBuffer is not enough\n",__FUNCTION__);
		panic("Error! FirstTxBuffer is not enough\n");
        //FirstTx_Index = 0;		
    }	
	*PhysicalAddress = (NDIS_PHYSICAL_ADDRESS) CYGARC_PHYSICAL_ADDRESS(*VirtualAddress);
}


void RTMP_FreeFirstTxBuffer(
	IN	PPCI_DEV				pPciDev,
	IN	ULONG	Length,
	IN	BOOLEAN	Cached,
	IN	PVOID	VirtualAddress,
	IN	NDIS_PHYSICAL_ADDRESS PhysicalAddress)
{
	//FirstTx_Index = 0;
}

PECOS_PKT_BUFFER RTMP_AllocatePacketHeader (
	IN	PRTMP_ADAPTER pAd)
{
    PECOS_PKT_BUFFER pPacket;
    struct mbuf     *pMBuf;

	pMBuf = (struct mbuf *)RTMP_MemPool_Alloc(pAd, sizeof(ECOS_PKT_BUFFER), MemPool_TYPE_Header);
	if (pMBuf == NULL)
		return NULL;
	pPacket = (PECOS_PKT_BUFFER) pMBuf->m_data;
	if (pPacket != NULL)
	{
        NdisZeroMemory(pPacket, sizeof(ECOS_PKT_BUFFER));
		pPacket->pHeaderMBuf = pMBuf;
	}

    return pPacket;
}


/*
 * FUNCTION: Allocate a packet buffer for DMA
 * ARGUMENTS:
 *     AdapterHandle:  AdapterHandle
 *     Length:  Number of bytes to allocate
 *     Cached:  Whether or not the memory can be cached
 *     VirtualAddress:  Pointer to memory is returned here
 *     PhysicalAddress:  Physical address corresponding to virtual address
 * Notes:
 *     Cached is ignored: always cached memory
 */

PNDIS_PACKET RTMP_AllocateRxPacketBuffer(
	IN	VOID					*pReserved,
	IN	VOID					*pPciDev,
	IN	ULONG	Length,
	IN	BOOLEAN	Cached,
	OUT	PVOID	*VirtualAddress,
	OUT	PNDIS_PHYSICAL_ADDRESS PhysicalAddress)
{
    PRTMP_ADAPTER	 pAd = (PRTMP_ADAPTER)pReserved;
    PECOS_PKT_BUFFER pPacket;

#ifdef ECOS_DEBUG
#ifdef CONFIG_NET_CLUSTER_SIZE_2048
	if(Length > (2048-BUFFER_HEAD_RESVERD))
	{
		panic("RTMP_AllocateRxPacketBuffer:Length over cluster size 2048!\n");
	}
#else
	if(Length > (4096-BUFFER_HEAD_RESVERD))
	{
		panic("RTMP_AllocateRxPacketBuffer:Length over cluster size 4096!\n");
    }
#endif
#endif

	pPacket = RTMP_AllocatePacketHeader(pReserved);
	if (pPacket == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN,("%s: Can't allocate packet structure\n"
		, __FUNCTION__));
        goto err;
    }

#ifdef DOT11_N_SUPPORT	//sht:comment,todo: check this define flag can remove.
    if ((mbstat.m_clfree == 0) && (pAd->ContinueMemAllocFailCount > 5))
    {
		pAd->ContinueMemAllocFailCount = 0;
        BaReOrderingBufferMaintain(pAd);
    }
#endif /* DOT11_N_SUPPORT */

    pPacket->pDataMBuf = (struct mbuf *)RTMP_MemPool_Alloc(pReserved, Length, MemPool_TYPE_MBUF);
	if (pPacket->pDataMBuf == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("%s: Can't allocate buffer from memory pool\n"
		, __FUNCTION__));
#ifdef DOT11_N_SUPPORT
                pAd->ContinueMemAllocFailCount++;
#endif /* DOT11_N_SUPPORT */
        goto release_pkt_header;
    }

    HAL_DCACHE_INVALIDATE(mtod(pPacket->pDataMBuf, PUCHAR), Length);
	if(Cached)
        //pPacket->pDataPtr = (PUCHAR) CYGARC_CACHED_ADDRESS(mtod(pPacket->pDataMBuf, PUCHAR));
		pPacket->pDataPtr = (PUCHAR) CYGARC_UNCACHED_ADDRESS(mtod(pPacket->pDataMBuf, PUCHAR));
	else
		pPacket->pDataPtr = (PUCHAR) CYGARC_UNCACHED_ADDRESS(mtod(pPacket->pDataMBuf, PUCHAR));
    NdisZeroMemory(pPacket->pDataPtr, BUFFER_HEAD_RESVERD);
    pPacket->pDataPtr += BUFFER_HEAD_RESVERD;
    pPacket->MemPoolType = MemPool_TYPE_MBUF;
    pPacket->pktLen = 0;

	*VirtualAddress = (PVOID) GET_OS_PKT_DATAPTR(pPacket);	
   	*PhysicalAddress = (NDIS_PHYSICAL_ADDRESS) CYGARC_PHYSICAL_ADDRESS(*VirtualAddress);

    return (PNDIS_PACKET) pPacket;
    
release_pkt_header:
	RTMP_MemPool_Free(pReserved, (PUCHAR) pPacket->pHeaderMBuf, MemPool_TYPE_Header);
err:
    *VirtualAddress = (PVOID) NULL;
    *PhysicalAddress = (NDIS_PHYSICAL_ADDRESS) NULL;
    return NULL;
}

NDIS_STATUS RTMP_AllocateNdisPacket_AppandMbuf(
	IN	PRTMP_ADAPTER	pAd,
	OUT PNDIS_PACKET   *ppPacket,
	IN	struct mbuf    *pMBuf)
{
	PECOS_PKT_BUFFER	pPacket = NULL;

	if (pMBuf == NULL)
	{
		DBGPRINT_ERR(("%s: pMBuf is NULL\n"
		, __FUNCTION__));
        *ppPacket = NULL;
        return NDIS_STATUS_FAILURE;        
    }

	pPacket = RTMP_AllocatePacketHeader(pAd);
	if (pPacket == NULL)
	{
		*ppPacket = NULL;
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("RTMP_AllocateNdisPacket_AppandMbuf Fail\n"));
		return NDIS_STATUS_FAILURE;
	}

	MEM_DBG_PKT_ALLOC_INC(pAd);
    HAL_DCACHE_FLUSH(mtod(pMBuf, PUCHAR), pMBuf->m_len);    
    pPacket->pDataMBuf = pMBuf;
    pPacket->pDataPtr = (PUCHAR) CYGARC_UNCACHED_ADDRESS(mtod(pPacket->pDataMBuf, PUCHAR));
    pPacket->MemPoolType = MemPool_TYPE_MBUF;
	pPacket->pktLen = pMBuf->m_len;
	pPacket->net_dev = pAd->net_dev;

/*
#ifdef ECOS_DEBUG
	diag_printf("---%s---\n",__FUNCTION__);
#endif
*/
	*ppPacket = (PNDIS_PACKET) pPacket;
	return NDIS_STATUS_SUCCESS;
}


/* The allocated NDIS PACKET must be freed via RTMPFreeNdisPacket() for TX */
NDIS_STATUS RTMPAllocateNdisPacket(
	IN	VOID			*pAdSrc,
	OUT PNDIS_PACKET	*ppPacket,
	IN	PUCHAR			pHeader,
	IN	UINT			HeaderLen,
	IN	PUCHAR			pData,
	IN	UINT			DataLen)
{
	PRTMP_ADAPTER		pAd = (PRTMP_ADAPTER)pAdSrc;
	PECOS_PKT_BUFFER	pPacket = NULL;

	pPacket = RTMP_AllocatePacketHeader(pAd);
	if (pPacket == NULL)
	{
		*ppPacket = NULL;
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("RTMPAllocateNdisPacket Fail\n"));
		return NDIS_STATUS_FAILURE;
	}
#ifdef ECOS_DEBUG
#ifdef CONFIG_NET_CLUSTER_SIZE_2048
		if((HeaderLen + DataLen  + LEN_CCMP_HDR + LEN_CCMP_MIC) > (2048-BUFFER_HEAD_RESVERD))
		{
			panic("RTMPAllocateNdisPacket:Length over cluster size 2048!\n");
		}
#else
		if((HeaderLen + DataLen  + LEN_CCMP_HDR + LEN_CCMP_MIC) > (4096-BUFFER_HEAD_RESVERD))
		{
			panic("RTMPAllocateNdisPacket:Length over cluster size 4096!\n");
		}
#endif
#endif

	// 1. Allocate a packet /* TODO:add LEN_CCMP_HDR + LEN_CCMP_MIC for PMF*/
	pPacket->pDataMBuf = (struct mbuf *)RTMP_MemPool_Alloc(pAd, (HeaderLen + DataLen  + LEN_CCMP_HDR + LEN_CCMP_MIC), MemPool_TYPE_MBUF);
	if (pPacket->pDataMBuf == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("%s: Can't allocate buffer from memory pool\n"
		, __FUNCTION__));
                *ppPacket = NULL;
		RTMP_MemPool_Free(pAd, pPacket->pHeaderMBuf, MemPool_TYPE_Header);
                return NDIS_STATUS_FAILURE;        
        }

    HAL_DCACHE_INVALIDATE(mtod(pPacket->pDataMBuf, PUCHAR), (HeaderLen + DataLen)); 
        pPacket->pDataPtr = (PUCHAR) CYGARC_UNCACHED_ADDRESS(mtod(pPacket->pDataMBuf, PUCHAR));  
        NdisZeroMemory(pPacket->pDataPtr, BUFFER_HEAD_RESVERD);
        pPacket->pDataPtr += BUFFER_HEAD_RESVERD;
        pPacket->MemPoolType = MemPool_TYPE_MBUF;
	pPacket->pktLen = 0;
	pPacket->net_dev = pAd->net_dev;

	if ((pHeader != NULL) && (HeaderLen > 0))
		NdisMoveMemory(GET_OS_PKT_DATAPTR(pPacket), pHeader, HeaderLen);

	if ((pData != NULL) && (DataLen > 0))
		NdisMoveMemory(GET_OS_PKT_DATAPTR(pPacket) + HeaderLen, pData, DataLen);

        if (pData != NULL) {
                GET_OS_PKT_LEN(pPacket) = (HeaderLen + DataLen);
                pPacket->pDataMBuf->m_len = GET_OS_PKT_LEN(pPacket);
        }
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s : pPacket = %p, len = %d\n", 
				__FUNCTION__, pPacket, (HeaderLen + DataLen)));
	*ppPacket = (PNDIS_PACKET) pPacket;
        
	return NDIS_STATUS_SUCCESS;
}

/* The allocated NDIS PACKET must be freed via RTMPFreeNdisPacket() for TX */
NDIS_STATUS RTMPAllocateNdisPacket2(
	IN	VOID			*pAdSrc,
	OUT PNDIS_PACKET	*ppPacket,
	IN	PUCHAR			pHeader,
	IN	UINT			HeaderLen,
	IN	PUCHAR			pData,
	IN	UINT			DataLen)
{
	PRTMP_ADAPTER		pAd = (PRTMP_ADAPTER)pAdSrc;
	PECOS_PKT_BUFFER	pPacket = NULL;

	pPacket = RTMP_AllocatePacketHeader(pAd);
	if (pPacket == NULL)
	{
		*ppPacket = NULL;
		DBGPRINT(RT_DEBUG_WARN, ("RTMPAllocateNdisPacket Fail\n"));
		return NDIS_STATUS_FAILURE;
	}

	// 1. Allocate a packet /* TODO:add LEN_CCMP_HDR + LEN_CCMP_MIC for PMF*/
	pPacket->pDataMBuf = (struct mbuf *)RTMP_MemPool_Alloc(pAd, (HeaderLen + DataLen  + LEN_CCMP_HDR + LEN_CCMP_MIC), MemPool_TYPE_MBUF);
	if (pPacket->pDataMBuf == NULL)
	{
		DBGPRINT(RT_DEBUG_WARN, ("%s: Can't allocate buffer from memory pool\n"
		, __FUNCTION__));
                *ppPacket = NULL;
		RTMP_MemPool_Free(pAd, pPacket->pHeaderMBuf, MemPool_TYPE_Header);
                return NDIS_STATUS_FAILURE;        
        }

    HAL_DCACHE_INVALIDATE(mtod(pPacket->pDataMBuf, PUCHAR), (HeaderLen + DataLen)); 
        pPacket->pDataPtr = (PUCHAR) CYGARC_UNCACHED_ADDRESS(mtod(pPacket->pDataMBuf, PUCHAR));  
        NdisZeroMemory(pPacket->pDataPtr, BUFFER_HEAD_RESVERD);
#ifdef CONFIG_NET_CLUSTER_SIZE_2048
        pPacket->pDataPtr += 8;
#else
	    pPacket->pDataPtr += 2000;
#endif
        pPacket->MemPoolType = MemPool_TYPE_MBUF;
	pPacket->pktLen = 0;
	pPacket->net_dev = pAd->net_dev;

	if ((pHeader != NULL) && (HeaderLen > 0))
		NdisMoveMemory(GET_OS_PKT_DATAPTR(pPacket), pHeader, HeaderLen);

	if ((pData != NULL) && (DataLen > 0))
		NdisMoveMemory(GET_OS_PKT_DATAPTR(pPacket) + HeaderLen, pData, DataLen);

        if (pData != NULL) {
                GET_OS_PKT_LEN(pPacket) = (HeaderLen + DataLen);
                pPacket->pDataMBuf->m_len = GET_OS_PKT_LEN(pPacket);
        }
	DBGPRINT(RT_DEBUG_INFO, ("%s : pPacket = %p, len = %d\n", 
				__FUNCTION__, pPacket, (HeaderLen + DataLen)));
	*ppPacket = (PNDIS_PACKET) pPacket;
        
	return NDIS_STATUS_SUCCESS;
}

#ifdef CONFIG_NET_CLUSTER_GROUP2

/* The allocated NDIS PACKET must be freed via RTMPFreeNdisPacket() for TX */
NDIS_STATUS RTMPAllocateNdisPacket_4k(
	IN	VOID			*pAdSrc,
	OUT PNDIS_PACKET	*ppPacket,
	IN	PUCHAR			pHeader,
	IN	UINT			HeaderLen,
	IN	PUCHAR			pData,
	IN	UINT			DataLen)
{
	PRTMP_ADAPTER		pAd = (PRTMP_ADAPTER)pAdSrc;
	PECOS_PKT_BUFFER	pPacket = NULL;

	pPacket = RTMP_AllocatePacketHeader(pAd);
	if (pPacket == NULL)
	{
		*ppPacket = NULL;
		DBGPRINT(RT_DEBUG_WARN, ("RTMPAllocateNdisPacket Fail\n"));
		return NDIS_STATUS_FAILURE;
	}

	// 1. Allocate a packet /* TODO:add LEN_CCMP_HDR + LEN_CCMP_MIC for PMF*/
	pPacket->pDataMBuf = (struct mbuf *)RTMP_MemPool_Alloc_4k(pAd, (HeaderLen + DataLen  + LEN_CCMP_HDR + LEN_CCMP_MIC), MemPool_TYPE_MBUF);
	if (pPacket->pDataMBuf == NULL)
	{
		DBGPRINT(RT_DEBUG_WARN, ("%s: Can't allocate buffer from memory pool\n"
		, __FUNCTION__));
                *ppPacket = NULL;
		RTMP_MemPool_Free(pAd, pPacket->pHeaderMBuf, MemPool_TYPE_Header);
                return NDIS_STATUS_FAILURE;        
        }

    	HAL_DCACHE_INVALIDATE(mtod(pPacket->pDataMBuf, PUCHAR), (HeaderLen + DataLen)); 
        pPacket->pDataPtr = (PUCHAR) CYGARC_UNCACHED_ADDRESS(mtod(pPacket->pDataMBuf, PUCHAR));  
        NdisZeroMemory(pPacket->pDataPtr, BUFFER_HEAD_RESVERD);
		//add flag for 4k cluster usage:offset 8 location set 0x7628ffff
		*(UINT32 *)(pPacket->pDataPtr + 8) = 0x7628ffff;	
		//diag_printf("allocate 4k*****>>>>\n");
	    pPacket->pDataPtr += BUFFER_HEAD_RESVERD;//2000;

        pPacket->MemPoolType = MemPool_TYPE_MBUF;
		pPacket->pktLen = 0;
		pPacket->net_dev = pAd->net_dev;

	if ((pHeader != NULL) && (HeaderLen > 0))
		NdisMoveMemory(GET_OS_PKT_DATAPTR(pPacket), pHeader, HeaderLen);

	if ((pData != NULL) && (DataLen > 0))
		NdisMoveMemory(GET_OS_PKT_DATAPTR(pPacket) + HeaderLen, pData, DataLen);

    if (pData != NULL) {
            GET_OS_PKT_LEN(pPacket) = (HeaderLen + DataLen);
            pPacket->pDataMBuf->m_len = GET_OS_PKT_LEN(pPacket);
    }
	
	DBGPRINT(RT_DEBUG_INFO, ("%s : pPacket = %p, len = %d\n", 
				__FUNCTION__, pPacket, (HeaderLen + DataLen)));
	*ppPacket = (PNDIS_PACKET) pPacket;
        
	return NDIS_STATUS_SUCCESS;
}
#endif

/*
  ========================================================================
  Description:
	This routine frees a miniport internally allocated NDIS_PACKET and its
	corresponding NDIS_BUFFER and allocated memory.
  ========================================================================
*/
VOID RTMPFreeNdisPacket(
	IN VOID 		*pAd,
	IN PNDIS_PACKET  pPacket)
{
	PECOS_PKT_BUFFER pPkt;
	static int error_cnt=0;

	if (pPacket != NULL)
	{
        pPkt = (PECOS_PKT_BUFFER) pPacket;
       	switch (pPkt->MemPoolType)
       	{
            case MemPool_TYPE_MBUF:
                if (pPkt->pDataMBuf != NULL)
                {
#ifdef CONFIG_NET_CLUSTER_GROUP2
                	if((*(UINT32 *)(pPkt->pDataMBuf->m_ext.ext_buf + 8) == 0x7628ffff) && (pPkt->pDataMBuf->m_ext.ext_size == NET_CLUSTER_GROUP2_SIZE))
                    	{RTMP_MemPool_Free_4k(pAd, (PUCHAR) pPkt->pDataMBuf, pPkt->MemPoolType);/*diag_printf("free 4k*****<<<<\n");*/}
					else
#endif						
                		RTMP_MemPool_Free(pAd, (PUCHAR) pPkt->pDataMBuf, pPkt->MemPoolType);
                }
				if (pPkt->pHeaderMBuf != NULL)
                    RTMP_MemPool_Free(pAd, (PUCHAR) pPkt->pHeaderMBuf, MemPool_TYPE_Header);
                pPacket = NULL;            
                break;
            default:
				diag_printf("\n*****************ZIQIANG.YU***************************\n");
				diag_printf("[%s]:this is not correct\n",__FUNCTION__);
				diag_printf("pPacket=%p\n pPkt->pDataMBuf=%p\n pPkt->pHeaderMBuf=%p pPkt->MemPoolType=%d\n",
				pPacket,pPkt->pDataMBuf,pPkt->pHeaderMBuf,pPkt->MemPoolType	);
				error_cnt++;
				diag_printf("error_cnt =%d\n",error_cnt);
				diag_printf("*************************************************\n");
                //kfree(pPkt);				
				//diag_printf("%s:Free successful\n",__FUNCTION__);
                break;
        }
    }
    else
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Packet is  NULL\n", __FUNCTION__));    
}

PNDIS_PACKET RtmpOSNetPktAlloc(
	IN VOID 		*pAdSrc,
	IN int size)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
    PECOS_PKT_BUFFER        pPacket;
	PVOID					AllocVa;
	NDIS_PHYSICAL_ADDRESS	AllocPa;

    pPacket = (PECOS_PKT_BUFFER) RTMP_AllocateRxPacketBuffer(pAd, ((POS_COOKIE)(pAd->OS_Cookie))->pci_dev, size, FALSE, &AllocVa, &AllocPa);
    return (PNDIS_PACKET) pPacket;    
}

PNDIS_PACKET RTMP_AllocateFragPacketBuffer(
	IN VOID 		*pAdSrc,
	IN	ULONG	Length)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
    PECOS_PKT_BUFFER        pPacket;
	PVOID					AllocVa;
	NDIS_PHYSICAL_ADDRESS	AllocPa;

    pPacket = (PECOS_PKT_BUFFER) RTMP_AllocateRxPacketBuffer(pAd, ((POS_COOKIE)(pAd->OS_Cookie))->pci_dev, Length, FALSE, &AllocVa, &AllocPa);
    return (PNDIS_PACKET) pPacket;
}

VOID build_tx_packet(
	IN	PRTMP_ADAPTER	pAd,
	IN	PNDIS_PACKET	pPacket,
	IN	PUCHAR	pFrame,
	IN	ULONG	FrameLen)
{
	PECOS_PKT_BUFFER	pPkt;

	ASSERT(pPacket);
    pPkt = (PECOS_PKT_BUFFER) pPacket;
    if (FrameLen > 0) {
    	NdisMoveMemory(GET_OS_PKT_DATAPTR(pPkt), pFrame, FrameLen);
    	GET_OS_PKT_TOTAL_LEN(pPkt) += FrameLen;
    }
}


void * skb_put(PECOS_PKT_BUFFER skb, INT len)
{
	PUCHAR p;

	ASSERT(skb);
	p = GET_OS_PKT_DATATAIL(skb);
	GET_OS_PKT_LEN(skb) += len;
	
	return p;
}

void * skb_pull(PECOS_PKT_BUFFER skb, INT len)
{
	ASSERT(skb);
	
	GET_OS_PKT_DATAPTR(skb) += len;
	GET_OS_PKT_LEN(skb) -= len;
		
	return GET_OS_PKT_DATAPTR(skb);

}

void * skb_trim(PECOS_PKT_BUFFER skb, INT newlen)
{
	ASSERT(skb);
	
	//GET_OS_PKT_DATAPTR(skb) += len;
	GET_OS_PKT_LEN(skb) = newlen;
		
	return GET_OS_PKT_DATAPTR(skb);

}


INT skb_headroom(PECOS_PKT_BUFFER skb)
{
	INT headroom=0;
	struct mbuf *pMBuf = NULL;
	PUCHAR pDataPtr = NULL;
	ASSERT(skb);
	pMBuf = RTPKT_TO_OSPKT(skb)->pDataMBuf;
	pDataPtr = (PUCHAR) CYGARC_CACHED_ADDRESS(GET_OS_PKT_DATAPTR(skb));
	//diag_printf("dataptr:%x %x,m_data:%x,ext_buf:%x\n", GET_OS_PKT_DATAPTR(skb), pDataPtr, pMBuf->m_data, pMBuf->m_ext.ext_buf);
	if (pMBuf->m_flags & M_EXT)
		headroom = (caddr_t)(pDataPtr) - (pMBuf->m_ext.ext_buf);
	else
		diag_printf("skb has not ext buf\n");
		
	return headroom;
}


void * skb_push(PECOS_PKT_BUFFER skb, INT len)
{
	ASSERT(skb);

	GET_OS_PKT_DATAPTR(skb) -= len;
	GET_OS_PKT_LEN(skb) += len;
		
	return GET_OS_PKT_DATAPTR(skb);
}

void * skb_reserve(PECOS_PKT_BUFFER skb, INT len)
{
	ASSERT(skb);
    
	GET_OS_PKT_DATAPTR(skb) += len;
	GET_OS_PKT_LEN(skb) -= len;
    if (GET_OS_PKT_LEN(skb) < 0)
        GET_OS_PKT_LEN(skb) = 0;
		
	return GET_OS_PKT_DATAPTR(skb);
}


PNDIS_PACKET skb_copy(PNDIS_PACKET pSrcPkt, INT flags)
{
    PRTMP_ADAPTER           pAd = NULL;
    PNET_DEV                pNetDev = NULL;
	POS_COOKIE			    pOSCookie = NULL;
    PECOS_PKT_BUFFER        pPacket = NULL;
    PECOS_PKT_BUFFER        pNewPacket = NULL;
	PVOID					AllocVa;
	NDIS_PHYSICAL_ADDRESS	AllocPa;
    NDIS_STATUS             Status;

    pPacket = (PECOS_PKT_BUFFER) pSrcPkt;
    if (pPacket == NULL) {
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: pSrcPkt is NULL\n", __FUNCTION__));
        goto err;
    }

	pNetDev = GET_OS_PKT_NETDEV(pPacket);
    if (pNetDev == NULL) {
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: pNetDev is NULL\n", __FUNCTION__));
        goto err;
	}

	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(GET_OS_PKT_NETDEV(pPacket));
    if (pAd == NULL) {
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: pAd is NULL\n", __FUNCTION__));
        goto err;
	}
	pOSCookie = (POS_COOKIE) pAd->OS_Cookie;

	pNewPacket = (PECOS_PKT_BUFFER) RTMP_AllocateRxPacketBuffer(pAd, pPacket->net_dev, RX_BUFFER_AGGRESIZE, FALSE, &AllocVa, &AllocPa);
	
    if (pNewPacket != NULL) {
        pNewPacket->net_dev = pPacket->net_dev;
        pNewPacket->pktLen = pPacket->pktLen;
        NdisCopyMemory(pNewPacket->cb, pPacket->cb, 48);
	pNewPacket->pDataPtr += pPacket->pDataPtr - (PUCHAR) ROUND_UP(pPacket->pDataPtr, 4);
        NdisCopyMemory(pNewPacket->pDataPtr, pPacket->pDataPtr, pPacket->pktLen);
    }

    return (PNDIS_PACKET) pNewPacket;
err:
    return NULL;
}

//#if 0
PNDIS_PACKET skb_clone(PNDIS_PACKET pSrcPkt, INT flags)
{
    PRTMP_ADAPTER           pAd = NULL;
    PNET_DEV                pNetDev = NULL;
	POS_COOKIE			    pOSCookie = NULL;
    PECOS_PKT_BUFFER        pPacket = NULL;
    PECOS_PKT_BUFFER        pNewPacket = NULL;
	PVOID					AllocVa;
	NDIS_PHYSICAL_ADDRESS	AllocPa;
    NDIS_STATUS             Status;

    pPacket = (PECOS_PKT_BUFFER) pSrcPkt;
    if (pPacket == NULL) {
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: pSrcPkt is NULL\n", __FUNCTION__));
        goto err;
    }

	pNetDev = GET_OS_PKT_NETDEV(pPacket);
    if (pNetDev == NULL) {
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: pNetDev is NULL\n", __FUNCTION__));
        goto err;
    }

	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(GET_OS_PKT_NETDEV(pPacket));
    if (pAd == NULL) {
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: pAd is NULL\n", __FUNCTION__));
        goto err;
    }
	pOSCookie = (POS_COOKIE) pAd->OS_Cookie;

	pNewPacket = RTMP_AllocatePacketHeader(pAd);
	if (pNewPacket == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("%s: Can't allocate packet structure\n"
		, __FUNCTION__));
        goto err;
    }

    pNewPacket->pDataMBuf = m_copym(pPacket->pDataMBuf, 0, M_COPYALL, M_DONTWAIT);
	if (pNewPacket->pDataMBuf == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("%s: Can't allocate buffer from memory pool\n", __FUNCTION__));
        goto release_pkt_header;
    }

    pNewPacket->MemPoolType = MemPool_TYPE_MBUF;
        pNewPacket->net_dev = pPacket->net_dev;
    NdisCopyMemory(pNewPacket->cb, pPacket->cb, 48);
    pNewPacket->pDataPtr = pPacket->pDataPtr;
        pNewPacket->pktLen = pPacket->pktLen;

    return (PNDIS_PACKET) pNewPacket;

release_pkt_header:
	RTMP_MemPool_Free(pAd, (PUCHAR) pNewPacket->pHeaderMBuf, MemPool_TYPE_Header);
err:
    return NULL;
}
//#endif

void RTMP_QueryPacketInfo(
	IN  PNDIS_PACKET pPacket,
	OUT PACKET_INFO  *pPacketInfo,
	OUT PUCHAR		 *pSrcBufVA,
	OUT	UINT		 *pSrcBufLen)
{
	pPacketInfo->BufferCount = 1;
	pPacketInfo->pFirstBuffer = (PNDIS_BUFFER) GET_OS_PKT_DATAPTR(pPacket);
	pPacketInfo->PhysicalBufferCount = 1;
	pPacketInfo->TotalPacketLength = (UINT) GET_OS_PKT_LEN(pPacket);

	*pSrcBufVA = GET_OS_PKT_DATAPTR(pPacket);
	*pSrcBufLen = GET_OS_PKT_LEN(pPacket); 
}


PNDIS_PACKET skb_clone_ndev(PNET_DEV ndev, PNDIS_PACKET pSrcPkt, INT flags)
{
    PRTMP_ADAPTER           pAd = NULL;
    PNET_DEV                pNetDev = NULL;
	POS_COOKIE			    pOSCookie = NULL;
    PECOS_PKT_BUFFER        pPacket = NULL;
    PECOS_PKT_BUFFER        pNewPacket = NULL;
	PVOID					AllocVa;
	NDIS_PHYSICAL_ADDRESS	AllocPa;
    NDIS_STATUS             Status;

    pPacket = (PECOS_PKT_BUFFER) pSrcPkt;
    if (pPacket == NULL) {
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: pSrcPkt is NULL\n", __FUNCTION__));
        goto err;
    }

	pNetDev = ndev;//GET_OS_PKT_NETDEV(pPacket);
    if (pNetDev == NULL) {
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: pNetDev is NULL\n", __FUNCTION__));
        goto err;
    }

	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(ndev);
    if (pAd == NULL) {
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: pAd is NULL\n", __FUNCTION__));
        goto err;
    }
	pOSCookie = (POS_COOKIE) pAd->OS_Cookie;

	pNewPacket = RTMP_AllocatePacketHeader(pAd);
	if (pNewPacket == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("%s: Can't allocate packet structure\n"
		, __FUNCTION__));
        goto err;
    }

    pNewPacket->pDataMBuf = m_copym(pPacket->pDataMBuf, 0, M_COPYALL, M_DONTWAIT);
	if (pNewPacket->pDataMBuf == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("%s: Can't allocate buffer from memory pool\n", __FUNCTION__));
        goto release_pkt_header;
    }

    pNewPacket->MemPoolType = MemPool_TYPE_MBUF;
        pNewPacket->net_dev = pPacket->net_dev;
    NdisCopyMemory(pNewPacket->cb, pPacket->cb, 48);
    pNewPacket->pDataPtr = pPacket->pDataPtr;
        pNewPacket->pktLen = pPacket->pktLen;

    return (PNDIS_PACKET) pNewPacket;

release_pkt_header:
	RTMP_MemPool_Free(pAd, (PUCHAR) pNewPacket->pHeaderMBuf, MemPool_TYPE_Header);
err:
    return NULL;
}


PNDIS_PACKET ClonePacket(PNET_DEV ndev, PNDIS_PACKET pkt, UCHAR *buf, ULONG sz)
{
	PECOS_PKT_BUFFER pClonedMblk;

	ASSERT(sz < 1530);	
    
	pClonedMblk = (PECOS_PKT_BUFFER) skb_clone_ndev(ndev, pkt, MEM_ALLOC_FLAG);
	if (pClonedMblk != NULL)
	{
		/* set the correct dataptr and data len */
		pClonedMblk->pDataPtr = pClonedMblk->pDataPtr + (buf - GET_OS_PKT_DATAPTR(pkt));
		pClonedMblk->pktLen = sz;
	}
	//diag_printf("---%s---\n",__FUNCTION__);
    return (PNDIS_PACKET) pClonedMblk;
}


PNDIS_PACKET DuplicatePacket(PNET_DEV pNetDev, PNDIS_PACKET pPacket)
{
	PNDIS_PACKET	pRetPacket = NULL;
	USHORT			DataSize;
	UCHAR			*pData;

	DataSize = (USHORT) GET_OS_PKT_LEN(pPacket);
	pData = (PUCHAR) GET_OS_PKT_DATAPTR(pPacket);	

	pRetPacket = skb_copy(RTPKT_TO_OSPKT(pPacket), MEM_ALLOC_FLAG);
	if (pRetPacket)
		SET_OS_PKT_NETDEV(pRetPacket, pNetDev);
	
	return pRetPacket;
}

	
PNDIS_PACKET duplicate_pkt_vlan(
	IN	PNET_DEV		pNetDev,
	IN	USHORT			VLAN_VID,
	IN	USHORT			VLAN_Priority,
	IN	PUCHAR			pHeader802_3,
	IN	UINT			HdrLen,
	IN	PUCHAR			pData,
	IN	ULONG			DataSize,
	IN	UCHAR			*TPID)
{
	RTMP_ADAPTER		*pAd = NULL;
	NDIS_STATUS			Status;
	PECOS_PKT_BUFFER	pPacket = NULL;
	UINT16              VLAN_Size;
	INT skb_len = HdrLen + DataSize + 2;
	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(pNetDev);

#ifdef WIFI_VLAN_SUPPORT
	if (VLAN_VID != 0)
		skb_len += LENGTH_802_1Q;
#endif /* WIFI_VLAN_SUPPORT */

    Status = RTMPAllocateNdisPacket(pAd, &pPacket, NULL, 0, NULL, skb_len);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("%s:can't allocate NDIS PACKET\n", __FUNCTION__));
        return NULL;
    }

	skb_reserve(pPacket, 2);

	/* copy header (maybe +VLAN tag) */
	VLAN_Size = VLAN_8023_Header_Copy(VLAN_VID, VLAN_Priority, pHeader802_3, HdrLen,
											pPacket->pDataPtr, TPID);
	skb_put(pPacket, HdrLen + VLAN_Size);

	/* copy data body */
	NdisMoveMemory(pPacket->pDataPtr + HdrLen + VLAN_Size, pData, DataSize);
	skb_put(pPacket, DataSize);
	pPacket->net_dev = pNetDev;
    
    return (PNDIS_PACKET) pPacket;
}


#define TKIP_TX_MIC_SIZE		8
PNDIS_PACKET duplicate_pkt_with_TKIP_MIC(
	IN	VOID			*pAd,
	IN	PNDIS_PACKET	pPacket)
{
        PECOS_PKT_BUFFER pBuf = (PECOS_PKT_BUFFER) pPacket;
        pBuf->pDataMBuf->m_len = pBuf->pktLen;
        if (M_TRAILINGSPACE(pBuf->pDataMBuf) < TKIP_TX_MIC_SIZE)
        {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Extend Tx.MIC for packet failed!, dropping packet!\n"));
                return NULL;
        }
        return pPacket;
}


/*
 * invaild or writeback cache 
 * and convert virtual address to physical address 
 */
ra_dma_addr_t ecos_pci_map_single(void *handle, void *ptr, size_t size, int sd_idx, int direction)
{	
	if (sd_idx == 1)
	{
		TX_BLK *pTxBlk = (TX_BLK *)ptr;
		if (pTxBlk->SrcBufLen > 0) {
			HAL_DCACHE_FLUSH((pTxBlk->pSrcBufData), pTxBlk->SrcBufLen);
			return CYGARC_PHYSICAL_ADDRESS(CYGARC_UNCACHED_ADDRESS(pTxBlk->pSrcBufData));
		} else
			return NULL;
	}
	else
	{
		HAL_DCACHE_FLUSH(ptr, size);    
		return CYGARC_PHYSICAL_ADDRESS(CYGARC_UNCACHED_ADDRESS(ptr));
	}
}


#ifdef PLATFORM_BUTTON_SUPPORT
/* Polling reset button in APSOC */
extern void CGI_reset_default(void);
VOID Restore_button_CheckHandler(
	IN	PRTMP_ADAPTER	pAd)
{
	UINT32 gpio_value, gpio_pol;
	BOOLEAN flg_pressed = 0;
	
    gpio_value = HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_DIR_OFFSET);
#ifdef RT5350
    gpio_value &= 0xfdff; /* GPIO 9 */
    HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_DIR_OFFSET) = gpio_value;
    gpio_value = HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_DATA_OFFSET);
    flg_pressed = ((~gpio_value) & 0x400)?1:0;
#endif

#ifdef MT7628 
	{	/* GPIO 38 */						
		gpio_value = HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_DIR_OFFSET); 
		gpio_value &= ~(1<<6);						
		HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_DIR_OFFSET) = gpio_value; 
		gpio_value = HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_DATA_OFFSET); 
		flg_pressed = ((~gpio_value) & (1<<6))?1:0;					
	}
#endif

	if (flg_pressed)
	{
	        ULONG nowtime;
	        NdisGetSystemUpTime(&nowtime);

                if (pAd->CommonCfg.RestoreHdrBtnTimestamp == 0)
                        pAd->CommonCfg.RestoreHdrBtnTimestamp = nowtime; 
                else if (((nowtime - pAd->CommonCfg.RestoreHdrBtnTimestamp) / HZ) > 4)
                {
        		/* execute Reset function */
	        	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Restore to factory default settings...\n"));			
				CGI_reset_default();
                        pAd->CommonCfg.RestoreHdrBtnTimestamp = 0;
                }
	}
	else
	{
		/* the button is released */
		pAd->CommonCfg.RestoreHdrBtnTimestamp = 0;		
	}
}
#endif /* PLATFORM_BUTTON_SUPPORT */

