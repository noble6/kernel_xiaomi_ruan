// SPDX-License-Identifier: GPL-2.0-only
/*
 * Bring-up aid for ruan: panic on purpose some time after boot.
 *
 * ruan has no serial console, and a hang leaves nothing behind. A panic
 * makes the bootloader save the minidump, kernel log included, to the
 * rawdump partition, where Android can read it after the next boot.
 *
 *   ruan_bringup.panic_after=<seconds>   0 (default) disables it
 *   ruan_bringup.panic_on_recovery=1     panic instead of rebooting into
 *                                        recovery, e.g. when init gives up
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/panic.h>
#include <linux/printk.h>
#include <linux/reboot.h>
#include <linux/string.h>
#include <linux/workqueue.h>

static unsigned int panic_after;
module_param(panic_after, uint, 0644);

static bool panic_on_recovery;
module_param(panic_on_recovery, bool, 0644);

static int ruan_bringup_reboot(struct notifier_block *nb, unsigned long action,
			       void *cmd)
{
	if (panic_on_recovery && cmd && strstr(cmd, "recovery"))
		panic("ruan_bringup: reboot to %s, saving the log", (char *)cmd);
	return NOTIFY_DONE;
}

static struct notifier_block ruan_bringup_reboot_nb = {
	.notifier_call = ruan_bringup_reboot,
	.priority = INT_MAX,
};

static void ruan_bringup_panic(struct work_struct *work)
{
	panic("ruan_bringup: panic_after=%u reached, saving the log", panic_after);
}
static DECLARE_DELAYED_WORK(ruan_bringup_work, ruan_bringup_panic);

static int __init ruan_bringup_init(void)
{
	if (panic_after) {
		pr_info("ruan_bringup: will panic in %u s\n", panic_after);
		schedule_delayed_work(&ruan_bringup_work, panic_after * HZ);
	}
	register_reboot_notifier(&ruan_bringup_reboot_nb);
	return 0;
}
late_initcall(ruan_bringup_init);
