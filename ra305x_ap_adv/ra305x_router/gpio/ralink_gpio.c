#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>     /* For dial_printf */
#include <cyg/hal/hal_if.h>     /* For CYGACC_CALL_IF_DELAY_US */

#include <gpio.h> 


// #define 	RALINK_GPIO_BASE		(RALINK_PIO_BASE - 0xA0000000) // phy address
#define 	RALINK_GPIO_BASE		RALINK_PIO_BASE // virtual address
#define	RALINK_PIO_SIZE			0x100

static struct ralink_gpio_chip *global_rg;

static struct ralink_gpio_chip rt305x_gpio_chips[] =
{
	{
		.chip = {
			.label	= "RT305X-GPIO0",
			.base	= 0,
			.ngpio	= 24,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x00,
			[RALINK_GPIO_REG_EDGE]	= 0x04,
			[RALINK_GPIO_REG_RENA]	= 0x08,
			[RALINK_GPIO_REG_FENA]	= 0x0c,
			[RALINK_GPIO_REG_DATA]	= 0x20,
			[RALINK_GPIO_REG_DIR]	= 0x24,
			[RALINK_GPIO_REG_POL]	= 0x28,
			[RALINK_GPIO_REG_SET]	= 0x2c,
			[RALINK_GPIO_REG_RESET]	= 0x30,
			[RALINK_GPIO_REG_TOGGLE] = 0x34,
		},
		.map_base = RALINK_GPIO_BASE,
		.map_size = RALINK_PIO_SIZE,
	},
	{
		.chip = {
			.label	= "RT305X-GPIO1",
			.base	= 24,
			.ngpio	= 16,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x38,
			[RALINK_GPIO_REG_EDGE]	= 0x3c,
			[RALINK_GPIO_REG_RENA]	= 0x40,
			[RALINK_GPIO_REG_FENA]	= 0x44,
			[RALINK_GPIO_REG_DATA]	= 0x48,
			[RALINK_GPIO_REG_DIR]	= 0x4c,
			[RALINK_GPIO_REG_POL]	= 0x50,
			[RALINK_GPIO_REG_SET]	= 0x54,
			[RALINK_GPIO_REG_RESET]	= 0x58,
			[RALINK_GPIO_REG_TOGGLE] = 0x5c,
		},
		.map_base = RALINK_GPIO_BASE,
		.map_size = RALINK_PIO_SIZE,
	},
	{
		.chip = {
			.label	= "RT305X-GPIO2",
			.base	= 40,
			.ngpio	= 12,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x60,
			[RALINK_GPIO_REG_EDGE]	= 0x64,
			[RALINK_GPIO_REG_RENA]	= 0x68,
			[RALINK_GPIO_REG_FENA]	= 0x6c,
			[RALINK_GPIO_REG_DATA]	= 0x70,
			[RALINK_GPIO_REG_DIR]	= 0x74,
			[RALINK_GPIO_REG_POL]	= 0x78,
			[RALINK_GPIO_REG_SET]	= 0x7c,
			[RALINK_GPIO_REG_RESET]	= 0x80,
			[RALINK_GPIO_REG_TOGGLE] = 0x84,
		},
		.map_base = RALINK_GPIO_BASE,
		.map_size = RALINK_PIO_SIZE,
	},
};

