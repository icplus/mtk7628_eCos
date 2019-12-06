#ifndef CYGONCE_IF_RA305X_H
#define CYGONCE_IF_RA305X_H
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
	if_ra305x.h

    Abstract:
	Hardware driver for Ralink Ra305x ethernet devices
   
    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

/*  Frame engine  */
#define RAFE_REG(_off)			HAL_FRAME_REG(_off)


#if defined(CONFIG_RT7620_ASIC)
#define CONFIG_PDMA_NEW
#define CONFIG_RAETH_SCATTER_GATHER_RX_DMA
#endif

#if defined(CONFIG_RT5350_ASIC) || defined(CONFIG_MT7628_ASIC)
/* RT5350: No GDMA, PSE, CDMA, PPE */
#define RAFE_PDMA_RELATED               0x0800
#define RAFE_TX_BASE_PTR0		(RAFE_PDMA_RELATED + 0x00)
#define RAFE_TX_MAX_CNT0		(RAFE_PDMA_RELATED + 0x04)
#define RAFE_TX_CTX_IDX0		(RAFE_PDMA_RELATED + 0x08)
#define RAFE_TX_DTX_IDX0		(RAFE_PDMA_RELATED + 0x0C)
#define RAFE_TX_BASE_PTR1		(RAFE_PDMA_RELATED + 0x10)
#define RAFE_TX_MAX_CNT1		(RAFE_PDMA_RELATED + 0x14)
#define RAFE_TX_CTX_IDX1		(RAFE_PDMA_RELATED + 0x18)
#define RAFE_TX_DTX_IDX1		(RAFE_PDMA_RELATED + 0x1C)
#define RAFE_TX_BASE_PTR2		(RAFE_PDMA_RELATED + 0x20)
#define RAFE_TX_MAX_CNT2		(RAFE_PDMA_RELATED + 0x24)
#define RAFE_TX_CTX_IDX2		(RAFE_PDMA_RELATED + 0x28)
#define RAFE_TX_DTX_IDX2		(RAFE_PDMA_RELATED + 0x2C)
#define RAFE_TX_BASE_PTR3		(RAFE_PDMA_RELATED + 0x30)
#define RAFE_TX_MAX_CNT3		(RAFE_PDMA_RELATED + 0x34)
#define RAFE_TX_CTX_IDX3		(RAFE_PDMA_RELATED + 0x38)
#define RAFE_TX_DTX_IDX3		(RAFE_PDMA_RELATED + 0x3C)
#define RAFE_RX_BASE_PTR0		(RAFE_PDMA_RELATED + 0x100)
#define RAFE_RX_MAX_CNT0		(RAFE_PDMA_RELATED + 0x104)
#define RAFE_RX_CALC_IDX0		(RAFE_PDMA_RELATED + 0x108)
#define RAFE_FS_DRX_IDX0                (RAFE_PDMA_RELATED + 0x10C)
#define RAFE_RX_BASE_PTR1               (RAFE_PDMA_RELATED + 0x110)
#define RAFE_RX_MAX_CNT1                (RAFE_PDMA_RELATED + 0x114)
#define RAFE_RX_CALC_IDX1               (RAFE_PDMA_RELATED + 0x118)
#define RAFE_FS_DRX_IDX1		(RAFE_PDMA_RELATED + 0x11C)
#define RAFE_PDMA_INFO                  (RAFE_PDMA_RELATED + 0x200)
#define RAFE_PDMA_GLO_CFG               (RAFE_PDMA_RELATED + 0x204)
#define RAFE_PDMA_RST_IDX               (RAFE_PDMA_RELATED + 0x208)
#define RAFE_PDMA_RST_CFG               (RAFE_PDMA_RST_IDX)
#define RAFE_DLY_INT_CFG                (RAFE_PDMA_RELATED + 0x20C)
#define RAFE_FREEQ_THRES                (RAFE_PDMA_RELATED + 0x210)
#define RAFE_INT_STATUS                 (RAFE_PDMA_RELATED + 0x220)
#define RAFE_FE_INT_STATUS              (RAFE_INT_STATUS)
#define RAFE_INT_MASK       	        (RAFE_PDMA_RELATED + 0x228)
#define RAFE_FE_INT_ENABLE     	        (RAFE_INT_MASK)
#define RAFE_PDMA_WRR       	        (RAFE_PDMA_RELATED + 0x280)
#define RAFE_PDMA_SCH_CFG      	        (RAFE_PDMA_WRR)

#define RAFE_SDM_OFFSET      	        0x0C00
#define RAFE_SDM_CON			(RAFE_SDM_OFFSET + 0x00)  //Switch DMA configuration
#define RAFE_SDM_RRING                  (RAFE_SDM_OFFSET + 0x04)  //Switch DMA Rx Ring
#define RAFE_SDM_TRING                  (RAFE_SDM_OFFSET + 0x08)  //Switch DMA Tx Ring
#define RAFE_SDM_MAC_ADRL		(RAFE_SDM_OFFSET + 0x0C)  //Switch MAC address LSB
#define RAFE_SDM_MAC_ADRH		(RAFE_SDM_OFFSET + 0x10)  //Switch MAC Address MSB
#define RAFE_SDM_TPCNT                  (RAFE_SDM_OFFSET + 0x100) //Switch DMA Tx packet count
#define RAFE_SDM_TBCNT                  (RAFE_SDM_OFFSET + 0x104) //Switch DMA Tx byte count
#define RAFE_SDM_RPCNT                  (RAFE_SDM_OFFSET + 0x108) //Switch DMA rx packet count
#define RAFE_SDM_RBCNT                  (RAFE_SDM_OFFSET + 0x10C) //Switch DMA rx byte count
#define RAFE_SDM_CS_ERR                 (RAFE_SDM_OFFSET + 0x110) //Switch DMA rx checksum error count

