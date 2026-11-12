/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _MI_BACKLIGHT_SY7758_H_
#define _MI_BACKLIGHT_SY7758_H_

#include <linux/types.h>

int mi_backlight_sy7758_init(void);
bool sy7758_present(void);
void sy7758_set_panel_name(const char *name);
int sy7758_backlight_update_status(unsigned int level);

#endif
