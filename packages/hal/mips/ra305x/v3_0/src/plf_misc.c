//==========================================================================
//
//      plf_misc.c
//
//      HAL platform miscellaneous functions
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
// Contributors: nickg, jlarmour, dmoseley, jskov
// Date:         2001-03-20
// Purpose:      HAL miscellaneous functions
// Description:  This file contains miscellaneous functions provided by the
//               HAL.
//
//####DESCRIPTIONEND####
//
//========================================================================*/

#define CYGARC_HAL_COMMON_EXPORT_CPU_MACROS
#include <pkgconf/hal.h>

#include <cyg/infra/cyg_type.h>         // Base types
#include <cyg/infra/cyg_trac.h>         // tracing macros
#include <cyg/infra/cyg_ass.h>          // assertion macros

#include <cyg/hal/hal_arch.h>           // architectural definitions

#include <cyg/hal/hal_intr.h>           // Interrupt handling

#include <cyg/hal/hal_cache.h>          // Cache handling

#include <cyg/hal/hal_if.h>


/*------------------------------------------------------------------------*/
/* PCI support                                                            */
#if defined(CYGPKG_IO_PCI)

//--------------------------------------------------------------------------
#include <cyg/io/pci_hw.h>
#include <cyg/io/pci.h>

#define WAITRETRY_MAX		10
#define WRITE_MODE              (1UL<<23)
#define DATA_SHIFT              0
#define ADDR_SHIFT              8

int wait_pciephy_busy(void)
{
	cyg_uint32 reg_value = 0x0, retry = 0;
	while(1){
		reg_value = HAL_PCI_REG(RAPCI_PHY0_CFG);

		if(reg_value & SPI_BUSY)
        		CYGACC_CALL_IF_DELAY_US(100 * 1000); // mdelay(100)
		else
			break;
		if(retry++ > WAITRETRY_MAX){
			diag_printf("PCIE-PHY retry failed.\n");
			return -1;
		}
	}
	return 0;
}

unsigned long pcie_phy(char rwmode, unsigned long addr, unsigned long val)
{
	unsigned long reg_value = 0x0;

	wait_pciephy_busy();
	if(rwmode == 'w'){
		reg_value |= WRITE_MODE;
		reg_value |= (val) << DATA_SHIFT;
	}
	reg_value |= (addr) << ADDR_SHIFT;

	// apply the action
	HAL_PCI_REG(RAPCI_PHY0_CFG) = reg_value;

        CYGACC_CALL_IF_DELAY_US(1 * 1000); // mdelay(1)
	wait_pciephy_busy();

	if(rwmode == 'r'){
		reg_value = HAL_PCI_REG(RAPCI_PHY0_CFG);
		return reg_value;
	}
	return 0;
}


void Pcie_BypassDLL(void)
{
	pcie_phy('w', 0x0, 0x80);
	pcie_phy('w', 0x1, 0x04);
}

void prom_pcieinit(void)
{
	cyg_uint32 reg;

	Pcie_BypassDLL();

	diag_printf(" PCIE: Elastic buffer control: Addr:0x68 -> 0xB4\n");
	pcie_phy('w', 0x68, 0xB4);

        HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) |= (RA305X_SYSCTL_PCIE0_RST);
        HAL_SYSCTL_REG(RA305X_SYSCTL_CLK_CFG1) &= ~(RA305X_SYSCTL_PCIE_CLK_EN);
        reg = HAL_SYSCTL_REG(RA305X_SYSCTL_PPLL_DRV);
        reg &= ~(0x1 << 19);
        reg |= (0x1 << 31);
        HAL_SYSCTL_REG(RA305X_SYSCTL_PPLL_DRV) = reg;
	diag_printf(" disable all power about PCIe\n");

}

#define MV_WRITE(ofs, data)  \
        *(volatile unsigned int *)(HAL_RA305X_PCI_BASE+(ofs)) = data
#define MV_READ(ofs, data)   \
        *(data) = (*(volatile unsigned int *)(HAL_RA305X_PCI_BASE+(ofs)))

