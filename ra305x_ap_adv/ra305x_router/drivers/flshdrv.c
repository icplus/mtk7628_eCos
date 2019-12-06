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
    flshdrv.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <cyg/kernel/kapi.h>
#include <stdio.h>
#include <cyg/io/flash.h>
#include <cyg/infra/diag.h>

#include "flash.h"
#include <pkgconf/system.h>
//==============================================================================
//                                    MACROS
//==============================================================================
#define flsh_printf diag_printf

#define  FLASH_LOG

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
tFlash FlshParm, *pFlsh ;

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================
int flsh_init(void);

//==============================================================================
//                               EXTERNAL VARIABLES
//==============================================================================


//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================

int flsh_init(void)
{
	cyg_flash_info_t info;
	int ret;
	int i, j, k;
	

	cyg_flash_set_global_printf((cyg_flash_printf *)&diag_printf);
	cyg_flash_init(NULL);
	ret = cyg_flash_get_info(0, &info);
	if (ret != CYG_FLASH_ERR_OK)
		return -1;
	
	memset(FlshParm.block, 0, sizeof(FlshParm.block));
	FlshParm.id = 0;
	FlshParm.base = info.start;
	for (i=0, k=0; i<info.num_block_infos; i++)
	{
		for (j=0; j <info.block_info[i].blocks; j++)
		{
			FlshParm.block[k++] = info.block_info[i].block_size >> F_BLOCK_UNIT_SFT;
		}
	}
	FlshParm.block_num = k;
	FlshParm.type = F_BOOT_SEC_NONE;
	FlshParm.flags |= F_FLAG_SETUP ;
	FlshParm.size = info.end - info.start;
	pFlsh = &FlshParm;
	
	return F_OK;
}

int flsh_erase(unsigned int addr, unsigned int len)
{
	int rc = -1;
	int i;
	unsigned int baddr, baddr_end, block_size ; /* block offset */
	cyg_flashaddr_t err_addr;
	char * pBuffer = NULL;


	rc = 0;        
	if (!(FlshParm.flags&F_FLAG_SETUP))
	{
		diag_printf("FlshParm flag is UP\n");
		rc = flsh_init(); // 0 for succeed
		if (rc)
		{
			diag_printf("flsh_init fails!!!!!\n");
			return rc;
		}
	}

	baddr =0;   /* look from 1st block */
	for (i = 0; (i < FlshParm.block_num) 
		&& (baddr < (addr+len )); baddr+=(FlshParm.block[i]<<F_BLOCK_UNIT_SFT), i++ ) 
	{
		block_size =  (FlshParm.block[i]<<F_BLOCK_UNIT_SFT);
		baddr_end = baddr + (FlshParm.block[i]<<F_BLOCK_UNIT_SFT);

		if (baddr >= FLASH_PROTECT_AREA)
		{
			if ((addr <= baddr) && ((addr + len) >= baddr_end))
			{
				rc = cyg_flash_erase(baddr, block_size, &err_addr);
				//diag_printf("rc = %d after cyg_flash_erase\n", rc);
			} else if (((addr+len) > baddr) && (addr < baddr_end))
			{
				int status;
				int writelen =0;
				
				if (pBuffer == NULL) {
					pBuffer = malloc(block_size);
					if (pBuffer == NULL) {
						diag_printf("flsh_erase: can't allocate memory\n");
						rc = -1; 
						break;
					}
				}
				
				status = cyg_flash_read((cyg_flashaddr_t)baddr, pBuffer, block_size, &err_addr);
				if (status != CYG_FLASH_ERR_OK) {
					diag_printf("flsh_erase : backup u-Boot configuration fail\n");
					rc = -1; 					
					break;
				}

			        rc = cyg_flash_erase(baddr, block_size, &err_addr);
			       // diag_printf("rc = %d after cyg_flash_erase\n", rc);

				if (addr > baddr)
				{	
					writelen = addr -baddr;
					status = cyg_flash_program((cyg_flashaddr_t)baddr, pBuffer, writelen, &err_addr);			
					if (status != CYG_FLASH_ERR_OK) {
						diag_printf("1. flsh_erase : restore u-Boot configuration fail\n"); 
						rc = -1; 						
						break;
					}
				}

				if ((addr + len) < baddr_end)
				{
					writelen = baddr_end -(addr + len);
					status = cyg_flash_program((cyg_flashaddr_t)(addr + len), (pBuffer + block_size - writelen), writelen, &err_addr);			
					if (status != CYG_FLASH_ERR_OK) {
						diag_printf("2. flsh_erase : restore u-Boot configuration fail\n"); 
						rc = -1; 						
						break;
					}
				}
			}
		}
	}

	if (pBuffer)
		free(pBuffer);

#ifdef  FLASH_LOG
	flsh_printf(".");
#endif
	return rc ;	
}

int flsh_write(unsigned int addr, unsigned int from, unsigned int len)
{
    int rc=0,i;
    unsigned int blen;
    unsigned int baddr; /* block offset */
    cyg_flashaddr_t err_addr;

    if (!(FlshParm.flags&F_FLAG_SETUP))
    {
        rc = flsh_init();
        if (rc)
            return rc;
    }

    baddr =0;   /* look from 1st block */
    for (i = 0; (i < FlshParm.block_num) &
            (baddr < (addr+len )); baddr+=(FlshParm.block[i]<<F_BLOCK_UNIT_SFT), i++ ) 
    {
		blen = FlshParm.block[i]<<F_BLOCK_UNIT_SFT;
        if (baddr < addr) /* block address < input address */
        {
            if (( baddr + blen ) <= addr ) /* not found */
                continue ;
            /* if the 1st block is not full-fill , find the remainder */
            blen = baddr + blen - addr;
    #ifdef  FLASH_LOG
            //flsh_printf("Not aliged with block bondary.\n");
    #endif
        }

	// Refresh watchdog to prevent from hanging up
	watchdog_tmr_func();

        if (len< blen)
            blen=len;

#ifdef  FLASH_LOG
        //flsh_printf("+");
#endif
        //rc=f_program( FlshParm.base+addr, blen, from );
	if (addr >= FLASH_PROTECT_AREA)
		rc = cyg_flash_program(addr, from, blen, &err_addr);
	else
	{
		rc = 0;
		diag_printf("FLASH: write addr=%x, len=%d\n", addr, blen);
	}
        if (rc)
        {
#ifdef  FLASH_LOG
            flsh_printf("%s(%d) f_program %08x\n",__FILE__,__LINE__,rc);
#endif
            break;
        }
		
		
        addr += blen;
        from += blen;
        len  -= blen;
    }
    return rc;
}

int flsh_read(unsigned int addr, unsigned int to, unsigned int len)
{
	cyg_flashaddr_t err_addr;
	int rc;
	
	rc = cyg_flash_read(addr, to, len, &err_addr);
	if (rc != CYG_FLASH_ERR_OK)
		diag_printf("FLASH: read err=%d eaddr=%x\n", rc, err_addr);
	return rc;
}

int flsh_block_start(int faddr, int *len)
{
	int i;
	int baddr;
	int p;
	
	for (i=0, baddr=0; i<FlshParm.block_num; i++)
	{
		p = baddr;
		baddr += (FlshParm.block[i]<<F_BLOCK_UNIT_SFT);
		
		if (faddr >= p && faddr < baddr)
		{
			*len = (FlshParm.block[i]<<F_BLOCK_UNIT_SFT);
			return p;
		}
	}
	
	return -1;
}


