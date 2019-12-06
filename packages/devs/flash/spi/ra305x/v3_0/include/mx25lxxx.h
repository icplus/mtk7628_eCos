#ifndef CYGONCE_DEVS_FLASH_SPI_MX25LXXX_H
#define CYGONCE_DEVS_FLASH_SPI_MX25LXXX_H

//=============================================================================
//
//      mx25lxxx.h
//
//      SPI flash driver for MXIC MX25Lxxx devices and compatibles.
//
//=============================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2008, 2009 Free Software Foundation, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under    
// the terms of the GNU General Public License as published by the Free     
// Software Foundation; either version 2 or (at your option) any later      
// version.                                                                 
//
// eCos is distributed in the hope that it will be useful, but WITHOUT      
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License    
// for more details.                                                        
//
// You should have received a copy of the GNU General Public License        
// along with eCos; if not, write to the Free Software Foundation, Inc.,    
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.            
//
// As a special exception, if other files instantiate templates or use      
// macros or inline functions from this file, or you compile this file      
// and link it with other works to produce a work based on this file,       
// this file does not by itself cause the resulting work to be covered by   
// the GNU General Public License. However the source code for this file    
// must still be made available in accordance with section (3) of the GNU   
// General Public License v2.                                               
//
// This exception does not invalidate any other reasons why a work based    
// on this file might be covered by the GNU General Public License.         
// -------------------------------------------                              
// ####ECOSGPLCOPYRIGHTEND####                                              
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   
// Date:        2009-10-26
// Purpose:     MXIC MX25Lxxx SPI flash driver implementation
//
//####DESCRIPTIONEND####
//
//=============================================================================

// Required data structures.
#include <cyg/io/flash_dev.h>

// Exported handle on the driver function table.
externC struct cyg_flash_dev_funs cyg_devs_flash_spi_mx25lxxx_funs;

//-----------------------------------------------------------------------------
// Macro used to generate a flash device object with the default MX25LXXX 
// settings.  Even though the block info data structure is declared here, the 
// details are not filled in until the device type is inferred during 
// initialisation.  This also applies to the 'end' field which is calculated 
// using the _start_ address and the inferred size of the device.
// _name_   is the root name of the instantiated data structures.
// _start_  is the base address of the device - for SPI based devices this can
//          have an arbitrary value, since the device is not memory mapped.
// _spidev_ is a pointer to a SPI device object of type cyg_spi_device.  This
//          is not typechecked during compilation so be careful!