static struct ralink_gpio_chip rt5350_gpio_chips[] =
{
	{
		.chip = {
			.label	= "RT5350-GPIO0",
			.base	= 0,
			.ngpio	= 21,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x00,
			[RALINK_GPIO_REG_EDGE]	= 0x04,
			[RALINK_GPIO_REG_RENA]	= 0x08,
			[RALINK_GPIO_REG_FENA]	= 0x0c,
			[RALINK_GPIO_REG_DATA]	= 0x20,
			[RALINK_GPIO_REG_DIR]	= 0x24,
			[RALINK_GPIO_REG_POL]	= 0x28,
			[RALINK_GPIO_REG_SET]	= 0x2c,
			[RALINK_GPIO_REG_RESET]	= 0x30,
			[RALINK_GPIO_REG_TOGGLE] = 0x34,
		},
		.map_base = RALINK_GPIO_BASE,
		.map_size = RALINK_PIO_SIZE,
	},
	{
		.chip = {
			.label	= "RT5350-GPIO1",
			.base	= 21,
			.ngpio	= 28,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x60,
			[RALINK_GPIO_REG_EDGE]	= 0x64,
			[RALINK_GPIO_REG_RENA]	= 0x68,
			[RALINK_GPIO_REG_FENA]	= 0x6c,
			[RALINK_GPIO_REG_DATA]	= 0x70,
			[RALINK_GPIO_REG_DIR]	= 0x74,
			[RALINK_GPIO_REG_POL]	= 0x78,
			[RALINK_GPIO_REG_SET]	= 0x7c,
			[RALINK_GPIO_REG_RESET]	= 0x80,
			[RALINK_GPIO_REG_TOGGLE] = 0x84,
		},
		.map_base = RALINK_GPIO_BASE,
		.map_size = RALINK_PIO_SIZE,
	}
};

static struct ralink_gpio_chip rt3883_gpio_chips[] =
{
	{
		.chip = {
			.label			= "RT3883-GPIO0",
			.base			= 0,
			.ngpio			= 24,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x00,
			[RALINK_GPIO_REG_EDGE]	= 0x04,
			[RALINK_GPIO_REG_RENA]	= 0x08,
			[RALINK_GPIO_REG_FENA]	= 0x0c,
			[RALINK_GPIO_REG_DATA]	= 0x20,
			[RALINK_GPIO_REG_DIR]	= 0x24,
			[RALINK_GPIO_REG_POL]	= 0x28,
			[RALINK_GPIO_REG_SET]	= 0x2c,
			[RALINK_GPIO_REG_RESET]	= 0x30,
			[RALINK_GPIO_REG_TOGGLE] = 0x34,
		},
		.map_base	= RALINK_GPIO_BASE,
		.map_size	= RALINK_PIO_SIZE,
	},
	{
		.chip = {
			.label			= "RT3883-GPIO1",
			.base			= 24,
			.ngpio			= 16,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x38,
			[RALINK_GPIO_REG_EDGE]	= 0x3c,
			[RALINK_GPIO_REG_RENA]	= 0x40,
			[RALINK_GPIO_REG_FENA]	= 0x44,
			[RALINK_GPIO_REG_DATA]	= 0x48,
			[RALINK_GPIO_REG_DIR]	= 0x4c,
			[RALINK_GPIO_REG_POL]	= 0x50,
			[RALINK_GPIO_REG_SET]	= 0x54,
			[RALINK_GPIO_REG_RESET]	= 0x58,
			[RALINK_GPIO_REG_TOGGLE] = 0x5c,
		},
		.map_base	= RALINK_GPIO_BASE,
		.map_size	= RALINK_PIO_SIZE,
	},
	{
		.chip = {
			.label			= "RT3883-GPIO2",
			.base			= 40,
			.ngpio			= 32,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x60,
			[RALINK_GPIO_REG_EDGE]	= 0x64,
			[RALINK_GPIO_REG_RENA]	= 0x68,
			[RALINK_GPIO_REG_FENA]	= 0x6c,
			[RALINK_GPIO_REG_DATA]	= 0x70,
			[RALINK_GPIO_REG_DIR]	= 0x74,
			[RALINK_GPIO_REG_POL]	= 0x78,
			[RALINK_GPIO_REG_SET]	= 0x7c,
			[RALINK_GPIO_REG_RESET]	= 0x80,
			[RALINK_GPIO_REG_TOGGLE] = 0x84,
		},
		.map_base	= RALINK_GPIO_BASE,
		.map_size	= RALINK_PIO_SIZE,
	},
	{
		.chip = {
			.label			= "RT3883-GPIO3",
			.base			= 72,
			.ngpio			= 24,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x88,
			[RALINK_GPIO_REG_EDGE]	= 0x8c,
			[RALINK_GPIO_REG_RENA]	= 0x90,
			[RALINK_GPIO_REG_FENA]	= 0x94,
			[RALINK_GPIO_REG_DATA]	= 0x98,
			[RALINK_GPIO_REG_DIR]	= 0x9c,
			[RALINK_GPIO_REG_POL]	= 0xa0,
			[RALINK_GPIO_REG_SET]	= 0xa4,
			[RALINK_GPIO_REG_RESET]	= 0xa8,
			[RALINK_GPIO_REG_TOGGLE] = 0xac,
		},
		.map_base	= RALINK_GPIO_BASE,
		.map_size	= RALINK_PIO_SIZE,
	},
};

