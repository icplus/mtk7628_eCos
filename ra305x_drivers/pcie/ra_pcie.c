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
	ra_pcie.c

    Abstract:
	Hardware driver for Ralink PCIE
   
    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#include <pkgconf/system.h>
#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/hal_cache.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_if.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/drv_api.h>
#include <ra_pcie.h>

#ifdef CYGPKG_IO_PCI
#include <cyg/io/pci.h>
#else
#error "Need PCI package here"
#endif

#define RA305X_PCIE_RST               (1 << 26)
#define RA305X_PCIE_CLK_EN            (1 << 26)

#define CONFIG_SHOW_SYSTEM_INFO

void __inline__ read_config(unsigned long bus, unsigned long dev, unsigned long func, unsigned long reg, unsigned long *val);
void __inline__ write_config(unsigned long bus, unsigned long dev, unsigned long func, unsigned long reg, unsigned long val);

#define read_c0_status()        __read_32bit_c0_register($12, 0)
#define write_c0_status(val)  __write_32bit_c0_register($12, 0, val)


#define __read_32bit_c0_register(source, sel)                                 \
({ int __res;                                                                                      \
           if (sel == 0)                                                                 \
                     __asm__ __volatile__(                                            \
                   ".set push                   \n" \
                                ".set\tnomips16\n\t"                                     \
                                "mfc0\t%0, " #source "\n\t"                         \
                   ".set pop                   \n" \
                                : "=r" (__res));                                      \
           else                                                                              \
                     __asm__ __volatile__(                                            \
                   ".set push                   \n" \
                                ".set\tmips32\n\t"                                          \
                                ".set\tnomips16\n\t"                                     \
                                "mfc0\t%0, " #source ", " #sel "\n\t"                     \
                                ".set\tmips0\n\t"                                  \
                   ".set pop                   \n" \
                                : "=r" (__res));                                      \
           __res;                                                                                    \
})

           #define __write_32bit_c0_register(register, sel, value)                             \
do {                                                                                         \
           if (sel == 0)                                                                 \
                     __asm__ __volatile__(                                            \
                                "mtc0\t%z0, " #register "\n\t"                                \
                                "ehb\n\t"\
                                : : "Jr" ((unsigned int)(value)));                    \
           else                                                                              \
                     __asm__ __volatile__(                                            \
                                ".set\tmips32\n\t"                                          \
                                "mtc0\t%z0, " #register ", " #sel "\n\t"       \
                                ".set\tmips0"                                                   \
                                : : "Jr" ((unsigned int)(value)));                    \
} while (0)


unsigned short inic_pci_read16(unsigned long addr)
{
	unsigned short val = 0;
	unsigned long irqFlag;
	unsigned long tmp;
	
	HAL_DISABLE_INTERRUPTS(irqFlag);
	tmp = read_c0_status() | (1<<2);
	write_c0_status(tmp);
	val = HAL_REG16(addr);
	tmp = read_c0_status() & (~(1<<2));
	write_c0_status(tmp);
	HAL_RESTORE_INTERRUPTS(irqFlag);
	return val;
}

void inic_pci_write16(unsigned long addr, unsigned short val)
{
	unsigned long irqFlag;
	unsigned long tmp;
	
	HAL_DISABLE_INTERRUPTS(irqFlag);
	tmp = read_c0_status() | (1<<2);
	write_c0_status(tmp); 
	HAL_REG16(addr) = val;
	tmp = read_c0_status() & (~(1<<2));
	write_c0_status(tmp);
	HAL_RESTORE_INTERRUPTS(irqFlag);
}

unsigned char inic_pci_read8(unsigned long addr)
{
	unsigned char val = 0;
	unsigned long irqFlag;
	unsigned long tmp;

	HAL_DISABLE_INTERRUPTS(irqFlag);
	tmp = read_c0_status() | (1<<2);
	write_c0_status(tmp);
	val = HAL_REG8(addr);
	tmp = read_c0_status() & (~(1<<2));
	write_c0_status(tmp);
	HAL_RESTORE_INTERRUPTS(irqFlag);

	return val;
}

void inic_pci_write8(unsigned long addr, unsigned char val)
{
	unsigned long irqFlag;
	unsigned long tmp;
	HAL_DISABLE_INTERRUPTS(irqFlag);
	tmp = read_c0_status() | (1<<2); 
	write_c0_status(tmp);
	HAL_REG8(addr) = val;
	tmp = read_c0_status() & (~(1<<2));
	write_c0_status(tmp);
	HAL_RESTORE_INTERRUPTS(irqFlag);
}

