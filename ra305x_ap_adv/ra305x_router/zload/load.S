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

    Abstract:
	This file handle the system exception. An exception is caused by an

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/
#define ZLOAD_S
 
 /************************************************************ ****************
 *                              INCLUDE FILES
 ***************************************************************************/

#include <cyg/hal/arch.inc>	
#include <autoconf.h>

#ifdef	CONFIG_DL_COOKIE
#include <dl_cookie.h>
#endif

/****************************************************************************
 *                                 MACROS
 ***************************************************************************/

#define APP_RUN_LOC		0x80000400
#define	UART_TX			0xb0000C04

/***************************************************************************
 *                          Code Begin
 **************************************************************************/

.text
/*---------------------------------------------------------------------------
 * FUNCTION
 *
 *  start
 *---------------------------------------------------------------------------
 * DESCRIPTION
 *
 *---------------------------------------------------------------------------
 * PARAMETERS
 *  
 *  a0 : the temp buffer
 *---------------------------------------------------------------------------
 * RETURN
 *
 *  NONE
 *-------------------------------------------------------------------------*/
		.ent    start
start:
#if	!defined(CONFIG_ZLOAD_BUF)
		j		next			# older version
#else
	#if (CONFIG_ZLOAD_BUF==0x80300000)
		j		next			# older version , 0x080c0010
	#else
		beq		zero,zero,next	# 0x100000f
	#endif
#endif
		.word	APP_RUN_LOC		# default run location

        .org    0x40            # reserve 64 Byte for image header
next:
#ifdef	CONFIG_DL_COOKIE
	beq	zero, zero, 1f
	.asciz	DL_COOKIE
	.balign 4
1:
#endif

		la		a0, start 		 # stack bottom
        sw      sp, -4(a0)       # save sp
        sw      ra, -8(a0)       # save ra
        sw      gp, -12(a0)      # save gp

        subu    sp, a0, 16

        /*------------------------------------------------------------
         * (1) Clear unintialized data segment bss (_fbss-->end)
         *----------------------------------------------------------*/
        la      v0, _fbss               // clear bss, but is it always the start address of BSS?
        la      v1, __bss_end           // end of bss
1:      
        sw      zero, 0(v0)             // clear bss (_fbss-->end)
        sw      zero, 4(v0)
        sw      zero, 8(v0)
        sw      zero, 12(v0)
        addu    v0, 16
        bltu    v0, v1, 1b

		la		gp, _gp
		la		a0, __heap1     # heap 
1:
		jal		load_image
        addu    sp, sp, 16
        lw      gp, -12(sp)
        lw      ra, -8(sp)
        lw      sp, -4(sp)
        jr      ra
        .end    start

#define DCACHE_SZ		0x4000
#define ICACHE_SZ		0x8000
#define DCACHE_LINE_SZ		32
#define ICACHE_LINE_SZ		32
/*---------------------------------------------------------------------------
 * FUNCTION
 *		cache_sync_all
 *-------------------------------------------------------------------------*/
		.ent	cache_sync_all	
		.globl 	cache_sync_all
cache_sync_all:
		la	a0, 0x80000000

		/*  Writeback and invalidate all dcache  */
		la	v0, DCACHE_SZ
		addu	v0, v0, a0
1:
		cache 	0x01, 0(a0)
		addiu	a0, a0, DCACHE_LINE_SZ
		bne	v0, a0, 1b

		/*  Invalidate entire icache  */
		la	a0, 0x80000000
		la	v0, ICACHE_SZ
		addu	v0, v0, a0
2:
		cache 	0x0, 0(a0)
		addiu	a0, a0, ICACHE_LINE_SZ
		bne	v0, a0, 2b

		jr		ra
		
		.end	cache_sync_all

/*---------------------------------------------------------------------------
 * FUNCTION
 *		putchar
 *-------------------------------------------------------------------------*/
 		.global	putchar
		.ent	putchar
		.set push
		.set noreorder
putchar:
		la		v0, UART_TX
		sw		a0, 0(v0)
		li		v0, 30000
1:
		bne		v0, zero, 1b
		addi		v0, v0, -1

		jr		ra
		nop
		.set pop
		.end	putchar

