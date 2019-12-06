#include <stdio.h>
#include <config.h>
#include <cyg/kernel/kapi.h>
#include <cfg_def.h>
#include <network.h>
#include <cfg_net.h>
#include<gpio.h>
#include<button.h>


//==============================================================================
//                                    MACROS
//==============================================================================

#ifdef	CONFIG_RST
static int reset_button_gpio;
static int button_check_time;
extern	int	IS_SWRST_ON();
extern	void CONFIG_SWRST();
extern	void ENABLE_SWRST();

void init_reset_button(struct gpio_button *buttons)
{
	reset_button_gpio=buttons->gpio;
	button_check_time=buttons->time;
	BUTTON_DBGPRINT("Registered [%s] at GPIO%d, timer=%dms\n ",buttons->name,reset_button_gpio,button_check_time);

	enable_reset_button(reset_button_gpio);
	timeout(&swrst_chk, 0, button_check_time);
}

void enable_reset_button(int gpio)
{
	gpio_direction_input(gpio);

	if(!gpio_get(gpio))
		{BUTTON_DBGPRINT("Enable GPIO%d input fail!!!\n",gpio);}
}

int is_button_press(void)
{
	return gpio_get(reset_button_gpio);
}
#endif	// CONFIG_RST



#ifndef HZ
#define HZ		100
#endif

#define SWRST_CHECK_TIME		(4*HZ/10)		/*  0.4 Sec*/
#define FACTORY_RESET_TIME		((32*HZ/10)/SWRST_CHECK_TIME)	/*  3.2 sec  */
//==============================================================================
//                               LOCAL VARIABLES   mjh
//==============================================================================
#define WATCHDOG_TIMEOUT 30 /* 30 sec default timeout */
#define RALINK_SYSCTL_BASE 0xB0000000
#define RALINK_TIMER_BASE 0xB0000100
#define TMRSTAT     (RALINK_TIMER_BASE)  /* Timer Status Register */
#define TMR0LOAD    (TMRSTAT + 0x10)  /* Timer0 Load Value */
#define TMR0VAL     (TMRSTAT + 0x14)  /* Timer0 Counter Value */
#define TMR0CTL     (TMRSTAT + 0x18)  /* Timer0 Control */
#define TMR1LOAD    (TMRSTAT + 0x20)  /* Timer1 Load Value */
#define TMR1VAL     (TMRSTAT + 0x24)  /* Timer1 Counter Value */
#define TMR1CTL     (TMRSTAT + 0x28)  /* Timer1 Control */
#define SYSCFG      RALINK_SYSCTL_BASE + 0x10  /* System Configuration Register */
#define GPIOMODE    RALINK_SYSCTL_BASE + 0x60
#define RALINK_REG_GPIOMODE2		(GPIOMODE + 0x4)  //7628

//mjh from 7628 sdk
#define RALINK_IRQ_ADDR			RALINK_INTCL_BASE
#define RALINK_PRGIO_ADDR		RALINK_PIO_BASE

//search in rt_mmap.h pwd : sdk asm/rt2880
#define RALINK_INTCL_BASE		0xB0000200
#define RALINK_PIO_BASE			0xB0000600


#define RALINK_REG_INTENA		(RALINK_IRQ_ADDR   + 0x80)
#define RALINK_REG_INTDIS		(RALINK_IRQ_ADDR   + 0x78)