unsigned int inic_pci_read32(unsigned long addr)
{
	unsigned int val = 0;
	unsigned long irqFlag;								
	unsigned long tmp;

	HAL_DISABLE_INTERRUPTS(irqFlag);
	tmp = read_c0_status() | (1<<2);
	write_c0_status(tmp);
 	val = HAL_REG(addr);
	tmp = read_c0_status() & (~(1<<2));
	write_c0_status(tmp);	
	HAL_RESTORE_INTERRUPTS(irqFlag);
	return val;
}


void inic_pci_write32(unsigned long addr, unsigned int val)
{
	unsigned long irqFlag;								
	unsigned long tmp;
	HAL_DISABLE_INTERRUPTS(irqFlag);
	tmp = read_c0_status() | (1<<2);
	write_c0_status(tmp);
 	HAL_REG(addr) = val;			
	tmp = read_c0_status() & (~(1<<2));
	write_c0_status(tmp);	
	HAL_RESTORE_INTERRUPTS(irqFlag);		
}

void __inline__ read_config(unsigned long bus, unsigned long dev, unsigned long func, unsigned long reg, unsigned long *val)
{
	unsigned int address;

	if(bus == 0) {
		reg&=0xff;
	}
	
	address = (((reg & 0xF00)>>8)<<24) | (bus << 16) | (dev << 11) | (func << 8) | (reg & 0xfc) | 0x80000000 ;
	RAPCI_REG(RAPCI_CFGADDR_REG) = address;
	*val = RAPCI_REG(RAPCI_CFGDATA_REG);
}

void __inline__ write_config(unsigned long bus, unsigned long dev, unsigned long func, unsigned long reg, unsigned long val)
{
	unsigned int address;

	if(bus == 0) {
		reg&=0xff;
	}
        /* set addr */
  	address = (((reg & 0xF00)>>8)<<24) | (bus << 16) | (dev << 11) | (func << 8) | (reg & 0xfc) | 0x80000000 ;
        /* start the configuration cycle */
	RAPCI_REG(RAPCI_CFGADDR_REG) = address;
        /* read the data */
	RAPCI_REG(RAPCI_CFGDATA_REG) = val;
}


/* pciBaseClass():
 * Based on appendix D of spec, return a simple string that describes
 * the base class of the incoming class code.
 */
char *
pciBaseClass(unsigned long classcode)
{
	unsigned long baseclass;
	unsigned long subclass_progif;

	baseclass = (classcode >> 16) & 0xff;
	subclass_progif = classcode & 0xffff;

	switch(baseclass) {
		case 0:
			return("pre-class-code-definitions");
		case 1:
			return("mass storage ctrlr");
		case 2:
			return("network ctrlr");
		case 3:
			return("display ctrlr");
		case 4:
			return("multimedia device");
		case 5:
			return("memory ctrlr");
		case 6:	/* Supply additional information for bridge class...
				 */
			switch(subclass_progif) {
				case 0x0000:
					return("host/pci bridge");
				case 0x0100:
					return("pci/isa bridge");
				case 0x0200:
					return("pci/eisa bridge");
				case 0x0300:
					return("pci/microchannel bridge");
				case 0x0400:
					return("pci/pci bridge");
				case 0x0500:
					return("pci/pcmcia bridge");
				case 0x0600:
					return("pci/nubus bridge");
				case 0x0700:
					return("pci/cardbus bridge");
				case 0x8000:
					return("other bridge type");
				default:
					return("bridge device");
			}
		case 7:
			return("simple communication ctrlr");
		case 8:
			return("base system peripheral");
		case 9:
			return("input device");
		case 10:
			return("docking station");
		case 11:
			return("processor");
		case 12:
			return("serial bus ctrlr");
		case 13:
			return("wireless ctrlr");
		case 14:
			return("intelligent io ctrlr");
		case 15:
			return("satellite communication ctrlr");
		case 16:
			return("encrypt/decrypt ctrlr");
		case 17:
			return("data acquisition ctrlr");
		case 256:
			return("no fit");
		default:
			return("reserved");
	}
}



