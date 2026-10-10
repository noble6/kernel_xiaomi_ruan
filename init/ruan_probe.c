// SPDX-License-Identifier: GPL-2.0
/*
 * Boot checkpoints for bring-up without a console: with ruan_probe=N on
 * the command line, the kernel stops at checkpoint N and shows that it
 * got there: it powers off (PSCI SYSTEM_OFF). With ruan_probe=Ng and a
 * checkpoint from 2 to 99, it paints the bootloader's splash buffer,
 * which a video mode panel keeps scanning out, green and spins instead.
 * With ruan_probe=Nf (N >= 2) it reboots to fastboot instead, through
 * the IMEM restart reason, and with ruan_probe=Ne to emergency download
 * mode (USB 05c6:9008). Needs no driver, so it works before any
 * console.
 *
 *    1  setup_arch(), the device tree is parsed
 *  101-109 setup_arch() steps
 *  111-124 start_kernel() steps
 *    2  start_kernel(), before rest_init()
 *    6  kernel_init_freeable() starts
 *    7  before the early initcalls
 *  900+ before the Nth early initcall
 *    8  after the early initcalls
 *    9  after the secondary CPUs are up (smp_init())
 *    3  before the initcalls
 *  1x   before initcall level x
 * 1000+ before the Nth initcall of levels 0-7
 *    4  after the initcalls
 *    5  before running the ramdisk's init
 */
#include <linux/compiler.h>
#include <linux/init.h>
#include <linux/ruan_probe.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/irqflags.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <asm/cacheflush.h>
#include <asm/memory.h>
#include <asm/processor.h>

#define PSCI_0_2_FN_SYSTEM_OFF	0x84000008
#define PSCI_0_2_FN_SYSTEM_RESET	0x84000009

/*
 * IMEM restart reason, which the bootloader reads (msm-poweroff on 5.10
 * wrote it next to the PMIC one).
 */
#define IMEM_RESTART_REASON	0x146aa65cUL
#define RESTART_BOOTLOADER	0x77665500
#define RESTART_RECOVERY	0x77665502

/*
 * Emergency download mode, as msm-poweroff does "reboot edl" on 5.10: set
 * the download mode through TrustZone (or TCSR_BOOT_MISC_DETECT, TCSR +
 * 0x13000, if that call is missing) and reset; the boot ROM then shows up
 * on USB as 05c6:9008, which the PC sees without anyone watching.
 */
#define SCM_BOOT_SET_DLOAD_MODE	0xc2000110	/* SiP fast call, svc 1 cmd 0x10 */
#define SCM_IO_WRITE		0xc2000502	/* SiP fast call, svc 5 cmd 2 */
#define SCM_ARGS_2		2
#define TCSR_BOOT_MISC_DETECT	0x1fd3000UL
#define DLOAD_EDL		1

/* ruan's splash buffer: 1600x2560 a8r8g8b8 */
#define SPLASH_BASE		0xb8000000UL
#define SPLASH_SIZE		(1600 * 2560 * 4)
#define GREEN			0xff00ff00

static int probe_at = -1;
static bool paint, fastboot, edl;

void __init ruan_probe_setup(void)
{
	const char *p = strstr(boot_command_line, "ruan_probe=");

	if (!p)
		return;
	probe_at = 0;
	for (p += strlen("ruan_probe="); *p >= '0' && *p <= '9'; p++)
		probe_at = probe_at * 10 + *p - '0';
	paint = *p == 'g';
	fastboot = *p == 'f';
	edl = *p == 'e';
}

static void psci(unsigned long fn)
{
	for (;;) {
		register unsigned long x0 asm("x0") = fn;

		asm volatile("smc #0" : "+r" (x0) : : "x1", "x2", "x3", "memory");
		cpu_relax();
	}
}

static unsigned long smc4(unsigned long fn, unsigned long a1,
			  unsigned long a2, unsigned long a3)
{
	register unsigned long x0 asm("x0") = fn;
	register unsigned long x1 asm("x1") = a1;
	register unsigned long x2 asm("x2") = a2;
	register unsigned long x3 asm("x3") = a3;

	asm volatile("smc #0" : "+r" (x0), "+r" (x1), "+r" (x2), "+r" (x3)
		     : : "x4", "x5", "x6", "x7", "memory");
	return x0;
}

static void reboot_edl(void)
{
	if (smc4(SCM_BOOT_SET_DLOAD_MODE, SCM_ARGS_2, DLOAD_EDL, 0))
		smc4(SCM_IO_WRITE, SCM_ARGS_2, TCSR_BOOT_MISC_DETECT, DLOAD_EDL);
	psci(PSCI_0_2_FN_SYSTEM_RESET);
}

static void set_restart_reason(u32 reason)
{
	void __iomem *r = ioremap(IMEM_RESTART_REASON, 4);

	if (r) {
		writel(reason, r);
		iounmap(r);
	}
}

static void paint_green(void)
{
	u32 *fb = (u32 *)__va(SPLASH_BASE);
	unsigned long i;

	for (i = 0; i < SPLASH_SIZE / 4; i++)
		fb[i] = GREEN;
	dcache_clean_poc((unsigned long)fb, (unsigned long)fb + SPLASH_SIZE);
}

void ruan_probe(int n)
{
	if (n != probe_at)
		return;

	if (edl)
		reboot_edl();

	/* from checkpoint 2 on, ioremap() works: reboot to fastboot */
	if (n >= 2 && fastboot) {
		set_restart_reason(RESTART_BOOTLOADER);
		psci(PSCI_0_2_FN_SYSTEM_RESET);
	}

	if (n >= 2 && n < 100 && paint) {
		local_irq_disable();
		paint_green();
		for (;;)
			cpu_relax();
	}

	psci(PSCI_0_2_FN_SYSTEM_OFF);
}

/*
 * "reboot bootloader" / "reboot recovery" without the PMIC drivers: also
 * set the IMEM restart reason.
 */
static int ruan_reboot(struct notifier_block *nb, unsigned long action, void *cmd)
{
	if (action == SYS_RESTART && cmd) {
		if (!strcmp(cmd, "bootloader"))
			set_restart_reason(RESTART_BOOTLOADER);
		else if (!strcmp(cmd, "recovery"))
			set_restart_reason(RESTART_RECOVERY);
		else if (!strcmp(cmd, "edl"))
			reboot_edl();
	}
	return NOTIFY_DONE;
}

static struct notifier_block ruan_reboot_nb = {
	.notifier_call = ruan_reboot,
};

static int __init ruan_reboot_init(void)
{
	return register_reboot_notifier(&ruan_reboot_nb);
}
late_initcall(ruan_reboot_init);
