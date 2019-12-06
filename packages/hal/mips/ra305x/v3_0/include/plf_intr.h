#ifndef CYGONCE_HAL_PLF_INTR_H
#define CYGONCE_HAL_PLF_INTR_H

//==========================================================================
//
//      plf_intr.h
//
//      Ralink Ra305x Interrupt and clock support
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
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
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    nickg
// Contributors: nickg, jskov,
//               gthomas, jlarmour, dmoseley
// Date:         2001-03-20
// Purpose:      Define Interrupt support
// Description:  The macros defined here provide the HAL APIs for handling
//               interrupts and the clock for the Ralink Ra305x board.
//              
// Usage:
//              #include <cyg/hal/plf_intr.h>
//              ...
//              
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/hal.h>

// First an assembly safe part

//--------------------------------------------------------------------------
// Interrupt vectors.

#ifndef CYGHWR_HAL_INTERRUPT_VECTORS_DEFINED

// These are decoded via the IP bits of the cause
// register when an external interrupt is delivered.
#ifdef  CONFIG_MT7628_ASIC
#define CYGNUM_HAL_INTERRUPT_IP0                 0		///is this line correct???
#define CYGNUM_HAL_INTERRUPT_IP1                 1
#define CYGNUM_HAL_INTERRUPT_IP2                 2
#define CYGNUM_HAL_INTERRUPT_ETH                 3
#define CYGNUM_HAL_INTERRUPT_IP5                 4
#define CYGNUM_HAL_INTERRUPT_COMPARE             5

#define CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE       6
#define CYGNUM_HAL_INTERRUPT_SYSCTL              (0+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_TIMER0              (24+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_WDTIMER             (23+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_TIMER1              (25+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)

//#define CYGNUM_HAL_INTERRUPT_ILLACC              (3+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_PCM                 (4+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_UART                (21+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_PIO              (6+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_DMA              (7+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
//#define CYGNUM_HAL_INTERRUPT_NAND             (8+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_PC              (9+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_I2S                 (10+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
//#define CYGNUM_HAL_INTERRUPT_XXXX                (11+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_UARTL                (20+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_UARTL3                (22+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)

#define CYGNUM_HAL_INTERRUPT_ESW                (17+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_OTG                (18+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#else 

#define CYGNUM_HAL_INTERRUPT_IP0                 0
#define CYGNUM_HAL_INTERRUPT_IP1                 1
#define CYGNUM_HAL_INTERRUPT_IP2                 2
#define CYGNUM_HAL_INTERRUPT_ETH                 3
#define CYGNUM_HAL_INTERRUPT_IP5                 4
#define CYGNUM_HAL_INTERRUPT_COMPARE             5

#define CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE       6
#define CYGNUM_HAL_INTERRUPT_SYSCTL              (0+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_TIMER0              (1+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_WDTIMER             (2+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_ILLACC              (3+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_PCM                 (4+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_UART                (5+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_PIO              (6+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_DMA              (7+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_NAND             (8+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_PC              (9+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_I2S                 (10+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_XXXX                (11+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_UARTL                (12+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_ESW                (17+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)
#define CYGNUM_HAL_INTERRUPT_OTG                (18+CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)

#endif

#define CYGNUM_HAL_INTERRUPT_BOGUS		 (CYGNUM_HAL_INTERRUPT_OTG+1)

// Min/Max ISR numbers
#define CYGNUM_HAL_ISR_MIN                 0
#define CYGNUM_HAL_ISR_MAX                 (32+6) //CYGNUM_HAL_INTERRUPT_BOGUS


#define CYGNUM_HAL_ISR_COUNT               (CYGNUM_HAL_ISR_MAX - CYGNUM_HAL_ISR_MIN + 1)

// The vector used by the Real time clock
#define CYGNUM_HAL_INTERRUPT_RTC           CYGNUM_HAL_INTERRUPT_COMPARE

#define CYGHWR_HAL_INTERRUPT_VECTORS_DEFINED

#endif	/*  CYGHWR_HAL_INTERRUPT_VECTORS_DEFINED  */


//--------------------------------------------------------------------------
#ifndef __ASSEMBLER__