#define NO_DEVICE				0xffffffff
int pciBusNum;

void pciscan(long bus, long func, int showhdr, int enumerate)
{
	long	device;
	unsigned char	hdr_type, rev_id;
	unsigned short	vendor_id, device_id;
	unsigned long	value, class_code;

	if (showhdr) {
		diag_printf("Bus Dev Vndr  Dev   Rev Hdr  Class\n");  
		diag_printf("Num Num Id    Id    Id  Type Code\n");  
	}

	if ((enumerate == 1) && (bus == 0))
		pciBusNum = 0;

	for(device=0;device<=31;device++) {
		/* Retrieve portions of the configuration header that
		 * are required by all PCI compliant devices...
		 * Vendor, Device and Revision IDs, Class Code and Header Type
		 * (see pg 191 of spec).
		 */

		/* Read reg_0 for vendor and device ids:
		 */
		read_config(bus, device, func, (0 << 2),&value);
		if (value == NO_DEVICE)
			continue;
		vendor_id = (unsigned short)(value & 0xffff);
		device_id = (unsigned short)((value>>16) & 0xffff);
		
		/* Read reg_2 for class code and revision id:
		 */
		read_config(bus, device, func, (2<<2),&value);
		rev_id = (unsigned char)(value & 0xff);
		class_code = (unsigned long)((value>>8) & 0xffffff);

		/* Read reg_3:  header type:
		 */
		read_config(bus, device, func, (3<<2),&value);
		hdr_type = (unsigned char)((value>>16) & 0xff);

		diag_printf("%2ld  %02ld  x%04x x%04x",bus,
			device,vendor_id,device_id);
		diag_printf(" x%02x x%02x  x%06lx (%s)\n",rev_id,
			hdr_type,class_code,pciBaseClass(class_code));

		/* If enumeration is enabled, see if this is a PCI-to-PCI
		 * bridge.  If it is, then nest into pciscan...
		 */
		if ((enumerate) && (class_code == 0x060400)) {
			unsigned long pribus, secbus, subbus;
			diag_printf("it's a pci bridge\n");
			pribus = pciBusNum & 0x0000ff;
			pciBusNum++;
			secbus = ((pciBusNum << 8) & 0x00ff00);
			subbus = ((pciBusNum << 16) & 0xff0000);

			read_config(bus, device, func, (6<<2),&value);
			value &= 0xffff0000;
			value |= (pribus | secbus);
			write_config(bus,device,func,(6<<2),value);

			pciscan(pciBusNum,func,0,1);

			read_config(bus, device, func, (6<<2),&value);
			value &= 0xff000000;
			value |= (subbus | pribus | secbus);
			write_config(bus,device,func,(6<<2),value);
		}
		
	}

}


