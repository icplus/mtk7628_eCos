//=============================================================================
//
//      mx25lxxx.c
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

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <cyg/io/flash.h>
#include <cyg/io/flash_dev.h>
#include <cyg/io/mx25lxxx.h>

#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/infra/diag.h>

#include <cyg/hal/plf_io.h>
#include <cyg/hal/hal_if.h>

#include <pkgconf/devs_flash_spi_mx25lxxx.h>

#include <string.h>

//==============================================================================
//                                    MACROS
//==============================================================================
#define FLASH_PAGESIZE		256

/* Flash opcodes. */
#define OPCODE_WREN		6	/* Write enable */
#define OPCODE_WRDI		4	/* Write disable */
#define OPCODE_RDSR		5	/* Read status register */
#define OPCODE_WRSR		1	/* Write status register */
#define OPCODE_READ		3	/* Read data bytes */
#define OPCODE_PP		2	/* Page program */
#define OPCODE_SE		0xD8	/* Sector erase */
#define OPCODE_RES		0xAB	/* Read Electronic Signature */
#define OPCODE_RDID		0x9F		/* Read JEDEC ID */

/* Status Register bits. */
#define SR_WIP			1	/* Write in progress */
#define SR_WEL			2	/* Write enable latch */
#define SR_BP0			4	/* Block protect 0 */
#define SR_BP1			8	/* Block protect 1 */
#define SR_BP2			0x10	/* Block protect 2 */
#define SR_EPE			0x20	/* Erase/Program error */
#define SR_SRWD			0x80	/* SR write protect */

#ifdef MXDBG
#define ra_dbg(args...) do { if (1) diag_printf(args); } while(0)
#else
#define ra_dbg(args...) do  { } while(0)
#endif /* MXDBG */

#define SPIC_READ_BYTES	1
#define SPIC_WRITE_BYTES	(1<<1)

#define CFG_HZ	(HAL_GET_SYSBUS_FREQ()/2)

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
struct chip_info {
	char *name;
	cyg_uint32 jedec_id;
	cyg_uint8 id;
	cyg_uint16 num_block_info;
	cyg_flash_block_info_t *block_info;
};

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
static unsigned int spi_wait_nsec = 0;

struct chip_info *spi_chip_info;

cyg_flash_block_info_t mx25l160_blk[] = {
	{ 64 * 1024, 32 }
};

cyg_flash_block_info_t mx25l320_blk[] = {
	{ 64 * 1024, 64 }
};

cyg_flash_block_info_t mx25l640_blk[] = {
	{ 64 * 1024, 128 }
};

cyg_flash_block_info_t mx25l1280_blk[] = {
	{ 64 * 1024, 256 }
};

cyg_flash_block_info_t at25df321_blk[] = {
	{ 64 * 1024, 64 }
};

cyg_flash_block_info_t fl016a1f_blk[] = {
	{ 64 * 1024, 32 }
};

cyg_flash_block_info_t Sfl128p_blk[] = {
	{ 64 * 1024, 256 }
};

cyg_flash_block_info_t Sfl128p2_blk[] = {
	{ 256 * 1024, 64 }
};
cyg_flash_block_info_t gd25q32b_blk[] = {
	{ 64 * 1024, 64 }
};


