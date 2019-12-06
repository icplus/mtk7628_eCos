
#ifndef CYGONCE_RA_PCIE_H
#define CYGONCE_RA_PCIE_H
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
	mt_pcie.h

    Abstract:
	Hardware driver for Ralink PCIE
   
    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/


#ifndef RAPCI_REG
#define RAPCI_REG(_off)                  HAL_PCI_REG(_off)
#endif

#define RAPCI_MEMWIN_BASE                (HAL_RA305X_PCI_BASE+0x10000)
#define RAPCI_IOWIN_BASE                 (HAL_RA305X_PCI_BASE+0x20000)

#define RAPCI_BRG_RELATED                0x0
//#define RAPCI_PCICFG_REG                 (RAPCI_BRG_RELATED+0x0)
#define RAPCI_PCIINT_REG                 (RAPCI_BRG_RELATED+0x08)
#define RAPCI_PCIENA_REG                 (RAPCI_BRG_RELATED+0x0C)
#define RAPCI_CFGADDR_REG                (RAPCI_BRG_RELATED+0x20)
#define RAPCI_CFGDATA_REG                (RAPCI_BRG_RELATED+0x24)
#define RAPCI_MEMBASE_REG                (RAPCI_BRG_RELATED+0x28)
#define RAPCI_IOBASE_REG                 (RAPCI_BRG_RELATED+0x2C)
#define RAPCI_ARBCTL_REG                 (RAPCI_BRG_RELATED+0x80)

#define RAPCIE_RC_CTL_RELATED             0x2000
#define RAPCIE_BAR0SETUP_REG              (RAPCIE_RC_CTL_RELATED+0x10)
#define RAPCIE_IMBASEBAR0_REG             (RAPCIE_RC_CTL_RELATED+0x18)
//#define RAPCIE_ID_REG                     (RAPCIE_RC_CTL_RELATED+0x30)
//#define RAPCIE_CLASS_REG                  (RAPCIE_RC_CTL_RELATED+0x34)
//#define RAPCIE_SUBID_REG                  (RAPCIE_RC_CTL_RELATED+0x38)
#define RAPCIE_STATUS_REG                 (RAPCIE_RC_CTL_RELATED+0x50)


#define	PCI_CFG_VEN_ID_REG 				0x0
#define	PCI_CFG_DEV_ID_REG 				0x2
#define	PCI_CFG_CMD_REG 				0x4
#define	PCI_CFG_STAT_REG 				0x6
#define	PCI_CFG_REV_REG	 				0x8
#define	PCI_CFG_IF_REG 					0x9
#define	PCI_CFG_SUBCLASS_REG 			0xa
#define	PCI_CFG_CLASS_REG 				0xb
#define	PCI_CFG_HDR_TYPE_REG 			0xe
#define	PCI_CFG_BIST_REG 				0xf
#define	PCI_CFG_BAR0_REG 				0x10
#define	PCI_CFG_BAR1_REG 				0x14
#define	PCI_CFG_BAR2_REG 				0x18
#define	PCI_CFG_BAR3_REG 				0x1c
#define	PCI_CFG_BAR4_REG 				0x20
#define	PCI_CFG_BAR5_REG 				0x24
#define	PCI_CFG_CIS_REG	 				0x28	/* Cardbus only */
#define PCI_CFG_SUB_VEN_ID_REG			0x2c
#define PCI_CFG_SUB_DEV_ID_REG			0x2e
#define PCI_CFG_ROM_BASE_REG			0x30
#define PCI_CFG_RES0_REG				0x34
#define PCI_CFG_RES1_REG				0x38
#define PCI_CFG_INT_REG					0x3c
#define PCI_CFG_CACHE_REG				0x3d
#define PCI_CFG_GNT_REG					0x3e
#define PCI_CFG_LAT_REG					0x3f

/* bridge devices only
 */
#define	PCI_CFG_BAR0_REG 				0x10
#define	PCI_CFG_BAR1_REG 				0x14
#define	PCI_CFG_PRI_BUS_REG				0x18
#define	PCI_CFG_SEC_BUS_REG				0x19
#define	PCI_CFG_SUB_BUS_REG				0x1a
#define	PCI_CFG_SEC_LAT_REG				0x1b
#define	PCI_CFG_IO_BASE_LO_REG			0x1c
#define	PCI_CFG_IO_LIMIT_LO_REG			0x1d
#define	PCI_CFG_MEM_BASE_REG			0x20
#define	PCI_CFG_MEM_LIMIT_REG			0x22
#define	PCI_CFG_PREFETCH_BASE_REG		0x24
#define	PCI_CFG_PREFETCH_LIMIT_REG		0x26
#define	PCI_CFG_IO_BASE_HI_REG			0x30
#define	PCI_CFG_IO_LIMIT_HI_REG			0x32

/* CLASS Codes
 */
#define PCI_CLASS_OLD					0x00
#define PCI_CLASS_MASS					0x01
#define PCI_CLASS_NET					0x02
#define PCI_CLASS_DISPLAY				0x03
#define PCI_CLASS_MULTIMEDIA			0x04
#define PCI_CLASS_MEMORY				0x05
#define PCI_CLASS_BRIDGE				0x06
#define PCI_CLASS_SIMPLE_COMM			0x07
#define PCI_CLASS_BASE_PERIPH			0x08
#define PCI_CLASS_INPUT					0x09
#define PCI_CLASS_DOCK					0x0a
#define PCI_CLASS_CPU					0x0b
#define PCI_CLASS_SERIAL_BUS			0x0c
#define PCI_CLASS_UNKNOWN				0xff

/* BRIDGE SUBCLASS Codes
 */
#define PCI_SUBCLASS_BRIDGE_PCI_PCI		0x04

/* BAR types
 */
#define PCI_MEM_BAR_TYPE_32BIT			0x00
#define PCI_MEM_BAR_TYPE_1M				0x01
#define PCI_MEM_BAR_TYPE_64BIT			0x02

/* PCI Command Register bit defines
 */
#define PCI_CFG_CMD_IO              	0x0001
#define PCI_CFG_CMD_MEM		        	0x0002
#define PCI_CFG_CMD_MASTER          	0x0004
#define PCI_CFG_CMD_SPECIAL         	0x0008
#define PCI_CFG_CMD_INVALIDATE      	0x0010
#define PCI_CFG_CMD_VGA_SNOOP       	0x0020
#define PCI_CFG_CMD_PARITY          	0x0040
#define PCI_CFG_CMD_WAIT            	0x0080
#define PCI_CFG_CMD_SERR            	0x0100
#define PCI_CFG_CMD_FAST_BACK       	0x0200

#define RT2880_PCI_SLOT1_BASE		0x20000000


#endif