#include <cyg/infra/cyg_type.h>
#include <cyg/hal/plf_io.h>

//--------------------------------------------------------------------------
// Interrupt controller access.

#ifndef CYGHWR_HAL_INTERRUPT_CONTROLLER_ACCESS_DEFINED

// Array which stores the configured priority levels for the configured
// interrupts.
externC volatile CYG_BYTE hal_interrupt_level[CYGNUM_HAL_ISR_COUNT];

#define HAL_INTERRUPT_MASK( _vector_ )                       \
    CYG_MACRO_START                                                         \
    if( (_vector_) <= CYGNUM_HAL_INTERRUPT_COMPARE )                        \
    {                                                                       \
        asm volatile (                                                      \
            "mfc0   $3,$12\n"                                               \
            "la     $2,0x00000400\n"                                        \
            "sllv   $2,$2,%0\n"                                             \
            "nor    $2,$2,$0\n"                                             \
            "and    $3,$3,$2\n"                                             \
            "mtc0   $3,$12\n"                                               \
            "nop; nop; nop\n"                                               \
            :                                                               \
            : "r"(_vector_)                                                 \
            : "$2", "$3"                                                    \
            );                                                              \
    }                                                                       \
    else                                                                    \
    {                                                                       \
	HAL_INTCL_REG(RA305X_INTC_DISABLE) = (1 << ((_vector_)-CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)); \
    }									\
    CYG_MACRO_END

#define HAL_INTERRUPT_UNMASK( _vector_ )                     \
    CYG_MACRO_START                                                         \
    if( (_vector_) <= CYGNUM_HAL_INTERRUPT_COMPARE )                        \
    {                                                                       \
        asm volatile (                                                      \
            "mfc0   $3,$12\n"                                               \
            "la     $2,0x00000400\n"                                        \
            "sllv   $2,$2,%0\n"                                             \
            "or     $3,$3,$2\n"                                             \
            "mtc0   $3,$12\n"                                               \
            "nop; nop; nop\n"                                               \
            :                                                               \
            : "r"(_vector_)                                                 \
            : "$2", "$3"                                                    \
            );                                                              \
    }										\
    else /* CTRL1 */                                                        \
    {                                                                       \
	HAL_INTCL_REG(RA305X_INTC_ENABLE) = (1 << ((_vector_)-CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE)); \
    }                                                                       \
    CYG_MACRO_END
                                                       
#define HAL_INTERRUPT_ACKNOWLEDGE( _vector_ )                   \
    CYG_MACRO_START                                             \
    cyg_uint32 _srvector_ = _vector_;                           \
    CYG_MACRO_END

#define HAL_INTERRUPT_CONFIGURE( _vector_, _level_, _up_ )                  \
    CYG_MACRO_START                                                         \
    CYG_MACRO_END

#define HAL_INTERRUPT_SET_LEVEL( _vector_, _level_ )

#define CYGHWR_HAL_INTERRUPT_CONTROLLER_ACCESS_DEFINED

#endif


//--------------------------------------------------------------------------
// Control-C support.

#if defined(CYGDBG_HAL_MIPS_DEBUG_GDB_CTRLC_SUPPORT)

# define CYGHWR_HAL_GDB_PORT_VECTOR CYGNUM_HAL_INTERRUPT_SER

externC cyg_uint32 hal_ctrlc_isr(CYG_ADDRWORD vector, CYG_ADDRWORD data);

# define HAL_CTRLC_ISR hal_ctrlc_isr

#endif


//----------------------------------------------------------------------------
// Reset.
#ifndef CYGHWR_HAL_RESET_DEFINED
externC void hal_ra305x_reset( void );
#define CYGHWR_HAL_RESET_DEFINED
#define HAL_PLATFORM_RESET()             hal_ra305x_reset()

#define HAL_PLATFORM_RESET_ENTRY CYGHWR_HAL_MIPS_RA305X_FLASH_BASE_ADDR

#endif // CYGHWR_HAL_RESET_DEFINED

#endif // __ASSEMBLER__

//--------------------------------------------------------------------------
#endif // ifndef CYGONCE_HAL_PLF_INTR_H
// End of plf_intr.h