#define CYG_DEVS_FLASH_SPI_MX25LXXX_DRIVER(_name_, _start_, _spidev_) \
struct cyg_flash_block_info _name_ ##_block_info; \
CYG_FLASH_DRIVER(_name_, &cyg_devs_flash_spi_mx25lxxx_funs, 0, \
  _start_, _start_, 1, & _name_ ##_block_info, (void*) _spidev_)
#ifdef CONFIG_MT7628_ASIC

#define SPI_WRITE_ENABLE	0
#define SPI_WRITE_DISABLE	1
#define SPI_RD_STATUS		2
#define SPI_WR_STATUS		3
#define SPI_RD_DATA		4
#define SPI_FAST_RD_DATA	5
#define SPI_PAGE_PROGRAM	6
#define SPI_SECTOR_ERASE	7
#define SPI_BULK_ERASE		8
#define SPI_DEEP_PWRDOWN	9
#define SPI_RD_SIG		10
#define SPI_RD_ID		11
#define SPI_RD_STATUS2		12
#define SPI_HPM_ENABLE		13
#define SPI_MAX_OPCODES		14

#define SFI_WRITE_BUFFER_SIZE	4
#define SFI_FLASH_ADDR_MASK	0x00ffffff

/*
 * ST Microelectronics Opcodes for Serial Flash
 */

#define STM_OP_WR_ENABLE	0x06	/* Write Enable */
#define STM_OP_WR_DISABLE	0x04	/* Write Disable */
#define STM_OP_RD_STATUS	0x05	/* Read Status */
#define STM_OP_RD_STATUS2	0x35	/* Read Status 2 */
#define STM_OP_WR_STATUS	0x01	/* Write Status */
#define STM_OP_RD_DATA		0x03	/* Read Data */
#define STM_OP_FAST_RD_DATA	0x0b	/* Fast Read Data */
#define STM_OP_PAGE_PGRM	0x02	/* Page Program */
#define STM_OP_SECTOR_ERASE	0xd8	/* Sector Erase */
#define STM_OP_BULK_ERASE	0xc7	/* Bulk Erase */
#define STM_OP_DEEP_PWRDOWN	0xb9	/* Deep Power-Down Mode */
#define STM_OP_RD_SIG		0xab	/* Read Electronic Signature */
#define STM_OP_RD_ID		0x9f	/* Read JEDEC ID */
#define STM_OP_HPM		    0xa3	/* High Performance Mode */

#define STM_STATUS_WIP		0x01	/* Write-In-Progress */
#define STM_STATUS_WEL		0x02	/* Write Enable Latch */
#define STM_STATUS_BP0		0x04	/* Block Protect 0 */
#define STM_STATUS_BP1		0x08	/* Block Protect 1 */
#define STM_STATUS_BP2		0x10	/* Block Protect 2 */
#define STM_STATUS_SRWD		0x80	/* Status Register Write Disable */

#define STM_STATUS_QE		0x02	/* Quad Enable */

#define SPI_REG_CTL		    (0x00)
#define SPI_REG_OPCODE		(0x04)
#define SPI_REG_DATA0		(0x08)
#define SPI_REG_DATA(x)		(SPI_REG_DATA0 + (x * 4))
#define SPI_REG_MASTER		(0x28)
#define SPI_REG_MOREBUF		(0x2c)
#define SPI_REG_Q_CTL		(0x30)
#define SPI_REG_SPACE_CR	(0x3c)

#define SPI_CTL_START		0x00000100
#define SPI_CTL_BUSY		0x00010000
#define SPI_CTL_TXCNT_MASK	0x0000000f
#define SPI_CTL_RXCNT_MASK	0x000000f0
#define SPI_CTL_TX_RX_CNT_MASK	0x000000ff
#define SPI_CTL_SIZE_MASK	0x00180000
#define SPI_CTL_ADDREXT_MASK	0xff000000

#define SPI_MBCTL_TXCNT_MASK		0x000001ff
#define SPI_MBCTL_RXCNT_MASK		0x001ff000
#define SPI_MBCTL_TX_RX_CNT_MASK	(SPI_MBCTL_TXCNT_MASK | SPI_MBCTL_RXCNT_MASK)
#define SPI_MBCTL_CMD_MASK		0x2f000000

#define SPI_CTL_CLK_SEL_MASK	0x03000000
#define SPI_OPCODE_MASK		0x000000ff

#define SPI_STATUS_WIP		STM_STATUS_WIP
#else

//=============================================================================

#define RA305X_SPISTAT_REG	0
#define RA305X_SPICTL_REG		0x14
#define RA305X_SPIDATA_REG	0x20

/* SPICFG register bit field */
#define RA305X_CFG_LSBFIRST		(0<<8)
#define RA305X_CFG_MSBFIRST		(1<<8)

#define RA305X_RXCLKEDGE_FALLING	(1<<5)		/* rx on the falling edge of the SPICLK signal */
#define RA305X_TXCLKEDGE_FALLING	(1<<4)		/* tx on the falling edge of the SPICLK signal */

#define RA305X_CLK_DIV2		0	/* system clock rat / 2  */
#define RA305X_CLK_DIV4		1	/* system clock rat / 4  */
#define RA305X_CLK_DIV8		2	/* system clock rat / 8  */
#define RA305X_CLK_DIV16		3	/* system clock rat / 16  */
#define RA305X_CLK_DIV32		4	/* system clock rat / 32  */
#define RA305X_CLK_DIV64		5	/* system clock rat / 64  */
#define RA305X_CLK_DIV128		6	/* system clock rat / 128 */

#define RA305X_CLK_POL		(1<<6)		/* spi clk*/

/* SPICTL register bit field */
#define RA305X_CTL_HIZSDO			(1<<3)
#define RA305X_CTL_STARTWR		(1<<2)
#define RA305X_CTL_STARTRD		(1<<1)
#define RA305X_CTL_SPIENA_LOW		0	/* #cs low active */
#define RA305X_CTL_SPIENA_HIGH		1

#endif
#endif // CYGONCE_DEVS_FLASH_SPI_MX25LXXX_H
