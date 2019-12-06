#ifndef BOARDS_H
#define BOARDS_H

#include <gpio.h>
#include <led.h>
#include <button.h>

struct ecos_boards
{
	const char *board_name;
	char *cpu_name;
	int board_id;
	unsigned int cpu_freq;
	unsigned int mem_size;
	unsigned int flash_size;
	struct ralink_gpio_mode *gpio_mode;
	struct gpio_leds *leds;
	struct gpio_button *buttons;
};

struct ecos_boards *board;

#endif /* BOARDS_H */
