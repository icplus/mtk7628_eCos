#ifndef __GPIO_H__
#define __GPIO_H__

#include <rt_mmap.h>

#define GPIO_DBGPRINT(fmt, args...)  \
{  \
	diag_printf("[GPIO]"); \
	diag_printf( fmt, ## args); \
}

typedef unsigned char 		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;

#define ARRAY_SIZE(arr) ( sizeof(arr) / sizeof((arr)[0]) )

/* Copy From Linux (delete something part) */
enum ralink_gpio_reg {
	RALINK_GPIO_REG_INT = 0,	/* Interrupt status */
	RALINK_GPIO_REG_EDGE,
	RALINK_GPIO_REG_RENA,
	RALINK_GPIO_REG_FENA,
	RALINK_GPIO_REG_DATA,
	RALINK_GPIO_REG_DIR,		/* Direction, 0:in, 1: out */
	RALINK_GPIO_REG_POL,		/* Polarity, 0: normal, 1: invert */
	RALINK_GPIO_REG_SET,
	RALINK_GPIO_REG_RESET,
	RALINK_GPIO_REG_TOGGLE,
	RALINK_GPIO_REG_MAX
};


struct gpio_chip {
	const char	*label;
	int			base;
	u16			ngpio;
// 	int			(*direction_input)(struct gpio_chip *chip,
// 						unsigned offset);
// 	int			(*direction_output)(struct gpio_chip *chip,
// 						unsigned offset, int value);
// 	int			(*get)(struct gpio_chip *chip,
// 						unsigned offset);
// 	void			(*set)(struct gpio_chip *chip,
// 						unsigned offset, int value);
};



struct ralink_gpio_chip {
	struct gpio_chip chip;

	unsigned long map_base;
	unsigned long map_size;

	u8 regs[RALINK_GPIO_REG_MAX];
};

struct ralink_gpio_data {
	unsigned int num_chips;
	struct ralink_gpio_chip *chips;
};


void gpio_direction_input(unsigned int pin);
void gpio_direction_output(unsigned int pin);

int gpio_get(unsigned int pin);
void gpio_set(unsigned int pin, int value);



//========================================================================
//  gpio mode
#if defined (CONFIG_RALINK_MT7620)

#define RALINK_GPIOMODE_DFT 			0x1C
#define RALINK_GPIOMODE_I2C				0x01
#define RALINK_GPIOMODE_SPI				0x02
#define RALINK_GPIOMODE_SPI_MT7620			0x800
#define RALINK_GPIOMODE_UARTF				0x1C
#define RALINK_GPIOMODE_UARTL				0x20
#define RALINK_GPIOMODE_JTAG				0x40
#define RALINK_GPIOMODE_MDIO				0x80
#define RALINK_GPIOMODE_MDIO_MT7620			0x180
#define RALINK_GPIOMODE_EPHY				0x4000
#define RALINK_GPIOMODE_EPHY_RT3883_MT7620		0x8000
#define RALINK_GPIOMODE_SPI_CS1			0x400000
#define RALINK_GPIOMODE_GE1				0x200
#define RALINK_GPIOMODE_GE2				0x400
#define RALINK_GPIOMODE_LNA_A				0x30000
#define RALINK_GPIOMODE_LNA_G				0xC0000
#define RALINK_GPIOMODE_WLED				0x2000
#define RALINK_GPIOMODE_PERST				0x20000
#define RALINK_GPIOMODE_ND_SD				0x80000
#define RALINK_GPIOMODE_PA_G				0x100000
#define RALINK_GPIOMODE_WDT				0x400000
#define RALINK_GPIOMODE_SPI_REFCLK			0x1000

#elif defined (CONFIG_RALINK_MT7628)

#define RALINK_GPIOMODE_DFT 			0x1C
#define RALINK_GPIOMODE_PWM1		(1 << 30)
#define RALINK_GPIOMODE_PWM0		(1 << 28)
#define RALINK_GPIOMODE_UART2		(1 << 26)
#define RALINK_GPIOMODE_UART1		(1 << 24)
#define RALINK_GPIOMODE_I2C			(1 << 20)
#define RALINK_GPIOMODE_REFCLK		(1 << 18)
#define RALINK_GPIOMODE_PERST		(1 << 16)
#define RALINK_GPIOMODE_WDT		(1 << 14)
#define RALINK_GPIOMODE_SPI			(1 << 12)
#define RALINK_GPIOMODE_SD			(1 << 10)
#define RALINK_GPIOMODE_UART0		(1 << 8)
#define RALINK_GPIOMODE_I2S			(1 << 6)
#define RALINK_GPIOMODE_SPI_CS1		(1 << 4)
#define RALINK_GPIOMODE_SPIS		(1 << 2)
#define RALINK_GPIOMODE_GPIO		(1 << 0)
/* mt7628kn only */
#define RALINK_GPIOMODE_P4_LED_KN	(1 << 26)
#define RALINK_GPIOMODE_P3_LED_KN	(1 << 24)
#define RALINK_GPIOMODE_P2_LED_KN	(1 << 22)
#define RALINK_GPIOMODE_P1_LED_KN	(1 << 20)
#define RALINK_GPIOMODE_P0_LED_KN	(1 << 18)
#define RALINK_GPIOMODE_WLED_KN	(1 << 16)
/* mt7628an only */
#define RALINK_GPIOMODE_P4_LED_AN	(1 << 10)
#define RALINK_GPIOMODE_P3_LED_KN	(1 << 8)
#define RALINK_GPIOMODE_P2_LED_KN	(1 << 6)
#define RALINK_GPIOMODE_P1_LED_KN	(1 << 4)
#define RALINK_GPIOMODE_P0_LED_KN	(1 << 2)
#define RALINK_GPIOMODE_WLED_AN	(1 << 0)

#endif


struct ralink_gpio_mode
{
//	char chip[12],
//	char board[20],
	bool i2c;
	bool spi;
	bool spi_refclk;
	bool uartf;
	bool uartl;
	bool jtag;
	bool mdio;
	bool ephy;
	bool ge1;
	bool ge2;
	bool pci;
	bool lana;
	bool lang;
	bool pa;
};

void gpio_mode_init(struct ralink_gpio_mode *board_gpio_mode);

#endif /* __GPIO_H__ */