static struct chip_info chips_data[] = {
	/* REVISIT: fill in JEDEC ids, for parts that have them */
	{
		name: "mx25l1605d",
		jedec_id: 0x2015c220, 
		id: 0xc2,
		num_block_info: 1,
		block_info: mx25l160_blk
	},
	{
		name: "mx25l3205d",
		jedec_id: 0x2016c220,
		id: 0xc2,
		num_block_info: 1,
		block_info: mx25l320_blk
	},
	{
		name: "mx25l6405d",
		jedec_id: 0x2017c220,
		id: 0xc2,
		num_block_info: 1,
		block_info: mx25l640_blk
	},
	{
		name: "mx25l12805d",
		jedec_id: 0x2018c220,
		id: 0xc2,
		num_block_info: 1,
		block_info: mx25l1280_blk
	},
	{
		name: "at25df321",
		jedec_id: 0x47000000,
		id: 0x1f,
		num_block_info: 1,
		block_info: at25df321_blk
	},
	{
		name: "fl016a1f",
		jedec_id: 0x02140000,
		id: 0x01,
		num_block_info: 1,
		block_info: fl016a1f_blk
	},
	{
		name: "Sfl128p",
		jedec_id: 0x20180301,
		id: 0x01,
		num_block_info: 1,
		block_info: Sfl128p_blk
	},	
	{
		name: "W25Q32BV",
		jedec_id: 0x40160000,
		id: 0xef,
		num_block_info: 1,
		block_info: gd25q32b_blk
	},
	{
		name: "Sfl128p",
		jedec_id: 0x20180300,
		id: 0x01,
		num_block_info: 1,
		block_info: gd25q32b_blk
	} ,
	{
		name: "GD25Q32B",
		jedec_id: 0x40160000,
		id: 0xc8,
		num_block_info: 1,
		block_info: gd25q32b_blk
   }
};

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

//==============================================================================
//                              EXTERNAL VARIABLES
//==============================================================================

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================

//------------------------------------------------------------------------------
// FUNCTION
//	spic_busy_wait
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
static void udelay(int delay)
{
    CYGACC_CALL_IF_DELAY_US(delay);
}



