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
    cmd_os.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#include <pkgconf/system.h>
#include <cyg/kernel/kapi.h>
#include <cyg/io/flash.h>
#include <cyg/infra/diag.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <cli_cmd.h>



static cyg_interrupt rtmp_timer0_interrupt;
static cyg_handle_t  rtmp_timer0_interrupt_handle;

static cyg_interrupt rtmp_timer1_interrupt;
static cyg_handle_t  rtmp_timer1_interrupt_handle;

static cyg_interrupt rtmp_wdt_interrupt;
static cyg_handle_t  rtmp_wdt_interrupt_handle;


static int dvt_cmd_init(int argc, char* argv[]);

static int dvt_timer_cmd(int argc , char* argv[]);


#ifdef CONFIG_CLI_NET_CMD
#define CLI_OSCMD_FLAG			0
#else
#define CLI_OSCMD_FLAG 		CLI_CMD_FLAG_HIDE
#endif

char timer_thread [] = {"timer 0/1/2/Wdt/All/Stop"};

stCLI_cmd dvt_cmds[] =
{
	{ "DVT" , &dvt_cmd_init, 0, (CLI_CMD_FLAG_HIDE|CLI_CMD_FLAG_FIRST), CLI_ROOT_CMD, 0 },
	{ "timer" , dvt_timer_cmd, &timer_thread, CLI_OSCMD_FLAG, 0, 0 },
	/*{ "i2c" , os_show_mem, "i2c dvt", CLI_OSCMD_FLAG, 0, 0 },
	{ "spiS" , os_spi_cmd, "SPI <rd/wr/er> <addr> [len]", CLI_OSCMD_FLAG, 0, 0 },	
	{ "gpio" , os_register_cmd, "Read/Write Register", CLI_OSCMD_FLAG, 0, 0 },*/
};

static int dvt_cmd_init(int argc, char* argv[])
{
	// default cmds
	CLI_init_cmd(&dvt_cmds[0], CLI_NUM_OF_CMDS(dvt_cmds) );	

	return 0;
}


// Serial I/O - low level interrupt handler (ISR)
static cyg_uint32 ra305x_timer0_isr(cyg_vector_t vector, cyg_addrword_t data)
{
    cyg_interrupt_mask(CYGNUM_HAL_INTERRUPT_TIMER0);
    cyg_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_TIMER0);
    return CYG_ISR_CALL_DSR;  // Cause DSR to be run
}


// Serial I/O - high level interrupt handler (DSR)
static void ra305x_timer0_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    cyg_uint32 iir;

    iir = *((unsigned int*)0xB0000100);
	if(iir & 0x1)
	{
		*((unsigned int*)0xB0000100) |= 0x1;	
		diag_printf("timer0 out\n");
		
	}
    
    cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_TIMER0);
	
}


// Function to initialize the device.  Called at bootstrap time.
static bool ra305x_timer0_init(void)
{

//#ifdef CYGDBG_IO_INIT
    diag_printf("ra305x_timer0_init\n");
//#endif

    cyg_interrupt_create(CYGNUM_HAL_INTERRUPT_TIMER0,
                             0,         // can change IRQ0 priority
                             NULL,   //  Data item passed to interrupt handler
                             ra305x_timer0_isr,
                             ra305x_timer0_dsr,
                             &rtmp_timer0_interrupt_handle,
                             &rtmp_timer0_interrupt);
    cyg_interrupt_attach(rtmp_timer0_interrupt_handle);
    cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_TIMER0);

    return true;
}

// Serial I/O - low level interrupt handler (ISR)
static cyg_uint32 ra305x_timer1_isr(cyg_vector_t vector, cyg_addrword_t data)
{
    cyg_interrupt_mask(CYGNUM_HAL_INTERRUPT_TIMER1);
    cyg_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_TIMER1);
    return CYG_ISR_CALL_DSR;  // Cause DSR to be run
}


// Serial I/O - high level interrupt handler (DSR)
static void ra305x_timer1_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    cyg_uint32 iir;

    iir = *((unsigned int*)0xB0000100);
	if(iir & 0x4)
	{
		*((unsigned int*)0xB0000100) |= 0x4;	
		diag_printf("timer1 out\n");
		
	}
    
    cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_TIMER1);
	
}


// Function to initialize the device.  Called at bootstrap time.
static bool ra305x_timer1_init(void)
{

//#ifdef CYGDBG_IO_INIT
    diag_printf("ra305x_timer1_init\n");
//#endif

    cyg_interrupt_create(CYGNUM_HAL_INTERRUPT_TIMER1,
                             0,         // can change IRQ0 priority
                             NULL,   //  Data item passed to interrupt handler
                             ra305x_timer1_isr,
                             ra305x_timer1_dsr,
                             &rtmp_timer1_interrupt_handle,
                             &rtmp_timer1_interrupt);
    cyg_interrupt_attach(rtmp_timer1_interrupt_handle);
    cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_TIMER1);

    return true;
}