void PCIE_read_config(unsigned long bus, unsigned long dev, unsigned long func, unsigned long reg, unsigned long *val)
{
	unsigned int address_reg, data_reg, address;

 	address_reg = 0x20;
    data_reg = 0x24;

    /* set addr */
    address = (((reg & 0xF00)>>8)<<24) | (bus << 16) | (dev << 11) | (func << 8) | (reg & 0xfc) | 0x80000000 ;

    /* start the configuration cycle */
    MV_WRITE(address_reg, address);
    /* read the data */
    MV_READ(data_reg, val);
	
	return;
}

void PCIE_write_config(unsigned long bus, unsigned long dev, unsigned long func, unsigned long reg, unsigned long val)
{
	unsigned int address_reg, data_reg, address;

 	address_reg = 0x20;
    data_reg = 0x24;

    /* set addr */
    address = (((reg & 0xF00)>>8)<<24) | (bus << 16) | (dev << 11) | (func << 8) | (reg & 0xfc) | 0x80000000 ;

    /* start the configuration cycle */
    MV_WRITE(address_reg, address);
    /* read the data */
    MV_WRITE(data_reg, val);
	
	return;
}

void cyg_hal_plf_pci_init(void)
{
    cyg_uint8  next_bus;
        cyg_uint32 reg;

#ifndef CONFIG_MT7628_ASIC
	prom_pcieinit();
        CYGACC_CALL_IF_DELAY_US(100 * 1000); // mdelay(100)
        reg = HAL_SYSCTL_REG(RA305X_SYSCTL_GPIO_PURPOSE);
        HAL_SYSCTL_REG(RA305X_SYSCTL_GPIO_PURPOSE) = (reg & ~(0x3 << 16)); // PERST_N
        HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) &= ~(RA305X_SYSCTL_PCIE0_RST);
        HAL_SYSCTL_REG(RA305X_SYSCTL_CLK_CFG1) |= RA305X_SYSCTL_PCIE_CLK_EN;

        CYGACC_CALL_IF_DELAY_US(100 * 1000); // mdelay(100)

        if (HAL_SYSCTL_REG(RA305X_SYSCTL_PPLL_CFG1) & (1<<23))
        {
        }
        else
        {
                HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) |= (RA305X_SYSCTL_PCIE0_RST);
                HAL_SYSCTL_REG(RA305X_SYSCTL_CLK_CFG1) &= (RA305X_SYSCTL_PCIE_CLK_EN);
                return;
        }

        reg = HAL_SYSCTL_REG(RA305X_SYSCTL_PPLL_DRV);
        reg |= (0x1 << 19);
        reg &= ~(0x1<<18);
        reg &= ~(0x1<<17);
        reg |= (0x1 << 31);
        HAL_SYSCTL_REG(RA305X_SYSCTL_PPLL_DRV) = reg;

        CYGACC_CALL_IF_DELAY_US(100 * 1000); // mdelay(100)
        diag_printf("Reset the PCI device card\n");
        HAL_PCI_REG(RAPCI_PCICFG_REG) &= ~(1<<1); // de-assert PERST

        diag_printf("waiting\n");
        CYGACC_CALL_IF_DELAY_US(500 * 1000); // mdelay(500)

	    if ((HAL_PCI_REG(0x2050) & 0x1) == 0)
	    {
          HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) |= (RA305X_SYSCTL_PCIE0_RST);
          HAL_SYSCTL_REG(RA305X_SYSCTL_CLK_CFG1) &= ~(RA305X_SYSCTL_PCIE_CLK_EN);
		  reg &= ~(0x1 << 19);
		  reg |= (0x1 << 31);
		  HAL_SYSCTL_REG(RA305X_SYSCTL_PPLL_DRV) = reg;
		  diag_printf("PCIE0 no card, disable it(RST&CLK)\n");
		  // if there are multiple card, should not return here
		  return;
	    }

