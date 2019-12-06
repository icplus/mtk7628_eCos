#include <config.h>
#include <cyg/kernel/kapi.h>
#include <cfg_def.h>
#include <version.h>
#include <network.h>
#include <sys_status.h>
#include <eventlog.h>
#include <tfq.h>

#include <gpio.h>
#include <led.h>
#include <boards.h>
#include <button.h>
#include <cpuid.h>

#define BOARDS_DBGPRINT(fmt, ...)     do {diag_printf("[Boards]");diag_printf(fmt, ##  __VA_ARGS__);} while (0)
//name gpio  status  active_low  locked
//status: ledon=0; ledoff=1;

/*.default_status  ledon=1; ledoff=0; led_sys=2;led_wps=3 */


static struct gpio_led board_leds[] =
{
	{
		.name			= "Sys",
		.gpio			= 37, //9,
		.default_status	= LED_SYS ,
		.active_low		= 1,
	},
	/*
	{ 
			// WLED_GPIO_MODE default normal, isn't  gpio mode
		.name			= "wifi",
		.gpio			= 72,
		.default_status	= LED_ON ,
		.active_low		= 1,
	},
	{    //usb light
		.name			= "usb_power",
		.gpio			= 52,
		.default_status	= LED_ON ,
		.active_low		= 0,
	}
	*/
};

static struct gpio_leds pandorabox_board_led =
{
	.led	= &board_leds,
	.led_num	= ARRAY_SIZE(board_leds),
};

static struct gpio_button  board_buttons =
{
	.name		= "Reset",
	.gpio		= 45,
	.time		= 10 /* check event 40ms */
};

static struct ralink_gpio_mode board_gpio_mode =
{
	.i2c		= 1,
	.spi		= 0, /* We use SPI flash,so SPI must not be gpio mode */
	.uartf	= 1,
	.uartl	= 0,
	//.jtag		= 1,
	//.mdio	= 1,
	//.spi_refclk = 1,
	//.pa	=1,
};

static struct ecos_boards pandorabox_board =
{
	.board_name	= "PandoraBox-lite W1",
	.cpu_name	= "MediaTek MT7628",
	.board_id	= 0x1,
	.mem_size	= 8192,
	.cpu_freq	= 360000000,
	.gpio_mode 	= &board_gpio_mode,
	.leds	= &pandorabox_board_led,
	.buttons	= &board_buttons
};

struct ecos_boards *board=&pandorabox_board;

int board_init(void)
{
	char id[10] = {0};

	cpu_info();

	BOARDS_DBGPRINT("Board Type : %s\n",board->board_name);
	BOARDS_DBGPRINT("CPU : %s running at %d MHz\n",board->cpu_name,board->cpu_freq/1000/1000);
	BOARDS_DBGPRINT("Memory : %d KByte\n",board->mem_size);
	//       strncpy(Id, *(volatile char *)RALINK_SYSCTL_BASE, 6);
	//       printf("0x%x ",(unsigned long *)RALINK_SYSCTL_BASE);
	//       BOARDS_DBGPRINT("CPU:Ralink %s \n",Id);

	gpio_mode_init(board->gpio_mode);

#ifdef CONFIG_LED
	led_init(board->leds);
#endif

#ifdef CONFIG_RST
	init_reset_button(board->buttons);
#endif
}