void pcie_host_mode_enable()
{
	cyg_uint32 reg;
	
	reg = HAL_SYSCTL_REG(0x60);
	HAL_SYSCTL_REG(0x60) = (reg & ~(0x3 << 16)); // PERST_N
	HAL_SYSCTL_REG(0x34) &= ~(RA305X_PCIE_RST);
	HAL_SYSCTL_REG(0x30) |= RA305X_PCIE_CLK_EN;

	CYGACC_CALL_IF_DELAY_US(100 * 1000); // mdelay(100)

	if (HAL_SYSCTL_REG(0x9c) & (1<<23))
	{
	}
	else
	{
		HAL_SYSCTL_REG(0x34) |= (RA305X_PCIE_RST);
		HAL_SYSCTL_REG(0x30) &= (RA305X_PCIE_CLK_EN);
		return;
	}

	reg = HAL_SYSCTL_REG(0xa0);
	reg |= (0x1 << 19);
	reg &= ~(0x1<<18);
	reg &= ~(0x1<<17);
	reg |= (0x1 << 31);
	HAL_SYSCTL_REG(0xa0) = reg;

	CYGACC_CALL_IF_DELAY_US(100 * 1000); // mdelay(100)
	diag_printf("Reset the PCI device card\n");
	RAPCI_REG(RAPCI_PCICFG_REG) &= ~(1<<1); // de-assert PERST
	
	diag_printf("waiting\n");
	CYGACC_CALL_IF_DELAY_US(500 * 1000); // mdelay(500)

	reg = (1 << 16);
	RAPCI_REG(RAPCI_PCICFG_REG) = reg;
        CYGACC_CALL_IF_DELAY_US(100 * 1000); // mdelay(100)
	
	// PCIe
	RAPCI_REG(RAPCIE_ID_REG) = 0x08021814;
	RAPCI_REG(RAPCIE_CLASS_REG) = 0x06040001;
	RAPCI_REG(RAPCIE_SUBID_REG) = 0x28801814;
	
	return; // Roger test
	
	pciscan(0,0,1,1);
	pciscan(1,0,1,1);

	if (RAPCI_REG(RAPCIE_STATUS_REG))
	{
		// Add concurrent code here
		diag_printf("PCIe host is link!!!\n");
	}
	else
	{
		// Add concurrent code here
		diag_printf("PCIe host is not link!!!\n");
		return;
	}

	// Set Base Address Mask, bit 0: enable
	RAPCI_REG(RAPCIE_BAR0SETUP_REG) = 0x00ff0001;
	
	RAPCI_REG(RAPCI_MEMBASE_REG) = RT2880_PCI_SLOT1_BASE;
	RAPCI_REG(RAPCIE_IMBASEBAR0_REG) = 0;

	/* set BAR0 offset of PCIe bridge  */
	write_config(0, 1, 0, PCI_CFG_BAR0_REG, ~0);

#ifdef CONFIG_SHOW_SYSTEM_INFO	
	read_config(0, 1, 0, PCI_CFG_BAR0_REG, &reg);
	diag_printf("***PCI_CFG_BAR0_REG: size = %x\n", reg);
#endif // CONFIG_SHOW_SYSTEM_INFO //

	write_config(0, 1, 0, PCI_CFG_BAR0_REG, 0);

#ifdef CONFIG_SHOW_SYSTEM_INFO
	read_config(0, 1, 0, PCI_CFG_BAR0_REG, &reg);
	diag_printf("***PCI_CFG_BAR0_REG: base address = %x\n", reg);
#endif // CONFIG_SHOW_SYSTEM_INFO //

	{
		unsigned int  memory_base;
		unsigned int  memory_limit;
		memory_base = RT2880_PCI_SLOT1_BASE;
		memory_limit = (RT2880_PCI_SLOT1_BASE + 0x100000)>>16;	//allocate 1M memory downstream
		reg = memory_base | (memory_limit<<16);
		write_config(0, 1, 0, 0x20, reg);	
		read_config(0, 1, 0, 0x20, &reg);	//Memory Base Register and Memory Limit Register
	}

	pcibios_fixup_resources(0, 0, 0);	
	pcibios_fixup_resources(0, 1, 0);	
	pcibios_fixup_resources(1, 0, 0);

	/* Enable PCIe interrupt input in RC mode */
	RAPCI_REG(RAPCI_PCIENA_REG) |= (1<<20);
}