static struct ralink_gpio_chip mt7620_gpio_chips[] =
{
	{
		.chip = {
			.label			= "MT7620-GPIO0",
			.base			= 0,
			.ngpio			= 24,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x00,
			[RALINK_GPIO_REG_EDGE]	= 0x04,
			[RALINK_GPIO_REG_RENA]	= 0x08,
			[RALINK_GPIO_REG_FENA]	= 0x0c,
			[RALINK_GPIO_REG_DATA]	= 0x20,
			[RALINK_GPIO_REG_DIR]	= 0x24,
			[RALINK_GPIO_REG_POL]	= 0x28,
			[RALINK_GPIO_REG_SET]	= 0x2c,
			[RALINK_GPIO_REG_RESET]	= 0x30,
			[RALINK_GPIO_REG_TOGGLE] = 0x34,
		},
		.map_base	= RALINK_GPIO_BASE,
		.map_size	= RALINK_PIO_SIZE,
	},
	{
		.chip = {
			.label			= "MT7620-GPIO1",
			.base			= 24,
			.ngpio			= 16,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x38,
			[RALINK_GPIO_REG_EDGE]	= 0x3c,
			[RALINK_GPIO_REG_RENA]	= 0x40,
			[RALINK_GPIO_REG_FENA]	= 0x44,
			[RALINK_GPIO_REG_DATA]	= 0x48,
			[RALINK_GPIO_REG_DIR]	= 0x4c,
			[RALINK_GPIO_REG_POL]	= 0x50,
			[RALINK_GPIO_REG_SET]	= 0x54,
			[RALINK_GPIO_REG_RESET]	= 0x58,
			[RALINK_GPIO_REG_TOGGLE] = 0x5c,
		},
		.map_base	= RALINK_GPIO_BASE,
		.map_size	= RALINK_PIO_SIZE,
	},
	{
		.chip = {
			.label			= "MT7620-GPIO2",
			.base			= 40,
			.ngpio			= 32,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x60,
			[RALINK_GPIO_REG_EDGE]	= 0x64,
			[RALINK_GPIO_REG_RENA]	= 0x68,
			[RALINK_GPIO_REG_FENA]	= 0x6c,
			[RALINK_GPIO_REG_DATA]	= 0x70,
			[RALINK_GPIO_REG_DIR]	= 0x74,
			[RALINK_GPIO_REG_POL]	= 0x78,
			[RALINK_GPIO_REG_SET]	= 0x7c,
			[RALINK_GPIO_REG_RESET]	= 0x80,
			[RALINK_GPIO_REG_TOGGLE] = 0x84,
		},
		.map_base	= RALINK_GPIO_BASE,
		.map_size	= RALINK_PIO_SIZE,
	},
	{
		.chip = {
			.label			= "MT7620-GPIO3",
			.base			= 72,
			.ngpio			= 1,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x88,
			[RALINK_GPIO_REG_EDGE]	= 0x8c,
			[RALINK_GPIO_REG_RENA]	= 0x90,
			[RALINK_GPIO_REG_FENA]	= 0x94,
			[RALINK_GPIO_REG_DATA]	= 0x98,
			[RALINK_GPIO_REG_DIR]	= 0x9c,
			[RALINK_GPIO_REG_POL]	= 0xa0,
			[RALINK_GPIO_REG_SET]	= 0xa4,
			[RALINK_GPIO_REG_RESET]	= 0xa8,
			[RALINK_GPIO_REG_TOGGLE] = 0xac,
		},
		.map_base	= RALINK_GPIO_BASE,
		.map_size	= RALINK_PIO_SIZE,
	},
};