#elif defined(CONFIG_RT7620_ASIC)
/* Old FE with New PDMA */
#define RAFE_PDMA_RELATED	0x0800
/* 1. PDMA */
#define RAFE_TX_BASE_PTR0            (RAFE_PDMA_RELATED+0x000)
#define RAFE_TX_MAX_CNT0             (RAFE_PDMA_RELATED+0x004)
#define RAFE_TX_CTX_IDX0             (RAFE_PDMA_RELATED+0x008)
#define RAFE_TX_DTX_IDX0             (RAFE_PDMA_RELATED+0x00C)
	
#define RAFE_TX_BASE_PTR1            (RAFE_PDMA_RELATED+0x010)
#define RAFE_TX_MAX_CNT1             (RAFE_PDMA_RELATED+0x014)
#define RAFE_TX_CTX_IDX1             (RAFE_PDMA_RELATED+0x018)
#define RAFE_TX_DTX_IDX1             (RAFE_PDMA_RELATED+0x01C)
	
#define RAFE_TX_BASE_PTR2            (RAFE_PDMA_RELATED+0x020)
#define RAFE_TX_MAX_CNT2             (RAFE_PDMA_RELATED+0x024)
#define RAFE_TX_CTX_IDX2             (RAFE_PDMA_RELATED+0x028)
#define RAFE_TX_DTX_IDX2             (RAFE_PDMA_RELATED+0x02C)
	
#define RAFE_TX_BASE_PTR3            (RAFE_PDMA_RELATED+0x030)
#define RAFE_TX_MAX_CNT3             (RAFE_PDMA_RELATED+0x034)
#define RAFE_TX_CTX_IDX3             (RAFE_PDMA_RELATED+0x038)
#define RAFE_TX_DTX_IDX3             (RAFE_PDMA_RELATED+0x03C)
	
#define RAFE_RX_BASE_PTR0            (RAFE_PDMA_RELATED+0x100)
#define RAFE_RX_MAX_CNT0             (RAFE_PDMA_RELATED+0x104)
#define RAFE_RX_CALC_IDX0            (RAFE_PDMA_RELATED+0x108)
#define RAFE_RX_DRX_IDX0             (RAFE_PDMA_RELATED+0x10C)
	
#define RAFE_RX_BASE_PTR1            (RAFE_PDMA_RELATED+0x110)
#define RAFE_RX_MAX_CNT1             (RAFE_PDMA_RELATED+0x114)
#define RAFE_RX_CALC_IDX1            (RAFE_PDMA_RELATED+0x118)
#define RAFE_RX_DRX_IDX1             (RAFE_PDMA_RELATED+0x11C)
	
#define RAFE_PDMA_INFO               (RAFE_PDMA_RELATED+0x200)
#define RAFE_PDMA_GLO_CFG            (RAFE_PDMA_RELATED+0x204)
#define RAFE_PDMA_RST_IDX            (RAFE_PDMA_RELATED+0x208)
#define RAFE_PDMA_RST_CFG            (RAFE_PDMA_RST_IDX)
#define RAFE_DLY_INT_CFG             (RAFE_PDMA_RELATED+0x20C)
#define RAFE_FREEQ_THRES             (RAFE_PDMA_RELATED+0x210)
#define RAFE_INT_STATUS              (RAFE_PDMA_RELATED+0x220)
#define RAFE_FE_INT_STATUS	     (RAFE_INT_STATUS)
#define RAFE_INT_MASK                (RAFE_PDMA_RELATED+0x228)
#define RAFE_FE_INT_ENABLE	     (RAFE_INT_MASK)

#define RAFE_SCH_Q01_CFG		(RAPDMA_OFFSET+0x280)
#define RAFE_SCH_Q23_CFG		(RAPDMA_OFFSET+0x284)

#define RAFE_FE_GLO_CFG          0x08
#define RAFE_FE_RST_GL           0x0C
#define RAFE_FE_INT_STATUS2	    0x10
#define RAFE_FE_INT_ENABLE2	    0x14
#define RAFE_FC_DROP_STA         0x18
#define RAFE_FOE_TS_T            0x1C

#if defined(CONFIG_PDMA_NEW)
#define RAFE_GDMA1_RELATED       0x0600
#define RAFE_GDMA1_FWD_CFG       (RAFE_GDMA1_RELATED + 0x00)
#define RAFE_GDMA1_SHPR_CFG      (RAFE_GDMA1_RELATED + 0x04)
#define RAFE_GDMA1_MAC_ADRL      (RAFE_GDMA1_RELATED + 0x08)
#define RAFE_GDMA1_MAC_ADRH      (RAFE_GDMA1_RELATED + 0x0C)
#endif

#define RAFE_PSE_RELATED         0x0500
#define RAFE_PSE_FQFC_CFG        (RAFE_PSE_RELATED + 0x00)
#define RAFE_PSE_IQ_CFG          (RAFE_PSE_RELATED + 0x04)
#define RAFE_PSE_QUE_STA         (RAFE_PSE_RELATED + 0x08)

