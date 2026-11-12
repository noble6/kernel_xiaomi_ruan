/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Xiaomi's hwid driver is not published. Drivers in this tree that ask it
 * for the board revision get "unknown" from these stubs, which makes them
 * take their generic path.
 */
#ifndef __HWID_H__
#define __HWID_H__

#include <linux/types.h>

enum {
	HARDWARE_PROJECT_UNKNOWN = 0,
	HARDWARE_PROJECT_A,
	HARDWARE_PROJECT_B,
	HARDWARE_PROJECT_C,
	HARDWARE_PROJECT_D,
	HARDWARE_PROJECT_E,
	HARDWARE_PROJECT_F,
	HARDWARE_PROJECT_O1,
	HARDWARE_PROJECT_O3,
	HARDWARE_PROJECT_O8,
	HARDWARE_PROJECT_O9,
	HARDWARE_PROJECT_P11,
};

enum {
	CountryCN = 0,
	CountryGlobal,
	CountryIndia,
	CountryJapan,
	CountryEEA,
};

static inline uint32_t get_hw_version_platform(void) { return HARDWARE_PROJECT_UNKNOWN; }
static inline uint32_t get_hw_country_version(void) { return 0; }
static inline uint32_t get_hw_version_major(void) { return 0; }
static inline uint32_t get_hw_version_minor(void) { return 0; }
static inline uint32_t get_hw_version_build(void) { return 0; }
static inline uint32_t get_hw_id_value(void) { return 0; }
static inline int get_hw_project_adc(void) { return 0; }
static inline int get_hw_build_adc(void) { return 0; }

#endif
