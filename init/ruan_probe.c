// SPDX-License-Identifier: GPL-2.0
/*
 * Boot checkpoints for bring-up without a console: with ruan_probe=N on
 * the command line, the kernel powers the device off (PSCI SYSTEM_OFF)
 * when it reaches checkpoint N. A device that turns off got that far; one
 * that resets before didn't. Needs nothing but the firmware, so it works
 * before any driver or console.
 */
#include <linux/compiler.h>
#include <linux/init.h>
#include <linux/ruan_probe.h>
#include <linux/string.h>
#include <asm/processor.h>

#define PSCI_0_2_FN_SYSTEM_OFF	0x84000008

static int probe_at = -1;

void __init ruan_probe_setup(void)
{
	const char *p = strstr(boot_command_line, "ruan_probe=");

	if (!p)
		return;
	probe_at = 0;
	for (p += strlen("ruan_probe="); *p >= '0' && *p <= '9'; p++)
		probe_at = probe_at * 10 + *p - '0';
}

void ruan_probe(int n)
{
	if (n != probe_at)
		return;

	for (;;) {
		register unsigned long x0 asm("x0") = PSCI_0_2_FN_SYSTEM_OFF;

		asm volatile("smc #0" : "+r" (x0) : : "x1", "x2", "x3", "memory");
		cpu_relax();
	}
}
