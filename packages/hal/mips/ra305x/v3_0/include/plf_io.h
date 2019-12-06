#ifndef CYGONCE_PLF_IO_H
#define CYGONCE_PLF_IO_H

//=============================================================================
//
//      plf_io.h
//
//      Ralink RA305x Platform specific IO support
//
//=============================================================================
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
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    
// Contributors: 
// Date:         2009-09-22
// Purpose:      Ralink Ra305x platform IO support
// Description: 
// Usage:        #include <cyg/hal/plf_io.h>
//
//####DESCRIPTIONEND####
//
//=============================================================================
#include <pkgconf/hal.h>
#include <cyg/hal/hal_misc.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/plf_intr.h>

#ifdef __ASSEMBLER__
#define HAL_REG(x)              x
#define HAL_REG8(x)             x
#define HAL_REG16(x)            x
#define HAL_REG32(x)            x
#else
#define HAL_REG(x)              (*(volatile CYG_WORD *)(x))
#define HAL_REG8(x)             (*(volatile CYG_BYTE *)(x))
#define HAL_REG16(x)            (*(volatile CYG_WORD16 *)(x))
#define HAL_REG32(x)            (*(volatile CYG_WORD *)(x))
#endif


//-----------------------------------------------------------------------------

/* Ralink Ra305x Memory Definitions */
#ifdef  CONFIG_MT7628_ASIC
#define HAL_RA305X_RAM_BASE                 0x00000000

#define HAL_RA305X_REGISTER_BASE            0xB0000000
#define HAL_RA305X_SYSCTL_BASE			    0xB0000000
#define HAL_RA305X_TIMER_BASE				0xB0000100
#define HAL_RA305X_INTCL_BASE				0xB0000200
#define HAL_RA305X_MEMCTRL_BASE			    0xB0000300
#define HAL_RA305X_RBUS_MATRIXCTL_BASE	    0xB0000400
#define HAL_RA305X_MIPS_CNT_BASE			0xB0000500
#define HAL_RA305X_PIO_BASE					0xB0000600
#define HAL_RA305X_SPI_SLAVE_BASE			0xB0000700
#define HAL_RA305X_I2C_BASE					0xB0000900
#define HAL_RA305X_I2S_BASE					0xB0000A00
#define HAL_RA305X_SPI_BASE					0xB0000B00
#define HAL_RA305X_UART_LITE1_BASE		    0xB0000C00
#define HAL_RA305X_UARTLITE_BASE			HAL_RA305X_UART_LITE1_BASE
#define HAL_RA305X_UART_LITE2_BASE		    0xB0000D00
#define HAL_RA305X_UART_BASE				HAL_RA305X_UART_LITE2_BASE
#define HAL_RA305X_UART_LITE3_BASE		    0xB0000E00
#define HAL_RA305X_PCM_BASE				    0xB0002000
#define HAL_RA305X_GDMA_BASE				0xB0002800
#define HAL_RA305X_AES_ENGINE_BASE		    0xB0004000
#define HAL_RA305X_PWM_BASE                 0xB0005000
#define HAL_RA305X_FRAME_ENGING_BASE	    0xB0100000
#define HAL_RA305X_PPE_BASE					0xB0100C00
#define HAL_RA305X_ETHSW_BASE				0xB0110000
#define HAL_RA305X_USB_DEV_BASE			    0xB0120000
#define HAL_RA305X_MSDC_BASE				0xB0130000
#define HAL_RA305X_PCI_BASE                 0xB0140000
#define HAL_RA305X_11N_MAC_BASE			    0xB0180000
#define HAL_RA305X_USB_HOST_BASE			0xB01C0000

#define HAL_RA305X_MCNT_CFG				    0xB0000500
#define HAL_RA305X_COMPARE					0xB0000504
#define HAL_RA305X_COUNT					0xB0000508

//Reset Control Register
#define RA305X_SYSCTL_SYS_RST               BIT0
#define RA305X_SYSCTL_SPISLAVE_RST		    BIT3
#define RA305X_SYSCTL_WIFI_RST				BIT4
#define RA305X_SYSCTL_TIMER_RST             BIT8
#define RA305X_SYSCTL_INTC_RST              BIT9
#define RA305X_SYSCTL_MC_RST                BIT10
#define RA305X_SYSCTL_PCM_RST               BIT11
#define RA305X_SYSCTL_UART_RST              BIT19
#define RA305X_SYSCTL_PIO_RST               BIT13
#define RA305X_SYSCTL_DMA_RST               BIT14
#define RA305X_SYSCTL_I2C_RST               BIT16
#define RA305X_SYSCTL_I2S_RST               BIT17
#define RA305X_SYSCTL_SPI_RST               BIT18
#define RA305X_SYSCTL_UARTL_RST             BIT12

