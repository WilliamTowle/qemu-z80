/*
 * Z80 virtual CPU header
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA  02110-1301 USA
 */
#ifndef CPU_Z80_H
#define CPU_Z80_H

#include "config.h"

#define TARGET_LONG_BITS 32

/* Z80 registers */

#define R_A     0
#define R_F     1

#define R_BC    2
#define R_DE    3
#define R_HL    4
#define R_IX    5
#define R_IY    6
#define R_SP    7

#define R_I     8
#define R_R     9

#define R_AX    10
#define R_FX    11
#define R_BCX   12
#define R_DEX   13
#define R_HLX   14

#define CPU_NB_REGS 15

#define CPUState struct CPUZ80State

#include "cpu-defs.h"

typedef struct CPUZ80State {
    CPU_COMMON

    /* Z80 registers */
    uint16_t pc;

    target_ulong regs[CPU_NB_REGS];

    int iff1;
    int iff2;
    int imode;
} CPUZ80State;

CPUZ80State *cpu_z80_init(const char *cpu_model);
int cpu_z80_exec(CPUZ80State *s);

#define TARGET_VIRT_ADDR_SPACE_BITS 32

#define cpu_init cpu_z80_init
#define cpu_exec cpu_z80_exec

#include "cpu-all.h"

static inline bool cpu_has_work(CPUState *env)
{
#if 1	/* temp'y */
#if defined(TARGET_Z80)
;printf("%s(): PARTIAL - assume '1' result OK\n", __func__);
#endif
	return 1;
#else	/* as implemented for i386 */
    return ((env->interrupt_request & CPU_INTERRUPT_HARD) &&
            (env->eflags & IF_MASK)) ||
           (env->interrupt_request & (CPU_INTERRUPT_NMI |
                                      CPU_INTERRUPT_INIT |
                                      CPU_INTERRUPT_SIPI |
                                      CPU_INTERRUPT_MCE));
#endif
}

#include "exec-all.h"

#endif /* CPU_Z80_H */
