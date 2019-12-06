#ifndef CYGONCE_HAL_PLATFORM_INC
#define CYGONCE_HAL_PLATFORM_INC
##=============================================================================
##
##	platform.inc
##
##	Ralink Ra305x board assembler header file
##
##=============================================================================
## ####ECOSGPLCOPYRIGHTBEGIN####                                            
## -------------------------------------------                              
## This file is part of eCos, the Embedded Configurable Operating System.   
## Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
##
## eCos is free software; you can redistribute it and/or modify it under    
## the terms of the GNU General Public License as published by the Free     
## Software Foundation; either version 2 or (at your option) any later      
## version.                                                                 
##
## eCos is distributed in the hope that it will be useful, but WITHOUT      
## ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    
## FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License    
## for more details.                                                        
##
## You should have received a copy of the GNU General Public License        
## along with eCos; if not, write to the Free Software Foundation, Inc.,    
## 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.            
##
## As a special exception, if other files instantiate templates or use      
## macros or inline functions from this file, or you compile this file      
## and link it with other works to produce a work based on this file,       
## this file does not by itself cause the resulting work to be covered by   
## the GNU General Public License. However the source code for this file    
## must still be made available in accordance with section (3) of the GNU   
## General Public License v2.                                               
##
## This exception does not invalidate any other reasons why a work based    
## on this file might be covered by the GNU General Public License.         
## -------------------------------------------                              
## ####ECOSGPLCOPYRIGHTEND####                                              
##=============================================================================
#######DESCRIPTIONBEGIN####
##
## Author(s): 	dmoseley
## Contributors:dmoseley, jskov
## Date:	2001-03-20
## Purpose:	Malta board definitions.
## Description:	This file contains various definitions and macros that are
##              useful for writing assembly code for the Malta board.
## Usage:
##		#include <cyg/hal/platform.inc>
##		...
##		
##
######DESCRIPTIONEND####
##
##=============================================================================

#include <cyg/hal/mips.inc>

#include <cyg/hal/hal_arch.h>
#include <cyg/hal/plf_io.h>

#------------------------------------------------------------------------------
# Additional bits for status register.
# We set the IM[0] bit to accept all interrupts.

#define INITIAL_SR_PLF 0x00000400

#------------------------------------------------------------------------------

#define CYGPKG_HAL_RESET_VECTOR_FIRST_CODE
	.macro hal_reset_vector_first_code
	.endm

#define CYGPKG_HAL_EARLY_INIT
	.macro	hal_early_init
	.endm

#------------------------------------------------------------------------------
# Interrupt decode macros

#ifndef CYGPKG_HAL_MIPS_INTC_DEFINED
# Only INTN[0] is connected on the Malta board. We need to access the
# interrupt controller to get the actual vector number.

#ifndef CYGPKG_HAL_MIPS_INTC_INIT_DEFINED

	.macro	hal_intc_init                           
	mfc0	v0,status
	nop
	lui	v1,0xFFFF
	ori	v1,v1,0x04FF
	and	v0,v0,v1	# Clear the IntMask bits except IM[0]
	mtc0	v0,status
	nop
	nop
	nop                        
	.endm

#define CYGPKG_HAL_MIPS_INTC_INIT_DEFINED
#endif

#ifndef CYGPKG_HAL_MIPS_INTC_DECODE_DEFINED
	.macro	hal_intc_decode vnum
	mfc0	a1, status		# get status register (interrupt mask)
	mfc0	a0, cause		# get cause register
	andi	a1, a1, 0xfc00		# isolate 6 interrupt bits
	and	a1, a1, a0		# apply interrupt mask
	beq	a1, zero, 1f
	li	v0, CYGNUM_HAL_INTERRUPT_BOGUS
	
	clz	a0, a1
	li	a1, 21
	subu	a0, a1, a0
	jal	hal_ra305x_irq_decode
	nop
	
1:
	move	\vnum, v0		# pick up vector number
	.endm

#define CYGPKG_HAL_MIPS_INTC_DECODE_DEFINED
#endif

#ifndef CYGPKG_HAL_MIPS_INTC_TRANSLATE_DEFINED
	.macro	hal_intc_translate inum,vnum
	move	\vnum,\inum			# Vector == interrupt number
	.endm

#define CYGPKG_HAL_MIPS_INTC_TRANSLATE_DEFINED
#endif

	.macro	hal_intc_decode_data
	.endm

	
#define CYGPKG_HAL_MIPS_INTC_DEFINED
#endif

// We also define our own interrupt tables in platform.S...
#define CYG_HAL_MIPS_ISR_TABLES_DEFINED


#------------------------------------------------------------------------------
# Diagnostic macros