#define RAFE_CDMA_RELATED        0x0400
#define RAFE_CDMA_CSG_CFG        (RAFE_CDMA_RELATED + 0x00)
#define RAFE_SMACCR0		    (0x3FE4)
#define RAFE_SMACCR1		    (0x3FE8)
#define RAFE_CKGCR               (0x3FF0)

#define RAFE_PDMA_FC_CFG	    (0x100)

#else /* !CONFIG_RT5350_ASIC */
/* 1. Frame Engine Global Registers */
#define RAFE_MDIO_ACCESS		0x0000
#define RAFE_MDIO_CFG			0x0004
#define RAFE_FE_GLO_CFG			0x0008
#define RAFE_FE_RST_GL			0x000C
#define RAFE_FE_INT_STATUS		0x0010
#define RAFE_FE_INT_ENABLE		0x0014
#define RAFE_MDIO_CFG2			0x0018
#define RAFE_FOC_TS_T			0x001C

/* 2. GDMA Registers */
#define RAFE_GDMA1_FWD_CFG		0x0020
#define RAFE_GDMA1_SCH_CFG		0x0024
#define RAFE_GDMA1_SHPR_CFG		0x0028
#define RAFE_GDMA1_MAC_ADRL		0x002C
#define RAFE_GDMA1_MAC_ADRH		0x0030

#define RAFE_GDMA2_FWD_CFG		0x0060
#define RAFE_GDMA2_SCH_CFG		0x0064
#define RAFE_GDMA2_SHPR_CFG		0x0068
#define RAFE_GDMA2_MAC_ADRL		0x006C
#define RAFE_GDMA2_MAC_ADRH		0x0060

/* 3. PSE */
#define RAFE_PSE_FQ_CFG			0x0040
#define RAFE_CDMA_FC_CFG		0x0044
#define RAFE_GDMA1_FC_CFG		0x0048
#define RAFE_GDMA2_FC_CFG		0x004C

/* 4. CDMA */
#define RAFE_CDMA_CSG_CFG		0x0080
#define RAFE_CDMA_SCH_CFG		0x0084

/* 5. PDMA */
#define RAFE_PDMA_RELATED               0x0100
#define RAFE_PDMA_GLO_CFG		(RAFE_PDMA_RELATED + 0x00)
#define RAFE_PDMA_RST_CFG               (RAFE_PDMA_RELATED + 0x04)
#define RAFE_PDMA_SCH_CFG		(RAFE_PDMA_RELATED + 0x08)
#define RAFE_DLY_INT_CFG                (RAFE_PDMA_RELATED + 0x0C)
#define RAFE_TX_BASE_PTR0		(RAFE_PDMA_RELATED + 0x10)
#define RAFE_TX_MAX_CNT0		(RAFE_PDMA_RELATED + 0x14)
#define RAFE_TX_CTX_IDX0		(RAFE_PDMA_RELATED + 0x18)
#define RAFE_TX_DTX_IDX0		(RAFE_PDMA_RELATED + 0x1C)
#define RAFE_TX_BASE_PTR1		(RAFE_PDMA_RELATED + 0x20)
#define RAFE_TX_MAX_CNT1		(RAFE_PDMA_RELATED + 0x24)
#define RAFE_TX_CTX_IDX1		(RAFE_PDMA_RELATED + 0x28)
#define RAFE_TX_DTX_IDX1		(RAFE_PDMA_RELATED + 0x2C)
#define RAFE_RX_BASE_PTR0		(RAFE_PDMA_RELATED + 0x30)
#define RAFE_RX_MAX_CNT0		(RAFE_PDMA_RELATED + 0x34)
#define RAFE_RX_CALC_IDX0		(RAFE_PDMA_RELATED + 0x38)
#define RAFE_RX_DRX_IDX0		(RAFE_PDMA_RELATED + 0x3C)
#define RAFE_TX_BASE_PTR2		(RAFE_PDMA_RELATED + 0x40)
#define RAFE_TX_MAX_CNT2		(RAFE_PDMA_RELATED + 0x44)
#define RAFE_TX_CTX_IDX2		(RAFE_PDMA_RELATED + 0x48)
#define RAFE_TX_DTX_IDX2		(RAFE_PDMA_RELATED + 0x4C)
#define RAFE_TX_BASE_PTR3		(RAFE_PDMA_RELATED + 0x50)
#define RAFE_TX_MAX_CNT3		(RAFE_PDMA_RELATED + 0x54)
#define RAFE_TX_CTX_IDX3		(RAFE_PDMA_RELATED + 0x58)
#define RAFE_TX_DTX_IDX3		(RAFE_PDMA_RELATED + 0x5C)
#define RAFE_PDMA_FC_CFG		(RAFE_PDMA_RELATED + 0xF0)

#define GDMA_TX_GBCNT0			0x700	 /* Transmit good byte cnt for GEport */
#define GDMA_TX_GPCNT0			0x704	 /* Transmit good pkt cnt for GEport */
#define GDMA_TX_SKIPCNT0		0x708	 /* Transmit skip cnt for GEport */
#define GDMA_TX_COLCNT0			0x70C	 /* Transmit collision cnt for GEport */

