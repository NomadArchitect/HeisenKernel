/*
 * Toggles a GPIO pin to restart a device
 *
 * Copyright (C) 2014 Google, Inc.
 *
 * Based on the gpio-poweroff driver.
 */
#include <heisen/reboot.h>
#include <heisen/kernel.h>
#include <heisen/init.h>
#include <heisen/delay.h>
#include <heisen/platform_device.h>
#include <heisen/gpio/consumer.h>
#include <heisen/module.h>
#include <heisen/of.h>

struct gpio_restart {
	struct gpio_desc *reset_gpio;
	uint32_t active_delay_ms;
	uint32_t inactive_delay_ms;
	uint32_t wait_delay_ms;
};

static int gpio_restart_notify(struct sys_off_data *data)
{
	struct gpio_restart *gpio_restart = data->cb_data;

	/* drive it active, also inactive->active edge */
	gpiod_direction_output(gpio_restart->reset_gpio, 1);
	mdelay(gpio_restart->active_delay_ms);

	/* drive inactive, also active->inactive edge */
	gpiod_set_value(gpio_restart->reset_gpio, 0);
	mdelay(gpio_restart->inactive_delay_ms);

	/* drive it active, also inactive->active edge */
	gpiod_set_value(gpio_restart->reset_gpio, 1);

	/* give it some time */
	mdelay(gpio_restart->wait_delay_ms);

	WARN_ON(1);

	return NOTIFY_DONE;
}

static int gpio_restart_probe(struct platform_device *pdev)
{
	struct gpio_restart *gpio_restart;
	bool open_source = false;
	int priority = 129;
	uint32_t property;
	int ret;

	gpio_restart = devm_kzalloc(&pdev->dev, sizeof(*gpio_restart),
			GFP_KERNEL);
	if (!gpio_restart)
		return -ENOMEM;

	open_source = of_property_read_bool(pdev->dev.of_node, "open-source");

	gpio_restart->reset_gpio = devm_gpiod_get(&pdev->dev, NULL,
			open_source ? GPIOD_IN : GPIOD_OUT_LOW);
	ret = PTR_ERR_OR_ZERO(gpio_restart->reset_gpio);
	if (ret) {
		if (ret != -EPROBE_DEFER)
			dev_err(&pdev->dev, "Could not get reset GPIO\n");
		return ret;
	}

	gpio_restart->active_delay_ms = 100;
	gpio_restart->inactive_delay_ms = 100;
	gpio_restart->wait_delay_ms = 3000;

	ret = of_property_read_u32(pdev->dev.of_node, "priority", &property);
	if (!ret) {
		if (property > 255)
			dev_err(&pdev->dev, "Invalid priority property: %u\n",
					property);
		else
			priority = property;
	}

	of_property_read_u32(pdev->dev.of_node, "active-delay",
			&gpio_restart->active_delay_ms);
	of_property_read_u32(pdev->dev.of_node, "inactive-delay",
			&gpio_restart->inactive_delay_ms);
	of_property_read_u32(pdev->dev.of_node, "wait-delay",
			&gpio_restart->wait_delay_ms);

	ret = devm_register_sys_off_handler(&pdev->dev,
					    SYS_OFF_MODE_RESTART,
					    priority,
					    gpio_restart_notify,
					    gpio_restart);
	if (ret) {
		dev_err(&pdev->dev, "%s: cannot register restart handler, %d\n",
				__func__, ret);
		return -ENODEV;
	}

	return 0;
}

static const struct of_device_id of_gpio_restart_match[] = {
	{ .compatible = "gpio-restart", },
	{},
};

static struct platform_driver gpio_restart_driver = {
	.probe = gpio_restart_probe,
	.driver = {
		.name = "restart-gpio",
		.of_match_table = of_gpio_restart_match,
	},
};

module_platform_driver(gpio_restart_driver);

MODULE_AUTHOR("David Riley <davidriley@chromium.org>");
MODULE_DESCRIPTION("GPIO restart driver");