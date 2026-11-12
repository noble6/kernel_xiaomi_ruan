// SPDX-License-Identifier: GPL-2.0-only
/*
 * Backlight for ruan (Redmi Pad Pro 5G / POCO Pad 5G): two SY7758 LED
 * drivers, one per I2C bus, or the SGM37604A some units carry instead.
 *
 * Xiaomi never published this driver; on 5.10 it is built into their
 * msm_drm module (mi_backlight_sy7758). The register sequence below is
 * the one that module uses. The chip is told apart by register 0x03:
 * 0x63 is the SY7758, 0x30 the SGM37604A.
 */

#define pr_fmt(fmt)	"sy7758:[%s:%d] " fmt, __func__, __LINE__

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/regmap.h>
#include <linux/string.h>

#include "mi_backlight_sy7758.h"

#define SY7758_REG_ID		0x03
#define SY7758_ID		0x63
#define SGM37604A_ID		0x30

struct sy7758_led {
	struct list_head entry;
	struct regmap *regmap;
	struct device *dev;
	bool is_sy7758;
	bool on;
	u32 level;
};

static LIST_HEAD(sy7758_dev_list);
static DEFINE_MUTEX(sy7758_dev_list_mutex);
static const char *sy7758_panel_name;
/* last non-zero panel level, restored when the panel powers back on */
static unsigned int sy7758_last_level;

static const struct regmap_config sy7758_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0xff,
};

static void sy7758_write(struct sy7758_led *sy, u8 reg, u8 val)
{
	int ret = regmap_write(sy->regmap, reg, val);

	if (ret)
		dev_err(sy->dev, "write 0x%02x = 0x%02x failed: %d\n", reg, val, ret);
}

static void sy7758_set_level(struct sy7758_led *sy, u32 lvl)
{
	if (sy->is_sy7758) {
		sy7758_write(sy, 0x10, lvl & 0xff);
		sy7758_write(sy, 0x11, lvl >> 8);
	} else {
		sy7758_write(sy, 0x1a, lvl & 0xf);
		sy7758_write(sy, 0x19, lvl >> 4);
	}
}

static void sy7758_enable(struct sy7758_led *sy, u32 lvl)
{
	dev_dbg(sy->dev, "enable, level 0x%03x\n", lvl);
	if (sy->is_sy7758) {
		sy7758_write(sy, 0x01, 0x83);
		sy7758_write(sy, 0xa2, 0x20);
		sy7758_write(sy, 0xa1, 0xef);
		sy7758_write(sy, 0x16, 0x0f);
		sy7758_set_level(sy, lvl);
		sy7758_write(sy, 0xa4, 0x02);
		sy7758_write(sy, 0xa2, 0x28);
	} else {
		sy7758_write(sy, 0x11, 0x65);
		sy7758_write(sy, 0x1b, 0x01);
		sy7758_set_level(sy, lvl);
	}
	sy->on = true;
}

/*
 * The chips lose their settings when the panel rails go down (doze -> off,
 * suspend) without a zero level being sent first. Check the enable
 * register instead of trusting our own state.
 */
static bool sy7758_is_enabled(struct sy7758_led *sy)
{
	unsigned int val = 0;

	if (regmap_read(sy->regmap, sy->is_sy7758 ? 0x01 : 0x1b, &val)) {
		dev_dbg(sy->dev, "enable reg read failed\n");
		return false;
	}
	dev_dbg(sy->dev, "enable reg 0x%02x\n", val);
	return sy->is_sy7758 ? val == 0x83 : val == 0x01;
}

static void sy7758_disable(struct sy7758_led *sy)
{
	sy7758_set_level(sy, 0);
	if (!sy->is_sy7758)
		usleep_range(10000, 10010);
	sy->on = false;
}

/* Panel level (0..4095) to chip level, as Xiaomi scales it per panel */
static u32 sy7758_map_level(u32 level)
{
	const char *name = sy7758_panel_name ? sy7758_panel_name : "";

	if (strstr(name, "n83 35 02 0a cphy nt36532 boe"))
		return level > 0x9e8 ? 0xeee : level * 1507 / 1000;
	if (strstr(name, "n83 42 02 0b cphy nt36532 csot"))
		return level > 0x9a5 ? 0xd9b : level * 141 / 100;
	return level;
}