#define GDMA_RX_GBCNT0			0x720
#define GDMA_RX_GPCNT0			0x724
#define GDMA_RX_OERCNT0			0x728
#define GDMA_RX_FERCNT0 		0x72C
#define GDMA_RX_SERCNT0			0x730
#define GDMA_RX_LERCNT0			0x734
#define GDMA_RX_CERCNT0			0x738
#define GDMA_RX_FCCNT1			0x73C
#endif /* CONFIG_RT5350_ASIC */

#define RAFE_TX_BASE			0x0
#define RAFE_TX_MAX_CNT			0x4
#define RAFE_TX_CTX_IDX			0x8
#define RAFE_TX_DTX_IDX			0xC

#define RAFE_RX_BASE			0x0
#define RAFE_RX_MAX_CNT			0x4
#define RAFE_RX_CALC_IDX		0x8
#define RAFE_RX_DRX_IDX			0xC

/*  RAFE_FE_RST_GL  */
#define RAFE_RST_GL_PSE_RESET		0x0001

/*  RAFE_FE_INT_STATUS  && RAFE_FE_INT_ENABLE  */

#if defined(CONFIG_RT5350_ASIC) || defined(CONFIG_MT7628_ASIC) || defined(CONFIG_RT7620_ASIC)
#define RAFE_INT_TX_DONE_INT0     	BIT0
#define RAFE_INT_TX_DONE_INT1     	BIT1
#define RAFE_INT_TX_DONE_INT2     	BIT2
#define RAFE_INT_TX_DONE_INT3     	BIT3
#define RAFE_INT_RX_DONE_INT0     	BIT16
#define RAFE_INT_RX_DONE_INT1     	BIT17
#define RAFE_INT_TX_DLY_INT       	BIT28
#define RAFE_INT_TX_COHERENT      	BIT29
#define RAFE_INT_RX_DLY_INT		BIT30
#define RAFE_INT_RX_COHERENT      	BIT31

#else /* !CONFIG_RT5350_ASIC */

#define RAFE_INT_RX_DLY_INT		BIT0
#define RAFE_INT_TX_DLY_INT       	BIT1
#define RAFE_INT_RX_DONE_INT0     	BIT2
#define RAFE_INT_TX_DONE_INT0     	BIT8
#define RAFE_INT_TX_DONE_INT1     	BIT9
#define RAFE_INT_TX_DONE_INT2     	BIT10
#define RAFE_INT_TX_DONE_INT3     	BIT11
#define RAFE_INT_RX_COHERENT      	BIT16
#define RAFE_INT_TX_COHERENT      	BIT17
#define RAFE_INT_GE1_STA_CHG		BIT18
#define RAFE_INT_PSE_FQ_EMPTY     	BIT20
#define RAFE_INT_PSE_P0_FC        	BIT21
#define RAFE_INT_PSE_P1_FC        	BIT22
#define RAFE_INT_GDM_OTHER_DROP		BIT23
#define RAFE_INT_PSE_BUF_DROP     	BIT24
#define RAFE_INT_PSE_P2_FC		BIT26
#endif /* CONFIG_RT5350_ASIC */

#define RAFE_INT_ALL			0xA5F70F07
#define RAFE_INT_NONE			0
#define DEF_RA305X_INT_MASK		(RAFE_INT_RX_DONE_INT0 | RAFE_INT_TX_DONE_INT0)

/*  RAFE_GDMA1_FWD_CFG  */
#define RAFE_GDMA1_ICS_EN		BIT22
#define RAFE_GDMA1_TCS_EN		BIT21
#define RAFE_GDMA1_UCS_EN		BIT20
#define RAFE_GDMA1_JMB_EN		BIT19
#define RAFE_GDMA1_DISPAD		BIT18
#define RAFE_GDMA1_DISCRC		BIT17
#define RAFE_GDMA1_STRPCRC		BIT16


#define RAFE_GDMA1_UFRC_P_CPU		0
#define RAFE_GDMA1_UFRC_P_GDMA1		(1 << 12)
#define RAFE_GDMA1_UFRC_P_PPE		(6 << 12)

#define RAFE_GDMA1_BFRC_P_CPU		0
#define RAFE_GDMA1_BFRC_P_GDMA1		(1 << 8)
#define RAFE_GDMA1_BFRC_P_PPE		(6 << 8)

#define RAFE_GDMA1_MFRC_P_CPU		0
#define RAFE_GDMA1_MFRC_P_GDMA1		(1 << 4)
#define RAFE_GDMA1_MFRC_P_PPE		(6 << 4)

#define RAFE_GDMA1_OFRC_P_CPU		0
#define RAFE_GDMA1_OFRC_P_GDMA1		1
#define RAFE_GDMA1_OFRC_P_PPE		6

/*  RAFE_PSE_FQ_CFG  */
#define DEF_RAFE_PSE_FQ_VAL		0x80504000

/*  RAFE_CDMA_CSG_CFG  */
#define RAFE_ICS_GEN_EN			BIT2
#define RAFE_UCS_GEN_EN			BIT1
#define RAFE_TCS_GEN_EN			BIT0


/*  RAFE_PDMA_GLO_CFG  */
#define RAFE_PDMA_TX_DMA_EN         	BIT0
#define RAFE_PDMA_TX_DMA_BUSY       	BIT1
#define RAFE_PDMA_RX_DMA_EN         	BIT2
#define RAFE_PDMA_RX_DMA_BUSY       	BIT3
#define RAFE_PDMA_BT_SIZE_4DWORDS       (0<<4)
#define RAFE_PDMA_BT_SIZE_8DWORDS       (1<<4)
#define RAFE_PDMA_BT_SIZE_16DWORDS      (2<<4)
#define RAFE_PDMA_TX_WB_DDONE       	BIT6
#define RAFE_RX_2B_OFFSET		BIT31

