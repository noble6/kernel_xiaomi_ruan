/* SPDX-License-Identifier: GPL-2.0-only */
/* Stand-in for Xiaomi's unpublished MCA charger logging. */
#ifndef __MCA_LOG_H__
#define __MCA_LOG_H__

#include <linux/printk.h>

#define mca_log_err(fmt, ...)	pr_err(fmt, ##__VA_ARGS__)
#define mca_log_info(fmt, ...)	pr_info(fmt, ##__VA_ARGS__)
#define mca_log_debug(fmt, ...)	pr_debug(fmt, ##__VA_ARGS__)

#endif
