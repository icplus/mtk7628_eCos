
#ifndef BUTTON_H
#define BUTTON_H

#include <cyg/kernel/kapi.h>

#define BUTTON_DBGPRINT(fmt, args...)   \
{  \
	diag_printf("[BUTTON]"); \
	diag_printf( fmt, ## args); \
}

#ifndef HZ
#define HZ		100
#endif

#define FACTORY_RESET_TIME	15	/*  3.2 sec  */

typedef struct gpio_button
{
	const char    *name;
	unsigned short gpio;
	unsigned long time; /* chech time */
	unsigned short status; /* pressed =1; not pressed=0; */
}BUTTON;


void init_reset_button(struct gpio_button *buttons);
void enable_reset_button(int gpio);
int is_button_press(void);
void swrst_chk(void);

#endif /* BUTTON_H */