static struct ralink_gpio_chip mt7621_gpio_chips[] =
{
	{
		.chip = {
			.label			= "MT7621-GPIO0",
			.base			= 0,
			.ngpio			= 32,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x90,
			[RALINK_GPIO_REG_EDGE]	= 0xA0,
			[RALINK_GPIO_REG_RENA]	= 0x50,
			[RALINK_GPIO_REG_FENA]	= 0x60,
			[RALINK_GPIO_REG_DATA]	= 0x20,
			[RALINK_GPIO_REG_DIR]	= 0x00,
			[RALINK_GPIO_REG_POL]	= 0x10,
			[RALINK_GPIO_REG_SET]	= 0x30,
			[RALINK_GPIO_REG_RESET]	= 0x40,
		},
		.map_base	= RALINK_GPIO_BASE,
		.map_size	= RALINK_PIO_SIZE,
	},
	{
		.chip = {
			.label			= "MT7621-GPIO1",
			.base			= 32,
			.ngpio			= 32,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x94,
			[RALINK_GPIO_REG_EDGE]	= 0xA4,
			[RALINK_GPIO_REG_RENA]	= 0x54,
			[RALINK_GPIO_REG_FENA]	= 0x64,
			[RALINK_GPIO_REG_DATA]	= 0x24,
			[RALINK_GPIO_REG_DIR]	= 0x04,
			[RALINK_GPIO_REG_POL]	= 0x14,
			[RALINK_GPIO_REG_SET]	= 0x34,
			[RALINK_GPIO_REG_RESET]	= 0x44,
		},
		.map_base	= RALINK_GPIO_BASE,
		.map_size	= RALINK_PIO_SIZE,
	},
	{
		.chip = {
			.label			= "MT7621-GPIO3",
			.base			= 64,
			.ngpio			= 32,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x98,
			[RALINK_GPIO_REG_EDGE]	= 0xA8,
			[RALINK_GPIO_REG_RENA]	= 0x58,
			[RALINK_GPIO_REG_FENA]	= 0x68,
			[RALINK_GPIO_REG_DATA]	= 0x28,
			[RALINK_GPIO_REG_DIR]	= 0x08,
			[RALINK_GPIO_REG_POL]	= 0x18,
			[RALINK_GPIO_REG_SET]	= 0x38,
			[RALINK_GPIO_REG_RESET]	= 0x48,
		},
		.map_base	= RALINK_GPIO_BASE,
		.map_size	= RALINK_PIO_SIZE,
	},
};

static struct ralink_gpio_chip mt7628_gpio_chips[] =
{
	{
		.chip = {
			.label			= "MT7628-GPIO0",
			.base			= 0,
			.ngpio			= 32,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x90,
			[RALINK_GPIO_REG_EDGE]	= 0xA0,
			[RALINK_GPIO_REG_RENA]	= 0x50,
			[RALINK_GPIO_REG_FENA]	= 0x60,
			[RALINK_GPIO_REG_DATA]	= 0x20,
			[RALINK_GPIO_REG_DIR]	= 0x00,
			[RALINK_GPIO_REG_POL]	= 0x10,
			[RALINK_GPIO_REG_SET]	= 0x30,
			[RALINK_GPIO_REG_RESET]	= 0x40,
		},
		.map_base	= RALINK_GPIO_BASE,
		.map_size	= RALINK_PIO_SIZE,
	},
	{
		.chip = {
			.label			= "MT7628-GPIO1",
			.base			= 32,
			.ngpio			= 32,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x94,
			[RALINK_GPIO_REG_EDGE]	= 0xA4,
			[RALINK_GPIO_REG_RENA]	= 0x54,
			[RALINK_GPIO_REG_FENA]	= 0x64,
			[RALINK_GPIO_REG_DATA]	= 0x24,
			[RALINK_GPIO_REG_DIR]	= 0x04,
			[RALINK_GPIO_REG_POL]	= 0x14,
			[RALINK_GPIO_REG_SET]	= 0x34,
			[RALINK_GPIO_REG_RESET]	= 0x44,
		},
		.map_base	= RALINK_GPIO_BASE,
		.map_size	= RALINK_PIO_SIZE,
	},
	{
		.chip = {
			.label			= "MT7628-GPIO3",
			.base			= 64,
			.ngpio			= 32,
		},
		.regs = {
			[RALINK_GPIO_REG_INT]	= 0x98,
			[RALINK_GPIO_REG_EDGE]	= 0xA8,
			[RALINK_GPIO_REG_RENA]	= 0x58,
			[RALINK_GPIO_REG_FENA]	= 0x68,
			[RALINK_GPIO_REG_DATA]	= 0x28,
			[RALINK_GPIO_REG_DIR]	= 0x08,
			[RALINK_GPIO_REG_POL]	= 0x18,
			[RALINK_GPIO_REG_SET]	= 0x38,
			[RALINK_GPIO_REG_RESET]	= 0x48,
		},
		.map_base	= RALINK_GPIO_BASE,
		.map_size	= RALINK_PIO_SIZE,
	},
};