#else
		unsigned long val = 0;
        diag_printf("MT7628 PCIE driver init!\n");
        reg = HAL_SYSCTL_REG(RA305X_SYSCTL_GPIO_PURPOSE);
        //HAL_SYSCTL_REG(RA305X_SYSCTL_GPIO_PURPOSE) = (reg & ~(0x3 << 16)); // PERST_N
        HAL_SYSCTL_REG(RA305X_SYSCTL_GPIO_PURPOSE) = (reg & ~(1 << 16)); // Jody,PCIe RESET GPIO mode
        HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) &= ~(RA305X_SYSCTL_PCIE0_RST);
        HAL_SYSCTL_REG(RA305X_SYSCTL_CLK_CFG1) |= RA305X_SYSCTL_PCIE_CLK_EN;
        CYGACC_CALL_IF_DELAY_US(100 * 1000); // mdelay(100)

        PCIE_read_config(0, 0, 0, 0x70c, &val);
	    val &= ~(0xff)<<8;
	    val |= 0x50<<8;
	    PCIE_write_config(0, 0, 0, 0x70c, val);
	    PCIE_read_config(0, 0, 0, 0x70c, &val);
	    diag_printf("Port 0 N_FTS = %x\n", (unsigned int)val);

        diag_printf("start PCIe register access\n");
        HAL_PCI_REG(RAPCI_PCICFG_REG) &= ~(1<<1); //de-assert PERST

   	    diag_printf("\n*************** MT7628 PCIe RC mode *************\n");
	    CYGACC_CALL_IF_DELAY_US(500 * 1000); // mdelay(500)
	    if(( HAL_PCI_REG(0x2050) & 0x1) == 0)
	    {
		  HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) |= (RA305X_SYSCTL_PCIE0_RST);
		  HAL_SYSCTL_REG(RA305X_SYSCTL_CLK_CFG1) &= ~(RA305X_SYSCTL_PCIE_CLK_EN);
		  diag_printf("PCIE0 no card, disable it(RST&CLK)\n");
		  return 0;
	    }

        HAL_PCI_REG(0x2010) = 0x7FFF0001;	//open 7FFF:2G; ENABLE
        HAL_PCI_REG(0x2018) = 0;
        //HAL_PCI_REG(0x2034) = 0x06040001;

        HAL_PCI_REG(0xc) |= (1<<20); // enable pcie0 interrupt

        PCIE_read_config(0, 0, 0, 0x4, &val);
        PCIE_write_config(0, 0, 0, 0x4, val|0x7);

        diag_printf("PCIE0 enabled\n");	
#endif
       	reg = (1 << 16);
	    HAL_PCI_REG(RAPCI_PCICFG_REG) = reg;
	    CYGACC_CALL_IF_DELAY_US(100 * 1000); // mdelay(100)

	    // PCIe
	    HAL_PCI_REG(RAPCIE_ID_REG) = 0x08021814;
	    HAL_PCI_REG(RAPCIE_CLASS_REG) = 0x06040001;
	    HAL_PCI_REG(RAPCIE_SUBID_REG) = 0x28801814;

        // Configure PCI bus.
        next_bus = 1;
        cyg_pci_configure_bus(0, &next_bus);
}


cyg_uint32 cyg_hal_plf_pci_cfg_read_dword (cyg_uint32 bus,
                                           cyg_uint32 devfn,
                                           cyg_uint32 offset)
{
    cyg_uint32 config_data;
    cyg_uint8 dev = CYG_PCI_DEV_GET_DEV(devfn);
    cyg_uint8 fn = CYG_PCI_DEV_GET_FN(devfn);

    config_data = (((offset & 0xF00)>>8)<<24) | (bus << 16) | (dev << 11) | (fn << 8) | (offset & 0xfc) | 0x80000000 ;

    HAL_PCI_REG(RAPCI_CFG_ADDR) = config_data;
    return  HAL_PCI_REG(RAPCI_CFG_DATA);
}