#ifdef CONFIG_MT7628_ASIC
#define MORE_BUF_MODE
static int spic_busy_wait(void)
{
	int n = 100000;
	do {
		if ((HAL_SPI_REG(SPI_REG_CTL) & SPI_CTL_BUSY) == 0)
			return 0;
		udelay(1);
	} while (--n > 0);

	printf("%s: fail \n", __func__);
	return -1;
}
#ifdef MORE_BUF_MODE
static int bbu_mb_spic_trans(const cyg_uint8 code, const cyg_uint32 addr, cyg_uint8 *buf, const size_t n_tx, const size_t n_rx, int flag)
{
	cyg_uint32 reg;
	int i, q, r;
	int rc = -1;

	if (flag != SPIC_READ_BYTES && flag != SPIC_WRITE_BYTES) {
		printf("we currently support more-byte-mode for reading and writing data only\n");
		return -1;
	}

	/* step 0. enable more byte mode */
	//ra_or(SPI_REG_MASTER, (1 << 2));
	HAL_SPI_REG(SPI_REG_MASTER) |= (1<<2);

	spic_busy_wait();

	/* step 1. set opcode & address, and fix cmd bit count to 32 (or 40) */

	{
		HAL_SPI_REG(SPI_REG_OPCODE) =(code << 24) & 0xff000000;
		//ra_outl(SPI_REG_OPCODE, (code << 24) & 0xff000000);
		//ra_or(SPI_REG_OPCODE, (addr & 0xffffff));
		
		HAL_SPI_REG(SPI_REG_OPCODE) |=(addr & 0xffffff);
	}
	//ra_and(SPI_REG_MOREBUF, ~SPI_MBCTL_CMD_MASK);
	HAL_SPI_REG(SPI_REG_MOREBUF) &= ~SPI_MBCTL_CMD_MASK;

	HAL_SPI_REG(SPI_REG_MOREBUF) |=(32 << 24);
		//ra_or(SPI_REG_MOREBUF, (32 << 24));

	/* step 2. write DI/DO data #0 ~ #7 */
	if (flag & SPIC_WRITE_BYTES) {
		if (buf == NULL) {
			printf("%s: write null buf\n", __func__);
			goto RET_MB_TRANS;
		}
		for (i = 0; i < n_tx; i++) {
			q = i / 4;
			r = i % 4;
			if (r == 0)
				HAL_SPI_REG(SPI_REG_DATA(q))=0;
			//ra_or(SPI_REG_DATA(q), (*(buf + i) << (r * 8)));
			HAL_SPI_REG(SPI_REG_DATA(q)) |=(*(buf + i) << (r * 8));
		}
	}

	/* step 3. set rx (miso_bit_cnt) and tx (mosi_bit_cnt) bit count */
	//ra_and(SPI_REG_MOREBUF, ~SPI_MBCTL_TX_RX_CNT_MASK);
	HAL_SPI_REG(SPI_REG_MOREBUF) &=~SPI_MBCTL_TX_RX_CNT_MASK;
	//ra_or(SPI_REG_MOREBUF, (n_rx << 3 << 12));
	HAL_SPI_REG(SPI_REG_MOREBUF) |= (n_rx << 3 << 12);
	//ra_or(SPI_REG_MOREBUF, n_tx << 3);
	HAL_SPI_REG(SPI_REG_MOREBUF) |= n_tx << 3;

	/* step 4. kick */
	//ra_or(SPI_REG_CTL, SPI_CTL_START);
	HAL_SPI_REG(SPI_REG_CTL) |= SPI_CTL_START;

	/* step 5. wait spi_master_busy */
	spic_busy_wait();
	if (flag & SPIC_WRITE_BYTES) {
		rc = 0;
		goto RET_MB_TRANS;
	}

	/* step 6. read DI/DO data #0 */
	if (flag & SPIC_READ_BYTES) {
		if (buf == NULL) {
			printf("%s: read null buf\n", __func__);
			return -1;
		}
		for (i = 0; i < n_rx; i++) {
			q = i / 4;
			r = i % 4;
			reg = HAL_SPI_REG(SPI_REG_DATA(q));
			*(buf + i) = (cyg_uint8)(reg >> (r * 8));
		}
	}

	rc = 0;
RET_MB_TRANS:
	/* step #. disable more byte mode */
	//ra_and(SPI_REG_MASTER, ~(1 << 2));
	HAL_SPI_REG(SPI_REG_MASTER) &= ~(1 << 2);
	return rc;
}
#endif // MORE_BUF_MODE //
static int bbu_spic_trans(const cyg_uint8 code, const cyg_uint32 addr, cyg_uint8 *buf, const size_t n_tx, const size_t n_rx, int flag)
{ 
	cyg_uint32 reg;

	spic_busy_wait();

	/* step 1. set opcode & address */
	//ra_outl(SPI_REG_OPCODE, ((addr & 0xffffff) << 8));
	HAL_SPI_REG(SPI_REG_OPCODE) = ((addr & 0xffffff) << 8);
	//ra_or(SPI_REG_OPCODE, code);
	HAL_SPI_REG(SPI_REG_OPCODE) |= code;

	/* step 2. write DI/DO data #0 */
	if (flag & SPIC_WRITE_BYTES) {
		if (buf == NULL) {
			printf("%s: write null buf\n", __func__);
			return -1;
		}
		
		HAL_SPI_REG(SPI_REG_DATA0) = 0;
		//ra_outl(SPI_REG_DATA0, 0);
		switch (n_tx) {
		case 8:
			HAL_SPI_REG(SPI_REG_DATA0) |= (*(buf+3) << 24);
			//ra_or(SPI_REG_DATA0, (*(buf+3) << 24));
		case 7:
			HAL_SPI_REG(SPI_REG_DATA0) |= (*(buf+2) << 16);
			
			//ra_or(SPI_REG_DATA0, (*(buf+2) << 16));
		case 6:
			HAL_SPI_REG(SPI_REG_DATA0) |= (*(buf+1) << 8);			
			//ra_or(SPI_REG_DATA0, (*(buf+1) << 8));
		case 5:
		case 2:			
			HAL_SPI_REG(SPI_REG_DATA0) |= *buf;
			//ra_or(SPI_REG_DATA0, *buf);
			break;
		default:
			printf("%s: fixme, write of length %d\n", __func__, n_tx);
			return -1;
		}
	}

	/* step 3. set mosi_byte_cnt */
	//ra_and(SPI_REG_CTL, ~SPI_CTL_TX_RX_CNT_MASK);	
	HAL_SPI_REG(SPI_REG_CTL) &= ~SPI_CTL_TX_RX_CNT_MASK;
	//ra_or(SPI_REG_CTL, (n_rx << 4));
	HAL_SPI_REG(SPI_REG_CTL) |= (n_rx << 4);
	//ra_or(SPI_REG_CTL, n_tx);
	HAL_SPI_REG(SPI_REG_CTL) |= n_tx;

	/* step 4. kick */
	
	HAL_SPI_REG(SPI_REG_CTL) |= SPI_CTL_START;
	//ra_or(SPI_REG_CTL, SPI_CTL_START);

	/* step 5. wait spi_master_busy */
	spic_busy_wait();
	if (flag & SPIC_WRITE_BYTES)
		return 0;

	/* step 6. read DI/DO data #0 */
	if (flag & SPIC_READ_BYTES) {
		if (buf == NULL) {
			printf("%s: read null buf\n", __func__);
			return -1;
		}
		//reg = ra_inl(SPI_REG_DATA0);
		reg = HAL_SPI_REG(SPI_REG_DATA0);
		switch (n_rx) {
		case 4:
			*(buf+3) = (cyg_uint8)(reg >> 24);
		case 3:
			*(buf+2) = (cyg_uint8)(reg >> 16);
		case 2:
			*(buf+1) = (cyg_uint8)(reg >> 8);
		case 1:
			*buf = (cyg_uint8)reg;
			break;
		default:
			printf("%s: fixme, read of length %d\n", __func__, n_rx);
			return -1;
		}
	}
	return 0;
}