void detect_chip(u8 offset)
{
	u32 i, count;

#if defined (CONFIG_RT3050_ASIC) || defined (CONFIG_RT3052_ASIC) || defined (CONFIG_RT3352_ASIC) || defined (CONFIG_RT3350_ASIC)
	count = ARRAY_SIZE(rt305x_gpio_chips);
#elif  defined (CONFIG_RALINK_RT3883)
	count = ARRAY_SIZE(rt3883_gpio_chips);
#elif  defined (CONFIG_RALINK_RT5350)
	count = ARRAY_SIZE(rt5350_gpio_chips);
#elif  defined (CONFIG_RALINK_MT7620)
	count = ARRAY_SIZE(mt7620_gpio_chips);
#elif  defined (CONFIG_RALINK_MT7621)
	count = ARRAY_SIZE(mt7621_gpio_chips);
#elif  defined (CONFIG_RALINK_MT7628)
	count = ARRAY_SIZE(mt7628_gpio_chips);
#endif

	//diag_printf("gpio array count = %d\n", count);
	for(i=0; i <= count; i++)
	{
#if defined (CONFIG_RT3050_ASIC) || defined (CONFIG_RT3052_ASIC) || defined (CONFIG_RT3352_ASIC) || defined (CONFIG_RT3350_ASIC)
		if(offset < rt305x_gpio_chips[i+1].chip.base)
		{
			global_rg = &rt305x_gpio_chips[i];
			break;
		}
#elif  defined (CONFIG_RT5350_ASIC)
		if(offset < rt5350_gpio_chips[i+1].chip.base)
		{
			global_rg = &rt5350_gpio_chips[i];
			break;
		}
#elif  defined (CONFIG_MT7620_ASIC)
		if(offset < mt7620_gpio_chips[i+1].chip.base)
		{
			global_rg = &mt7620_gpio_chips[i];
			break;
		}
#elif  defined (CONFIG_MT7621_ASIC)
		if(offset < mt7621_gpio_chips[i+1].chip.base)
		{
			global_rg = &mt7621_gpio_chips[i];
			break;
		}
#elif  defined (CONFIG_MT7628_ASIC)
		if(offset < mt7628_gpio_chips[i+1].chip.base)
		{
			global_rg = &mt7628_gpio_chips[i];
			break;
		}
#else
		diag_printf("No GPIO lib support!!!\n");
#endif
	}
}

void ralink_gpio_wr(struct ralink_gpio_chip *chip, u8 reg, u32 val)
{
	HAL_REG32(chip->map_base + chip->regs[reg]) = val;
}

u32 ralink_gpio_rr(struct ralink_gpio_chip *chip, u8 reg)
{
	return HAL_REG32(chip->map_base + chip->regs[reg]);
}

static int ralink_gpio_direction_input(unsigned offset)
{
	u32 t;
	detect_chip(offset);
	t = ralink_gpio_rr(global_rg, RALINK_GPIO_REG_DIR);
	t &= ~(1 << (offset - global_rg->chip.base));
	ralink_gpio_wr(global_rg, RALINK_GPIO_REG_DIR, t);

	return 0;
}