#define RA305X_SYSCTL_UARTL3_RST            BIT20

#define RA305X_SYSCTL_FE_RST                BIT21
#define RA305X_SYSCTL_UHST_RST              BIT22
#define RA305X_SYSCTL_SW_RST                BIT23
#define RA305X_SYSCTL_EPHY_RST              BIT24
#define RA305X_SYSCTL_UDEV_RST              BIT25
#define RA305X_SYSCTL_PCIE0_RST             BIT26
#define RA305X_SYSCTL_PCIE1_RST             BIT27
#define RA305X_SYSCTL_MIPS_CNT_RST          BIT28
#define RA305X_SYSCTL_CRYPTO_RST            BIT29

#else

#define HAL_RA305X_RAM_BASE                 0x00000000

#define HAL_RA305X_REGISTER_BASE            0xB0000000
#define HAL_RA305X_SYSCTL_BASE              0xB0000000
#define HAL_RA305X_TIMER_BASE               0xB0000100
#define HAL_RA305X_INTCL_BASE               0xB0000200
#define HAL_RA305X_MEMCTRL_BASE             0xB0000300
#define HAL_RA305X_PCM_BASE                 0xB0000400
#define HAL_RA305X_UART_BASE                0xB0000500
#define HAL_RA305X_PIO_BASE                 0xB0000600
#define HAL_RA305X_GDMA_BASE                0xB0000700
#define HAL_RA305X_NANDCTRL_BASE            0xB0000800
#define HAL_RA305X_I2C_BASE                 0xB0000900
#define HAL_RA305X_I2S_BASE                 0xB0000A00
#define HAL_RA305X_SPI_BASE                 0xB0000B00
#define HAL_RA305X_UARTLITE_BASE            0xB0000C00
#define HAL_RA305X_FRAME_ENGING_BASE        0xB0100000
#define HAL_RA305X_ETHSW_BASE               0xB0110000
#define HAL_RA305X_PCI_BASE                 0xB0140000
#define HAL_RA305X_11N_MAC_BASE             0xB0180000
#define HAL_RA305X_USB_OTG_BASE             0xB01C0000


/*  RA305X_SYSCTL_RESET  */
#define RA305X_SYSCTL_SYS_RST			    BIT0
#define RA305X_SYSCTL_CPU_RST			    BIT1
#define RA305X_SYSCTL_TIMER_RST		        BIT8
#define RA305X_SYSCTL_INTC_RST			    BIT9
#define RA305X_SYSCTL_MC_RST			    BIT10
#define RA305X_SYSCTL_PCM_RST			    BIT11
#define RA305X_SYSCTL_UART_RST		        BIT12
#define RA305X_SYSCTL_PIO_RST			    BIT13
#define RA305X_SYSCTL_DMA_RST			    BIT14

#define RA305X_SYSCTL_I2C_RST			    BIT16
#define RA305X_SYSCTL_I2S_RST			    BIT17
#define RA305X_SYSCTL_SPI_RST			    BIT18
#define RA305X_SYSCTL_UARTL_RST		        BIT19
#define RA305X_SYSCTL_RT2782_RST		    BIT20
#define RA305X_SYSCTL_FE_RST			    BIT21
#define RA305X_SYSCTL_OTG_RST			    BIT22
#define RA305X_SYSCTL_SW_RST			    BIT23
#define RA305X_SYSCTL_EPHY_RST		        BIT24
#define RA305X_SYSCTL_PCIE0_RST		        BIT26
#define RA305X_SYSCTL_PCIE1_RST		        BIT27

#endif


