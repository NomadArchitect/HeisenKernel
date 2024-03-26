/*
 * Poweroff & reset driver for Actions Semi ATC260x PMICs
 *
 * Copyright (c) 2020 Cristian Ciocaltea <cristian.ciocaltea@gmail.com>
 */

#include <heisen/delay.h>
#include <heisen/mfd/atc260x/core.h>
#include <heisen/module.h>
#include <heisen/platform_device.h>
#include <heisen/power_supply.h>
#include <heisen/reboot.h>
#include <heisen/regmap.h>

struct atc260x_pwrc {
	struct device *dev;
	struct regmap *regmap;
	int (*do_poweroff)(const struct atc260x_pwrc *pwrc, bool restart);
};

static int atc2603c_do_poweroff(const struct atc260x_pwrc *pwrc, bool restart)
{
	int ret, deep_sleep = 0;
	uint reg_mask, reg_val;

	/* S4-Deep Sleep Mode is NOT available for WALL/USB power */
	if (!restart && !power_supply_is_system_supplied()) {
		deep_sleep = 1;
		dev_info(pwrc->dev, "Enabling S4-Deep Sleep Mode");
	}

	/* Update wakeup sources */
	reg_val = ATC2603C_PMU_SYS_CTL0_ONOFF_LONG_WK_EN |
		  (restart ? ATC2603C_PMU_SYS_CTL0_RESET_WK_EN
			   : ATC2603C_PMU_SYS_CTL0_ONOFF_SHORT_WK_EN);

	ret = regmap_update_bits(pwrc->regmap, ATC2603C_PMU_SYS_CTL0,
				 ATC2603C_PMU_SYS_CTL0_WK_ALL, reg_val);
	if (ret)
		dev_warn(pwrc->dev, "failed to write SYS_CTL0: %d\n", ret);

	/* Update power mode */
	reg_mask = ATC2603C_PMU_SYS_CTL3_EN_S2 | ATC2603C_PMU_SYS_CTL3_EN_S3;

	ret = regmap_update_bits(pwrc->regmap, ATC2603C_PMU_SYS_CTL3, reg_mask,
				 deep_sleep ? 0 : ATC2603C_PMU_SYS_CTL3_EN_S3);
	if (ret) {
		dev_err(pwrc->dev, "failed to write SYS_CTL3: %d\n", ret);
		return ret;
	}

	/* Trigger poweroff / restart sequence */
	reg_mask = restart ? ATC2603C_PMU_SYS_CTL0_RESTART_EN
			   : ATC2603C_PMU_SYS_CTL1_EN_S1;
	reg_val = restart ? ATC2603C_PMU_SYS_CTL0_RESTART_EN : 0;

	ret = regmap_update_bits(pwrc->regmap,
				 restart ? ATC2603C_PMU_SYS_CTL0 : ATC2603C_PMU_SYS_CTL1,
				 reg_mask, reg_val);
	if (ret) {
		dev_err(pwrc->dev, "failed to write SYS_CTL%d: %d\n",
			restart ? 0 : 1, ret);
		return ret;
	}

	/* Wait for trigger completion */
	mdelay(200);

	return 0;
}

static int atc2609a_do_poweroff(const struct atc260x_pwrc *pwrc, bool restart)
{
	int ret, deep_sleep = 0;
	uint reg_mask, reg_val;

	/* S4-Deep Sleep Mode is NOT available for WALL/USB power */
	if (!restart && !power_supply_is_system_supplied()) {
		deep_sleep = 1;
		dev_info(pwrc->dev, "Enabling S4-Deep Sleep Mode");
	}

	/* Update wakeup sources */
	reg_val = ATC2609A_PMU_SYS_CTL0_ONOFF_LONG_WK_EN |
		  (restart ? ATC2609A_PMU_SYS_CTL0_RESET_WK_EN
			   : ATC2609A_PMU_SYS_CTL0_ONOFF_SHORT_WK_EN);

	ret = regmap_update_bits(pwrc->regmap, ATC2609A_PMU_SYS_CTL0,
				 ATC2609A_PMU_SYS_CTL0_WK_ALL, reg_val);
	if (ret)
		dev_warn(pwrc->dev, "failed to write SYS_CTL0: %d\n", ret);

	/* Update power mode */
	reg_mask = ATC2609A_PMU_SYS_CTL3_EN_S2 | ATC2609A_PMU_SYS_CTL3_EN_S3;

	ret = regmap_update_bits(pwrc->regmap, ATC2609A_PMU_SYS_CTL3, reg_mask,
				 deep_sleep ? 0 : ATC2609A_PMU_SYS_CTL3_EN_S3);
	if (ret) {
		dev_err(pwrc->dev, "failed to write SYS_CTL3: %d\n", ret);
		return ret;
	}

	/* Trigger poweroff / restart sequence */
	reg_mask = restart ? ATC2609A_PMU_SYS_CTL0_RESTART_EN
			   : ATC2609A_PMU_SYS_CTL1_EN_S1;
	reg_val = restart ? ATC2609A_PMU_SYS_CTL0_RESTART_EN : 0;

	ret = regmap_update_bits(pwrc->regmap,
				 restart ? ATC2609A_PMU_SYS_CTL0 : ATC2609A_PMU_SYS_CTL1,
				 reg_mask, reg_val);
	if (ret) {
		dev_err(pwrc->dev, "failed to write SYS_CTL%d: %d\n",
			restart ? 0 : 1, ret);
		return ret;
	}

	/* Wait for trigger completion */
	mdelay(200);

	return 0;
}