/*  RAFE_PDMA_RST_CFG  */
#define RAFE_PDMA_PST_DTX_IDX0       	BIT0
#define RAFE_PDMA_PST_DTX_IDX1       	BIT1
#define RAFE_PDMA_PST_DTX_IDX2       	BIT2
#define RAFE_PDMA_PST_DTX_IDX3       	BIT3
#define RAFE_PDMA_PST_DRX_IDX0       	BIT16



/*  Ethernet switch  */
#define RAESW_REG(_off)			HAL_ETHSW_REG(_off)

#if defined(CONFIG_RT7620_ASIC)
#define RAESW_INTSTATUS_REG		0x700c
#define RAESW_INTMASK_REG		0x7008
#else
#define RAESW_INTSTATUS_REG		0x0000
#define RAESW_INTMASK_REG		0x0004
#endif
#define RAESW_REG_OFF_08		0x0008
#define RAESW_REG_OFF_14		0x0014

/*  Address table register  */
#define RAESW_ADDRTAB_SEARCH_REG	0x0024
#define RAESW_ADDRTAB_STATUS0_REG       0x0028
#define RAESW_ADDRTAB_STATUS1_REG       0x002C
#define RAESW_ADDRTAB_STATUS2_REG       0x0030
#define RAESW_MACWT_AD0_REG		0x0034
#define RAESW_MACWT_AD1_REG             0x0038
#define RAESW_MACWT_AD2_REG             0x003C


/*  VLAN registers  */
#define RAESW_PVID0_REG			0x0040
#define RAESW_PVID_REG_BASE		RAESW_PVID0_REG
#define RAESW_PVID1_REG			0x0044
#define RAESW_PVID2_REG			0x0048
#define RAESW_PVID3_REG			0x004C
#define RAESW_VLANID0_REG		0x0050
#define RAESW_VLANID_REG_BASE		RAESW_VLANID0_REG
#define RAESW_VLANID1_REG		0x0054
#define RAESW_VLANID2_REG		0x0058
#define RAESW_VLANID3_REG		0x005C
#define RAESW_VLANID4_REG		0x0060
#define RAESW_VLANID5_REG		0x0064
#define RAESW_VLANID6_REG		0x0068
#define RAESW_VLANID7_REG		0x006C
#define RAESW_VLANMEM0_REG		0x0070
#define RAESW_VLANMEM_REG_BASE		RAESW_VLANMEM0_REG
#define RAESW_VLANMEM1_REG		0x0074
#define RAESW_VLANMEM2_REG		0x0078
#define RAESW_VLANMEM3_REG		0x007C
#define RAESW_PORT_ABILITY_REG		0x0080
#define RAESW_REG_OFF_84		0x0084
#define RAESW_REG_OFF_8C		0x008C
#define RAESW_PORTCTRL0_REG		0x0090
#define RAESW_PORTCTRL1_REG		0x0094
#define RAESW_PORTCTRL2_REG		0x0098
#define RAESW_GCTRL_REG			0x009C

/*  PHY register  */
#if defined(CONFIG_RT7620_ASIC) // 7620 OSBN PHY MODE
#define RAESW_PHY_CTRL0_REG		0x7004
#else // RALINK PHY MODE
#define RAESW_PHY_CTRL0_REG		0x00C0
#define RAESW_PHY_CTRL1_REG		0x00C4
#endif // RALINK/OSBN PHY MODE

#define RAESW_REG_OFF_C8		0x00C8
#define RAESW_REG_OFF_CC		0x00CC
#define RAESW_REG_OFF_E4		0x00E4

#if defined(CONFIG_RT5350_ASIC) || defined(CONFIG_MT7628_ASIC)
/*  LED register  */
#define RAESW_LED_CONTROL		0x0168
#endif /* CONFIG_RT5350_ASIC */


/*  RAESW_INTSTATUS_REG && RAESW_INTMASK_REG  */
#if defined(CONFIG_RT7620_ASIC)  // 7620 OSBN PHY MODE
#define P5_LINK_CH			BIT5
#define P4_LINK_CH			BIT4
#define P3_LINK_CH			BIT3
#define P2_LINK_CH			BIT2
#define P1_LINK_CH			BIT1
#define P0_LINK_CH			BIT0

#define DEF_RA305X_ESW_INT_MASK		(P0_LINK_CH | P1_LINK_CH  |P2_LINK_CH | P3_LINK_CH | P4_LINK_CH | P5_LINK_CH)
#define RA305X_ESW_INT_ALL		0xffffffff

/*  mii registers  */
#define PHY_CONTROL_REG             0x00
#define PHY_STATUS_REG              0x01
#define PHY_ID_ONE                  0x02
#define PHY_ID_TWO                  0x03
#define PHY_AUTONEG_ADVERT          0x04
#define PHY_AUTONEG_REMOTE          0x05
#define PHY_STATUS_DETECT_REG       0x12

