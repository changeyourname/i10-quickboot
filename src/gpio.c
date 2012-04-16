
#include <types.h>
#include <common.h>
#include <io.h>
#include <hardware.h>

#define GPIOx_BASE_ADDR(x)			(GPIO0_BASE_ADDR + ((0x8000) * (x)))
#define BIT(i)      (1 << (i))


#define GPIO_DATA		(0x00)
#define GPIO_DIR		(0x04)
#define GPIO_CTRL		(0x08)
#define GPIO_EXT		(0x0c)

static uint32_t gpio_bank_state[6] = {0};
static enum clock_id gpio_bank_clkid[6] = { CLK_GPIO0, CLK_GPIO1, CLK_GPIO2, CLK_GPIO3, CLK_GPIO4, CLK_GPIO5};

int gpio_get_value(unsigned gpio)
{
	unsigned long base = GPIOx_BASE_ADDR(gpio>>5);
	unsigned long bitmask = BIT(gpio & 0x1f);

	return !!(GET_REG(base + GPIO_EXT) & bitmask);
}

int gpio_set_value(unsigned gpio, int value)
{
	unsigned long base = GPIOx_BASE_ADDR(gpio>>5);
	unsigned long bitmask = BIT(gpio & 0x1f);

	if (value)
		SET_REG_BITS(base + GPIO_DATA, bitmask);		// set gpio high
	else 
		CLEAR_REG_BITS(base + GPIO_DATA, bitmask);	// set gpio low

	return 0;
}

static void set_gpio_direction(unsigned gpio, int output)
{
	unsigned long base = GPIOx_BASE_ADDR(gpio>>5);
	unsigned long bitmask = BIT(gpio & 0x1f);

	CLEAR_REG_BITS(base + GPIO_CTRL, bitmask);	// set gpio in software mode

	if (output) {
		SET_REG_BITS(base + GPIO_DIR, bitmask);	// set gpio direction output
	} else {
		CLEAR_REG_BITS(base + GPIO_DIR, bitmask);	// set gpio direction input
	}
}

int gpio_direction_input(unsigned gpio)
{
	set_gpio_direction(gpio, 0);

	return 0;
}

int gpio_direction_output(unsigned gpio, int value)
{
	gpio_set_value(gpio, value);

	set_gpio_direction(gpio, 1);

	return 0;
}

int gpio_request(unsigned gpio)
{
	unsigned bank = gpio >> 5;

	if (!gpio_bank_state[bank]) {
		clock_switch(gpio_bank_clkid[bank], 1);
	}

	gpio_bank_state[bank] |= BIT(gpio & 0x1f);	
	return 0;
}

int gpio_free(unsigned gpio)
{
	unsigned bank = gpio >> 5;

	gpio_bank_state[bank] &= ~BIT(gpio & 0x1f);

	if (!gpio_bank_state[bank])
		clock_switch(gpio_bank_clkid[bank], 0);

	return 0;
}
