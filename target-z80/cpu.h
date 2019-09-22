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

//#define EXCP00_DIVZ	0
//#define EXCP01_SSTP	1
//#define EXCP02_NMI	2
//#define EXCP03_INT3	3
//#define EXCP04_INTO	4
//#define EXCP05_BOUND	5
#define EXCP06_ILLOP	6
//#define EXCP07_PREX	7
//#define EXCP08_DBLE	8
//#define EXCP09_XERR	9
//#define EXCP0A_TSS	10
//#define EXCP0B_NOSEG	11
//#define EXCP0C_STACK	12
//#define EXCP0D_GPF	13
//#define EXCP0E_PAGE	14
//#define EXCP10_COPR	16
//#define EXCP11_ALGN	17
//#define EXCP12_MCHK	18
#define EXCP_KERNEL_TRAP	19

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

    /* emulator internal eflags handling */
    uint32_t hflags; /* hidden flags, see HF_xxx constants */
} CPUZ80State;

CPUZ80State *cpu_z80_init(const char *cpu_model);
int cpu_z80_exec(CPUZ80State *s);

struct siginfo;
int cpu_z80_signal_handler(int host_signum, void *pinfo,
                           void *puc);

#define TARGET_PAGE_BITS 12

#define TARGET_PHYS_ADDR_SPACE_BITS 32
#define TARGET_VIRT_ADDR_SPACE_BITS 32

/* helper.c */
int cpu_z80_handle_mmu_fault(CPUZ80State *env, target_ulong addr,
                             int is_write1, int mmu_idx, int is_softmmu);
#define cpu_handle_mmu_fault cpu_z80_handle_mmu_fault

#define cpu_init cpu_z80_init
#define cpu_exec cpu_z80_exec
#define cpu_signal_handler cpu_z80_signal_handler

#define MMU_USER_IDX 1

#include "cpu-all.h"

static inline bool cpu_has_work(CPUState *env)
{
    /* see i386? has env->eflags bit relating to IF_MASK; NMIs (and
     * others) also cause a 'true' result
     */
    return env->interrupt_request & CPU_INTERRUPT_HARD;
}

#include "exec-all.h"

static inline void cpu_pc_from_tb(CPUState *env, TranslationBlock *tb)
{
    env->pc = tb->pc;
    env->hflags = tb->flags;
}

static inline void cpu_get_tb_cpu_state(CPUState *env, target_ulong *pc,
                                        target_ulong *cs_base, int *flags)
{
    *pc = env->pc;
    *cs_base = 0;
    *flags = env->hflags;
}

#endif /* CPU_Z80_H */