void pcibios_fixup_resources(int bus,int dev, int func)	
{
	unsigned int reg;	
	unsigned int mem_pool = 0;
	int i, j;
	unsigned BaseAddr, data = 0;

#ifdef CONFIG_SHOW_SYSTEM_INFO
	diag_printf("============================\n");
	diag_printf("bus = %d, dev = %d\n", bus, dev);
	diag_printf("============================\n");

	// PCI bios fixup resources
	diag_printf("PCI bios fixup resources\n");
#endif // CONFIG_SHOW_SYSTEM_INFO //

	if((bus == 1) && (dev == 0))
	{

		BaseAddr = PCI_CFG_BAR0_REG; /* 0x10 */
		for(i=0; i<6; i++, BaseAddr+=4)
		{
			// detect resource usage (size) BAR0 ~ BAR5

			read_config(bus, dev, 0, BaseAddr, &data);
#ifdef CONFIG_SHOW_SYSTEM_INFO			
			diag_printf("***BAR[%d] = %8x\n", i,data );
#endif // CONFIG_SHOW_SYSTEM_INFO //

			write_config(bus, dev, 0, BaseAddr, ~0);
			read_config(bus, dev, 0, BaseAddr, &data);

#ifdef CONFIG_SHOW_SYSTEM_INFO
			diag_printf("***BAR[%d] = %8x\n", i,data );
#endif // CONFIG_SHOW_SYSTEM_INFO //

			if(data != 0)
			{
				// resource request exist
				if(data & 1)
				{
					// IO space, IO Mapping 
				}
				else
				{
					// Memory Mapping
					// Memory space, form bit4
					for(j=4; j<32; j++)
						if(data&(1<<j))break;		

					write_config(bus, dev, 0, BaseAddr, RT2880_PCI_SLOT1_BASE + mem_pool);
					read_config(bus, dev, 0, BaseAddr, &reg);
					mem_pool += 1<<j;

#ifdef CONFIG_SHOW_SYSTEM_INFO
					diag_printf("***BAR[%d] = %8x\n", i,reg );
					diag_printf("mem_pool = %x\n", mem_pool);
#endif // CONFIG_SHOW_SYSTEM_INFO //
				}
			}

		}	

		// allocate interrupt line
		{
			read_config(bus, dev, 0, PCI_CFG_INT_REG, &reg);
			diag_printf("1. PCI_CFG_INT_REG = %x\n", reg);

			/* set interrupt line 0x2, set pci interrupt pin 0x2 */
			/* Interrupt Pin: read-only
			 * A value of 1 corresponds to INTA#
			 * A value of 2 corresponds to INTB#
			 */
			reg &= 0xFFFF0000;			
			reg |= 0x0105; 
			write_config(bus, dev, 0, PCI_CFG_INT_REG, reg);	
			read_config(bus, dev, 0, PCI_CFG_INT_REG, &reg);
			diag_printf("2. PCI_CFG_INT_REG = %x\n", reg);
		}

	}


	read_config(bus, dev, 0, 0x0C, &reg);
	reg |= 0x2001;
	write_config(0, dev, 0, 0x0C, reg);
	read_config(bus, dev, 0, 0x0C, &reg);


	// Set device
	read_config(bus, dev, 0, PCI_CFG_CMD_REG, &reg);

	reg |= PCI_CFG_CMD_MASTER|
		PCI_CFG_CMD_MEM|
		PCI_CFG_CMD_INVALIDATE|
		PCI_CFG_CMD_FAST_BACK|
		PCI_CFG_CMD_SERR|
		PCI_CFG_CMD_WAIT|
		PCI_CFG_CMD_PARITY|
		PCI_CFG_CMD_IO;


	write_config(bus, dev, 0, PCI_CFG_CMD_REG, reg);
	read_config(bus, dev, 0, PCI_CFG_CMD_REG, &reg);


}

#ifdef CYGPKG_IO_PCI

extern void cyg_pci_init(void);

#define MTK_PCI_VENDOR_ID		0x14C3
#define NIC7610_PCIe_DEVICE_ID		0x7610
#define NIC7650_PCIe_DEVICE_ID		0x7650

int ra305x_pcie_init(cyg_uint32 *intline)
{
	cyg_pci_device_id devid = 0;
	cyg_pci_device dev_info;
	cyg_bool ret;
	int i, reg;
	
	cyg_pci_init();
	ret = cyg_pci_find_device(0x14c3, 0x7603, &devid);
	if (!ret)
		ret = cyg_pci_find_device(0x1400, 0x7603, &devid);
	
	if (ret) {
		cyg_pci_get_device_info(devid, &dev_info);
		ret = cyg_pci_configure_device(&dev_info);

		diag_printf("            Vendor Id : 0x%08X\n", dev_info.vendor);
		diag_printf("            Device Id : 0x%08X\n", dev_info.device);
		diag_printf("            bar 0     : 0x%08X\n", dev_info.base_address[0]);
		diag_printf("            bar 1     : 0x%08X\n", dev_info.base_address[1]);
		diag_printf("            map 0     : 0x%08X\n", dev_info.base_map[0]);
		diag_printf("            map 1     : 0x%08X\n", dev_info.base_map[1]);

		// write interrupt line
		read_config(1, 0, 0, PCI_CFG_INT_REG, &reg);
		dev_info.hal_vector = reg & 0xff;
		*intline = dev_info.hal_vector;
		RAPCI_REG(RAPCI_MEMBASE_REG) = dev_info.base_address[0];
		diag_printf("hal_vec = %x, base address = %x\n", dev_info.hal_vector, dev_info.base_address[0]);
	}
	
	RAPCI_REG(RAPCIE_BAR0SETUP_REG) = 0x7fff0001;
	RAPCI_REG(RAPCI_PCIENA_REG) |= (1<<20);

	return dev_info.base_address[0];
}

#endif


