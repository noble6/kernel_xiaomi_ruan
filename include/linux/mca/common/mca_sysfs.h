/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Stand-in for Xiaomi's unpublished MCA sysfs helper. The attribute tables
 * still compile, but nothing gets registered.
 */
#ifndef __MCA_SYSFS_H__
#define __MCA_SYSFS_H__

#include <linux/device.h>
#include <linux/string.h>

enum { SYSFS_DEV_1 = 1, SYSFS_DEV_2, SYSFS_DEV_3, SYSFS_DEV_4, SYSFS_DEV_5 };

struct mca_sysfs_attr_info {
	struct device_attribute attr;
	int sysfs_attr_name;
};

#define mca_sysfs_attr_ro(_func, _mode, _type, _name) \
	{ .attr = __ATTR(_name, _mode, _func##_show, NULL), .sysfs_attr_name = _type }
#define mca_sysfs_attr_rw(_func, _mode, _type, _name) \
	{ .attr = __ATTR(_name, _mode, _func##_show, _func##_store), .sysfs_attr_name = _type }
#define mca_sysfs_attr_wo(_func, _mode, _type, _name) \
	{ .attr = __ATTR(_name, _mode, NULL, _func##_store), .sysfs_attr_name = _type }

static inline struct mca_sysfs_attr_info *
mca_sysfs_lookup_attr(const char *name, struct mca_sysfs_attr_info *tbl, int size)
{
	int i;

	for (i = 0; i < size; i++)
		if (!strcmp(name, tbl[i].attr.attr.name))
			return &tbl[i];
	return NULL;
}

static inline int mca_sysfs_create_files(int dev, struct mca_sysfs_attr_info *tbl, int size)
{
	return 0;
}

#endif
