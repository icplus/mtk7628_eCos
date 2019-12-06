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
    flash.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#ifndef _FLASH_H
#define _FLASH_H


#define FLASH_BASE  0x80c00000

#include <cyg/kernel/kapi.h>

#ifdef SMALL_UBOOT_PARTITION
#define FLASH_PROTECT_AREA	0x10000
#else
#define FLASH_PROTECT_AREA	0x30000
#endif

#define CFG_FLSH_BOOT_SZ   		0x1000
#define CFG_FLSH_CONF_SZ   		0x5000

// for bottom type flash
#define	CFG_FLSH_BOOT_LOC_BOT	0x3000
#define	CFG_FLSH_CONF_LOC_BOT	0x4000
#define CFG_FLSH_FMW_LOC_BOT   	0x10000

// for top type flash 
#define	CFG_FLSH_BOOT_LOC_TOP	0xf000
#define CFG_FLSH_FMW_LOC_TOP   	0x10000

// for unitform 64K block flash
/* eCos SPI flash configuration */
#ifdef SMALL_UBOOT_PARTITION
#define	CFG_FLSH_BOOT_LOC_NO_BOOT_SEC	0x01f000
#define	CFG_FLSH_CONF_LOC_NO_BOOT_SEC	0x21000
#define	CFG_FLSH_FMW_LOC_NO_BOOT_SEC	0x30000
#define	CFG_FLSH_CONF_SZ_NO_BOOT_SEC	0x5000
#else
// for unitform 64K block flash
#define	CFG_FLSH_BOOT_LOC_NO_BOOT_SEC	0x40000
#define	CFG_FLSH_CONF_LOC_NO_BOOT_SEC	0x31000
#define	CFG_FLSH_FMW_LOC_NO_BOOT_SEC	0x50000
#define	CFG_FLSH_CONF_SZ_NO_BOOT_SEC	0x5000
#endif



#define	F_DELAY_US(us)	HAL_DELAY_US(us)

#define F_UNIMPLEMENTED        -1
#define F_OK                    0       // Successful operation
#define F_ER_PROG_OK            F_OK    // Programmed OK
#define F_ER_NOT_ERASED         1       // Erase check before programming failed
#define F_ER_PROG_FAIL          2       // Program operation failed
#define F_ER_VERIFY_FAIL        3       // Verify of programming failed
#define F_ER_ERASE_FAIL         4       // Erase operation failed
#define F_ER_NOT_FLASH          5       // Address range supplied is not flash
#define F_ER_ODD_BYTES          6       // An odd number of bytes was requested
#define F_ER_ALIGNED            6       // not word aligned error
#define F_ER_TIMEOUT            7       // Timeout occured during programming
#define F_ER_NOT_READY          8       // Not ready 
#define	F_ER_INVALID			9		// nand flash invalid block
#define	F_ER_CHKSUM				10		// nand flash oob check sum not matched

#define MAX_BLOCK           256 //136 //40 , for mxlv640, 127+8 block 

#define	F_ERASE_TO	1000000	//1sec
#define	F_PROG_TO	360		//360us

typedef struct
{
    unsigned int base ;
    int id ;
	unsigned int size;
    char flags ;
	char type ;
    short block_num;
    unsigned char block[MAX_BLOCK];
} tFlash ;

// Valid values for erase_type paramter to f_erase()
#define F_CHIP_ERASE    0       // Erase the entire chip
#define F_SECTOR_ERASE  1       // Erase only the addressed sector
#define	F_BLOCK_UNIT_SFT 10	// block size in 1K unit
#define	F_BLOCK_UNIT	(1<<F_BLOCK_UNIT_SFT)	// block size in 1K unit

#define	F_FLAG_SETUP	1	// initial

#define	F_BOOT_SEC_NONE	0	// no boot sector
#define	F_BOOT_SEC_BOT	1	// bottom
#define	F_BOOT_SEC_TOP	2	// top
/* nrflash.c */

extern tFlash FlshParm, *pFlsh ;

extern int flsh_erase(unsigned int faddr, unsigned int len);
extern int flsh_write(unsigned int faddr, unsigned int from, unsigned int len);
extern int flsh_init(void);
int flsh_block_start(int faddr, int *len);

#endif