#define HAL_SYSCTL_REG(_reg)         			HAL_REG32(HAL_RA305X_SYSCTL_BASE+(_reg))
#define HAL_TIMER_REG(_reg)          			HAL_REG32(HAL_RA305X_TIMER_BASE+(_reg))
#define HAL_INTCL_REG(_reg)          			HAL_REG32(HAL_RA305X_INTCL_BASE+(_reg))
#define HAL_MEMCTRL_REG(_reg)        		    HAL_REG32(HAL_RA305X_MEMCTRL_BASE+(_reg))
#define HAL_PCM_REG(_reg)            			HAL_REG32(HAL_RA305X_PCM_BASE+(_reg))
#define HAL_UART_REG(_reg)           			HAL_REG32(HAL_RA305X_UART_BASE+(_reg))
#define HAL_PIO_REG(_reg)            			HAL_REG32(HAL_RA305X_PIO_BASE+(_reg))
#define HAL_GDMA_REG(_reg)           			HAL_REG32(HAL_RA305X_GDMA_BASE+(_reg))
#define HAL_NANDCTRL_REG(_reg)      	 	    HAL_REG32(HAL_RA305X_NANDCTRL_BASE+(_reg))
#define HAL_I2C_REG(_reg)            			HAL_REG32(HAL_RA305X_I2C_BASE+(_reg))
#define HAL_I2S_REG(_reg)            			HAL_REG32(HAL_RA305X_I2S_BASE+(_reg))
#define HAL_SPI_REG(_reg)            			HAL_REG32(HAL_RA305X_SPI_BASE+(_reg))
#define HAL_UARTL_REG(_reg)          			HAL_REG32(HAL_RA305X_UARTLITE_BASE+(_reg))
#define HAL_FRAME_REG(_reg)          			HAL_REG32(HAL_RA305X_FRAME_ENGING_BASE+(_reg))
#define HAL_ETHSW_REG(_reg)          			HAL_REG32(HAL_RA305X_ETHSW_BASE+(_reg))
#define HAL_PCI_REG(_reg)            			HAL_REG32(HAL_RA305X_PCI_BASE+(_reg))
#define HAL_11NMAC_REG(_reg)         		    HAL_REG32(HAL_RA305X_11N_MAC_BASE+(_reg))
#define HAL_USBOTG_REG(_reg)         		    HAL_REG32(HAL_RA305X_USB_OTG_BASE+(_reg))


//--------------------------------------------------------------------------
/*  System control registers */
#define RA305X_SYSCTL_CFG					0x10
#define RA305X_SYSCTL_CLK_CFG1				0x30
#define RA305X_SYSCTL_RESET					0x34
#define RA305X_SYSCTL_GPIO_PURPOSE		    0x60
#define RA305X_SYSCTL_PPLL_CFG1			    0x9c
#define RA305X_SYSCTL_PPLL_DRV			    0xa0

/*  RA305X_SYSCTL_CFG  */
#define RA305X_SYSCFG_CLKSEL_384M			0x00040000

/*  RA305X_SYSCTL_CLK_CFG1  */
#define RA305X_SYSCTL_PCIE_CLK_EN			BIT26




/*  RA305X_SYSCTL_GPIO_PURPOSE  */
#define RA305X_GPIO_MDIO_DIS				BIT7


//--------------------------------------------------------------------------
/*  Interrupt controller  */
#ifdef  CONFIG_MT7628_ASIC

#define RA305X_INTC_IRQSTATUS0				0x9C /*IRQ state*/
#define RA305X_INTC_IRQSTATUS1				0xA0
#define RA305X_INTC_SEL0					0x00 /*set bit to 1, add interrupt to IRQ*/
//#define RA305X_INTC_INTTYPE					0x00
#define RA305X_INTC_RAWSTATUS				0xA4
#define RA305X_INTC_ENABLE					0x80 /*SET IRQ MASK Register, in IRQ MASK Register 1 means enable IRQ*/
#define RA305X_INTC_DISABLE					0x78 /*CLEAR IRQ MASK Register, in IRQ MASK Register 0 means disable IRQ*/
#else
#define RA305X_INTC_IRQSTATUS0				0x00
#define RA305X_INTC_IRQSTATUS1				0x04
#define RA305X_INTC_INTTYPE					0x20
#define RA305X_INTC_RAWSTATUS				0x30
#define RA305X_INTC_ENABLE					0x34
#define RA305X_INTC_DISABLE					0x38

#endif

#define RA305X_INTC_ALLINT					0xffffffff
#define RA305X_INTC_GINT_ENABLE			    BIT31


//--------------------------------------------------------------------------
/*  UART controller  */
#ifdef CONFIG_MT7628_ASIC
#define UART_RBR	  		0x00
#define UART_TBR  			0x00
#define UART_IER  			0x04
#define UART_IIR 		 	0x08
#define UART_FCR  			0x08
#define UART_LCR  			0x0C
#define UART_MCR  			0x10
#define UART_LSR  			0x14
#define UART_DLL  			0x00
#define UART_DLM  			0x04
#else
#define UART_RBR			0x00
#define UART_TBR			0x04
#define UART_IER			0x08
#define UART_IIR			0x0C
#define UART_FCR			0x10
#define UART_LCR			0x14
#define UART_MCR			0x18
#define UART_LSR			0x1C
#define UART_DLL			0x28
#define UART_DLM			0x30

#endif