static int ralink_gpio_direction_output(unsigned offset, int value)
{
	u32 t, reg;

	detect_chip(offset);

	reg = (value) ? RALINK_GPIO_REG_SET : RALINK_GPIO_REG_RESET;
	ralink_gpio_wr(global_rg, reg, 1 << (offset - global_rg->chip.base));
	t = ralink_gpio_rr(global_rg, RALINK_GPIO_REG_DIR);
	t |= 1 << (offset - global_rg->chip.base);

	ralink_gpio_wr(global_rg, RALINK_GPIO_REG_DIR, t);
	return 0;
}

static void ralink_gpio_set(unsigned offset, int value)
{
	u32 reg;

	detect_chip(offset);
	reg = (value) ? RALINK_GPIO_REG_SET : RALINK_GPIO_REG_RESET;
	ralink_gpio_wr(global_rg, reg, 1 << (offset - global_rg->chip.base));
}


static int ralink_gpio_get(unsigned offset)
{
	u32 t;

	detect_chip(offset);
	t = ralink_gpio_rr(global_rg, RALINK_GPIO_REG_DATA);
	return !!(t & (1 << (offset - global_rg->chip.base) ));
}


void gpio_direction_input(unsigned int pin)
{
	ralink_gpio_direction_input(pin);
}

void gpio_direction_output(unsigned int pin)
{
	ralink_gpio_direction_output(pin,1);
}

int gpio_get(unsigned int pin)
{
	return ralink_gpio_get(pin);
}

void gpio_set(unsigned int pin, int value)
{
	ralink_gpio_set(pin, value);
}



void gpio_mode_init(struct ralink_gpio_mode *board_gpio_mode)
{
	u32 gpiomode;

	GPIO_DBGPRINT("MTK/Ralink Board GPIO_MODE init:\n");

	//config these pins to gpio mode
	gpiomode = HAL_REG32(REG_GPIOMODE);
#if 0
	gpiomode &= ~0x1C;  //clear bit[2:4]UARTF_SHARE_MODE
	GPIO_DBGPRINT("\tUARTF:SHART\n");
#endif
	gpiomode |= RALINK_GPIOMODE_DFT;

	if (board_gpio_mode->i2c)
	{
		GPIO_DBGPRINT("\tI2C:GPIO\n");
		gpiomode |= RALINK_GPIOMODE_I2C;
	}

	if (board_gpio_mode->spi)
	{
		GPIO_DBGPRINT("\tSPI:GPIO\n");
		gpiomode |= RALINK_GPIOMODE_SPI;
	}

#if 0
	if (board_gpio_mode->uartf)
	{
		gpiomode |= RALINK_GPIOMODE_UARTF;
		GPIO_DBGPRINT("\tUART_F:GPIO\n");
	}

	if (board_gpio_mode->spi_refclk)
	{
		GPIO_DBGPRINT("\tSPI_REFCLK:GPIO\n");
		gpiomode |= RALINK_GPIOMODE_SPI_REFCLK;
	}

	if (board_gpio_mode->uartl)
	{
		GPIO_DBGPRINT("\tUART_L:GPIO\n");
		gpiomode |= RALINK_GPIOMODE_UARTL;
	}

	if (board_gpio_mode->jtag)
	{
		GPIO_DBGPRINT("\tJTAG:GPIO\n");
		gpiomode |= RALINK_GPIOMODE_JTAG;
	}

	if (board_gpio_mode->pa)
	{
		GPIO_DBGPRINT("\tPA_G:GPIO\n");
		gpiomode |= RALINK_GPIOMODE_PA_G;
	}

	if (board_gpio_mode->mdio)
	{
		GPIO_DBGPRINT("\tMDIO:GPIO\n");
#if defined(CONFIG_RT5350_ASIC) || defined(CONFIG_RT5350_FPGA)
		GPIO_DBGPRINT("\tMDIO:NONE\n");
#else
		gpiomode |= RALINK_GPIOMODE_MDIO;
#endif
	}

	if (board_gpio_mode->ge1)
	{
		GPIO_DBGPRINT("\tGE1:GPIO\n");
		gpiomode |= RALINK_GPIOMODE_GE1;
	}

	if (board_gpio_mode->ge2)
	{
		GPIO_DBGPRINT("\tGE2:GPIO\n");
		gpiomode |= RALINK_GPIOMODE_GE2;
	}

	if (board_gpio_mode->lana)
	{
		GPIO_DBGPRINT("\tLAN_A:GPIO\n");
		gpiomode |= RALINK_GPIOMODE_LNA_A;
	}

	if (board_gpio_mode->lang)
	{
		GPIO_DBGPRINT("\tLAN_G:GPIO\n");
		gpiomode |= RALINK_GPIOMODE_LNA_G;
	}
#endif

	HAL_REG32(REG_GPIOMODE) = gpiomode;
	GPIO_DBGPRINT("GPIO_MODE_REGs: 0x%x\n",HAL_REG32(REG_GPIOMODE));
}