cyg_uint16 cyg_hal_plf_pci_cfg_read_word (cyg_uint32 bus,
                                          cyg_uint32 devfn,
                                          cyg_uint32 offset)
{
    cyg_uint32 config_data;
    cyg_uint8 dev = CYG_PCI_DEV_GET_DEV(devfn);
    cyg_uint8 fn = CYG_PCI_DEV_GET_FN(devfn);

    config_data = (((offset & 0xF00)>>8)<<24) | (bus << 16) | (dev << 11) | (fn << 8) | (offset & 0xfc) | 0x80000000 ;

    HAL_PCI_REG(RAPCI_CFG_ADDR) = config_data;
    return  ((HAL_PCI_REG(RAPCI_CFG_DATA) >> ((offset & 3) << 3)) & 0xffff);
}

cyg_uint8 cyg_hal_plf_pci_cfg_read_byte (cyg_uint32 bus,
                                         cyg_uint32 devfn,
                                         cyg_uint32 offset)
{
    cyg_uint32 config_data;
    cyg_uint8 dev = CYG_PCI_DEV_GET_DEV(devfn);
    cyg_uint8 fn = CYG_PCI_DEV_GET_FN(devfn);
    cyg_uint8 ret;

    config_data = (((offset & 0xF00)>>8)<<24) | (bus << 16) | (dev << 11) | (fn << 8) | (offset & 0xfc) | 0x80000000 ;

    HAL_PCI_REG(RAPCI_CFG_ADDR) = config_data;
    return  ((HAL_PCI_REG(RAPCI_CFG_DATA) >> ((offset & 3) << 3)) & 0xff);
}

void cyg_hal_plf_pci_cfg_write_dword (cyg_uint32 bus,
                                      cyg_uint32 devfn,
                                      cyg_uint32 offset,
                                      cyg_uint32 data)
{
    cyg_uint32 config_data;
    cyg_uint8 dev = CYG_PCI_DEV_GET_DEV(devfn);
    cyg_uint8 fn = CYG_PCI_DEV_GET_FN(devfn);

    config_data = (((offset & 0xF00)>>8)<<24) | (bus << 16) | (dev << 11) | (fn << 8) | (offset & 0xfc) | 0x80000000 ;

    HAL_PCI_REG(RAPCI_CFG_ADDR) = config_data;
    HAL_PCI_REG(RAPCI_CFG_DATA) = data;
}

void cyg_hal_plf_pci_cfg_write_word (cyg_uint32 bus,
                                     cyg_uint32 devfn,
                                     cyg_uint32 offset,
                                     cyg_uint16 data)
{
    cyg_uint32 config_data;
    cyg_uint8 dev = CYG_PCI_DEV_GET_DEV(devfn);
    cyg_uint8 fn = CYG_PCI_DEV_GET_FN(devfn);
    cyg_uint32 value;

    config_data = (((offset & 0xF00)>>8)<<24) | (bus << 16) | (dev << 11) | (fn << 8) | (offset & 0xfc) | 0x80000000 ;

    HAL_PCI_REG(RAPCI_CFG_ADDR) = config_data;
    value = HAL_PCI_REG(RAPCI_CFG_DATA);


    value = (value & ~(0xffff << ((offset & 3) << 3))) | (data << ((offset & 3) << 3));

    HAL_PCI_REG(RAPCI_CFG_ADDR) = config_data;
    HAL_PCI_REG(RAPCI_CFG_DATA) = value;
}

void cyg_hal_plf_pci_cfg_write_byte (cyg_uint32 bus,
                                     cyg_uint32 devfn,
                                     cyg_uint32 offset,
                                     cyg_uint8  data)
{
    cyg_uint32 config_data;
    cyg_uint8 dev = CYG_PCI_DEV_GET_DEV(devfn);
    cyg_uint8 fn = CYG_PCI_DEV_GET_FN(devfn);
    cyg_uint32 value;

    config_data = (((offset & 0xF00)>>8)<<24) | (bus << 16) | (dev << 11) | (fn << 8) | (offset & 0xfc) | 0x80000000 ;

    HAL_PCI_REG(RAPCI_CFG_ADDR) = config_data;
    value = HAL_PCI_REG(RAPCI_CFG_DATA);


    value = (value & ~(0xff << ((offset & 3) << 3))) | (data << ((offset & 3) << 3));

    HAL_PCI_REG(RAPCI_CFG_ADDR) = config_data;
    HAL_PCI_REG(RAPCI_CFG_DATA) = value;
}

