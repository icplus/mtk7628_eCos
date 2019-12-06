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
    leddrv.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <stdio.h>
#include <cyg/kernel/kapi.h>
#include <network.h>
#include <cfg_net.h>
#include <led.h>

//==============================================================================
//                                    MACROS
//==============================================================================


extern	int	GPIO_LED_NUM;
extern	int led_gpio[];
extern	LED_STAT led[];


//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================
static void led_tmr_func(void * data);
static void led_set_timeout(void);

static void led_tmr_func(void * data)
{
	int i;
	cyg_tick_count_t ct;
	
	LED_STAT * lp;
	
	
	ct=cyg_current_time();
	
	for (i=0;i<GPIO_LED_NUM; i++)
	{
		lp=&led[i];
		
		if (( ct - lp->last_tick ) >= lp->period)
		{
			if (lp->stat == LED_BLINK)
			{
				DO_LED_TOGGLE(lp);
			}
			else
			{
				lp->period = LED_MAX_PERIOD; // don't check next time
				if (lp->stat == LED_ON)
				{
					DO_LED_ON(lp);
				}
				else
				{
					DO_LED_OFF(lp);
				}
			}
			
			lp->last_tick = ct;
		}
	}
	
	led_set_timeout();
}

static void led_set_timeout(void)
{
	int i;
	LED_STAT * lp;
	int min_p=LED_MAX_PERIOD;

	for (i=0;i<GPIO_LED_NUM; i++)
	{
		lp=&led[i];
		if ( lp->period < min_p )
			min_p = lp->period ;
	}
	
	untimeout(&led_tmr_func, 0 );
	timeout(&led_tmr_func, 0, min_p);
}


//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
int LED_set(int id, int stat, int period)
{
	LED_STAT *lp;
	cyg_tick_count_t ct;
	short np = LED_MAX_PERIOD;
	
	if (id >= GPIO_LED_NUM )
		return -1;
	
	lp=&led[id];
	ct=cyg_current_time();
	switch (stat)
	{
	case LED_OFF: // steady off
		DO_LED_OFF(lp);
		break;
		
	case LED_ON: // steady on
		DO_LED_ON(lp);
		break;
		
	case LED_BLINK:
		if (period < LED_MIN_PERIOD)
			period=LED_MIN_PERIOD;
		
		np = period/2 ;
		DO_LED_ON(lp);
		break;
		
	case LED_TOGGLE:
		if ( (lp->stat==LED_BLINK) || ( ct - lp->last_tick) < (LED_MIN_PERIOD/2) )
			return 0; // blink state, skip
		
		np = LED_RECHECK_PERIOD;
		DO_LED_TOGGLE(lp);
		break;
		
	case LED_OFF2:
		DO_LED_OFF(lp);
		return 0;
		
	case LED_ON2:
		DO_LED_ON(lp);
		return 0;
		
	case LED_TOGGLE2:
		DO_LED_TOGGLE(lp);
		return 0;
		
	default:
		break;
	}
	
	lp->last_tick = ct ;
	if (LED_TOGGLE != stat)
		lp->stat = stat;
	
	if (np != lp->period)
	{
		lp->period = np;
		led_set_timeout();
	}
	
	return 0;
}


int LED_init(void)
{
	int i;
	LED_STAT * lp;

	for (i=0; i<GPIO_LED_NUM; i++)
	{
		lp = &led[i];
		memset(lp, 0, sizeof(LED_STAT));
		
		lp->gpio = led_gpio[i];
		lp->period = LED_MAX_PERIOD;

		CONFIG_GPIO_LED(lp);
		DO_LED_OFF(lp);
		ENABLE_GPIO_LED(lp);
	}
	
	return 0;
}