/*  IER register  */
#define UART_IER_ERBFI					BIT0		/*  Rx data ready interrupt  */
#define UART_IER_ETBEI					BIT1		/*  Tx buffer empty interrupt  */
#define UART_IER_ELSI					BIT2		/*  Line status interrupt  */
/*  IIR register  */
#define UART_IIR_FIFOES_MASK			0x00C0		/*  FIFO mode enable Status  */
#define UART_IIR_IID_MASK				0x000E		/*  Interrupt source  */
#define UART_IIR_IP						BIT0		/*  Interrupt pending (0: pending, 1: no more)  */
#define UART_IIR_ID_MASK				0x000F
#define UART_IIR_NONE					0x0001
#define UART_IIR_TX_EMPTY				0x0002
#define UART_IIR_RX_AVIL				0x0004
#define UART_IIR_RX_LINE_STATUS		    0x0006
#define UART_IIR_RX_TIMEOUT			    0x000C
#define UART_IIR_MODEM_STATUS		    0x0000
/*  FCR register  */
#define UART_FCR_RXTRG_MASK			    0x00C0		/*  Rx interrupt trigger level  */
#define UART_FCR_TXTRG_MASK			    0x0030		/*  Tx interrupt trigger level  */
#define UART_FCR_DMAMODE				BIT3		/*  Enable DMA transfer  */
#define UART_FCR_TXRST					BIT2		/*  Tx FIFO reset  */
#define UART_FCR_RXRST					BIT1		/*  Rx FIFO reset  */
#define UART_FCR_FIFOEN					BIT0		/*  Tx/Rx FIFO enable*/
/*  LCR register  */
#define UART_LCR_WLS_MASK				0x0003		/*  Word length select */
#define UART_LCR_WLS_6BIT				BIT0
#define UART_LCR_WLS_7BIT				BIT1
#define UART_LCR_WLS_8BIT				(BIT0|BIT1)
#define UART_LCR_1STB					0
#define UART_LCR_2STB					BIT2		/*  Stop bit  */
#define UART_LCR_PEN					BIT3		/*  Parity enable  */
#define UART_LCR_EPS					BIT4		/*  Even parity  */
#define UART_LCR_STKYP					BIT5		/*  Sticky parity  */
#define UART_LCR_SB						BIT6		/*  Set break  */
#define UART_LCR_DLAB					BIT7		/*  Divisor latch access */
/*  MCR register  */
#define UART_MCR_DTS					BIT0
#define UART_MCR_RTS					BIT1
#define UART_MCR_LOOP_EN				BIT4		/*  Loopback mode enable */
/*  MSR register  */
#define UART_MSR_DCTS					BIT0		/*  Delta Clear To Send  */
#define UART_MSR_DDSR					BIT1		/*  Delta Data Set Reday  */
#define UART_MSR_TERI					BIT2		/*  Trailing Edge Ring Indicator */
#define UART_MSR_DDCD					BIT3		/*  Delta Data Carrier Detect  */
#define UART_MSR_CRS					BIT4		/*  Clear TO Send  */
#define UART_MSR_DSR					BIT5		/*  Data Set Ready  */
#define UART_MSR_RI						BIT6		/*  Ring Indicator  */
#define UART_MSR_DSD					BIT7		/*  Data Carrier Detect  */
/*  LSR register  */
#define UART_LSR_DR						BIT0		/*  Rx Data Ready  */
#define UART_LSR_OE						BIT1		/*  Overrun error  */
#define UART_LSR_PE						BIT2		/*  Parity error  */
#define UART_LSR_FE						BIT3		/*  Frame error  */
#define UART_LSR_BI						BIT4		/*  Break interrupt  */
#define UART_LSR_THRE					BIT5		/*  Tx data request  */
#define UART_LSR_TEMT					BIT6		/*  Tx Empty  */
#define UART_LSR_FIFO_ERR				BIT7		/*  FIFO ERROR  */

#ifdef  CYGPKG_IO_PCI

// Translate the PCI interrupt requested by the device (INTA#, INTB#,
// INTC# or INTD#) to the associated CPU interrupt (i.e., HAL vector).
#define HAL_PCI_TRANSLATE_INTERRUPT( __bus, __devfn, __vec, __valid)          \
    CYG_MACRO_START                                                           \
    cyg_uint8 __req;                                                          \
    HAL_PCI_CFG_READ_UINT8(__bus, __devfn, CYG_PCI_CFG_INT_PIN, __req);       \
    if (0 != __req) {                                                         \
        __vec = 0x2;                                                          \
        __valid = true;                                                       \
    } else {                                                                  \
        /* Device will not generate interrupt requests. */                    \
        __valid = false;                                                      \
    }                                                                         \
    CYG_MACRO_END