// These are the same for both AUTONEG registers
#define PHY_AUTONEG_NEXT            BIT15
#define PHY_AUTONEG_ACK             BIT14
#define PHY_AUTONEG_REMOTEFAULT     BIT13
#define PHY_AUTONEG_100BASET4       BIT9
#define PHY_AUTONEG_100BASETX_FDX   BIT8
#define PHY_AUTONEG_100BASETX_HDX   BIT7
#define PHY_AUTONEG_10BASET_FDX     BIT6
#define PHY_AUTONEG_10BASET_HDX     BIT5
#define PHY_AUTONEG_CSMA_802_3      BIT0
#define PHY_AUTONEG_CAP_MASK	    (PHY_AUTONEG_10BASET_HDX   | \
				     PHY_AUTONEG_10BASET_FDX   | \
				     PHY_AUTONEG_100BASETX_HDX | \
				     PHY_AUTONEG_100BASETX_FDX)


#else // RALINK PHY MODE

#define RAESW_INT_P0_QUEUE_FULL		BIT14
#define RAESW_INT_P1_QUEUE_FULL		BIT15
#define RAESW_INT_P2_QUEUE_FULL		BIT16
#define RAESW_INT_P3_QUEUE_FULL		BIT17
#define RAESW_INT_P4_QUEUE_FULL		BIT18
#define RAESW_INT_P5_QUEUE_FULL		BIT19
#define RAESW_INT_P6_QUEUE_FULL		BIT20
#define RAESW_INT_QUEUE_EXHAUSTED	BIT24
#define RAESW_INT_BC_STORM		BIT25
#define RAESW_INT_PORT_ST_CHG		BIT26

#define DEF_RA305X_ESW_INT_MASK		RAESW_INT_PORT_ST_CHG
#define RA305X_ESW_INT_ALL		0xffffffff

/*  RAESW_PVID_REG  */
#define RAESW_PVID_MASK			0xfff
#define RAESW_PVID_SHIFT		12

/*  RAESW_VLANID_REG  */
#define RAESW_VLANID_MASK		0xfff
#define RAESW_VLANID_SHIFT		12

/*  RAESW_VLANMEM_REG  */
#define RAESW_VLANMEM_MASK		0xFF
#define RAESW_VLANMEM_SHIFT		8
#define RAESW_CPU_PORT			6


/*  RAESW_PORT_ABILITY_REG  */
#define RAESW_PHY_DUPLEX_SHIFT		9
#define RAESW_PHY_LINK_SHIFT		25
#define RAESW_PHY_PORT_MASK		0x3f

/*  RAESW_PHY_CTRL0_REG  */
#define RAESW_PHY_MII_WRITE		BIT13
#define RAESW_PHY_MII_READ		BIT14
#define RAESW_PHY_MII_REGNUM_SHIFT	8
#define RAESW_PHY_MII_DATA_SHIFT	16

/*  RAESW_PHY_CTRL1_REG  */
#define RAESW_PHY_MII_WRITEOK		BIT0
#define RAESW_PHY_MII_READOK		BIT1

/*  mii registers  */
#define PHY_CONTROL_REG             0x00
#define PHY_STATUS_REG              0x01
#define PHY_ID_ONE                  0x02
#define PHY_ID_TWO                  0x03
#define PHY_AUTONEG_ADVERT          0x04
#define PHY_AUTONEG_REMOTE          0x05
#define PHY_STATUS_DETECT_REG       0x12

#define PHY_CONTROL_RESET           BIT15
#define PHY_CONTROL_LOOPBACK        BIT14
#define PHY_CONTROL_SPEED100        BIT13
#define PHY_CONTROL_AUTONEG_EN      BIT12
#define PHY_CONTROL_POWERDOWN       BIT11
#define PHY_CONTROL_MII_DIS         BIT10
#define PHY_CONTROL_AUTONEG_RST     BIT9
#define PHY_CONTROL_DPLX_FULL       BIT8
#define PHY_CONTROL_COLLTEST        BIT7
  
#define PHY_STATUS_CAP_T4           BIT15
#define PHY_STATUS_CAP_100TXF       BIT14
#define PHY_STATUS_CAP_100TXH       BIT13
#define PHY_STATUS_CAP_10TF         BIT12
#define PHY_STATUS_CAP_10TH         BIT11
#define PHY_STATUS_CAP_SUPR         BIT6
#define PHY_STATUS_AUTONEG_ACK      BIT5
#define PHY_STATUS_REMOTEFAULT      BIT4
#define PHY_STATUS_CAP_AUTONEG      BIT3
#define PHY_STATUS_LINK_OK          BIT2
#define PHY_STATUS_JABBER           BIT1
#define PHY_STATUS_EXTREGS          BIT0

// These are the same for both AUTONEG registers
#define PHY_AUTONEG_NEXT            BIT15
#define PHY_AUTONEG_ACK             BIT14
#define PHY_AUTONEG_REMOTEFAULT     BIT13
#define PHY_AUTONEG_100BASET4       BIT9
#define PHY_AUTONEG_100BASETX_FDX   BIT8
#define PHY_AUTONEG_100BASETX_HDX   BIT7
#define PHY_AUTONEG_10BASET_FDX     BIT6
#define PHY_AUTONEG_10BASET_HDX     BIT5
#define PHY_AUTONEG_CSMA_802_3      BIT0
#define PHY_AUTONEG_CAP_MASK	    (PHY_AUTONEG_10BASET_HDX   | \
				     PHY_AUTONEG_10BASET_FDX   | \
				     PHY_AUTONEG_100BASETX_HDX | \
				     PHY_AUTONEG_100BASETX_FDX)