#endif // defined(CYGPKG_IO_PCI)



//--------------------------------------------------------------------------


static void hal_init_irq(void);

//--------------------------------------------------------------------------

void hal_platform_init(void)
{
    // Set up eCos/ROM interfaces
    //diag_printf("%s\n",__FUNCTION__);
   
    hal_if_init();
    hal_init_irq();
}


/*------------------------------------------------------------------------*/
/* Reset support                                                          */

void hal_ra305x_reset(void)
{
	HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) = RA305X_SYSCTL_SYS_RST;
	for (;;);	/*  Wait system reset  */
}


//--------------------------------------------------------------------------
// IRQ 
static void hal_init_irq(void)
{

#ifdef CONFIG_MT7628_ASIC
/*set all interrupt to IRQ, not FIQ*/
	HAL_INTCL_REG(RA305X_INTC_SEL0) = RA305X_INTC_ALLINT;
#endif
	/*  Mask all interrupt  */
	HAL_INTCL_REG(RA305X_INTC_DISABLE) = RA305X_INTC_ALLINT;
	
#ifndef  CONFIG_MT7628_ASIC//7628 don't have globle control register
	/*  Enable global interrupt  */
	HAL_INTCL_REG(RA305X_INTC_ENABLE) = RA305X_INTC_GINT_ENABLE;
#endif
}


cyg_uint32 hal_ra305x_irq_decode(cyg_uint32 rawirq)
{
	cyg_uint32 irq, irqstatus;


	if (rawirq > CYGNUM_HAL_INTERRUPT_IP0)
		return rawirq;
	
	irqstatus = HAL_INTCL_REG(RA305X_INTC_IRQSTATUS0);
	if (irqstatus == 0)
		return CYGNUM_HAL_INTERRUPT_BOGUS;
	HAL_LSBIT_INDEX(irq, irqstatus);
	irq += CYGNUM_HAL_INTERRUPT_EXTERNAL_BASE;
	if (irq > CYGNUM_HAL_ISR_MAX)
		irq = CYGNUM_HAL_INTERRUPT_BOGUS;
	//if (rawirq != 5)
	//diag_printf("rawirq = %d irq = %d\n",rawirq,irq);
	return irq;
}



/*------------------------------------------------------------------------*/
/* Exception                                                              */
#if 0
extern void hal_mips_exception_dump(CYG_ADDRWORD, int, CYG_ADDRWORD);
void hal_install_default_exception_handler(void)
{
	int i;
	
	for (i=0; i<CYGNUM_HAL_EXCEPTION_COUNT; i++)
		cyg_exception_set_handler(i, hal_mips_exception_dump, 0, NULL, NULL);
}
#endif

/*------------------------------------------------------------------------*/
/* MISC                                                                   */
cyg_uint32 hal_get_cpu_freq(void)
{
	return CYGHWR_HAL_MIPS_RA305X_CPU_CLOCK;
}

cyg_uint32 hal_get_sysbus_freq(void)
{
	cyg_uint32 cpu_freq;

	cpu_freq = CYGHWR_HAL_MIPS_RA305X_CPU_CLOCK;
/*
	Workaround code for MT7620 platform  by Eddy, TODO: need better solution
	For SDR, the CPU clock is 600000000, and the sysbus is 
	CPU clock / 5
  
	For DDR, the CPU clock is 580000000, and sysbus is
	CPU clock / 3
*/
#ifdef CONFIG_MT7628_ASIC
	return 40*1000*1000;/*40MHz*/
#else
	if (cpu_freq == 600000000)
		return cpu_freq/5; 
	else
		return cpu_freq/3;
#endif
}

/*------------------------------------------------------------------------*/
/* End of plf_misc.c                                                      */