// Initialize the PCI bus.
#ifndef __ASSEMBLER__
externC void cyg_hal_plf_pci_init(void);
extern cyg_uint32 cyg_hal_plf_pci_cfg_read_dword (cyg_uint32 bus,
						  cyg_uint32 devfn,
						  cyg_uint32 offset);
extern cyg_uint16 cyg_hal_plf_pci_cfg_read_word  (cyg_uint32 bus,
						  cyg_uint32 devfn,
						  cyg_uint32 offset);
extern cyg_uint8 cyg_hal_plf_pci_cfg_read_byte   (cyg_uint32 bus,
						  cyg_uint32 devfn,
						  cyg_uint32 offset);
extern void cyg_hal_plf_pci_cfg_write_dword (cyg_uint32 bus,
					     cyg_uint32 devfn,
					     cyg_uint32 offset,
					     cyg_uint32 val);
extern void cyg_hal_plf_pci_cfg_write_word  (cyg_uint32 bus,
					     cyg_uint32 devfn,
					     cyg_uint32 offset,
					     cyg_uint16 val);
extern void cyg_hal_plf_pci_cfg_write_byte   (cyg_uint32 bus,
					      cyg_uint32 devfn,
					      cyg_uint32 offset,
					      cyg_uint8 val);

#define HAL_PCI_INIT() cyg_hal_plf_pci_init()
#endif
#define HAL_PCI_ALLOC_BASE_MEMORY 0x20000000
#define HAL_PCI_ALLOC_BASE_IO     0x00000000

// This is where the PCI spaces are mapped in the CPU's address space.
//
#define HAL_PCI_PHYSICAL_MEMORY_BASE CYGARC_UNCACHED_ADDRESS(0)
#define HAL_PCI_PHYSICAL_IO_BASE     0x00000000

#define RAPCI_PCICFG_REG              0x0
#define RAPCI_CFG_ADDR                0x20
#define RAPCI_CFG_DATA                0x24
#define RAPCI_PHY0_CFG                0x90

#define RAPCIE_RC_CTL_BASE            0x2000
#define RAPCIE_ID_REG                 (RAPCIE_RC_CTL_BASE + 0x30)
#define RAPCIE_CLASS_REG              (RAPCIE_RC_CTL_BASE + 0x34)
#define RAPCIE_SUBID_REG              (RAPCIE_RC_CTL_BASE + 0x38)


/* RAPCI_PHY0_CFG */
#define SPI_BUSY                      (1 << 31)
#define SPI_DONE                      (1 << 30)

// Read a value from the PCI configuration space of the appropriate
// size at an address composed from the bus, devfn and offset.
#define HAL_PCI_CFG_READ_UINT8( __bus, __devfn, __offset, __val )  \
    __val = cyg_hal_plf_pci_cfg_read_byte((__bus),  (__devfn), (__offset))

#define HAL_PCI_CFG_READ_UINT16( __bus, __devfn, __offset, __val ) \
    __val = cyg_hal_plf_pci_cfg_read_word((__bus),  (__devfn), (__offset))

#define HAL_PCI_CFG_READ_UINT32( __bus, __devfn, __offset, __val ) \
    __val = cyg_hal_plf_pci_cfg_read_dword((__bus),  (__devfn), (__offset))

// Write a value to the PCI configuration space of the appropriate
// size at an address composed from the bus, devfn and offset.
#define HAL_PCI_CFG_WRITE_UINT8( __bus, __devfn, __offset, __val )  \
    cyg_hal_plf_pci_cfg_write_byte((__bus),  (__devfn), (__offset), (__val))

#define HAL_PCI_CFG_WRITE_UINT16( __bus, __devfn, __offset, __val ) \
    cyg_hal_plf_pci_cfg_write_word((__bus),  (__devfn), (__offset), (__val))

#define HAL_PCI_CFG_WRITE_UINT32( __bus, __devfn, __offset, __val ) \
    cyg_hal_plf_pci_cfg_write_dword((__bus),  (__devfn), (__offset), (__val))

#endif //  CYGPKG_IO_PCI


#ifndef __ASSEMBLER__
externC cyg_uint32 hal_get_cpu_freq(void);
#define HAL_GET_CPU_FREQ()			hal_get_cpu_freq()
externC cyg_uint32 hal_get_sysbus_freq(void);
#define HAL_GET_SYSBUS_FREQ()			hal_get_sysbus_freq()
#endif


//-----------------------------------------------------------------------------
// end of plf_io.h
#endif // CYGONCE_PLF_IO_H