#if 0
static struct ralink_gpio_data rt305x_gpio_data =
{
	.chips = rt305x_gpio_chips,
	.num_chips = ARRAY_SIZE(rt305x_gpio_chips),
};

static struct ralink_gpio_data rt5350_gpio_data =
{
	.chips = rt5350_gpio_chips,
	.num_chips = ARRAY_SIZE(rt5350_gpio_chips),
};

static struct ralink_gpio_data rt3883_gpio_data =
{
	.chips = rt3883_gpio_chips,
	.num_chips = ARRAY_SIZE(rt3883_gpio_chips),
};

static struct ralink_gpio_data mt7620_gpio_data =
{
	.chips = mt7620_gpio_chips,
	.num_chips = ARRAY_SIZE(mt7620_gpio_chips),
};

static struct ralink_gpio_data mt7621_gpio_data =
{
	.chips = mt7621_gpio_chips,
	.num_chips = ARRAY_SIZE(mt7621_gpio_chips),
};

static struct ralink_gpio_data mt7628_gpio_data =
{
	.chips = mt7628_gpio_chips,
	.num_chips = ARRAY_SIZE(mt7628_gpio_chips),
};

static __init void ralink_gpio_chip_add(struct ralink_gpio_chip *rg)
{
	rg->regs_base = ioremap(rg->map_base, rg->map_size);
	//     printf("%s:maped at address:0x%p.\n",rg->chip.label, rg->regs_base);
	rg->chip.direction_input = ralink_gpio_direction_input;
	rg->chip.direction_output = ralink_gpio_direction_output;
	rg->chip.get = ralink_gpio_get;
	rg->chip.set = ralink_gpio_set;
// 	rg->chip.request = ralink_gpio_request;
// 	rg->chip.free = ralink_gpio_free;
// 	rg->chip.to_irq = ralink_gpio_to_irq;

	/* set polarity to low for all lines */
	ralink_gpio_wr(rg, RALINK_GPIO_REG_POL, 0);
}

int ralink_gpio_init(struct ralink_gpio_data *data)
{
	int i;

	for (i = 0; i < data->num_chips; i++)
		ralink_gpio_chip_add(&data->chips[i]);

	return 0;
}


int  ralink_gpiolib_init(void)
{
	printk("Generic GPIO Driver  2013-2017 PandoraBox Team \n");

#if defined (CONFIG_RALINK_RT3052)
	ralink_gpio_init(&rt305x_gpio_data);
#elif defined (CONFIG_RALINK_RT3352)
	ralink_gpio_init(&rt5350_gpio_data);
#elif defined (CONFIG_RALINK_RT3883)
	ralink_gpio_init(&rt3883_gpio_data);
#elif defined (CONFIG_RALINK_MT7620)
	ralink_gpio_init(&mt7620_gpio_data);
#elif defined (CONFIG_RALINK_MT7621)
	ralink_gpio_init(&mt7621_gpio_data);
#elif defined (CONFIG_RALINK_MT7628)
	ralink_gpio_init(&mt7628_gpio_data);
#else
	printf("Unknow CPU types for GPIOlib.\n");
#endif

	return 0;
}
#endif
