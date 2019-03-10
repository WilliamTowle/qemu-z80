/*
 * Skeleton for minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 * WmT, 2018-2022 [...after Stuart Brady, under GPLv2+...]
 */

#ifndef CPU_Z80_H
#define CPU_Z80_H

#include "config.h"

#define TARGET_LONG_BITS 32

#define CPUState struct CPUZ80State

#include "cpu-defs.h"

typedef struct CPUZ80State {
	CPU_COMMON
} CPUZ80State;

#define TARGET_VIRT_ADDR_SPACE_BITS 32

#include "cpu-all.h"

#endif /* CPU_Z80_H */