#else
static int spic_busy_wait(void)
{
	do {
		if ((HAL_SPI_REG(RA305X_SPISTAT_REG) & 0x01) == 0)
			return 0;
	} while (spi_wait_nsec >> 1);

	return -1;
}

static int spic_transfer(const cyg_uint8 *cmd, int n_cmd, cyg_uint8 *buf, int n_buf, int flag)
{
	int retval = -1;

	/* assert CS and we are already CLK normal high */
	HAL_SPI_REG(RA305X_SPICTL_REG) &= ~RA305X_CTL_SPIENA_HIGH;

	/* write command */
	for (retval = 0; retval < n_cmd; retval++) {
		HAL_SPI_REG(RA305X_SPIDATA_REG) = cmd[retval];
		HAL_SPI_REG(RA305X_SPICTL_REG) |= RA305X_CTL_STARTWR;
		if (spic_busy_wait()) {
			retval = -1;
			goto end_trans;
		}
	}

	/* read / write  data */
	if (flag & SPIC_READ_BYTES) {
		for (retval = 0; retval < n_buf; retval++) {
			HAL_SPI_REG(RA305X_SPICTL_REG) |= RA305X_CTL_STARTRD;
			if (spic_busy_wait()) {
				goto end_trans;
			}
			buf[retval] = (cyg_uint8)HAL_SPI_REG(RA305X_SPIDATA_REG);
		}

	}
	else if (flag & SPIC_WRITE_BYTES) {
		for (retval = 0; retval < n_buf; retval++) {
			HAL_SPI_REG(RA305X_SPIDATA_REG) = buf[retval];
			HAL_SPI_REG(RA305X_SPICTL_REG) |= RA305X_CTL_STARTWR;
			if (spic_busy_wait()) {
				goto end_trans;
			}
		}
	}

end_trans:
	/* de-assert CS and */
	HAL_SPI_REG(RA305X_SPICTL_REG) |= RA305X_CTL_SPIENA_HIGH;

	return retval;
}