#endif // (OSBN/RALINK) PHY MODE endif


#define ETH_ADDR_LEN			6
#define ETH_CRC_LEN			4
#define ETH_MIN_FRAMELEN		60
#define ETH_MIN_VLANFRAME_LEN		64

#define ETH_VLANTAG_LEN			4
#ifndef ETHERTYPE_VLAN
#define ETHERTYPE_VLAN			0x8100
#endif


/*  PDMA RX descriptor  */
typedef struct pdma_rxdesc_s {
	cyg_uint32 pbuf0;
	cyg_uint32 rx_status;
	cyg_uint32 reserved0;
	cyg_uint32 rx_info;
} pdma_rxdesc_t;

/*  Rx status  */
#define RX_PLEN_MASK			0x3fff
#define RX_PLEN1_MASK			RX_PLEN_MASK
#define RX_LS1				BIT14
#define RX_PLEN0_SHIFT			16
#define RX_PLEN0_MASK			(RX_PLEN_MASK << RX_PLEN0_SHIFT)
#define RX_LS0				BIT30
#define RX_DDONE			BIT31
#define RX_PLEN1(_status)		((_status) & RX_PLEN1_MASK)
#define RX_PLEN0(_status)		(((_status) >> RX_PLEN0_SHIFT) & RX_PLEN_MASK)

#define RA305X_RXBUF_OFFSET		2

#if defined(CONFIG_PDMA_NEW)

/*  Rx info  */
#define RX_FOE_ENTRY_MASK		0x3fff
//#define RX_FVLD				BIT14
#define RX_CRSN_SHIFT			14
#define RX_CRSN_MASK			(0x1f << RX_AI_SHIFT)
#define RX_SP_SHIFT			19
#define RX_SP_MASK			(0x7 << RX_SP_SHIFT)
//#define RX_AIS				BIT27
#define RX_L4F				BIT22
#define RX_L4FVLD			BIT23
#define RX_TACK				BIT24
#define RX_IPF				BIT25
#define RX_IPFVLD			BIT26

#define RX_FOE_ENTRY(_info)		((_info)&RX_FOE_ENTRY_MASK)
//#define RX_AI(_info)			(((_info)>>RX_AI_SHIFT)&0xff)
#define RX_SP(_info)			(((_info)>>RX_SP_SHIFT) & 0x7)

#else // #if defined(CONFIG_PDMA_NEW)

/*  Rx info  */
#define RX_FOE_ENTRY_MASK		0x3fff
#define RX_FVLD				BIT14
#define RX_AI_SHIFT			16
#define RX_AI_MASK			(0xff << RX_AI_SHIFT)
#define RX_SP_SHIFT			24
#define RX_SP_MASK			(0x7 << RX_SP_SHIFT)
#define RX_AIS				BIT27
#define RX_L4F				BIT28
#define RX_IPF				BIT29
#define RX_L4FVLD			BIT30
#define RX_IPFVLD			BIT31

#define RX_FOE_ENTRY(_info)		((_info)&RX_FOE_ENTRY_MASK)
#define RX_AI(_info)			(((_info)>>RX_AI_SHIFT)&0xff)
#define RX_SP(_info)			(((_info)>>RX_SP_SHIFT) & 0x7)
#endif // #if defined(CONFIG_PDMA_NEW)

/*  PDMA TX descriptor  */
typedef struct pdma_txdesc_s {
	cyg_uint32 pbuf0;
	cyg_uint32 tx_buf;
	cyg_uint32 pbuf1;
	cyg_uint32 tx_ctrl;
} pdma_txdesc_t;


/*  tx_buf  */
#define TX_BUFLEN_MASK			0x3fff
#define TX_LS1				BIT14
#define TX_BURST			BIT15
#define TX_BUF0_LEN_SHIFT		16
#define TX_BUF0_LEN_MASK		(TX_BUFLEN_MASK << TX_BUF0_LEN_SHIFT)
#define TX_LS0				BIT30
#define TX_DDONE			BIT31

#define TX_BUF1_LEN(_len)		((_len)&TX_BUFLEN_MASK)
#define TX_BUF0_LEN(_len)		(((_len)&TX_BUFLEN_MASK) << TX_BUF0_LEN_SHIFT)


/*  tx_ctrl  */
#define TX_VIDX(_vidx)			((_vidx)&0xf)
#define TX_VPRI(_pri)			(((_pri)&0x7)<< 4)
#define TX_INSV				BIT7
#define TX_SIDX(_sidx)			(((_sidx) & 0xf) << 8)
#define TX_INSP				BIT12

#if defined(CONFIG_PDMA_NEW)

#define TX_UDF(_udf)			(((_udf) & 0x1f) << 15)
#define TX_FP_BMAP(_bm)			(((_bm) & 0xff) << 20)
#define TX_TSO				BIT28

#else // CONFIG_PDMA_NEW

#define TX_RXIF				BIT13
#define TX_QN(_qn)			(((_qn) & 0x7) << 16)
#define TX_PN(_pn)			(((_pn) & 0x7) << 24)

#endif // CONFIG_PDMA_NEW

#define TX_TCO				BIT29
#define TX_UCO				BIT30
#define TX_ICO				BIT31