#ifndef CYGPKG_HAL_MIPS_DIAG_DEFINED

	.macro	hal_diag_init
	.endm

	.macro	hal_diag_excpt_start
	.endm

	.macro	hal_diag_intr_start
	.endm

	.macro	hal_diag_restore
	.endm

#define CYGPKG_HAL_MIPS_DIAG_DEFINED

#endif // ifndef CYGPKG_HAL_MIPS_DIAG_DEFINED
	
#------------------------------------------------------------------------------
# MEMC macros.
# 
	
#if defined(CYG_HAL_STARTUP_ROM)

#------------------------------------------------------------------------------
# Make sure the jump to _start in vectors.S is done uncached
#

#define CYGARC_START_FUNC_UNCACHED

	.macro	hal_memc_init
	.endm

#define CYGPKG_HAL_MIPS_MEMC_DEFINED

#endif	


#------------------------------------------------------------------------------
# Macro for copying vectors to RAM if necessary.
#if !defined(CYGSEM_HAL_USE_ROM_MONITOR)

	.macro	hal_vectors_init
        # If we don~t play nice with a ROM monitor, copy the required
        # vectors into the proper location.
	.set push
	.set noreorder
	
	# Install utlb vector
	la	t0,CYGARC_KSEG_CACHED_BASE		# dest addr
	la	t1,utlb_vector				# source addr
	la	t3,utlb_vector_end			# end dest addr
1:	
	lw	v0,0(t1)				# get word
	addi	t1,t1,4	
	sw	v0,0(t0)				# write word
	bne	t1,t3,1b
	addi	t0,t0,4					# delay slot

	# Install general vector
	la	t0,CYGARC_KSEG_CACHED_BASE+0x180	# dest addr
	la	t1,other_vector				# source addr
	la	t3,other_vector_end			# end dest addr
1:	
	lw	v0,0(t1)				# get word
	addi	t1,t1,4
	sw	v0,0(t0)				# write word
	bne	t1,t3,1b
	addi	t0,t0,4					# delay slot

	# Now clear the region in the caches
	la	t0,CYGARC_KSEG_CACHED_BASE		# dest addr
	ori	t1,t0,0x200				# source addr
1:	cache	0x15,0(t0)	        		# Flush word from data cache
	cache	0x10,0(t0)				# Invalidate icache for word
	addiu	t0,t0,0x20
	bne	t0,t1,1b
	nop

	 .set pop
	.endm

#else

	.macro	hal_vectors_init
	.endm

#endif

#------------------------------------------------------------------------------
# Monitor initialization.
	
#ifndef CYGPKG_HAL_MIPS_MON_DEFINED

#if	defined(CYG_HAL_STARTUP_ROM) ||			\
	(	defined(CYG_HAL_STARTUP_RAM) &&		\
		!defined(CYGSEM_HAL_USE_ROM_MONITOR))
	# If we are starting up from ROM, or we are starting in
	# RAM and NOT using a ROM monitor, initialize the VSR table.

	.macro	hal_mon_init
	hal_vectors_init
        # Set default exception VSR for all vectors
        ori     a0,zero,16  # CYGNUM_HAL_VSR_COUNT
	la	a1,__default_exception_vsr
	la	a2,hal_vsr_table
1:      sw      a1,0(a2)
        addi    a2,a2,4
        addi    a0,a0,-1
        bne     a0,zero,1b
         nop

        # Now set special VSRs
       	la	a0,hal_vsr_table
        # Set interrupt VSR
	la	a1,__default_interrupt_vsr
        sw      a1,0*4(a0)  # CYGNUM_HAL_VECTOR_INTERRUPT
        # Add special handler on breakpoint vector to allow GDB and
        # GCC to both use 'break' without conflicts.
	la	a1,__break_vsr_springboard
	sw	a1,9*4(a0)  # CYGNUM_HAL_VECTOR_BREAKPOINT
        # Set exception handler on special vectors
        # FIXME: Should use proper definitions
	la	a1,__default_exception_vsr
	sw	a1,32*4(a0)             # debug
	sw	a1,33*4(a0)             # utlb
	sw	a1,34*4(a0)             # nmi
	.endm
	
#elif defined(CYG_HAL_STARTUP_RAM) && defined(CYGSEM_HAL_USE_ROM_MONITOR)

	# Initialize the VSR table entries
	# We only take control of the interrupt vector,
	# the rest are left to the ROM for now...

	.macro	hal_mon_init
	hal_vectors_init
	la	a0,__default_interrupt_vsr
	la	a3,hal_vsr_table
	sw	a0,0(a3)
	.endm

#else

	.macro	hal_mon_init
	hal_vectors_init
	.endm

#endif
			

#define CYGPKG_HAL_MIPS_MON_DEFINED

#endif	


#------------------------------------------------------------------------------
#endif // ifndef CYGONCE_HAL_PLATFORM_INC
# end of platform.inc