static int atc2603c_init(const struct atc260x_pwrc *pwrc)
{
	int ret;

	/*
	 * Delay transition from S2/S3 to S1 in order to avoid
	 * DDR init failure in Bootloader.
	 */
	ret = regmap_update_bits(pwrc->regmap, ATC2603C_PMU_SYS_CTL3,
				 ATC2603C_PMU_SYS_CTL3_S2S3TOS1_TIMER_EN,
				 ATC2603C_PMU_SYS_CTL3_S2S3TOS1_TIMER_EN);
	if (ret)
		dev_warn(pwrc->dev, "failed to write SYS_CTL3: %d\n", ret);

	/* Set wakeup sources */
	ret = regmap_update_bits(pwrc->regmap, ATC2603C_PMU_SYS_CTL0,
				 ATC2603C_PMU_SYS_CTL0_WK_ALL,
				 ATC2603C_PMU_SYS_CTL0_HDSW_WK_EN |
				 ATC2603C_PMU_SYS_CTL0_ONOFF_LONG_WK_EN);
	if (ret)
		dev_warn(pwrc->dev, "failed to write SYS_CTL0: %d\n", ret);

	return ret;
}

static int atc2609a_init(const struct atc260x_pwrc *pwrc)
{
	int ret;

	/* Set wakeup sources */
	ret = regmap_update_bits(pwrc->regmap, ATC2609A_PMU_SYS_CTL0,
				 ATC2609A_PMU_SYS_CTL0_WK_ALL,
				 ATC2609A_PMU_SYS_CTL0_HDSW_WK_EN |
				 ATC2609A_PMU_SYS_CTL0_ONOFF_LONG_WK_EN);
	if (ret)
		dev_warn(pwrc->dev, "failed to write SYS_CTL0: %d\n", ret);

	return ret;
}

static int atc260x_pwrc_pm_handler(struct sys_off_data *data)
{
	struct atc260x_pwrc *pwrc = data->cb_data;

	pwrc->do_poweroff(pwrc, false);

	WARN_ONCE(1, "Unable to power off system\n");

	return NOTIFY_DONE;
}

static int atc260x_pwrc_restart_handler(struct sys_off_data *data)
{
	struct atc260x_pwrc *pwrc = data->cb_data;
	pwrc->do_poweroff(pwrc, true);

	return NOTIFY_DONE;
}

static int atc260x_pwrc_probe(struct platform_device *pdev)
{
	struct atc260x *atc260x = dev_get_drvdata(pdev->dev.parent);
	struct atc260x_pwrc *priv;
	int ret;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = &pdev->dev;
	priv->regmap = atc260x->regmap;

	switch (atc260x->ic_type) {
	case ATC2603C:
		priv->do_poweroff = atc2603c_do_poweroff;
		ret = atc2603c_init(priv);
		break;
	case ATC2609A:
		priv->do_poweroff = atc2609a_do_poweroff;
		ret = atc2609a_init(priv);
		break;
	default:
		dev_err(priv->dev,
			"Poweroff not supported for ATC260x PMIC type: %u\n",
			atc260x->ic_type);
		return -EINVAL;
	}

	if (ret)
		return ret;

	ret = devm_register_sys_off_handler(priv->dev,
					    SYS_OFF_MODE_POWER_OFF,
					    SYS_OFF_PRIO_DEFAULT,
					    atc260x_pwrc_pm_handler,
					    priv);
	if (ret)
		dev_err(priv->dev, "failed to register power-off handler: %d\n",
			ret);

	ret = devm_register_sys_off_handler(priv->dev,
					    SYS_OFF_MODE_RESTART,
					    SYS_OFF_PRIO_HIGH,
					    atc260x_pwrc_restart_handler,
					    priv);
	if (ret)
		dev_err(priv->dev, "failed to register restart handler: %d\n",
			ret);

	return ret;
}

static struct platform_driver atc260x_pwrc_driver = {
	.probe = atc260x_pwrc_probe,
	.driver = {
		.name = "atc260x-pwrc",
	},
};

module_platform_driver(atc260x_pwrc_driver);

MODULE_DESCRIPTION("Poweroff & reset driver for ATC260x PMICs");
MODULE_AUTHOR("Cristian Ciocaltea <cristian.ciocaltea@gmail.com>");
MODULE_LICENSE("GPL");