void sy7758_set_panel_name(const char *name)
{
	sy7758_panel_name = name;
}

bool sy7758_present(void)
{
	bool present;

	mutex_lock(&sy7758_dev_list_mutex);
	present = !list_empty(&sy7758_dev_list);
	mutex_unlock(&sy7758_dev_list_mutex);
	return present;
}

int sy7758_backlight_update_status(unsigned int level)
{
	struct sy7758_led *sy;
	u32 lvl = sy7758_map_level(level);

	if (lvl > 0xfff)
		return -EINVAL;

	if (level)
		sy7758_last_level = level;

	mutex_lock(&sy7758_dev_list_mutex);
	list_for_each_entry(sy, &sy7758_dev_list, entry) {
		if (!lvl)
			sy7758_disable(sy);
		else if (!sy->on || !sy7758_is_enabled(sy))
			sy7758_enable(sy, lvl);
		else
			sy7758_set_level(sy, lvl);
		sy->level = lvl;
	}
	mutex_unlock(&sy7758_dev_list_mutex);

	pr_debug("level %u -> 0x%03x\n", level, lvl);
	return 0;
}

/*
 * Panel power events. The chips are reset when the panel rails go down,
 * and nothing sends a level on the way back up if Android's brightness
 * didn't change, so restore the last one ourselves.
 */
void sy7758_panel_power(bool on)
{
	struct sy7758_led *sy;

	pr_debug("panel power %s, last level %u\n", on ? "on" : "off", sy7758_last_level);
	if (on) {
		if (sy7758_last_level)
			sy7758_backlight_update_status(sy7758_last_level);
		return;
	}

	mutex_lock(&sy7758_dev_list_mutex);
	list_for_each_entry(sy, &sy7758_dev_list, entry) {
		sy7758_disable(sy);
		sy->level = 0;
	}
	mutex_unlock(&sy7758_dev_list_mutex);
}

static int sy7758_probe(struct i2c_client *i2c)
{
	struct sy7758_led *sy;
	unsigned int id = 0;
	int ret;

	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	sy = devm_kzalloc(&i2c->dev, sizeof(*sy), GFP_KERNEL);
	if (!sy)
		return -ENOMEM;

	sy->dev = &i2c->dev;
	sy->regmap = devm_regmap_init_i2c(i2c, &sy7758_regmap_config);
	if (IS_ERR(sy->regmap))
		return PTR_ERR(sy->regmap);

	ret = regmap_read(sy->regmap, SY7758_REG_ID, &id);
	if (ret) {
		dev_err(&i2c->dev, "no chip at 0x%02x: %d\n", i2c->addr, ret);
		return ret;
	}
	sy->is_sy7758 = (id == SY7758_ID);
	if (!sy->is_sy7758 && id != SGM37604A_ID)
		dev_warn(&i2c->dev, "unknown id 0x%02x, treating as SGM37604A\n", id);

	/* The bootloader leaves the backlight on */
	sy->on = true;

	mutex_lock(&sy7758_dev_list_mutex);
	list_add_tail(&sy->entry, &sy7758_dev_list);
	mutex_unlock(&sy7758_dev_list_mutex);

	dev_info(&i2c->dev, "%s backlight\n", sy->is_sy7758 ? "SY7758" : "SGM37604A");
	return 0;
}

static void sy7758_remove(struct i2c_client *i2c)
{
}

static const struct of_device_id sy7758_match_table[] = {
	{ .compatible = "sy,sy7758" },
	{ },
};

static struct i2c_driver sy7758_driver = {
	.driver = {
		.name = "sy7758",
		.of_match_table = sy7758_match_table,
	},
	.probe = sy7758_probe,
	.remove = sy7758_remove,
};

int mi_backlight_sy7758_init(void)
{
	return i2c_add_driver(&sy7758_driver);
}
