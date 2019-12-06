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
    watchdog

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/
//==============================================================================
//                                INCLUDE FILES
//==============================================================================

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>     /* For dial_printf */
#include <cyg/hal/hal_if.h>     /* For CYGACC_CALL_IF_DELAY_US */

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
#if defined CONFIG_HARDWARE_WATCHDOG_INTERVAL
#define WATCHDOG_TIMEOUT CONFIG_HARDWARE_WATCHDOG_INTERVAL
#else
#define WATCHDOG_TIMEOUT 30 /* 30 sec default timeout */
#endif

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

enum timer_mode {
    FREE_RUNNING,
    PERIODIC,
    TIMEOUT,
    WATCHDOG
};

enum timer_clock_freq {
    SYS_CLK,          /* System clock     */
    SYS_CLK_DIV4,     /* System clock /4  */
    SYS_CLK_DIV8,     /* System clock /8  */
    SYS_CLK_DIV16,    /* System clock /16 */
    SYS_CLK_DIV32,    /* System clock /32 */
    SYS_CLK_DIV64,    /* System clock /64 */
    SYS_CLK_DIV128,   /* System clock /128 */
    SYS_CLK_DIV256,   /* System clock /256 */
    SYS_CLK_DIV512,   /* System clock /512 */
    SYS_CLK_DIV1024,  /* System clock /1024 */
    SYS_CLK_DIV2048,  /* System clock /2048 */
    SYS_CLK_DIV4096,  /* System clock /4096 */
    SYS_CLK_DIV8192,  /* System clock /8192 */
    SYS_CLK_DIV16384, /* System clock /16384 */
    SYS_CLK_DIV32768, /* System clock /32768 */
    SYS_CLK_DIV65536  /* System clock /65536 */
};

static cyg_handle_t watchdog_thread_h;
static cyg_thread watchdog_thread;
static char watchdog_stack[1024];
static int WdgLoadValue;

#ifdef CONFIG_MT7628_ASIC
void SetWdgTimerClock(int prescale)
{
	unsigned int result;
	result=HAL_REG32(TMRSTAT + 0x20);
	result &= 0x0000FFFF;
	result |= (prescale << 16); //unit = 1u
	HAL_REG32(TMRSTAT + 0x20)= result;
}
static void RaWdgStart(void)
{
    SetWdgTimerClock(1000); // 1 msec
    WdgLoadValue = WATCHDOG_TIMEOUT * 1000;
	HAL_REG32(TMRSTAT + 0x24)=WdgLoadValue;
	 SetWdgTimerEbl(TMRSTAT + 0x20,1);
}
#else
void SetTimerMode(unsigned int timer, enum timer_mode mode)
{
	unsigned int result;

	result = HAL_REG32(timer);
	result &= ~(0x3<<4); //watchdog mode
	result = result | (mode << 4);
	HAL_REG32(timer) = result;
}


void SetWdgTimerClock(unsigned int timer, enum timer_clock_freq prescale)
{
	unsigned int result;

	result = HAL_REG32(timer);
	result &= ~0xF;
	result = result | (prescale & 0xF);
	HAL_REG32(timer) = result;
}
#endif

void SetWdgTimerEbl(unsigned int timer, unsigned int ebl)
{
	unsigned int result;

	result = HAL_REG32(timer);

	if (ebl == 1) {
		result |= (1<<7);
	} else {
		result &= ~(1<<7);
	}

	HAL_REG32(timer) = result;

#if defined (CONFIG_RT3050_ASIC) || defined (CONFIG_RT3052_ASIC)
        if(timer == TMR1CTL) {
                result = HAL_REG32(SYSCFG);

                if(ebl == 1){
                        result |= (1<<2); /* SRAM_CS_MODE is used as wdg reset */
                }else {
                        result &= ~(1<<2);
                }

                HAL_REG32(SYSCFG) = result;
        }
#elif defined (CONFIG_RT3352_ASIC) || defined (CONFIG_RT5350_ASIC)
        if(timer == TMR1CTL) {
                result = HAL_REG32(GPIOMODE);

                if(ebl == 1){
                        result |= (0x1<<21); /* SPI_CS1 as watch dog reset */
                }else {
                        result &= ~(0x1<<21);
                }

                HAL_REG32(GPIOMODE) = result;
        }
#endif
        
}


int Watchdog_init()
{
	/*
	 * For user easy configuration, We assume the unit of watch dog timer is 1s,
	 * so we need to calculate the TMR1LOAD value.
	 *
	 * Unit= 1/(SysClk/65536), 1 Sec = (SysClk)/65536
	 *
	 */
#ifdef CONFIG_MT7628_ASIC
	RaWdgStart();
#else
	SetTimerMode(TMR1CTL,WATCHDOG);
	SetWdgTimerClock(TMR1CTL,SYS_CLK_DIV65536);

#if defined (CONFIG_RT3050_ASIC)
	WdgLoadValue = WATCHDOG_TIMEOUT * ((CYGHWR_HAL_MIPS_RA305X_CPU_CLOCK*3)/65536);
#else
	WdgLoadValue = WATCHDOG_TIMEOUT * ((CYGHWR_HAL_MIPS_RA305X_CPU_CLOCK/10)/65536); //fixed at 40Mhz
#endif
	HAL_REG32(TMR1LOAD) = WATCHDOG_TIMEOUT * ((CYGHWR_HAL_MIPS_RA305X_CPU_CLOCK*10)/65536);;

	SetWdgTimerEbl(TMR1CTL,1);
 #endif /* end of CONFIG_MT7628_ASIC */
	diag_printf("\nStarted WatchDog Timer\n");

	return 0;
}

void Watchdog_proc()
{
	unsigned int value=0;
	while(1)
	{
#ifdef CONFIG_MT7628_ASIC
		value = HAL_REG32(TMRSTAT);
		HAL_REG32(TMRSTAT) |=(1 << 9); //reset watchdog; using this
#else
		HAL_REG32(TMR1LOAD) = WdgLoadValue;
#endif

		cyg_thread_delay((WATCHDOG_TIMEOUT/4)*100); //unit: ticks
	}
}

void Watchdog_start()
{
	if (Watchdog_init() == 0)
	{
		cyg_thread_create(1,(cyg_thread_entry_t *)Watchdog_proc,NULL,"Watchdog_thread",
			(void *)&watchdog_stack[0],sizeof(watchdog_stack), &watchdog_thread_h, &watchdog_thread);

		printf("setup Watchdog=%d\n", WATCHDOG_TIMEOUT);
		cyg_thread_resume(watchdog_thread_h);
	}
}