static int spic_read(const cyg_uint8 *cmd, size_t n_cmd, cyg_uint8 *rxbuf, size_t n_rx)
{
	return spic_transfer(cmd, n_cmd, rxbuf, n_rx, SPIC_READ_BYTES);
}

static int spic_write(const cyg_uint8 *cmd, size_t n_cmd, const cyg_uint8 *txbuf, size_t n_tx)
{
	return spic_transfer(cmd, n_cmd, (cyg_uint8 *)txbuf, n_tx, SPIC_WRITE_BYTES);
}
#endif

int spic_init(void)
{

#ifndef CONFIG_MT7628_ASIC
	/* use normal(SPI) mode instead of GPIO mode */
	HAL_REG32(HAL_RA305X_SYSCTL_BASE+RA305X_SYSCTL_GPIO_PURPOSE) &= ~(1 << 1);

	/* reset spi block */
	HAL_REG32(HAL_RA305X_SYSCTL_BASE+RA305X_SYSCTL_RESET) = RA305X_SYSCTL_SPI_RST;
	HAL_REG32(HAL_RA305X_SYSCTL_BASE+RA305X_SYSCTL_RESET) = 0;
	udelay(1);

	/* clk_div should depend on spi-flash. */
	HAL_SPI_REG(RA305X_SYSCTL_CFG) = RA305X_CFG_MSBFIRST | RA305X_TXCLKEDGE_FALLING | RA305X_CLK_DIV16 | RA305X_CLK_POL;

	/* set idle state */
	HAL_SPI_REG(RA305X_SPICTL_REG) = RA305X_CTL_HIZSDO | RA305X_CTL_SPIENA_HIGH;

	spi_wait_nsec = (8 * 1000 / ((CFG_HZ / 1000 / 1000 / RA305X_CLK_DIV16) )) >> 1 ;
	return 0;
#endif
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
static int raspi_read_devid(cyg_uint8 *rxbuf, int n_rx)
{
	cyg_uint8 code = OPCODE_RDID;
	int retval;
#ifdef CONFIG_MT7628_ASIC
		retval = bbu_spic_trans(code, 0, rxbuf, 1, 3, SPIC_READ_BYTES);
		if (!retval)
			retval = n_rx;
#else
	retval = spic_read(&code, 1, rxbuf, n_rx);
	if (retval != n_rx) {
		ra_dbg("%s: ret: %x\n", __func__, retval);
		return retval;
	}
#endif
	return retval;
}

static int raspi_read_sr(cyg_uint8 *val)
{
	int retval;
	cyg_uint8 code = OPCODE_RDSR;
#ifdef CONFIG_MT7628_ASIC
	retval = bbu_spic_trans(code, 0, val, 1, 1, SPIC_READ_BYTES);
	return retval;

#else
	retval = spic_read(&code, 1, val, 1);
#endif
	if (retval != 1) {
		ra_dbg("%s: ret: %x\n", __func__, retval);
		return -1;
	}
	return 0;
}

static int raspi_write_sr(cyg_uint8 *val)
{
	int retval;
	cyg_uint8 code = OPCODE_WRSR;
	
#ifdef CONFIG_MT7628_ASIC
	{
		// put the value to be written in address register, so it will be transfered
		cyg_uint32 address = (*val) << 24;
		retval = bbu_spic_trans(code, address, val, 2, 0, SPIC_WRITE_BYTES);
	}
	return retval;
#else
	retval = spic_write(&code, 1, val, 1);
	if (retval != 1) {
		ra_dbg("%s: ret: %x\n", __func__, retval);
		return -1;
	}
	return 0;
#endif
}

static inline int raspi_write_enable(void)
{
	cyg_uint8 code = OPCODE_WREN;
#ifdef CONFIG_MT7628_ASIC
		return bbu_spic_trans(code, 0, NULL, 1, 0, 0);
#else
	return spic_write(&code, 1, NULL, 0);
#endif
	
}

static inline int raspi_write_disable(void)
{
	cyg_uint8 code = OPCODE_WRDI;

#ifdef CONFIG_MT7628_ASIC
	return bbu_spic_trans(code, 0, NULL, 1, 0, 0);
#else
	return spic_write(&code, 1, NULL, 0);
#endif
}

static inline int raspi_unprotect(void)
{
	cyg_uint8 sr = 0;

	if (raspi_read_sr(&sr) < 0) {
		ra_dbg("%s: read_sr fail: %x\n", __func__, sr);
		return -1;
	}

	if ((sr & SR_BP0) == SR_BP0) {
		sr = 0;
		raspi_write_sr(&sr);
	}
}

static int raspi_wait_ready(int sleep_ms)
{
	int count;
	int sr;

	//udelay(1000 * sleep_ms);

	/* one chip guarantees max 5 msec wait here after page writes,
	 * but potentially three seconds (!) after page erase.
	 */
	for (count = 0;  count < ((sleep_ms+1) *1000*500); count++) {
		if ((raspi_read_sr((cyg_uint8 *)&sr)) < 0)
			break;
		else if (!(sr & (SR_WIP | SR_EPE))) {
			return 0;
		}

		udelay(1);
		/* REVISIT sometimes sleeping would be best */
	}

	ra_dbg("%s: read_sr fail: %x\n", __func__, sr);
	return -1;
}

static int mx25lxxx_erase_sector(struct cyg_flash_dev *dev, cyg_flashaddr_t block_base)
{
	cyg_uint8 buf[4];
	int retval = CYG_FLASH_ERR_INVALID;

	//diag_printf("%s: addr:%x\n", __func__, block_base);

	/* Wait until finished previous write command. */
	if (raspi_wait_ready(3))
		return retval;

	/* Send write enable, then erase commands. */
	raspi_write_enable();
	raspi_unprotect();
#ifdef  CONFIG_MT7628_ASIC 
	bbu_spic_trans(OPCODE_SE, block_base, NULL, 4, 0, 0);
	raspi_wait_ready(950);
	retval = CYG_FLASH_ERR_OK;
#else

	/* Set up command buffer. */
	buf[0] = OPCODE_SE;
	buf[1] = block_base >> 16;
	buf[2] = block_base >> 8;
	buf[3] = block_base;

	spic_write(buf, 4, 0 , 0);
	retval = CYG_FLASH_ERR_OK;
	/* Wait until finished write command. */
	raspi_wait_ready(950);
#endif
	return retval;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
struct chip_info *chip_prob(void)
{
	struct chip_info *info, *match;
	cyg_uint8 buf[5];
	cyg_uint32 jedec, weight;
	int i;

	raspi_read_devid(buf, 5);
	jedec = (cyg_uint32)((cyg_uint32)(buf[1] << 24) | ((cyg_uint32)buf[2] << 16) | ((cyg_uint32)buf[3] <<8) | (cyg_uint32)buf[4]);

#ifdef CONFIG_MT7628_ASIC
	diag_printf("flash manufacture id: %x, device id %x %x\n", buf[0], buf[1], buf[2]);
#else
	diag_printf("spi device id: %x %x %x %x %x (%x)\n", buf[0], buf[1], buf[2], buf[3], buf[4], jedec);
#endif

	/* assign default as AT25D */
	weight = 0xffffffff;
	match = &chips_data[0];
	for (i = 0; i < sizeof(chips_data)/sizeof(chips_data[0]); i++) {
		info = &chips_data[i];
		if (info->id == buf[0]) {
#ifdef CONFIG_MT7628_ASIC
			if ((cyg_uint8)(info->jedec_id >> 24 & 0xff) == buf[1] &&
				(cyg_uint8)(info->jedec_id >> 16 & 0xff) == buf[2])
#else
			if (info->jedec_id == jedec)
#endif
				{
				diag_printf("flash: %s\n", info->name);
				return info;
			}

			if (weight > (info->jedec_id ^ jedec)) {
				weight = info->jedec_id ^ jedec;
				match = info;
			}
		}
	}
	diag_printf("Warning: un-recognized chip ID, please update bootloader!\n");

	return match;
}

//------------------------------------------------------------------------------
// FUNCTION
//	mx25lxxx_init
//
// DESCRIPTION
//	Initialise the SPI flash, reading back the flash parameters.
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
static int mx25lxxx_init(struct cyg_flash_dev *dev)
{
	int i;
	
	spic_init();
	spi_chip_info = chip_prob();
	
	dev->num_block_infos = spi_chip_info->num_block_info;
	dev->block_info = spi_chip_info->block_info;
	dev->start = 0;
	dev->end = dev->start;
	for (i=0; i<spi_chip_info->num_block_info; i++)
	{
		dev->end += spi_chip_info->block_info[i].block_size * spi_chip_info->block_info[i].blocks;
	}
	dev->end-=1;
	return CYG_FLASH_ERR_OK;
}

static int mx25lxxx_read(struct cyg_flash_dev *dev, const cyg_flashaddr_t base, void *data, size_t len)
{
	cyg_uint8 cmd[4];
	int rdlen = 0;
	int retval = CYG_FLASH_ERR_INVALID;
	cyg_flashaddr_t base_tmp = base;

	//ra_dbg("%s: addr:%x len:%d \n", __func__, base, len);

	/* sanity checks */
	if (len == 0)
		return retval;

	/* Wait till previous write/erase is done. */
	if (raspi_wait_ready(1)) {
		/* REVISIT status return?? */
		return retval;
	}

#ifdef CONFIG_MT7628_ASIC
		do {
			int rc, more;
	#if 0//def MORE_BUF_MODE
			more = 32;
	#else
			more = 4;
	#endif
			if (len - rdlen <= more) {
	#ifdef MORE_BUF_MODE
				rc = bbu_mb_spic_trans(STM_OP_RD_DATA, base_tmp, (data+rdlen), 0, (len-rdlen), SPIC_READ_BYTES);
	#else
				rc = bbu_spic_trans(OPCODE_READ, base_tmp, (data+rdlen), 4, (len-rdlen), SPIC_READ_BYTES);
	#endif
				if (rc != 0) {
					printf("%s: failed\n", __func__);
					break;
				}
				rdlen = len;
			}
			else {
	#ifdef MORE_BUF_MODE
				rc = bbu_mb_spic_trans(STM_OP_RD_DATA, base_tmp, (data+rdlen), 0, more, SPIC_READ_BYTES);
	#else
				rc = bbu_spic_trans(OPCODE_READ, base_tmp, (data+rdlen), 4, more, SPIC_READ_BYTES);
	#endif
				if (rc != 0) {
					printf("%s: failed\n", __func__);
					break;
				}
				rdlen += more;
				base_tmp += more;
			}
		} while (rdlen < len);
#else

	/* NOTE: OPCODE_FAST_READ (if available) is faster... */

	/* Set up the write data buffer. */
	cmd[0] = OPCODE_READ;
	cmd[1] = base >> 16;
	cmd[2] = base >> 8;
	cmd[3] = base;

	rdlen = spic_read(cmd, 4, (cyg_uint8 *)data, len);

	if (rdlen != len)
		ra_dbg("warning: rdlen != len\n");
#endif
	retval = CYG_FLASH_ERR_OK;
	return retval;
}

static int mx25lxxx_program(struct cyg_flash_dev *dev, cyg_flashaddr_t base, const void *data, size_t len)
{
	cyg_uint32 page_offset, page_size;
	int rc = 0, retlen = 0;
	cyg_uint8 cmd[4];
	int retval = CYG_FLASH_ERR_INVALID;
#ifdef CONFIG_MT7628_ASIC
		int wrto, wrlen, more;
		char *wrbuf;
#endif


	//diag_printf("%s: addr:%x len:%d \n", __func__, base, len);

	/* sanity checks */
	if (len == 0)
		return retval;
	if (base + len > spi_chip_info->block_info->block_size * spi_chip_info->block_info->blocks)
		return retval;

	/* Wait until finished previous write command. */
	if (raspi_wait_ready(1)) {
		return retval;
	}

	/* Set up the opcode in the write buffer. */
	cmd[0] = OPCODE_PP;
	cmd[1] = base >> 16;
	cmd[2] = base >> 8;
	cmd[3] = base;

	/* what page do we start with? */
	page_offset = base % FLASH_PAGESIZE;

	/* write everything in PAGESIZE chunks */
	while (len > 0) {
		if (len > FLASH_PAGESIZE-page_offset)
			page_size = FLASH_PAGESIZE-page_offset;
		else
			page_size = len;
		page_offset = 0;
		
		raspi_wait_ready(3);
		raspi_write_enable();
		raspi_unprotect();
#ifdef CONFIG_MT7628_ASIC
		wrto = base;
		wrlen = page_size;
		wrbuf = data;
		do {
	#ifdef MORE_BUF_MODE
			more = 32;
	#else
			more = 4;
	#endif
			if (wrlen <= more) {
	#ifdef MORE_BUF_MODE
				bbu_mb_spic_trans(STM_OP_PAGE_PGRM, wrto, wrbuf, wrlen, 0, SPIC_WRITE_BYTES);
	#else
				bbu_spic_trans(STM_OP_PAGE_PGRM, wrto, wrbuf, wrlen+4, 0, SPIC_WRITE_BYTES);
	#endif
				retlen += wrlen;
				wrlen = 0;
			}
			else {
	#ifdef MORE_BUF_MODE
				bbu_mb_spic_trans(STM_OP_PAGE_PGRM, wrto, wrbuf, more, 0, SPIC_WRITE_BYTES);
	#else
				bbu_spic_trans(STM_OP_PAGE_PGRM, wrto, wrbuf, more+4, 0, SPIC_WRITE_BYTES);
	#endif
				retlen += more;
				wrto += more;
				wrlen -= more;
				wrbuf += more;
			}
			if (wrlen > 0) {
				raspi_write_disable();
				raspi_wait_ready(2);
				raspi_write_enable();
			}
		} while (wrlen > 0);
#else

		/* write the next page to flash */
		cmd[1] = base >> 16;
		cmd[2] = base >> 8;
		cmd[3] = base;
		rc = spic_write(cmd, 4, (char *)data, page_size);
#endif

		if (rc > 0) {
			retlen += rc;
			if (rc < page_size) {
				diag_printf("%s: rc:%x page_size:%x\n",
						__func__, rc, page_size);
				return retval;
			}
		}

		len -= page_size;
		base += page_size;
		//data += page_size;
		data = (const void *)((char *)data+page_size);
	}
	raspi_wait_ready(100);
	//printf("\n");
	retval = CYG_FLASH_ERR_OK;
	return retval;
}

//=============================================================================
// Fill in the driver data structures.
//=============================================================================
CYG_FLASH_FUNS (
	cyg_devs_flash_spi_mx25lxxx_funs,	// Exported name of function pointers.
	mx25lxxx_init,	// Flash initialisation.
	cyg_flash_devfn_query_nop,	// Query operations not supported.
	mx25lxxx_erase_sector,	// Sector erase.
	mx25lxxx_program,	// Program multiple pages.
	mx25lxxx_read,	// Read arbitrary amount of data.
	cyg_flash_devfn_lock_nop,	// Locking not supported (no per-sector locks).
	cyg_flash_devfn_unlock_nop
);

CYG_FLASH_DRIVER(
	mx25lxxx_flash_device,
	&cyg_devs_flash_spi_mx25lxxx_funs,
	0,
	0,
	0x400000,
	1,
	mx25l160_blk,
	NULL
);