// Serial I/O - low level interrupt handler (ISR)
static cyg_uint32 ra305x_wdt_isr(cyg_vector_t vector, cyg_addrword_t data)
{
    cyg_interrupt_mask(CYGNUM_HAL_INTERRUPT_WDTIMER);
    cyg_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_WDTIMER);
    return CYG_ISR_CALL_DSR;  // Cause DSR to be run
}


// Serial I/O - high level interrupt handler (DSR)
static void ra305x_wdt_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    cyg_uint32 iir;

    iir = *((unsigned int*)0xB0000100);
	if(iir & 0x2)
	{
		*((unsigned int*)0xB0000100) |= 0x2;	
		diag_printf("wdt out\n");
		
	}
    
    cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_WDTIMER);
	
}


// Function to initialize the device.  Called at bootstrap time.
static bool ra305x_wdt_init(void)
{

//#ifdef CYGDBG_IO_INIT
    diag_printf("ra305x_wdttimer_init\n");
//#endif

    cyg_interrupt_create(CYGNUM_HAL_INTERRUPT_WDTIMER,
                             0,         // can change IRQ0 priority
                             NULL,   //  Data item passed to interrupt handler
                             ra305x_wdt_isr,
                             ra305x_wdt_dsr,
                             &rtmp_wdt_interrupt_handle,
                             &rtmp_wdt_interrupt);
    cyg_interrupt_attach(rtmp_wdt_interrupt_handle);
    cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_WDTIMER);

    return true;
}


static bool ra305x_timer_stop(void)
{
	*((unsigned int*)0xB0000110) &= ~(0x1<<7);//stop timer0
	*((unsigned int*)0xB0000120) &= ~(0x1<<7);//stop timer0
	*((unsigned int*)0xB0000130) &= ~(0x1<<7);//stop timer0
}

void timer0_test(void)
{
	unsigned int result=0;
	//set timer
	*((unsigned int*)0xB0000110) = (0x1<<4) | ((1000)<<16);//reset timter0 load LMT value 
	*((unsigned int*)0xB0000114) |= 1000*10;//reset timter0 load LMT value 	
	//set dsr
	ra305x_timer0_init();
	//start timer
	*((unsigned int*)0xB0000100) |= 0x1<<8;//reset timter0 load LMT value 
	result = *((unsigned int*)0xB0000110);
	result |= (0x1<<7) | (0x1<<4);//start timer
	*((unsigned int*)0xB0000110) = result;//start timer

}

void timer1_test(void)
{
	unsigned int result=0;
	//set timer
	*((unsigned int*)0xB0000130) = (0x1<<4) | ((1000)<<16);//reset timter0 load LMT value 
	*((unsigned int*)0xB0000134) |= 1000*10;//reset timter0 load LMT value 	
	//set dsr
	ra305x_timer1_init();
	//start timer
	*((unsigned int*)0xB0000100) |= 0x1<<10;//reset timter0 load LMT value 
	result = *((unsigned int*)0xB0000130);
	result |= (0x1<<7) | (0x1<<4);//start timer
	*((unsigned int*)0xB0000130) = result;//start timer

}
void wdttimer_test(void)
{
	unsigned int result=0;
	//set timer
	*((unsigned int*)0xB0000120) = (0x1<<4) | ((1000)<<16);//reset timter0 load LMT value 
	*((unsigned int*)0xB0000124) |= 1000*10;//reset timter0 load LMT value 	
	//set dsr
	ra305x_wdt_init();
	//start timer
	*((unsigned int*)0xB0000100) |= 0x1<<9;//reset timter0 load LMT value 
	result = *((unsigned int*)0xB0000120);
	result |= (0x1<<7) | (0x1<<4);//start timer
	*((unsigned int*)0xB0000120) = result;//start timer

}


static int dvt_timer_cmd(int argc , char* argv[])
{
        /* Get the thread id from the hidden control */
        cyg_handle_t thread = 0;
        cyg_uint16 id;


        if ((argc==0))
        {
            diag_printf("timer 0/1/Wdt/All/Stop\n");
            return 0;
        }

        if (argc == 1)
        {
            if (!strcmp(argv[0], "0"))
            {
            	timer0_test();
                 return 0;
            }
            if (!strcmp(argv[0], "Stop"))
            {
            	ra305x_timer_stop();
                 return 0;
            }
            if (!strcmp(argv[0], "1"))
            {
            	timer1_test();
                 return 0;
            }
            if (!strcmp(argv[0], "Wdt"))
            {
            	wdttimer_test();
                 return 0;
            }			
            if (!strcmp(argv[0], "All"))
            {
            	timer0_test();
				timer1_test();
				wdttimer_test();
                 return 0;
            }				
        }
help:
		diag_printf("timer 0/1/Wdt/All/Stop\n");

        return 0;
}