/*  interface  */
typedef struct if_ra305x_s {
	struct eth_drv_sc *sc;
	struct if_ra305x_s *next;
	
	cyg_uint16 unit;
	cyg_uint16 init_portmask;
	cyg_uint16 vid;
	cyg_uint16 portmask;
	cyg_uint8 *ptxvtag;
	cyg_uint8 macaddr[ETH_ADDR_LEN];
} if_ra305x_t;


/*  Uncached buffer  */
typedef struct ubuf_s {
	struct ubuf_s *next;
} ubuf_t;
#define UBUF_LEN		1536
#define UBUF_ALIGN		32

/*  buffer descriptor for Tx  */
typedef struct bufdesc_s {
	struct eth_drv_sc *sc;
	void *buf;
//	cyg_uint8 type;
} bufdesc_t;

#define BUF_TYPE_NONE		0
#define BUF_TYPE_UBUF		1
#define BUF_TYPE_SGLIST		2


/*  Rx machine  */
typedef struct rxctrl_s {
	pdma_rxdesc_t *prxd;
	cyg_uint32 *prxbuf;
	cyg_uint16 num_rxd;
	cyg_uint16 num_freerxd;
	cyg_uint16 rx_head;
//	cyg_uint16 rx_tail;
	
	cyg_uint32 rx_base;
#ifdef KEEP_PRIVATE_STAT
	cyg_uint32 rx_pktcnt;
	cyg_uint32 rx_bytecnt;
	cyg_uint32 rx_tooshort;
	cyg_uint32 rx_noifp;
	cyg_uint32 rx_drop;
#endif
} rxctrl_t;


/*  Tx machine  */
typedef struct txctrl_s {
	pdma_txdesc_t *ptxd;
	bufdesc_t *ptxbuf;
	cyg_uint16 num_txd;
	cyg_uint16 num_freetxd;
	cyg_uint16 tx_head;
	cyg_uint16 tx_tail;

	cyg_uint32 tx_base;
	
#ifdef KEEP_PRIVATE_STAT
	cyg_uint32 tx_pktcnt;
	cyg_uint32 tx_bytecnt;
	cyg_uint32 tx_ubuf;
	cyg_uint32 tx_sglist;
	cyg_uint32 tx_drop;
#endif
} txctrl_t;

#ifdef KEEP_PRIVATE_STAT
#define ADD_PRIVATE_COUNTER(_cnt, _val)		_cnt += _val
#else
#define ADD_PRIVATE_COUNTER(_cnt, _val)
#endif

typedef struct eth_ra305x_s {
	cyg_vector_t vector;		/*  interrupt vector  */
	cyg_handle_t int_hdl;		/*  interrupt handle  */
	cyg_interrupt int_obj;		/*  interrupt object  */
	cyg_uint32 int_mask;
	struct eth_drv_sc *sc_main;

	cyg_vector_t esw_vector;	/*  interrupt vector  */
	cyg_handle_t esw_int_hdl;	/*  interrupt handle  */
	cyg_interrupt esw_int_obj;	/*  interrupt object  */
	cyg_uint32 esw_int_mask;
	cyg_uint32 esw_linkmask;
	
	cyg_uint16 if_cnt;
	cyg_uint16 act_ifmask;
	
	txctrl_t tx0;
	
	rxctrl_t rx0;
	rxctrl_t *act_rx;
	
	if_ra305x_t *iflist;
	
	cyg_uint8 *pzerobuf;
	cyg_uint32 *pvtag;
	cyg_uint16 num_ubuf;
	cyg_uint16 num_freeubuf;
	ubuf_t *pfree_ubuf;
} eth_ra305x_t;

/*
 * Memory segments (32bit kernel mode addresses)
 */
#define KUSEG			0x00000000
#define KSEG0			0x80000000
#define KSEG1			0xa0000000
#define KSEG2			0xc0000000
#define KSEG3			0xe0000000
#define K0BASE			KSEG0

/*
 * Map an address to a certain kernel segment
 */
#define KSEG0ADDR(a)		((a) | KSEG0)
#define KSEG1ADDR(a)		((a) | KSEG1)
#define KSEG2ADDR(a)		((a) | KSEG2)
#define KSEG3ADDR(a)		((a) | KSEG3)

// PHYS_TO_K1
#define PHYS_TO_K1(physaddr) KSEG1ADDR(physaddr)

typedef unsigned int RA2880_REG;

#define sysRegRead(phys)        \
        (*(volatile RA2880_REG *)PHYS_TO_K1(phys))

#define sysRegWrite(phys, val)  \
        ((*(volatile RA2880_REG *)PHYS_TO_K1(phys)) = (val))

#ifdef RT_BIG_ENDIAN
#define cpu_to_le32(x) le32_to_cpu((x))
unsigned int le32_to_cpu(unsigned int x)
{
        return (((x & 0x000000ffU) << 24) |
                ((x & 0x0000ff00U) <<  8) |
                ((x & 0x00ff0000U) >>  8) |
                ((x & 0xff000000U) >> 24));
}
#else
#define cpu_to_le32(x)	(x)
#define le32_to_cpu(x)	(x)
#endif
void if_ra305x_get_eth_count(cyg_uint32* rx_pktcnt, cyg_uint32* rx_bytes,cyg_uint32* tx_pktcnt,cyg_uint32* tx_bytes);


void mii_write_register(int phyaddr, int regnum, int val);
int mii_read_register(int phyaddr, int regnum);

// --------------------------------------------------------------------------
#endif // CYGONCE_IF_RA305X_H
// End of if_ra305x.h


