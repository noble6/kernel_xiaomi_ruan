/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_RUAN_PROBE_H
#define _LINUX_RUAN_PROBE_H

#ifdef CONFIG_RUAN_BOOT_PROBE
void ruan_probe_setup(void);
void ruan_probe(int n);
#else
static inline void ruan_probe_setup(void) { }
static inline void ruan_probe(int n) { }
#endif

#endif
