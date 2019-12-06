#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h> //For dial_printf
#include <cyg/hal/hal_if.h>  //For CYGACC_CALL_IF_DELAY_US
#include <cyg/hal/hal_intr.h> //For HAL_DELAY_US(n)   void hal_delay_us(int us);
#include <led.h>
#include <gpio.h>

#include <sys/param.h>

int sys_led_on;
static struct gpio_led sys_led;
static int led_blink_time=60;

int led_start(void)
{
	return 0;
}

static void led_tmr_func(void *data)
{
//	diag_printf("SYS LED at GPIO%d = %d  \n",sys_led.gpio,sys_led_on);
	if(sys_led_on==0)
	{
		gpio_set(sys_led.gpio,sys_led_on);
		sys_led_on = 1;
	}
	else
	{
		gpio_set(sys_led.gpio,sys_led_on);
		sys_led_on = 0;
	}
 	timeout(&led_tmr_func, 0, led_blink_time);
}

int led_init(struct gpio_leds *leds)
{ 
	int i, value;
//	LED_DBGPRINT("[%d] leds will be Registered.\n",leds->led_num);

	for (i=0;i<leds->led_num;i++)
	{
		LED_DBGPRINT("Registered [%s] at GPIO%d \n", leds->led[i].name, leds->led[i].gpio);

		if(leds->led[i].active_low == 1)
			value = 0;
		else
			value=1;

		switch(leds->led[i].default_status)
		{
			case LED_OFF:
				gpio_direction_output(leds->led[i].gpio);
				gpio_set(leds->led[i].gpio,!value);
				break;
			case LED_ON:
				gpio_direction_output(leds->led[i].gpio);
				gpio_set(leds->led[i].gpio,value);
				break;
			case LED_SYS:
				sys_led = leds->led[i];
				gpio_direction_output(sys_led.gpio);
				timeout(&led_tmr_func, 0, led_blink_time);
				break;
			case LED_WPS:
				break;
			default:
				LED_DBGPRINT("Unknow LEDs status %d!!!\n");
				break;
		}
	}
// 	LED_DBGPRINT("Registered [%s] at GPIO%d \n",leds->name,leds->gpio);
// 	sys_led.gpio = leds->gpio;
// 	sys_led.default_status =leds->default_status; /* ledon=1; ledoff=0; led_sys=2;led_wps=3 */
// 	sys_led.active_low =leds->active_low;
// 	sys_led.locked =leds->locked;
//
// 	timeout(&led_tmr_func, 0, led_blink_time);
}


#if 0
void led_thread_init(void)
{
	cyg_handle_t handle;
	struct gpio_led  *leds=&sys_led;
	//				entry   入口参数  Name    Stack   Size         Handle           进程数据结构
	cyg_thread_create(15, led_thread, (cyg_addrword_t) leds,"led_thread", (void *) stack[0],STACKSIZE, &thread[0], &thread_obj[0]);
	cyg_thread_resume(thread[0]);
}

#define NTHREADS 1
#define STACKSIZE 4096
static cyg_handle_t thread[NTHREADS];
static cyg_thread thread_obj[NTHREADS];
static char stack[NTHREADS][STACKSIZE];

static void led_thread(struct gpio_led *leds)
{
	cyg_thread_exit();
}
#endif