#define RALINK_REG_PIOINT		(RALINK_PRGIO_ADDR + 0x90)
#define RALINK_REG_PIOEDGE		(RALINK_PRGIO_ADDR + 0xA0)
#define RALINK_REG_PIORENA		(RALINK_PRGIO_ADDR + 0x50)
#define RALINK_REG_PIOFENA		(RALINK_PRGIO_ADDR + 0x60)
#define RALINK_REG_PIODATA		(RALINK_PRGIO_ADDR + 0x20)
#define RALINK_REG_PIODIR		(RALINK_PRGIO_ADDR + 0x00)
#define RALINK_REG_PIOSET		(RALINK_PRGIO_ADDR + 0x30)
#define RALINK_REG_PIORESET		(RALINK_PRGIO_ADDR + 0x40)
//--------------------------------------------------
#define RALINK_REG_PIO6332INT		(RALINK_PRGIO_ADDR + 0x94)
#define RALINK_REG_PIO6332EDGE		(RALINK_PRGIO_ADDR + 0xA4)
#define RALINK_REG_PIO6332RENA		(RALINK_PRGIO_ADDR + 0x54)
#define RALINK_REG_PIO6332FENA		(RALINK_PRGIO_ADDR + 0x64)
#define RALINK_REG_PIO6332DATA		(RALINK_PRGIO_ADDR + 0x24)
#define RALINK_REG_PIO6332DIR		(RALINK_PRGIO_ADDR + 0x04)
#define RALINK_REG_PIO6332SET		(RALINK_PRGIO_ADDR + 0x34)
#define RALINK_REG_PIO6332RESET		(RALINK_PRGIO_ADDR + 0x44)

#define RALINK_REG_PIO9564INT		(RALINK_PRGIO_ADDR + 0x98)
#define RALINK_REG_PIO9564EDGE		(RALINK_PRGIO_ADDR + 0xA8)
#define RALINK_REG_PIO9564RENA		(RALINK_PRGIO_ADDR + 0x58)
#define RALINK_REG_PIO9564FENA		(RALINK_PRGIO_ADDR + 0x68)
#define RALINK_REG_PIO9564DATA		(RALINK_PRGIO_ADDR + 0x28)
#define RALINK_REG_PIO9564DIR		(RALINK_PRGIO_ADDR + 0x08)
#define RALINK_REG_PIO9564SET		(RALINK_PRGIO_ADDR + 0x38)
#define RALINK_REG_PIO9564RESET		(RALINK_PRGIO_ADDR + 0x48)

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================

//kill mjh thread
extern cyg_handle_t led_thread_h;
extern cyg_handle_t wsc_thread_h;

//mjh reset key  gpio45
void CONFIG_SWRST()
{
	unsigned int result;
	printf("reset key init====icplus\n");
	
	result = HAL_REG32(GPIOMODE);     //uart1  gpiomode1
	result &= ~0x3000000;    //clear 0    bit 25 bit 24
	result |= 0x1000000;     //set 1         gpio mode
	HAL_REG32(GPIOMODE) = result;
	
	result = HAL_REG32(RALINK_REG_PIO6332DIR); //45   input
	result &= ~0x1<<(45-32);          //clear 0 input                              
	HAL_REG32(RALINK_REG_PIO6332DIR) = result;

}
int IS_SWRST_ON()
{
	unsigned int result;
	result = HAL_REG32(RALINK_REG_PIO6332DATA);
	result = result>>(45-32);
	result &= ~0xfffffffe;
	if(result == 0)    //press 0
		return 1;
	else
		return 0;
}
//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void swrst_chk(void)
{
	static cyg_uint32 pressed_time=0;
	
	/*  Check reset button  */
	if (IS_SWRST_ON()) {
		
		if (pressed_time>0) {
			//  Reset button released  
			if (pressed_time <= FACTORY_RESET_TIME) {
				diag_printf("SW RESET -> reboot!\n");
			} else {
				CFG_reset_default();
			}
			mon_snd_cmd(MON_CMD_REBOOT);
			pressed_time = 0;
			// Stop checking the reset button and wait for rebooting 
			return;
		}
	}
	else {
		/*  Reset button pressed  */
		pressed_time ++;
		//pressed_time = 0;
		
		if (pressed_time == (FACTORY_RESET_TIME+1)) {
			diag_printf("Factory default!\n");
		}
	}

	/*  Schedule next check*/
	timeout(&swrst_chk, 0, SWRST_CHECK_TIME);
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void swrst_init(void)
{
#ifdef	CONFIG_RST
	CONFIG_SWRST();
	//ENABLE_SWRST();  //mjh 注释掉
#endif
	
	timeout(&swrst_chk, 0, SWRST_CHECK_TIME);
}


