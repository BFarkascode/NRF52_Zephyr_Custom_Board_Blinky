/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   100

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#define BUTTON0_NODE DT_ALIAS(sw0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);					//here we dig up the driver pointer and struct

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON0_NODE, gpios);

/*static struct gpio_callback button_cb_data;													//we define a callback handle

void button_push(const struct device *dev, struct gpio_callback *cb, uint32_t pins)			//we define the callback function
{
	gpio_pin_toggle_dt(&led);
}

int main(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT | GPIO_ACTIVE_HIGH);					//we turn on the LED from the start
	if (ret < 0) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);									//confiugre button GPIO

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);			//configure interrupt on the button
	if (ret < 0) {
		return 0;
	}

	gpio_init_callback(&button_cb_data, button_push, BIT(button.pin));					//we init the callback function on the desired parameters

	gpio_add_callback(button.port, &button_cb_data);									//we start the interrupt

	while (1) {

		k_msleep(1000);

	}
	return 0;
}*/


int main(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT | GPIO_ACTIVE_LOW);		//we turn on the LED from the start
	
	if (ret < 0) {
		return 0;
	}


	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);			//we keep the existing config values
	if (ret < 0) {
		return 0;
	}

	while (1) {
		uint8_t val;
		val = gpio_pin_get_dt(&button);

		gpio_pin_set_dt(&led, val);

	}
	return 0;
}